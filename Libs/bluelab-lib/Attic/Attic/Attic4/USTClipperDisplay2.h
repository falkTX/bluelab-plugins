//
//  USTClipperDisplay2.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipperDisplay2__
#define __UST__USTClipperDisplay2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class FifoDecimator2;

// Draw filled waveforms
// Draw clipped waveform with another color
class USTClipperDisplay2
{
public:
    USTClipperDisplay2(GraphControl11 *graph);
    
    virtual ~USTClipperDisplay2();
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    GraphControl11 *mGraph;
    
    FifoDecimator2 *mFifoDecim;
    FifoDecimator2 *mFifoDecimClip;
};

#endif /* defined(__UST__USTClipperDisplay2__) */
