//
//  GraphTimeAxis4.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphTimeAxis4__
#define __BL_InfrasonicViewer__GraphTimeAxis4__

class GraphControl11;

// From GraphTimeAxis
// - fixes and improvements
//
// From GraphTimeAxis2
// - formatting: hh:mm:ss
//
// From GraphTimeAxis2
// - Possible to display when there is only milliseconds
// (FIX_DISPLAY_MS)
// (FIX_ZERO_SECONDS_MILLIS)
// (SQUEEZE_LAST_CROPPED_LABEL)
class GraphTimeAxis4
{
public:
    GraphTimeAxis4();
    
    virtual ~GraphTimeAxis4();
    
    void Init(GraphControl11 *graph, int bufferSize,
              BL_FLOAT timeDuration, BL_FLOAT spacingSeconds,
              int yOffset = 0);
    
    void Reset(int bufferSize, BL_FLOAT timeDuration,
               BL_FLOAT spacingSeconds);
    
    void Update(BL_FLOAT currentTime);
    
    static BL_FLOAT ComputeTimeDuration(int numBuffers, int bufferSize,
                                      int oversampling, BL_FLOAT sampleRate);
    
protected:
    GraphControl11 *mGraph;
    
    int mBufferSize;
    
    BL_FLOAT mTimeDuration;
    
    // For example, one label every 1s, or one label every 0.5 sedons
    BL_FLOAT mSpacingSeconds;
    
    BL_FLOAT mCurrentTime;
};

#endif /* defined(__BL_InfrasonicViewer__GraphTimeAxis4__) */
