//
//  MultiViewer.cpp
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

// Hack
#define USE_GRAPH_OGL 1

#include <GUIHelper11.h>
#include <GraphControl11.h>

#include <SamplesToSpectrogram.h>

#include <GraphTimeAxis4.h>

#include <BLUtils.h>

#include "MultiViewer.h"


#define GRAPH_NUM_CURVES 3
#define GRAPH_CONTROL_NUM_POINTS 1024 //512 //256

// 2 curves, for overlay
#define GRAPH_WAVEFORM_CURVE0 1
#define GRAPH_WAVEFORM_CURVE  2
#define Y_AXIS_CURVE_HZ       0

#define Y_LOG_SCALE_FACTOR 3.5


MultiViewer::MultiViewer(Plugin *plug, IGraphics *pGraphics,
                         GUIHelper11 *guiHelper,
                         int graphId, const char *graphFn,
                         int graphFrames, int graphX, int graphY,
                         int graphParam, const char *graphParamName,
                         int graphShadowsId, const char *graphShwdowsFn,
                         BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    // Graph
    mGraph = guiHelper->CreateGraph(plug, pGraphics,
                                    graphX, graphY,
                                    graphFn, graphParam,
                                    GRAPH_NUM_CURVES, GRAPH_CONTROL_NUM_POINTS);
    
    mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
    
    mGraph->SetClearColor(0, 0, 0, 255);
    
    int sepColor[4] = { 24, 24, 24, 255 };
    mGraph->SetSeparatorY0(2.0, sepColor);
    
    //
    mSamplesToSpectro = new SamplesToSpectrogram(sampleRate);
    
    BLSpectrogram3 *spectro = mSamplesToSpectro->GetSpectrogram();
    mGraph->SetSpectrogram(spectro, 0.0, 0.0, 1.0, 1.0);
    
    // Curves
    //
    
    // Normal
    mGraph->SetCurveColor(GRAPH_WAVEFORM_CURVE, 255, 255, 255); //64, 64, 255);
    mGraph->SetCurveAlpha(GRAPH_WAVEFORM_CURVE, 1.0);
    mGraph->SetCurveLineWidth(GRAPH_WAVEFORM_CURVE, 1.0);
    mGraph->SetCurveFill(GRAPH_WAVEFORM_CURVE, false);
    mGraph->SetCurveYScale(GRAPH_WAVEFORM_CURVE, false, -1.0, 1.0);
    mGraph->SetCurveLimitToBounds(GRAPH_WAVEFORM_CURVE, true);
    
    // Overlay
    mGraph->SetCurveColor(GRAPH_WAVEFORM_CURVE0, 64, 64, 64);
    mGraph->SetCurveAlpha(GRAPH_WAVEFORM_CURVE0, 1.0);
    mGraph->SetCurveLineWidth(GRAPH_WAVEFORM_CURVE0, 2.0); //2.0);
    mGraph->SetCurveFill(GRAPH_WAVEFORM_CURVE0, false);
    mGraph->SetCurveYScale(GRAPH_WAVEFORM_CURVE0, false, -1.0, 1.0);
    mGraph->SetCurveLimitToBounds(GRAPH_WAVEFORM_CURVE0, true);
    
    // Time axis
    int bufferSize = mSamplesToSpectro->GetBufferSize();
    mTimeAxis = new GraphTimeAxis4();
    mTimeAxis->Init(mGraph, bufferSize, 1.0, 0.1);
    
    // Freq axis
    UpdateFrequencyScale();
}

MultiViewer::~MultiViewer()
{
    delete mSamplesToSpectro;
    
    delete mTimeAxis;
}

void
MultiViewer::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    UpdateFrequencyScale();
    
    mSamplesToSpectro->Reset(sampleRate);
}

GraphControl11 *
MultiViewer::GetGraph()
{
    return mGraph;
}

void
MultiViewer::SetTime(BL_FLOAT durationSeconds, BL_FLOAT timeOrigin)
{
    int bufferSize = mSamplesToSpectro->GetBufferSize();
    
    mTimeAxis->Reset(bufferSize, durationSeconds, durationSeconds/10.0);
    mTimeAxis->Update(timeOrigin);
}

void
MultiViewer::SetSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mSamplesToSpectro->SetSamples(samples);
    
    // Update
    mGraph->UpdateSpectrogram(true, true);
    
    // Curves
    WDL_TypedBuf<BL_FLOAT> decimValues;
    BL_FLOAT decFactor = ((BL_FLOAT)GRAPH_CONTROL_NUM_POINTS)/samples.GetSize();
    BLUtils::DecimateSamples(&decimValues, samples, decFactor);
    
    mGraph->SetCurveValues3(GRAPH_WAVEFORM_CURVE, &decimValues);
    mGraph->SetCurveValues3(GRAPH_WAVEFORM_CURVE0, &decimValues);
}

void
MultiViewer::UpdateFrequencyScale()
{
    if (mGraph == NULL)
        return;
    
#define NUM_AXIS_DATA 9
    char *AXIS_DATA [NUM_AXIS_DATA][2];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        AXIS_DATA[i][0] = (char *)malloc(255);
        AXIS_DATA[i][1] = (char *)malloc(255);
    }
    
    sprintf(AXIS_DATA[0][1], "");
    sprintf(AXIS_DATA[1][1], "100Hz");
    sprintf(AXIS_DATA[2][1], "500Hz");
    sprintf(AXIS_DATA[3][1], "1KHz");
    sprintf(AXIS_DATA[4][1], "2KHz");
    sprintf(AXIS_DATA[5][1], "5KHz");
    sprintf(AXIS_DATA[6][1], "10KHz");
    sprintf(AXIS_DATA[7][1], "20KHz");
    sprintf(AXIS_DATA[8][1], "");
    
    BL_FLOAT freqs[NUM_AXIS_DATA] =
    { 50.0, 100.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0, 40000.0 };
    
    BL_FLOAT minHzValue;
    BL_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                   BUFFER_SIZE, mSampleRate);
    
    
    // Avoid a shift
    minHzValue = 0.0;
    
    // Normalize
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        freqs[i] = (freqs[i] - minHzValue)/(maxHzValue - minHzValue);
        
        //freqs[i] = BLUtils::LogScale(freqs[i], Y_LOG_SCALE_FACTOR);
        freqs[i] = BLUtils::LogScaleNormInv(freqs[i],
                                          (BL_FLOAT)1.0, (BL_FLOAT)Y_LOG_SCALE_FACTOR);
        
        sprintf(AXIS_DATA[i][0], "%g", freqs[i]);
    }
    
    // Dummy Y axis curve, for the scale of the axis
    mGraph->SetCurveYScale(Y_AXIS_CURVE_HZ, false, 0.0, 1.0);
    
    int axisColor[4] = { 48, 48, 48, /*255*/0 }; // invisible horizontal bars
    
    // Choose maximum brightness color for labels,
    // to see them well over clear spectrograms
    int axisLabelColor[4] = { 255, 255, 255, 255 };
    int axisOverlayColor[4] = { 48, 48, 48, 255 };
    
    int width = mGraph->GetRECT().W();
    
    mGraph->RemoveVAxis();
    
    mGraph->AddVAxis(AXIS_DATA, NUM_AXIS_DATA, axisColor, axisLabelColor, 0.0, width - 40.0,
                     axisOverlayColor);
    
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        free(AXIS_DATA[i][0]);
        free(AXIS_DATA[i][1]);
    }
}
