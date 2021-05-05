#ifndef BL_UTILS_ALGO_H
#define BL_UTILS_ALGO_H

#include <BLTypes.h>

class BLUtilsAlgo
{
 public:
    // "swap" x and y, used when rotating a view
    static void Rotate90(int width, int height, BL_FLOAT *x, BL_FLOAT *y,
                         bool flipX, bool flipY);

    static void Rotate90Delta(int width, int height, BL_FLOAT *dx, BL_FLOAT *dy,
                              bool flipX, bool flipY);
};

#endif
