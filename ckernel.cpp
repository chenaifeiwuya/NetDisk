#include "ckernel.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
cKernel::cKernel(QObject *parent) : QObject(parent)
{

    m_sql = new CSqlite;
    m_quit=false;
    setSystempath();
    m_MainDialog=new Maindialog;
    connect(m_MainDialog,SIGNAL(SIG_close()),this,SLOT(slot_destroy()));
    connect(m_MainDialog,SIGNAL(SIG_loginCommit(QString,QString)),this,SLOT(slot_loginCommit(QString,QString)));
    connect(m_MainDialog,SIGNAL(SIG_registerCommit(QString,QString,QString)),this,SLOT(slot_registerCommit(QString,QString,QString)));
    loadFile();

    m_tcpClient = new TcpClientMediator;
    connect(m_tcpClient, SIGNAL(SIG_ReadyData(uint,char*,int)), this, SLOT(slot_dealClientData(uint,char*,int)));
    //客户端连接真实地址
    m_tcpClient->OpenNet("192.168.174.134");   //端口是一个有默认值的参数

#ifdef USE_SERVER
    m_tcpServer = new TcpServerMediator;
    connect(m_tcpServer, SIGNAL(SIG_ReadyData(uint,char*,int)), this, SLOT(slot_dealServerData(uint,cahr*,int)));
    m_tcpServer->OpenNet();
#endif
    setNetPackMap();
    m_MainDialog->show();

    m_MainWindow=new MainWindow;
    connect(m_MainWindow,SIGNAL(SIG_close()),this,SLOT(slot_destroy()));
    connect(m_MainWindow,SIGNAL(SIG_uploadFile(QString,QString)),this,SLOT(slot_uploadFile(QString,QString)));
    connect(this, SIGNAL(SIG_updateUploadFileProgress(int,int)), m_MainWindow, SLOT(slot_updateUploadFileProgress(int,int)));
    connect(m_MainWindow,SIGNAL(SIG_downloadFile(int,QString)), this, SLOT(slot_downloadFile(int,QString)));
    connect(m_MainWindow,SIGNAL(SIG_addFolder(QString,QString)), this, SLOT(slot_addFolder(QString,QString)));
    connect(this,SIGNAL(SIG_updateDownloadFileProgress(int,int)), m_MainWindow, SLOT(slot_updateDownloadFileProgress(int,int)));
    connect(m_MainWindow,SIGNAL(SIG_changeDir(QString)),this,SLOT(slot_changeDir(QString)));
    connect(m_MainWindow,SIGNAL(SIG_uploadFolder(QString,QString)),this,SLOT(slot_uploadFolder(QString,QString)));
    connect(m_MainWindow,SIGNAL(SIG_refreshPageInfo(QString)),this,SLOT(slot_refreshPageInfo(QString)));
    connect(m_MainWindow,SIGNAL(SIG_shareFile(QVector<int>,QString)),this,SLOT(slot_shareFile(QVector<int>,QString)));
    connect(m_MainWindow,SIGNAL(SIG_getShareByLink(int,QString)),this,SLOT(slot_getShareByLink(int,QString)));
    connect(m_MainWindow,SIGNAL(SIG_deleteFile(QVector<int>,QString)),this,SLOT(slot_deleteFile(QVector<int>,QString)));
    connect(m_MainWindow,SIGNAL(SIG_setUploadPause(int,int)), this,SLOT(slot_setUploadPause(int,int)));
    connect(m_MainWindow,SIGNAL(SIG_setDownloadPause(int,int)), this,SLOT(slot_setDownloadPause(int,int)));
    connect(m_MainWindow,SIGNAL(SIG_updateLimitSize(int)),this,SLOT(slot_updateLimitSize(int)));


    connect(&timer,SIGNAL(timeout()),this,SLOT(slot_showSpeed()));
    timer.setInterval(1000);  //间隔为1s
    timer.start();
    limitSize = 0;   //0表示不限速,默认限速大小为不限速
}



#define MD5_KEY "1234"
//password_1234
//生成MD5函数
static std::string getMD5(QString val)  //static函数：当前文件可用
{
    QString str = QString("%1_%2").arg(val).arg(MD5_KEY);
    MD5 md(str.toStdString());
    qDebug() <<str<<"mds: "<<md.toString().c_str();
    return md.toString();

}

#include<QTextCodec>

// QString -> char* gb2312
void Utf8ToGB2312( char* gbbuf , int nlen ,QString& utf8)
{
    //转码的对象
    QTextCodec * gb2312code = QTextCodec::codecForName( "gb2312");
    //QByteArray char 类型数组的封装类 里面有很多关于转码 和 写IO的操作
    QByteArray ba = gb2312code->fromUnicode( utf8 );// Unicode -> 转码对象的字符集

    strcpy_s ( gbbuf , nlen , ba.data() );
}

// char* gb2312 --> QString utf8
QString GB2312ToUtf8( char* gbbuf )
{
    //转码的对象
    QTextCodec * gb2312code = QTextCodec::codecForName( "gb2312");
    //QByteArray char 类型数组的封装类 里面有很多关于转码 和 写IO的操作
    return gb2312code->toUnicode( gbbuf );// 转码对象的字符集 -> Unicode
}

//获取文件md5
static std::string getFileMD5(QString path)
{
    MD5 md;
    //打开文件  读取文件内容  读到md5类里面， 生成md5
    FILE* pFile = nullptr;
    //fopen 如果有中文 支持ANSI编码  使用ascii码
    //path里面是utf8（qt 默认） 需要转码
    char buf[1000] = "";
    Utf8ToGB2312(buf,1000, path);
    pFile=fopen(buf,"rb");  //二进制只读打开
    if(!pFile){
        qDebug()<<"file md5 open fail";
        return NULL;
    }
    int len=0;
    do{
        len = fread(buf, 1, 1000, pFile );   //缓冲区，一次读多少，读多少次，文件指针  返回值为读成功次数
        md.update(buf,len);
    }while(len > 0);
    fclose(pFile);

    qDebug() << "file md5:" <<md.toString().c_str();
    return md.toString();

}


