//
//  DitherMaker.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#ifndef Denoiser_DitherMaker_h
#define Denoiser_DitherMaker_h

#include <BLTypes.h>


class DitherMaker
{
public:
    static void Dither(BL_FLOAT *samples, int nFrames);
};
#endif
