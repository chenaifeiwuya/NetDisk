#include "Mysql.h"



int CMysql::ConnectMysql(const char* server,const char* user,const char* password, const char* database)
{
    conn = NULL;
    conn = mysql_init(NULL);
    mysql_set_character_set(conn,"utf8");   //设置链接对象的编码集
    if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))  //连接函数
    {
        return FALSE;
    }
    pthread_mutex_init(&m_lock , NULL);

    return TRUE;
}

int CMysql::SelectMysql(char* szSql,int nColumn,list<string>& lst)  //查询函数,返回值只代表sql语句是否正确执行，不代表结果是否为空
{
    MYSQL_RES * results = NULL;

    pthread_mutex_lock(&m_lock );   //加锁目的：保证每次查询之后取出结果后才会进行第二条sql语句的执行，如果结果不取出就再次执行mysql_query()会返回false。

    if(mysql_query(conn,szSql)) {   //执行sql语句,执行出错返回非0（查询语句结果为空不是执行错误），成功的话，返回0
        pthread_mutex_unlock(&m_lock );
        return FALSE;
    }
    results = mysql_store_result(conn);  //获得语句执行结果
    pthread_mutex_unlock(&m_lock );

    if(NULL == results)return FALSE;  //查询语句即使查询结果为空这里也不会等于NULL！
    MYSQL_ROW record;
    while((record = mysql_fetch_row(results)))   //将结果读取到record里（results是多条查询结果，而mysql_fetch_row函数每次获取一条结果）
    {
        for(int i=0; i<nColumn; i++)
        {
            lst.push_back( record[i] );    //将一条结果中的每一个属性列保存到链表中
       //     q_Push(pQueue,(void*)record[i]);
        }
    }
    return TRUE;
}


int CMysql::UpdataMysql(char *szsql)  //更新函数
{
    if(!szsql)return FALSE;
    pthread_mutex_lock(&m_lock );
    if(mysql_query(conn,szsql)){   //执行更新语句，执行成功返回0
        pthread_mutex_unlock(&m_lock );
        return FALSE;
    }
    pthread_mutex_unlock(&m_lock );
    return TRUE;
}

void CMysql::DisConnect()
{
    mysql_close(conn);
}
