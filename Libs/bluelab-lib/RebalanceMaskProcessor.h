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
