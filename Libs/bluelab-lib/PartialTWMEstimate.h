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
//  PartialTWMEstimate.h
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#ifndef __BL_SASViewer__PartialTWMEstimate__
#define __BL_SASViewer__PartialTWMEstimate__

#include <vector>
using namespace std;

#include <PartialTracker3.h>


// Estimate fundamental frequency from series of partials (more or less harmonic)
// (works well for artifacts and reverb)
//
// See: https://pdfs.semanticscholar.org/c94b/56f21f32b3b7a9575ced317e3de9b2ad9081.pdf
//
// and: https://pdfs.semanticscholar.org/5c39/c1a6c13b8ee7a4947d150267b803e6aa5ab6.pdf
//
class PartialTWMEstimate
{
public:
    PartialTWMEstimate(BL_FLOAT sampleRate);
    
    virtual ~PartialTWMEstimate();
    
    // Estimate the fundamental frequency from a series of partials
    BL_FLOAT Estimate(const vector<PartialTracker3::Partial> &partials);
    
    // Same as above, but optimized: start with rought precision then
    // increase around the interesting frequency
    BL_FLOAT EstimateMultiRes(const vector<PartialTracker3::Partial> &partials);
    
    // See optimization in (4.2)
    // here: https://pdfs.semanticscholar.org/5c39/c1a6c13b8ee7a4947d150267b803e6aa5ab6.pdf
    BL_FLOAT EstimateOptim(const vector<PartialTracker3::Partial> &partials);
    
    
    // Fix big freqs jumps (TEST)
    BL_FLOAT FixFreqJumps(BL_FLOAT freq0, BL_FLOAT prevFreq);
    
    // Gest nearest hamonic from input freq, comared to reference freq
    BL_FLOAT GetNearestOctave(BL_FLOAT freq, BL_FLOAT refFreq);
    
    // Gest nearest partial from input freq, comared to reference freq
    BL_FLOAT GetNearestHarmonic(BL_FLOAT freq, BL_FLOAT refFreq);
    
protected:
    BL_FLOAT Estimate(const vector<PartialTracker3::Partial> &partials,
                    BL_FLOAT freqAccuracy,
                    BL_FLOAT minFreq, BL_FLOAT maxFreq);
    
    BL_FLOAT ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
                           BL_FLOAT testFreq, BL_FLOAT maxFreq, BL_FLOAT AmaxInv);
    
    BL_FLOAT ComputeErrorK(const PartialTracker3::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT AmaxInv);
    
    BL_FLOAT ComputeErrorN(const PartialTracker3::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT AmaxInv);
    

    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SASViewer__PartialTWMEstimate__) */
