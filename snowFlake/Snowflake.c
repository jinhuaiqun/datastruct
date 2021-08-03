#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>

#define SEQ_BITS 12
#define HOST_BITS 10
#define TIMESTAMP_BITS 41

#define TIMESTAMP_LEFTSHIFT 22
#define HOST_LEFTSHIFT 12

#define SEQ_MASK (-1^(-1<<SEQ_BITS))
#define MAX_HOSTID (-1^(-1<<HOST_BITS))

#define TWEPOCH 1609459200000 /*开始时间戳 2021-01-01 00:00:00.000*/


typedef struct Snowflake{
    pthread_mutex_t mutex;
    uint64_t lastTimeStamp; 
    uint32_t sequence;
    uint32_t hostId;
    uint64_t (*GetId)(struct Snowflake *); 
} Snowflake_t;

static uint64_t GetTimeStamp(void)
{
    struct timeval t;

    gettimeofday(&t, NULL);    

    return t.tv_sec*1000+t.tv_usec/1000;
}

static uint64_t TillNextMilSecond(uint64_t lastTimeStamp)
{
    uint64_t timeStamp = GetTimeStamp(); 

    while (timeStamp <= lastTimeStamp) {
        timeStamp = GetTimeStamp();
    }
    return timeStamp;
}


static uint64_t GetId(Snowflake_t *snow)
{
    int32_t hostId = 0;
    uint64_t timeStamp = 0;
    uint64_t lastTimeStamp = 0;
    uint32_t sequence = 0;

    pthread_mutex_lock(&snow->mutex);

    hostId = snow->hostId;
    lastTimeStamp = snow->lastTimeStamp;
    sequence = snow->sequence;
    
    timeStamp = GetTimeStamp();
    if (timeStamp < lastTimeStamp) {
        printf("clock moved backwards.  Refusing to generate id for %ld milliseconds\n", lastTimeStamp - timeStamp);
        pthread_mutex_unlock(&snow->mutex);
        return 0;
    } 

    if (timeStamp == lastTimeStamp) {
        sequence = (sequence + 1) & SEQ_MASK;
        if (sequence == 0) {
            timeStamp = TillNextMilSecond(lastTimeStamp);     
        }

    } else { 
        sequence = 0;        
    }
    
    snow->lastTimeStamp = timeStamp;
    snow->sequence = sequence;

    pthread_mutex_unlock(&snow->mutex);

    return ((timeStamp-TWEPOCH)<<TIMESTAMP_LEFTSHIFT) 
            | (hostId << HOST_LEFTSHIFT)
            | (sequence);
}

int initSnowflake(Snowflake_t *snow, uint32_t hostId)
{
    if (snow == NULL)
        return -1;

    if (hostId > MAX_HOSTID)
        return -1;

    pthread_mutex_init(&snow->mutex, NULL);
    snow->hostId = hostId; 
    snow->sequence = 0;
    snow->lastTimeStamp = 0;

    snow->GetId = GetId;
    return 0;
}

void destroySnowflake(Snowflake_t *snow)
{
    if (snow == NULL)
        return;

    pthread_mutex_destroy(&snow->mutex);    
}

void *thread_func(void *arg)
{
    int i = 0;
    Snowflake_t *pSnow = (Snowflake_t *)arg;

    for (i = 0; i < 3000000; i++) {
         pSnow->GetId(pSnow);
    }
}

int main(void)
{
    int i = 0;
    int ret = 0;
    uint32_t hostId = 111;
    pthread_t tid[10];
    Snowflake_t snow;
    struct timeval tv1,tv2;

    ret = initSnowflake(&snow, hostId);
    assert(ret != -1);

    gettimeofday(&tv1, NULL);

    for (i = 0; i < 10; i++) {
        ret = pthread_create(&tid[i], NULL, thread_func, (void *)&snow);
        assert(ret == 0);
    }

    for (i = 0; i < 10; i++) {
        pthread_join(tid[i], NULL);
    }
    
    destroySnowflake(&snow);

    gettimeofday(&tv2, NULL);
    uint64_t time = (((uint64_t)tv2.tv_sec * 1000000LLU) + ((uint64_t)tv2.tv_usec)) - (((uint64_t)tv1.tv_sec * 1000000LLU) + ((uint64_t)tv1.tv_usec));
    printf("Use time %ld milsecond\n", time/1000);
    return 0;    
}

