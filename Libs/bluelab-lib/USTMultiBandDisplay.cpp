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
//  MultiBandDisplay.cpp
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

#include <FilterFreqResp.h>
#include <FilterLR4Crossover.h>

#include <GUIHelper11.h>

#include <BLUtils.h>

#include "USTMultiBandDisplay.h"

#if !SPLITTER_N_BANDS
#include <CrossoverSplitter5Bands.h>
#else
#include <CrossoverSplitterNBands4.h>
#endif

//#define NUM_POINTS 256
//#define DECIM_FACTOR 1.0

#define NUM_POINTS_GRAPH 256
#define NUM_POINTS_DATA 1024
#define DECIM_FACTOR 0.25

#define Y_SCALE_COEFF 0.25
//#define Y_SCALE_COEFF 0.5 // DEBUG

#define CURVE_FILL_ALPHA 0.2

#define HIGH_RES_LOG_CURVES 1

#define MIN_DB2 -200.0

#define ORANGE_COLOR_SCHEME 0
#define BLUE_COLOR_SCHEME 1

#if BLUE_COLOR_SCHEME
#define CURVE_FILL_ALPHA2 0.8
#endif

#define HIDE_FREQ_AXIS 1

// Protools crash at plug initialization
//#define FIX_PROTOOLS_CRASH 1
// #bl-iplug2: Set to 0 since IPlug2
#define FIX_PROTOOLS_CRASH 0

#if !SPLITTER_N_BANDS
USTMultiBandDisplay::USTMultiBandDisplay(CrossoverSplitter5Bands *splitter,
                                         BL_FLOAT sampleRate)
#else
USTMultiBandDisplay::USTMultiBandDisplay(CrossoverSplitterNBands4 *splitter,
                                         BL_FLOAT sampleRate)
#endif
{
    mGraph = NULL;
    
    mSplitter = splitter;
    
    mFilterResp = new FilterFreqResp();
    
    mSampleRate = sampleRate;

    for (int i = 0; i < MB_NUM_CURVES; i++)
      mCurves[i] = NULL;
    CreateCurves();
}

USTMultiBandDisplay::~USTMultiBandDisplay()
{
    delete mFilterResp;

    for (int i = 0; i < MB_NUM_CURVES; i++)
      delete mCurves[i];
}

void
USTMultiBandDisplay::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
#if !HIDE_FREQ_AXIS
    GUIHelper10::UpdateFrequencyAxis(mGraph, NUM_POINTS_DATA, sampleRate, HIGH_RES_LOG_CURVES);
#endif
}

void
USTMultiBandDisplay::SetGraph(GraphControl12 *graph)
{
    if (graph == NULL)
        return;
    
    mGraph = graph;
    
    //int numCurves = mSplitter->GetNumFilters();
    
    mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
    
    //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
    mGraph->SetClearColor(0, 0, 0, 255);
    
#if !HIDE_FREQ_AXIS
    GUIHelper10::UpdateFrequencyAxis(mGraph, NUM_POINTS_DATA, mSampleRate, HIGH_RES_LOG_CURVES);
#endif
    
#if 0
    mGraph->SetYScaleFactor(1.0);
    
    BL_FLOAT sampleRate = GetSampleRate();
    GUIHelper9::UpdateFrequencyAxis(mGraph, mBufferSize, sampleRate, HIGH_RES_LOG_CURVES);
    
    // V axis dummy curve (for the scale)
    //mGraph->SetCurveYScale(GRAPH_VAXIS_CURVE, false, MIN_DB, MAX_DB);
    
    // When no border, with is better readable
#define OFFSET_X 4
    
    // Add vAxis at the end, to be sure to have the correct dB scale (depends on curve).
    mGraph->AddVAxis(SIG_AXIS_DATA, NUM_SIG_AXIS_DATA,
                     axisColor, axisLabelColor, 0.0, OFFSET_X);
#endif
    
    //for (int i = 0; i < 8; i++)
    for (int i = 0; i < MB_NUM_CURVES; i++)
    {
      mCurves[i]->SetYScale(Scale::LINEAR, 0.0, 0.002*Y_SCALE_COEFF);
        
        // Signal
        //mGraph->SetCurveDescription(i, "signal", descrColor);
        
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        mCurves[i]->SetColor(64, 64, 255);
#endif
        
#if ORANGE_COLOR_SCHEME
        mCurves[i]->SetColor(232, 110, 36);
#endif

#if BLUE_COLOR_SCHEME
        mCurves[i]->SetColor(170, 202, 209); //205, 243, 252);
        mCurves[i]->SetFillColor(113, 130, 182); //78, 90, 126);
#endif

        mCurves[i]->SetAlpha(1.0);
        mCurves[i]->SetLineWidth(2.0);
        mCurves[i]->SetFill(true);
        mCurves[i]->SetFillAlpha(CURVE_FILL_ALPHA);
        //mGraph->SetCurveYScale(i, true, MIN_DB, MAX_DB);
        
#if BLUE_COLOR_SCHEME
        if (i < 5)
            mCurves[i]->SetFillAlpha(CURVE_FILL_ALPHA2);
#endif
    }
    
#if !FIX_PROTOOLS_CRASH
    Update(-1);
#endif
    
    for (int i = 0; i < MB_NUM_CURVES; i++)
        mGraph->AddCurve(mCurves[i]);
}

