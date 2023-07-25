#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class Maindialog; }
QT_END_NAMESPACE

class Maindialog : public QDialog
{
    Q_OBJECT
signals:
    void SIG_close();
    void SIG_registerCommit(QString,QString,QString);
    void SIG_loginCommit(QString,QString);
public:
    Maindialog(QWidget *parent = nullptr);
    ~Maindialog();

    void closeEvent(QCloseEvent* event)override;

private slots:

    void on_pb_clear_register_clicked();

    void on_pb_register_register_clicked();

    void on_pb_clear_login_clicked();

    void on_pb_login_login_clicked();

private:
    Ui::Maindialog *ui;
};
#endif // MAINDIALOG_H

//点击 x--> 执行关闭事件  --》弹窗询问  --》发送关闭信号  --》核心类接收，然后回收资源
