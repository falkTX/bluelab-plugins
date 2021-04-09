#ifndef LOCK_FREE_OBJ_H
#define LOCK_FREE_OBJ_H

#define LOCK_FREE_NUM_BUFFERS 3

// buffer 0: filled by the audio thread
// buffer 1: intermediate buffer, will be locked for transmission
// between audio thread and gui thread
// buffer 2: read by the gui thread. Used for apply data in gui thread
class LockFreeObj
{
 public:
    // Copy from buffer 0 to buffer 1
    virtual void PushData() {}
    
    // Copy from buffer 1 to buffer 2
    virtual void PullData() {}

    // Apply data from buffer 2
    virtual void ApplyData() {}
};

#endif
