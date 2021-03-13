#ifndef SIMPLE_INPAINT_COMP_H
#define SIMPLE_INPAINT_COMP_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// SimpleInpaint: Very simple, but no so bad on real cases!
// SimpleInpaintComp: try to improves phases issues
class SimpleInpaintComp
{
 public:
    SimpleInpaintComp(bool processHorizontal, bool processVertical);
    virtual ~SimpleInpaintComp();

    void Process(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 int width, int height);
    
 protected:
    void ProcessHorizontal(WDL_TypedBuf<WDL_FFT_COMPLEX> *values,
                           int width, int height);

    void ProcessVertical(WDL_TypedBuf<WDL_FFT_COMPLEX> *values,
                         int width, int height);

    void ProcessBothDir(WDL_TypedBuf<WDL_FFT_COMPLEX> *values,
                        int width, int height);

    //
    bool mProcessHorizontal;
    bool mProcessVertical;
};

#endif
