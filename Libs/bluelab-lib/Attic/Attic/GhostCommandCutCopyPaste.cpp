#include <Scale.h>

#include "GhostCommandCutCopyPaste.h"

GhostCommandCutCopyPaste::GhostCommandCutCopyPaste(BL_FLOAT sampleRate)
: GhostCommandCopyPaste(sampleRate) {}

GhostCommandCutCopyPaste::GhostCommandCutCopyPaste(const GhostCommandCutCopyPaste &other)
: GhostCommandCopyPaste(other.mSampleRate) {}

GhostCommandCutCopyPaste::~GhostCommandCutCopyPaste() {}

void
GhostCommandCutCopyPaste::Copy(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                               const vector<WDL_TypedBuf<BL_FLOAT> > &phases,
                               int offsetXLines)
{
    GhostCommandCopyPaste::Copy(magns, phases, offsetXLines);
}

void
GhostCommandCutCopyPaste::Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                                vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    GhostCommandCopyPaste::Apply(magns, phases);
}

void
GhostCommandCutCopyPaste::Undo(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                               vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    GhostCommandCopyPaste::Undo(magns, phases);

    /*if (!mIsPasteDone)
      return;
      
      // Set the selection to the state chan pasted
      for (int i = 0; i < 4; i++)
      mSelection[i] = mPastedSelection[i];
      
      GhostCommand::Undo(magns, phases);
    */
}

bool
GhostCommandCutCopyPaste::IsPasteDone()
{
    return mIsPasteDone;
}

void
GhostCommandCutCopyPaste::ComputePastedSelection()
{
    BL_FLOAT copySelectWidth = mCopiedSelection[2] - mCopiedSelection[0];
    BL_FLOAT copySelectHeight = mCopiedSelection[3] - mCopiedSelection[1];
    
    // Paste the selection at the same y that was copied
    // (because we must paste at the same frequency range)
    mPastedSelection[0] = mSelection[0];
    mPastedSelection[1] = mCopiedSelection[1]; // y0 when copied
    
    mPastedSelection[2] = mSelection[0] + copySelectWidth;
    mPastedSelection[3] = mCopiedSelection[1] + copySelectHeight;
}

void
GhostCommandCutCopyPaste::GetPastedSelection(BL_FLOAT pastedSelection[4],
                                             Scale::Type yScale)
//bool yLogScale)
{
    for (int i = 0; i < 4; i++)
        pastedSelection[i] = mPastedSelection[i];
    
#if 0 // TODO
    if (yLogScale)
    {
        pastedSelection[1] = BLUtils::LogScaleNormInv(pastedSelection[1], 1.0, Y_LOG_SCALE_FACTOR);
        pastedSelection[3] = BLUtils::LogScaleNormInv(pastedSelection[3], 1.0, Y_LOG_SCALE_FACTOR);
    }
#endif
    
    pastedSelection[1] = mScale->ApplyScale(yScale, pastedSelection[1],
                                            (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
    pastedSelection[3] = mScale->ApplyScale(yScale, pastedSelection[3],
                                            (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
}

int
GhostCommandCutCopyPaste::GetOffsetXLines()
{
    return mOffsetXLines;
}
