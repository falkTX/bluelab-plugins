//
//  ImageInpaint2.h
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#ifndef __BL_Ghost__ImageInpaint2__
#define __BL_Ghost__ImageInpaint2__

// Use real inpainting by using isophotes
class ImageInpaint2
{
public:
    static void Inpaint(BL_FLOAT *image, int width, int height,
                        BL_FLOAT borderRatio,
                        bool processHorizontal, bool processVertical);
};

#endif /* defined(__BL_Ghost__ImageInpaint2__) */
