#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "blueprint.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConvertImage_clicked();
    void on_btnConvertBP_clicked();
    void on_btnLoadROMFile_clicked();
    void on_spnROMWordSize_valueChanged(int arg1);
    void on_spnROMDataBits_valueChanged(int arg1);
    void on_btnROMGenerate_clicked();
private:
    Ui::MainWindow *ui_;
    QByteArray romdata_;
    Blueprint::Layer selectedConversionLayer () const;
};
#endif // MAINWINDOW_H
