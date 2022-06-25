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
 
#include <Scale.h>

#include "GhostCommandCopyPaste.h"

GhostCommandCopyPaste::GhostCommandCopyPaste(BL_FLOAT sampleRate)
: GhostCommand(sampleRate)
{
    mIsPasteDone = false;

    for (int i = 0; i < 4; i++)
        mCopiedSelection[i] = 0.0;

    for (int i = 0; i < 4; i++)
        mPastedSelection[i] = 0.0;
    
    mSrcTrackNumSamples = -1;
    mDstTrackNumSamples = -1;
}

GhostCommandCopyPaste::GhostCommandCopyPaste(const GhostCommandCopyPaste &other)
: GhostCommand(other.mSampleRate)
{
    mCopiedMagns = other.mCopiedMagns;
    mCopiedPhases = other.mCopiedPhases;
    
    for (int i = 0; i < 4; i++)
        mCopiedSelection[i] = other.mCopiedSelection[i];

    for (int i = 0; i < 4; i++)
        mPastedSelection[i] = other.mPastedSelection[i];
    
    mSrcTrackNumSamples = other.mSrcTrackNumSamples;
    mDstTrackNumSamples = other.mDstTrackNumSamples;
    
    mIsPasteDone = false;
}

GhostCommandCopyPaste::~GhostCommandCopyPaste() {}

void
GhostCommandCopyPaste::Copy(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &phases,
                            int srcTrackNumSamples)
{
    mSrcTrackNumSamples = srcTrackNumSamples;
    
    // Save the selection when copied
    for (int i = 0; i < 4; i++)
        mCopiedSelection[i] = mSelection[i];
    
    vector<WDL_TypedBuf<BL_FLOAT> > magnsSel;
    ExtractAux(&magnsSel, magns);
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesSel;
    ExtractAux(&phasesSel, phases);
    
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

    if ((mSrcTrackNumSamples > 0) && (mDstTrackNumSamples > 0) &&
        (mSrcTrackNumSamples != mDstTrackNumSamples))
    {
        //copySelectWidth =
        //    (copySelectWidth*mSrcTrackNumSamples)/mDstTrackNumSamples;
        copySelectWidth =
            copySelectWidth*(((BL_FLOAT)mSrcTrackNumSamples)/mDstTrackNumSamples);
    }
    
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

void
GhostCommandCopyPaste::SetDstTrackNumSamples(int numSamples)
{
    mDstTrackNumSamples = numSamples;
}