//开发中暂未使用此函数读取ip地址和端口
void cKernel::loadFile()  //加载配置文件
{

    //ip地址和端口的默认值
       QString m_ip="192.168.4.143";
       QString m_port="8004";
    //1：拼写地址
    QString path=QCoreApplication::applicationDirPath() + "/config.ini";  //applicationDirPath()函数获取当前application地址
    //2:查看该地址处是否有文件，如果文件不存在则创建并写入默认值，如果文件存在则读取文件信息
    QFileInfo fileInfo(path);
    if(fileInfo.exists())   //存在则读取信息
    {
        QSettings setting(path,QSettings::IniFormat);
        //打开组
        setting.beginGroup("net");
        //读取配置信息
        auto strip=setting.value("ip");
        auto strport=setting.value("port");
        if(!strip.toString().isEmpty()) m_ip=strip.toString();
        if(!strport.toString().isEmpty()) m_port=strport.toString();

        //关闭组
        setting.endGroup();
    }
    else{   //不存在则创建并写入默认值
        //利用QSetting来创建/读写配置文件
        QSettings setting(path,QSettings::IniFormat);
        //打开组
        setting.beginGroup("net");
        //设置key value
        setting.setValue("ip",m_ip);
        setting.setValue("port",m_port);
        //关闭组
        setting.endGroup();
    }
    qDebug()<<__func__<<m_ip<<" "<<m_port;
}

#define NetMap(a) m_netPackMap[ a - _DEF_PACK_BASE]
void cKernel::setNetPackMap()  //添加协议与函数的映射
{
    memset(m_netPackMap,0,sizeof(PFUN)* _DEF_PACK_COUNT);
    //协议映射表： key 协议头偏移量   value  函数指针
    //通过协议头找到对应处理函数
    NetMap(_DEF_PACK_REGISTER_RS) = &cKernel::slot_dealRegisterRs;
    NetMap(_DEF_PACK_LOGIN_RS) = &cKernel::slot_dealLoginRs;
    NetMap(_DEF_PACK_UPLOAD_FILE_RS) = &cKernel::slot_dealUploadFileRs;
    NetMap(_DEF_PACK_FILE_CONTENT_RS) = &cKernel::slot_dealFileContentRs;
    NetMap(_DEF_PACK_GET_FILE_INFO_RS) =&cKernel::slot_dealGetFileInfoRs;
    NetMap(_DEF_PACK_FILE_HEADER_RQ) = &cKernel::slot_dealFileHeaderRq;
    NetMap(_DEF_PACK_FILE_CONTENT_RQ) = &cKernel::slot_dealFileContentRq;
    NetMap(_DEF_PACK_ADD_FOLDER_RS) = &cKernel::slot_dealAddFolderRs;
    NetMap(_DEF_PACK_SHARE_FILE_RS) = &cKernel::slot_dealShareFileRs;
    NetMap(_DEF_PACK_MY_SHARE_RS) = &cKernel::slot_dealMyShareRs;
    NetMap(_DEF_PACK_FOLDER_HEADER_RQ) = &cKernel::slot_dealFolderHeadRq;
    NetMap(_DEF_PACK_DELETE_FILE_RS) =&cKernel::slot_dealDeleteFileRs;
    NetMap(_DEF_PACK_CONTINUE_UPLOAD_RS) = &cKernel::slot_dealContinueUploadRs;
}

void cKernel::SendData(char *buffer, int len)
{
    m_tcpClient->SendData(0,buffer,len);
}

//添加系统路径: exe同级   ./NetDisk
#include<QDir>
void cKernel::setSystempath()
{
    QString path = QCoreApplication::applicationDirPath() + "/NetDisk";

    QDir dir;
    //没有文件夹  创建
    if(!dir.exists(path)){
        dir.mkdir(path);  //只能常见一层
    }
    //默认路径
    m_sysPath = path;
}

void cKernel::slot_destroy()  //回收资源的槽函数
{
    timer.stop();
    qDebug()<<__func__;
    m_quit=true;
    delete m_MainDialog;
    m_MainDialog=NULL;
    delete m_MainWindow;
    m_MainWindow=NULL;
    return;
}

void cKernel::slot_loginCommit(QString tel, QString password)  //登录提交
{
    qDebug()<<__func__;
    STRU_LOGIN_RQ rq;
    strcpy(rq.tel, tel.toStdString().c_str());
    //strcpy(rq.password, password.toStdString().c_str());
    strcpy( rq.password,getMD5(password).c_str());
    //qDebug()<<rq.password;
    SendData((char*)&rq, sizeof(rq));
}

void cKernel::slot_registerCommit(QString tel, QString password, QString name)  //注册提交
{
    qDebug()<<__func__;
    STRU_REGISTER_RQ rq;
    strcpy(rq.tel, tel.toStdString().c_str());
    //strcpy(rq.password, password.toStdString().c_str());
    strcpy( rq.password,getMD5(password).c_str());
    //兼容中文(自己实现:Mysql编码方式和Qt不一样)
    std::string strName = name.toStdString();
    strcpy(rq.name,strName.c_str());
    SendData((char*)&rq, sizeof(rq));
}

#include<QDateTime>
//上传文件
void cKernel::slot_uploadFile(QString path, QString dir)
{
    QFileInfo qFileInfo(path);
    //文件信息存储
    FileInfo info;
    info.absolutePath=path;
    info.dir=dir;
    info.md5 = QString::fromStdString(getFileMD5(path));
    info.name = qFileInfo.fileName();
    info.size = qFileInfo.size();   //fopen   fseek  ftell

    info.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    info.type = "file";

    char buf[1000] = "";
    Utf8ToGB2312(buf,1000, path);
    info.pFile = fopen(buf,"rb");
    if(!info.pFile)
    {
        qDebug()<<"file open faile";
        return;
    }
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    //bug修复 反复检测时间戳是否存在
    while(m_mapTimestampToFileInfo.count(timestamp) > 0)
    {
        timestamp++;   //通过时间戳在运行时唯一确定一个元素，当上传文件夹的时候，由于电脑处理速度过快，容易导致处理文件夹中不同文件时形成相同的时间戳，形成的时间戳一样的话，
                        //由于客户端和服务器运行时存储时间戳的结构体是map，所以其中一个文件信息会被覆盖，导致传输错误。
    }


    info.timestamp = timestamp;
    qDebug()<<"timestamp:"<<timestamp ;
    //存储到map里面  key 时间戳 value 文件信息
    m_mapTimestampToFileInfo[timestamp] = info;

    //发上传文件请求
    STRU_UPLOAD_FILE_RQ rq;
    //兼容中文：先转换成string,再转换成char*
    std::string strDir = dir.toStdString();
    strcpy(rq.dir, strDir.c_str());
    //兼容中文
    std::string strName = info.name.toStdString();
    strcpy(rq.fileName, strName.c_str());


    strcpy(rq.fileType,"file");
    strcpy(rq.md5,info.md5.toStdString().c_str());

    rq.size = info.size;
    strcpy(rq.time, info.time.toStdString().c_str());
    rq.timestamp = timestamp;
    rq.userid = m_id;
    SendData((char*)&rq,sizeof(rq));
}

