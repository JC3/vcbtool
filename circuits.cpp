#include "circuits.h"
#include <QDebug>
#include <stdexcept>

using std::runtime_error;

namespace Circuits {

Blueprint * ROM (int addressBits, int dataBits, ROMDataLSBSide dataLSB, const QVector<quint64> &data) {

    const Blueprint::Ink trace = Blueprint::Trace1;

    // things will go nuts if bit counts are too high but for now just let
    // things go nuts. maybe overflow checking some day.
    quint64 addresses = 1ULL << addressBits;
    int width = 3 + 2 * addresses + 1;
    int height = 1 + 4 * (addressBits - 1) + 2 * dataBits;
    qDebug() << "rom will be" << width << "x" << height;

    Blueprint *bp = new Blueprint(width, height);
    int row, col;

    // address inputs
    row = height - 1;
    for (int a = 0; a < addressBits; ++ a) {
        if (a == 0) {
            bp->set(0, row, Blueprint::Read);
            bp->set(1, row, Blueprint::Buffer);
            bp->set(2, row, Blueprint::Write);
            row -= 1;
        } else {
            bp->set(1, row-3, Blueprint::Not);
            bp->set(2, row-3, Blueprint::Write);
            bp->set(0, row-2, trace);
            bp->set(1, row-2, Blueprint::Read);
            bp->set(1, row-1, Blueprint::Buffer);
            bp->set(2, row-1, Blueprint::Write);
            row -= 4;
        }
    }

    // crosses, gates, background traces
    for (row = 0; row < height; row += 2) {
        for (col = 3; col < width - 1; col += 2) {
            bp->set(col, row, Blueprint::Cross);
            bp->set(col+1, row, trace);
        }
        if (row < height - 1) {
            bool isand = true;
            for (col = 3; col < width - 1; col += 2) {
                Blueprint::Ink ink = isand ? Blueprint::And : Blueprint::Nor;
                bp->set(col, row+1, ink);
                bp->set(col+1, row+1, ink);
                isand = !isand;
            }
        }
    }

    // outputs
    for (int bit = 0; bit < dataBits; ++ bit)
        bp->set(width - 1, bit * 2, trace);

    // address and data bits
    // to do: truncate if data vector smaller than address space
    col = width - 2;
    bool isnor = true;
    for (quint64 address = 0; address < addresses; ++ address) {

        // ---- data bits
        quint64 curdata = data.value(address);
        row = (dataLSB == Top ? 0 : (2 * (dataBits - 1)));
        for (int bit = 0; bit < dataBits; ++ bit) {
            if (curdata & (1ULL << bit))
                bp->set(col, row, Blueprint::Write);
            row += (dataLSB == Top ? 2 : -2);
        }

        // ---- address bits
        // - in columns with the nors, the not row gets an R if the bit is 1, otherwise the buffer row gets it.
        // - in columns with the ands, the buffer row gets an R if the bit is 1, otherwise the not row gets it.
        row = height - 1;
        for (int bit = 0; bit < addressBits; ++ bit) {
            bool one = (address & (1ULL << bit)) != 0;
            // i *could* simplify these to logical expressions, but this is why i suck at vcb.
            bool rbuf = isnor ? (one ? false : true) : (one ? true : false);
            bool rnot = !rbuf;
            // there is no not row for the first bit
            if (bit == 0) {
                assert(rbuf);
                if (rbuf) bp->set(col, row, Blueprint::Read);
                row -= 2;
            } else {
                if (rbuf) bp->set(col, row, Blueprint::Read);
                if (rnot) bp->set(col, row - 2, Blueprint::Read);
                row -= 4;
            }
        }
        isnor = !isnor;

        // ---- advance
        col -= 2;

    }

    return bp;

}


// todo: specify layer and ink as parameters
Blueprint * Text (QImage font, QString text, Blueprint::Ink logicInk, Blueprint::Ink decoOnInk, Blueprint::Ink decoOffInk) {

    constexpr char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789.!:+-*/\\=()[]|^&_<>\"'";
    constexpr int charsetlen = sizeof(charset) - 1;

    if (font.width() % charsetlen != 0)
        throw runtime_error("font image width does not match character set length");

    const QByteArray str = text.toUpper().toLatin1();

    font = font.convertToFormat(QImage::Format_RGB888);

    const int charwidth = font.width() / charsetlen;
    const int charheight = font.height();
    const int bpwidth = (charwidth + 1) * str.size();
    const int bpheight = charheight;

    QVector<QPair<Blueprint::Layer,Blueprint::Ink> > layerInks;
    layerInks.append({Blueprint::Logic, logicInk});
    layerInks.append({Blueprint::DecoOn, decoOnInk});
    layerInks.append({Blueprint::DecoOff, decoOffInk});

    Blueprint *bp = new Blueprint(bpwidth, bpheight);

    for (int k = 0; k < str.size(); ++ k) {
        const char ch = str[k];
        const char *chloc = strchr(charset, ch);
        if (!chloc) { qDebug() << "skipping undefined character" << ch; continue; }
        const int index = chloc - charset;
        const int srcx0 = index * charwidth;
        const int dstx0 = k * (charwidth + 1);
        for (auto layerInk : layerInks) {
            const Blueprint::Layer layer = layerInk.first;
            const Blueprint::Ink ink = layerInk.second;
            if (!ink.isValid())
                continue;
            for (int cy = 0; cy < charheight; ++ cy) {
                for (int cx = 0; cx < charwidth; ++ cx) {
                    const int srcx = srcx0 + cx;
                    const int srcy = cy;
                    const int dstx = dstx0 + cx;
                    const int dsty = cy;
                    QRgb srcpix = font.pixel(srcx, srcy);
                    if (qRed(srcpix) < 128) bp->setPixel(layer, dstx, dsty, ink);
                }
            }
        }
    }

    return bp;

}


}
