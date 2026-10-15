# QML test binary (Qt Quick Test) - the app's UI, driven headless against the real QML.
#
# Built separately from both the app and the engine tests, so a failure in any of the
# three never blocks the others:
#   cd build-qmltest && qmake6 -o Makefile ../tests/qml/qmltests.pro && mingw32-make -j8
#   QT_QPA_PLATFORM=offscreen ./release/tst_app.exe -o testlog.txt,txt
#
# The engine tests (tests/tests.pro) cover the parser and the models; this target covers
# what they cannot: QML that fails to load or import, what a delegate actually renders,
# and the focus/keyboard behavior the accessibility work (W1-W8) added.
QT += core gui quick quickcontrols2 testlib concurrent
CONFIG += console c++17 qmltestcase
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_app

INCLUDEPATH += ../../src

SOURCES += \
    ../../src/gamecfg/gameconfig.cpp \
    ../../src/gamecfg/lex/cmdlexer.cpp \
    ../../src/gamecfg/lex/configlexer.cpp \
    ../../src/ui/commandfiltermodel.cpp \
    ../../src/ui/commandlistmodel.cpp \
    ../../src/ui/configcontroller.cpp \
    ../../src/ui/consolecommandmodel.cpp \
    ../../src/ui/keytable.cpp \
    tst_app.cpp

HEADERS += \
    ../../src/gamecfg/gameconfig.h \
    ../../src/gamecfg/lex/lexers.h \
    ../../src/ui/commandfiltermodel.h \
    ../../src/ui/commandlistmodel.h \
    ../../src/ui/configcontroller.h \
    ../../src/ui/consolecommandmodel.h \
    ../../src/ui/keytable.h

# The app's own QML and its bundled data, so the tests walk the real tree.
RESOURCES += ../../qml.qrc
RESOURCES += ../../res/res.qrc
