//
//  BLReverbViewer.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__BLReverbViewer__
#define __BL_Reverb__BLReverbViewer__

class BLReverb;
class MultiViewer;

class BLReverbViewer
{
public:
    BLReverbViewer(BLReverb *reverb, MultiViewer *viewer,
                   BL_FLOAT durationSeconds, BL_FLOAT sampleRate);
    
    virtual ~BLReverbViewer();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetDuration(BL_FLOAT durationSeconds);
    
    void Update();
    
protected:
    BLReverb *mReverb;
    MultiViewer *mViewer;
    
    BL_FLOAT mDurationSeconds;
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_Reverb__BLReverbViewer__) */
