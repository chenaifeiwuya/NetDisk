#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "err_str.h"
#include <malloc.h>

#include<iostream>
#include<map>
#include<list>


//边界值
#define _DEF_SIZE           45
#define _DEF_BUFFERSIZE     1000
#define _DEF_PORT           8000
#define _DEF_SERVERIP       "0.0.0.0"
#define _DEF_LISTEN         128
#define _DEF_EPOLLSIZE      4096
#define _DEF_IPSIZE         16
#define _DEF_COUNT          10
#define _DEF_TIMEOUT        10
#define _DEF_SQLIEN         400
#define TRUE                true
#define FALSE               false



/*-------------数据库信息-----------------*/
#define _DEF_DB_NAME    "NetDisk"
#define _DEF_DB_IP      "localhost"
#define _DEF_DB_USER    "root"
#define _DEF_DB_PWD     "mmm959825"
/*--------------------------------------*/
#define _MAX_PATH           (260)
#define _DEF_BUFFER         (4096)
#define _DEF_CONTENT_SIZE	(1024)
#define _MAX_SIZE           (40)

#define DEF_PATH "/home/chneaifeiwuya/myColin_Pro/NetDiskInfo/"
//自定义协议   先写协议头 再写协议结构
//登录 注册 获取好友信息 添加好友 聊天 发文件 下线请求
#define _DEF_PACK_BASE	(10000)
#define _DEF_PACK_COUNT (100)

//注册
#define _DEF_PACK_REGISTER_RQ	(_DEF_PACK_BASE + 0 )
#define _DEF_PACK_REGISTER_RS	(_DEF_PACK_BASE + 1 )
//登录
#define _DEF_PACK_LOGIN_RQ	(_DEF_PACK_BASE + 2 )
#define _DEF_PACK_LOGIN_RS	(_DEF_PACK_BASE + 3 )


////////////////////文件上传/////////////////
//上传文件请求
#define _DEF_PACK_UPLOAD_FILE_RQ       (_DEF_PACK_BASE + 4 )
//上传文件回复
#define _DEF_PACK_UPLOAD_FILE_RS       (_DEF_PACK_BASE + 5 )

//文件内容请求
#define _DEF_PACK_FILE_CONTENT_RQ       (_DEF_PACK_BASE + 6 )
//文件内容回复
#define _DEF_PACK_FILE_CONTENT_RS       (_DEF_PACK_BASE + 7 )



//返回的结果
//注册请求的结果
#define tel_is_exist		(0)
#define name_is_exist       (1)
#define register_success	(2)
//登录请求的结果
#define user_not_exist		(0)
#define password_error		(1)
#define login_success		(2)


typedef int PackType;

using namespace std;
struct UserInfo{
    int userid;
    string name;
    int clientfd;
};

//文件信息
struct FileInfo
{

    FileInfo():fid(0) , size(0),fileFd( 0 )
      , pos(0) {

    }

    int fid;   //数据库中的文件id
    string name;
    string dir;   //网盘目录
    string time;
    int size;    //int 32位 最大值2GB-----假定翁判文件都是在2GB以内
    string md5;
    string type;
    string absolutePath;   //文件本地绝对路径

    int pos; //上传或下载到什么位置

    int fileFd;  //文件描述符
};

//协议结构
//注册
typedef struct STRU_REGISTER_RQ
{
    STRU_REGISTER_RQ():type(_DEF_PACK_REGISTER_RQ)
    {
        memset( tel  , 0, sizeof(tel));
        memset( name  , 0, sizeof(name));
        memset( password , 0, sizeof(password) );
    }
    //需要手机号码 , 密码, 昵称
    PackType type;
    char tel[_MAX_SIZE];
    char name[_MAX_SIZE];
    char password[_MAX_SIZE];

}STRU_REGISTER_RQ;

typedef struct STRU_REGISTER_RS
{
    //回复结果
    STRU_REGISTER_RS(): type(_DEF_PACK_REGISTER_RS) , result(register_success)
    {
    }
    PackType type;
    int result;

}STRU_REGISTER_RS;

