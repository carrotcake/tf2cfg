#include "gamecfg/gameconfig.h"
#include "ui/editwindow.h"
#include "ui/startwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFontDatabase::addApplicationFont(":/font/tf2build.ttf");
    QFontDatabase::addApplicationFont(":/font/tf2professor.ttf");
    QFontDatabase::addApplicationFont(":/font/tf2secondary.ttf");

    StartWindow w;
    EditWindow e;
    GameCfg cfg("config.cfg");
    cfg.print_config();
    w.show();
    e.show();
    return a.exec();
}
