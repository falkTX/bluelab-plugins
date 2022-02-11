//
//  DitherMaker.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#ifndef Denoiser_DitherMaker_h
#define Denoiser_DitherMaker_h

class DitherMaker
{
public:
    static void Dither(double *samples, int nFrames);
};
#endif
