#ifndef CIRCUITS_H
#define CIRCUITS_H

#include "blueprint.h"
#include <QFileInfo>
#include <QVector>

namespace Circuits {

struct ROMData {

    enum AddressBit { Off, On, DontCare };
    using DataBit = bool;
    using Address = QVector<AddressBit>; // msb first
    using Data = QVector<DataBit>;

    static Address makeAddress (quint64 number, int bits);

    QFileInfo sourceFile;
    int addressBits;
    int dataBits;
    QMap<Address,Data> data;

    ROMData () : addressBits(0), dataBits(0) { }

    struct RawOptions {
        int wordSize;
        bool bigEndian;
        int addressBits;
        int dataBits;
        RawOptions () : wordSize(4), bigEndian(false), addressBits(0), dataBits(0) { }
    };

    static ROMData * fromRaw (const QString &filename, const RawOptions &options);

    struct CSVOptions {
        int skipRows;
        int addressBits; // -1 = auto
        int dataBits; // -1 = auto
        CSVOptions () : skipRows(0), addressBits(-1), dataBits(-1) { }
    };

    static ROMData * fromCSV (const QString &filename, const CSVOptions &options);

};

enum ROMDataLSBSide { Bottom=0, Top=1 };
enum ROMAddress0Side { Near=0, Far=1 };

Blueprint * ROM (const ROMData *data, ROMDataLSBSide dataLSB, ROMAddress0Side addr0Side, bool omitEmpty);
Blueprint * ROM (int addressBits, int dataBits, ROMDataLSBSide dataLSB, ROMAddress0Side addr0Side, const QVector<quint64> &data, bool omitEmpty);
Blueprint * Text (QImage font, QString fontCharset, int kerning, QString text, Blueprint::Ink logicInk = Blueprint::Annotation, Blueprint::Ink decoOnInk = Blueprint::Invalid, Blueprint::Ink decoOffInk = Blueprint::Invalid);

}

#endif // CIRCUITS_H