//登录
typedef struct STRU_LOGIN_RQ
{
    //登录需要: 手机号 密码
    STRU_LOGIN_RQ():type(_DEF_PACK_LOGIN_RQ)
    {
        memset( tel , 0, sizeof(tel) );
        memset( password , 0, sizeof(password) );
    }
    PackType type;
    char tel[_MAX_SIZE];
    char password[_MAX_SIZE];

}STRU_LOGIN_RQ;

typedef struct STRU_LOGIN_RS
{
    // 需要 结果 , 用户的id
    STRU_LOGIN_RS(): type(_DEF_PACK_LOGIN_RS) , result(login_success),userid(0)
    {
        memset(name,0,sizeof(name));
    }
    PackType type;
    int result;
    int userid;
    char name[_MAX_SIZE];

}STRU_LOGIN_RS;



//上传文件请求
struct STRU_UPLOAD_FILE_RQ
{
    STRU_UPLOAD_FILE_RQ():type(_DEF_PACK_UPLOAD_FILE_RQ)
      ,userid(0),size(0),timestamp(0){
        memset( fileName , 0, sizeof(fileName) );
        memset( dir , 0, sizeof(dir) );
        memset( md5 , 0, sizeof(md5) );
        memset( fileType , 0, sizeof(fileType) );
        memset( time , 0, sizeof(time) );
    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid; //服务器与时间戳配合,区分不同任务
    char fileName[_MAX_PATH]; //上传文件名字
    int size;//大小
    char dir[_MAX_PATH];//上传到什么目录
    char md5[_MAX_SIZE]; //上传文件的md5, 用于验证文件是否完整无误
    char fileType[_MAX_SIZE];//文件类型
    char time[_MAX_SIZE]; //上传时间
};

//上传文件回复
struct STRU_UPLOAD_FILE_RS
{
    STRU_UPLOAD_FILE_RS(): type(_DEF_PACK_UPLOAD_FILE_RS)
      , userid(0), fileid(0),result(1),timestamp(0){

    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid;//用户id
    int fileid; //文件id
    int result; //结果
};


//上传文件内容请求
struct STRU_FILE_CONTENT_RQ
{
    STRU_FILE_CONTENT_RQ():type(_DEF_PACK_FILE_CONTENT_RQ),
        userid(0),fileid(0),len(0),timestamp(0){
        memset( content , 0 , sizeof(content));
    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid;//用户id
    int fileid;//文件id
    char content[_DEF_BUFFER];//文件内容 也叫文件块   _DEF_BUFFER  4096
    int len;//文件内容长度
};

//文件内容回复
struct STRU_FILE_CONTENT_RS
{
    STRU_FILE_CONTENT_RS():type(_DEF_PACK_FILE_CONTENT_RS),
        userid(0),fileid(0),result(1),len(0),timestamp(0){

    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid;//用户id
    int fileid;//文件id
    int result;//结果
    int len;//文件内容长度

};



//下载文件内容请求
#define _DEF_PACK_GET_FILE_INFO_RQ  (_DEF_PACK_BASE + 8)
//文件内容回复
#define _DEF_PACK_GET_FILE_INFO_RS  (_DEF_PACK_BASE + 9)

#define _DEF_TYPE_LEN       (10)

//获取文件信息请求
struct STRU_GET_FILE_INFO_RQ
{
    STRU_GET_FILE_INFO_RQ():type(_DEF_PACK_GET_FILE_INFO_RQ){
        memset(dir,0,sizeof(dir));
    }
    PackType type;
    int userid;
    char dir[_MAX_PATH];
};


//文件信息
struct STRU_FILE_INFO
{
    STRU_FILE_INFO():fileid(0),size(0){
        memset(name, 0, sizeof(name));
        memset(time, 0, sizeof(time));
        memset(fileType, 0, sizeof(fileType));
    }
    int fileid;
    char name[_MAX_SIZE];
    char time[_MAX_SIZE];
    int size;
    char fileType[_DEF_TYPE_LEN];
};

//获取文件信息回复
struct STRU_GET_FILE_INFO_RS
{
    STRU_GET_FILE_INFO_RS(): type(_DEF_PACK_GET_FILE_INFO_RS), count(0){
        memset(dir,0,sizeof(dir));
    }

    void init(){
        type = _DEF_PACK_GET_FILE_INFO_RS;
        count=0;
        memset(dir, 0, sizeof(dir));
    }

