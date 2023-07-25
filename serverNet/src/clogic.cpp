#include "clogic.h"


void CLogic::setNetPackMap()
{
    NetPackMap(_DEF_PACK_REGISTER_RQ)    = &CLogic::RegisterRq;
    NetPackMap(_DEF_PACK_LOGIN_RQ)       = &CLogic::LoginRq;
    NetPackMap(_DEF_PACK_UPLOAD_FILE_RQ) = &CLogic::UploadFileRq;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RQ) = &CLogic::FileContentRq;
    NetPackMap(_DEF_PACK_GET_FILE_INFO_RQ) = &CLogic::GetFileInfoRq;
    NetPackMap(_DEF_PACK_DOWNLOAD_FILE_RQ) = &CLogic::DownloadFileRq;
    NetPackMap(_DEF_PACK_FILE_HEADER_RS) = &CLogic::FileHeaderRs;
    NetPackMap(_DEF_PACK_FILE_CONTENT_RS) = &CLogic::FIleContentRs;
    NetPackMap(_DEF_PACK_ADD_FOLDER_RQ) = &CLogic::AddFolderRq;
    NetPackMap(_DEF_PACK_MY_SHARE_RQ) = &CLogic::MyShareRq;
    NetPackMap(_DEF_PACK_GET_SHARE_RQ) = &CLogic::GetShareRq;
    NetPackMap(_DEF_PACK_SHARE_FILE_RQ) = &CLogic::ShareFileRq;
    NetPackMap(_DEF_PACK_DELETE_FILE_RQ) = &CLogic::DeleteFileRq;
    NetPackMap(_DEF_PACK_CONTINUE_DOWNLOAD_RQ) = &CLogic::ContinueDownloadRq;
    NetPackMap(_DEF_PACK_CONTINUE_UPLOAD_RQ) = &CLogic::ContinueUploadRq;
}

#define _DEF_COUT_FUNC_    cout << "clientfd:"<< clientfd << __func__ << endl;

//注册
void CLogic::RegisterRq(sock_fd clientfd,char* szbuf,int nlen)
{
    //cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_

            //拆包 查看 tel name是否存在 然后写入password
    STRU_REGISTER_RQ* rq=(STRU_REGISTER_RQ*)szbuf;
    STRU_REGISTER_RS rs;  //返回包
    char sqlstr[1000];
    sprintf(sqlstr,"select u_tel from t_user where u_tel = '%s'",rq->tel);
    list<string> lst;  //用于保存查询结果
    cout<<rq->tel<<endl;
    m_sql->SelectMysql(sqlstr,1,lst);  //查询列数为1,返回结果为查询返回条数
    if(!lst.empty())  //lst不为空，表示手机号存在
    {
        rs.result=tel_is_exist;
    }
    else
    {
        //继续查询昵称是否存在
        sprintf(sqlstr,"select u_tel from t_user where u_name = '%s'",rq->name);
        m_sql->SelectMysql(sqlstr,1,lst);
        if(!lst.empty())  //昵称重复
        {
            rs.result=name_is_exist;
        }
        else
            {
                //将信息写入数据库
                sprintf(sqlstr,"insert into t_user(u_tel,u_password,u_name) values ('%s','%s','%s'); ",rq->tel,rq->password,rq->name);
                m_sql->UpdataMysql(sqlstr);
                rs.result=register_success;

                //select u_id
                sprintf(sqlstr,"select u_id from t_user where u_tel = '%s';",rq->tel);
                m_sql->SelectMysql(sqlstr,1,lst);
                if(!lst.empty())  //num==1:user_info create success
                {
                    int id=stoi(lst.front());
                    lst.pop_front();

                    //create datebase:
                    char pathBuff[_MAX_PATH];
                    sprintf(pathBuff,"%s%d",DEF_PATH,id); //file_path

                    umask(0);
                    int err=mkdir(pathBuff,0777);  //0777:create file
                    if(err==0)
                    {
                        cout<<"Create file success!"<<endl;
                    }
                    else
                    {
                        perror("Create file error:");
                    }
                }
                else
                {
                    cout<<"select failed with:"<<sqlstr<<endl;
                }
        }

    }
          m_tcp->SendData(clientfd,(char*)&rs,sizeof(rs));
}

