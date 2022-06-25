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
 
#include <BLUtils.h>

#include "GhostCommandCut.h"

GhostCommandCut::GhostCommandCut(BL_FLOAT sampleRate)
: GhostCommand(sampleRate) {}

GhostCommandCut::~GhostCommandCut() {}

void
GhostCommandCut::Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                       vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    // NOTE: no need to reset phases
    
    WDL_TypedBuf<BL_FLOAT> selectedMagns;
    
    // Get the selected data, just for convenience
    GetSelectedDataY(*magns, &selectedMagns);
    
    // For the moment, do not use fade, just fill all with zeros
    BLUtils::FillAllZero(&selectedMagns);
    
    // And replace in the result
    ReplaceSelectedDataY(magns, selectedMagns);

    // Phases
    // Reset phases also, to avoid some small oscillating values
    /*WDL_TypedBuf<BL_FLOAT> selectedPhases;
      GetSelectedDataY(*phases, &selectedPhases);
      BLUtils::FillAllZero(&selectedPhases);
      ReplaceSelectedDataY(phases, selectedPhases);*/
}