    PackType type;
    char dir[_MAX_PATH];
    int count;
    //文件信息数组
    STRU_FILE_INFO fileInfo[];  //柔性数组


};




////////////////////文件下载/////////////////
//下载文件请求
#define _DEF_PACK_DOWNLOAD_FILE_RQ			(_DEF_PACK_BASE + 10 )

//下载文件夹请求
#define _DEF_PACK_DOWNLOAD_FOLDER_RQ		(_DEF_PACK_BASE + 11 )

//下载文件回复
#define _DEF_PACK_DOWNLOAD_FILE_RS			(_DEF_PACK_BASE + 12 )

//下载文件头请求
#define _DEF_PACK_FILE_HEADER_RQ			(_DEF_PACK_BASE + 13 )

//下载文件头回复
#define _DEF_PACK_FILE_HEADER_RS			(_DEF_PACK_BASE + 14 )



//下载文件请求
struct STRU_DOWNLOAD_FILE_RQ
{
    STRU_DOWNLOAD_FILE_RQ():type(_DEF_PACK_DOWNLOAD_FILE_RQ)
      ,userid(0),fileid(0),timestamp(0){
        memset( dir , 0, sizeof(dir) );
    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid; //服务器与时间戳配合,区分不同任务
    int fileid; //文件id
    char dir[ _MAX_PATH ]; //文件所属目录

};

//下载文件夹请求
struct STRU_DOWNLOAD_FOLDER_RQ
{
    STRU_DOWNLOAD_FOLDER_RQ():type(_DEF_PACK_DOWNLOAD_FOLDER_RQ)
      ,userid(0),fileid(0),timestamp(0){
        memset( dir , 0, sizeof(dir) );
    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid; //服务器与时间戳配合,区分不同任务
    int fileid; //文件id
    char dir[ _MAX_PATH ]; //文件所属目录

};

//下载文件回复 ( 一般也不会出现问题, 所以这个包也不用 )
struct STRU_DOWNLOAD_FILE_RS
{
    STRU_DOWNLOAD_FILE_RS():type(_DEF_PACK_DOWNLOAD_FILE_RS)
    ,timestamp(0),userid(0),fileid(0),result(1){
    }
    PackType type;
    int timestamp;//时间戳用于区分不同任务
    int userid; //服务器与时间戳配合,区分不同任务
    int fileid; //文件id
    int result; //结果
};

//文件头请求
struct STRU_FILE_HEADER_RQ
{
    STRU_FILE_HEADER_RQ():type(_DEF_PACK_FILE_HEADER_RQ)
      ,fileid(0),size(0),timestamp(0){
        memset( fileName , 0, sizeof(fileName) );
        memset( dir , 0, sizeof(dir) );
        memset( md5 , 0, sizeof(md5) );
        memset( fileType , 0, sizeof(fileType) );
    }
    PackType type;
    int timestamp;
    int fileid;
    char fileName[_MAX_PATH];
    int size;//大小
    char dir[_MAX_PATH];//路径
    char md5[_MAX_SIZE];
    char fileType[_MAX_SIZE];//文件类型
};

//文件头回复
struct STRU_FILE_HEADER_RS
{
    STRU_FILE_HEADER_RS(): type(_DEF_PACK_FILE_HEADER_RS)
      , userid(0), fileid(0),result(1),timestamp(0){

    }
    PackType type;
    int timestamp;
    int userid;
    int fileid;
    int result;
};

//////////////////新建文件夹/////////////////////
//新建文件夹请求
#define _DEF_PACK_ADD_FOLDER_RQ       (_DEF_PACK_BASE + 15 )
//新建文件夹回复
#define _DEF_PACK_ADD_FOLDER_RS       (_DEF_PACK_BASE + 16 )

//新建文件夹请求
struct STRU_ADD_FOLDER_RQ
{
    STRU_ADD_FOLDER_RQ():type(_DEF_PACK_ADD_FOLDER_RQ)
      ,timestamp(0),userid(0){
        memset( fileName , 0, sizeof(fileName) );
        memset( dir , 0, sizeof(dir) );
        memset( time , 0, sizeof(time) );
    }
    PackType type;
    int timestamp;
    int userid;
    char fileName[_MAX_PATH];
    char dir[_MAX_PATH];//路径
    char time[_MAX_SIZE]; //上传时间
};

//新建文件夹回复
struct STRU_ADD_FOLDER_RS
{
    STRU_ADD_FOLDER_RS(): type(_DEF_PACK_ADD_FOLDER_RS)
     ,timestamp(0) ,userid(0), result(1){

    }
    PackType type;
    int timestamp;
    int userid;
    int result;
};


////////////////秒传/////////////////
#define _DEF_PACK_QUICK_UPLOAD_RS (_DEF_PACK_BASE + 17)

struct STRU_QUICK_UPLOAD_RS
{
    STRU_QUICK_UPLOAD_RS():type(_DEF_PACK_QUICK_UPLOAD_RS), timestamp(0),userid(0),result(0){

    }
    PackType type;
    int timestamp;
    int userid;
    int result;
};


///////////////分享文件 ////////////////////
///分享文件请求
#define _DEF_PACK_SHARE_FILE_RQ       (_DEF_PACK_BASE + 18 )
/// 分享文件回复
#define _DEF_PACK_SHARE_FILE_RS       (_DEF_PACK_BASE + 19 )

//分享文件请求 : 包含 谁 分享 什么目录下面的 哪些文件( 文件id 数组 )  分享时间
struct STRU_SHARE_FILE_RQ
{
    void init(){
        type = _DEF_PACK_SHARE_FILE_RQ;
        userid = 0;
        memset( dir , 0 , sizeof(dir) );
        memset( shareTime , 0 , sizeof(shareTime) );
        itemCount = 0;
    }
    PackType type;
    int userid;
    char dir[_MAX_PATH ];
    char shareTime[_MAX_SIZE];
    int itemCount;
    int fileidArray[];
};

// 收到回复 就刷新分享列表
struct STRU_SHARE_FILE_RS
{
    STRU_SHARE_FILE_RS(): type( _DEF_PACK_SHARE_FILE_RS ),result(0){

    }
    PackType type;
    int result;
};

///////////////刷新分享列表 ////////////////////
//获取自己的分享请求
#define _DEF_PACK_MY_SHARE_RQ   (_DEF_PACK_BASE + 20 )
//获取自己的分享回复
#define _DEF_PACK_MY_SHARE_RS   (_DEF_PACK_BASE + 21 )
//获取自己的分享请求  谁获取  考虑加个时间 比如获取半个月的  todo
struct STRU_MY_SHARE_RQ
{
    STRU_MY_SHARE_RQ():type( _DEF_PACK_MY_SHARE_RQ) , userid(0){

    }
    PackType type;
    int userid;
    // 考虑加入时间 获取指定时间范围的分享
};

//分享文件信息: 名字 大小 分享时间 链接 (密码 todo )
struct STRU_MY_SHARE_FILE
{
    char name[_MAX_PATH];
    int size;
    char time[_MAX_SIZE];
    int shareLink;
};

//获取自己的分享回复  分享文件的列表 文件: 名字 大小 分享时间 链接 (密码 todo )
struct STRU_MY_SHARE_RS
{
    void init() {
        type = _DEF_PACK_MY_SHARE_RS; itemCount = 0;
    }
    PackType type;
    int itemCount;
    STRU_MY_SHARE_FILE items[];
};


/////////////////////////////获取分享//////////////////////////////////
//获取分享请求
#define _DEF_PACK_GET_SHARE_RQ       (_DEF_PACK_BASE + 22 )
//获取分享回复
#define _DEF_PACK_GET_SHARE_RS       (_DEF_PACK_BASE + 23 )

//获取分享
struct STRU_GET_SHARE_RQ
{
    STRU_GET_SHARE_RQ():type(_DEF_PACK_GET_SHARE_RQ)
      ,userid(0), shareLink(0){
        memset(dir , 0 , sizeof(dir));
        memset(time , 0 , sizeof(time));
    }
    PackType type;
    int userid;
    int shareLink; // 9位 首位是1-9 数字
    char dir[_MAX_PATH];
    char time[_MAX_SIZE];
    //直接加载这个路径下面
};

//获取分享回复 :收到刷新
struct STRU_GET_SHARE_RS
{
    STRU_GET_SHARE_RS():type(_DEF_PACK_GET_SHARE_RS)
      ,result(0) {
        memset(dir , 0 , sizeof(dir));
    }
    PackType type;
    int result;
    char dir[_MAX_PATH];
};


//下载文件夹的头请求
#define _DEF_PACK_FOLDER_HEADER_RQ   (_DEF_PACK_BASE + 24)

//文件夹的头请求
struct STRU_FOLDER_HEADER_RQ
{
    STRU_FOLDER_HEADER_RQ():type(_DEF_PACK_FOLDER_HEADER_RQ)
      ,fileid(0),timestamp(0){
        memset(fileName, 0, sizeof(fileName));
        memset(dir, 0, sizeof(dir));

    }
    PackType type;
    int timestamp;
    int fileid;
    char fileName[_MAX_PATH];
    char dir[_MAX_PATH];
};

//////////////////删除文件///////////////////
//删除文件请求
#define _DEF_PACK_DELETE_FILE_RQ       (_DEF_PACK_BASE + 25 )
//删除文件回复
#define _DEF_PACK_DELETE_FILE_RS       (_DEF_PACK_BASE + 26 )

//删除文件请求 : 某人 删除某路径下的 某文件 fileid数组
struct STRU_DELETE_FILE_RQ
{
    void init()
    {
        type = _DEF_PACK_DELETE_FILE_RQ;
        userid = 0;
        fileCount = 0;
        memset( dir , 0 , sizeof(dir) );
    }
    PackType type;
    int userid;
    char dir[_MAX_PATH];
    int fileCount;
    int fileidArray[];
};

//删除文件回复
struct STRU_DELETE_FILE_RS
{
    STRU_DELETE_FILE_RS():type(_DEF_PACK_DELETE_FILE_RS)
      ,result(1){
        memset( dir , 0 , sizeof(dir) );
    }
    PackType type;
    int result;
    char dir[_MAX_PATH];
};

/////////////////下载续传协议 //////////////
//请求
#define _DEF_PACK_CONTINUE_DOWNLOAD_RQ     (_DEF_PACK_BASE + 27)
// 告诉服务器 从哪里开始传数据块 就可以直接传了  不需要等待收到回复

// 服务器中 map 有和没有  有 timestamp  没有 需要唯一确认文件 uid fid fdir
struct STRU_CONTINUE_DOWNLOAD_RQ
{
    STRU_CONTINUE_DOWNLOAD_RQ():type(_DEF_PACK_CONTINUE_DOWNLOAD_RQ){
        userid = 0;
        timestamp = 0;
        fileid = 0;
        pos = 0;
        memset( dir , 0 , sizeof(dir));
    }

