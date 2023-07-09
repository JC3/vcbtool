#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox> // beta temp
#include <QFile> // beta temp

int main(int argc, char *argv[])
{

    /* beta temp load stylesheet */
    QString css;
    QFile file("stylesheet-dev.css");
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(nullptr, "Stylesheet Load Error", "stylesheet-dev.css: " + file.errorString());
    } else {
        QByteArray data = file.readAll();
        css = QString::fromLatin1(data);
    }
    /* end temp load stylesheet */

#if QT_VERSION_MAJOR <= 5
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication::setOrganizationName("jasonc");
    QApplication::setApplicationName("vcbtool");
    QApplication a(argc, argv);
    a.setStyleSheet(css); // beta temp
    bool debugMode = true; // beta temp // false;
    for (int a = 1; a < argc; ++ a)
        if (!strcmp(argv[a], "--debug"))
            debugMode = true;
    MainWindow w(debugMode);
    w.show();
    return a.exec();
}
