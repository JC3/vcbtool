#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTextStream>
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
    on_chkROMCSV_toggled(ui_->chkROMCSV->isChecked());
    setWindowTitle(windowTitle() + " " + VCBTOOL_VERSION);

    int defFontIndex = 0;
    QFile fontsJson("fonts.json");
    if (!fontsJson.open(QFile::ReadOnly | QFile::Text))
        QMessageBox::critical(this, "Error", "Error loading fonts.json: " + fontsJson.errorString());
    else {
        QJsonObject data = QJsonDocument::fromJson(fontsJson.readAll()).object();
        int index = 0;
        for (QString name : data.keys()) {
            QJsonObject jdesc = data[name].toObject();
            FontDesc desc;
            desc.filename = jdesc["file"].toString();
            desc.charset = jdesc["charset"].toString();
            desc.kerning = jdesc["kerning"].toInt(0);
            fonts_[name] = desc;
            ui_->cbTextFont->addItem(name);
            if (name == "3x5") defFontIndex = index;
            ++ index;
        }
    }
    ui_->cbTextFont->setCurrentIndex(defFontIndex);

    QSettings s;
    ui_->tabWidget->setCurrentIndex(s.value("tab", 0).toInt());
    if (s.contains("geometry"))
        setGeometry(s.value("geometry").toRect());
    ui_->actAlwaysOnTop->setChecked(s.value("top", false).toBool());

}

