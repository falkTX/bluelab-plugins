#include <Scale.h>

#include "GhostCommandCopyPaste.h"

GhostCommandCopyPaste::GhostCommandCopyPaste(BL_FLOAT sampleRate,
                                             int keepBorderSize)
: GhostCommand(sampleRate)
{
    mIsPasteDone = false;
    mKeepBorderSize = keepBorderSize;
}

GhostCommandCopyPaste::GhostCommandCopyPaste(const GhostCommandCopyPaste &other)
: GhostCommand(other.mSampleRate)
{
    mCopiedMagns = other.mCopiedMagns;
    mCopiedPhases = other.mCopiedPhases;
    
    for (int i = 0; i < 4; i++)
        mCopiedSelection[i] = other.mCopiedSelection[i];

    mKeepBorderSize = other.mKeepBorderSize;
    
    mIsPasteDone = false;
}

GhostCommandCopyPaste::~GhostCommandCopyPaste() {}

void
GhostCommandCopyPaste::Copy(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &phases)
{
    // Save the selection when copied
    for (int i = 0; i < 4; i++)
        mCopiedSelection[i] = mSelection[i];
    
    vector<WDL_TypedBuf<BL_FLOAT> > magnsSel;
    ExtractAux(&magnsSel, magns, mKeepBorderSize);
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesSel;
    ExtractAux(&phasesSel, phases, mKeepBorderSize);
    
    GetSelectedDataY(magnsSel, &mCopiedMagns);
    GetSelectedDataY(phasesSel, &mCopiedPhases);
}

void
GhostCommandCopyPaste::Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                             vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    // Set the selection to the pasted selection
    // in order to process only the selected area
    ComputePastedSelection();

    // Save the current selection
    BL_FLOAT saveSelection[4];
    for (int i = 0; i < 4; i++)
        saveSelection[i] = mSelection[i];
    
    for (int i = 0; i < 4; i++)
        mSelection[i] = mPastedSelection[i];
    
    // Paste
    ReplaceSelectedDataY(magns, mCopiedMagns);
    ReplaceSelectedDataY(phases, mCopiedPhases);
    
    mIsPasteDone = true;
}

void
GhostCommandCopyPaste::Undo(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                            vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    if (!mIsPasteDone)
        return;
    
    // Set the selection to the state chan pasted
    for (int i = 0; i < 4; i++)
        mSelection[i] = mPastedSelection[i];
    
    GhostCommand::Undo(magns, phases);
}

bool
GhostCommandCopyPaste::IsPasteDone()
{
    return mIsPasteDone;
}

void
GhostCommandCopyPaste::ComputePastedSelection()
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
GhostCommandCopyPaste::GetPastedSelection(BL_FLOAT pastedSelection[4],
                                          Scale::Type yScale)
{
    for (int i = 0; i < 4; i++)
        pastedSelection[i] = mPastedSelection[i];

    pastedSelection[1] = mScale->ApplyScale(yScale, pastedSelection[1],
                                            (BL_FLOAT)0.0,
                                            (BL_FLOAT)(mSampleRate*0.5));
    
    pastedSelection[3] = mScale->ApplyScale(yScale,
                                            pastedSelection[3],
                                            (BL_FLOAT)0.0,
                                            (BL_FLOAT)(mSampleRate*0.5));
}
