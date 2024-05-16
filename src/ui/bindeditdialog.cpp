#include "ui/bindeditdialog.h"
#include "ui_bindeditdialog.h"

BindEditDialog::BindEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BindEditDialog)
{
    ui->setupUi(this);
}

BindEditDialog::~BindEditDialog()
{
    delete ui;
}

void BindEditDialog::set_original_bind(const QString str)
{
    ui->lineEdit->setText(str);
};

void BindEditDialog::on_BindEditDialog_accepted() {}

void BindEditDialog::on_BindEditDialog_rejected() {}
