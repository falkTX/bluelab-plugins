//
//  BLScanDisplay.h
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __BL__BLScanDisplay__
#define __BL__BLScanDisplay__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class GraphControl12;
class GraphCurve5;

// USTClipperDisplay2:
// - Draw filled waveforms
// - Draw clipped waveform with another color
//
// USTClipperDisplay3: use Sample Pyramid
//
// USTClipperDisplay4: use sweep update + exponential zoom
//
// BLScanDisplay: from USTClipperDisplay4
class BLScanDisplay
{
public:
    BLScanDisplay(BL_GUI_FLOAT sampleRate);
    
    virtual ~BLScanDisplay();
    
    void SetGraph(GraphControl12 *graph);
    
    void Reset(BL_GUI_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    void AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void SetEnabled(bool flag);
    
    void SetDirty();
    
    void SetZoom(BL_GUI_FLOAT zoom);

    void ResetSweepBar();
    
protected:
    void AddSamplesZoom();
    void AddSamplesZoomClip();
    
    long GetNumSamples();
    
    void DecimateSamplesOneLine(const WDL_TypedBuf<BL_GUI_FLOAT> &bufSamples,
                                BL_GUI_FLOAT *decimLineMin,
                                BL_GUI_FLOAT *decimLineMax);

    void UpdateSweepBar();

    void CreateCurves();

    //
    GraphControl12 *mGraph;
    
    BL_GUI_FLOAT mSampleRate;

    bool mIsEnabled;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentSamples;
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentClippedSamples;
    
    // Zoom
    BL_GUI_FLOAT mZoom;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesUp;
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesDown;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesUpClip;
    WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValuesDownClip;
    
    long mSweepPos;
    long mSweepPosClip;

    // Curves
    GraphCurve5 *mAxisCurve;
    GraphCurve5 *mWaveformUpCurve;
    GraphCurve5 *mWaveformDownCurve;
    GraphCurve5 *mWaveformClipUpCurve;
    GraphCurve5 *mWaveformClipDownCurve;
    GraphCurve5 *mSweepBarCurve;
};

#endif
