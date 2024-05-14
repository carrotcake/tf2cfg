#include "editwindow.h"
#include "ui_editwindow.h"

EditWindow::EditWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::EditWindow)
{
    ui->setupUi(this);
}

EditWindow::~EditWindow()
{
    delete ui;
}
