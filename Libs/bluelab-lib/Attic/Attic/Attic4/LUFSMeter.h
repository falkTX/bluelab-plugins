//
//  LUFSMeter.h
//  UST
//
//  Created by applematuer on 8/14/19.
//
//

#ifndef __UST__LUFSMeter__
#define __UST__LUFSMeter__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include <ebur128.h>

#include <BLTypes.h>

// See: https://github.com/jiixyj/libebur128

// NOTE: this is non sense to calculate loudness for each channels
// when stereo input. We must compute loudness globally for all channels.
#define USE_SINGLE_CHANNEL 0

#define ENABLE_SHORT_TERM 1

#define NUM_DOWNSAMPLERS 2

class Oversampler4;
class LUFSMeter
{
public:
    LUFSMeter(int numChannels, BL_FLOAT sampleRate);
    
    virtual ~LUFSMeter();
    
    void Reset(BL_FLOAT sampleRate);
    
#if !USE_SINGLE_CHANNEL
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples);
#else
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
#endif
    
    // Very small window time (ms)
    void GetLoudnessMomentary(BL_FLOAT *ioLoudness);
    
#if ENABLE_SHORT_TERM
    // Small window size (some seconds)
    void GetLoudnessShortTerm(BL_FLOAT *ioLoudness);
#endif
    
protected:
    ebur128_state *mState;
    
    int mNumChannels;
    
    // Optimization: Downsample before computing loudness
    Oversampler4 *mDownsamplers[NUM_DOWNSAMPLERS];
};

#endif /* defined(__UST__LUFSMeter__) */