//登录
void CLogic::LoginRq(sock_fd clientfd ,char* szbuf,int nlen)
{
//    cout << "clientfd:"<< clientfd << __func__ << endl;
    _DEF_COUT_FUNC_

//    STRU_LOGIN_RS rs;
//    rs.m_lResult = password_error;
//    SendData( clientfd , (char*)&rs , sizeof rs );
   STRU_LOGIN_RQ* rq=(STRU_LOGIN_RQ*)szbuf;
   STRU_LOGIN_RS rs;
   list<string> lst;
   char sqlstr[1000];
   memset(sqlstr,0,sizeof(sqlstr));
   sprintf(sqlstr,"select u_password, u_id,u_name from t_user where u_tel = '%s'",rq->tel);
   //查询是否存在该帐号
   m_sql->SelectMysql(sqlstr,3,lst);  //3是查询列数，即查询几个属性
   if(lst.empty())
   {
       rs.result=user_not_exist;
   }
   else
   {
       //u_tel is exist, then check the u_password
       if(!strcmp(lst.front().c_str(),rq->password))  //strcmp()返回值为1表示相等
       {
           rs.result=login_success;
           lst.pop_front();
           int id=atoi(lst.front().c_str());
           rs.userid=id;
           lst.pop_front();
           strcpy(rs.name,lst.front().c_str());
           lst.pop_front();

       }
       else
       {
           rs.result=password_error;
       }
   }
   //cout<<rs.type<<endl;
   m_tcp->SendData(clientfd,(char*)&rs,sizeof(rs));


}

//处理文件上传请求
void CLogic::UploadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_UPLOAD_FILE_RQ* rq=(STRU_UPLOAD_FILE_RQ*)szbuf;
    //查看是否秒传 todo
    {
                //秒传还没有写，暂时测试会出现问题，因为上传相同文件时因为路径和MD5一致，查询数据库时会返回多个记录，但是因为程序只读取了第一个
                //所以每次读取到的文件id都是第一次上传相同文件时的id
                //所以，虽然能够正确上传文件，却无法在文件上传后将状态f_state改为1,因为f_state是根据文件id来改的

        //查看是否是秒传
        //查看 请求中到md5对应到文件在数据库是否存在 md5 相等状态位1(md5是客户端在发送上传请求时随文件信息一起传输过来的)
        //状态位0表示正在上传客户端做校验不让其上传
        char sqlbuf[1000] = "";
        sprintf(sqlbuf, "select f_id from t_file where f_MD5 ='%s' and f_state = 1;",rq->md5);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf, 1 , lstRes);
        if(!res){
            cout<<"select fail:"<<sqlbuf<<endl;
            return;
        }
        //如果存在
        if(lstRes.size() != 0){
            int fileid = stoi(lstRes.front());
            lstRes.pop_front();
            //写入用户文件关系
            sprintf(sqlbuf,"insert into t_user_file(u_id, f_id, f_dir, f_name, f_uploadtime) values(%d, %d, '%s', '%s', '%s');",rq->userid, fileid,rq->dir,rq->fileName,rq->time);
            res = m_sql->UpdataMysql(sqlbuf);
            if(!res){
                cout<<"insert fail:"<<sqlbuf<<endl;
                return;
            }
            //写秒传回复
            STRU_QUICK_UPLOAD_RS rs;
            rs.result = 1;
            rs.timestamp = rq->timestamp;
            rs.userid = rq->userid;

            SendData(clientfd, (char*)&rs,sizeof(rs));
            return;

        }
    }
    //不是秒传
    FileInfo *info= new FileInfo;
    char strpath[1000] = "";
    sprintf(strpath,"%s%d%s%s", DEF_PATH, rq->userid, rq->dir, rq->md5);
    info->absolutePath = strpath;  //通过这个 写数据库 打开文件 文件名字 md5
    //文件名字 md5
    info->dir = rq->dir;
    //info->fid
    info->md5 =rq->md5;
    info->name = rq->fileName;
    info->size = rq->size;
    info->time = rq->time;
    info->type = rq->type;

    //用linux的文件io
    info->fileFd = open(strpath,O_CREAT|O_WRONLY|O_TRUNC,00777);
    if(info->fileFd < 0)
    {
        std::cout<<"file open faile"<<std::endl;
        return ;
    }
    //map存储文件信息
    int64_t user_time = rq->userid* getNumber() + rq->timestamp;
    m_mapTimestampToFileInfo.insert(user_time, info);
    //文件信息创建  打开文件

    //map存储文件信息

    //数据库记录
        //插入文件信息 （引用计数 0， 状态 0 -> 上传结束后改为1 ）
        char sqlbuf[1000] = "";
        sprintf(sqlbuf,"insert into t_file ( f_size,f_path,f_count,f_MD5,f_state,f_type) values (%d ,'%s',0,'%s',0,'file');",rq->size,strpath,rq->md5);
        bool res=m_sql->UpdataMysql(sqlbuf);
        if(!res){
            printf("update faile:%s\n",sqlbuf);
        }

        //查文件id
        sprintf(sqlbuf,"select f_id from t_file where f_path = '%s' and f_MD5 = '%s'",strpath,rq->md5);
        list<std::string> lstRes;
        res = m_sql->SelectMysql(sqlbuf,1,lstRes);
        if(!res)
        {
            printf("Select failed: %s\n",sqlbuf );
        }
        if(lstRes.size() > 0)
            info->fid = stoi(lstRes.front());   //保存文件id    stoi()：string转int

        lstRes.clear();
        //插入用户文件关系 （由于触发器  引用计数 -> 1）
        sprintf(sqlbuf, "insert into t_user_file (u_id , f_id, f_dir,f_name,f_uploadtime) values (%d,%d,'%s','%s','%s');",rq->userid,info->fid,
                rq->dir,rq->fileName, rq->time );
        res=m_sql->UpdataMysql(sqlbuf);
        if(!res){
            printf("update faile:%s\n",sqlbuf);
        }

    //写回复包
    STRU_UPLOAD_FILE_RS rs;
    rs.fileid=info->fid;
    rs.result=1;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;

    SendData(clientfd,(char*)&rs, sizeof(rs));
}

