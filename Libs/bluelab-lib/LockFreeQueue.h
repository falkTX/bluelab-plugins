#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <vector>
using namespace std;

// NOTE: not totally memory optimized
// (we will clear() often, then re-push().
// Memory is not re-used
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
    
    void get(long index, T &value)
    { if (index < mQueue.size()) value = mQueue[index]; }

    void push(const LockFreeQueue<T> &q)
    { for (int i = 0; i < q.mQueue.size(); i++) mQueue.push_back(q.mQueue[i]); }
    
 protected:
    vector<T> mQueue;
};

#endif
