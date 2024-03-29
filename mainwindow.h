#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "blueprint.h"
#include "styleeditordialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(bool debugMode, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConvertImage_clicked();
    void on_btnConvertBP_clicked();
    void on_btnLoadROMFile_clicked();
    void on_spnROMWordSize_valueChanged(int arg1);
    void on_spnROMDataBits_valueChanged(int arg1);
    void on_btnROMGenerate_clicked();
    void on_btnNetlistCheck_clicked();
    void on_btnNetlistGraph_clicked();
    void on_cbTextFont_activated(int index);
    void on_txtTextContent_textChanged(const QString &arg1);
    void on_chkTextLogic_toggled(bool checked);
    void on_chkTextDecoOn_toggled(bool checked);
    void on_chkTextDecoOff_toggled(bool checked);
    void on_cbTextLogicInk_currentIndexChanged(int index);
    void on_clrTextDecoOn_colorChanged(const QColor &);
    void on_clrTextDecoOff_colorChanged(const QColor &);
    void on_actAlwaysOnTop_toggled(bool checked);
    void on_chkROMCSV_toggled(bool checked);
    void on_btnROMCSVHelp_clicked();
    void on_btnMiscGray8_clicked();
    void on_btnMiscRGB332_clicked();
    void on_cbAddress0_activated(int index);
    void on_btnMiscX11_clicked();
    void on_btnConvertEmoji_clicked();
    void on_cbSystemFont_activated(int index);
    void on_spnSystemFontHeight_valueChanged(int arg1);
    void on_btnFontSystem_toggled(bool checked);
    void on_btnFontBuiltIn_toggled(bool checked);
    void on_actLatestRelease_triggered();
    void on_actBugReports_triggered();
    void on_actStyleEditor_triggered();

    void on_btnViewGraph_clicked();

private:

    struct FontDesc {
        QString filename;
        QString charset;
        int kerning;
        FontDesc () : kerning(0) { }
    };

    Ui::MainWindow *ui_;
    StyleEditorDialog *sedit_;
    QString romfile_;
    QByteArray romdata_;
    QMap<QString,FontDesc> fonts_;
    Blueprint::Layer selectedConversionLayer () const;
    void doGenerateText ();
};
#endif // MAINWINDOW_H
