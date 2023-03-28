#include "compiler.h"
#include <stdexcept>
#include <QSet>
#include <QMap>
#include <QDebug>
#include <QElapsedTimer>

using std::runtime_error;

Compiler::Compiler (const Blueprint *bp, QObject *parent) :
    QObject(parent)
{
    compileBlueprint(bp);
}

void Compiler::compileBlueprint (const Blueprint *bp) {

    QElapsedTimer timer;
    timer.start();

    const std::function<int(QVector<int>&,int)> find = [&find] (QVector<int> &ds, int a) {
        if (ds[a] != a) {
            ds[a] = find(ds, ds[a]); // path compression
            return ds[a];
        } else {
            return a;
        }
    };

    const auto unite = [&find] (QVector<int> &ds, int a, int b) {
        int pa = find(ds, a);
        int pb = find(ds, b);
        if (pa != pb)
            ds[pb] = pa; // naive unbalanced
    };

    const int width = bp->width();
    const int height = bp->height();

    const auto index = [&width] (int x, int y) {
        return y * width + x;
    };

    // --- initialize

    // translate qcolors to compiler component ids
    QImage bpImage = bp->layer(Blueprint::Logic);
    QVector<QVector<Component> > logic(height);
    for (int y = 0; y < height; ++ y) {
        logic[y] = QVector<Component>(width);
        for (int x = 0; x < width; ++ x)
            logic[y][x] = Comp(bp->get(x, y));
    }

    // initially, every pixel has a unique id
    QVector<int> comps(width * height);
    for (int k = 0; k < comps.size(); ++ k)
        comps[k] = k;

    // --- pass: merge all pixels into components

    // in first pass, make note of inks that touch busses/tunnels/meshes and also r/w
    using Conn = QPair<QPoint,QPoint>; // pair(bus/tunnel/mesh, touching ink)
    using Conns = QVector<Conn>;
    Conns busConns, tunnelConns, meshConns, readConns, writeConns;

    // also make note of global roots so we can unify them all into single components
    int wirelessRoot[4] = { -1, -1, -1, -1 }, meshRoot = -1;

    const auto addConn = [] (Component p, Component n, QPoint qp, QPoint qn, Conns &conns, auto f) {
        if (!IsEmpty(n) && !f(n) && !IsCross(n) && f(p)) conns.append({qp, qn});
        if (!IsEmpty(p) && !f(p) && !IsCross(p) && f(n)) conns.append({qn, qp});
    };

    const auto checkPass1 = [&] (int px, int py, int nx, int ny) {
        // merge pixels into an entity
        Component p = logic[py][px], n = logic[ny][nx];
        if (Same(p, n)) unite(comps, index(px, py), index(nx, ny));
        // note bus/tunnel/mesh connections
        QPoint qp(px, py), qn(nx, ny);
        addConn(p, n, qp, qn, busConns, IsBus);
        addConn(p, n, qp, qn, tunnelConns, IsTunnel);
        addConn(p, n, qp, qn, meshConns, IsMesh);
        addConn(p, n, qp, qn, readConns, IsRead);
        addConn(p, n, qp, qn, writeConns, IsWrite);
    };

    const auto outside = [] (int v, int hi) { // outside range [0, hi)
        return v < 0 || v >= hi;
    };

    const auto uniteCross = [&] (int ax, int ay, int bx, int by) {
        if (!(outside(ax, width) || outside(ay, height) || outside(bx, width) || outside(by, height))) {
            Component a = logic[ay][ax], b = logic[by][bx];
            if (Same(a, b)) unite(comps, index(ax, ay), index(bx, by));
        }
    };

    // build initial connected components
    for (int y = 0; y < height; ++ y) {
        for (int x = 0; x < width; ++ x) {
            // merge neighbors
            if (x < width - 1) checkPass1(x, y, x+1, y);
            if (y < height - 1) checkPass1(x, y, x, y+1);
            // merge across crosses
            if (IsCross(logic[y][x])) {
                uniteCross(x-1, y, x+1, y);
                uniteCross(x, y-1, x, y+1);
            }
            // merge all global components
            Component p = logic[y][x];
            if (IsWifi(p)) {
                int channel = WirelessIndex(p);
                if (wirelessRoot[channel] == -1)
                    wirelessRoot[channel] = index(x, y);
                else
                    unite(comps, index(x, y), wirelessRoot[channel]);
            } else if (IsMesh(p)) {
                if (meshRoot == -1)
                    meshRoot = index(x, y);
                else
                    unite(comps, index(x, y), meshRoot);
            }
        }
    }

    // flatten everything, it'll make the rest of the code a bit simpler and shouldn't
    // take too long. if it does take too long, performance can be improved by a) *not*
    // flattening everything and b) keeping the tree balanced in unite().
    for (int k = 0; k < comps.size(); ++ k)
        comps[k] = find(comps, comps[k]);

    // --- tunnel, mesh, bus

    const auto indexq = [&] (const QPoint &p) {
        return index(p.x(), p.y());
    };

    const auto findp = [&] (const QPoint &p) {
        return find(comps, indexq(p));
    };  

    const auto type = [&] (int compid) {
        int x = compid % width;
        int y = compid / width;
        return logic[y][x];
    };

    const auto uniteGroup = [&] (const QVector<int> &group) {
        for (int k = 1; k < group.size(); ++ k)
            unite(comps, group[0], group[k]);
    };

    const auto uniteGroupByType = [&] (const QSet<int> &group) {
        QMap<Component,QVector<int> > bytype;
        for (int id : group)
            bytype[type(id)].append(id);
        for (const QVector<int> &typegroup : bytype)
            uniteGroup(typegroup);
    };

    using CompConns = QMap<int,QSet<int> >; // ID => adjacent IDs

    // tunnels
    // note: each tunnel will be processed twice, once for each endpoint. while this
    // is a minor performance hit, allowing it greatly simplifies the code and won't
    // cause any issues.
    {
        for (const Conn &conn : tunnelConns) {
            QPoint tp = conn.first;
            QPoint pp = conn.second;
            Component startp = logic[pp.y()][pp.x()];
            if (IsMesh(startp)) continue; // meshes don't go through tunnels
            int dx = tp.x() - pp.x(), dy = tp.y() - pp.y();
            assert(dx >= -1 && dx <= 1);
            assert(dy >= -1 && dy <= 1);
            assert(dx || dy);
            assert(!(dx && dy));
            bool matched = false;
            int x = tp.x(), y = tp.y();
            while (!matched) {
                x += dx;
                y += dy;
                if (dx && (x <= 0 || x >= width - 1)) break;
                if (dy && (y <= 0 || y >= height - 1)) break;
                Component endt = logic[y][x];
                Component endp = logic[y+dy][x+dx];
                if (IsTunnel(endt) && endp == startp) {
                    unite(comps, index(pp.x(), pp.y()), index(x+dx, y+dy));
                    matched = true;
                }
            }
            if (!matched)
                qDebug() << "warning: unmatched tunnel" << tp << "->" << pp;
        }
    }

    // mesh
    {
        QSet<int> mgroup;
        for (const Conn &conn : meshConns) {
            int id = findp(conn.first);
            int cid = indexq(conn.second);
            // all meshes are connected
            assert(IsMesh(type(id)));
            mgroup.insert(cid);
        }
        uniteGroupByType(mgroup);
    }

    // busses (after meshes and tunnels!)
    {
        CompConns bconns;
        for (const Conn &conn : busConns) {
            int id = findp(conn.first);
            int cid = indexq(conn.second);
            assert(IsBus(type(id)));
            bconns[id].insert(cid); // busses are keyed on bus entity id
        }
        for (const QSet<int> &group : bconns.values())
            uniteGroupByType(group);
    }

    // flatten everything again to simplify code
    for (int k = 0; k < comps.size(); ++ k)
        comps[k] = find(comps, comps[k]);

    // --- build graph from r/w inks

    sgraph_ = SimpleGraph();
    bpwidth_ = width;
    bpheight_ = height;

    // entity list
    for (int id : comps) {
        Component t = type(id);
        if (IsActive(t) || IsTrace(t))
            sgraph_.entities[id] = t;
    }

    // read connections
    for (const Conn &conn : readConns) {
        if (IsActive(type(indexq(conn.second)))) {
            int from = findp(conn.first);
            int to = findp(conn.second);
            sgraph_.connections.insert({from, to});
        }
    }

    // write connections
    for (const Conn &conn : writeConns) {
        if (IsActive(type(indexq(conn.second)))) {
            int from = findp(conn.second);
            int to = findp(conn.first);
            sgraph_.connections.insert({from, to});
        }
    }

    // --- debugging:

    qint64 nsecs = timer.nsecsElapsed();
    qDebug() << "compiled in" << nsecs / 1000000 << "ms";

    //for (int k = 0; k < comps.size(); ++ k)  comps[k] = find(comps, comps[k]);

    //QSet<int> names;
    //for (int name : comps)
    //    names.insert(name);
 //   qDebug() << "entities" << entities_;
 //   qDebug() << "connections" << connections_;

 //   for (int y = 0; y < height; ++ y) {
 //       QString row;
 //       for (int x = 0; x < width; ++ x) {
 //           if (type(index(x,y)) == Empty)
  //              row += "      ";
 //           else
  //              row += QString().asprintf("%5d ", comps[index(x,y)]);
  //      }
   //     qDebug().noquote() << row;
 //   }
    //qDebug() << busConns;
    //qDebug() << tunnelConns;
    //qDebug() << meshConns;
    //qDebug() << wirelessConns;

}

