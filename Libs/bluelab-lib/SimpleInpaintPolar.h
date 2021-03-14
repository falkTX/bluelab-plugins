#ifndef SIMPLE_INPAINT_POLAR_H
#define SIMPLE_INPAINT_POLAR_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Very simple, but no so bad on real cases!
// Interpolate phases also, to avoid phases issues
// See: https://cradpdf.drdc-rddc.gc.ca/PDFS/unc341/p811241_A1b.pdf
// and: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
class SimpleInpaintPolar
{
 public:
    SimpleInpaintPolar(bool processHorizontal, bool processVertical);
    virtual ~SimpleInpaintPolar();

    void Process(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 int width, int height);
    
 protected:
    void ProcessHorizontal(WDL_TypedBuf<BL_FLOAT> *values,
                           int width, int height);

    void ProcessVertical(WDL_TypedBuf<BL_FLOAT> *values,
                         int width, int height);

    void ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *values,
                        int width, int height);

    //
    //void UnwrapPhasesHorizontal(WDL_TypedBuf<BL_FLOAT> *phases,
    //                            int width, int height);

    void ProcessPhasesHorizontal(WDL_TypedBuf<BL_FLOAT> *phases,
                                 int width, int height);

    // TODO: put this in BLUtils
    // And add notes in PhaseUnwrapper
    // Unwrap is "closest", not "next"
    // See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
    // this is the correct mehtod
    static void FindClosestPhase(BL_FLOAT *phase, BL_FLOAT refPhase);

    // This is a debug method, tham makes modulus PI and not TWO_PI
    // Useful for debugging
    static void FindClosestPhase180(BL_FLOAT *phase, BL_FLOAT refPhase);

    static void UnwrapPhases2(WDL_TypedBuf<BL_FLOAT> *phases,
                              bool dbgUnwrap180 = false);
        
    static void DBG_DumpPhaseRow(const char *fileName, int index,
                                 WDL_TypedBuf<BL_FLOAT> *phases,
                                 int width, int height);
    
    static void DBG_DumpSignal(const char *fileName,
                               WDL_TypedBuf<BL_FLOAT> *magns,
                               WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height,
                               int index);
    
    //
    bool mProcessHorizontal;
    bool mProcessVertical;
};

#endif
