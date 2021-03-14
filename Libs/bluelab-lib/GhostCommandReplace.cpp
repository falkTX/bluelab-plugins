#include <BLTypes.h>

#include <ImageInpaint2.h>

#include <SimpleInpaint.h>
#include <SimpleInpaintComp.h>
//#include <SimpleInpaintPolar.h>
#include <SimpleInpaintPolar2.h>

#include "GhostCommandReplace.h"

// Origin: 0 (use ImageInpaint2)
#define USE_IMAGE_INPAINT 0
#define USE_SIMPLE_INPAINT 0 //1
#define USE_SIMPLE_INPAINT_COMP 0
#define USE_SIMPLE_INPAINT_POLAR 1

GhostCommandReplace::GhostCommandReplace(BL_FLOAT sampleRate,
                                         bool processHorizontal, bool processVertical)
: GhostCommand(sampleRate)
{
    mProcessHorizontal = processHorizontal;
    mProcessVertical = processVertical;
}

GhostCommandReplace::~GhostCommandReplace() {}

void
GhostCommandReplace::Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                           vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    // Do not use phases
    
#define BORDER_RATIO 0.1

    // Get the selected data, just for convenience
    WDL_TypedBuf<BL_FLOAT> selectedMagns;
    GetSelectedDataY(*magns, &selectedMagns);

    // Phases, for SimpleInpaintComp
    WDL_TypedBuf<BL_FLOAT> selectedPhases;
    GetSelectedDataY(*phases, &selectedPhases);
    
    int y0;
    int y1;
    GetDataBoundsSlice(*magns, &y0, &y1);
    
    int width = (int)magns->size();
    int height = y1 - y0;
    
#if 0 // old method, worked only for background noise
    ImageInpaint::Inpaint(selectedMagns.Get(),
                          width, height, BORDER_RATIO);
#endif

#if USE_IMAGE_INPAINT
    // New method, use real (but simple) inpainting
    ImageInpaint2::Inpaint(selectedMagns.Get(),
                           width, height, BORDER_RATIO,
                           mProcessHorizontal,
                           mProcessVertical);
#endif

#if USE_SIMPLE_INPAINT
    SimpleInpaint inpaint(mProcessHorizontal, mProcessVertical);
    inpaint.Process(&selectedMagns, width, height);
#endif

#if USE_SIMPLE_INPAINT_COMP
    // Complex simple inpaint
    SimpleInpaintComp inpaint(mProcessHorizontal, mProcessVertical);
    inpaint.Process(&selectedMagns, &selectedPhases, width, height);
#endif

#if USE_SIMPLE_INPAINT_POLAR
    SimpleInpaintPolar2 inpaint(mProcessHorizontal, mProcessVertical);
    inpaint.Process(&selectedMagns, &selectedPhases, width, height);
#endif
    
    // And replace in the result
    ReplaceSelectedDataY(magns, selectedMagns);
    ReplaceSelectedDataY(phases, selectedPhases);
}
