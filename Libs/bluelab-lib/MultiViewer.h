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