//上传文件夹槽函数
void cKernel::slot_uploadFolder(QString path, QString dir)
{
    qDebug()<<__func__;
    QFileInfo info(path);   //用于获取文件名字
    QDir dr(path);
     //当前文件夹的处理  addFolder
    qDebug()<<"folder:"<<info.fileName()<<"dir:"<<dir;
    slot_addFolder(info.fileName(),dir);
    //获取文件夹下面那一层  所有文件的路径（文件信息）
    QFileInfoList lst = dr.entryInfoList();  //获取路径下所有文件的文件信息列表
    //遍历所有文件
    QString newDir = dir + info.fileName() + "/";
    for(int i=0; i< lst.size() ; ++i)
    {
        QFileInfo file = lst.at(i);
        //如果是  . 继续
        if(file.fileName() == ".") continue;
        //如果是 .. 继续
        if(file.fileName() == "..") continue;
        //如果是文件 uploadFile -> 路径 文件信息的绝对路径 传到什么目录 /05/项目
        if(file.isFile()){
            qDebug() <<"file:"<<file.absoluteFilePath() << "dir:"<<newDir;
            slot_uploadFile(file.absoluteFilePath(),newDir);
        }

        //如果是文件夹 slot_uploadFolder 递归
        if(file.isDir()){
            slot_uploadFolder(file.absoluteFilePath(),newDir);
        }
    }
}

//刷新当前页面
void cKernel::slot_refreshPageInfo(QString path)
{
    qDebug()<<__func__;
    //向服务器发送获取当前目录文件列表
    STRU_GET_FILE_INFO_RQ rq;
    rq.userid = m_id;
    //兼容中文
    std::string strDir = path.toStdString();
    strcpy(rq.dir , strDir.c_str());

    SendData((char*)&rq, sizeof(rq));
}

//获取当前网盘目录
void cKernel::slot_getCurDirFileList()
{
    //向服务器发送获取当前目录文件列表
    STRU_GET_FILE_INFO_RQ rq;
    rq.userid = m_id;
    //兼容中文
    std::string strDir = m_curDir.toStdString();
    strcpy(rq.dir , strDir.c_str());

    SendData((char*)&rq, sizeof(rq));
}

//下载文件
void cKernel::slot_downloadFile(int fileid, QString dir)
{
    //写请求
    STRU_DOWNLOAD_FILE_RQ rq;
    //兼容中文
    std::string strDir = dir.toStdString();
    strcpy(rq.dir , strDir.c_str());
    rq.fileid = fileid;
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    while(m_mapTimestampToFileInfo.count(timestamp) > 0)
    {
        timestamp++;
    }
    //这里直接将一个空的fileInfo写入map占位,因为如果不先占位而等到服务器回复的话，很容易导致时间戳重复，map中信息被覆盖。
    FileInfo file;
    m_mapTimestampToFileInfo[timestamp] = file;
    rq.timestamp = timestamp;
    rq.userid = m_id;
    SendData((char*)&rq, sizeof(rq));
}

//下载文件夹
void cKernel::slot_downloadFoder(int, QString)
{

}

void cKernel::slot_shareFile(QVector<int> fileidArray, QString dir)
{
    qDebug()<<__func__;
    //打包
    int packlen = sizeof(STRU_SHARE_FILE_RQ) + sizeof(int)*fileidArray.size();

    STRU_SHARE_FILE_RQ * rq = (STRU_SHARE_FILE_RQ *)malloc(packlen);    //结构体中包含有柔性数组
    rq->init();
    rq->itemCount = fileidArray.size();
    for(int i=0;i<fileidArray.size(); ++i)
    {
        rq->fileidArray[i] = fileidArray[i];
    }
    rq->userid = m_id;
    std::string strDir = dir.toStdString();
    strcpy(rq->dir, strDir.c_str());
    QString time = QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss");
    strcpy(rq->shareTime, time.toStdString().c_str());

    SendData((char*)rq, packlen);
    free(rq);

}

void cKernel::slot_deleteFile(QVector<int> fileidArray, QString dir)
{
    //发送请求
    int packlen = sizeof(STRU_DELETE_FILE_RQ) + fileidArray.size()*sizeof(int);

    STRU_DELETE_FILE_RQ *rq = (STRU_DELETE_FILE_RQ*)malloc(packlen);
    rq->init();
    string strDir = dir.toStdString();
    strcpy(rq->dir, strDir.c_str());
    rq->fileCount = fileidArray.size();

    rq->userid = m_id;
    for(int i=0;i<rq->fileCount; ++i){
        rq->fileidArray[i] = fileidArray[i];
    }

    SendData((char*)rq, packlen);
    free(rq);
}

//新建文件夹
void cKernel::slot_addFolder(QString name, QString dir)
{
    //发请求包
    STRU_ADD_FOLDER_RQ rq;

    string strDir = dir.toStdString();
    strcpy(rq.dir, strDir.c_str());

    string strName = name.toStdString();
    strcpy(rq.fileName, strName.c_str());
    string strTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
    strcpy(rq.time, strTime.c_str());

    rq.timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();

    rq.userid = m_id;
    SendData((char*)&rq, sizeof(rq));

}

