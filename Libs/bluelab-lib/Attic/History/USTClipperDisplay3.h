//
//  USTClipperDisplay3.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipperDisplay3__
#define __UST__USTClipperDisplay3__

#include "IPlug_include_in_plug_hdr.h"

class GraphControl11;
class SamplesPyramid;
class SamplesPyramid2;

// USTClipperDisplay2:
// - Draw filled waveforms
// - Draw clipped waveform with another color
//
// USTClipperDisplay3: use Sample Pyramid
//
class USTClipperDisplay3
{
public:
    USTClipperDisplay3(GraphControl11 *graph, BL_FLOAT sampleRate);
    
    virtual ~USTClipperDisplay3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void SetDirty();
    
    void SetZoom(BL_FLOAT zoom);
    
protected:
    void AddSamplesZoom();
    void AddSamplesZoomClip();
    void SetClipValueZoom();
    
    long GetPyramidSize();
    
    GraphControl11 *mGraph;
    
    BL_FLOAT mSampleRate;
    
    SamplesPyramid2 *mPyramid;
    SamplesPyramid2 *mPyramidClip;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentSamples;
    WDL_TypedBuf<BL_FLOAT> mCurrentClippedSamples;
    
    // Zoom
    BL_FLOAT mZoom;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentDecimValuesUp;
    WDL_TypedBuf<BL_FLOAT> mCurrentDecimValuesDown;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentDecimValuesUpClip;
    WDL_TypedBuf<BL_FLOAT> mCurrentDecimValuesDownClip;

    BL_FLOAT mCurrentClipValue;
};

#endif /* defined(__UST__USTClipperDisplay2__) */
