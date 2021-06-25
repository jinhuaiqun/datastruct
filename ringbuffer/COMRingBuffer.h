/*
 * @文件名称: 
 * @描述: 
 * @版权: Copyright(c)2020,Beijingtongtech.co.ltdAllrightsreserved
 * @版本: 9.0.0
 * @备注: 
 */
#ifndef __COMRINGBUFFER_H__
#define __COMRINGBUFFER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0

//	__asm__ __volatile__("" : : : "memory");
	
static inline void ComMfence(void)
{
#if defined(__i386__) || defined(__x86_64__)
	__asm__ __volatile__("mfence" : : : "memory");
#elif defined(__ARM_ARCH) || defined(__ARM_64BIT_STATE) || defined(__arm64__) || defined(__aarch64__)
	__asm__ __volatile__("dmb st" : : : "memory");


#elif defined(_MIPS_ARCH_LOONGSON3A)

asm volatile( \
".set mips2\n\t"\
"sync\n\t"\
".set mips0"\
:/*nooutput*/\
:/*noinput*/\
:"memory");

#endif
}


#define COM_MALLOC_INIT(old, type, size, action)                                                                       \
	do {                                                                                                               \
		int   _maxAttempts;                                                                                            \
		int   _size;                                                                                                   \
		void* _ptr = NULL;                                                                                             \
		for (_maxAttempts = 3, _size = COM_MAX(size, 1); 0 < _maxAttempts && NULL == _ptr;                             \
				_ptr = malloc(_size), _maxAttempts--)                                                                     \
		;                                                                                                          \
		if (NULL != _ptr) {                                                                                            \
			old = (type)_ptr;                                                                                          \
			memset(old, 0, _size);                                                                                     \
		}                                                                                                              \
		else {                                                                                                         \
			printf("Malloc failed, size:%d", size);                                                                     \
			action                                                                                                     \
		}                                                                                                              \
	} while (false);


#define COM_MEM_FREE(X)                                                                                                \
	do {                                                                                                               \
		if (X) {                                                                                                       \
			free(X);                                                                                                   \
			X = NULL;                                                                                                  \
		}                                                                                                              \
	} while (false);

#define COM_MAX(a, b) (((a) > (b)) ? (a) : (b))
#if defined(COM_COMPILER_IS_GCC)
#    define __com_aligned__(a) __attribute__((aligned(a)))

#elif defined(COM_COMPILER_IS_MSVC)
#    define __com_aligned__(a) __declspec(align(a))
#else
#    define __com_aligned__(a)
#endif

#define TRUE 1
#define FALSE 0

typedef struct _RingBuffer RingBuffer;

RingBuffer *RingBufferCreate(int n);
void RingBufferDestroy(RingBuffer *rb);
void *RingBufferSrGet(RingBuffer *rb);
int RingBufferSwPut(RingBuffer *rb, void *ptr);

#endif /* end of include guard: __COMRINGBUFFER_H__ */