//更新当前路径槽函数
void cKernel::slot_changeDir(QString dir)
{
    //更新当前的目录
    m_curDir = dir;
    //刷新列表
    m_MainWindow->slot_deleteAllFileInfo();
    slot_getCurDirFileList();
}

void cKernel::slot_getMyShare()
{
    STRU_MY_SHARE_RQ rq;
    rq.userid = m_id;
    SendData((char*)&rq,sizeof(rq));
}

/*
 *=============================================================================================
 *=============================================================================================
 *=============================================================================================
 */

//客户端处理数据
void cKernel::slot_dealClientData(uint lSendIP, char *buf, int nlen)
{
    qDebug()<<__func__;
   // QString str = QString("来自服务器：%1").arg( QString::fromStdString( buf ));
   // QMessageBox::about( NULL, "提示", str);   //about：模态窗口 ，即阻塞的
    int type=*(int*)buf;
    //通过协议头拿到处理函数并执行
    qDebug()<<type;
    if(type >= _DEF_PACK_BASE && type<_DEF_PACK_BASE+_DEF_PACK_COUNT)
    {
        PFUN pf=m_netPackMap[type-_DEF_PACK_BASE];
        if(pf)
        {
            (this->*pf)(lSendIP,buf,nlen);
        }
    }
    else{
        qDebug()<<"Type Error!";
    }
    //回收空间
    delete[] buf;
}

void cKernel::slot_dealLoginRs(uint lSendIp, char *buf, int len)
{
    qDebug()<<__func__;   //打印函数名
    STRU_LOGIN_RS* rs=(STRU_LOGIN_RS*)buf;
    switch(rs->result)
    {
        case user_not_exist:
            QMessageBox::about(m_MainDialog,"提示","账户不存在，请先注册！");
            break;
        case password_error:
            QMessageBox::about(m_MainDialog,"提示","密码错误！");
            break;
        case login_success:
            QMessageBox::about(m_MainDialog,"提示","登录成功");
            m_id=rs->userid;
            InitDatabase(m_id);   //这里传的int参数是SQLite数据库名称  初始化数据库
            m_name=QString(rs->name);
            m_MainDialog->hide();   //隐藏登录页面
            m_MainWindow->show();   //显示登录后的页面
            m_curDir="/";
            slot_getCurDirFileList();
            slot_getMyShare();
            m_MainWindow->slot_setName(m_name);
            break;
        default:
            break;
    }
}

void cKernel::slot_dealRegisterRs(unsigned int, char *buf, int)
{
    qDebug()<<__func__;   //打印函数名
    STRU_REGISTER_RS* rs=(STRU_REGISTER_RS*)buf;
    qDebug()<<rs->result;
    switch(rs->result)
    {
    case tel_is_exist:
        QMessageBox::about(m_MainDialog,"提示","手机号已存在，注册失败!");
        break;
    case name_is_exist:
        QMessageBox::about(m_MainDialog,"提示","昵称已存在，注册失败！");
        break;
    case register_success:
        QMessageBox::about(m_MainDialog,"提示","注册成功！");
        break;
    default:
        break;
    }
}


//上传请求回复
void cKernel::slot_dealUploadFileRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_UPLOAD_FILE_RS * rs= (STRU_UPLOAD_FILE_RS*)buf;
    //首先查看结果是否为真
    if(!rs->result){
        qDebug()<<"上传失败";
        return ;   //为假就退出
    }
    //为真
        //获取文件信息
    if(m_mapTimestampToFileInfo.count(rs->timestamp) == 0){
        qDebug() <<"not found";
        return;
    }
    FileInfo& info = m_mapTimestampToFileInfo[rs->timestamp];
    //更新fileid
    info.fileid = rs->fileid;
    //插入上传信息到“上传中”的控件里
    slot_writeUploadTask(info);
     m_MainWindow->slot_insertUploadFile(info);
    //发送文件块（内容请求）
    STRU_FILE_CONTENT_RQ rq;
    rq.fileid = rs->fileid;
    rq.timestamp = rs->timestamp;
    rq.userid = m_id;

    rq.len = fread(rq.content, 1, _DEF_BUFFER, info.pFile);

    SendData((char*)&rq,sizeof(rq));



}

#include<QThread>

//服务器接收到上传文件块后的信息回复
void cKernel::slot_dealFileContentRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_FILE_CONTENT_RS* rs=(STRU_FILE_CONTENT_RS*)buf;
    //找文件信息结构体
    if(m_mapTimestampToFileInfo.count(rs->timestamp) == 0)   //如果没有找到
    {
        qDebug()<<"file not found!";
        return;
    }
    FileInfo& info=m_mapTimestampToFileInfo[rs->timestamp];
    //结果
        if(!rs->result){
        //假  跳回
            fseek(info.pFile , -1*(rs->len), SEEK_CUR);
        }
        else{
            //真 pos+len
            info.pos += rs->len;
            info.secondSize += rs->len;
            //更新上传进度
            //方案一:写信号槽  因为考虑到多线程
            //方案二：直接调用  一定要当前函数在主线程
            Q_EMIT SIG_updateUploadFileProgress(info.timestamp, info.pos);

        //判断是否结束
        if(info.pos >= info.size){
                //删除SQLite数据库中的文件信息
                slot_deleteUploadTask(info);
                //是  关闭文件  回收  返回
                fclose(info.pFile);
                m_mapTimestampToFileInfo.erase(rs->timestamp);

                //更新文件列表
                slot_refreshPageInfo(m_curDir);
                return;
            }
            while(info.secondSize >= limitSize && limitSize!=0)
            {
                //sleep();
                QThread::msleep(100);
                //为了避免阻塞窗口线程，影响事件循环， 加入下面的处理，蒋信号取出并执行
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
        }
        //判断是否暂停
        while(info.isPause)
        {
            //sleep();
            QThread::msleep(100);
            //为了避免阻塞窗口线程，影响事件循环， 加入下面的处理，蒋信号取出并执行
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //看一下有没有未处理的事件，如果有则执行，  100是时间，单位是ms
            /*
             * 一句话概述：作用是处理密集型耗时的事情。
              有时候需要处理一些跟界面无关的但非常耗时的事情，这些事情跟界面在同一个线程中，由于时间太长，导致界面无法响应，处于“假死”状态。
              例如：在应用程序中保存文件到硬盘上，从开始保存直到文件保存完毕，程序不响应用户的任何操作，
              窗口也不会重新绘制，从而处于“无法响应”状态，这是一个非常糟糕的体验 。

            在这种情况下，有一种方法是使用多线程，即在子线程中处理文件保存，主线程负责界面相关。

            而如果不想使用多线程，最简单的办法就是在文件保存过程中频繁调用QApplication::processEvents()。
            该函数的作用是让程序处理那些还没有处理的事件，然后再把使用权返回给调用者。
             * */
            //避免程序退出一直卡在这里
            if(m_quit) return;  //当程序退出的时候，此函数也直接返回
        }

        //发文件块
        STRU_FILE_CONTENT_RQ rq;
        rq.fileid = rs->fileid;
        rq.timestamp = rs->timestamp;
        rq.userid = m_id;
        rq.len = fread(rq.content, 1 , _DEF_BUFFER, info.pFile);

        SendData((char*)&rq, sizeof(rq));
}

