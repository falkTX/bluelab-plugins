//
//  GraphTimeAxis5.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphTimeAxis5__
#define __BL_InfrasonicViewer__GraphTimeAxis5__

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
class GUIHelper12;
class GraphControl12;
class GraphTimeAxis5
{
public:
    GraphTimeAxis5(bool displayLines = true, bool roundToIntLabels = false);
    
    virtual ~GraphTimeAxis5();
    
    void Init(GraphControl12 *graph,
              GraphAxis2 *graphAxis, GUIHelper12 *guiHelper,
              int bufferSize,
              BL_FLOAT timeDuration, BL_FLOAT spacingSeconds,
              BL_FLOAT yOffset = 0);
    
    void Reset(int bufferSize, BL_FLOAT timeDuration,
               BL_FLOAT spacingSeconds);
    
    void UpdateFromTransport(BL_FLOAT currentTime);
    void Update();
    void SetTransportPlaying(bool flag);
    
    // Must be called from ProcessBlock(), to indicate
    // that the plugin is not bypassed
    void SetMustUpdate();
    
    static BL_FLOAT ComputeTimeDuration(int numBuffers, int bufferSize,
                                        int oversampling, BL_FLOAT sampleRate);
    
protected:
    void Update(BL_FLOAT currentTime);
    
    //
    GraphAxis2 *mGraphAxis;
    
    int mBufferSize;
    
    BL_FLOAT mTimeDuration;
    
    // For example, one label every 1s, or one label every 0.5 seconss
    BL_FLOAT mSpacingSeconds;
    
    BL_FLOAT mCurrentTime;
    
    //
    bool mTransportIsPlaying;
    BL_FLOAT mCurrentTimeTransport;
    long int mTransportTimeStamp;
    
    bool mDisplayLines;
    
    bool mMustUpdate;
    
    bool mRoundToIntLabels;
};

#endif

#endif /* defined(__BL_InfrasonicViewer__GraphTimeAxis5__) */
