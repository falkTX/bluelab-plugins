//
//  SamplesPyramid3.h
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#ifndef __BL_Ghost__SamplesPyramid3__
#define __BL_Ghost__SamplesPyramid3__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "../../WDL/fastqueue.h"
#include "IPlug_include_in_plug_hdr.h"

// SamplesPyramid3: for Ghost
// Fix alignement

// WORKAROUND: limit the maximum pyramid depth
//
// (Tested on long file, and long capture: perfs still ok)
//
// If the level is too high, the waveform gets messy
// (we don't detect well minima and maxima)
#define DEFAULT_MAX_PYRAMID_LEVEL 8

// Use float to save memory
#define SP_FLOAT float

// Make "MipMaps" with samples
// (to optimize when displaying waveform of long sound)
class SamplesPyramid3
{
public:
    SamplesPyramid3(int maxLevel = DEFAULT_MAX_PYRAMID_LEVEL);
    
    virtual ~SamplesPyramid3();
    
    void Reset();
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> &samples);

    // Return the number of pushed samples
    long PushValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PopValues(long numSamples);
    
    void ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void GetValues(BL_FLOAT start, BL_FLOAT end, long numValues,
                   WDL_TypedBuf<BL_FLOAT> *samples);
    
protected:
    void ResetTmpBuffers();

    void UpsampleResult(int numValues,
                        WDL_TypedBuf<SP_FLOAT> *result,
                        const WDL_TypedBuf<SP_FLOAT> &buffer,
                        SP_FLOAT leftT, SP_FLOAT rightT);

    void DBG_DisplayMemory();
        
    //
    
    //vector<WDL_TypedBuf<BL_FLOAT> > mSamplesPyramid;
    //vector<WDL_TypedFastQueue<BL_FLOAT> > mSamplesPyramid;
    vector<WDL_TypedFastQueue<SP_FLOAT> > mSamplesPyramid;
    
    // Keep a push buffer, to be able to push
    // by bloocks of power of two (otherwise,
    // there are artefacts on the second half).
    //WDL_TypedBuf<BL_FLOAT> mPushBuf;
    //WDL_TypedFastQueue<BL_FLOAT> mPushBuf;
    WDL_TypedFastQueue<SP_FLOAT> mPushBuf;
    
    long mRemainToPop;

    int mMaxPyramidLevel;
    
private:
    // Tmp buffers
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf1;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf2;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf3;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf4;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf5;
    WDL_TypedBuf<SP_FLOAT> mTmpBuf6;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf7;
    vector<WDL_TypedBuf<SP_FLOAT> > mTmpBuf8;
    WDL_TypedBuf<SP_FLOAT> mTmpBuf9;
    WDL_TypedBuf<SP_FLOAT> mTmpBuf10;
};

#endif /* defined(__BL_Ghost__SamplesPyramid3__) */
