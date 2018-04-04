
#pragma once
#include <pthread.h>
#include <sys/time.h>
typedef struct condition{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}condition_t;
//初始化
int condition_init(condition_t *pc);

//加锁
int condition_lock(condition_t *pc);

//解锁
int condition_unlock(condition_t *pc);


int condition_wait(condition_t *pc);

int condition_timewait(condition_t* pc,const struct timespec* pt);

int condition_signal(condition_t *pc);

int condition_broadcast(condition_t* pc);

int condition_destroy(condition_t* pc);
