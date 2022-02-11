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
    
protected:
    void Update();
    
    //
    GraphAxis2 *mGraphAxis;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    bool mDisplayLines;
    
    Scale::Type mScale;
};

#endif /* defined(__BL_InfrasonicViewer__GraphFreqAxis2__) */
