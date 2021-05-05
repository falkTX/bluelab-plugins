#ifndef BL_UTILS_ALGO_H
#define BL_UTILS_ALGO_H

#include <BLTypes.h>

class BLUtilsAlgo
{
 public:
    // "swap" x and y, used when rotating a view
    static void Rotate90(int width, int height, BL_FLOAT *x, BL_FLOAT *y,
                         bool flipX, bool flipY);
};

#endif
