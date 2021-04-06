//
//  GraphTimeAxis6.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphTimeAxis6__
#define __BL_InfrasonicViewer__GraphTimeAxis6__

#ifdef IGRAPHICS_NANOVG

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
//
// GraphTimeAxis5: from GraphTimeAxis4
// - use new GraphControl12
//
// GraphTimeAxis6: fromGraphTimeAxis5:
// Improved for Ghost

#define MAX_NUM_LABELS 128

class GUIHelper12;
class GraphControl12;
class GraphTimeAxis6
{
public:
    GraphTimeAxis6(bool displayLines = true, bool squeezeBorderLabels = true);
    
    virtual ~GraphTimeAxis6();
    
    void Init(GraphControl12 *graph,
              GraphAxis2 *graphAxis, GUIHelper12 *guiHelper,
              int bufferSize,
              BL_FLOAT timeDuration, int maxNumLabels,
              BL_FLOAT yOffset = 0);
    
    void Reset(int bufferSize, BL_FLOAT timeDuration, int maxNumLabels);
    
    void UpdateFromTransport(BL_FLOAT transportTime);
    void UpdateFromDraw();
    void SetTransportPlaying(bool flag, BL_FLOAT transportTime = -1.0);

    // Time in seconds
    void GetMinMaxTime(BL_FLOAT *minTimeSec, BL_FLOAT *maxTimeSec);
    
    static BL_FLOAT ComputeTimeDuration(int numBuffers, int bufferSize,
                                        int oversampling, BL_FLOAT sampleRate);
    
protected:
    void Update(BL_FLOAT currentTime);
    
    //
    GraphControl12 *mGraph;
    
    GraphAxis2 *mGraphAxis;
    
    int mBufferSize;
    
    BL_FLOAT mTimeDuration;
    
    int mMaxNumLabels;
    
    BL_FLOAT mCurrentTime;
    
    //
    bool mTransportIsPlaying;

    BL_FLOAT mTransportValueSec;
    double mStartTransportTimeStamp;
    
    bool mDisplayLines;
    
    bool mSqueezeBorderLabels;

    char *mHAxisData[MAX_NUM_LABELS][2];
    bool mAxisDataAllocated;
};

#endif

#endif /* defined(__BL_InfrasonicViewer__GraphTimeAxis6__) */
