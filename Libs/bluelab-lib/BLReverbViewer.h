//
//  BLReverbViewer.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__BLReverbViewer__
#define __BL_Reverb__BLReverbViewer__

#ifdef IGRAPHICS_NANOVG

class BLReverb;
//class MultiViewer;
class MultiViewer2;

class BLReverbViewer
{
public:
    BLReverbViewer(BLReverb *reverb, MultiViewer2 *viewer,
                   BL_FLOAT durationSeconds, BL_FLOAT sampleRate);
    
    virtual ~BLReverbViewer();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetDuration(BL_FLOAT durationSeconds);
    
    void Update();
    
protected:
    BLReverb *mReverb;
    MultiViewer2 *mViewer;
    
    BL_FLOAT mDurationSeconds;
    BL_FLOAT mSampleRate;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Reverb__BLReverbViewer__) */