//处理文件块请求
void CLogic::FileContentRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
            STRU_FILE_CONTENT_RQ* rq=(STRU_FILE_CONTENT_RQ*)szbuf;

    //获取文件信息
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    FileInfo * info = nullptr;
    if( !m_mapTimestampToFileInfo.find(user_time , info)){   //not found   程序依靠时间戳+用户id来在传输中区分不同文件
        cout<<"file not found"<<endl;
        return;
    }
    STRU_FILE_CONTENT_RS rs;
    //写入
    int len = write(info->fileFd , rq->content,rq->len);
    if(len != rq->len){
            //失败  跳回到读取之前
            rs.result=0;
            lseek(info->fileFd,-1*len,SEEK_CUR);   //写入多少，返回多少
        }
     else{       //成功  pos更新位置
        rs.result=1;
        info->pos+=len;
                //看是否到达末尾
        if(info->pos >= info->size){
            cout<<"end!!!"<<rq->fileid<<endl;
                    //是  关闭文件  更新数据库文件状态为1
            close(info->fileFd);
            char sqlBuf[1000];
            sprintf(sqlBuf,"update t_file set f_state=1 where f_id = %d;", rq->fileid);
            bool res=m_sql->UpdataMysql(sqlBuf);
            if(!res){   //if update failed , cout it
                cout<<"update failed: "<<sqlBuf;
            }

                    //回收map节点
            m_mapTimestampToFileInfo.erase(user_time);
            delete info;
            info = nullptr;
        }
    }
    //返回结果
    rs.fileid = rq->fileid;
    rs.len=rq->len;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;

    SendData(clientfd, (char*)&rs, sizeof(rs));

}


//文件信息请求
void CLogic::GetFileInfoRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_
    //拆包
    STRU_GET_FILE_INFO_RQ* rq=(STRU_GET_FILE_INFO_RQ*)szbuf;
    //根据 id  dir  查表(视图)获取文件信息
    char sqlBuf[1024];
    memset(sqlBuf,0,sizeof(sqlBuf));
    sprintf(sqlBuf,"select f_id, f_name,f_size, f_uploadtime, f_type from user_file_info where u_id = %d and f_dir = '%s' and f_state = 1;",rq->userid, rq->dir);
    list<string> lstRes;
    bool res=m_sql->SelectMysql(sqlBuf,5,lstRes);
    if(!res){
        cout<<"select faile: "<<sqlBuf<<endl;
        return;
    }
    if(lstRes.size() == 0)
        return;
    int count=lstRes.size()/5;  //计算有几条文件信息

    //写回复包
    int packlen=sizeof(STRU_GET_FILE_INFO_RS) + count*sizeof    (FileInfo);
    STRU_GET_FILE_INFO_RS* rs=(STRU_GET_FILE_INFO_RS*)malloc(packlen);

    rs->init();
    rs->count=count;
    strcpy(rs->dir,rq->dir);
    for(int i=0;i<count;i++)
    {
        int f_id=stoi(lstRes.front());
        lstRes.pop_front();
        string name=lstRes.front();
        lstRes.pop_front();
        int f_size=stoi(lstRes.front());
        lstRes.pop_front();
        string time=lstRes.front();
        lstRes.pop_front();
        string f_type = lstRes.front();
        lstRes.pop_front();

        rs->fileInfo[i].fileid = f_id;
        strcpy(rs->fileInfo[i].fileType,f_type.c_str());
        strcpy(rs->fileInfo[i].name, name.c_str());
        strcpy(rs->fileInfo[i].time, time.c_str());
        rs->fileInfo[i].size = f_size;
    }


    //发送
    SendData(clientfd,(char*)rs, packlen);

    free(rs);
}

