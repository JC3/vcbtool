#ifndef CIRCUITS_H
#define CIRCUITS_H

#include "blueprint.h"

namespace Circuits {

enum ROMDataLSBSide { Bottom=0, Top=1 };
enum ROMAddress0Side { Near=0, Far=1 };

Blueprint * ROM (int addressBits, int dataBits, ROMDataLSBSide dataLSB, ROMAddress0Side addr0Side, const QVector<quint64> &data, bool omitEmpty);
Blueprint * Text (QImage font, QString fontCharset, int kerning, QString text, Blueprint::Ink logicInk = Blueprint::Annotation, Blueprint::Ink decoOnInk = Blueprint::Invalid, Blueprint::Ink decoOffInk = Blueprint::Invalid);
Blueprint * Text (QFont font, int fontHeight, QString text, Blueprint::Ink logicInk, Blueprint::Ink decoOnInk, Blueprint::Ink decoOffInk);

}

#endif // CIRCUITS_H
