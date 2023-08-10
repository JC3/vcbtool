#include "colorselector.h"
#include <QMouseEvent>
#include <QColorDialog>

ColorSelector::ColorSelector (QWidget *parent) :
    QLabel(parent)
{
    setAutoFillBackground(true);
    setSelectedColor(QColor(255, 255, 255));
}

QColor ColorSelector::selectedColor () const {
    return palette().color(backgroundRole());
}

void ColorSelector::setSelectedColor (QColor color) {
    QPalette p = palette();
    p.setColor(backgroundRole(), color);
    setPalette(p);
    if (usecss_)
        setStyleSheet(QString("#%4 { background:rgb(%1,%2,%3); }")
                      .arg(color.red())
                      .arg(color.green())
                      .arg(color.blue())
                      .arg(objectName()));
    else
        setStyleSheet(QString());
    emit colorChanged(color);
}

void ColorSelector::mousePressEvent (QMouseEvent *ev) {
    if (ev->button() == Qt::LeftButton) {
        QColor color = QColorDialog::getColor(selectedColor(), this, "Choose Color");
        if (color.isValid())
            setSelectedColor(color);
    }
}

void ColorSelector::setUseStyleSheet (bool use) {
     usecss_ = use;
     setSelectedColor(selectedColor());
}

