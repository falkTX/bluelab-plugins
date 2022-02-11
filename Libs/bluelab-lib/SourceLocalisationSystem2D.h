//
//  SourceLocalisationSystem2D.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__SourceLocalisationSystem2D__
#define __BL_Bat__SourceLocalisationSystem2D__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// NOTE (SourceLocalisationSystem): This looks not very correct to implement delay
// on Fft samples using standard DelayLine...
//
// SourceLocalisationSystem2: use fft phase shift
//
// SourceLocalisationSystem2D: same as previous, but 3 mics and horizontal + vertical data
class DelayLinePhaseShift;

class SourceLocalisationSystem2D
{
public:
    SourceLocalisationSystem2D(int bufferSize, BL_FLOAT sampleRate,
                               int numBands);
    
    virtual ~SourceLocalisationSystem2D();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                    const WDL_TypedBuf<WDL_FFT_COMPLEX> samplesY[2]);
    
    void GetLocalization(vector<WDL_TypedBuf<BL_FLOAT> > *localization);
    
protected:
    void Init();
    
    void ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> samplesY[2],
                             vector<vector<WDL_TypedBuf<BL_FLOAT> > > *coincidence);
    
    void ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *diffSamps,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf0,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf1,
                     int index);

    
    void FindMinima(vector<vector<WDL_TypedBuf<BL_FLOAT> > > *coincidence);
    
    void TimeIntegrate(vector<vector<WDL_TypedBuf<BL_FLOAT> > > *ioCoincidence);

    void Threshold(vector<vector<WDL_TypedBuf<BL_FLOAT> > > *ioCoincidence);

    void FreqIntegrate(const vector<vector<WDL_TypedBuf<BL_FLOAT> > > &coincidence,
                       vector<WDL_TypedBuf<BL_FLOAT> > *localization);

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
    
    typedef struct
    {
        DelayLinePhaseShift *mDelayLineX;
        DelayLinePhaseShift *mDelayLineY;
    } DelayLinePhaseShiftXY;
    
    vector<vector<vector<DelayLinePhaseShiftXY> > > mDelayLines[2];
    
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentLocalization;
    
    // For time integrate
    vector<vector<WDL_TypedBuf<BL_FLOAT> > > mPrevCoincidence;
};

#endif /* defined(__BL_Bat__SourceLocalisationSystem2D__) */
