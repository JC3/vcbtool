#include "blueprint.h"
#include <zstd.h>
#include <stdexcept>
#include <QCryptographicHash>
#include <QDebug>
#include <QElapsedTimer>

using std::runtime_error;

const Blueprint::Ink Blueprint::Cross = QColor(102, 120, 142, 255);
const Blueprint::Ink Blueprint::Tunnel = QColor(83, 85, 114, 255);
const Blueprint::Ink Blueprint::Mesh = QColor(100, 106, 87, 255);
const Blueprint::Ink Blueprint::Bus1 = QColor(122, 47, 36, 255);
const Blueprint::Ink Blueprint::Bus2 = QColor(62, 122, 36, 255);
const Blueprint::Ink Blueprint::Bus3 = QColor(36, 65, 122, 255);
const Blueprint::Ink Blueprint::Bus4 = QColor(37, 98, 122, 255);
const Blueprint::Ink Blueprint::Bus5 = QColor(122, 45, 102, 255);
const Blueprint::Ink Blueprint::Bus6 = QColor(122, 112, 36, 255);
const Blueprint::Ink Blueprint::Write = QColor(77, 56, 62, 255);
const Blueprint::Ink Blueprint::Read = QColor(46, 71, 93, 255);
const Blueprint::Ink Blueprint::Trace1 = QColor(42, 53, 65, 255);
const Blueprint::Ink Blueprint::Trace2 = QColor(159, 168, 174, 255);
const Blueprint::Ink Blueprint::Trace3 = QColor(161, 85, 94, 255);
const Blueprint::Ink Blueprint::Trace4 = QColor(161, 108, 86, 255);
const Blueprint::Ink Blueprint::Trace5 = QColor(161, 133, 86, 255);
const Blueprint::Ink Blueprint::Trace6 = QColor(161, 152, 86, 255);
const Blueprint::Ink Blueprint::Trace7 = QColor(153, 161, 86, 255);
const Blueprint::Ink Blueprint::Trace8 = QColor(136, 161, 86, 255);
const Blueprint::Ink Blueprint::Trace9 = QColor(108, 161, 86, 255);
const Blueprint::Ink Blueprint::Trace10 = QColor(86, 161, 141, 255);
const Blueprint::Ink Blueprint::Trace11 = QColor(86, 147, 161, 255);
const Blueprint::Ink Blueprint::Trace12 = QColor(86, 123, 161, 255);
const Blueprint::Ink Blueprint::Trace13 = QColor(86, 98, 161, 255);
const Blueprint::Ink Blueprint::Trace14 = QColor(102, 86, 161, 255);
const Blueprint::Ink Blueprint::Trace15 = QColor(135, 86, 161, 255);
const Blueprint::Ink Blueprint::Trace16 = QColor(161, 85, 151, 255);
const Blueprint::Ink Blueprint::Buffer = QColor(146, 255, 99, 255);
const Blueprint::Ink Blueprint::And = QColor(255, 198, 99, 255);
const Blueprint::Ink Blueprint::Or = QColor(99, 242, 255, 255);
const Blueprint::Ink Blueprint::Xor = QColor(174, 116, 255, 255);
const Blueprint::Ink Blueprint::Not = QColor(255, 98, 138, 255);
const Blueprint::Ink Blueprint::Nand = QColor(255, 162, 0, 255);
const Blueprint::Ink Blueprint::Nor = QColor(48, 217, 255, 255);
const Blueprint::Ink Blueprint::Xnor = QColor(166, 0, 255, 255);
const Blueprint::Ink Blueprint::LatchOn = QColor(99, 255, 159, 255);
const Blueprint::Ink Blueprint::LatchOff = QColor(56, 77, 71, 255);
const Blueprint::Ink Blueprint::Clock = QColor(255, 0, 65, 255);
const Blueprint::Ink Blueprint::LED = QColor(255, 255, 255, 255);
const Blueprint::Ink Blueprint::Timer = QColor(255, 103, 0, 255);
const Blueprint::Ink Blueprint::Random = QColor(229, 255, 0, 255);
const Blueprint::Ink Blueprint::Break = QColor(224, 0, 0, 255);
const Blueprint::Ink Blueprint::Wifi0 = QColor(255, 0, 191, 255);
const Blueprint::Ink Blueprint::Wifi1 = QColor(255, 0, 175, 255);
const Blueprint::Ink Blueprint::Wifi2 = QColor(255, 0, 159, 255);
const Blueprint::Ink Blueprint::Wifi3 = QColor(255, 0, 143, 255);
const Blueprint::Ink Blueprint::Annotation = QColor(58, 69, 81, 255);
const Blueprint::Ink Blueprint::Filler = QColor(140, 171, 161, 255);
const Blueprint::Ink Blueprint::Empty = QColor(0, 0, 0, 0);
const Blueprint::Ink Blueprint::Invalid = QColor();

