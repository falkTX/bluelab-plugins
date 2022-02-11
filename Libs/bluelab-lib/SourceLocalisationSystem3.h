//
//  SourceLocalisationSystem3.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__SourceLocalisationSystem3__
#define __BL_Bat__SourceLocalisationSystem3__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// See article: "Localization of multiple sound sources with two microphones", Chen Liu,b) Bruce C. Wheeler...
// https://www.researchgate.net/publication/12274192_Localization_of_multiple_sound_sources_with_two_microphones
//

// NOTE (SourceLocalisationSystem): This looks not very correct to implement delay
// on Fft samples using standard DelayLine...
//
// SourceLocalisationSystem2: use fft phase shift
//
class DelayLinePhaseShift2;

class SourceLocalisationSystem3
{
public:
    enum Mode
    {
        PHASE_DIFF,
        AMP_DIFF
    };
    
    SourceLocalisationSystem3(Mode mode, int bufferSize, BL_FLOAT sampleRate,
                              int numBands);
    
    virtual ~SourceLocalisationSystem3();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2]);
    
    void GetLocalization(WDL_TypedBuf<BL_FLOAT> *localization);
    
    int GetNumBands();
    
    void SetTimeSmooth(BL_FLOAT smoothCoeff);
    
    void SetIntermicDistanceCoeff(BL_FLOAT intermicCoeff);
    
    //
    void DBG_GetCoincidence(int *width, int *height, WDL_TypedBuf<BL_FLOAT> *data);
    
protected:
    void Init();
    
    // Standard algorithm
    void ComputeCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                             vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    // Modified algorithm, use amps instaed of phases
    // (to be used with coincident microphones)
    void ComputeAmpCoincidences(const WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2],
                                vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *diffSamps,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf0,
                     const vector<vector<WDL_FFT_COMPLEX> > &buf1,
                     int bandIndex);

    
    void FindMinima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    void FindMinima2(WDL_TypedBuf<BL_FLOAT> *coincidence);
    void FindMinima3(WDL_TypedBuf<BL_FLOAT> *coincidence);
    void NormalizeMinima(WDL_TypedBuf<BL_FLOAT> *coincidence);
    //void FindMaxima(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    // Really find several minima (and set them to 1)
    void FindMinima4(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    // Set the minima to the band amplitude value
    void FindMinima5(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void TimeIntegrate(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence);
    
    void Threshold(vector<WDL_TypedBuf<BL_FLOAT> > *ioCoincidence);

    void FreqIntegrate(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                       WDL_TypedBuf<BL_FLOAT> *localization);

    // Simple custom method
    void FreqIntegrateStencil(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                              WDL_TypedBuf<BL_FLOAT> *localization);
    
    // Cross correlation method
    void FreqIntegrateStencil2(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                               WDL_TypedBuf<BL_FLOAT> *localization);
    
    // Use two steps method described in https://pdfs.semanticscholar.org/665b/802b0236201adff4df707a26080cf808873e.pdf
    // "A SIMPLE BINARY IMAGE SIMILARITY MATCHING METHOD BASED ON EXACT PIXEL MATCHING" - Mikiyas Teshome.
    void FreqIntegrateStencil3(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                               WDL_TypedBuf<BL_FLOAT> *localization);
    
    // Calmes method
    void FreqIntegrateStencilCalmes(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                    WDL_TypedBuf<BL_FLOAT> *localization);
    
    void FreqIntegrateStencilCalmes2(const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                                     WDL_TypedBuf<BL_FLOAT> *localization);
    
    void InvertValues(WDL_TypedBuf<BL_FLOAT> *localization);

    
    void DeleteDelayLines();
    
    void GenerateMask(vector<WDL_TypedBuf<BL_FLOAT> > *mask,
                      int numBins, int numBands,int bandIndex);
    
    void MaskDilation(vector<WDL_TypedBuf<BL_FLOAT> > *mask);
    
    void MaskMinMax(vector<WDL_TypedBuf<BL_FLOAT> > *mask);
    
    void GenerateStencilMasks();
    
    void BuildMinMaxMap(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence);
    
    void LowPassFilter(WDL_TypedBuf<WDL_FFT_COMPLEX> *samples);

    
    //
    void DBG_DumpCoincidence(const char *fileName,
                             const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence,
                             BL_FLOAT colorCoeff);
    
    void DBG_DumpCoincidenceLine(const char *fileName,
                                 int index,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &coincidence);

    
    void DBG_DumpFftSamples(const char *fileName,
                            const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples);
    
    void DBG_DumpFftSamples(const char *fileName,
                            const vector<WDL_FFT_COMPLEX> &fftSamples);

    void DBG_DumpCoincidenceSum(const char *fileName,
                                vector<WDL_TypedBuf<BL_FLOAT> > &coincidence);
    
    void DBG_DumpCoincidenceSum2(const char *fileName,
                                 vector<WDL_TypedBuf<BL_FLOAT> > &coincidence);

    //
    void DBG_DisplayLocalizationcurve(vector<WDL_TypedBuf<BL_FLOAT> > *coincidence,
                                      const  WDL_TypedBuf<BL_FLOAT> &localization);

    
    //
    Mode mMode;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mNumBands;
    
    BL_FLOAT mIntermicDistanceCoeff;
    
    BL_FLOAT mTimeSmooth;
    
    //
    vector<vector<DelayLinePhaseShift2 *> > mDelayLines[2];
    
    WDL_TypedBuf<BL_FLOAT> mCurrentLocalization;
    
    // For time integrate
    vector<WDL_TypedBuf<BL_FLOAT> > mPrevCoincidence;
    
    // Keep it as member, to optimize
    vector<vector<WDL_FFT_COMPLEX> > mDelayedSamples[2];
    
    // Keep it as member, to optimize
    WDL_TypedBuf<WDL_FFT_COMPLEX> mResultDiff;
    
    // Keep it as member, to optimize
    vector<WDL_TypedBuf<BL_FLOAT> > mCoincidence;
    
    // Precompute and keep all masks
    vector<vector<WDL_TypedBuf<BL_FLOAT> > > mStencilMasks;
};

#endif /* defined(__BL_Bat__SourceLocalisationSystem3__) */
