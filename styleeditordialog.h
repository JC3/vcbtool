#ifndef STYLEEDITORDIALOG_H
#define STYLEEDITORDIALOG_H

#include <QDialog>

namespace Ui {
class StyleEditorDialog;
}

class StyleEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StyleEditorDialog(QWidget *parent = nullptr);
    ~StyleEditorDialog();

private slots:
    void on_btnSave_clicked();
    void on_btnRevert_clicked();

    void on_txtStyle_textChanged();

private:
    Ui::StyleEditorDialog *ui;
    void loadStyle ();
    void applyStyle ();
};

#endif // STYLEEDITORDIALOG_H
