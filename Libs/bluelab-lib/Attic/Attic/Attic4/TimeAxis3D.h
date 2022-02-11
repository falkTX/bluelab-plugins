//
//  TimeAxis3D.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__TimeAxis3D__
#define __BL_InfrasonicViewer__TimeAxis3D__

class Axis3D;

// From GraphTimeAxis
// - fixes and improvements
//
// From GraphTimeAxis2
// - formatting: hh:mm:ss
//
// TimeAxis3D: from GraphTimeAxis3
//
class TimeAxis3D
{
public:
    TimeAxis3D(Axis3D *axis);
    
    virtual ~TimeAxis3D();
    
    void Init(int bufferSize,
              BL_FLOAT timeDuration, BL_FLOAT spacingSeconds,
              int yOffset = 0);
    
    void Reset(int bufferSize, BL_FLOAT timeDuration,
               BL_FLOAT spacingSeconds);
    
    void Update(BL_FLOAT currentTime);
    
    static BL_FLOAT ComputeTimeDuration(int numBuffers, int bufferSize,
                                      int oversampling, BL_FLOAT sampleRate);
    
protected:
    Axis3D *mAxis;
    
    int mBufferSize;
    
    BL_FLOAT mTimeDuration;
    
    // For example, one label every 1s, or one label every 0.5 sedons
    BL_FLOAT mSpacingSeconds;
    
    BL_FLOAT mCurrentTime;
};

#endif /* defined(__BL_InfrasonicViewer__GraphTimeAxis3__) */