static bool operator < (const QColor &a, const QColor &b) {
    if (a.red() != b.red())
        return a.red() < b.red();
    else if (a.green() != b.green())
        return a.green() < b.green();
    else if (a.blue() != b.blue())
        return a.blue() < b.blue();
    else if (a.alpha() != b.alpha())
        return a.alpha() < b.alpha();
    else
        return false;
}

Compiler::Component Compiler::Comp (Blueprint::Ink ink) {

    static const QMap<Blueprint::Ink,Component> m = {
        { Blueprint::Cross, Cross },
        { Blueprint::Tunnel, Tunnel },
        { Blueprint::Mesh, Mesh },
        { Blueprint::Bus1, Bus1 },
        { Blueprint::Bus2, Bus2 },
        { Blueprint::Bus3, Bus3 },
        { Blueprint::Bus4, Bus4 },
        { Blueprint::Bus5, Bus5 },
        { Blueprint::Bus6, Bus6 },
        { Blueprint::Write, Write },
        { Blueprint::Read, Read },
        { Blueprint::Trace1, Trace1 },
        { Blueprint::Trace2, Trace2 },
        { Blueprint::Trace3, Trace3 },
        { Blueprint::Trace4, Trace4 },
        { Blueprint::Trace5, Trace5 },
        { Blueprint::Trace6, Trace6 },
        { Blueprint::Trace7, Trace7 },
        { Blueprint::Trace8, Trace8 },
        { Blueprint::Trace9, Trace9 },
        { Blueprint::Trace10, Trace10 },
        { Blueprint::Trace11, Trace11 },
        { Blueprint::Trace12, Trace12 },
        { Blueprint::Trace13, Trace13 },
        { Blueprint::Trace14, Trace14 },
        { Blueprint::Trace15, Trace15 },
        { Blueprint::Trace16, Trace16 },
        { Blueprint::Buffer, Buffer },
        { Blueprint::And, And },
        { Blueprint::Or, Or },
        { Blueprint::Nor, Nor },
        { Blueprint::Not, Not },
        { Blueprint::Nand, Nand },
        { Blueprint::Xor, Xor },
        { Blueprint::Xnor, Xnor },
        { Blueprint::LatchOn, LatchOn },
        { Blueprint::LatchOff, LatchOff },
        { Blueprint::Clock, Clock },
        { Blueprint::LED, LED },
        { Blueprint::Timer, Timer },
        { Blueprint::Random, Random },
        { Blueprint::Break, Break },
        { Blueprint::Wifi0, Wifi0 },
        { Blueprint::Wifi1, Wifi1 },
        { Blueprint::Wifi2, Wifi2 },
        { Blueprint::Wifi3, Wifi3 },
        { Blueprint::Annotation, Empty },
        { Blueprint::Filler, Empty },
        { Blueprint::Empty, Empty }
    };

    return m[ink];

}


