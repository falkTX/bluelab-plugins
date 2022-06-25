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
    BLScanDisplay(int numCurves, BL_GUI_FLOAT sampleRate);
    
    virtual ~BLScanDisplay();
    
    void SetGraph(GraphControl12 *graph);
    
    void Reset(BL_GUI_FLOAT sampleRate);
    
    void AddSamples(int curveNum, const WDL_TypedBuf<BL_FLOAT> &samples,
                    bool advanceSweep);
    
    void SetEnabled(bool flag);
    
    void SetDirty();
    
    void SetZoom(BL_GUI_FLOAT zoom);
    // Lazy evaluation, to avoid thread problem (crash)
    void UpdateZoom();
    
    void ResetSweepBar();

    void SetCurveStyle(int curveNum,
                       const char *description, int descrColor[4],
                       bool isSampleCurve, BL_GUI_FLOAT lineWidth,
                       bool fillFlag, int color[4]);
                       
protected:
    void AddSamplesZoom(int curveNum);
    
    long GetNumSamples();
    
    void DecimateSamplesOneLine(const WDL_TypedBuf<BL_GUI_FLOAT> &bufSamples,
                                BL_GUI_FLOAT *decimLineMin,
                                BL_GUI_FLOAT *decimLineMax);

    void UpdateSweepBar();

    void CreateCurves(int numCurves);
    void DeleteCurves();
    
    //
    GraphControl12 *mGraph;
    
    BL_GUI_FLOAT mSampleRate;

    bool mIsEnabled;
    
    // Zoom
    BL_GUI_FLOAT mZoom;
    
    long mSweepPos;

    // Curves
    GraphCurve5 *mAxisCurve;
    GraphCurve5 *mSweepBarCurve;

    bool mNeedUpdateZoom;
    
    //
    class Curve
    {
    public:
        Curve()
        {
            mCurves[0] = NULL;
            mCurves[1] = NULL;

            //mIsSampleCurve = false;
            mIsSampleCurve = true;
        }

        virtual ~Curve()
        {
            if (mCurves[0] != NULL)
                delete mCurves[0];

            if (mCurves[1] != NULL)
                delete mCurves[1];
        }

    public:
        bool mIsSampleCurve;
        
        // Up and down curves, for samples
        // 0 -> up | 1 -> down
        GraphCurve5 *mCurves[2];

        WDL_TypedBuf<BL_GUI_FLOAT> mCurrentSamples;

        WDL_TypedBuf<BL_GUI_FLOAT> mCurrentDecimValues[2];
    };

    vector<Curve> mCurves;
};

#endif
