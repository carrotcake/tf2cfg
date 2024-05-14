#include "startwindow.h"
#include "gameconfig.h"
#include "ui_startwindow.h"

StartWindow::StartWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::StartWindow)
{
    ui->setupUi(this);
}

StartWindow::~StartWindow()
{
    delete ui;
}

void StartWindow::on_cfgPathBrowseButton_clicked()
{
    QString file_path = QFileDialog::getOpenFileName(this,
                                                     tr("Open Config"),
                                                     "",
                                                     tr("Source Config File (*.cfg)"));
    if (!file_path.isEmpty()) {
        ui->cfgPresetBox->setCurrentIndex(-1);
        ui->cfgPathEdit->setText(file_path);
        ui->startGoButton->setEnabled(true);
    }
}

void StartWindow::on_cfgPresetBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
    case 1:
        ui->cfgPathEdit->setText(CFG_STRINGS[index]);
        ui->startGoButton->setEnabled(true);
        break;
    default:
        ui->cfgPathEdit->setText("");
        break;
    }
}

void StartWindow::on_startGoButton_clicked()
{
    //load config
    GameCfg config(ui->cfgPathEdit->text());
    config.print_config();
    //create main window

    this->destroy();
    // delete this;
}
