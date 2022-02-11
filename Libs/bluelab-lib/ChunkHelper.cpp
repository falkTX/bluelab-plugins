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
