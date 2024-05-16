#ifndef STARTWINDOW_H
#define STARTWINDOW_H

#include <QFileDialog>
#include <QMainWindow>
#include "ui/editwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class StartWindow;
}
QT_END_NAMESPACE

class StartWindow : public QMainWindow {
    Q_OBJECT

public:
    StartWindow(QWidget *parent = nullptr);
    ~StartWindow();

private slots:
    void on_cfgPathBrowseButton_clicked();

    void on_cfgPresetBox_currentIndexChanged(int index);

    void on_startGoButton_clicked();

    void changeWindow();

private:
    inline static const QString CFG_STRINGS[2] = {":/cfg/b4nny.cfg", ":/cfg/default.cfg"};
    Ui::StartWindow *ui;
    EditWindow edit_window;
};
#endif // STARTWINDOW_H
