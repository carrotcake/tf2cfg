#include "ui/editwindow.h"
#include "ui/bindeditdialog.h"
#include "ui_editwindow.h"

EditWindow::EditWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::EditWindow) {
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
}

EditWindow::~EditWindow() {
    delete ui;
}

void EditWindow::loadCfg(const QString filename) {
    cfg.appendCfg(filename);
    cfg.printCfg();
    ui->listWidget->clear();
    for (const cmd::Entry &cmd : cfg.getCmdList()) {
        ui->listWidget->addItem(cmd.args.join(" "));
    }
}

void EditWindow::on_tabWidget_currentChanged(int index) {
    if (index == 0) {
        ui->btn_prev_page->setEnabled(false);
    } else {
        ui->btn_prev_page->setStatusTip(ui->tabWidget->tabText(ui->tabWidget->currentIndex() - 1));
        ui->btn_prev_page->setEnabled(true);
    }

    if (index == ui->tabWidget->count() - 1) {
        ui->btn_next_page->setEnabled(false);
    } else {
        ui->btn_next_page->setStatusTip(ui->tabWidget->tabText(ui->tabWidget->currentIndex() + 1));
        ui->btn_next_page->setEnabled(true);
    }
}

void EditWindow::on_btn_prev_page_clicked() {
    ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() - 1);
}

void EditWindow::on_btn_next_page_clicked() {
    ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() + 1);
}

void EditWindow::on_keybWidget_keybButtonClicked(int id) {
    QString keyCode = KeybForm::KEYCODES[id];
    printf("%s\n", keyCode.toStdString().c_str());
    fflush(stdout);
    BindEditDialog d;
    d.setOriginalBind("bind \"" + keyCode + "\"");
    d.exec();
}