//下载文件请求
void CLogic::DownloadFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_DOWNLOAD_FILE_RQ * rq = (STRU_DOWNLOAD_FILE_RQ*)szbuf;

    //查数据库 查什么？ f_name, f_path, f_MD5, f_size
    char sqlbuf[1000] = "";
    sprintf(sqlbuf,"select f_name , f_path , f_MD5 , f_size from user_file_info where u_id = %d and f_dir = '%s' and f_id = %d",rq->userid, rq->dir, rq->fileid);
    list<string> lstRes;
    bool res = m_sql->SelectMysql(sqlbuf, 4, lstRes);
    if(!res){    //如果没有  返回
        cout<<"select fail:"<<sqlbuf <<endl;
        return;
    }
    if(lstRes.size() == 0) return;
    string strName = lstRes.front();  lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();
    //有写文件信息
    FileInfo * info = new FileInfo;
    info->dir = rq->dir;
    info->fid = rq->fileid;

    info->md5 = strMD5;
    info->name = strName;
    info->size = size;
    info->type = "file";
    info->absolutePath = strPath;

    info->fileFd = open(info->absolutePath.c_str() , O_RDONLY);
    if(info->fileFd<=0){
        cout<<"file open fail"<<endl;
        return;
    }

    //key求出来
    int64_t user_time = rq->userid * getNumber() + rq->timestamp;
    //存到map里
    m_mapTimestampToFileInfo.insert(user_time , info);
    //发送文件头请求
    STRU_FILE_HEADER_RQ headrq;
    strcpy(headrq.dir , rq->dir);
    headrq.fileid = rq->fileid;
    strcpy(headrq.fileName, info->name.c_str());
    strcpy(headrq.md5, info->md5.c_str());
    strcpy(headrq.md5, info->md5.c_str());
    strcpy(headrq.fileType, "file");
    headrq.size = info->size;
    headrq.timestamp = rq->timestamp;

    SendData(clientfd, (char*)&headrq, sizeof(headrq));
}

//
void CLogic::FileHeaderRs(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_FILE_HEADER_RS* rs = (STRU_FILE_HEADER_RS*)szbuf;

    //get file info
    int64_t user_time = rs->userid*getNumber() + rs->timestamp;
    FileInfo* info = nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time,info)) return;

    //send file
    STRU_FILE_CONTENT_RQ rq;
    //read file
    rq.len = read(info->fileFd, rq.content, _DEF_BUFFER);
    if(rq.len < 0)
    {
        perror("read file error:");
        return;
    }
    rq.fileid = rs->fileid;
    rq.timestamp = rs->timestamp;
    rq.userid = rs->userid;

    SendData(clientfd, (char*)&rq, sizeof(rq));

}

//处理文件块回复
void CLogic::FIleContentRs(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_FILE_CONTENT_RS * rs = (STRU_FILE_CONTENT_RS*)szbuf;

    //文件信息管理
    int64_t user_time = rs->userid * getNumber() + rs->timestamp;
    FileInfo* info = nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time,info)) return;
    //判断是否成功
    if(rs->result != 1)
    {

        //不成功跳回去
        lseek(info->fileFd, -1*(rs->len), SEEK_CUR);
    }else
    {
        //success  pos+=len
        info->pos += rs->len;
        //判断是否结束
        if(info->pos >= info->size)
        {
            close(info->fileFd);
            m_mapTimestampToFileInfo.erase(user_time);
            delete info;
            info = nullptr;
            return;
        }
    }

    //write file content
    STRU_FILE_CONTENT_RQ rq;
    //read file
    rq.len = read(info->fileFd,rq.content,_DEF_BUFFER);
    if(rq.len == 0)return;
    if(rq.len < 0){
        perror("read fail:");
        return;
    }
    rq.fileid = rs->fileid;
    rq.timestamp = rs->timestamp;
    rq.userid = rs->userid;

    //send
    SendData(clientfd, (char*)&rq, sizeof(rq));


    //
}

//创建文件夹申请
void CLogic::AddFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;

    STRU_ADD_FOLDER_RQ * rq =(STRU_ADD_FOLDER_RQ*)szbuf;

    //数据库写表  插入文件信息
    //f_size, f_path, f_count, f_MD5, f_state, f_type
    char pathbuf[1000] = "";
    sprintf(pathbuf, "%s%d%s%s",DEF_PATH,rq->userid, rq->dir, rq->fileName);  // /root/id/dir/name

    char sqlbuf[1000]="";
    sprintf(sqlbuf,"insert into t_file(f_size,f_path,f_count,f_MD5,f_state,f_type) values (0,'%s',0,'?',1,'folder');",pathbuf);
    bool res = m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout<<"update fail:"<<sqlbuf<<endl;
        return;
    }
    //查询id
    sprintf(sqlbuf,"select f_id from t_file where f_path = '%s';",pathbuf);
    list<string>lstRes;
    res=m_sql->SelectMysql(sqlbuf,1,lstRes);
    if(!res){
        cout<<"SelectMysql fail:"<<sqlbuf <<endl;
        return;
    }
    if(lstRes.size() == 0)return;

    int id=stoi(lstRes.front());lstRes.pop_front();

    //写入用户文件关系  -- 隐藏 触发器引用计数会+1
    //u_id, f_id, f_dir, f_name, f_uploadtime
    sprintf(sqlbuf,"insert into t_user_file(u_id,f_id,f_dir,f_name,f_uploadtime) values (%d, %d, '%s', '%s', '%s');",rq->userid, id, rq->dir, rq->fileName, rq->time);
    res=m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout<<"update fail:"<<sqlbuf<<endl;
        return;
    }

    //创建目录
    umask(0);
    mkdir(pathbuf,0777);

    //write packet
    STRU_ADD_FOLDER_RS rs;
    rs.result = 1;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    //send
    SendData(clientfd,(char*)&rs,sizeof(rs));
}

