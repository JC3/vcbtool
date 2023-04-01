#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <QLabel>

class ColorSelector : public QLabel {
    Q_OBJECT
public:
    explicit ColorSelector(QWidget *parent = nullptr);
    QColor selectedColor () const;
public slots:
    void setSelectedColor (QColor color);
signals:
    void colorChanged (QColor color);
protected:
    void mousePressEvent (QMouseEvent *ev) override;
};

#endif // COLORSELECTOR_H
