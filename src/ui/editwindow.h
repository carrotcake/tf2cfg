#ifndef EDITWINDOW_H
#define EDITWINDOW_H

#include <QMainWindow>
#include "gamecfg/gameconfig.h"

namespace Ui {
class EditWindow;
}

class EditWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EditWindow(QWidget *parent = nullptr);
    ~EditWindow();

    void load_cfg(const QString filename);

private slots:
    void on_tabWidget_currentChanged(int index);

    void on_btn_prev_page_clicked();

    void on_btn_next_page_clicked();

private:
    Ui::EditWindow *ui;
    GameCfg cfg;
};

#endif // EDITWINDOW_H