void CLogic::ShareFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_SHARE_FILE_RQ *rq=(STRU_SHARE_FILE_RQ*)szbuf;

    //随机生成分享链接
    //分享码规则  9位分享码
    int link = 0;
    do{
        link = 1+random()%9;   //随机生成 1-9
        link *= 100000000;
        link += random()%100000000;
        //去重 查链接是否已经存在
        char sqlbuf[1000] ="";
        sprintf(sqlbuf,"select s_link from t_user_file where s_link = %d;",link);
        list<string> lstRes;
        bool res = m_sql->SelectMysql(sqlbuf,1,lstRes);
        if(!res){
            cout<<"select fail"<<sqlbuf<<endl; return;
        }
        if(lstRes.size() > 0){
            link = 0;
        }
    }while(link == 0);

    //遍历 所有文件，将其分享链接设置
    int itemCount = rq->itemCount;
    for(int i=0;i<itemCount;++i){
        ShareItem(rq->userid, rq->fileidArray[i],rq->dir,rq->shareTime,link);
    }
    //写回复
    STRU_SHARE_FILE_RS rs;
    rs.result = 1;
    SendData(clientfd, (char*)&rs,sizeof(rs));
}

void CLogic::ShareItem(int userid, int fileid, string dir, string time, int link)
{
    char sqlbuf[1000] = "";
    sprintf(sqlbuf,"update t_user_file set s_link = '%d' , s_linkTime = '%s' where u_id = %d and f_id = %d and f_dir = '%s';"
            ,link, time.c_str(),userid,fileid,dir.c_str());
    bool res = m_sql->UpdataMysql(sqlbuf);
    if(!res){
        cout<<"UpdataMysql fail"<<sqlbuf<<endl;
        return;
    }
}

void CLogic::MyShareRq(sock_fd clientfd, char *szbuf, int nlen)
{
    //拆包
    STRU_MY_SHARE_RQ* rq=(STRU_MY_SHARE_RQ*)szbuf;
    //rq->userid;
    //根据id查询 获得分享文件列表
    //查到内容 f_name  f_size  s_linkTime s_link
    char sqlbuf[1000] = "";
    sprintf(sqlbuf,"select f_name, f_size, s_linkTime, s_link from user_file_info where u_id = %d and s_link is not null and s_linkTime is not null;",rq->userid);
    list<string> lst;
    bool res=m_sql->SelectMysql(sqlbuf,4,lst);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl; return;
    }

    int count = lst.size();
    if(count/4==0 ||(count%4 !=0)) return;

    count/=4;
    //写回复
    int packlen = sizeof(STRU_MY_SHARE_RS) + count*sizeof(STRU_MY_SHARE_FILE);

    STRU_MY_SHARE_RS *rs=(STRU_MY_SHARE_RS*)malloc(packlen);
    rs->init();
    rs->itemCount = count;
    for(int i=0;i<count;++i)
    {
        string name = lst.front(); lst.pop_front();
        int size = stoi(lst.front()); lst.pop_front();
        string time = lst.front(); lst.pop_front();
        int link=stoi(lst.front()); lst.pop_front();

        strcpy(rs->items[i].name, name.c_str());
        rs->items[i].size = size;
        strcpy(rs->items[i].time, time.c_str());
        rs->items[i].shareLink = link;
    }

    //发送
    SendData(clientfd,(char*)rs,packlen);

    free(rs);
}

