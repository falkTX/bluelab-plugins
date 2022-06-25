/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
    void set(long index, const T &value)
    { if (index < mSize) mQueue[index] = value; }
    
    void push(const LockFreeQueue2<T> &q)
    { for (int i = 0; i < q.mSize; i++) push(q.mQueue[i]); }
    void set(long index, const LockFreeQueue2<T> &q)
    { for (int i = 0; i < q.mSize; i++) set(index, q.mQueue[i]); }
    
 protected:
    vector<T> mQueue;
    long mSize;
};

#endif
