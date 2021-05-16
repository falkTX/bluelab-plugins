//
//  GraphFreqAxis2.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphFreqAxis2__
#define __BL_InfrasonicViewer__GraphFreqAxis2__

#include <Scale.h>

// GraphFreqAxis2: from GraphFreqAxis
// For GraphControl12
//
class GraphAxis2;
class GUIHelper12;
class GraphFreqAxis2
{
public:
    GraphFreqAxis2(bool displayLines = true,
                   Scale::Type scale = Scale::MEL);
    
    virtual ~GraphFreqAxis2();

    void Init(GraphAxis2 *graphAxis, GUIHelper12 *guiHelper,
              bool horizontal,
              int bufferSize, BL_FLOAT sampleRate,
              int graphWidth);
    
    // Bounds on screen, [0, 1] by default
    void SetBounds(BL_FLOAT bounds[2]);
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    void SetMaxFreq(BL_FLOAT maxFreq);

    // To avoid data race in some cases
    // Use this mechanism to postpone reset in the GUI thread, e.g in OnIdle()
    // If called from plug OnReset(), this mechanism is not necessary
    void SetResetParams(int bufferSize, BL_FLOAT sampleRate);
    bool MustReset();
    void Reset();

    void SetScale(Scale::Type scale);
    
protected:
    void Update();
    
    void UpdateAxis(int numAxisData,
                    const BL_FLOAT freqs[],
                    const char *labels[],
                    BL_FLOAT minHzValue, BL_FLOAT maxHzValue);
    
    //
    GraphAxis2 *mGraphAxis;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    bool mDisplayLines;
    
    Scale::Type mScale;

    BL_FLOAT mMaxFreq;

    bool mMustReset;
};

#endif /* defined(__BL_InfrasonicViewer__GraphFreqAxis2__) */
