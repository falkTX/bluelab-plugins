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

void
BLUtilsAlgo::Rotate90Delta(int width, int height, BL_FLOAT *dx, BL_FLOAT *dy,
                           bool flipX, bool flipY)
{
    *dx = *dx / width;
    *dy = *dy / height;
    
    BL_FLOAT tmp = *dx;
    *dx = *dy;
    *dy = tmp;
    
    *dx *= width;
    *dy *= height;
    
    if (flipX)
        *dx = -(*dx);

    if (flipY)
        *dy = -(*dy);
}
