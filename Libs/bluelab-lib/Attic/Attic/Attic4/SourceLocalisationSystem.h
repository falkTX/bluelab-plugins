//
//  SourceLocalisationSystem.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__SourceLocalisationSystem__
#define __BL_Bat__SourceLocalisationSystem__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// NOTE: This looks not very correct to implement delay
// on Fft samples using standard DelayLine...

class DualDelayLine;

class SourceLocalisationSystem
{
public:
    SourceLocalisationSystem(int bufferSize, BL_FLOAT sampleRate,
                             int numBands);
    
    virtual ~SourceLocalisationSystem();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2]);
    
    void GetLocalization(WDL_TypedBuf<BL_FLOAT> *localization);
    
protected:
    void Init();
    
    void ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                             vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *diffSamps,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf0,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf1,
                     int index);

    
    void FindMinima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    void FindMaxima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void TimeIntegrate(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence);

    void Threshold(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence);

    void FreqIntegrate(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                       WDL_TypedBuf<BL_FLOAT> *localization);

    void DeleteDelayLines();
    
    void DBG_DumpCoincidence(const char *fileName,
                             const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                             BL_FLOAT colorCoeff);
    
    void DBG_DumpCoincidenceLine(const char *fileName,
                                 int index,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence);

    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mNumBands;
    
    vector<vector<DualDelayLine *> > mDelayLines[2];
    
    WDL_TypedBuf<BL_FLOAT> mCurrentLocalization;
    
    // For time integrate
    vector<WDL_TypedBuf<BL_FLOAT> > mPrevCoincidence;
};

#endif /* defined(__BL_Bat__SourceLocalisationSystem__) */