//获取文件列表
void cKernel::slot_dealGetFileInfoRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_GET_FILE_INFO_RS* rs=(STRU_GET_FILE_INFO_RS*)buf;
    if(m_curDir != QString::fromStdString(rs->dir)) return;

    //先删除原来的
    m_MainWindow->slot_deleteAllFileInfo();
    //获取元素
    int count = rs->count;
    for(int i=0;i<count;i++)
    {
        FileInfo info;

        info.fileid = rs->fileInfo[i].fileid;
        info.type = QString::fromStdString(rs->fileInfo[i].fileType);
        info.name = QString::fromStdString(rs->fileInfo[i].name);
        info.fileid = rs->fileInfo[i].fileid ;
        info.size = rs->fileInfo[i].size;
        info.time = rs->fileInfo[i].time;

        //插入到控件中
        m_MainWindow->slot_insertFileInfo(info);
        m_MainWindow->slot_insertExploreGameInfo(info);
    }
}

//处理文件信息头
void cKernel::slot_dealFileHeaderRq(unsigned int lSendIp, char *buf, int nlen)
{
    STRU_FILE_HEADER_RQ * rq = (STRU_FILE_HEADER_RQ *)buf;
    FileInfo info;
    //默认路径 sysPath（不含最后的'/'）+dir + name
    //dir 可能有很多层 需要循环创建目录
    QString tmpDir = QString::fromStdString(rq->dir);  //首先将路径转换成QString类型
    QStringList dirList = tmpDir.split("/");  //分割函数,将路径从  “/” 分割
    QString pathsum = m_sysPath;
    for(QString & node:dirList)
    {
        if(!node.isEmpty()){
            pathsum += "/";
            pathsum += node;

            QDir dir;
            if(!dir.exists(pathsum)){   //如果路径不存在，则创建文件夹
                dir.mkdir(pathsum);
            }
        }
    }


    info.dir = QString::fromStdString(rq->dir);
    info.fileid = rq->fileid;
    info.md5 = QString::fromStdString(rq->md5);
    info.name = QString::fromStdString(rq->fileName);

    info.size = rq->size;
    info.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    info.timestamp = rq->timestamp;
    info.type = "file";

    //默认路径  sysPath(不含最后的'/')+dir + name
    //dir 可能有很多层  需要循环创建目录  todo
    info.absolutePath = QString("%1%2%3").arg(m_sysPath).arg(info.dir).arg(info.name);  //默认路径
    //m_sysPath保存的是exe同级路径   dir是所在网盘目录   name是文件名
    //打开文件
    char pathbuf[1000] = ""
;
    Utf8ToGB2312(pathbuf, 1000, info.absolutePath);
    qDebug()<<pathbuf;
    info.pFile = fopen(pathbuf,"wb");  //二进制写
    if(!info.pFile){
        qDebug() << "file open fail";
        return;
    }
    //todo 保存下载信息到控件
    m_MainWindow->slot_insertDownloadFile(info);
    //保存map里面
    m_mapTimestampToFileInfo[rq->timestamp] = info;
    //保存文件信息到SQLite数据库
    slot_writeDownloadTask(info);
    //写回复
    STRU_FILE_HEADER_RS rs;
    rs.fileid = rq->fileid;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = m_id;

    SendData((char*)&rs, sizeof(rs));

}

#include<QThread>
//处理接收到的文件块(下载)
void cKernel::slot_dealFileContentRq(unsigned int lSendIP, char *buf, int nlen)
{
    //拆包
    STRU_FILE_CONTENT_RQ * rq = (STRU_FILE_CONTENT_RQ *)buf;

    //拿到文件信息结构
    if(m_mapTimestampToFileInfo.count(rq->timestamp) == 0) return;
    FileInfo & info = m_mapTimestampToFileInfo[rq->timestamp];

    STRU_FILE_CONTENT_RS rs;
    //写文件
    int len = fwrite(rq->content, 1, rq->len, info.pFile);
    if(len != rq->len){
        //不成功，跳回去
        rs.result = 0;
        fseek(info.pFile, -1*len, SEEK_CUR);
    }else{
        //成功 pos+=len
        rs.result = 1;
        info.pos += len;
        info.secondSize += len;
        //更新进度 todo
        Q_EMIT SIG_updateDownloadFileProgress(info.pos , rq->timestamp);

        //要看 有没有到末尾  是否结束
        if(info.pos >= info.size){
            slot_deleteDownloadTask(info);  //删除SQLite数据库信息
            //结束  关闭文件 回收
            fclose(info.pFile);
            m_mapTimestampToFileInfo.erase(rq->timestamp);
        }
        while(info.secondSize >= limitSize && limitSize != 0)
        {
            //sleep();
            QThread::msleep(100);
            //为了避免阻塞窗口线程，影响事件循环， 加入下面的处理，蒋信号取出并执行
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }
    //判断是否暂停
    while(info.isPause)
    {
        //sleep();
        QThread::msleep(100);
        //为了避免阻塞窗口线程，影响事件循环， 加入下面的处理，蒋信号取出并执行
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //看一下有没有未处理的事件，如果有则执行，  100是时间，单位是ms
        /*
         * 一句话概述：作用是处理密集型耗时的事情。
          有时候需要处理一些跟界面无关的但非常耗时的事情，这些事情跟界面在同一个线程中，由于时间太长，导致界面无法响应，处于“假死”状态。
          例如：在应用程序中保存文件到硬盘上，从开始保存直到文件保存完毕，程序不响应用户的任何操作，
          窗口也不会重新绘制，从而处于“无法响应”状态，这是一个非常糟糕的体验 。

        在这种情况下，有一种方法是使用多线程，即在子线程中处理文件保存，主线程负责界面相关。

        而如果不想使用多线程，最简单的办法就是在文件保存过程中频繁调用QApplication::processEvents()。
        该函数的作用是让程序处理那些还没有处理的事件，然后再把使用权返回给调用者。
         * */
        //避免程序退出一致卡在这里
        if(m_quit) return;  //当程序退出的时候，此函数也直接返回
    }


    //写回复
    rs.fileid = rq->fileid;
    rs.len = rq->len;
    //写回复
    rs.fileid = rq->fileid;
    rs.len = rq->len;
    rs.timestamp = rq->timestamp;
    rs.userid = m_id;

    //发送
    SendData((char*)&rs , sizeof(rs));
}

//处理文件夹添加回复
void cKernel::slot_dealAddFolderRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_ADD_FOLDER_RS * rs = (STRU_ADD_FOLDER_RS *)buf;

    //判断是否成功
    if(rs->result != 1) return;
    //先删除原来的  slot_deleteAllFileInfo
    m_MainWindow->slot_deleteAllFileInfo();
    //更新文件列表
    slot_getCurDirFileList();
}

