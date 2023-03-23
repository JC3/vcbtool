#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <stdexcept>
#include "circuits.h"
#include "compiler.h"

using std::runtime_error;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    ui_->lblROMWarning->setText("");
}

MainWindow::~MainWindow()
{
    delete ui_;
}

Blueprint::Layer MainWindow::selectedConversionLayer () const {
    if (ui_->optLayerDecoOn->isChecked())
        return Blueprint::DecoOn;
    else if (ui_->optLayerDecoOff->isChecked())
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
        ui_->txtConvertedBP->setPlainText(bp.bpString());
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_btnConvertBP_clicked()
{
    try {
        Blueprint bp(ui_->txtConvertedBP->toPlainText());
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
    try {
        ui_->lblROMWarning->setText("");
        QString filename = QFileDialog::getOpenFileName(this, "Load ROM Data File");
        QFile file(filename);
        if (!file.open(QFile::ReadOnly))
            throw runtime_error(file.errorString().toStdString());
        romdata_ = file.readAll();
        ui_->lblROMFileInfo->setText(QString("%2 (%1 bytes)").arg(romdata_.size()).arg(QFileInfo(file).fileName()));
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_btnROMGenerate_clicked()
{
    try {

        if (romdata_.isEmpty())
            ui_->lblROMWarning->setText("No data file loaded, ROM will be empty.");

        int wordSize = ui_->spnROMWordSize->value();
        bool bigEndian = (ui_->cbROMByteOrder->currentIndex() == 0);
        int dataBits = ui_->spnROMDataBits->value();
        int addrBits = ui_->spnROMAddrBits->value();
        //bool reverseData = (ui_->cbROMBitOrder->currentIndex() == 0); // Circuits::ROM puts msb near address lines

        const auto getWord = [&] (int offset) {
            if (offset < 0 || offset >= romdata_.size())
                return 0ULL;
            quint64 word = 0;
            if (bigEndian) {
                for (int k = offset; k < offset + wordSize; ++ k) {
                    quint8 b = (k < romdata_.size() ? romdata_[k] : 0);
                    word = (word << 8) | b;
                }
            } else {
                for (int k = offset + wordSize - 1; k >= offset; -- k) {
                    quint8 b = (k < romdata_.size() ? romdata_[k] : 0);
                    word = (word << 8) | b;
                }
            }
            return word;
        };

        QVector<quint64> data;
        for (int j = 0; j < romdata_.size(); j += wordSize)
            data.append(getWord(j));

        Blueprint *bp = Circuits::ROM(addrBits, dataBits, data);
        ui_->txtROMBP->setPlainText(bp->bpString());
        delete bp;

    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_spnROMWordSize_valueChanged(int arg1)
{
    ui_->cbROMByteOrder->setEnabled(arg1 > 1);
}


void MainWindow::on_spnROMDataBits_valueChanged(int arg1)
{
    //ui_->cbROMBitOrder->setEnabled(arg1 > 1);
}


void MainWindow::on_btnNetlistGenerate_clicked()
{
    try {
        Blueprint bp(ui_->txtNetlistBP->toPlainText());
        Compiler c(&bp);
        //qDebug().noquote() << c.buildDot().join("\n");
        QFile f("dotgraph.txt");
        f.open(QFile::WriteOnly  |QFile::Text);
        f.write(c.buildDot().join("\n").toLatin1());
        f.close();
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}

