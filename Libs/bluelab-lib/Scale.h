//
//  Scale.h
//  Created by Pan on 08/04/18.
//
//

#ifndef __BL_Spectrogram__ColorMap__
#define __BL_Spectrogram__ColorMap__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class MelScale;
class FilterBank;
class Scale
{
public:
    // Note: log10, or log-anything is the same since we use normalized values
    enum Type
    {
        LINEAR = 0,
        NORMALIZED,
        DB,
        LOG,
        LOG10,
        LOG_FACTOR,
        MEL, // Quick Mel
        MEL_FILTER, // Mel with real filters,
        MEL_INV,
        MEL_FILTER_INV,
        DB_INV,
        LOW_ZOOM, // Zoom on low freqs
        LOG_NO_NORM, // Log, but without any normalization
        LOG_NO_NORM_INV
    };

    // Filter banks
    enum FilterBankType
    {
        FILTER_BANK_LINEAR = 0,
        FILTER_BANK_LOG,
        FILTER_BANK_LOG10,
        FILTER_BANK_LOG_FACTOR,
        FILTER_BANK_MEL,
        FILTER_BANK_LOW_ZOOM,
        NUM_FILTER_BANKS
    };
     
    Scale();
    virtual ~Scale();

    // Apply to Y

    // Apply value by value
    
    // Generic
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT ApplyScale(Type scaleType,
                                   BL_FLOAT x,
                                   BL_FLOAT minValue = -1.0,
                                   BL_FLOAT maxValue = -1.0);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT ApplyScaleInv(Type scaleType,
                                      BL_FLOAT x,
                                      BL_FLOAT minValue = -1.0,
                                      BL_FLOAT maxValue = -1.0);

    // OPTIM: Apply for each value
    void ApplyScaleForEach(Type scaleType,
                           WDL_TypedBuf<BL_FLOAT> *values,
                           BL_FLOAT minValue = -1.0,
                           BL_FLOAT maxValue = -1.0);
    
    void ApplyScaleInvForEach(Type scaleType,
                              WDL_TypedBuf<BL_FLOAT> *values,
                              BL_FLOAT minValue = -1.0,
                              BL_FLOAT maxValue = -1.0);
    
    // Apply to X
    
    //template <typename FLOAT_TYPE>
    void ApplyScale(Type scaleType,
                    WDL_TypedBuf<BL_FLOAT> *values,
                    BL_FLOAT minValue = -1.0,
                    BL_FLOAT maxValue = -1.0);

    // Filter banks
    //
    // NOTE: can decimate or increase the data size
    // as the same time as scaling!
    
    void ApplyScaleFilterBank(FilterBankType type,
                              WDL_TypedBuf<BL_FLOAT> *result,
                              const WDL_TypedBuf<BL_FLOAT> &magns,
                              BL_FLOAT sampleRate, int numFilters);

    void ApplyScaleFilterBankInv(FilterBankType type,
                                 WDL_TypedBuf<BL_FLOAT> *result,
                                 const WDL_TypedBuf<BL_FLOAT> &magns,
                                 BL_FLOAT sampleRate, int numFilters);

    FilterBankType TypeToFilterBankType(Type type);
    Type FilterBankTypeToType(FilterBankType fbType);
    
protected:
    BL_FLOAT ValueToNormalized(BL_FLOAT y,
                               BL_FLOAT minValue,
                               BL_FLOAT maxValue);

    BL_FLOAT ValueToNormalizedInv(BL_FLOAT y,
                                  BL_FLOAT minValue,
                                  BL_FLOAT maxValue);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToDB(BL_FLOAT y, BL_FLOAT mindB,
                                       BL_FLOAT maxdB);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToDBInv(BL_FLOAT y, BL_FLOAT mindB,
                                          BL_FLOAT maxdB);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToLog10(BL_FLOAT x, BL_FLOAT minValue,
                                          BL_FLOAT maxValue);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToLog10Inv(BL_FLOAT x, BL_FLOAT minValue,
                                             BL_FLOAT maxValue);

    BL_FLOAT NormalizedToLog(BL_FLOAT x, BL_FLOAT minValue,
                             BL_FLOAT maxValue);
    
