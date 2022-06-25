/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
class USTClipperDisplay4
{
public:
    USTClipperDisplay4(BL_GUI_FLOAT sampleRate);
    
    virtual ~USTClipperDisplay4();
    
    void SetGraph(GraphControl12 *graph);
    
    void Reset(BL_GUI_FLOAT sampleRate);
    
    void SetClipValue(BL_GUI_FLOAT clipValue);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void SetEnabled(bool flag);
    
    void SetDirty();
    
    void SetZoom(BL_GUI_FLOAT zoom);
    
protected:
    void AddSamplesZoom();
    void AddSamplesZoomClip();
    void SetClipValueZoom();
    
    long GetNumSamples();
    
    void DecimateSamplesOneLine(const WDL_TypedBuf<BL_GUI_FLOAT> &bufSamples,
                                BL_GUI_FLOAT *decimLineMin,
                                BL_GUI_FLOAT *decimLineMax);

    void UpdateSweepBar();

    void CreateCurves();

    //
    GraphControl12 *mGraph;
    
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

    // Curves
    GraphCurve5 *mAxisCurve;
    GraphCurve5 *mWaveformUpCurve;
    GraphCurve5 *mWaveformDownCurve;
    GraphCurve5 *mWaveformClipUpCurve;
    GraphCurve5 *mWaveformClipDownCurve;
    GraphCurve5 *mClipLoCurve;
    GraphCurve5 *mClipHiCurve;
    GraphCurve5 *mSweepBarCurve;
};

#endif /* defined(__UST__USTClipperDisplay2__) */
