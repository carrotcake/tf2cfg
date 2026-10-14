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
    src/gamecfg/gameconfig.cpp \
    src/gamecfg/lex/cmdlexer.cpp \
    src/gamecfg/lex/configlexer.cpp \
    src/ui/commandfiltermodel.cpp \
    src/ui/commandlistmodel.cpp \
    src/ui/consolecommandmodel.cpp \
    src/ui/configcontroller.cpp

HEADERS += \
    src/gamecfg/convars.h \
    src/gamecfg/gameconfig.h \
    src/gamecfg/lex/lexers.h \
    src/ui/commandfiltermodel.h \
    src/ui/commandlistmodel.h \
    src/ui/consolecommandmodel.h \
    src/ui/configcontroller.h

# The previous Qt Widgets UI was retired and deleted once the QML rewrite shipped;
# git history has it. See docs/legacy/ui-qml-rewrite.md section 11.
# The engine test binary is a separate project: tests/tests.pro

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

STATECHARTS +=

RESOURCES += \
    res/res.qrc \
    qml.qrc
