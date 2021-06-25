/*
 * @文件名称:
 * @描述:
 * @版权:  Copyright (c) 2020, Beijing tongtech.co.ltd All rights reserved
 * @版本: 9.0.0
 * @备注:
 */

#include "COMRingBuffer.h"

#define RING_MAX_SIZE 30
#define CACHE_LINE_SIZE 64
#define MAX_LOOP 100000000

pthread_spinlock_t  spin;

struct _RingBuffer
{
    volatile uint32_t write __com_aligned__(CACHE_LINE_SIZE); /* index where we put data */
    volatile uint32_t read  __com_aligned__(CACHE_LINE_SIZE); /* index where we get data */
    uint32_t                mask;                             /**/
    uint32_t                size;
    void**                  array;
};

RingBuffer* RingBufferCreate(int n)
{
    RingBuffer* rb = NULL;

	if (0 >= n || RING_MAX_SIZE < n) {
		return NULL;
	}

	COM_MALLOC_INIT(rb, RingBuffer*, sizeof(RingBuffer), return NULL;);
	rb->size = (1 << n) + 1;
	rb->mask = (1 << n) - 1;
	COM_MALLOC_INIT(rb->array, void**, sizeof(void*) * rb->size, COM_MEM_FREE(rb); return NULL;)

	return rb;
}

void  RingBufferDestroy(RingBuffer* rb)
{
	if (NULL != rb && NULL != rb->array) {
        COM_MEM_FREE(rb->array);
    }

    COM_MEM_FREE(rb);
}

/* Single Read */
void* RingBufferSrGet(RingBuffer* rb)
{
	void *ptr = NULL;

	/* ringbuffer empty */
    //pthread_spin_lock(&spin);
	if (NULL == rb || rb->write == rb->read) {
      //  pthread_spin_unlock(&spin);
        return NULL;
	}

    //	barrier();
	ComMfence();
	ptr = rb->array[rb->read];
	rb->read = (rb->read + 1) & rb->mask;

    //pthread_spin_unlock(&spin);
	return ptr;
}

/*Single Write*/
int RingBufferSwPut(RingBuffer* rb, void *ptr)
{
	if (NULL == rb || NULL == rb->array || NULL == ptr) {
		return -1;
	}


	/* ringbuffer is full */
   // pthread_spin_lock(&spin);
	if (rb->read == ((rb->write + 1) & rb->mask)) {
    //    pthread_spin_unlock(&spin);
        return -1;
	}
	rb->array[rb->write] = ptr;
	ComMfence();
	rb->write = (rb->write + 1) & rb->mask;
   // pthread_spin_unlock(&spin);

	return 0;
}

void *thread1(void *arg)
{
    RingBuffer *rb = (RingBuffer *)arg;
    int arraysize = (1 << 21) + 1;
    uint32_t mask = (1 << 21) - 1;
    uint64_t loop_mask = (1 << 64) -1;
    uint64_t loop = 0;
    int32_t index = 0;

    uint64_t *arr = (uint64_t*)malloc(sizeof(uint64_t)*arraysize);

    printf("loop_mask = %lu, arraysize=%ld\n", loop_mask, arraysize);

    for (;loop < MAX_LOOP;) {

        arr[index] = loop;
        if (0 != RingBufferSwPut(rb, (void *)&arr[index])) {
            //printf("RingBufferSwPut full\n");
            continue; 
        }

        index = (index + 1) & mask;

        loop++;
    }
    //printf("put index :%d, value:%lu\n", index, arr[index]);
}

void *thread2(void *arg)
{
    RingBuffer *rb = (RingBuffer *)arg;
    int i = 0;
    uint64_t value, *ptr = NULL, loop = 0;

    for (;loop < MAX_LOOP;) {
        ptr = RingBufferSrGet(rb);
        if (NULL == ptr) {
            continue; 
        }

        value = *(uint64_t*)ptr;
        if (value != loop) {
            printf("## [ERROR] value!=loop value:%lu, loop:%lu\n", value, loop);
            exit(1); 
        }
            //printf("value:%lu, loop:%lu\n", value, loop);
        loop++;
    }
}

int main(int argc, char *argv[])
{
    pthread_t t1, t2;
    RingBuffer *rb;
    struct timeval start, end;
    gettimeofday(&start, NULL);

//    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    rb = RingBufferCreate(20);
    assert(rb != NULL);

    pthread_create(&t1, 0, thread1, (void *)rb);
    pthread_create(&t2, 0, thread2, (void *)rb);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    RingBufferDestroy(rb);

    gettimeofday(&end, NULL);
    long long total_time = end.tv_sec - start.tv_sec;

    printf("Ringbuffer with nothing: Use %lld seconds\n", total_time);

	return 0;
}
