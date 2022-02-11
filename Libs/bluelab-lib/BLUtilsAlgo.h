#ifndef BL_UTILS_ALGO_H
#define BL_UTILS_ALGO_H

#include <BLTypes.h>

class BLUtilsAlgo
{
 public:
    // "swap" x and y, used when rotating a view
    template <typename FLOAT_TYPE>
    static void Rotate90(int width, int height, FLOAT_TYPE *x, FLOAT_TYPE *y,
                         bool flipX, bool flipY);

    template <typename FLOAT_TYPE>
    static void Rotate90Delta(int width, int height, FLOAT_TYPE *dx, FLOAT_TYPE *dy,
                              bool flipX, bool flipY);
};

#endif
