#include "maindialog.h"
#include "ui_maindialog.h"
#include <QMessageBox>
#include <QRegExp>
Maindialog::Maindialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Maindialog)
{
    ui->setupUi(this);
    setWindowTitle("登录&注册");

    //窗口默认为登录
    ui->tw_page->setCurrentIndex(0);
}

Maindialog::~Maindialog()
{
    delete ui;
}

void Maindialog::closeEvent(QCloseEvent *event)  //closeEvent事件
{
    if(QMessageBox::question(this,"退出提示","是否退出?") == QMessageBox::Yes)
    {
        //关闭
        event->accept();   //表示该事件已被处理，不用再向父类控件传递
        Q_EMIT SIG_close();
    }
    else
    {
        event->ignore();  //忽视该控件，向父类控件传递该事件
    }
}





void Maindialog::on_pb_clear_register_clicked()
{
    ui->le_tel_register->setText("");
    ui->le_password_register->setText("");
    ui->le_confirm_register->setText("");
    ui->le_name_register->setText("");
}


void Maindialog::on_pb_register_register_clicked()
{

    QString tel = ui->le_tel_register->text();
    QString password = ui->le_password_register->text();
    QString confirm = ui->le_confirm_register->text();
    QString name = ui->le_name_register->text();
    QString tempName = name;
    //过滤
    //查看是否输入为空
    if(tel.isEmpty() || password.isEmpty() || confirm.isEmpty() || tempName.remove(" ").isEmpty())
    {
        QMessageBox::about(this,"提示","输入内容不能为空");
    }
    //手机号是 否合法  --正则表达式
    QRegExp exp( "^1[256789][0-9]\{9\}$");
    bool res=exp.exactMatch(tel);
    if(!res)
    {
        QMessageBox::about(this,"提示","手机号非法");
        return;
    }

    //密码是否过长
    if(password.size() > 20)
    {
        QMessageBox::about(this,"提示","手机号非法");
        return ;
    }

    //密码确认要一致
    if(confirm != password){
        QMessageBox::about(this,"提示","两次输入不一定");
        return;
    }
    //昵称 是否过程  是否有敏感词(敏感词实现自己做)
    if(name.size() > 10){
        QMessageBox::about(this, "提示", "昵称过长，不能超过10" );
        return;
    }

    //发信号
    Q_EMIT SIG_registerCommit(tel,password,name);
}


void Maindialog::on_pb_clear_login_clicked()
{
    ui->le_tel_login->setText("");
    ui->le_password_login->setText("");
}


void Maindialog::on_pb_login_login_clicked()
{
    //注册信息采集
    QString tel = ui->le_tel_login->text();
    QString password = ui->le_password_login->text();
    //过滤
    //查看是否输入为空
    if(tel.isEmpty() || password.isEmpty())
    {
        QMessageBox::about(this,"提示","输入内容不能为空");
    }
    //手机号是 否合法  --正则表达式
    QRegExp exp( "^1[256789][0-9]\{9\}$");
    bool res=exp.exactMatch(tel);
    if(!res)
    {
        QMessageBox::about(this,"提示","手机号非法");
        return;
    }

    //密码是否过长
    if(password.size() > 20)
    {
        QMessageBox::about(this,"提示","手机号非法");
        return ;
    }

    //发信号
    Q_EMIT SIG_loginCommit(tel,password);
}


//手机号输入完毕后的动态检测是否正确
void Maindialog::on_le_tel_register_editingFinished()
{
    QRegExp pt("^1[3-8][0-9]{9}");  //正则类型,用于匹配手机号
    QString tel = ui->le_tel_register->text();
    if(!pt.exactMatch(tel) || tel.size()!=11)   //整串匹配成功才能返回true，否则都返回false
    {
        QMessageBox::information(this,"提示","请输入合法的手机号!",QMessageBox::Ok);
        ui->le_tel_register->setText("");
    }
}

void Maindialog::on_le_tel_login_editingFinished()
{
    QRegExp pt("^1[3-8][0-9]{9}");  //正则类型,用于匹配手机号
    QString tel = ui->le_tel_login->text();
    if(!pt.exactMatch(tel) || tel.size()!=11)   //整串匹配成功才能返回true，否则都返回false
    {
        QMessageBox::information(this,"提示","请输入合法的手机号!",QMessageBox::Ok);
        ui->le_tel_register->setText("");
    }
}