void CLogic::GetShareRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_GET_SHARE_RQ * rq = (STRU_GET_SHARE_RQ*)szbuf;

    //根据分享码 查询到一系列文件
    //查询树形：f_id f_name f_dir, f_type,u_id(分享别人到)
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_id, f_name,f_dir,f_type,u_id from user_file_info where s_link = %d;",rq->shareLink);
    list<string>lst;
    bool res=m_sql->SelectMysql(sqlbuf,5,lst);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl; return;
    }
    STRU_GET_SHARE_RS rs;
    if(lst.size() == 0){
        rs.result = 0;
        SendData(clientfd,(char*)&rs,sizeof(rs));
        return;
    }
    rs.result = 1;
    if(lst.size() %5 != 0) return;
    //遍历文件列表
    while(lst.size() != 0){
        int fileid = stoi(lst.front()); lst.pop_front();
        string name = lst.front(); lst.pop_front();
        string fromdir = lst.front(); lst.pop_front();
        string type = lst.front(); lst.pop_front();
        int fromuserid = stoi(lst.front()); lst.pop_front();

        if(type == "file"){
            //如果是文件
            //插入信息到用户文件关系表  //insert into t_user_file
            GetShareByFile(rq->userid, fileid, rq->dir, name,rq->time);
        }else{
            //如果是文件夹
            //插入信息到用户文集那关系表
            //拼接路径 获取人目录/->/06/分享人到目录/->/06/
            //根据心路经 在分享人那边查询 文件夹下的文件
            //遍历列表 --> 递归
            GetShareByFolder(rq->userid, fileid, rq->dir, name, rq->time, fromuserid, fromdir);
        }
    }
    //写回复包
    strcpy(rs.dir, rq->dir);
    //发送
    SendData(clientfd,(char*)&rs,sizeof(rs));

}

void CLogic::GetShareByFile(int userid, int fileid, string dir, string name, string time)
{
    char sqlbuf[1000] = "";
    sprintf(sqlbuf,"insert into t_user_file (u_id,f_id,f_dir,f_name,f_uploadtime) values(%d,%d,'%s','%s','%s');",userid,fileid,dir.c_str(),name.c_str(),time.c_str());

    bool res=m_sql->UpdataMysql(sqlbuf);
    if(!res){
        printf("updata fail:%s\n",sqlbuf);
    }
}

void CLogic::GetShareByFolder(int userid, int fileid, string dir, string name, string time, int fromuserid, string fromdir)
{
    //是文件夹
    //插入信息到 用户文件关系表
    GetShareByFile(userid, fileid, dir,name,time);
    //拼接路径
    //获取人目录
    string newdir = dir+name+"/";
    //分享人的目录
    string newfromdir = fromdir+name+"/";

    //根据新路径 在分享人那边查询  文件夹下的文件
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_id, f_name,f_type from user_file_info where u_id = %d and f_dir = '%s';",fromuserid,newfromdir.c_str());
    list<string>lst;
    bool res=m_sql->SelectMysql(sqlbuf,3,lst);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl;  return;
    }
    //便利列表
    while(lst.size() != 0){
        int fileid = stoi(lst.front()); lst.pop_front();
        string name=lst.front(); lst.pop_front();
        string type = lst.front(); lst.pop_front();

        if(type == "file"){
            //是文件
            GetShareByFile(userid,fileid,newdir,name,time);
        }else{
            //是文件夹->递归
            GetShareByFolder(userid,fileid,newdir,name,time,fromuserid,newfromdir);
        }
    }
}

void CLogic::DownloadFolderRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_DOWNLOAD_FOLDER_RQ* rq = (STRU_DOWNLOAD_FOLDER_RQ*)szbuf;

    //查数据库表  拿到信息 查的属性
    //f_id, f_name, f_path, f_MD5, f_size, f_dir, f_type
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type, f_id, f_name, f_path, f_MD5, f_size, f_dir from user_file_info where u_id = %d and f_dir = '%s' and f_id = %d;",rq->userid,rq->dir,rq->fileid);
    list<string> lstRes;
    bool res=m_sql->SelectMysql(sqlbuf, 7, lstRes);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl;
        return;
    }
    if(lstRes.size() == 0) return;
    string type=lstRes.front(); lstRes.pop_front();
    int timestamp = rq->timestamp;
    //下载文件夹
    DownloadFolder(rq->userid, timestamp,clientfd,lstRes);
}

void CLogic::DownloadFolder(int userid, int &timestamp, sock_fd clientfd, list<string> &lstRes)
{
    int fileid = stoi(lstRes.front()); lstRes.pop_front();
    string strName = lstRes.front(); lstRes.pop_front();
    string strPath =lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size=stoi(lstRes.front()); lstRes.pop_front();
    string dir = lstRes.front(); lstRes.pop_front();

    //发送创建文件夹请求
    STRU_FOLDER_HEADER_RQ rq;
    rq.timestamp = ++timestamp;  //时间戳处理
    strcpy(rq.dir, dir.c_str());
    rq.fileid = fileid;
    strcpy(rq.fileName, strName.c_str());
    SendData(clientfd,(char*)&rq,sizeof(rq));

    //拼接路径
    string newdir = dir + strName+"/";

    //查询呢我敌人userid所有文件信息（包含type）列表3个文件21项
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type, f_id, f_name, f_path, f_MD5, f_size, f_dir from user_file_info where u_id = %d and f_dir = '%s';",userid, newdir.c_str());
    list<string> newlstRes;
    bool res= m_sql->SelectMysql(sqlbuf, 7 ,newlstRes);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl;
        return;
    }
    while(newlstRes.size() != 0){
        string type = newlstRes.front(); newlstRes.pop_front();
        if(type == "file"){
            //如果是文件 下载文件流程
            DownloadFile(userid, timestamp,clientfd,newlstRes);
        }
    }

}

