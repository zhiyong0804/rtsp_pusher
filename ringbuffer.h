#ifndef RINGBUFFER_H_HRFNP6CQ
#define RINGBUFFER_H_HRFNP6CQ

#include <pthread.h>
#include <inttypes.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define RING_BUFFER_16_SIZE 65535

typedef volatile int spinlock_t;

#define INIT_SPINLOCK(lock) lock = 0

#define spinlock_lock(lock)                           \
do {                                                  \
    while (!__sync_bool_compare_and_swap(lock, 0, 1)) \
    sched_yield();                                \
} while(0)

#define spinlock_unlock(lock) \
do {                          \
    *lock = 0;                \
} while(0)

typedef struct RingBuffer16_
{
    unsigned short write;
    unsigned short read;
    spinlock_t spin;
    void *array[RING_BUFFER_16_SIZE];
} RingBuffer16;

RingBuffer16 *RingBufferInit(void);
void RingBufferDestroy(RingBuffer16 *);
int RingBufferIsEmpty(RingBuffer16 *);
int RingBufferIsFull(RingBuffer16 *);
uint16_t RingBufferSize(RingBuffer16 *);
void RingBufferWait(RingBuffer16 *rb);

/**
 * Multiple Reader, Single Write ring buffer, fixed at 65536 items so we can use
 * unsigned shorts that just wrap around
 */
void *RingBufferMrSwGet(RingBuffer16 *);
int RingBufferMrSwPut(RingBuffer16 *, void *);

/**
 * Single Reader, Single Writer ring buffer, fixed at 65536 items so we can use
 * unsigned shorts that just wrap around
 */
void *RingBufferSrSwGet(RingBuffer16 *);
int RingBufferSrSwPut(RingBuffer16 *, void *);

/**
 * Multiple Reader, Multi Writer ring buffer, fixed at 65536 items so we can use
 * unsigned char's that just wrap around
 */
void *RingBufferMrMwGet(RingBuffer16 *);
void *RingBufferMrMwGetNoWait(RingBuffer16 *);
int RingBufferMrMwPut(RingBuffer16 *, void *);
void *RingBufferPeek(RingBuffer16 *);

#endif /* end of include guard: RINGBUFFER_H_HRFNP6CQ */
