#ifndef CIRCUITS_H
#define CIRCUITS_H

#include "blueprint.h"

namespace Circuits {

enum ROMDataLSBSide { Bottom=0, Top=1 };

Blueprint * ROM (int addressBits, int dataBits, ROMDataLSBSide dataLSB, const QVector<quint64> &data);
Blueprint * Text (QImage font, QString text);

}

#endif // CIRCUITS_H
