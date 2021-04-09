#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <deque>
using namespace std;

// TODO: make it more memory optimized (re-unse memory)
template<typename T>
class LockFreeQueue
{
 public:
    LockFreeQueue() {}
    ~LockFreeQueue() {}

    bool empty() const { return mQueue.empty(); }
    long size() const { return mQueue.size(); }
    
    void clear() { mQueue.clear(); }

    void push(const T &value) { mQueue.push_back(value); }
    void peek(T &value) { if (!mQueue.empty()) value = mQueue[0]; }
    void pop() { mQueue.pop_front(); }

    void push(const LockFreeQueue<T> &q)
    { for (int i = 0; i < q.mQueue.size(); i++) mQueue.push_back(q.mQueue[i]); }
    
 protected:
    deque<T> mQueue;
};

#endif