//#############################################################################
// technically all the compiler code was above. everything below is analysis.
//#############################################################################


QString Compiler::Desc (Component comp) {

    static const QMap<Component,QString> s = {
        { Empty, "Empty" },
        { Cross, "Cross" },
        { Tunnel, "Tunnel" },
        { Mesh, "Mesh" },
        { Bus1, "Bus1" },
        { Bus2, "Bus2" },
        { Bus3, "Bus3" },
        { Bus4, "Bus4" },
        { Bus5, "Bus5" },
        { Bus6, "Bus6" },
        { Write, "Trace" },
        { Read, "Trace" },
        { Trace1, "Trace" },
        { Trace2, "Trace" },
        { Trace3, "Trace" },
        { Trace4, "Trace" },
        { Trace5, "Trace" },
        { Trace6, "Trace" },
        { Trace7, "Trace" },
        { Trace8, "Trace" },
        { Trace9, "Trace" },
        { Trace10, "Trace" },
        { Trace11, "Trace" },
        { Trace12, "Trace" },
        { Trace13, "Trace" },
        { Trace14, "Trace" },
        { Trace15, "Trace" },
        { Trace16, "Trace" },
        { Buffer, "Buffer" },
        { And, "And" },
        { Or, "Or" },
        { Nor, "Nor" },
        { Not, "Not" },
        { Nand, "Nand" },
        { Xor, "Xor" },
        { Xnor, "Xnor" },
        { LatchOn, "LatchOn" },
        { LatchOff, "LatchOff" },
        { Clock, "Clock" },
        { LED, "LED" },
        { Timer, "Timer" },
        { Random, "Random" },
        { Break, "Break" },
        { Wifi0, "Wifi0" },
        { Wifi1, "Wifi1" },
        { Wifi2, "Wifi2" },
        { Wifi3, "Wifi3" }
    };

    return s[comp];

}


