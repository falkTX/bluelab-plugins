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
//  USTMultiBandDisplay.h
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#ifndef __UST__USTMultiBandDisplay__
#define __UST__USTMultiBandDisplay__

class GraphControl12;
class GraphCurve5;
class FilterFreqResp;

#define SPLITTER_N_BANDS 1 //0

#if !SPLITTER_N_BANDS
class CrossoverSplitter5Bands;
#else
class CrossoverSplitterNBands4;
#endif

// 5 + 1 for debug
#define MB_NUM_CURVES 6 // 8 ??
#define MB_NUM_POINTS 256

class USTMultiBandDisplay
{
public:
#if !SPLITTER_N_BANDS
    USTMultiBandDisplay(CrossoverSplitter5Bands *splitter,
                        BL_FLOAT sampleRate);
#else
    USTMultiBandDisplay(CrossoverSplitterNBands4 *splitter,
                        BL_FLOAT sampleRate);
#endif
    
    virtual ~USTMultiBandDisplay();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetGraph(GraphControl12 *graph);
    
    void Update(int filterNum);
    
protected:
    void CreateCurves();

    //
    GraphControl12 *mGraph;
    
#if !SPLITTER_N_BANDS
    CrossoverSplitter5Bands *mSplitter;
#else
    CrossoverSplitterNBands4 *mSplitter;
#endif
    
    FilterFreqResp *mFilterResp;
    
    BL_FLOAT mSampleRate;

    // Curves
    GraphCurve5 *mCurves[MB_NUM_CURVES];
};

#endif /* defined(__UST__USTMultiBandDisplay__) */
