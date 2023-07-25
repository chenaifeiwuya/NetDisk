#ifndef CLOGIC_H
#define CLOGIC_H

#include"TCPKernel.h"

class CLogic    //逻辑类
{
public:
    CLogic( TcpKernel* pkernel )
    {
        m_pKernel = pkernel;
        m_sql = pkernel->m_sql;
        m_tcp = pkernel->m_tcp;
    }
public:
    //设置协议映射
    void setNetPackMap();
    int getNumber()
    {
        return 1000000000;
    }

    /************** 发送数据*********************/
    void SendData( sock_fd clientfd, char*szbuf, int nlen )
    {
        m_pKernel->SendData( clientfd ,szbuf , nlen );
    }
    /************** 网络处理 *********************/
    //注册
    void RegisterRq(sock_fd clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(sock_fd clientfd, char*szbuf, int nlen);
    //文件上传请求
    void UploadFileRq(sock_fd clientfd, char *szbuf, int nlen);
    //文件块请求
    void FileContentRq(sock_fd clientfd, char *szbuf, int nlen);
    //文件信息请求
    void GetFileInfoRq(sock_fd clientfd,char *szbuf, int nlen);
    //下载文件请求
    void DownloadFileRq(sock_fd clientfd, char* szbuf, int nlen);
    //文件头回复
    void FileHeaderRs(sock_fd clientfd, char* szbuf, int nlen);
    //文件块回复
    void FIleContentRs(sock_fd clientfd,char * szbuf, int nlen);
    //创建文件夹申请
    void AddFolderRq(sock_fd clientfd, char* szbuf, int nlen);
    //分享文件申请
    void ShareFileRq(sock_fd clientfd,char* szbuf, int nlen);
    //分享一个文件
    void ShareItem(int userid, int fileid, string dir, string time, int lin);

    void MyShareRq(sock_fd clientfd, char *szbuf, int nlen);

    void GetShareRq(sock_fd clientfd, char* szbuf,int nlen);

    void GetShareByFile(int userid, int fileid, string dir, string name,string time);

    void GetShareByFolder(int userid, int fileid,string dir,string name,string time,int fromuserid,string fromdir);

    void DownloadFolderRq(sock_fd clientfd, char *szbuf, int nlen);

    void DownloadFolder(int userid,int& timestamp, sock_fd clientfd,list<string> &lstRes);

    void DownloadFile(int userid,int& timestamp,sock_fd clientfd, list<string>& lstRes);   //此函数用于下载文件夹时调用

    void DeleteFileRq(sock_fd clientfd,char *szbuf, int nlen);

    void DeleteOneItem(int userid, int fileid,string dir);

    void DeleteFile(int, int, string, string);

    void DeleteFolder(int userid, int fileid,string dir,string name);

    void ContinueDownloadRq(sock_fd clientfd, char *szbuf, int nlen);

    void ContinueUploadRq(sock_fd clientfd,char* szbuf,int nlen);
    /*******************************************/

private:
    TcpKernel* m_pKernel;
    CMysql * m_sql;
    Block_Epoll_Net * m_tcp;
    MyMap<int, UserInfo*> m_mapIDToUserInfo;
    //key userid * 1000000000 + timestamp value 文件信息
    MyMap<int64_t , FileInfo*> m_mapTimestampToFileInfo;
};

#endif // CLOGIC_H
