//
//  USTClipperDisplay4.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipperDisplay4__
#define __UST__USTClipperDisplay4__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class GraphControl11;

// USTClipperDisplay2:
// - Draw filled waveforms
// - Draw clipped waveform with another color
//
// USTClipperDisplay3: use Sample Pyramid
//
// USTClipperDisplay4: use sweep update + exponential zoom
//
class USTClipperDisplay4
{
public:
    USTClipperDisplay4(GraphControl11 *graph, BL_GUI_FLOAT sampleRate);
    
    virtual ~USTClipperDisplay4();
    
    void Reset(BL_GUI_FLOAT sampleRate);
    
    void SetClipValue(BL_GUI_FLOAT clipValue);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void SetDirty();
    
    void SetZoom(BL_GUI_FLOAT zoom);
    
protected:
    void AddSamplesZoom();
    void AddSamplesZoomClip();
    void SetClipValueZoom();
    
    long GetNumSamples();
    
    void DecimateSamplesOneLine(const WDL_TypedBuf<BL_GUI_FLOAT> &bufSamples,
                                BL_GUI_FLOAT *decimLineMin, BL_GUI_FLOAT *decimLineMax);

    void UpdateSweepBar();

    
    GraphControl11 *mGraph;
    
    BL_GUI_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentSamples;
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentClippedSamples;
    
    // Zoom
    BL_GUI_FLOAT mZoom;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesUp;
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesDown;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesUpClip;
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesDownClip;
    
    BL_GUI_FLOAT mCurrentClipValue;
    
    long mSweepPos;
    long mSweepPosClip;
};

#endif /* defined(__UST__USTClipperDisplay2__) */
