//
//  GraphTimeAxis2.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphTimeAxis2__
#define __BL_InfrasonicViewer__GraphTimeAxis2__

#if IGRAPHICS_NANOVG

class GraphControl11;

// From GraphTimeAxis
// - fixes and improvements
//
class GraphTimeAxis2
{
public:
    GraphTimeAxis2();
    
    virtual ~GraphTimeAxis2();
    
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

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_InfrasonicViewer__GraphTimeAxis2__) */
