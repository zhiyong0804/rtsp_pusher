#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <string.h>
#include "ringbuffer.h"

RingBuffer16 *RingBufferInit(void)
{
    RingBuffer16 *rb = (RingBuffer16 *)malloc(sizeof(RingBuffer16));
    if (unlikely(rb == NULL)) {
        return NULL;
    }

    memset(rb, 0, sizeof(RingBuffer16));

    rb->write = 0;
    rb->read = 0;
    INIT_SPINLOCK(rb->spin);

    return rb;
}

void RingBufferDestroy(RingBuffer16 *rb)
{
    if (rb != NULL) {
        free(rb);
    }
}

/**
 * @brief check the ringbuffer is empty (no data in it)
 *
 * @param rb ringbuffer
 * 
 * @retval 1 empty
 * @retval 0 not empty
 */
int RingBufferIsEmpty(RingBuffer16 *rb)
{
    return (rb->write == rb->read);
}

/**
 * @brief check the ringbuffer is full (no more data will fit)
 *
 * @param rb ringbuffer
 *
 * @retval 1 empty
 * @retval 0 no empty
 */
int RingBufferIsFull(RingBuffer16 *rb)
{
    return ((unsigned short)(rb->write + 1) == rb->read);
}

void *RingBufferPeek(RingBuffer16 *rb)
{
    return rb->array[rb->read];
}


/**
 * @brief get number of items in the ringbuffer
 *
 * @param rb ringbuffer
 */
uint16_t RingBufferSize(RingBuffer16 *rb)
{
    return (uint16_t)(rb->write - rb->read);
}

/**
 * @brief wait function for condition where ringbuffer is either full or empty.
 * 
 * @param rb ringbuffer
 *
 * Based on RINGBUFFER_MUTEX_WAIT define, we either sleep and spin or use thread
 * condition to wait.
 */
void RingBufferWait(RingBuffer16 *rb __attribute__((unused)))
{
    usleep(5);
}

/**
 * @brief get the next ptr from the ring buffer
 *
 * Because we allow for multiple readers we take great care in making sure that
 * the threads don't interfere with one another.
 */
void *RingBufferMrSwGet(RingBuffer16 *rb)
{
    void *ptr;
    /**
     * local pointer for data races. If CAS fails we increase our local array
     * idx to try the next array member until we succeed. Or when the buffer is
     * empty again we jump back to the waiting loop.
     */
    unsigned short readp;

    /* buffer is empty, wait... */
retry:
    while (rb->write == rb->read) {
        RingBufferWait(rb);
    }

    /* atomically update rb->read */
    readp = rb->read - 1;
    do {
        /* with multiple readers we can get in the situation that we exitted
         * from the wait loop buf the rb is empty again once we get here. */
        if (rb->write == rb->read)
            goto retry;

        readp++;
        ptr = rb->array[readp];
    } while (!__sync_bool_compare_and_swap(&rb->read, readp, (readp + 1)));

    return ptr;
}

/**
 * @brief put a ptr in the RingBuffer
 */
int RingBufferMrSwPut(RingBuffer16 *rb, void *ptr)
{
    /* buffer is full, wait... */
    while ((unsigned short)(rb->write + 1) == rb->read) {
        RingBufferWait(rb);
    }

    rb->array[rb->write] = ptr;
    (void)__sync_add_and_fetch(&rb->write, 1);

    return 0;
}

void *RingBufferSrSwGet(RingBuffer16 *rb)
{
    void *ptr = NULL;

    /* buffer is empty, wait... */
    while (rb->write == rb->read) {
        RingBufferWait(rb);
    }

    ptr = rb->array[rb->read];
    (void)__sync_add_and_fetch(&rb->read, 1);

    return ptr;
}

int RingBufferSrSwPut(RingBuffer16 *rb, void *ptr)
{
    /* buffer is full, wait... */
    while ((unsigned short)(rb->write + 1) == rb->read) {
        RingBufferWait(rb);
    }

    rb->array[rb->write] = ptr;
    (void)__sync_add_and_fetch(&rb->write, 1);

    return 0;
}

/**
 * @brief get the next ptr from the ring buffer
 *
 * Because we allow for multiple readers we take great case in making sure that
 * we thread don't intefere with one another.
 */
void *RingBufferMrMwGet(RingBuffer16 *rb)
{
    void *ptr;
    /**
     * local pointer for data races. If CAS fails we increase our local array
     * idx to try the next array member until we succeed. Or when the buffer is
     * empty again we jump back to the waiting loop.
     */
    unsigned short readp;

    /* buffer is empty, wait... */
retry:
    while (rb->write == rb->read) {
        RingBufferWait(rb);
    }

    /* atomically update rb->read */
    readp = rb->read - 1;
    do {
        /* with multiple readers we can get in the situation that we exited from
         * the wait loop but the rb is empty again once we get here.
         */
        if (rb->write == rb->read)
            goto retry;

        readp++;
        ptr = rb->array[readp];
    } while (!__sync_bool_compare_and_swap(&rb->read, readp, (readp + 1)));

    return ptr;
}

void *RingBufferMrMwGetNoWait(RingBuffer16 *rb)
{
    void *ptr;
    /**
     * local pointer for data races. If CAS fails we increase our local array
     * idx to try the next array member until we succeed. Or when the buffer is
     * empty again we jump back to the waiting loop.
     */
    unsigned short readp;

    /* buffer is empty, wait... */
retry:
    while (rb->write == rb->read) {
        return NULL;
    }

    /* atomically update rb->read */
    readp = rb->read - 1;
    do {
        /* with multiple readers we can get in the situation that we exited from
         * the wait loop but the rb is empty again once we get here.
         */
        if (rb->write == rb->read)
            goto retry;

        readp++;
        ptr = rb->array[readp];
    } while (!__sync_bool_compare_and_swap(&rb->read, readp, (readp + 1)));

    return ptr;

}

/**
 * @brief put a ptr in the RingBuffer.
 *
 * As we support multiple writers we need to protect 2 things:
 *   1. writing the ptr to the array
 *   2. incrementing the rb->write idx
 *
 * We can't do both at the same time in one atomic operation, so we need to
 * (spin) lock it. We do increment rb->write atomically after that, so that we
 * don't need to use the lock in out *Get function.
 *
 * @param rb the ringbuffer
 * @param ptr ptr to store
 *
 * @retval 0 ok
 * @retval -1 wait loop interrupted because of engine flags
 */
int RingBufferMrMwPut(RingBuffer16 *rb, void *ptr)
{
retry:
    while ((unsigned short)(rb->write + 1) == rb->read) {
        RingBufferWait(rb);
    }

    /* get our lock */
    spinlock_lock(&rb->spin);
    /* if while we got our lock the buffer changed, we need to retry */
    if ((unsigned short)(rb->write + 1) == rb->read) {
        spinlock_unlock(&rb->spin);
        goto retry;
    }

    /* update the ring buffer */
    rb->array[rb->write] = ptr;
    (void)__sync_add_and_fetch(&rb->write, 1);
    spinlock_unlock(&rb->spin);

    return 0;
}
