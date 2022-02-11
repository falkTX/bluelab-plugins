//
//  InstantCompressor.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__InstantCompressor__
#define __UST__InstantCompressor__

// See: https://www.musicdsp.org/en/latest/Effects/169-compressor.html

// Apply compression, instantly, on already computed rms avg
class InstantCompressor
{
public:
    InstantCompressor(double sampleRate);
    
    virtual ~InstantCompressor();
    
    void  Reset(double sampleRate);
    
    // Set all parameters at a time
    void SetParameters(double threshold, double slope,
                       double  tatt, double  trel);
    
    // set parameters one by one
    void SetThreshold(double threshold);
    void SetSlope(double slope);
    void SetAttack(double tatt);
    void SetRelease(double trel);
    
    // Niko
    void SetKnee(double kneePercent);
    
    void Process(double *rmsAmp);
    
    double GetGain();
    
protected:
    double mSampleRate;
    
    // threshold (percents)
    double mThreshold;

    // slope angle (percents)
    double mSlope;
    
    // attack time  (ms)
    double mAttack;
    
    // release time (ms)
    double mRelease;
    
    // envelope
    double  mEnv;
    
    // Current compression gain
    double mGain;
    
    // Niko
    double mKneePercent;
};

#endif /* defined(__UST__InstantCompressor__) */
