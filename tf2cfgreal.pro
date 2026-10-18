QT       += core gui quick quickcontrols2 concurrent

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ./src

# QML tooling (Qt Creator completion / qmllint) looks in these directories.
QML_IMPORT_PATH += qml
QML_DESIGNER_IMPORT_PATH += qml

# The C++ backend is a QML module (res/qml style: `import tf2cfg`), so the type
# metadata qmltyperegistrar writes lets qmllint/qmlls check `Cfg.` and `KeyTable.`
# accesses instead of treating them as unqualified. Runtime registration is generated
# too (tf2cfgreal_qmltyperegistrations.cpp), so the app registers nothing by hand.
CONFIG += qmltypes
QML_IMPORT_NAME = tf2cfg
QML_IMPORT_MAJOR_VERSION = 1

# qmltyperegistrar's generated registration includes each type's header by basename
# (`__has_include(<configcontroller.h>)`), so the directory has to be on the include path.
INCLUDEPATH += $$PWD/src/ui

# qmllint and qmlls resolve `import tf2cfg` through a module directory. qmltyperegistrar can
# write its .qmltypes straight into it (QMLTYPES_FILENAME), so the build needs no copy step:
# qmlmodules/tf2cfg/ ends up holding the qmldir and the file it names. Lint with
# `-I qmlmodules` (see .github/workflows/ci.yml). The .qmltypes is a byproduct of the
# registrar, so a build directory from before this setting keeps the old layout until its
# registrar output (tf2cfgreal_qmltyperegistrations.cpp) is deleted.
TF2CFG_QML_MODULES = $$OUT_PWD/qmlmodules
mkpath($$TF2CFG_QML_MODULES/tf2cfg)
QMLTYPES_FILENAME = $$TF2CFG_QML_MODULES/tf2cfg/tf2cfg.qmltypes
TF2CFG_QMLDIR = "module tf2cfg"
TF2CFG_QMLDIR += "typeinfo tf2cfg.qmltypes"
write_file($$TF2CFG_QML_MODULES/tf2cfg/qmldir, TF2CFG_QMLDIR)

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