Blueprint::Blueprint (int width, int height, QObject *parent) :
    QObject(parent)
{
    QImage bpImage(width, height, QImage::Format_RGBA8888);
    bpImage.fill(0);
    layers_[Logic] = bpImage.copy();
    layers_[DecoOn] = bpImage.copy();
    layers_[DecoOff] = bpImage.copy();
}

Blueprint::Blueprint (QImage bpImage, Layer layer, QObject *parent) :
    QObject(parent)
{
    bpImage = bpImage.copy();
    bpImage.convertTo(QImage::Format_RGBA8888);
    QImage emptyImage(bpImage.width(), bpImage.height(), QImage::Format_RGBA8888);
    emptyImage.fill(0);
    layers_[Logic] = (layer == Logic ? bpImage : emptyImage).copy();
    layers_[DecoOn] = (layer == DecoOn ? bpImage : emptyImage).copy();
    layers_[DecoOff] = (layer == DecoOff ? bpImage : emptyImage).copy();
}

Blueprint::Blueprint (QString bpString, QObject *parent) :
    QObject(parent),
    bpString_(bpString)
{

    QElapsedTimer timer;
    timer.start();

    static const auto getInt = [] (const QByteArray &d, int offset, int size) {
        if (offset + size > d.size())
            throw runtime_error("Unexpected end of blueprint string.");
        qint64 i = 0;
        for (int k = offset; k < offset + size; ++ k)
            i = (i << 8) | (quint8)d[k];
        return i;
    };

    if (!bpString.startsWith("VCB+"))
        throw runtime_error("Invalid blueprint string -- missing header.");

    QByteArray bp = QByteArray::fromBase64(bpString.mid(4).toLatin1());
    if (bp.isNull())
        throw runtime_error("Invalid blueprint string -- base64 decode failed.");

    quint32 bpVersion = getInt(bp, 0, 3);
    if (bpVersion != 0)
        throw runtime_error("Unsupported blueprint version.");

    //quint64 bpChecksum = getInt(bp, 3, 6);
    quint32 bpWidth = getInt(bp, 9, 4);
    quint32 bpHeight = getInt(bp, 13, 4);

    //qDebug() << bpVersion << bpWidth << bpHeight;
    int pos = 17;
    while (pos < bp.size()) {
        quint32 blockSize = getInt(bp, pos, 4);
        quint32 blockId = getInt(bp, pos + 4, 4);
        quint32 dataSize = getInt(bp, pos + 8, 4);
        //qDebug()  << "block" << blockSize << blockId << dataSize;
        if (blockId == 0) {
            QByteArray data(dataSize, 0);
            size_t result = ZSTD_decompress(data.data(), data.size(), bp.data() + pos + 12, blockSize - 12);
            if (result != dataSize)
                throw runtime_error("Invalid blueprint string -- zstd decompress failed.");
            QImage bpImage((const uchar *)data.constData(), bpWidth, bpHeight, QImage::Format_RGBA8888);
            layers_[(Layer)blockId] = bpImage.copy();
        }
        pos += blockSize;
    }

    if (layers_.empty())
        throw runtime_error("Invalid blueprint string -- no layers found.");

    qint64 nsecs = timer.nsecsElapsed();
    qDebug() << "blueprint decoded: " << (double)nsecs / 1000000.0 << "ms";

}