    BL_FLOAT NormalizedToLogInv(BL_FLOAT x, BL_FLOAT minValue,
                                BL_FLOAT maxValue);

#if 0 // Legacy test
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToLogCoeff(BL_FLOAT x,
                                             BL_FLOAT minValue,
                                             BL_FLOAT maxValue);
#endif
    
    // Apply to axis for example
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToLogScale(BL_FLOAT value);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToLogScaleInv(BL_FLOAT value);

    BL_FLOAT NormalizedToLowZoom(BL_FLOAT x, BL_FLOAT minValue,
                                 BL_FLOAT maxValue);
    
    BL_FLOAT NormalizedToLowZoomInv(BL_FLOAT x, BL_FLOAT minValue,
                                    BL_FLOAT maxValue);
    
    // Apply to spectrogram for example
    //template <typename FLOAT_TYPE>
    /*static*/ void DataToLogScale(WDL_TypedBuf<BL_FLOAT> *values);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToMel(BL_FLOAT x,
                                        BL_FLOAT minFreq,
                                        BL_FLOAT maxFreq);
    
    //template <typename FLOAT_TYPE>
    /*static*/ BL_FLOAT NormalizedToMelInv(BL_FLOAT x,
                                           BL_FLOAT minFreq,
                                           BL_FLOAT maxFreq);

    BL_FLOAT ToLog(BL_FLOAT x);
    
    BL_FLOAT ToLogInv(BL_FLOAT x);
    
    //template <typename FLOAT_TYPE>
    /*static*/ void DataToMel(WDL_TypedBuf<BL_FLOAT> *values,
                              BL_FLOAT minFreq, BL_FLOAT maxFreq);
    
    //template <typename FLOAT_TYPE>
    void DataToMelFilter(WDL_TypedBuf<BL_FLOAT> *values,
                         BL_FLOAT minFreq, BL_FLOAT maxFreq);
    
    //template <typename FLOAT_TYPE>
    void DataToMelFilterInv(WDL_TypedBuf<BL_FLOAT> *values,
                            BL_FLOAT minFreq, BL_FLOAT maxFreq);

    // OPTIM: process in block
    //
    void ValueToNormalizedForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                  BL_FLOAT minValue, BL_FLOAT maxValue);
    
    void ValueToNormalizedInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                     BL_FLOAT minValue, BL_FLOAT maxValue);
    
    void NormalizedToDBForEach(WDL_TypedBuf<BL_FLOAT> *values,
                               BL_FLOAT mindB, BL_FLOAT maxdB);

    void NormalizedToDBInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                  BL_FLOAT mindB, BL_FLOAT maxdB);    
 
    void NormalizedToLog10ForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                  BL_FLOAT minValue, BL_FLOAT maxValue);
    
    void NormalizedToLog10InvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                     BL_FLOAT minValue, BL_FLOAT maxValue);

    void NormalizedToLogForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                BL_FLOAT minValue, BL_FLOAT maxValue);
    
    void NormalizedToLogInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                   BL_FLOAT minValue, BL_FLOAT maxValue);
    
    void NormalizedToLogScaleForEach(WDL_TypedBuf<BL_FLOAT> *values);
    
    void NormalizedToLogScaleInvForEach(WDL_TypedBuf<BL_FLOAT> *values);
    
    void NormalizedToMelForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                BL_FLOAT minFreq, BL_FLOAT maxFreq);
    
    void NormalizedToMelInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                   BL_FLOAT minFreq, BL_FLOAT maxFreq);

    void NormalizedToLowZoomForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                    BL_FLOAT minValue, BL_FLOAT maxValue);
    
    void NormalizedToLowZoomInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                       BL_FLOAT minValue, BL_FLOAT maxValue);

    void ToLogForEach(WDL_TypedBuf<BL_FLOAT> *values);
    
    void ToLogInvForEach(WDL_TypedBuf<BL_FLOAT> *values);
    
    //
    // Must keep the object, for precomputed filter bank
    MelScale *mMelScale;
    
    FilterBank *mFilterBanks[NUM_FILTER_BANKS];
    
private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
};

#endif
