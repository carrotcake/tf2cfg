#ifndef BINDEDITDIALOG_H
#define BINDEDITDIALOG_H

#include <QDialog>

namespace Ui {
class BindEditDialog;
}

class BindEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit BindEditDialog(QWidget *parent = nullptr);
    ~BindEditDialog();

    void set_original_bind(const QString str);

private slots:
    void on_BindEditDialog_accepted();

    void on_BindEditDialog_rejected();

private:
    Ui::BindEditDialog *ui;
};

#endif // BINDEDITDIALOG_H
