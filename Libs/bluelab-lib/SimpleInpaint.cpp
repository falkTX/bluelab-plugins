#include "SimpleInpaint.h"

SimpleInpaint::SimpleInpaint(bool processHorizontal, bool processVertical)
{
    mProcessHorizontal = processHorizontal;
    mProcessVertical = processVertical;
}

SimpleInpaint::~SimpleInpaint() {}

void
SimpleInpaint::Process(WDL_TypedBuf<BL_FLOAT> *magns,
                       int width, int height)
{
    if ((width <= 2) || (height <= 2))
        return;
    
    if (mProcessHorizontal && !mProcessVertical)
    {
        ProcessHorizontal(magns, width, height);
        
        return;
    }

    if (!mProcessHorizontal && mProcessVertical)
    {
        ProcessVertical(magns, width, height);
        
        return;
    }

    // else...
    ProcessBothDir(magns, width, height);   
}

void
SimpleInpaint::ProcessHorizontal(WDL_TypedBuf<BL_FLOAT> *magns,
                                 int width, int height)
{
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT m0 = magns->Get()[0 + j*width];
        BL_FLOAT m1 = magns->Get()[(width - 1) + j*width];
        
        for (int i = 1; i < width - 1; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(width - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            magns->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaint::ProcessVertical(WDL_TypedBuf<BL_FLOAT> *magns,
                               int width, int height)
{
    for (int i = 0; i < width ; i++)
    {
        BL_FLOAT m0 = magns->Get()[i + 0*width];
        BL_FLOAT m1 = magns->Get()[i + (height - 1)*width];
        
        for (int j = 1; j < height - 1; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/(height - 1);

            BL_FLOAT m = (1.0 - t)*m0 + t*m1;

            magns->Get()[i + j*width] = m;
        }
    }
}

void
SimpleInpaint::ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                              int width, int height)
{
    WDL_TypedBuf<BL_FLOAT> horiz = *magns;
    ProcessHorizontal(&horiz, width, height);

    WDL_TypedBuf<BL_FLOAT> vert = *magns;
    ProcessHorizontal(&vert, width, height);

    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT h = horiz.Get()[i];
        BL_FLOAT v = vert.Get()[i];

        BL_FLOAT m = (h + v)*0.5;
        magns->Get()[i] = m;
    }
}
