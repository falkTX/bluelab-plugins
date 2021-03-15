#include <BLUtils.h>

#include "SimpleInpaintComp.h"

SimpleInpaintComp::SimpleInpaintComp(bool processHorizontal, bool processVertical)
{
    mProcessHorizontal = processHorizontal;
    mProcessVertical = processVertical;
}

SimpleInpaintComp::~SimpleInpaintComp() {}

void
SimpleInpaintComp::Process(WDL_TypedBuf<BL_FLOAT> *magns,
                           WDL_TypedBuf<BL_FLOAT> *phases,
                           int width, int height)
{
    if ((width <= 2) || (height <= 2))
        return;

    WDL_TypedBuf<WDL_FFT_COMPLEX> comp;
    BLUtils::MagnPhaseToComplex(&comp, *magns, *phases);

    if (mProcessHorizontal && !mProcessVertical)
    {
        ProcessHorizontal(&comp, width, height);
    }
    else if (!mProcessHorizontal && mProcessVertical)
    {
        ProcessVertical(&comp, width, height);
    }
    else
    {
        ProcessBothDir(&comp, width, height);
    }
    
    BLUtils::ComplexToMagnPhase(magns, phases, comp);
}

void
SimpleInpaintComp::ProcessHorizontal(WDL_TypedBuf<WDL_FFT_COMPLEX> *values,
                                     int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        const WDL_FFT_COMPLEX &c0 = values->Get()[0 + j*width];
        const WDL_FFT_COMPLEX &c1 = values->Get()[(width - 1) + j*width];
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            WDL_FFT_COMPLEX &c = values->Get()[i + j*width];
            
            c.re = (1.0 - t)*c0.re + t*c1.re;
            c.im = (1.0 - t)*c0.im + t*c1.im;
        }
    }
}

void
SimpleInpaintComp::ProcessVertical(WDL_TypedBuf<WDL_FFT_COMPLEX> *values,
                                   int width, int height)
{
    for (int i = 0; i < width ; i++)
    {
        const WDL_FFT_COMPLEX &c0 = values->Get()[i + 0*width];
        const WDL_FFT_COMPLEX &c1 = values->Get()[i + (height - 1)*width];
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);

            WDL_FFT_COMPLEX &c = values->Get()[i + j*width];
            
            c.re = (1.0 - t)*c0.re + t*c1.re;
            c.im = (1.0 - t)*c0.im + t*c1.im;
        }
    }
}

void
SimpleInpaintComp::ProcessBothDir(WDL_TypedBuf<WDL_FFT_COMPLEX> *values,
                                  int width, int height)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> horiz = *values;
    ProcessHorizontal(&horiz, width, height);

    WDL_TypedBuf<WDL_FFT_COMPLEX> vert = *values;
    ProcessHorizontal(&vert, width, height);

    for (int i = 0; i < values->GetSize(); i++)
    {
        const WDL_FFT_COMPLEX &h = horiz.Get()[i];
        const WDL_FFT_COMPLEX &v = vert.Get()[i];

        WDL_FFT_COMPLEX &c = values->Get()[i];
        
        c.re = (h.re + v.re)*0.5;
        c.im = (h.im + v.im)*0.5;
    }
}
