#include "BLUtilsAlgo.h"

void
BLUtilsAlgo::Rotate90(int width, int height, BL_FLOAT *x, BL_FLOAT *y,
                      bool flipX, bool flipY)
{
    *x = *x / width;
    *y = *y / height;
    
    BL_FLOAT tmp = *x;
    *x = *y;
    *y = tmp;
    
    *x *= width;
    *y *= height;
    
    if (flipX)
        *x = width - *x;

    if (flipY)
        *y = height - *y;
}