Compiler::GraphResults Compiler::buildGraphViz (GraphSettings settings) const {

    GraphResults results;

    if (settings.timings)
        settings.ioclusters = false;

    SimpleGraph graph = settings.compressed ? compressedConnections() : sgraph_;

    QStringList dot;
    dot.append("digraph {");

    /*for (int id : graph.entities.keys()) {
        QString label = Desc(graph.entities[id]);
        dot.append(QString("  %1[label=\"%2\"];").arg(id).arg(label));
    }*/

    ComplexGraph cgraph = buildComplexGraph(graph);

    for (Node *node : cgraph.values()) {
        node->purpose = Node::Other;
        if (IsTrace(node->type) || IsLatch(node->type) || IsLED(node->type)) {
            if (node->from.empty() && !node->to.empty())
                node->purpose = Node::Input;
            else if (node->to.empty() && !node->from.empty())
                node->purpose = Node::Output;
        }
    }

    if (settings.timings || settings.timinglabels)
        results.stats = computeTimings(cgraph);

    for (int id : graph.entities.keys()) {

        QMap<QString,QString> attrs;
        QString cluster;

        QString label = Desc(graph.entities[id]);
        if (settings.timinglabels)
            label += QString(" (%1-%2)").arg(cgraph[id]->mintiming).arg(cgraph[id]->maxtiming);
        attrs["label"] = label;

        if (settings.ioclusters) {
            switch (cgraph[id]->purpose) {
            case Node::Input: cluster = "input"; break;
            case Node::Output: cluster = "output"; break;
            default: break;
            }
        } else if (settings.timings) {
            int ticks = cgraph[id]->maxtiming;
            if (ticks >= 0) cluster = QString("%1").arg(ticks);
        }

        if (settings.positions != GraphSettings::None) {
            float posx = (id % bpwidth_) * settings.scale;
            float posy = (bpheight_ - id / bpwidth_) * settings.scale;
            attrs["pos"] = QString("%1,%2%3").arg(posx).arg(posy).arg(settings.positions == GraphSettings::Absolute ? "!" : "");
        }

        if (cgraph[id]->critpath)
            attrs["color"] = "red";

        QStringList attrstrs;
        for (QString k : attrs.keys())
            attrstrs += k + "=\"" + attrs[k] + "\"";
        QString attrstr = attrstrs.join(",");

        if (cluster != "")
            dot.append(QString("  subgraph cluster_%3 { %1[%2] };").arg(id).arg(attrstr).arg(cluster));
        else
            dot.append(QString("  %1[%2];").arg(id).arg(attrstr));

    }

    for (QPair conn : graph.connections) {

        QMap<QString,QString> attrs;

        {
            const Node *from = cgraph[conn.first];
            const Node *to = cgraph[conn.second];
            if (from->critpath && to->critpath && from->maxtiming >= to->maxtiming - 1)
                attrs["color"] = "red";
        }

        QStringList attrstrs;
        for (QString k : attrs.keys())
            attrstrs += k + "=\"" + attrs[k] + "\"";
        QString attrstr = attrstrs.join(",");

        dot.append(QString("  %1->%2[%3];").arg(conn.first).arg(conn.second).arg(attrstr));

    }

    deleteComplexGraph(cgraph);

    dot.append("}");

    results.graphviz = dot;
    return results;

}