//处理分享文件回复
void cKernel::slot_dealShareFileRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_SHARE_FILE_RS * rs = (STRU_SHARE_FILE_RS *)buf;
    qDebug()<<__func__;
    //判断是否成功
    if(rs->result != 1) return;

    //刷新  发获取请求
    slot_getMyShare();
}

//处理我的分享回复
void cKernel::slot_dealMyShareRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_MY_SHARE_RS * rs=(STRU_MY_SHARE_RS *)buf;
    int count = rs->itemCount;
    // rs->items;
    //遍历  分享文件的信息  添加到控件上面
    m_MainWindow->slot_deleteAllShareInfo();
    for(int i=0; i<count;++i){
        m_MainWindow->slot_insertShareFileInfo(rs->items[i].name, rs->items[i].size, rs->items[i].time, rs->items[i].shareLink);
    }
}

void cKernel::slot_getShareByLink(int code, QString dir)
{
    //发请求
    STRU_GET_SHARE_RQ rq;
    string tmpDir = dir.toStdString();
    strcpy(rq.dir, tmpDir.c_str());
    rq.shareLink = code;
    string time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
    strcpy(rq.time , time.c_str());
    rq.userid = m_id;

    SendData((char*)&rq, sizeof(rq));
}


void cKernel::slot_downloadFolder(int fileid, QString dir)
{
    STRU_DOWNLOAD_FOLDER_RQ rq;
    string strDir = dir.toStdString();
    strcpy(rq.dir , strDir.c_str());

    rq.fileid = fileid;
    int timestamp = QDateTime::currentDateTime().toString("hhmmsszzz").toInt();
    while(m_mapTimestampToFileInfo.count(timestamp) > 0){
        timestamp++;
    }
    rq.timestamp = timestamp;
    rq.userid = m_id;

    SendData((char*)&rq, sizeof(rq));
}

void cKernel::slot_dealFolderHeadRq(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_FOLDER_HEADER_RQ *rq = (STRU_FOLDER_HEADER_RQ*)buf;
    //创建目录
    //dir 可能有很多层 需要循环创建目录
    QString tmpDir = QString::fromStdString(rq->dir);
    QStringList dirList = tmpDir.split("/");  //分割函数 NetDisk 111

    QString pathsum = m_sysPath;
    for(QString &node: dirList)
    {
        if(!node.isEmpty()){
            pathsum += "/";
            pathsum += node;

            QDir dir;
            if(!dir.exists(pathsum)){
                dir.mkdir(pathsum);
            }
        }
    }
    pathsum += "/";
    pathsum += QString::fromStdString(rq->fileName);

    QDir dir;
    if(!dir.exists(pathsum)){
        dir.mkdir(pathsum);
    }
}

void cKernel::slot_dealDeleteFileRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_DELETE_FILE_RS * rs = (STRU_DELETE_FILE_RS*)buf;
    //看是否刷新
    if(rs->result == 1)
    {
        if(QString::fromStdString(rs->dir) == m_curDir)
            m_MainWindow->slot_deleteAllFileInfo();
        slot_getCurDirFileList();
    }
}

void cKernel::slot_setUploadPause(int timestamp, int isPause)
{
    //isPause 1 从正在上传变为暂停  isPause 0 从暂停 变为开始继续上传
    //需要找到文件信息结构体
    //map里面 有 程序未退的情况  直接置位
    //map中没有  证明程序退出的  断点旭川需要走协议
    if(m_mapTimestampToFileInfo.count(timestamp) > 0){
        m_mapTimestampToFileInfo[timestamp].isPause = isPause;   //是暂停还是继续由isPause决定
    }else
    {
        //断点续传 todo
        //创建fileinfo 然后打开文件 放到map里面
        FileInfo info = m_MainWindow->slot_getUploadFileInfoByTimestamp(timestamp);
        //转化 路径转成ASCII
        char pathbuf[1000] = "";
        Utf8ToGB2312(pathbuf,sizeof(pathbuf),info.absolutePath);
        //打开文件 如果文件无法打开，就退出
        info.pFile = fopen(pathbuf,"rb");
        if(!info.pFile){
            qDebug()<<"打开失败"<<info.absolutePath; return;
        }
        info.isPause = 0;  //避免开始 就停在循环那里

        m_mapTimestampToFileInfo[timestamp] = info;

        //发送上传续传请求
        STRU_CONTINUE_UPLOAD_RQ rq;
        string strDir = info.dir.toStdString();
        strcpy(rq.dir,strDir.c_str());
        rq.fileid = info.fileid;
        rq.timestamp = timestamp;
        rq.userid = m_id;

        SendData((char*)&rq, sizeof(rq));
    }
}

