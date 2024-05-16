#include "ui/startwindow.h"
#include "ui_startwindow.h"

StartWindow::StartWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::StartWindow) {
    ui->setupUi(this);
    edit_window.hide();
}

StartWindow::~StartWindow() {
    delete ui;
}

void StartWindow::on_cfgPathBrowseButton_clicked() {
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

void StartWindow::on_cfgPresetBox_currentIndexChanged(int index) {
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

void StartWindow::changeWindow() {
    if (this->isVisible()) {
        edit_window.hide();
        this->show();
    } else {
        this->hide();
        edit_window.show();
    }
}

void StartWindow::on_startGoButton_clicked() {
    edit_window.load_cfg(ui->cfgPathEdit->text());
    edit_window.show();
    this->hide();
    // create main window

    // delete this;
}
