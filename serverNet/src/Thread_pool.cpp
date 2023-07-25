#include <Thread_pool.h>
#include "packdef.h"


STRU_POOL_T::STRU_POOL_T(int max, int min, int que_max)
{
    //初始化标志
    this->thread_max = max;
    this->thread_min = min;
    this->thread_alive = 0;
    this->thread_busy = 0;
    this->thread_shutdown = TRUE;
    this->thread_wait = 0;
    this->queue_max = que_max;
    this->queue_cur = 0;
    this->queue_front = 0;
    this->queue_rear = 0;
    //初始化互斥锁 条件变量
    if(pthread_cond_init(&this->not_full,NULL)!=0 ||
            pthread_cond_init(&this->not_empty,NULL)!=0 ||
            pthread_mutex_init(&this->lock,NULL)!=0)
    {
        err_str("init cond or mutex error:",-1);
    }
    //初始化线程数组
    if((this->tids = (pthread_t*)malloc(sizeof(pthread_t)*max)) == NULL)
    {
        err_str("malloc tids error:",-1);
    }
    bzero(this->tids,sizeof(pthread_t)*max);
    //初始化任务队列
    if((this->queue_task = (task_t*)malloc(sizeof(task_t)*que_max))==NULL)
    {
        err_str("malloc task queue error:",-1);
    }

}



int thread_pool::if_thread_alive(pthread_t tid)
{
    if((pthread_kill(tid,0)) == -1)
    {
        if(errno == ESRCH)
            return FALSE;
    }

    return TRUE;
}


bool thread_pool::Pool_create(int max,int min,int que_max)
{
    m_pool = new STRU_POOL_T( max , min , que_max );

    int err = 0;
    for(int i=0; i< min; i++)
    {
        if((err = pthread_create(&m_pool->tids[i],NULL,  Custom, (void*)m_pool))>0)
        {
            printf("create custom error:%s\n",strerror(err));
            return false;
        }
        ++(m_pool->thread_alive);
    }

    if((err = pthread_create(&(m_pool->manager_tid),NULL, Manager,(void*)m_pool))>0)
    {
        printf("create custom error:%s\n",strerror(err));
        return false;
    }
    return true;
}



int thread_pool::Producer_add( void *(*task)(void *arg),void *arg)
{
    pthread_mutex_lock(&m_pool->lock);
    while(m_pool->queue_cur == m_pool->queue_max && m_pool->thread_shutdown  )  //如果任务队列满了
    {
        pthread_cond_wait(&m_pool->not_full,&m_pool->lock);  //挂起在not_full信号量下
    }
    if(!m_pool->thread_shutdown  )   //线程池被销毁
    {
        pthread_mutex_unlock(&m_pool->lock);
        return -1;
    }
    m_pool->queue_task[m_pool->queue_front].task = task;   //队首保存任务(函数指针)
    m_pool->queue_task[m_pool->queue_front].arg = arg;  //参数
    m_pool->queue_front = (m_pool->queue_front + 1) % m_pool->queue_max;  //队首偏移
    ++(m_pool->queue_cur);  //任务队列大小加一
    pthread_cond_signal(&m_pool->not_empty);  //唤醒消费者
    pthread_mutex_unlock(&m_pool->lock);   //解锁
    return 0;
}

void * thread_pool::Custom(void * arg)
{
    pool_t * p = (pool_t*)arg;
    task_t task;
    while(p->thread_shutdown)
    {
        pthread_mutex_lock(&p->lock);
        while(p->queue_cur == 0 && p->thread_shutdown  )
        {
            pthread_cond_wait(&p->not_empty,&p->lock);
        }
        if(!p->thread_shutdown)
        {
            pthread_mutex_unlock(&p->lock);
            pthread_exit(NULL);
        }
        if(p->thread_wait > 0 && p->thread_alive > p->thread_min)
        {
            --(p->thread_wait);
            --(p->thread_alive);
            pthread_mutex_unlock(&p->lock);
            pthread_exit(NULL);
        }
        task.task = p->queue_task[p->queue_rear].task;
        task.arg = p->queue_task[p->queue_rear].arg;
        p->queue_rear = (p->queue_rear + 1) % p->queue_max;
        --(p->queue_cur);
        pthread_cond_signal(&p->not_full);
        ++(p->thread_busy);
        pthread_mutex_unlock(&p->lock);
        //执行核心工作
        (*task.task)(task.arg);
        pthread_mutex_lock(&p->lock);
        --(p->thread_busy);
        pthread_mutex_unlock(&p->lock);
    }
    return 0;
}

void *thread_pool::Manager(void *arg)
{
    pool_t * p = (pool_t *)arg;
    int alive;
    int cur;
    int busy;
    int add = 0;
    while(p->thread_shutdown )
    {
        pthread_mutex_lock(&p->lock);
        alive = p->thread_alive;
        busy = p->thread_busy;
        cur = p->queue_cur;
        pthread_mutex_unlock(&p->lock);
        if((cur > alive - busy || (float)busy / alive*100 >= (float)80 ) &&
                p->thread_max > alive)
        {
            for(int j = 0;j<p->thread_min;j++)
            {
                for(int i = 0;i<p->thread_max;i++)
                {
                    if(p->tids[i] == 0 || !if_thread_alive(p->tids[i]))
                    {
                        pthread_mutex_lock(&p->lock);
                        pthread_create(&p->tids[i],NULL,Custom,(void*)p);
                        ++(p->thread_alive);
                        pthread_mutex_unlock(&p->lock);
                        break;
                    }
                }
            }
        }
        if(busy *2 < alive - busy && alive > p->thread_min)
        {
            pthread_mutex_lock(&p->lock);
            p->thread_wait = _DEF_COUNT;
            pthread_mutex_unlock(&p->lock);
            for(int i=0; i<_DEF_COUNT; i++)
            {
                pthread_cond_signal(&p->not_empty);
            }
        }
        sleep(_DEF_TIMEOUT);
    }
    return 0;
}




