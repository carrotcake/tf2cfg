QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VPATH += ./src

INCLUDEPATH += ./src

SOURCES += \
    main.cpp \
    src/gamecfg/cvar.cpp \
    src/gamecfg/lex/cmdlexer.cpp \
    ui/bindeditdialog.cpp \
    ui/editwindow.cpp \
    ui/keybform.cpp \
    ui/startwindow.cpp \
    gamecfg/lex/configlexer.cpp \
    gamecfg/gameconfig.cpp

HEADERS += \
    src/gamecfg/convars.h \
    ui/bindeditdialog.h \
    ui/editwindow.h \
    ui/keybform.h \
    ui/startwindow.h \
    gamecfg/lex/lexers.h \
    gamecfg/gameconfig.h

FORMS += \
    ui/bindeditdialog.ui \
    ui/editwindow.ui \
    ui/keybform.ui \
    ui/startwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

STATECHARTS +=

RESOURCES += \
    res/res.qrc
