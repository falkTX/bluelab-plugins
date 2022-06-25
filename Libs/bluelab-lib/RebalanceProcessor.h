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
//  RebalanceProcessor.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceProcessor_hpp
#define RebalanceProcessor_hpp

#include <BLTypes.h>
#include <ResampProcessObj.h>
#include <Rebalance_defs.h>


class FftProcessObj16;
class RebalanceDumpFftObj2;
class RebalanceProcessFftObjComp3;
class RebalanceMaskPredictorComp6;

class RebalanceProcessor : public ResampProcessObj
{
public:
    RebalanceProcessor(BL_FLOAT sampleRate, BL_FLOAT targetSampleRate,
                       int bufferSize, int targetBufferSize,
                       int overlapping,
                       int numSpectroCols);
    
    virtual ~RebalanceProcessor();
    
    void InitDetect(const IPluginBase &plug,
                    IGraphics& graphics);
    
    void InitDump();
    
    // Must be called at least once
    void Reset(BL_FLOAT sampleRate);
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    // Predict
    //
    void SetVocal(BL_FLOAT vocal);
    void SetBass(BL_FLOAT bass);
    void SetDrums(BL_FLOAT drums);
    void SetOther(BL_FLOAT other);
    
    void SetMasksContrast(BL_FLOAT contrast);
    
    void SetVocalSensitivity(BL_FLOAT vocalSensitivity);
    void SetBassSensitivity(BL_FLOAT bassSensitivity);
    void SetDrumsSensitivity(BL_FLOAT drumsSensitivity);
    void SetOtherSensitivity(BL_FLOAT otherSensitivity);
    
    // Dump
    //
    bool HasEnoughDumpData();
    
    void GetSpectroData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS]);
    void GetStereoData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS]);
    
    // Debug
    void SetDbgThreshold(BL_FLOAT thrs);
    
protected:
    bool ProcessSamplesBuffers(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers,
                               vector<WDL_TypedBuf<BL_FLOAT> > *ioResampBuffers) override;
    
    //
    int mBufferSize;
    int mTargetBufferSize;
    int mOverlapping;
    
    int mBlockSize;
    
    int mNumSpectroCols;
    
    FftProcessObj16 *mNativeFftObj;
    FftProcessObj16 *mTargetFftObj;
    
    RebalanceDumpFftObj2 *mDumpObj;
    
    RebalanceMaskPredictorComp6 *mMaskPred;
    
    RebalanceProcessFftObjComp3 *mDetectProcessObjs[2];
};

#endif /* RebalanceProcessor_hpp */
