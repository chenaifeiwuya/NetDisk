#ifndef CKERNEL_H
#define CKERNEL_H

#include "maindialog.h"
#include <QObject>
#include <INetMediator.h>
#include <TcpClientMediator.h>
#include <TcpServerMediator.h>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <packdef.h>
#include <mainwindow.h>
#include "md5.h"
#include "csqlite.h"
#include <QDir>


//核心处理类
//单例
//协议映射表
class cKernel;
typedef void (cKernel::*PFUN)(uint lSendIP, char *buf, int nlen);


class cKernel : public QObject
{
    Q_OBJECT
private:
    explicit cKernel(QObject *parent = nullptr);      //explicit:防止隐式类型转换
    explicit cKernel(const cKernel& kernel){}
    ~cKernel(){}
signals:
    void SIG_updateUploadFileProgress(int, int);
    void SIG_updateDownloadFileProgress(int, int);


public:
    static cKernel* GetInstance(){
        //获取单例对象，此种写法和饿汉式相似。
           static cKernel kernel;
           return &kernel;

      }
    void loadFile();  //加载配置文件

private:
    void setNetPackMap();//初始化协议和函数的映射关系
    void SendData(char *buffer, int len);
    void setSystempath();

private slots:      //普通槽函数
    void slot_destroy();
    void slot_loginCommit(QString,QString);
    void slot_registerCommit(QString,QString,QString);
    void slot_uploadFile(QString,QString);
    void slot_uploadFolder(QString path, QString dir);
    void slot_refreshPageInfo(QString path);
    void slot_getCurDirFileList();
    void slot_downloadFile(int ,QString);
    void slot_downloadFoder(int ,QString);
    void slot_shareFile(QVector<int> ,QString);
    void slot_deleteFile(QVector<int> , QString);
    void slot_addFolder(QString, QString);
    void slot_changeDir(QString);
    void slot_getMyShare();
    void slot_getShareByLink(int code, QString dir);
    void slot_downloadFolder(int fileid, QString dir);
    void slot_dealFolderHeadRq(unsigned int lSendIp, char *buf, int nlen);
    void slot_dealDeleteFileRs(unsigned int lSendIp, char *buf, int nlen);
    void slot_setUploadPause(int timestamp, int isPause);
    void slot_setDownloadPause(int timestamp, int isPause);

//数据库相关槽函数
    //缓存上传的任务
    void slot_writeUploadTask(FileInfo & info);
    //缓存下载的任务
    void slot_writeDownloadTask(FileInfo & info);
    //完成任务，删除上传记录
    void slot_deleteUploadTask(FileInfo & info);
    //完成任务，删除下载任务
    void slot_deleteDownloadTask(FileInfo & info);
    //加载上传任务
    void slot_getUploadTask(QList<FileInfo> &infoList);
    //加载下载任务
    void slot_getDownloadTask(QList<FileInfo> &infoList);
private:
    //登陆之后，初始化数据
    void InitDatabase(int id);


private slots:   //网络槽函数
    void slot_dealClientData(uint,char*,int);
    void slot_dealLoginRs(uint,char*,int);
    void slot_dealRegisterRs(unsigned int,char*,int);
    void slot_dealUploadFileRs(unsigned int lSendIp, char* buf, int nlen);
    void slot_dealFileContentRs(unsigned int lSendIp, char*buf, int nlen);
    void slot_dealGetFileInfoRs(unsigned int lSendIp, char* buf, int nlen);
    void slot_dealFileHeaderRq(unsigned int lSendIp, char* buf, int nlen);
    void slot_dealFileContentRq(unsigned int lSendIP, char *buf, int nlen);
    void slot_dealAddFolderRs(unsigned int lSendIp, char* buf, int nlen);
    void slot_dealShareFileRs(unsigned int lSendIp, char *buf, int nlen);
    void slot_dealMyShareRs(unsigned int lSendIp, char *buf, int nlen);
    void slot_dealGetShareRs(unsigned int lSendIp, char *buf, int nlen);
    void slot_dealContinueUploadRs(unsigned int lSendIp,char* buf, int nlen);

#ifdef USE_SERVER
    void slot_dealServerData(uint,char*,int);
#endif

public:
    Maindialog *m_MainDialog;
    MainWindow *m_MainWindow;
    INetMediator *m_tcpClient;
    QString m_ip;
    QString m_port;
    int m_id;
    QString m_name;   //昵称
    QString m_curDir;   //网盘当前目录路径
    QString m_sysPath;   //默认存储的系统路径（绝对路径） exe同级下 NetDIsk文件夹下
    //key 时间戳  hhmmsszzz  int 21 xxxx xxxx value 文件信息
    std::map<int , FileInfo> m_mapTimestampToFileInfo;

    PFUN m_netPackMap[_DEF_PACK_COUNT];
    bool m_quit;   //程序退出标志位

    //数据库
    CSqlite* m_sql;
private:



#ifdef USE_SERVER
    INetMediator *m_tcpServer;
#endif

};
#endif // CKERNEL_H
