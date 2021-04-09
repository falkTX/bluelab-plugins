#ifndef LOCK_FREE_QUEUE2_H
#define LOCK_FREE_QUEUE2_H

#include <vector>
using namespace std;

// Reuse memory
template<typename T>
class LockFreeQueue2
{
 public:
    LockFreeQueue2() { mSize = 0; }
    ~LockFreeQueue2() {}

    bool empty() const { return (mSize == 0); }
    long size() const { return mSize; }
    
    void clear() { mSize = 0; }

    void push(const T &value)
    { if (mSize < mQueue.size())
        { mQueue[mSize] = value; mSize++; }
        else
        { mQueue.push_back(value); mSize = mQueue.size(); } }
    
    void get(long index, T &value)
    { if (index < mSize) value = mQueue[index]; }

    void push(const LockFreeQueue2<T> &q)
    { for (int i = 0; i < q.mSize; i++) push(q.mQueue[i]); }
    
 protected:
    vector<T> mQueue;
    long mSize;
};

#endif
