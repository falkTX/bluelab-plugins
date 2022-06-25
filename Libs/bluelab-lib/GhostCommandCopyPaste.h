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
 
#ifndef GHOST_COMMAND_COPY_PASTE_H
#define GHOST_COMMAND_COPY_PASTE_H

#include <GhostCommand.h>

#include "IPlug_include_in_plug_hdr.h"

class GhostCommandCopyPaste : public GhostCommand
{
public:
    GhostCommandCopyPaste(BL_FLOAT sampleRate);
   
    // To manage on copy then multiple pastes
    GhostCommandCopyPaste(const GhostCommandCopyPaste &other);
    
    virtual ~GhostCommandCopyPaste();
    
    void Copy(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
              const vector<WDL_TypedBuf<BL_FLOAT> > &phases,
              int srcTrackNumSamples);
    
    // Apply is paste
    void Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
               vector<WDL_TypedBuf<BL_FLOAT> > *phases) override;
    
    void Undo(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
              vector<WDL_TypedBuf<BL_FLOAT> > *phases) override;
    
    bool IsPasteDone();
    
    void ComputePastedSelection();
    
    void GetPastedSelection(BL_FLOAT pastedSelection[4],
                            Scale::Type yScale);

    void SetDstTrackNumSamples(int numSamples);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mCopiedMagns;
    WDL_TypedBuf<BL_FLOAT> mCopiedPhases;
    
    BL_FLOAT mCopiedSelection[4];
    BL_FLOAT mPastedSelection[4];
    
    bool mIsPasteDone;

    // For managing "copy from one track, paste to another"
    int mSrcTrackNumSamples;
    int mDstTrackNumSamples;
};

#endif
