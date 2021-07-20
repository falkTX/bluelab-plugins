#ifndef CHROMAGRAM_OBJ_H
#define CHROMAGRAM_OBJ_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#define USE_FREQ_OBJ 1

class HistoMaskLine2;

#if USE_FREQ_OBJ
class FreqAdjustObj3;
#endif

class ChromagramObj
{
 public:
    ChromagramObj(int bufferSize, int oversampling,
                  int freqRes, BL_FLOAT sampleRate);
    virtual ~ChromagramObj();

    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);

    void SetATune(BL_FLOAT aTune);
    
    void SetSharpness(BL_FLOAT sharpness);

    void MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &phases,
                           WDL_TypedBuf<BL_FLOAT> *chromaLine,
                           HistoMaskLine2 *maskLine = NULL);

    // NOTE: could be static, but needs mATune
    BL_FLOAT ChromaToFreq(BL_FLOAT chromaVal, BL_FLOAT minFreq) const;
        
 protected:
    void MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                           WDL_TypedBuf<BL_FLOAT> *chromaLine,
                           HistoMaskLine2 *maskLine = NULL);
    
#if USE_FREQ_OBJ
    void MagnsToChromaLineFreqs(const WDL_TypedBuf<BL_FLOAT> &magns,
                                const WDL_TypedBuf<BL_FLOAT> &realFreqs,
                                WDL_TypedBuf<BL_FLOAT> *chromaLine,
                                HistoMaskLine2 *maskLine = NULL);
#endif

    BL_FLOAT ComputeC0Freq() const;

    //
    BL_FLOAT mSampleRate;
    int mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mATune;
    BL_FLOAT mSharpness;

#if USE_FREQ_OBJ
    FreqAdjustObj3 *mFreqObj;
#endif

 private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
};

#endif
