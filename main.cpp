#include "gameconfig.h"
#include "startwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    StartWindow w;
    GameCfg cfg("config.cfg");
    cfg.print_config();
    w.show();
    return a.exec();
}
