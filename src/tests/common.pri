QT       += testlib
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += testcase
CONFIG   += c++11
QMAKE_CXXFLAGS += -std=c++11
TEMPLATE = app
DEFINES  += SRCDIR=\\\"$$PWD/\\\"

win32 {
    INCLUDEPATH += $$PWD/..
    LIBS += -L$$PWD/../framework -L$$PWD/../mlt++ -lmlt++ -lmlt
    isEmpty(PREFIX) {
        message("Install PREFIX not set; using C:\\Projects\\Shotcut. You can change this with 'qmake PREFIX=...'")
        PREFIX = C:\\Projects\\Shotcut
    }
    target.path = $$PREFIX
    INSTALLS += target
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += mlt++-7
}