void Blueprint::setPixel (Layer which, int x, int y, Ink ink) {

    bpString_ = ""; // invalidate current blueprint string

    QImage &image = layers_[which];
    if (x < 0 || y < 0 || x >= image.width() || y >= image.height())
        throw runtime_error("Coordinates out of range for layer.");

    image.setPixelColor(x, y, ink);

}

Blueprint::Ink Blueprint::getPixel (Layer which, int x, int y) const {

    const QImage &image = layers_[which];
    if (x < 0 || y < 0 || x >= image.width() || y >= image.height())
        throw runtime_error("Coordinates out of range for layer.");

    return image.pixelColor(x, y);

}

QString Blueprint::bpString () const {

    if (bpString_ == "")
        generateBlueprintString();

    return bpString_;

}

void Blueprint::generateBlueprintString () const {

    QByteArray raw;

    const auto appendInt4 = [&](quint32 value) {
        raw.append((value >> 24) & 255);
        raw.append((value >> 16) & 255);
        raw.append((value >> 8) & 255);
        raw.append((value >> 0) & 255);
    };

    quint32 width = layers_[Logic].width();
    quint32 height = layers_[Logic].height();
    appendInt4(width);
    appendInt4(height);

    for (Layer layer : { Logic, DecoOn, DecoOff }) {
        QImage image = layers_[layer];
        quint32 uncompressedSize = image.width() * image.height() * 4;
        QByteArray compressedData(uncompressedSize * 2 + 100, 0);
        compressedData.detach();
        quint32 compressedSize = (quint32)ZSTD_compress(compressedData.data(), compressedData.size(), image.constBits(), uncompressedSize, 22);
        quint32 blockSize = 12 + compressedSize;
        appendInt4(blockSize);
        appendInt4((quint32)layer);
        appendInt4(uncompressedSize);
        raw.append(compressedData.constData(), compressedSize);
    }

    QByteArray rawBase64 = raw.toBase64();

    QByteArray hash = QCryptographicHash::hash(rawBase64, QCryptographicHash::Sha1);
    QByteArray hashBase64 = hash.toBase64().left(8);

    bpString_ = "VCB+AAAA" + QString::fromLatin1(hashBase64) + QString::fromLatin1(rawBase64);

}


QString Blueprint::toDiscordEmoji () const {

    static const QMap<Ink,QString> EmojiMap = {
        { And, "and" },
        { Annotation, "ann" },
        { Buffer, "bfr" },
        { Cross, "crs" },
        { Filler, "fil" },
        { LED, "led" },
        { LatchOff, "lt0" },
        { LatchOn, "lt1" },
        { Nand, "nan" },
        { Nor, "nor" },
        { Not, "not" },
        { Or, "or" },
        { Read, "rd" },
        { Write, "wr" },
        { Xnor, "xnr" },
        { Xor, "xor" },
        { Trace1, "t00" },
        { Trace2, "t01" },
        { Trace3, "t02" },
        { Trace4, "t03" },
        { Trace5, "t04" },
        { Trace6, "t05" },
        { Trace7, "t06" },
        { Trace8, "t07" },
        { Trace9, "t08" },
        { Trace10, "t09" },
        { Trace11, "t10" },
        { Trace12, "t11" },
        { Trace13, "t12" },
        { Trace14, "t13" },
        { Trace15, "t14" },
        { Trace16, "t15" },
        { Empty, "pd" }
    };

    QString emojistr = ":pd:\n";
    for (int y = 0; y < height(); ++ y) {
        for (int x = 0; x < width(); ++ x) {
            Ink ink = get(x, y);
            emojistr += ":" + EmojiMap.value(ink, "vcb") + ":";
        }
        emojistr += "\n";
    }

    return emojistr;

}

