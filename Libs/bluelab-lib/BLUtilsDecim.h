#ifndef BL_UTILS_DECIM_H
#define BL_UTILS_DECIM_H

#include "IPlug_include_in_plug_hdr.h"

class BLUtilsDecim
{
 public:
    // Samples
    template <typename FLOAT_TYPE>
    static void DecimateValues(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<FLOAT_TYPE> &buf,
                               FLOAT_TYPE decFactor);
    
    template <typename FLOAT_TYPE>
    static void DecimateValues(WDL_TypedBuf<FLOAT_TYPE> *ioValues,
                               FLOAT_TYPE decFactor);
    
    template <typename FLOAT_TYPE>
    static void DecimateValuesDb(WDL_TypedBuf<FLOAT_TYPE> *result,
                                 const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                 FLOAT_TYPE decFactor, FLOAT_TYPE minValueDb);
    
    // For samples (i.e preserve waveform)
    template <typename FLOAT_TYPE>
    static void DecimateSamples(WDL_TypedBuf<FLOAT_TYPE> *result,
                                const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                FLOAT_TYPE decFactor);

    template <typename FLOAT_TYPE>
    static void DecimateSamples(WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                                FLOAT_TYPE decFactor);
    
    // DOESN'T WORK ?
    // Fix for flat sections at 0
    template <typename FLOAT_TYPE>
    static void DecimateSamples2(WDL_TypedBuf<FLOAT_TYPE> *result,
                                 const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                 FLOAT_TYPE decFactor);

    template <typename FLOAT_TYPE>
    static void DecimateSamples3(WDL_TypedBuf<FLOAT_TYPE> *result,
                                 const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                 FLOAT_TYPE decFactor);
    
    // Quick and rought version
    template <typename FLOAT_TYPE>
    static void DecimateSamplesFast(WDL_TypedBuf<FLOAT_TYPE> *result,
                                    const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                    FLOAT_TYPE decFactor);

    template <typename FLOAT_TYPE>
    static void DecimateSamplesFast(WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                                    FLOAT_TYPE decFactor);
    
    // Very basic
    template <typename FLOAT_TYPE>
    static void DecimateStep(WDL_TypedBuf<FLOAT_TYPE> *ioSamples, int step);

    template <typename FLOAT_TYPE>
    static void DecimateStep(const WDL_TypedBuf<FLOAT_TYPE> &inSamples,
                             WDL_TypedBuf<FLOAT_TYPE> *outSamples,
                             int step);
};

#endif
