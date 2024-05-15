#include "gamecfg/gameconfig.h"
#include "ui/startwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /***********************************************************/ // Font Family Name:
    QFontDatabase::addApplicationFont(":/font/tf2build.ttf");     // "TF2 Build"
    QFontDatabase::addApplicationFont(":/font/tf2professor.ttf"); // "TF2 Professor"
    QFontDatabase::addApplicationFont(":/font/tf2secondary.ttf"); // "TF2"
    /***********************************************************/ // from res.qrc

    StartWindow w;
    //  EditWindow e;

    GameCfg cfg("config.cfg");
    cfg.print_cfg();

    w.show();
    // e.show();

    return a.exec();
}
