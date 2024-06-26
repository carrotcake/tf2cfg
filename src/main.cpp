#include "ui/startwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFontDatabase>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    /********************************************************************/   // Font Family:
    QFontDatabase::addApplicationFont("://font/tf2build.ttf");               // "TF2 Build"
    QFontDatabase::addApplicationFont("://font/tf2professor.ttf");           // "TF2 Professor"
    QFontDatabase::addApplicationFont("://font/tf2secondary.ttf");           // "TF2"
    QFontDatabase::addApplicationFont("://font/LiberationMono-Regular.ttf"); // "Liberation Mono"
    /********************************************************************/   // from res.qrc

    StartWindow w;

    w.show();

    return a.exec();
}