    PackType type;
    int userid;
    int timestamp;
    int fileid;
    int pos;
    char dir[_MAX_PATH];
};
// 上传续传协议 /////////////
// 会询问服务器 已经上传到哪里了  所以要有回复 服务器把已经到哪里了 传给客户端
// 服务器 map 有 userid + timestamp 没有 查表 与下载续传查表 流程一样.
//请求
#define _DEF_PACK_CONTINUE_UPLOAD_RQ     (_DEF_PACK_BASE + 28)
//回复  客户端在回复处理时 , 发文件块
#define _DEF_PACK_CONTINUE_UPLOAD_RS     (_DEF_PACK_BASE + 29)

struct STRU_CONTINUE_UPLOAD_RQ
{
    STRU_CONTINUE_UPLOAD_RQ():type(_DEF_PACK_CONTINUE_UPLOAD_RQ){
        userid = 0;
        timestamp = 0;
        fileid = 0;
        memset( dir , 0 , sizeof(dir));
    }

    PackType type;
    int userid;
    int timestamp;
    int fileid;
    char dir[_MAX_PATH];
};

struct STRU_CONTINUE_UPLOAD_RS
{
    STRU_CONTINUE_UPLOAD_RS():type(_DEF_PACK_CONTINUE_UPLOAD_RS){
        fileid = 0;
        timestamp = 0;
        pos = 0;
        //memset( dir , 0 , sizeof(dir));
    }

    PackType type;
    int fileid;
    int timestamp;
    int pos;
    //char dir[_MAX_PATH_SIZE];
};




