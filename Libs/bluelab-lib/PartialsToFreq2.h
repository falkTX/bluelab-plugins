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
 
//
//  PartialsToFreq2.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsToFreq2__
#define __BL_SASViewer__PartialsToFreq2__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

// PartialsToFreq: original code, moved from SASFrame to here
// PartialsToFreq2: improved algorithm
//
class PartialTWMEstimate2;
class PartialsToFreq2
{
public:
    PartialsToFreq2(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialsToFreq2();
    
    void Reset(BL_FLOAT sampleRate);
    
    BL_FLOAT ComputeFrequency(const vector<PartialTracker3::Partial> &partials);
    
protected:
    void GetBestPartials(vector<PartialTracker3::Partial> *partials);
    
    void ComputeAgeScores(const vector<PartialTracker3::Partial> &partials,
                          vector<BL_FLOAT> *ageScores);

    void ComputeAmpScores(const vector<PartialTracker3::Partial> &partials,
                          vector<BL_FLOAT> *ampScores);
    
    void ComputeFreqScores(const vector<PartialTracker3::Partial> &partials,
                           vector<BL_FLOAT> *freqScores);
    
    //
    void ThresholdPartials(vector<PartialTracker3::Partial> *partials);

    //
    void ThresholdPartialsRelative(vector<PartialTracker3::Partial> *partials);
    
    //
    void SelectAlivePartials(vector<PartialTracker3::Partial> *partials);

    
    PartialTWMEstimate2 *mFreqEstimate;
};

#endif /* defined(__BL_SASViewer__PartialsToFreq2__) */
