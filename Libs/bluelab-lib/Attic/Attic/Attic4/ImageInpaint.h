//
//  ImageInpaint.h
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#ifndef __BL_Ghost__ImageInpaint__
#define __BL_Ghost__ImageInpaint__

// Uses "gti" (gaussian)
// Works when the background to keep is a regular texture
// (such as noise, or regular background)
//
// Not desinged to manage shapes
//
class ImageInpaint
{
public:
    static void Inpaint(BL_FLOAT *image, int width, int height,
                        BL_FLOAT borderRatio);
};

#endif /* defined(__BL_Ghost__ImageInpaint__) */
