#ifndef BL_UTILS_PHASES_H
#define BL_UTILS_PHASES_H


class BLUtilsPhases
{
 public:
    // fmod() managing correctly negative numbers
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE fmod_negative(FLOAT_TYPE x, FLOAT_TYPE y);
    
    // Phases
    template <typename FLOAT_TYPE>
    static void FindNextPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);
    
    template <typename FLOAT_TYPE>
    static void UnwrapPhases(WDL_TypedBuf<FLOAT_TYPE> *phases,
                             bool adjustFirstPhase = true);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE MapToPi(FLOAT_TYPE val);

    template <typename FLOAT_TYPE>
    static void MapToPi(WDL_TypedBuf<FLOAT_TYPE> *values);

    // New unwrap
    
    // Real unwrap is "closest", not "next"
    // See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
    // this is the correct mehtod
    template <typename FLOAT_TYPE>
    static void FindClosestPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);

    // Use "closest"
    template <typename FLOAT_TYPE>
    static void UnwrapPhases2(WDL_TypedBuf<FLOAT_TYPE> *phases,
                              bool dbgUnwrap180 = false);

    // This is a debug method, tham makes modulus PI and not TWO_PI
    // Useful for debugging
    template <typename FLOAT_TYPE>
    static void FindClosestPhase180(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);
};

#endif