MainWindow::~MainWindow()
{

    QSettings s;
    s.setValue("tab", ui_->tabWidget->currentIndex());
    s.setValue("geometry", geometry());
    s.setValue("top", ui_->actAlwaysOnTop->isChecked());

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
        romfile_ = filename;
        romdata_ = file.readAll();
        ui_->lblROMFileInfo->setText(QString("%2 (%1 bytes)").arg(romdata_.size()).arg(QFileInfo(file).fileName()));
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


static bool readCSVRow (QTextStream &in, QStringList *row) {

    static const int delta[][5] = {
        //  ,    "   \n    ?  eof
        {   1,   2,  -1,   0,  -1  }, // 0: parsing (store char)
        {   1,   2,  -1,   0,  -1  }, // 1: parsing (store column)
        {   3,   4,   3,   3,  -2  }, // 2: quote entered (no-op)
        {   3,   4,   3,   3,  -2  }, // 3: parsing inside quotes (store char)
        {   1,   3,  -1,   0,  -1  }, // 4: quote exited (no-op)
        // -1: end of row, store column, success
        // -2: eof inside quotes
    };

    row->clear();

    if (in.atEnd())
        return false;

    int state = 0, t;
    char ch;
    QString cell;

    while (state >= 0) {

        if (in.atEnd())
            t = 4;
        else {
            in >> ch;
            if (ch == ',') t = 0;
            else if (ch == '\"') t = 1;
            else if (ch == '\n') t = 2;
            else t = 3;
        }

        state = delta[state][t];

        if (state == 0 || state == 3) {
            cell += ch;
        } else if (state == -1 || state == 1) {
            row->append(cell);
            cell = "";
        }

    }

    if (state == -2)
        throw runtime_error("End-of-file found while inside quotes.");

    return true;

}


void MainWindow::on_btnROMGenerate_clicked()
{
    try {

        int wordSize = ui_->spnROMWordSize->value();
        bool bigEndian = (ui_->cbROMByteOrder->currentIndex() == 0);
        int dataBits = ui_->spnROMDataBits->value();
        int addrBits = ui_->spnROMAddrBits->value();
        Circuits::ROMDataLSBSide dataLSB = (Circuits::ROMDataLSBSide)ui_->cbROMDataLSB->currentIndex();
        Circuits::ROMAddress0Side addr0Side = (Circuits::ROMAddress0Side)ui_->cbAddress0->currentIndex();
        int skipRows = ui_->spnROMSkipRows->value();
        bool omitEmpty = ui_->chkROMOmit->isChecked();

        QVector<quint64> data;

        if (ui_->chkROMCSV->isChecked()) {

            if (romfile_ == "")
                throw runtime_error("For CSV mode, you must choose an input file.");

            QFile csv(romfile_);
            csv.open(QFile::ReadOnly | QFile::Text);

            QTextStream in(&csv);
            QStringList row;
            while (readCSVRow(in, &row)) {
                if (skipRows-- > 0)
                    continue;
                if (row.empty())
                    continue;
                int address = row[0].toInt();
                if (address >= data.size())
                    data.resize(address + 1, 0);
                quint64 value = 0;
                for (int k = 1; k < row.length(); ++ k) {
                    int bit = (row[k].toInt() ? 1 : 0);
                    value = (value << 1) | bit;
                }
                data[address] = value;
                qDebug() << address << value;
            }

        } else {

            if (romdata_.isEmpty())
                ui_->lblROMWarning->setText("No data file loaded, ROM will be empty.");

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

            for (int j = 0; j < romdata_.size(); j += wordSize)
                data.append(getWord(j));

        }

        Blueprint *bp = Circuits::ROM(addrBits, dataBits, dataLSB, addr0Side, data, omitEmpty);
        ui_->txtROMBP->setPlainText(bp->bpString());
        delete bp;

    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_chkROMCSV_toggled(bool checked)
{
    ui_->cbROMByteOrder->setEnabled(ui_->spnROMWordSize->value() > 1 && !checked);
    ui_->spnROMWordSize->setEnabled(!checked);
    ui_->spnROMSkipRows->setEnabled(checked);
}


void MainWindow::on_spnROMWordSize_valueChanged(int arg1)
{
    ui_->cbROMByteOrder->setEnabled(arg1 > 1 && !ui_->chkROMCSV->isChecked());
}


void MainWindow::on_spnROMDataBits_valueChanged(int arg1)
{
    ui_->cbROMDataLSB->setEnabled(arg1 > 1);
}


void MainWindow::on_cbAddress0_activated(int index)
{
    ui_->chkROMOmit->setEnabled(index == (int)Circuits::Near);
}


void MainWindow::on_btnNetlistCheck_clicked()
{
    try {
        Blueprint bp(ui_->txtNetlistBP->toPlainText());
        Compiler c(&bp);
        Compiler::AnalysisSettings s;
        s.checkTraces = ui_->chkUnconnectedTraces->isChecked();
        s.checkGates = ui_->chk2InputGates->isChecked();
        s.checkCrosses = ui_->chkMissingCrosses->isChecked();
        //ui_->txtNetlistOut->setPlainText(c.analyzeCircuit(s).join("\n"));
        QStringList messages = c.analyzeCircuit(s);
        messages += Compiler::analyzeBlueprint(s, &bp);
        QString indexed;
        for (int k = 0; k < messages.size(); ++ k)
            indexed += QString::asprintf("%3d) %s\n", k + 1, messages[k].toLatin1().constData());
        ui_->txtNetlistOut->setPlainText(indexed);
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_btnNetlistGraph_clicked()
{
    try {
        Blueprint bp(ui_->txtNetlistBP->toPlainText());
        Compiler c(&bp);
        Compiler::GraphSettings s;
        s.compressed = ui_->chkCleanGraph->isChecked();
        s.ioclusters = ui_->chkClusterIO->isChecked();
        s.timings = ui_->chkClusterTiming->isChecked();
        s.timinglabels = ui_->chkLabelTiming->isChecked();
        s.positions = (Compiler::GraphSettings::PosMode)ui_->cbPositions->currentIndex();
        s.scale = ui_->txtPosScale->text().toFloat();
        s.squareio = ui_->chkSquareIO->isChecked();
        Compiler::GraphResults r = c.buildGraphViz(s);
        ui_->txtNetlistOut->setPlainText(r.graphviz.join("\n"));
        if (r.stats.critpathlen != -1) {
            QString str = QString("minmax=%1 maxmin=%2 crit=%3").arg(r.stats.minmaxtime).arg(r.stats.maxmintime).arg(r.stats.critpathlen);
            ui_->lblGraphStats->setText(str);
        } else
            ui_->lblGraphStats->setText("-");
    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}


void MainWindow::on_cbTextFont_activated(int)
{
    doGenerateText();
}


void MainWindow::on_txtTextContent_textChanged(const QString &)
{
    doGenerateText();
}


void MainWindow::on_chkTextLogic_toggled(bool)
{
    doGenerateText();
}


void MainWindow::on_chkTextDecoOn_toggled(bool)
{
    doGenerateText();
}


void MainWindow::on_chkTextDecoOff_toggled(bool)
{
    doGenerateText();
}


void MainWindow::on_cbTextLogicInk_currentIndexChanged(int)
{
    doGenerateText();
}


void MainWindow::on_clrTextDecoOn_colorChanged(const QColor &)
{
    doGenerateText();
}


void MainWindow::on_clrTextDecoOff_colorChanged(const QColor &)
{
    doGenerateText();
}


void MainWindow::doGenerateText () {
    try {

        static const QMap<int,Blueprint::Ink> LogicInks = {
            { 0, Blueprint::Annotation },
            { 1, Blueprint::Filler },
            { 2, Blueprint::LED }
        };

        QString fontfile = fonts_[ui_->cbTextFont->currentText()].filename;
        if (fontfile == "")
            throw runtime_error("invalid internal font id");
        QImage fontimage;
        if (!fontimage.load(fontfile))
            throw runtime_error(("couldn't load " + fontfile).toStdString());
        QString charset = fonts_[ui_->cbTextFont->currentText()].charset;
        int kerning = fonts_[ui_->cbTextFont->currentText()].kerning;

        QString text = ui_->txtTextContent->text();
        Blueprint::Ink logicInk, onInk, offInk;
        if (ui_->chkTextLogic->isChecked())
            logicInk = LogicInks[ui_->cbTextLogicInk->currentIndex()];
        if (ui_->chkTextDecoOn->isChecked())
            onInk = ui_->clrTextDecoOn->selectedColor();
        if (ui_->chkTextDecoOff->isChecked())
            offInk = ui_->clrTextDecoOff->selectedColor();

        Blueprint *bp = Circuits::Text(fontimage, charset, kerning, text, logicInk, onInk, offInk);
        ui_->txtTextBP->setPlainText(bp->bpString());

        if (ui_->chkTextAutoCopy->isChecked())
            QGuiApplication::clipboard()->setText(bp->bpString());

        delete bp;

    } catch (const std::exception &x) {
        QMessageBox::critical(this, "Error", x.what());
    }
}

void MainWindow::on_actAlwaysOnTop_toggled(bool checked)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    show();
}


void MainWindow::on_btnROMCSVHelp_clicked()
{
    QMessageBox::information(this, "CSV File Format", "Each row represents an entry. The first column must contain the 0-based decimal address of the entry. Each remaining column contains a 0 or a 1. The last column will be the LSB of the data.");
}


void MainWindow::on_btnMiscGray8_clicked()
{
    QStringList colors;
    for (int g = 0; g <= 255; ++ g)
        colors.append(QString::asprintf("%02x%02x%02x", g, g, g));
    ui_->txtMisc->setPlainText(colors.join(", "));
}


void MainWindow::on_btnMiscRGB332_clicked()
{
    static const auto rescale = [] (int v, int oldmax, int newmax) {
        return qRound((float)newmax * (float)v / (float)oldmax);
    };
    QStringList colors;
    for (int k = 0; k <= 255; ++ k) {
        int r = rescale((k >> 5) & 7, 7, 255);
        int g = rescale((k >> 2) & 7, 7, 255);
        int b = rescale((k >> 0) & 3, 3, 255);
        colors.append(QString::asprintf("%02x%02x%02x", r, g, b));
    }
    ui_->txtMisc->setPlainText(colors.join(", "));
}


void MainWindow::on_btnMiscX11_clicked()
{
    const char palette[] =
            "000000, 800000, 008000, 808000, 000080, 800080, 008080, c0c0c0, "
            "808080, ff0000, 00ff00, ffff00, 0000ff, ff00ff, 00ffff, ffffff, "
            "000000, 00005f, 000087, 0000af, 0000d7, 0000ff, 005f00, 005f5f, "
            "005f87, 005faf, 005fd7, 005fff, 008700, 00875f, 008787, 0087af, "
            "0087d7, 0087ff, 00af00, 00af5f, 00af87, 00afaf, 00afd7, 00afff, "
            "00d700, 00d75f, 00d787, 00d7af, 00d7d7, 00d7ff, 00ff00, 00ff5f, "
            "00ff87, 00ffaf, 00ffd7, 00ffff, 5f0000, 5f005f, 5f0087, 5f00af, "
            "5f00d7, 5f00ff, 5f5f00, 5f5f5f, 5f5f87, 5f5faf, 5f5fd7, 5f5fff, "
            "5f8700, 5f875f, 5f8787, 5f87af, 5f87d7, 5f87ff, 5faf00, 5faf5f, "
            "5faf87, 5fafaf, 5fafd7, 5fafff, 5fd700, 5fd75f, 5fd787, 5fd7af, "
            "5fd7d7, 5fd7ff, 5fff00, 5fff5f, 5fff87, 5fffaf, 5fffd7, 5fffff, "
            "870000, 87005f, 870087, 8700af, 8700d7, 8700ff, 875f00, 875f5f, "
            "875f87, 875faf, 875fd7, 875fff, 878700, 87875f, 878787, 8787af, "
            "8787d7, 8787ff, 87af00, 87af5f, 87af87, 87afaf, 87afd7, 87afff, "
            "87d700, 87d75f, 87d787, 87d7af, 87d7d7, 87d7ff, 87ff00, 87ff5f, "
            "87ff87, 87ffaf, 87ffd7, 87ffff, af0000, af005f, af0087, af00af, "
            "af00d7, af00ff, af5f00, af5f5f, af5f87, af5faf, af5fd7, af5fff, "
            "af8700, af875f, af8787, af87af, af87d7, af87ff, afaf00, afaf5f, "
            "afaf87, afafaf, afafd7, afafff, afd700, afd75f, afd787, afd7af, "
            "afd7d7, afd7ff, afff00, afff5f, afff87, afffaf, afffd7, afffff, "
            "d70000, d7005f, d70087, d700af, d700d7, d700ff, d75f00, d75f5f, "
            "d75f87, d75faf, d75fd7, d75fff, d78700, d7875f, d78787, d787af, "
            "d787d7, d787ff, d7af00, d7af5f, d7af87, d7afaf, d7afd7, d7afff, "
            "d7d700, d7d75f, d7d787, d7d7af, d7d7d7, d7d7ff, d7ff00, d7ff5f, "
            "d7ff87, d7ffaf, d7ffd7, d7ffff, ff0000, ff005f, ff0087, ff00af, "
            "ff00d7, ff00ff, ff5f00, ff5f5f, ff5f87, ff5faf, ff5fd7, ff5fff, "
            "ff8700, ff875f, ff8787, ff87af, ff87d7, ff87ff, ffaf00, ffaf5f, "
            "ffaf87, ffafaf, ffafd7, ffafff, ffd700, ffd75f, ffd787, ffd7af, "
            "ffd7d7, ffd7ff, ffff00, ffff5f, ffff87, ffffaf, ffffd7, ffffff, "
            "080808, 121212, 1c1c1c, 262626, 303030, 3a3a3a, 444444, 4e4e4e, "
            "585858, 626262, 6c6c6c, 767676, 808080, 8a8a8a, 949494, 9e9e9e, "
            "a8a8a8, b2b2b2, bcbcbc, c6c6c6, d0d0d0, dadada, e4e4e4, eeeeee";
    ui_->txtMisc->setPlainText(palette);
}

