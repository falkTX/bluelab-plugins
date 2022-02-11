//
//  Oversampler5.h
//  UST
//
//  Created by applematuer on 8/12/20.
//
//

#ifndef __UST__Oversampler5__
#define __UST__Oversampler5__


// From Oversampler4
// - use r8brain-free-src-version-4.6
// (try to avoid filtering high frequencies)
//
namespace r8b {
class CDSPResampler24;
}

class Oversampler5
{
public:
    // Up or downsample
    Oversampler5(int oversampling, bool upSample);
    
    virtual ~Oversampler5();
    
    void Reset(double sampleRate, int blockSize);
    
    int GetLatency();
    
    void Resample(WDL_TypedBuf<double> *ioBuf);
    
    int GetOversampling();
    
    double *GetOutEmptyBuffer();
    
protected:
    int mOversampling;
    
    WDL_TypedBuf<double> mResultBuf;
    WDL_TypedBuf<double> mOutEmptyBuf;
    
    double mSampleRate;
    
    r8b::CDSPResampler24 *mResampler;
    
    // Tell if we upsample or downsample
    bool mUpSample;
};

#endif /* defined(__UST__Oversampler5__) */
