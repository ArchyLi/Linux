#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "cond.h"

//初始化
int condition_init(condition_t *pc){
    int ret = 0;
    do{
        if((ret = pthread_mutex_init(&(pc->mutex),NULL)) != 0)
            break;
        if((ret = pthread_cond_init(&(pc->cond), NULL))!= 0)
            break;
    }while(0);
    return ret;
}

//加锁
int condition_lock(condition_t *pc){
    return pthread_mutex_lock(&(pc->mutex));
}

//解锁
int condition_unlock(condition_t *pc){
    return pthread_mutex_unlock(&(pc->mutex));
}

//等待
int condition_wait(condition_t *pc){
    return pthread_cond_wait(&(pc->cond),&(pc->mutex));
}

//超时等待，如果超时则返回
int condition_timewait(condition_t* pc,const struct timespec* pt){
    return pthread_cond_timedwait(&(pc->cond), &(pc->mutex),pt);
}

//信号通知
int condition_signal(condition_t *pc){
    return pthread_cond_signal(&(pc->cond));
}

//
int condition_broadcast(condition_t* pc){
    return pthread_cond_broadcast(&(pc->cond));
}

// 销毁
int condition_destroy(condition_t* pc){
    pthread_mutex_destroy(&(pc->mutex));
    pthread_cond_destroy(&(pc->cond));
    return 0;
}
