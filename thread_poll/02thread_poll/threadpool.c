#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "threadpool.h"
#include "cond.h"

//线程处理函数（注意和run函数不同！）
void* rout(void* arg)
{
    //获取arg及pool本身
    
    //因为线程池线程不断的执行任务，没有任务则等待，除非销毁。
    struct timespec sp;
    threadpool_t *pool = (threadpool_t*)arg;
    int timeout = 0;
    while(1)
    {
        timeout = 0;
        pool->idle++;
        condition_lock(&pool->ready);
        //任务队列没有任务，并且没有得到退出通知，就等待
        while(pool->first == NULL && pool->quit == 0)
        {
            printf("%lu thread i waiting...\n",pthread_self());
            clock_gettime(CLOCK_REALTIME,&sp);
            sp.tv_sec += 2;
            timeout = condition_timewait(&pool->ready,&sp);
            if(timeout == ETIMEDOUT)
            {
                timeout = 1;
                break;
            }
        }
        //否则，空闲线程数减一
        pool->idle--;

        //等到任务
        if(pool->first != NULL)
        {
            task_t *newtask = pool->first;
            pool->first = newtask->next;

            //防止任务执行太长时间导致别的任务不能操作队列
            condition_unlock(&pool->ready);
            (newtask->run)(newtask->arg);
            condition_lock(&pool->ready);
            free(newtask);
        }
        //退出标志位为1，销毁线程池
        if((pool->quit == 1) && (pool->first== NULL))
        {
            pool->counter--;
            if(pool->counter == 0)
               condition_signal(&pool->ready);
            condition_unlock(&pool->ready);
            break;
        }

        //超时处理
        if(timeout == 1 && pool->first == NULL)
        {
            condition_unlock(&pool->ready);
            break;
        }
        condition_unlock(&pool->ready);
    }
    printf("%lu thread exited\n",pthread_self());
}

//初始化线程池
void threadpool_init(threadpool_t *pool,int threads)
{
    condition_init(&(pool->ready));
    pool->first = NULL;
    pool->last = NULL;
    pool->counter = 0;
    pool->idle = 0;
    pool->max_threads = threads;
    pool->quit = 0;
}

//往线程池中添加任务
void threadpool_add_task(threadpool_t* pool,void *(run)(void *arg),void* arg)
{
    //此处因为要操作链表，所以要上锁
    //
    
    //此处因为有空闲线程，所以要用信号通知。
    
    //没有空闲线程则看他达到上线没有，没有则创建，传递函数和pool（是哪一个线程）
    //PS：线程处理函数，不是run,因为run是任务节点里面存放的，线程池里面的任务和任务节点的任务不一样
    //最后线程个数增加
    
    //如果达到上线，则在任务队列等待
    task_t *newtask = (task_t*)malloc(sizeof(task_t));
    newtask->run = run;
    newtask->arg = arg;
    condition_lock(&pool->ready);
    if(pool->first == NULL)
    {
        pool->first = newtask;
    }
    else
    {
        pool->last = newtask;
    }
    pool->last = newtask;
    //如果空闲线程数大于0则唤醒
    if(pool->idle > 0)
    {
        condition_signal(&pool->ready);
    }
    //如果没有空闲线程，当前线程数小于最大线程数时就给线程池中创建新线程
    else if(pool->counter < pool->max_threads)
    {
        pthread_t tid;
        pthread_create(&tid,NULL,rout,(void*)pool);
        pool->counter++;
    }
    condition_unlock(&pool->ready);
}

//销毁线程池
void threadpool_destroy(threadpool_t *pool){
    condition_lock(&pool->ready);
    if(pool->quit == 1)
    {
        condition_unlock(&pool->ready);
        return;
    }
    pool->quit = 1;

    if(pool->idle > 0)
    {
        condition_broadcast(&pool->ready);
    }
    while(pool->counter > 0)
    {
        condition_wait(&pool->ready);
    }
    condition_destroy(&pool->ready);
    condition_unlock(&pool->ready);
}
