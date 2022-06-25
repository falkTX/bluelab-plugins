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
//  MultiViewer2.h
//  BL-Reverb
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef __BL_Reverb__MultiViewer2__
#define __BL_Reverb__MultiViewer2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class GUIHelper12;
class GraphControl12;
class SpectrogramDisplay2;
class SamplesToSpectrogram;
class GraphTimeAxis6;
class GraphFreqAxis2;
class GraphAxis2;
class GraphCurve5;
class MultiViewer2
{
public:
    MultiViewer2(BL_FLOAT sampleRate, int bufferSize);
    
    virtual ~MultiViewer2();
    
    void Reset(BL_FLOAT sampleRate, int bufferSize);

    void SetGraph(GraphControl12 *graph,
                  SpectrogramDisplay2 *spectroDisplay,
                  GUIHelper12 *guiHelper = NULL);
    
    void SetTime(BL_FLOAT durationSeconds, BL_FLOAT timeOrigin = 0.0);
    void SetSamples(const WDL_TypedBuf<BL_FLOAT> &data);
    
protected:
    void UpdateFrequencyScale();
    
    //
    BL_FLOAT mSampleRate;
    int mBufferSize;
    
    GraphControl12 *mGraph;
    SpectrogramDisplay2 *mSpectroDisplay;
    
    SamplesToSpectrogram *mSamplesToSpectro;
    
    GraphTimeAxis6 *mTimeAxis;
    GraphAxis2 *mHAxis;
    
    GraphFreqAxis2 *mFreqAxis;
    GraphAxis2 *mVAxis;

    // 2, for overlay
    GraphCurve5 *mWaveformCurves[2];
    
    //
    GUIHelper12 *mGUIHelper;
};

#endif /* defined(__BL_Reverb__MultiViewer2__) */