Compiler::SimpleGraph Compiler::compressedConnections () const {

    ComplexGraph nodes = buildComplexGraph(sgraph_);

    // remove traces
    {
        QVector<Node *> nodework(nodes.values().toVector());
        for (Node *node : nodework) {
            if (IsTrace(node->type) && node->from.size() == 1 && node->to.size() != 0) {
                for (Node *from : node->from)
                    for (Node *to : node->to)
                        Node::connect(from, to);
                nodes.remove(node->id);
                delete node;
            }
        }
    }

    SimpleGraph cgraph;

    // build new entity list
    for (Node *node : nodes.values())
        cgraph.entities[node->id] = node->type;

    // build new connection list
    for (Node *node : nodes.values())
        for (Node *to : node->to)
            cgraph.connections.insert({node->id, to->id});

    deleteComplexGraph(nodes);
    return cgraph;

}


QStringList Compiler::analyzeCircuit (const AnalysisSettings &settings) const {

    ComplexGraph nodes = buildComplexGraph(sgraph_);

    QStringList results;

    const auto print = [&] (const Node *node, QString message) {
        int x = node->id % bpwidth_;
        int y = node->id / bpwidth_;
        qDebug() << "analysis:" << x << y << message;
        results.append(QString("%1, %2: %3").arg(x).arg(y).arg(message));
    };

    const auto inputcheck = [&] (const Node *node, Component type, int minInput, int minOutput) {
        if (node->type == type) {
            if (node->from.size() < minInput) print(node, QString("%1 has less than %2 inputs").arg(Desc(node->type)).arg(minInput));
            if (node->to.size() < minOutput) print(node, QString("%1 has less than %2 outputs").arg(Desc(node->type)).arg(minOutput));
        }
    };

    //constexpr int GateMinIn = 1;
    //constexpr bool CheckTraces = false;
    const int GateMinIn = settings.checkGates ? 2 : 1;
    const bool CheckTraces = settings.checkTraces;

    for (Node *node : nodes.values()) {
        if (CheckTraces) {
            if (IsTrace(node->type) && node->to.empty())
                print(node, "nothing reads from this trace");
            if (IsTrace(node->type) && node->from.empty())
                print(node, "nothing writes to this trace");
        }
        inputcheck(node, Buffer, 1, 1);
        inputcheck(node, And, GateMinIn, 1);
        inputcheck(node, Or, GateMinIn, 1);
        inputcheck(node, Nor, GateMinIn, 1);
        inputcheck(node, Not, 1, 1);
        inputcheck(node, Nand, GateMinIn, 1);
        inputcheck(node, Xor, GateMinIn, 1);
        inputcheck(node, Xnor, GateMinIn, 1);
        inputcheck(node, LatchOn, 0, 1);
        inputcheck(node, LatchOff, 0, 1);
        inputcheck(node, Clock, 0, 1);
        inputcheck(node, LED, 1, 0);
        inputcheck(node, Timer, 0, 1);
        inputcheck(node, Random, 0, 1);
        inputcheck(node, Break, 1, 0);
        inputcheck(node, Wifi0, 1, 1);
        inputcheck(node, Wifi1, 1, 1);
        inputcheck(node, Wifi2, 1, 1);
        inputcheck(node, Wifi3, 1, 1);
    }

    deleteComplexGraph(nodes);

    return results;

}


