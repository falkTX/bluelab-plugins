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
//  MultiViewer2.cpp
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GUIHelper12.h>
#include <GraphControl12.h>

#include <SpectrogramDisplay2.h>

#include <SamplesToSpectrogram.h>

#include <GraphTimeAxis6.h>
#include <GraphFreqAxis2.h>
#include <GraphAxis2.h>

#include <BLUtils.h>

#include "MultiViewer2.h"

#define MULTI_VIEWER2_BUFFER_SIZE 2048

#define GRAPH_CONTROL_NUM_POINTS 1024 //512 //256

#define BEVEL_CURVES 1

#define TIME_AXIS_NUM_LABELS 10

MultiViewer2::MultiViewer2(BL_FLOAT sampleRate, int bufferSize)
{
    mSampleRate = sampleRate;
    mBufferSize = bufferSize;
    
    mGUIHelper = NULL;
    mGraph = NULL;
    
    mSpectroDisplay = NULL;
    
    //
    mSamplesToSpectro = new SamplesToSpectrogram(sampleRate);

    // Time axis
    mHAxis = new GraphAxis2();
    mTimeAxis = new GraphTimeAxis6(true, false);

    mVAxis = new GraphAxis2();
    mFreqAxis = new GraphFreqAxis2(false, Scale::MEL);

    for (int i = 0; i < 2; i++)
        mWaveformCurves[i] = NULL;
    
    // Freq axis
    UpdateFrequencyScale();
}

MultiViewer2::~MultiViewer2()
{
    delete mSamplesToSpectro;    

    delete mHAxis;
    delete mTimeAxis;

    delete mVAxis;
    delete mFreqAxis;

    for (int i = 0; i < 2; i++)
        delete mWaveformCurves[i];
}

void
MultiViewer2::Reset(BL_FLOAT sampleRate, int bufferSize)
{
    mSampleRate = sampleRate;
    mBufferSize = bufferSize;
    
    //
    if ((mGraph != NULL) && (mGUIHelper != NULL))
    {
        mTimeAxis->Init(mGraph, mHAxis, mGUIHelper, mBufferSize, 1.0, TIME_AXIS_NUM_LABELS);
    
        int graphWidth;
        int graphHeight;
        mGraph->GetSize(&graphWidth, &graphHeight);
    
        mFreqAxis->Init(mVAxis, mGUIHelper, false, mBufferSize, mSampleRate, graphWidth);
    }
    //
    UpdateFrequencyScale();
    
    mSamplesToSpectro->Reset(sampleRate);
}

void
MultiViewer2::SetGraph(GraphControl12 *graph,
                       SpectrogramDisplay2 *spectroDisplay,
                       GUIHelper12 *guiHelper)
{
    mGraph = graph;
    mSpectroDisplay = spectroDisplay;
    mGUIHelper = guiHelper;
    
    if (mGraph == NULL)
        return;
    
    if ((mGraph != NULL) && (mGUIHelper != NULL))
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraph->SetClearColor(0, 0, 0, 255);
        int sepColor[4] = { 24, 24, 24, 255 };
        mGraph->SetSeparatorY0(2.0, sepColor);

        mGraph->SetHAxis(mHAxis);
        mGraph->SetVAxis(mVAxis);
    }

    BLSpectrogram4 *spectro = mSamplesToSpectro->GetSpectrogram();
    
    mSpectroDisplay = spectroDisplay;
    if (mSpectroDisplay != NULL)
    {
        mSpectroDisplay->SetBounds(0.0, 0.0, 1.0, 1.0);
        mSpectroDisplay->SetSpectrogram(spectro);

        if (mGraph != NULL)
            mGraph->AddCustomDrawer(mSpectroDisplay);
    }
    
    // Curves
    //
    if (mWaveformCurves[0] == NULL)
        // Not yet created
    {    
        // Waveform curve
        int waveformColor[4];
        mGUIHelper->GetGraphCurveColorBlue(waveformColor);
    
        mWaveformCurves[0] = new GraphCurve5(GRAPH_CONTROL_NUM_POINTS);
        mWaveformCurves[0]->SetXScale(Scale::LINEAR, 0.0, 1.0);
        mWaveformCurves[0]->SetYScale(Scale::LINEAR, -1.0, 1.0);
        mWaveformCurves[0]->SetColor(waveformColor[0], waveformColor[1], waveformColor[2]);
        mWaveformCurves[0]->SetLineWidth(1.0);
      
#if BEVEL_CURVES
        mWaveformCurves[0]->SetBevel(true);
#endif
  }

    if (mWaveformCurves[1] == NULL)
        // Not yet created
    {    
        // WaveformOverlay curve
        int waveformOverlayColor[4];
        mGUIHelper->GetGraphCurveColorGray(waveformOverlayColor);
        
        mWaveformCurves[1] = new GraphCurve5(GRAPH_CONTROL_NUM_POINTS);
        mWaveformCurves[1]->SetXScale(Scale::LINEAR, 0.0, 1.0);
        mWaveformCurves[1]->SetYScale(Scale::LINEAR, -1.0, 1.0);
        mWaveformCurves[1]->SetColor(waveformOverlayColor[0],
                                        waveformOverlayColor[1],
                                        waveformOverlayColor[2]);
        mWaveformCurves[1]->SetLineWidth(2.0);
        
#if BEVEL_CURVES
        mWaveformCurves[1]->SetBevel(true);
#endif
    }

    mGraph->AddCurve(mWaveformCurves[1]);
    mGraph->AddCurve(mWaveformCurves[0]);
    
    mTimeAxis->Init(mGraph, mHAxis, mGUIHelper, mBufferSize, 1.0, TIME_AXIS_NUM_LABELS);

    int graphWidth;
    int graphHeight;
    mGraph->GetSize(&graphWidth, &graphHeight);
    
    mFreqAxis->Init(mVAxis, mGUIHelper, false, mBufferSize, mSampleRate, graphWidth);
}

void
MultiViewer2::SetTime(BL_FLOAT durationSeconds, BL_FLOAT timeOrigin)
{
    int bufferSize = mSamplesToSpectro->GetBufferSize();
    
    mTimeAxis->Reset(bufferSize, durationSeconds, TIME_AXIS_NUM_LABELS);
    //mTimeAxis->UpdateFromTransport(timeOrigin);
    mTimeAxis->Update(timeOrigin);
}

void
MultiViewer2::SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mSamplesToSpectro->SetSamples(samples);

    if (mSpectroDisplay != NULL)
        mSpectroDisplay->UpdateSpectrogram();
    
    if (mWaveformCurves[0] != NULL)
        mWaveformCurves[0]->SetValues4(samples);
    
    if (mWaveformCurves[1] != NULL)
        mWaveformCurves[1]->SetValues4(samples);
}

void
MultiViewer2::UpdateFrequencyScale()
{
    if (mGraph == NULL)
        return;

    BL_FLOAT minHzValue;
    BL_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                     MULTI_VIEWER2_BUFFER_SIZE, mSampleRate);
    mFreqAxis->SetMaxFreq(maxHzValue);
}

#endif // IGRAPHICS_NANOVG
