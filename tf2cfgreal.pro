QT       += core gui quick quickcontrols2 concurrent

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ./src

# QML tooling (Qt Creator completion / qmllint) looks in these directories.
QML_IMPORT_PATH += qml
QML_DESIGNER_IMPORT_PATH += qml

SOURCES += \
    src/main.cpp \
    src/gamecfg/cvar.cpp \
    src/gamecfg/gameconfig.cpp \
    src/gamecfg/lex/cmdlexer.cpp \
    src/gamecfg/lex/configlexer.cpp \
    src/ui/commandlistmodel.cpp \
    src/ui/consolecommandmodel.cpp \
    src/ui/configcontroller.cpp

HEADERS += \
    src/gamecfg/convars.h \
    src/gamecfg/gameconfig.h \
    src/gamecfg/lex/lexers.h \
    src/ui/commandlistmodel.h \
    src/ui/consolecommandmodel.h \
    src/ui/configcontroller.h

# The previous Qt Widgets UI is kept for reference but is not built.
# It lives in src/legacy/widgets/ - see the README there.

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

STATECHARTS +=

RESOURCES += \
    res/res.qrc \
    qml.qrc
