#ifndef NS_PROXYER_MSG_QUEUE_H_
#define NS_PROXYER_MSG_QUEUE_H_

#include "ringbuffer.h"

template <class T>
class MsgQueue
{
public:
    MsgQueue() : queue_(NULL)
    {
    }
    
    void init()
    {
        queue_ = RingBufferInit();
    }

    ~MsgQueue()
    {
        RingBufferDestroy(queue_);
    }

    virtual int enqueue(T* item)
    {
        return RingBufferMrMwPut(queue_, item);
    }
    
    virtual T* dequeue_no_wait()
    {
        return static_cast<T*>(RingBufferMrMwGetNoWait(queue_));
    }

	virtual T* peek()
	{
        return static_cast<T*>(RingBufferPeek(queue_));
	}
    
    virtual T* dequeue()
    {
        return static_cast<T*>(RingBufferMrMwGet(queue_));
    }

    virtual bool isEmpty()
    {
        return RingBufferIsEmpty(queue_);
    }
    
    virtual bool isFull()
    {
        return RingBufferIsFull(queue_);
    }
private:
    RingBuffer16* queue_;
};

#endif