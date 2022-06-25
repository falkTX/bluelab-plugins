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
 
//
//  ChunksHelper.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 25/04/17.
//
//

#if 0
#include "ChunkHelper.h"


int
ChunkHelper::GetSize(ByteChunk &chunk)
{
    int size = chunk.Size();
    if (size > 0)
        // Skipe the 'int' which indicates the size and
        // which is encoded at the beginning of the buffer chunk
        size -= 4;
    
    return size;
}

int
ChunkHelper::GetDoubleSize(ByteChunk &chunk)
{
    int size = ChunkHelper::GetSize(chunk)/sizeof(BL_FLOAT);
    
    return size;
}
#endif
