//
//  StereoWidthProcess.h
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_PitchShift__StereoWidthProcess__
#define __BL_PitchShift__StereoWidthProcess__

#include "FftProcessObj14.h"

// Fill missing values for source position
#define FILL_MISSING_VALUES 0

// Temporal smoothing of the distances
#define TEST_TEMPORAL_SMOOTHING 0
#define TEMPORAL_DIST_SMOOTH_COEFF 0.5 //0.9

#if TEST_TEMPORAL_SMOOTHING
#include <SmoothAvgHistogram.h>
#endif

// Spatial smoothing of the distances
// (smooth from near frequencies)
#define TEST_SPATIAL_SMOOTHING 0
#define SPATIAL_SMOOTHING_WINDOW_SIZE 128

#if TEST_SPATIAL_SMOOTHING
#include <CMA2Smoother.h>
#endif

#define DEBUG_FREQS_DISPLAY 0
#define DEBUG_SPAT_DISPLAY 1

class StereoWidthProcess : public MultichannelProcess
{
public:
    StereoWidthProcess(int bufferSize,
                       double overlapping, double oversampling,
                       double sampleRate);
    
    virtual ~StereoWidthProcess();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, double sampleRate);
    
    void SetWidthChange(double widthChange);
    
    void SetFakeStereo(bool flag);
    
    void ProcessInputSamples(vector<WDL_TypedBuf<double> * > *ioSamples);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples);
    
    // Get the result
    void GetWidthValues(WDL_TypedBuf<double> *xValues,
                        WDL_TypedBuf<double> *yValues);
    
protected:
    // We have to change the magns too
    // ... because if we decrease at the maximum, the two magns channels
    // must be similar
    // (otherwise it vibrates !)
    void ApplyWidthChange(WDL_TypedBuf<double> *ioPhasesL,
                          WDL_TypedBuf<double> *ioPhasesR,
                          WDL_TypedBuf<double> *ioMagnsL,
                          WDL_TypedBuf<double> *ioMagnsR);
    
    void Stereoize(WDL_TypedBuf<double> *ioPhasesL,
                   WDL_TypedBuf<double> *ioPhasesR);
    
    void GenerateRandomCoeffs(int size);

    // Return true is the distance an agle have been actually computed
    bool MagnPhasesToSourcePos(double *sourceR, double *sourceTheta,
                               double magnsL, double magnsR,
                               double phasesL, double phasesR,
                               double freq);
    
    bool SourcePosToMagnPhases(double sourceR, double sourceTheta,
                               double micsDistance,
                               double *ioMagnL, double *ioMagnR,
                               double *ioPhaseL, double *ioPhaseR,
                               double freq);
    
    double ComputeFactor(double normVal, double maxVal);

    double ComputeAvgPhaseDiff(const WDL_TypedBuf<double> &monoMagns,
                               const WDL_TypedBuf<double> &diffPhases);
                             

                                            
    int mBufferSize;
    double mOverlapping;
    double mOversampling;
    double mSampleRate;
    
    double mWidthChange;
    
    bool mFakeStereo;
    
    WDL_TypedBuf<double> mWidthValuesX;
    WDL_TypedBuf<double> mWidthValuesY;
    
    // unused ?
    WDL_TypedBuf<double> mCurrentSamples;
    
    WDL_TypedBuf<double> mRandomCoeffs;
    
#if TEST_TEMPORAL_SMOOTHING
    SmoothAvgHistogram *mSourceRsHisto;
    SmoothAvgHistogram *mSourceThetasHisto;
#endif
};

#endif /* defined(__BL_PitchShift__StereoPhasesProcess__) */
