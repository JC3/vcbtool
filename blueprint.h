#ifndef BLUEPRINT_H
#define BLUEPRINT_H

#include <QObject>
#include <QColor>
#include <QImage>
#include <QMap>

class Blueprint : public QObject {
    Q_OBJECT
public:
    using Ink = QColor;
    static const Ink Cross;
    static const Ink Tunnel;
    static const Ink Mesh;
    static const Ink Bus1;
    static const Ink Bus2;
    static const Ink Bus3;
    static const Ink Bus4;
    static const Ink Bus5;
    static const Ink Bus6;
    static const Ink Write;
    static const Ink Read;
    static const Ink Trace1;
    static const Ink Trace2;
    static const Ink Trace3;
    static const Ink Trace4;
    static const Ink Trace5;
    static const Ink Trace6;
    static const Ink Trace7;
    static const Ink Trace8;
    static const Ink Trace9;
    static const Ink Trace10;
    static const Ink Trace11;
    static const Ink Trace12;
    static const Ink Trace13;
    static const Ink Trace14;
    static const Ink Trace15;
    static const Ink Trace16;
    static const Ink Buffer;
    static const Ink And;
    static const Ink Or;
    static const Ink Nor;
    static const Ink Not;
    static const Ink Nand;
    static const Ink Xor;
    static const Ink Xnor;
    static const Ink LatchOn;
    static const Ink LatchOff;
    static const Ink Clock;
    static const Ink LED;
    static const Ink Timer;
    static const Ink Random;
    static const Ink Break;
    static const Ink Wifi0;
    static const Ink Wifi1;
    static const Ink Wifi2;
    static const Ink Wifi3;
    static const Ink Annotation;
    static const Ink Filler;
    static const Ink Empty;
    enum Layer {
        Logic = 0,
        DecoOn = 1,
        DecoOff = 2
    };
    explicit Blueprint (QString bpString, QObject *parent = nullptr);
    Blueprint (QImage bpImage, Layer layer, QObject *parent = nullptr);
    Blueprint (int width, int height, QObject *parent = nullptr);
    QImage layer (Layer which) const { return layers_[which]; }
    void setPixel (Layer which, int x, int y, Ink ink);
    void set (int x, int y, Ink ink) { setPixel(Logic, x, y, ink); }
    QString bpString () const;
    int width () const { return layers_.first().width(); }
    int height () const { return layers_.first().height(); }
    Ink getPixel (Layer which, int x, int y) const;
    Ink get (int x, int y) const { return getPixel(Logic, x, y); }
private:
    mutable QString bpString_;
    QMap<Layer,QImage> layers_;
    void generateBlueprintString () const;
};

#endif // BLUEPRINT_H