void CLogic::DownloadFile(int userid, int &timestamp, sock_fd clientfd, list<string> &lstRes)
{
    int fileid = stoi(lstRes.front()); lstRes.pop_front();
    string strName = lstRes.front(); lstRes.pop_front();
    string strPath = lstRes.front(); lstRes.pop_front();
    string strMD5 = lstRes.front(); lstRes.pop_front();
    int size = stoi(lstRes.front()); lstRes.pop_front();
    string dir = lstRes.front(); lstRes.pop_front();

    //有写文件信息
    FileInfo* info = new FileInfo;
    info->absolutePath = strPath;
    info->dir = dir;
    info->fid = fileid;
}

void CLogic::DeleteFileRq(sock_fd clientfd, char *szbuf, int nlen)
{
        //拆包
        STRU_DELETE_FILE_RQ * rq=(STRU_DELETE_FILE_RQ*)szbuf;
        //id列表
        for(int i=0;i<rq->fileCount;++i){
            int fileid = rq->fileidArray[i];
            //删除每一项
            DeleteOneItem(rq->userid, fileid, rq->dir);
        }
        //写回复
        STRU_DELETE_FILE_RS rs;
        rs.result = 1;
        strcpy(rs.dir,rq->dir);

        SendData(clientfd,(char*)&rs,sizeof(rs));
}

void CLogic::DeleteOneItem(int userid, int fileid, string dir)
{
    //删除文件需要 u_id f_dir f_id
    //需要知道到底是什么类型 type name path
    //再次查询id 看能不能找到数据库记录，如果不能，删除本地文件
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"select f_type,f_name,f_path from user_file_info where u_id = %d and f_id = %d and f_dir = '%s';",userid,fileid,dir.c_str());
    list<string> lst;
    bool res=m_sql->SelectMysql(sqlbuf,3,lst);
    if(!res)
    {
           cout<<"SelectMysql fail:"<<sqlbuf<<endl; return;
    }

    if(lst.size() == 0) return;
    string type = lst.front(); lst.pop_front();
    string name = lst.front();lst.pop_front();
    string path = lst.front(); lst.pop_front();
    if(type == "file"){
        DeleteFile(userid,fileid,dir,path);
    }else
        {
        DeleteFolder(userid,fileid,dir,name);
        rmdir(path.c_str());   //删除文件夹本身，需要绝对路径
    }
}

void CLogic::DeleteFile(int userid, int fileid, string dir, string path)
{
    //删除用户文件对应到关系
    char sqlbuf[1000] = "";
    sprintf(sqlbuf, "delete from t_user_file where u_id = %d and f_id = %d and f_dir = '%s';",userid,fileid,dir.c_str());
    bool res=m_sql->UpdataMysql(sqlbuf);
    if(!res)
        {
        cout<<"delete fail:"<<sqlbuf<<endl; return;
    }

    //查询数据库中是否还存有这个fileid的信息
    memset(sqlbuf,0,sizeof(sqlbuf));
    list<string> lst;
    sprintf(sqlbuf,"select f_id from t_file where f_id = %d;",fileid);
    res=m_sql->SelectMysql(sqlbuf,1,lst);
    if(!res){
        cout<<"select fail:"<<sqlbuf<<endl; return;
    }
    if(lst.size() == 0){
        unlink(path.c_str());   //文件io删除文件,这里用的是绝对路径
    }
}

void CLogic::DeleteFolder(int userid, int fileid, string dir, string name)
{
    //删除用户文件对应到关系u_id f_dir f_id
    char sqlbuf[1000]="";
    sprintf(sqlbuf,"delete from t_user_file where u_id = %d and f_id = %d and f_dir = '%s';",userid,fileid,dir.c_str());
    bool res = m_sql->UpdataMysql(sqlbuf);
    if(!res)
        {
        cout<<"delete fail:"<<sqlbuf<<endl; return;
    }
    //拼接路径
    std::string newdir=dir+name+"/";

    //查表 根据新路径查表 得到列表 f_type f_id name path
    sprintf(sqlbuf,"select f_type, f_id,f_name,f_path from user_file_info where u_id = %d and f_dir = '%s';",userid,newdir.c_str());
    list<string> lst;
    res=m_sql->SelectMysql(sqlbuf,4,lst);
    if(!res)
        {
        cout<<"SelectMysql fail:"<<sqlbuf; return;
    }
    while(lst.size() != 0)
        {
        //循环
        string type = lst.front(); lst.pop_front();
        int fileid = stoi(lst.front()); lst.pop_front();
        string name=lst.front(); lst.pop_front();
        string path = lst.front(); lst.pop_front();
        if(type == "file")
            {
            //如果是文件
            DeleteFile(userid, fileid, newdir, path);

        }else
            {
            //如果是文件夹
            DeleteFolder(userid,fileid,newdir,name);
            rmdir(path.c_str());   //删除文件夹本身，需要用绝对路径

        }
    }
}

