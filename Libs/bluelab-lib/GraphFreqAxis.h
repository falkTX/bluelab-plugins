//
//  GraphFreqAxis.h
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifndef __BL_InfrasonicViewer__GraphFreqAxis__
#define __BL_InfrasonicViewer__GraphFreqAxis__

#ifdef IGRAPHICS_NANOVG

class GraphControl11;
class GraphFreqAxis
{
public:
    GraphFreqAxis();
    
    virtual ~GraphFreqAxis();
    
    void Init(GraphControl11 *graph,
              int bufferSize, BL_FLOAT sampleRate);
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
protected:
    void Update();
    
    //
    GraphControl11 *mGraph;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_InfrasonicViewer__GraphFreqAxis__) */
