#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
/*
*线程池里所有运行和等待的任务都是一个CThread_worker
*由于所有任务都在链表里，所以是一个链表结构
*/
typedef struct worker
{
    /*回调函数，任务运行时会调用此函数，注意也可声明成其它形式*/
    void *(*process) (void *arg);
    void *arg;/*回调函数的参数*/
    struct worker *next;
}CThread_worker;
 
/*线程池结构*/
typedef struct
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
    /*链表结构，线程池中所有等待任务*/
    CThread_worker *queue_head;
    /*是否销毁线程池*/
    int shutdown;
    pthread_t *threadid;
    /*线程池中允许的活动线程数目*/
    int max_thread_num;
    /*当前等待队列的任务数目*/
    int cur_queue_size;
}CThread_pool;

int pool_add_worker(void* (*process)(void *arg), void* arg);
static void* thread_routine(void* arg);//信号处理函数

static CThread_pool* pool;

void pool_init(int max_thread_num)
{
    pool = (CThread_pool*)malloc(sizeof(CThread_pool));
    pthread_cond_init(&(pool->queue_ready), NULL);
    pthread_mutex_init(&(pool->queue_lock),NULL);
    pool->queue_head = NULL;
    pool->shutdown = 0;
    pool->threadid = (pthread_t*)malloc(max_thread_num * sizeof(pthread_t));
    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;
    for(int i = 0; i<max_thread_num; i++)
        pthread_create(&(pool->threadid[i]), NULL, thread_routine, NULL);
}

int pool_add_worker(void *(*process) (void *arg), void* arg)
{
    CThread_worker* newworker = malloc( sizeof(CThread_worker));
    newworker->arg = arg;
    newworker->process = process;
    newworker->next = NULL;

    pthread_mutex_lock(&(pool->queue_lock));

    CThread_worker* number = pool->queue_head;
    if(number != NULL)
    {
        while(number->next != NULL)
            number = number->next;
    }
    else{
        pool->queue_head = newworker;
    }
    pool->cur_queue_size++;

    pthread_mutex_unlock(&(pool->queue_lock));
    pthread_cond_signal(&(pool->queue_ready));

    return 0;
}

int pool_destroy()
{
    if(pool->shutdown)
        return -1;//防止重复调用
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->queue_ready));//唤醒等待定所有线程，线程池要被销毁
    for(int i = 0; i < pool -> max_thread_num; i++)
    {
        pthread_join(pool->threadid[i], NULL);
    }
    free(pool->threadid);

    CThread_worker* head = NULL;
    while( pool -> queue_head != NULL)
    {
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(head);
    }
    pthread_cond_destroy(&(pool->queue_ready));
    pthread_mutex_destroy(&(pool->queue_lock));

    free(pool);
    pool = NULL;
    return 0;
}

void* thread_routine(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&(pool->queue_lock));
        //此处等待队列为0，并且不销毁线程池，则阻塞
        while(pool->cur_queue_size == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
        }
        //线程池要被销毁
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->queue_lock));
            pthread_exit(NULL);
        }

        //等待队列长度-1
        pool->cur_queue_size--;
        //取出链表中头元素
        CThread_worker* worker = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        pthread_mutex_unlock(&(pool->queue_lock));
        
        //调用回调函数，执行任务
        (*(worker->process)) (worker->arg);
        free(worker);
        worker = NULL;
    }
    //此处不可达
    pthread_exit(NULL);
}

void* myprocess(void* arg)
{
    printf("ha: 0x%u, work %d \n",(unsigned int)pthread_self(),*(int*) arg);
    sleep(1);
    return NULL;
}

int main()
{
    pool_init (3);

    //连续投放十个任务
    int *workernum = (int*)malloc(sizeof(int)*10);
    for(int i =0;i<10;i++)
    {
        workernum[i] = i;
        pool_add_worker(myprocess,&workernum[i]);
    }
    //等待所有任务
    sleep(5);
    pool_destroy();
    free(workernum);
    workernum = NULL;
    return 0;
}
