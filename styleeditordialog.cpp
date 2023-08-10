#include "styleeditordialog.h"
#include "ui_styleeditordialog.h"
#include <QFile>
#include <QMessageBox>
#include <QApplication>

StyleEditorDialog::StyleEditorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StyleEditorDialog)
{
    ui->setupUi(this);
    loadStyle();
}

StyleEditorDialog::~StyleEditorDialog()
{
    delete ui;
}

void StyleEditorDialog::on_btnSave_clicked()
{
    QFile file("stylesheet-dev.css");
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, "Save Error", file.errorString());
        return;
    }
    file.write(ui->txtStyle->toPlainText().toLatin1());
    file.close();
}


void StyleEditorDialog::on_btnRevert_clicked()
{
    if (QMessageBox::warning(this, "Confirm", "Are you sure?", QMessageBox::Yes| QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    loadStyle();
}


void StyleEditorDialog::on_txtStyle_textChanged()
{
    applyStyle();
}

void StyleEditorDialog::loadStyle () {
    QFile file("stylesheet-dev.css");
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, "Load Error", file.errorString());
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    ui->txtStyle->setPlainText(QString::fromLatin1(data));
    applyStyle();
}


void StyleEditorDialog::applyStyle () {
    qApp->setStyleSheet(ui->txtStyle->toPlainText());
}
