#ifndef SIMPLE_INPAINT_H
#define SIMPLE_INPAINT_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Very simple, but no so bad on real cases!
class SimpleInpaint
{
 public:
    SimpleInpaint(bool processHorizontal, bool processVertical);
    virtual ~SimpleInpaint();

    void Process(WDL_TypedBuf<BL_FLOAT> *magns,
                 int width, int height);
    
 protected:
    void ProcessHorizontal(WDL_TypedBuf<BL_FLOAT> *magns,
                           int width, int height);

    void ProcessVertical(WDL_TypedBuf<BL_FLOAT> *magns,
                         int width, int height);

    void ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                        int width, int height);

    //
    bool mProcessHorizontal;
    bool mProcessVertical;
};

#endif
