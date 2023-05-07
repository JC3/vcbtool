#include "circuits.h"
#include <QDebug>
#include <QFile>
#include <stdexcept>

using std::runtime_error;

namespace Circuits {

ROMData::Address ROMData::makeAddress (quint64 number, int bits) {

    ROMData::Address addr;
    for (int bit = bits - 1; bit >= 0; -- bit)
        addr.append((number & (1 << bit)) ? ROMData::On : ROMData::Off);
    return addr;

}

ROMData * ROMData::fromRaw (const QString &filename, const RawOptions &options) {

    std::unique_ptr<ROMData> rom = std::make_unique<ROMData>();
    rom->sourceFile = QFileInfo(filename);
    rom->addressBits = options.addressBits;
    rom->dataBits = options.dataBits;

    QFile file;
    if (!file.open(QFile::ReadOnly))
        throw runtime_error(file.errorString().toStdString());

    QByteArray romdata = file.readAll();

    const auto getWord = [&] (int offset) {
        if (offset < 0 || offset >= romdata.size())
            return 0ULL;
        quint64 word = 0;
        if (options.bigEndian) {
            for (int k = offset; k < offset + options.wordSize; ++ k) {
                quint8 b = (k < romdata.size() ? romdata[k] : 0);
                word = (word << 8) | b;
            }
        } else {
            for (int k = offset + options.wordSize - 1; k >= offset; -- k) {
                quint8 b = (k < romdata.size() ? romdata[k] : 0);
                word = (word << 8) | b;
            }
        }
        return word;
    };

    // makes data msb first
    const auto makeData = [&] (quint64 number) {
        ROMData::Data data;
        for (int bit = options.dataBits - 1; bit >= 0; -- bit)
             data.append(number & (1 << bit));
        return data;
    };

    quint64 addr = 0;
    for (auto k = 0; k < romdata.size(); k += options.wordSize, ++ addr)
        rom->data[makeAddress(k, options.addressBits)] = makeData(getWord(k));

    return rom.release();

}

ROMData * ROMData::fromCSV (const QString &filename, const CSVOptions &options) {

    return nullptr;

}

Blueprint * ROM (int addressBits, int dataBits, ROMDataLSBSide dataLSB, ROMAddress0Side addr0Side, const QVector<quint64> &data, bool omitEmpty) {

    const Blueprint::Ink trace = Blueprint::Trace5;

    if (omitEmpty && addr0Side == Far)
        throw runtime_error("Empty columns can only be omitted if address 0 is set to input side. Sorry.");

    // things will go nuts if bit counts are too high but for now just let
    // things go nuts. maybe overflow checking some day.
    quint64 addresses = 1ULL << addressBits;
    int width = 3 + 2 * addresses + 1;
    int height = (omitEmpty ? 3 : 1) + 4 * (addressBits - 1) + 2 * dataBits;
    qDebug() << "rom will be" << width << "x" << height;

    Blueprint *bp = new Blueprint(width, height);
    int row, col;

    // address inputs
    row = height - 1;
    for (int a = 0; a < addressBits; ++ a) {
        if (a == 0) {
            if (omitEmpty) {
                bp->set(1, row-2, Blueprint::Not);
                bp->set(2, row-2, Blueprint::Write);
                bp->set(0, row-1, trace);
                bp->set(1, row-1, Blueprint::Read);
                bp->set(1, row, Blueprint::Buffer);
                bp->set(2, row, Blueprint::Write);
                row -= 3;
            } else {
                bp->set(0, row, Blueprint::Read);
                bp->set(1, row, addr0Side == Far ? Blueprint::Buffer : Blueprint::Not);
                bp->set(2, row, Blueprint::Write);
                row -= 1;
            }
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

    int lastColumn = 0;

    // address and data bits
    // to do: truncate if data vector smaller than address space
    col = (addr0Side == Far ? (width - 2) : (width - 2 * addresses) );
    bool isnor = (addr0Side == Far);
    for (quint64 address = 0; address < addresses; ++ address) {

        quint64 curdata = data.value(address);
        if (omitEmpty && curdata == 0)
            continue;

        // ---- data bits
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
            // there is no not row for the first bit when !omitEmpty
            if (bit == 0 && !omitEmpty) {
                /*assert(rbuf);
                if (rbuf)*/ bp->set(col, row, Blueprint::Read);
                row -= 2;
            } else {
                if (rbuf) bp->set(col, row, Blueprint::Read);
                if (rnot) bp->set(col, row - 2, Blueprint::Read);
                row -= 4;
            }
        }
        isnor = !isnor;

        // ---- advance
        if (col > lastColumn) lastColumn = col;
        col += (addr0Side == Far ? -2 : 2);

    }

    // remove extra columns if requested
    if (omitEmpty) {
        assert(addr0Side == Near);
        for (int col = lastColumn + 1; col < width; ++ col)
            for (int row = 0; row < height; ++ row) {
                bp->set(col, row, Blueprint::Empty);
            }
    }

    // outputs
    for (int bit = 0; bit < dataBits; ++ bit)
        bp->set(lastColumn + 1, bit * 2, trace);

    return bp;

}


Blueprint * Text (QImage font, QString fontCharset, int kerning, QString text, Blueprint::Ink logicInk, Blueprint::Ink decoOnInk, Blueprint::Ink decoOffInk) {

    qDebug().noquote() << fontCharset;

    // ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 _.:! +-*/\= "' |^& ()[]<> @~#,?%{}`¬
    //constexpr char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789.!:+-*/\\=()[]|^&_<>\"'@~#,?%{}`\xAC";
    //constexpr int charsetlen = sizeof(charset) - 1;
    const QByteArray charset = fontCharset.toLatin1();
    const int charsetlen = charset.length();

    if (font.width() % charsetlen != 0)
        throw runtime_error("font image width does not match character set length");

    const QByteArray str = (charset.contains('a') ? text.toLatin1() : text.toUpper().toLatin1());

    font = font.convertToFormat(QImage::Format_RGB888);

    const int charwidth = font.width() / charsetlen;
    const int charheight = font.height();
    const int bpwidth = (charwidth + 1 + kerning) * str.size();
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
        const int dstx0 = k * (charwidth + 1 + kerning);
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
