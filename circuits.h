#ifndef CIRCUITS_H
#define CIRCUITS_H

#include "blueprint.h"

namespace Circuits {

Blueprint * ROM (int addressBits, int dataBits, const QVector<quint64> &data);
Blueprint * Text (QImage font, QString text);

}

#endif // CIRCUITS_H
