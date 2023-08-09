QMAKE_TARGET_DESCRIPTION = "VCB Tool"
VERSION = 1.8.4
DEFINES += VCBTOOL_VERSION='\\"$$VERSION\\"'

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    blueprint.cpp \
    circuits.cpp \
    colorselector.cpp \
    compiler.cpp \
    main.cpp \
    mainwindow.cpp \
    styleeditordialog.cpp

HEADERS += \
    blueprint.h \
    circuits.h \
    colorselector.h \
    compiler.h \
    mainwindow.h \
    styleeditordialog.h

FORMS += \
    mainwindow.ui \
    styleeditordialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -L$$PWD/contrib/zstd/static/ -llibzstd_static

INCLUDEPATH += $$PWD/contrib/zstd/include
DEPENDPATH += $$PWD/contrib/zstd/include

win32:!win32-g++: PRE_TARGETDEPS +=   # zstd static lib cannot link with msvc
else:win32-g++: PRE_TARGETDEPS += $$PWD/contrib/zstd/static/libzstd_static.lib

DISTFILES += \
    README.md \
    deploy.bat \
    font_3x4.png \
    font_3x5.png \
    font_4x5.png \
    font_5x7.png \
    font_fixedsys.png \
    font_fixedsys12.png \
    font_topaz.png \
    font_topaz_sans.png \
    fonts.json \
    stylesheet-dev.css
