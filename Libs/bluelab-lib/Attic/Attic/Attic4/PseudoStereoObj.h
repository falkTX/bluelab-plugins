//
//  PseudoStereoObj.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__PseudoStereoObj__
#define __UST__PseudoStereoObj__

#include "IPlug_include_in_plug_hdr.h"

//#define NUM_FILTERS 4 //1 //2

#define MIN_MODE 0
#define MAX_MODE 7

#define DEFAULT_MODE 7 //0 //3

// Gerzon method
//
// (not finished: colors the sound too much compared to the Waves plugin)
class FilterRBJNX;
class USTStereoWidener;

class PseudoStereoObj
{
public:
    PseudoStereoObj(BL_FLOAT sampleRate,
                    BL_FLOAT width = 1.0, //0.25, //0.5, //1.0,
                    int mode = DEFAULT_MODE);
    
    virtual ~PseudoStereoObj();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetWidth(BL_FLOAT width);
    
    // For debugging
    void SetCutoffFreq(BL_FLOAT freq);
    void SetMode(BL_FLOAT mode);
    
    void ProcessSample(BL_FLOAT sampIn, BL_FLOAT *sampOut0, BL_FLOAT *sampOut1);
    
    void ProcessSamples(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                        WDL_TypedBuf<BL_FLOAT> *sampsOut0,
                        WDL_TypedBuf<BL_FLOAT> *sampsOut1);
    
    void ProcessSamples(vector<WDL_TypedBuf<BL_FLOAT> > *samplesVec);
    
protected:
    void InitFilters();
    
    BL_FLOAT GetCutoffFrequency(int filterNum);

    
    //
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mWidth;
    int mMode; // Num filters ?
    
    BL_FLOAT mCutoffFreq;
    
    FilterRBJNX *mFilter0;
    FilterRBJNX *mFilter1;
    
    FilterRBJNX *mModeFilters[MAX_MODE+1][2];
    
    USTStereoWidener *mStereoWidener;
};

#endif /* defined(__UST__PseudoStereoObj__) */
