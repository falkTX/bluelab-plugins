//
//  USTClipper4.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipper4__
#define __UST__USTClipper4__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class USTClipperDisplay3;
class USTClipperDisplay4;
class ClipperOverObj4;
class DelayObj4;

// From USTClipper3
// - try to have the same quality as SIR Clipper
class USTClipper4
{
public:
    USTClipper4(GraphControl11 *graph, BL_FLOAT sampleRate);
    
    virtual ~USTClipper4();
    
    //void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    void SetEnabled(bool flag);
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void SetZoom(BL_FLOAT zoom);
    
    int GetLatency();
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
protected:
    friend class ClipperOverObj4;
    static void ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue);
    // Knee ratio is n [0, 1]
    static void ComputeClippingKnee(BL_FLOAT inSample, BL_FLOAT *outSample,
                                    BL_FLOAT clipValue, BL_FLOAT kneeRatio);

    void DBG_TestClippingKnee(BL_FLOAT clipValue);
    
    //
    USTClipperDisplay4 *mClipperDisplay;
    
    BL_FLOAT mClipValue;
    
    ClipperOverObj4 *mClipObjs[2];
    
    bool mIsEnabled;
    BL_FLOAT mSampleRate;
    
    int mCurrentBlockSize;
    
    DelayObj4 *mInputDelay;
};

#endif /* defined(__UST__USTClipper4__) */
