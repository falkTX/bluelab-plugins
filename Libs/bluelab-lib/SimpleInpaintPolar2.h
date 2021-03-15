#ifndef SIMPLE_INPAINT_POLAR2_H
#define SIMPLE_INPAINT_POLAR2_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Very simple, but no so bad on real cases!
// Interpolate phases also, to avoid phases issues
// See: https://cradpdf.drdc-rddc.gc.ca/PDFS/unc341/p811241_A1b.pdf
// and: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
//
class SimpleInpaintPolar2
{
 public:
    SimpleInpaintPolar2(bool processHoriz, bool processVert);
    virtual ~SimpleInpaintPolar2();

    void Process(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 int width, int height);
    
 protected:
    // Magns
    void ProcessMagnsHoriz(WDL_TypedBuf<BL_FLOAT> *magns, int width, int height);
    void ProcessMagnsVert(WDL_TypedBuf<BL_FLOAT> *magns, int width, int height);
    void ProcessMagnsBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                             int width, int height);

    // "both", magns + phases
    void ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                        WDL_TypedBuf<BL_FLOAT> *phases,
                        int width, int height);
    
    // Phases, horizontal
    //
    
    //
    void ProcessPhasesHorizLToR(WDL_TypedBuf<BL_FLOAT> *phases,
                                int width, int height);

    // TEST
    // From right to left
    void ProcessPhasesHorizRToL(WDL_TypedBuf<BL_FLOAT> *phases,
                                int width, int height);

    // Combined, L -> R or R -> L (choose the best)
    void ProcessPhasesHorizCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    WDL_TypedBuf<BL_FLOAT> *phases,
                                    int width, int height);

    // Method 1 (not good)
    void CombinePhaseHorizInterp(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                 const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                 WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                 int width, int height);

    // Method 2 (a bit better, but not very good)
    void CombinePhaseHorizInterpComp(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                     const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                     const WDL_TypedBuf<BL_FLOAT> &magns,
                                     WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                     int width, int height);

    // Method 3: take the bigger magn to find the direction
    // for each line => very good!
    void CombinePhaseHorizBiggestMagn(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                      const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                      const WDL_TypedBuf<BL_FLOAT> &magns,
                                      WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                      int width, int height);

    // Phases, vertical
    //

    // Very good
    void ProcessPhasesVertDToU(WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height);

    
    void ProcessPhasesVertUToD(WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height);

    // Combined, D -> U or U -> D (choose the best)
    void ProcessPhasesVertCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   WDL_TypedBuf<BL_FLOAT> *phases,
                                   int width, int height);
    
    void CombinePhaseVertBiggestMagn(const WDL_TypedBuf<BL_FLOAT> &phasesD,
                                     const WDL_TypedBuf<BL_FLOAT> &phasesU,
                                     const WDL_TypedBuf<BL_FLOAT> &magns,
                                     WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                     int width, int height);
    
    // TODO: put this in BLUtils
    //
    // And add notes in PhaseUnwrapper
    // Unwrap is "closest", not "next"
    // See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
    // this is the correct mehtod
    static void FindClosestPhase(BL_FLOAT *phase, BL_FLOAT refPhase);

    // TODO: put it in BLUtils
    static void UnwrapPhases2(WDL_TypedBuf<BL_FLOAT> *phases,
                              bool dbgUnwrap180 = false);

    // This is a debug method, tham makes modulus PI and not TWO_PI
    // Useful for debugging
    static void FindClosestPhase180(BL_FLOAT *phase, BL_FLOAT refPhase);

    // 1 line, all the frequencies
    static void DBG_DumpPhaseCol(const char *fileName, int timeIndex,
                                 WDL_TypedBuf<BL_FLOAT> *phases,
                                 int width, int height);

    // TODO: put this in BLUtils
    static void InterpComp(BL_FLOAT magn0, BL_FLOAT phase0,
                           BL_FLOAT magn1, BL_FLOAT phase1,
                           BL_FLOAT t,
                           BL_FLOAT *resMagn, BL_FLOAT *resPhase);

    // => BLUtils
    static void InterpComp(const WDL_TypedBuf<BL_FLOAT> &magns0,
                           const WDL_TypedBuf<BL_FLOAT> &phases0,
                           const WDL_TypedBuf<BL_FLOAT> &magns1,
                           const WDL_TypedBuf<BL_FLOAT> &phases1,
                           BL_FLOAT t,
                           WDL_TypedBuf<BL_FLOAT> *resMagns,
                           WDL_TypedBuf<BL_FLOAT> *resPhases);
    
    // 1 line, all the times
    static void DBG_DumpPhaseLine(const char *fileName, int freqIndex,
                                  WDL_TypedBuf<BL_FLOAT> *phases,
                                  int width, int height);
    
    static void DBG_DumpSignal(const char *fileName,
                               WDL_TypedBuf<BL_FLOAT> *magns,
                               WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height,
                               int index);
    
    //
    bool mProcessHoriz;
    bool mProcessVert;
};

#endif
