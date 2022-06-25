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