void cKernel::slot_setDownloadPause(int timestamp, int isPause)
{
    //isPause 1 从正在下载变为暂停  isPause 0 从暂停变为开始继续下载
    //需要找到文件信息结构体
    //map里面 有 程序未退的情况  直接置位
    //map中没有  证明程序退出过  断点续传  需要走协议
    if(m_mapTimestampToFileInfo.count(timestamp) > 0){
        m_mapTimestampToFileInfo[timestamp].isPause = isPause;
    }else
    {
        //断点续传 todo
        //下载的信息 存到数据库 ， 重新登录加载， 然后点击开始（继续）
        if(isPause == 0){
            //断点续传
            //1:创建信息结构体  装到map里面
            //信息  在哪里？  可以直接从空间里面取出
            FileInfo info = m_MainWindow->slot_getDownloadFileInfoByTimestamp(timestamp);
            //转化 路径转成ASCII
            char pathbuf[1000] = "";
            Utf8ToGB2312(pathbuf, 1000, info.absolutePath);
            //打开文件  二进制追加  不能是w  因为会清空
            info.pFile = fopen(pathbuf,"ab");

            if(!info.pFile){
                qDebug()<<"打开失败"<<info.absolutePath; return;
            }
            info.isPause = 0;  //避免开始  就停在循环那里

            m_mapTimestampToFileInfo[timestamp] = info;

            //2.发协议 告诉服务器 文件下载到哪里了，然后服务器跳转到那里，从那里开始继续读，然后文件块发送
            //服务器接收 有两种可能  1.文件信息还在（客户端出现异常很快好了，没有超过预定删除客户端所有信息的时间）2.不在（超过了时间）
            STRU_CONTINUE_DOWNLOAD_RQ rq;
            rq.fileid = info.fileid;
            string dirstr = info.dir.toStdString();
            strcpy(rq.dir, dirstr.c_str());
            rq.pos = info.pos;
            rq.timestamp = info.timestamp;
            rq.userid = m_id;
            SendData((char*)&rq, sizeof(rq));

        }
    }
}

//显示每个上传下载的进度s
void cKernel::slot_showSpeed()
{
    m_MainWindow->slot_showSpeed(m_mapTimestampToFileInfo);

}

void cKernel::slot_updateLimitSize(int newLimit)
{
    limitSize = newLimit * 1024;
}



void cKernel::slot_writeUploadTask(FileInfo &info)
{
    QString sqlbuf = QString("insert into t_upload values(%1, %2, '%3',  '%4', '%5', %6, '%7','%8','%9');")\
                             .arg(info.timestamp) \
                             .arg(info.fileid)\
                            .arg(info.name)\
                            .arg(info.dir)\
                            .arg(info.time)\
                            .arg(info.size)\
                            .arg(info.md5)\
                            .arg(info.type)\
                            .arg(info.absolutePath);
    m_sql->UpdateSql(sqlbuf);
}

void cKernel::slot_writeDownloadTask(FileInfo &info)
{
    QString sqlbuf = QString("insert into t_download values(%1, %2, '%3',  '%4', '%5', %6, '%7','%8','%9');")\
                             .arg(info.timestamp) \
                             .arg(info.fileid)\
                            .arg(info.name)\
                            .arg(info.dir)\
                            .arg(info.time)\
                            .arg(info.size)\
                            .arg(info.md5)\
                            .arg(info.type)\
                            .arg(info.absolutePath);
    m_sql->UpdateSql(sqlbuf);
}

void cKernel::slot_deleteUploadTask(FileInfo &info)
{
    QString sqlbuf=QString("delete from t_upload where timestamp = %1;").arg(info.timestamp);
    m_sql->UpdateSql(sqlbuf);
}

void cKernel::slot_deleteDownloadTask(FileInfo &info)
{
    QString sqlbuf=QString("delete from t_download where timestamp = %1 and f_absolutePath = '%2';")\
            .arg(info.timestamp)\
            .arg(info.absolutePath);
    m_sql->UpdateSql(sqlbuf);
}

void cKernel::slot_getUploadTask(QList<FileInfo> &infoList)
{
    //获取所有的任务
 QString sqlbuf = "select * from t_upload;";
 QStringList lst;
 m_sql->SelectSql(sqlbuf, 9, lst);

 while(lst.size() != 0){
     FileInfo info;
     info.timestamp = QString(lst.front()).toInt(); lst.pop_front();
     info.fileid = QString(lst.front()).toInt(); lst.pop_front();
     info.name = lst.front(); lst.pop_front();
     info.dir = lst.front(); lst.pop_front();
     info.time = lst.front(); lst.pop_front();
     info.size =QString(lst.front()).toInt(); lst.pop_front();
     info.md5 = lst.front(); lst.pop_front();
     info.type = lst.front(); lst.pop_front();
     info.absolutePath = lst.front(); lst.pop_front();

     infoList.push_back(info);
 }
}

void cKernel::slot_getDownloadTask(QList<FileInfo> &infoList)
{
    //获取所有的任务
 QString sqlbuf = "select * from t_download;";
 QStringList lst;
 m_sql->SelectSql(sqlbuf, 9, lst);

 while(lst.size() != 0){
     FileInfo info;
     info.timestamp = QString(lst.front()).toInt(); lst.pop_front();
     info.fileid = QString(lst.front()).toInt(); lst.pop_front();
     info.name = lst.front(); lst.pop_front();
     info.dir = lst.front(); lst.pop_front();
     info.time = lst.front(); lst.pop_front();
     info.size =QString(lst.front()).toInt(); lst.pop_front();
     info.md5 = lst.front(); lst.pop_front();
     info.type = lst.front(); lst.pop_front();
     info.absolutePath = lst.front(); lst.pop_front();
/*
     QDir dir;
     if(!dir.exists( info.absolutePath)){
         continue;
     }
     else
     {
         QFileInfo fi( info.absolutePath);
         info.size = fi.size();
     }*/

     infoList.push_back(info);
 }
}

