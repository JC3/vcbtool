#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <stdexcept>

using std::runtime_error;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lblROMWarning->setText("");
}

MainWindow::~MainWindow()
{
    delete ui;
}

Blueprint::Layer MainWindow::selectedConversionLayer () const {
    if (ui->optLayerDecoOn->isChecked())
        return Blueprint::DecoOn;
    else if (ui->optLayerDecoOff->isChecked())
        return Blueprint::DecoOff;
    else
        return Blueprint::Logic;
}

void MainWindow::on_btnConvertImage_clicked()
{
    try {
        QString filename = QFileDialog::getOpenFileName(this, "Load Image");
        if (filename == "")
            return;
        QImage bpImage;
        if (!bpImage.load(filename))
            throw runtime_error("Failed to load image.");
        Blueprint::Layer layer = selectedConversionLayer();
        Blueprint bp(bpImage, layer);
        ui->txtConvertedBP->setPlainText(bp.bpString());
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_btnConvertBP_clicked()
{
    try {
        Blueprint bp(ui->txtConvertedBP->toPlainText());
        Blueprint::Layer layer = selectedConversionLayer();
        QImage bpImage = bp.layer(layer);
        QString filename = QFileDialog::getSaveFileName(this, "Save Image");
        if (filename == "")
            return;
        if (!bpImage.save(filename))
            throw runtime_error("Failed to save image.");
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_btnLoadROMFile_clicked()
{

}


void MainWindow::on_btnROMGenerate_clicked()
{

}


void MainWindow::on_spnROMWordSize_valueChanged(int arg1)
{
    ui->cbROMByteOrder->setEnabled(arg1 > 1);
}


void MainWindow::on_spnROMDataBits_valueChanged(int arg1)
{
    ui->cbROMBitOrder->setEnabled(arg1 > 1);
}

