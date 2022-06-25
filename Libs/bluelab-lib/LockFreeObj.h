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
