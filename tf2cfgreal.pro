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
    src/ui/configcontroller.cpp \
    src/ui/keytable.cpp

HEADERS += \
    src/gamecfg/convars.h \
    src/gamecfg/gameconfig.h \
    src/gamecfg/lex/lexers.h \
    src/ui/commandfiltermodel.h \
    src/ui/commandlistmodel.h \
    src/ui/consolecommandmodel.h \
    src/ui/configcontroller.h \
    src/ui/keytable.h

# The previous Qt Widgets UI was retired and deleted once the QML rewrite shipped;
# git history has it. See docs/legacy/ui-qml-rewrite.md section 11.
# The engine test binary is a separate project: tests/tests.pro

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Windows: bake the app icon into the exe (qmake runs windres). The .ico is
# generated at build time by release.yml's Windows job and must exist in
# res/icon/ before qmake; local Windows builds render it with the same magick
# script (see .github/workflows/release.yml, "Render the app icon").
win32: RC_FILE = res/icon/tf2cfg.rc

STATECHARTS +=

RESOURCES += \
    res/res.qrc \
    qml.qrc
