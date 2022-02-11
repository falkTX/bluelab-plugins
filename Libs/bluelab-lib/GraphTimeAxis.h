//
//  GraphTimeAxis.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphTimeAxis__
#define __BL_InfrasonicViewer__GraphTimeAxis__

#ifdef IGRAPHICS_NANOVG

class GraphControl11;
class GUIHelper12;
class GraphTimeAxis
{
public:
    GraphTimeAxis();
    
    virtual ~GraphTimeAxis();
    
    void Init(GraphControl11 *graph, GUIHelper12 *guiHelper,
              int bufferSize, BL_FLOAT timeDuration, int numLabels,
              int yOffset = 0);
    
    void Reset(int bufferSize, BL_FLOAT timeDuration);
    
    void Update(BL_FLOAT currentTime);
    
    static BL_FLOAT ComputeTimeDuration(int numBuffers, int bufferSize,
                                      int oversampling, BL_FLOAT sampleRate);
    
protected:
    GraphControl11 *mGraph;
    
    int mBufferSize;
    
    BL_FLOAT mTimeDuration;
    
    int mNumLabels;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_InfrasonicViewer__GraphTimeAxis__) */
