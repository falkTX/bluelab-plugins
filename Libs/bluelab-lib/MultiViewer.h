//
//  MultiViewer.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__MultiViewer__
#define __BL_Reverb__MultiViewer__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class GUIHelper11;
class GraphControl11;
class SamplesToSpectrogram;
class GraphTimeAxis4;

using namespace iplug;
using namespace iplug::igraphics;

class MultiViewer
{
public:
    MultiViewer(Plugin *plug, IGraphics *pGraphics, GUIHelper11 *guiHelper,
                int graphId, const char *graphFn,
                int graphFrames, int graphX, int graphY,
                int graphParam, const char *graphParamName,
                int graphShadowsId, const char *graphShwdowsFn,
                BL_FLOAT sampleRate);
    
    virtual ~MultiViewer();
    
    void Reset(BL_FLOAT sampleRate);
    
    GraphControl11 *GetGraph();
    
    void SetTime(BL_FLOAT durationSeconds, BL_FLOAT timeOrigin = 0.0);
    
    void SetSamples(const WDL_TypedBuf<BL_FLOAT> &data);
    
protected:
    void UpdateFrequencyScale();
    
    //
    BL_FLOAT mSampleRate;
    
    GraphControl11 *mGraph;
    
    SamplesToSpectrogram *mSamplesToSpectro;
    
    GraphTimeAxis4 *mTimeAxis;
};

#endif /* defined(__BL_Reverb__MultiViewer__) */