void cKernel::InitDatabase(int id)
{
    //首先找到exe  去同级目录  创建数据文件 /datebase/id.db
    QString path = QCoreApplication::applicationDirPath() + "/datebase/";   //DirPath和FIlePath的区别！！
    QDir dir;
    if(!dir.exists(path)){   //如果不存在则创建
        dir.mkdir(path);
    }
    path = path + QString("%1.db").arg(id);

    QFileInfo info(path);
    if(info.exists())
    {
    //首先查看有没有这个文件
        //如果有的话则直接加载
            //连接
        m_sql->ConnectSql(path);
            //测试  读取数据
        /*
        QString sqlbuf = "select count(*) from t_upload;";
        QStringList lst;
        m_sql->SelectSql(sqlbuf, 1, lst);
        qDebug()<<"upload item count: "<<lst.front();
        lst.clear();

        sqlbuf = "select count(*) from t_download;";
        m_sql->SelectSql(sqlbuf, 1, lst);
        qDebug()<<"download item count: "<<lst.front();
        lst.clear();
        */

        QList<FileInfo> uploadTaskList;
        QList<FileInfo> downloadTaskList;

        slot_getUploadTask(uploadTaskList);
        slot_getDownloadTask(downloadTaskList);

        for(FileInfo & info: uploadTaskList){
            //如果这个文件没有了 不能继续（判断文件是否还存在）
            QFileInfo fi(info.absolutePath);
            if(!fi.exists()) continue;  //如果不存在,跳过本文件

            //修改任务的初始状态
            info.isPause = 1;

            m_MainWindow->slot_insertUploadFile(info);
            //上传续传  控件  上看不到进行到多少
            //todo 获取当前位置
            info.pos = fi.size();
            //同步控件的位置
             m_MainWindow->slot_updateDownloadFileProgress(info.pos,info.timestamp);
        }
        //加载下载任务
        for(FileInfo & info : downloadTaskList)
        {
            //如果这个文件没有了不能继续
            QFileInfo fi(info.absolutePath);
            if(!fi.exists()) continue;

            //修改任务的初始状态
            info.isPause = 1;
            //进行到多少可以知道
            info.pos = fi.size();

            m_MainWindow->slot_insertDownloadFile(info);

            //控件同步位置
            m_MainWindow->slot_updateDownloadFileProgress(info.pos,info.timestamp);

        }



    }else{
        //没有  创建表
        QFile file(path);
        if(!file.open( QIODevice::WriteOnly )) return;  //如果打开失败则返回
        file.close();
        //连接
        m_sql->ConnectSql(path);
        //创建表
        QString sqlbuf = "create table t_upload( \
                tiemestamp int,   \
                f_id int,   \
                f_name varchar(260), \
                f_dir varchar(260),  \
                f_time varchar(60),  \
                f_size int,  \
                f_md5 varchar(60),  \
                f_type varchar(60),  \
                f_absolutePath varchar(260))";
         m_sql->UpdateSql(sqlbuf);

        sqlbuf = "create table t_download( \
                tiemestamp int,    \
                f_id int,   \
                f_name varchar(260),   \
                f_dir varchar(260),   \
                f_time varchar(60),   \
                f_size int,          \
                f_md5 varchar(60),   \
                f_type varchar(60),  \
                f_absolutePath varchar(260)  \
                )";
         m_sql->UpdateSql(sqlbuf);
    }
}

void cKernel::slot_dealGetShareRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_GET_SHARE_RS * rs = (STRU_GET_SHARE_RS *)buf;

    //根据结果
    if(rs->result == 0){
        //错误返回提示
        QMessageBox::about(m_MainWindow,QString("提示"),QString("获取分享失败"));
    }else{
        //正确刷新
        if(QString::fromStdString(rs->dir) == m_curDir){
            slot_getCurDirFileList();
        }
    }
}

void cKernel::slot_dealContinueUploadRs(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_CONTINUE_UPLOAD_RS* rs = (STRU_CONTINUE_UPLOAD_RS*)buf;

    //通过map拿到文件信息
    if(m_mapTimestampToFileInfo.count(rs->timestamp) == 0) return;
    FileInfo & info = m_mapTimestampToFileInfo[rs->timestamp];
    //文件位置跳转  pos 更新 界面显示 百分比更新
    info.pos = rs->pos;
    fseek(info.pFile, rs->pos, SEEK_SET);  //从起始位置跳 pos那么多

    m_MainWindow->slot_updateUploadFileProgress(info.timestamp,info.pos);
    //发送文件块请求
    STRU_FILE_CONTENT_RQ rq;
    //读文件
    int len = fread(rq.content, 1, _DEF_BUFFER,info.pFile);
    rq.len = len;
    rq.fileid = info.fileid;
    rq.timestamp = info.timestamp;
    rq.userid = m_id;

    SendData((char*)&rq,sizeof(rq));

}

//获取游戏商城的信息
void cKernel::slot_dealGetGameStoryInfo(unsigned int lSendIp, char *buf, int nlen)
{
    //拆包
    STRU_GET_FILE_INFO_RS* rs=(STRU_GET_FILE_INFO_RS*)buf;
    if(m_curDir != QString::fromStdString(rs->dir)) return;

    //先删除原来的
    m_MainWindow->slot_deleteAllExploreGameInfo();
    //获取元素
    int count = rs->count;
    for(int i=0;i<count;i++)
    {
        FileInfo info;

        info.fileid = rs->fileInfo[i].fileid;
        info.type = QString::fromStdString(rs->fileInfo[i].fileType);
        info.name = QString::fromStdString(rs->fileInfo[i].name);
        info.fileid = rs->fileInfo[i].fileid ;
        info.size = rs->fileInfo[i].size;
        info.time = rs->fileInfo[i].time;

        //插入到控件中
        m_MainWindow->slot_insertFileInfo(info);
    }
}

#ifdef USE_SERVER
void cKernel::slot_dealServerData(uint lSendIP, char *buf, int nlen)
{
    QString str = QString("来自客户端：%1").arg( QString::fromStdString( buf ));
    QMessageBox::about( NULL, "提示", str);   //about：模态窗口 ，即阻塞的
    m_tcpServer->SendData(lSendIP,buf,nlen);
    //回收空间
    delete[] buf;
}
#endif
