﻿#ifndef COMPILER_H
#define COMPILER_H

#include "blueprint.h"
#include <QObject>
#include <QSet>
#include <QMap>

class Compiler : public QObject {
    Q_OBJECT
public:

    using Component = unsigned;

    static constexpr Component ActiveBit = 0x10000000;
    static constexpr Component CatMask   = 0x0FFF0000;
    static constexpr Component BusBit    = 0x00010000;
    static constexpr Component TraceBit  = 0x00020000;
    static constexpr Component WifiBit   = 0x00001000; // not a category!
    static constexpr Component LatchBit  = 0x00002000; // not a category!
    static constexpr Component Empty = 0;
    static constexpr Component Cross = 1;
    static constexpr Component Tunnel = 2;
    static constexpr Component Mesh = 3;
    static constexpr Component Bus1 = BusBit | 4;
    static constexpr Component Bus2 = BusBit | 5;
    static constexpr Component Bus3 = BusBit | 6;
    static constexpr Component Bus4 = BusBit | 7;
    static constexpr Component Bus5 = BusBit | 8;
    static constexpr Component Bus6 = BusBit | 9;
    static constexpr Component Write = TraceBit | 10;
    static constexpr Component Read = TraceBit | 11;
    static constexpr Component Trace1 = TraceBit | 12;
    static constexpr Component Trace2 = TraceBit | 13;
    static constexpr Component Trace3 = TraceBit | 14;
    static constexpr Component Trace4 = TraceBit | 15;
    static constexpr Component Trace5 = TraceBit | 16;
    static constexpr Component Trace6 = TraceBit | 17;
    static constexpr Component Trace7 = TraceBit | 18;
    static constexpr Component Trace8 = TraceBit | 19;
    static constexpr Component Trace9 = TraceBit | 20;
    static constexpr Component Trace10 = TraceBit | 21;
    static constexpr Component Trace11 = TraceBit | 22;
    static constexpr Component Trace12 = TraceBit | 23;
    static constexpr Component Trace13 = TraceBit | 24;
    static constexpr Component Trace14 = TraceBit | 25;
    static constexpr Component Trace15 = TraceBit | 26;
    static constexpr Component Trace16 = TraceBit | 27;
    static constexpr Component Buffer = ActiveBit | 28;
    static constexpr Component And = ActiveBit | 29;
    static constexpr Component Or = ActiveBit | 30;
    static constexpr Component Nor = ActiveBit | 31;
    static constexpr Component Not = ActiveBit | 32;
    static constexpr Component Nand = ActiveBit | 33;
    static constexpr Component Xor = ActiveBit | 34;
    static constexpr Component Xnor = ActiveBit | 35;
    static constexpr Component LatchOn = ActiveBit | LatchBit | 36;
    static constexpr Component LatchOff = ActiveBit | LatchBit | 37;
    static constexpr Component Clock = ActiveBit | 38;
    static constexpr Component LED = ActiveBit | 39;
    static constexpr Component Timer = ActiveBit | 40;
    static constexpr Component Random = ActiveBit | 41;
    static constexpr Component Break = ActiveBit | 42;
    static constexpr Component Wifi0 = ActiveBit | WifiBit | 43;
    static constexpr Component Wifi1 = ActiveBit | WifiBit | 44;
    static constexpr Component Wifi2 = ActiveBit | WifiBit | 45;
    static constexpr Component Wifi3 = ActiveBit | WifiBit | 46;

    static inline bool IsEmpty (Component id) { return id == Empty; }
    static inline bool IsBus (Component id) { return (id & BusBit); }
    static inline bool IsTrace (Component id) { return (id & TraceBit); }
    static inline bool IsWifi (Component id) { return (id & WifiBit); }
    static inline bool IsLatch (Component id) { return (id & LatchBit); }
    static inline bool IsTunnel (Component id) { return id == Tunnel; }
    static inline bool IsMesh (Component id) { return id == Mesh; }
    static inline bool IsCross (Component id) { return id == Cross; }
    static inline bool IsRead (Component id) { return id == Read; }
    static inline bool IsWrite (Component id) { return id == Write; }
    static inline bool IsLED (Component id) { return id == LED; }
    static inline bool IsActive (Component id) { return (id & ActiveBit); }
    static inline Component Category (Component id) { return (id & CatMask); }

    static inline bool Same (Component a, Component b) {
        if (a == b) return true;
        Component cata = Category(a);
        Component catb = Category(b);
        return cata && (cata == catb);
    }

    static inline int WirelessIndex (Component a) {
        switch (a) {
        case Wifi0: return 0;
        case Wifi1: return 1;
        case Wifi2: return 2;
        case Wifi3: return 3;
        default: return -1;
        }
    }

    static Component Comp (Blueprint::Ink ink);
    static QString Desc (Component comp);
    static QString IECLabel (Component comp);

    explicit Compiler (const Blueprint *bp, QObject *parent = nullptr);

    struct TimingStats {
        int minmaxtime;
        int maxmintime;
        int maxmaxtime;
        int critpathlen;
        TimingStats () : minmaxtime(-1), maxmintime(-1), maxmaxtime(-1), critpathlen(-1) { }
    };

    struct GraphSettings {
        enum PosMode { None=0, Absolute=1, Suggested=2 };
        bool compressed;
        bool ioclusters;
        bool timings;
        bool timinglabels;
        PosMode positions;
        float scale;
        bool squareio;
        bool highlightloops;
        bool iecsymbols;
        GraphSettings () : compressed(false), ioclusters(false), timings(false), timinglabels(false), positions(None), scale(1.0f), squareio(false), highlightloops(true), iecsymbols(false) { }
    };

    struct GraphResults {
        QStringList graphviz;
        TimingStats stats;
    };

    GraphResults buildGraphViz (GraphSettings settings) const;

    struct AnalysisSettings {
        bool checkTraces;
        bool checkGates;
        bool checkCrosses;
        bool rogueCrosses;
        bool checkLoops;
        AnalysisSettings () : checkTraces(true), checkGates(true), checkCrosses(true), rogueCrosses(true), checkLoops(true) { }
    };

    QStringList analyzeCircuit (const AnalysisSettings &settings) const;
    static QStringList analyzeBlueprint (const AnalysisSettings &settings, const Blueprint *blueprint);

private:

    struct SimpleGraph {
        QMap<int,Component> entities;
        QSet<QPair<int,int> > connections;
    };

    void compileBlueprint (const Blueprint *bp);
    SimpleGraph compressedConnections () const;

    SimpleGraph sgraph_;
    int bpwidth_;
    int bpheight_;

    struct Node {
        enum Purpose { Other, Input, Output };
        QVector<Node*> from;
        QVector<Node*> to;
        int id;
        Component type;
        Purpose purpose;
        int mintiming, maxtiming;
        bool critpath;
        bool hit;
        bool isloop;
        Node (int id, Component type) : id(id), type(type), purpose(Other), mintiming(-1), maxtiming(-1), critpath(false), hit(false), isloop(false) { }
        ~Node () {
            for (Node *in : from) in->to.removeOne(this);
            for (Node *out : to) out->from.removeOne(this);
        }
        static void connect (Node *from, Node *to) {
            from->to.append(to);
            to->from.append(from);
        }
    };

    using ComplexGraph = QMap<int,Node*>;

    static ComplexGraph buildComplexGraph (const SimpleGraph &sgraph);

    static void deleteComplexGraph (ComplexGraph &graph) {
        qDeleteAll(graph.values());
        graph.clear();
    }

    static TimingStats computeTimings (ComplexGraph &graph);

};

#endif // COMPILER_H