Compiler::ComplexGraph Compiler::buildComplexGraph (const SimpleGraph &sgraph) {

    ComplexGraph nodes;

    // create nodes
    for (int id : sgraph.entities.keys())
        nodes[id] = new Node(id, sgraph.entities[id]);

    // connect nodes
    for (QPair<int,int> conn : sgraph.connections) {
        Node *from = nodes[conn.first];
        Node *to = nodes[conn.second];
        assert(from);
        assert(to);
        Node::connect(from, to);
    }

    return nodes;

}


Compiler::TimingStats Compiler::computeTimings (ComplexGraph &graph) {

    TimingStats stats;

    // naive implementation, will freeze on loops!
    const std::function<void(Node*,int,int)> compute = [&compute] (Node *node, int tickmin, int tickmax) {
        if (node->hit) {
            node->isloop = true;
            qDebug() << "node" << node->id << Desc(node->type) << "is loop; breaking...";
            return;
        }
        node->mintiming = (node->mintiming >= 0 ? std::min(node->mintiming, tickmin) : tickmin);
        node->maxtiming = std::max(node->maxtiming, tickmax);
        int nexttickmin = IsTrace(node->type) ? node->mintiming : (node->mintiming + 1);
        int nexttickmax = IsTrace(node->type) ? node->maxtiming : (node->maxtiming + 1);
        for (Node *to : node->to) {
            node->hit = true;
            compute(to, nexttickmin, nexttickmax);
            node->hit = false;
        }
    };

    for (Node *node : graph.values()) {
        node->mintiming = node->maxtiming = -1;
        node->critpath = false;
        node->hit = false;
        node->isloop = false;
    }

    for (Node *node : graph.values())
        if (node->purpose == Node::Input)
            compute(node, 0, 0);

    int maxmintime = -1, maxmaxtime = -1, minmaxtime = -1;
    for (Node *node : graph.values()) {
        if (node->purpose == Node::Output) {
            maxmintime = std::max(maxmintime, node->mintiming);
            minmaxtime = (minmaxtime == -1 ? node->maxtiming : std::min(minmaxtime, node->maxtiming));
            maxmaxtime = std::max(maxmaxtime, node->maxtiming);
        }
    }
    qDebug() << "maxmintime" << maxmintime << "minmaxtime" << minmaxtime << "maxmaxtime" << maxmaxtime;

    stats.maxmintime = maxmintime;
    stats.minmaxtime = minmaxtime;
    stats.maxmaxtime = maxmaxtime;

    bool critpathEndsWithEntity = false;
    QList<Node *> critnodes;
    for (Node *node : graph.values())
        if (node->purpose == Node::Output && node->maxtiming == maxmaxtime) {
            critnodes.append(node);
            qDebug() << "  crit node type" << Desc(node->type);
            if (!IsTrace(node->type))
                critpathEndsWithEntity = true;
        }

    // timing is signal *arrival* time so our circuit actually takes one tick longer if
    // the output node is not a trace.
    //stats.critpathlen = (critpathEndsWithEntity ? (maxmaxtime + 1) : maxmaxtime);
    // actually, for now just leave it because current output node detection will force it
    // to be either a trace or an LED, and it's not really useful to count the LED.
    stats.critpathlen = maxmaxtime;
    // in fact, we should probably also subtract 1 if the input nodes are latches because
    // that usually just means the blueprint contained latches for test input. but that's
    // a little more complicated and it would be better to make input latches just not
    // count for a tick in the timing analysis.
    // hmm the best approach is probably a checkbox that says "skip input latches" in tick
    // counts.
    // TODO

    QSet<Node *> visited;
    while (!critnodes.empty()) {
        QSet<Node *> prev;
        for (Node *node : critnodes) {
            if (!visited.contains(node)) {
                visited.insert(node);
                node->critpath = true;
                for (Node *from : node->from)
                    if (from->maxtiming >= node->maxtiming - 1)
                        prev.insert(from);
            }
        }
        critnodes = prev.values();
    }

    return stats;

}
