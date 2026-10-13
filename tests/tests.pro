# Engine test binary. Built separately from the app so a failing test never blocks
# a release build:
#   cd build-test && qmake ../tests/tests.pro && mingw32-make && release/tf2cfg_test.exe
QT += core testlib

TEMPLATE = app
TARGET = tf2cfg_test
CONFIG += console c++17
CONFIG -= app_bundle

INCLUDEPATH += ../src

SOURCES += \
    ../src/gamecfg/gameconfig.cpp \
    ../src/gamecfg/lex/cmdlexer.cpp \
    ../src/gamecfg/lex/configlexer.cpp \
    ../src/ui/commandlistmodel.cpp \
    tst_engine.cpp

HEADERS += \
    ../src/gamecfg/gameconfig.h \
    ../src/gamecfg/lex/lexers.h \
    ../src/ui/commandlistmodel.h

# GameCfg reads the command catalog from the resource bundle, so the tests get it
# from the same place the app does instead of depending on the working directory.
RESOURCES += ../res/res.qrc