void
USTMultiBandDisplay::Update(int filterNum)
{
    if (mGraph == NULL)
        return;
    
#if !SPLITTER_N_BANDS
    // Set points
    int numFilters = mSplitter->GetNumFilters();
    for (int i = 0; i < numFilters; i++)
    {
        const FILTER_CLASS *filter = mSplitter->GetFilter(i);
     
        // Make a copy to avoid injecting data to the original filters
        FILTER_CLASS *copyFilter = new FILTER_CLASS();
        BL_FLOAT sampleRate = filter->GetSampleRate();
        copyFilter->Reset(sampleRate);
        BL_FLOAT cutoffFreq = filter->GetCutoffFreq();
        copyFilter->SetCutoffFreq(cutoffFreq);
        
        WDL_TypedBuf<BL_FLOAT> freqRespLo;
        WDL_TypedBuf<BL_FLOAT> freqRespHi;
        mFilterResp->GetFreqResp(&freqRespLo, &freqRespHi,
                                 NUM_POINTS_DATA, copyFilter);
       
#if 0
        mCurves[i*2]->SetValues4(&freqRespLo);
        mCurves[i*2 + 1]->SetValues4(&freqRespHi);
#endif
        
        mCurves[i*2]->SetValuesXDbDecimateDb(&freqRespLo,
					     NUM_POINTS_GRAPH, mSampleRate,
					     DECIM_FACTOR, MIN_DB2);
        
        mCurves[i*2 + 1]->SetValuesXDbDecimateDb(&freqRespHi,
						 NUM_POINTS_GRAPH, mSampleRate,
						 DECIM_FACTOR, MIN_DB2);
    }
#else
    
    CrossoverSplitterNBands4 splitterCopy = *mSplitter;
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(NUM_POINTS_DATA*2);
    mFilterResp->GenImpulse(&impulse);
    
    WDL_TypedBuf<BL_FLOAT> bands[5];
    splitterCopy.Split(impulse, bands);
    
    for (int i = 0; i < 5; i++)
    {
        WDL_TypedBuf<BL_GUI_FLOAT> bandResp;
        mFilterResp->GetFreqResp(bands[i], &bandResp);
        
        mCurves[i]->SetValuesXDbDecimateDb(&bandResp,
					   NUM_POINTS_GRAPH, mSampleRate,
					   DECIM_FACTOR, MIN_DB2);
    }
    
#if 1 // DEBUG
    WDL_TypedBuf<BL_FLOAT> sumBands;
    sumBands.Resize(bands[0].GetSize());
    BLUtils::FillAllZero(&sumBands);
    
    for (int i = 0; i < 5; i++)
    {
        BLUtils::AddValues(&sumBands, bands[i]);
    }
    
    WDL_TypedBuf<BL_GUI_FLOAT> sumBandResp;
    mFilterResp->GetFreqResp(sumBands, &sumBandResp);
    
    mCurves[5]->SetValuesXDbDecimateDb(&sumBandResp,
				       NUM_POINTS_GRAPH, mSampleRate,
				       DECIM_FACTOR, MIN_DB2);
#endif
    
#endif
    
    // Uselect all curves
    for (int i = 0; i < MB_NUM_CURVES/*8*/; i++)
    {
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
      mCurves[i]->SetColor(64, 64, 255);
#endif

#if ORANGE_COLOR_SCHEME
      mCurves[i]->SetColor(232, 110, 36);
#endif
        
#if BLUE_COLOR_SCHEME
      mCurves[i]->SetColor(170, 202, 209); //205, 243, 252);
#endif
    }
    
#if SPLITTER_N_BANDS
    int numFilters = 5;
    // In fact this is num bands
#endif
    
    // Select current curve (highlight)
    if ((filterNum >= 0) && (filterNum < numFilters))
    {
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
#define HIGHLIGHT_INTENSITY 200 //128
        mCurves[filterNum*2]->SetColor(HIGHLIGHT_INTENSITY, HIGHLIGHT_INTENSITY, 255);
        mCurves[filterNum*2 + 1]->SetColor(HIGHLIGHT_INTENSITY, HIGHLIGHT_INTENSITY, 255);
#endif
        
#if ORANGE_COLOR_SCHEME
        
#if !SPLITTER_N_BANDS
        mCurves[filterNum*2]->SetColor(252, 228, 205);
        mCurves[filterNum*2 + 1]->SetColor(252, 228, 205);
#else
#endif
        mCurves[filterNum]->SetColor(252, 228, 205);
        if (filterNum  + 1< numFilters)
	  mCurves[filterNum + 1]->SetColor(252, 228, 205);
#endif
        
#if BLUE_COLOR_SCHEME
        
#if !SPLITTER_N_BANDS
        mCurves[filterNum*2]->SetColor(255, 255, 255);
        mCurves[filterNum*2 + 1]->SetColor(255, 255, 255);
#else
#endif
        mCurves[filterNum]->SetColor(255, 255, 255);
        if (filterNum  + 1 < numFilters)
        {
            mCurves[filterNum + 1]->SetColor(255, 255, 255);
            
            // Set curve alpha not working
            //mGraph->SetCurveAlpha(filterNum + 1, 1.0);
        }
#endif

    }
    
    // Update graph
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
USTMultiBandDisplay::CreateCurves()
{
  for (int i = 0; i < MB_NUM_CURVES; i++)
    mCurves[i] = new GraphCurve5(MB_NUM_POINTS);
}

#endif // IGRAPHICS_NANOVG
