#include "gamecfg/gameconfig.h"
#include "ui/editwindow.h"
#include "ui/startwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    StartWindow w;
    EditWindow e;
    GameCfg cfg("config.cfg");
    cfg.print_config();
    w.show();
    e.show();
    return a.exec();
}
