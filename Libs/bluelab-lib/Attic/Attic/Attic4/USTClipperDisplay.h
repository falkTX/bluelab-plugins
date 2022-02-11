//
//  USTClipperDisplay.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipperDisplay__
#define __UST__USTClipperDisplay__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class FifoDecimator2;

class USTClipperDisplay
{
public:
    USTClipperDisplay(GraphControl11 *graph);
    
    virtual ~USTClipperDisplay();
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    GraphControl11 *mGraph;
    
    FifoDecimator2 *mFifoDecim;
};

#endif /* defined(__UST__USTClipperDisplay__) */
