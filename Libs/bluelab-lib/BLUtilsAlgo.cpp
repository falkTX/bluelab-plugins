#include "BLUtilsAlgo.h"

template <typename FLOAT_TYPE>
void
BLUtilsAlgo::Rotate90(int width, int height, FLOAT_TYPE *x, FLOAT_TYPE *y,
                      bool flipX, bool flipY)
{
    *x = *x / width;
    *y = *y / height;
    
    FLOAT_TYPE tmp = *x;
    *x = *y;
    *y = tmp;
    
    *x *= width;
    *y *= height;
    
    if (flipX)
        *x = width - *x;

    if (flipY)
        *y = height - *y;
}
template void BLUtilsAlgo::Rotate90(int width, int height,
                                    float *x, float *y,
                                    bool flipX, bool flipY);
template void BLUtilsAlgo::Rotate90(int width, int height,
                                    double *x, double *y,
                                    bool flipX, bool flipY);
    
template <typename FLOAT_TYPE>
void
BLUtilsAlgo::Rotate90Delta(int width, int height, FLOAT_TYPE *dx, FLOAT_TYPE *dy,
                           bool flipX, bool flipY)
{
    *dx = *dx / width;
    *dy = *dy / height;
    
    FLOAT_TYPE tmp = *dx;
    *dx = *dy;
    *dy = tmp;
    
    *dx *= width;
    *dy *= height;
    
    if (flipX)
        *dx = -(*dx);

    if (flipY)
        *dy = -(*dy);
}
template void BLUtilsAlgo::Rotate90Delta(int width, int height,
                                         float *dx, float *dy,
                                         bool flipX, bool flipY);
template void BLUtilsAlgo::Rotate90Delta(int width, int height,
                                         double *dx, double *dy,
                                         bool flipX, bool flipY);
