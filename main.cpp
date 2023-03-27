#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION_MAJOR <= 5
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication::setOrganizationName("jasonc");
    QApplication::setApplicationName("vcbtool");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
