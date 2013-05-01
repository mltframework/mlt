QT       += testlib
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += testcase
TEMPLATE = app
DEFINES  += SRCDIR=\\\"$$PWD/\\\"

win32 {
    INCLUDEPATH += include/mlt++ include/mlt
    LIBS += -Llib -lmlt++ -lmlt
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++
}
