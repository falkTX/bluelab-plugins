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
 
#ifndef REBALANCE_MASK_PROCESSOR_H
#define REBALANCE_MASK_PROCESSOR_H

#include <BLTypes.h>

// Include for defines
#include <Rebalance_defs.h>

#include "IPlug_include_in_plug_hdr.h"

class RebalanceMaskProcessor
{
 public:
    RebalanceMaskProcessor();
    virtual ~RebalanceMaskProcessor();

    // A mask can be only one line of mask for example
    void Process(const WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
                 WDL_TypedBuf<BL_FLOAT> *resultMask);

    // For RebalanceStereo
    void ProcessSeparate(const WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
                         WDL_TypedBuf<BL_FLOAT> resultMasks[NUM_STEM_SOURCES]);
    
    void SetVocalMix(BL_FLOAT vocalMix);
    void SetBassMix(BL_FLOAT bassMix);
    void SetDrumsMix(BL_FLOAT drumsMix);
    void SetOtherMix(BL_FLOAT otherMix);
    
    void SetVocalSensitivity(BL_FLOAT vocalSensitivity);
    void SetBassSensitivity(BL_FLOAT bassSensitivity);
    void SetDrumsSensitivity(BL_FLOAT drumsSensitivity);
    void SetOtherSensitivity(BL_FLOAT otherSensitivity);

    // Masks contrast, relative one to each other (previous soft/hard)
    void SetContrast(BL_FLOAT contrast);

 protected:
    // Sensivity
    void ApplySensitivitySoft(BL_FLOAT masks[NUM_STEM_SOURCES]);
    void ApplySensitivity(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);

    void ApplyMix(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);
    
    void ApplyMasksContrast(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);

    void NormalizeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);

    //
    // Parameters
    BL_FLOAT mMixes[NUM_STEM_SOURCES];
    BL_FLOAT mSensitivities[NUM_STEM_SOURCES];

    // Masks contract, relative one to the othersn
    struct MaskContrastStruct
    {
        int mMaskId;
        BL_FLOAT mValue;
        
        static bool ValueSmaller(const MaskContrastStruct m0,
                                 const MaskContrastStruct &m1)
        {
            return m0.mValue < m1.mValue;
        }
    };
    
    BL_FLOAT mMasksContrast;
    
private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0[NUM_STEM_SOURCES];
    vector<MaskContrastStruct> mTmpBuf1;
};

#endif