void CLogic::ContinueDownloadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_CONTINUE_DOWNLOAD_RQ * rq=(STRU_CONTINUE_DOWNLOAD_RQ *)szbuf;

    //看是否存在  文件信息
    int64_t user_time = rq->userid*getNumber() + rq->timestamp;
    FileInfo *info=nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time, info)){
        //没有  创建文件信息  --由查表而来  添加到map
        info = new FileInfo;
        //查数据库 查什么 ？f_name,f_path,f_MD5,f_size
        char sqlbuf[1000]="";
        sprintf(sqlbuf,"select f_name, f_path, f_MD5, f_size from user_file_info where u_id = %d and f_dir \
            = '%s' and f_id = %d", rq->userid, rq->dir, rq->fileid);
                list<string> lstRes;
                bool res=m_sql->SelectMysql(sqlbuf, 4, lstRes);
        if(!res){
            cout<<"select fail:"<<sqlbuf<<endl;
            return;
        }
        //如果没有  返回
        if(lstRes.size() == 0) return;

        string strName = lstRes.front(); lstRes.pop_front();
        string strPath = lstRes.front(); lstRes.pop_front();
        string strMD5 = lstRes.front(); lstRes.pop_front();
        int size = stoi(lstRes.front()); lstRes.pop_front();

        //有  写文件信息
        info->absolutePath = strPath;
        info->dir = rq->dir;
        info->fid=rq->fileid;

        info->md5 = strMD5;
        info->name = strName;
        info->size = size;
        info->type = "file";

        info->fileFd = open(info->absolutePath.c_str(), O_RDONLY);
        if(info->fid <= 0){
            cout<<"file open fail"<<endl;
            return;
        }

        m_mapTimestampToFileInfo.insert(user_time, info);
    }

    //现在有信息了
    //文件指针跳转pos位置，同步 pos
    lseek(info->fileFd, rq->pos-1,SEEK_SET);   //SET是起始位置   从第pos个字节写起，包括第pos个字节，所以光标偏移为数为pos-1
    info->pos = rq->pos;
    //读文件块， 发送文件块请求
    STRU_FILE_CONTENT_RQ contentRq;

    contentRq.fileid = rq->fileid;
    contentRq.timestamp = rq->timestamp;
    contentRq.userid = rq->userid;

    contentRq.len = read(info->fid, contentRq.content, _DEF_BUFFER);

    SendData(clientfd, (char*)&contentRq,sizeof(contentRq));

}

void CLogic::ContinueUploadRq(sock_fd clientfd, char *szbuf, int nlen)
{
    _DEF_COUT_FUNC_;
    //拆包
    STRU_CONTINUE_UPLOAD_RQ* rq = (STRU_CONTINUE_UPLOAD_RQ*)szbuf;

    int64_t user_time = rq->userid*getNumber() + rq->timestamp;
    //需要看map中是否存在
    FileInfo* info=nullptr;
    if(!m_mapTimestampToFileInfo.find(user_time, info))
    {
        //不存在  创建
        info = new FileInfo;
        info->dir = rq->dir;
        info->fid = rq->fileid;
        info->type = "file";

        //查表 获取信息 查4列 分别是 f_path,f_md5,f_name,f_size
        char sqlbuf[1000]="";
        sprintf(sqlbuf,"select f_name,f_path,f_size,f_MD5 from user_file_info\
 where u_id = %d and f_id=%d and f_dir='%s' and f_state=0;",rq->userid,rq->fileid,rq->dir);
        list<string> lst;
        bool res=m_sql->SelectMysql(sqlbuf,4,lst);
        if(!res){
            cout<<"select fail:"<<sqlbuf<<endl;return;

        }
        if(lst.size()==0)return;
        info->name = lst.front();lst.pop_front();
        info->absolutePath = lst.front();lst.pop_front();
        info->size = stoi(lst.front());lst.pop_front();
        info->md5 = lst.front();lst.pop_front();

        //给info赋值 然后打开文件 info加到map中
        info->fileFd = open(info->absolutePath.c_str(), O_WRONLY);
        if(info->fileFd <= 0){
            cout<<"file open fail;"<<errno<<endl;
            return;
        }
        m_mapTimestampToFileInfo.insert(user_time, info);
    }
    //现在已经有这个信息了 lseek 跳转并读取文件当前写的位置（文件末尾）更新 pos
    info->pos = lseek(info->fileFd, 0, SEEK_END);

    //写回复 返回
    STRU_CONTINUE_UPLOAD_RS rs;
    rs.fileid = rq->fileid;
    rs.pos = info->pos;
    rs.timestamp = rq->timestamp;

    SendData(clientfd,(char*)&rs,sizeof(rs));
}
