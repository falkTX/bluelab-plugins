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
 
#ifndef SI_SPLITTER_H
#define SI_SPLITTER_H

#include <vector>
using namespace std;

#include <BLTypes.h>

// QUESTION: log freqs? / log magns?
// IDEA: use Marquardt instead?
class SISplitter
{
public:
    SISplitter();
    virtual ~SISplitter();

    void split(const vector<BL_FLOAT> &magns,
               vector<BL_FLOAT> *sig,
               vector<BL_FLOAT> *noise);

    void setResolution(int reso);
        
protected:
    // Normalized spectral irregularity - Jensen, 1999
    BL_FLOAT computeSpectralIrreg_J(const vector<BL_FLOAT> &magns,
                                    int startBin, int endBin);
    // Spectral irregularity - Krimphoff et al., 1994
    BL_FLOAT computeSpectralIrreg_K(const vector<BL_FLOAT> &magns,
                                    int startBin, int endBin);
    
    void computeSpectralIrregWin(const vector<BL_FLOAT> &magns,
                                 vector<BL_FLOAT> *siWin,
                                 int winSize, int overlap);
    
    BL_FLOAT computeScore(const vector<BL_FLOAT> &magns,
                          BL_FLOAT refMagn,
                          int startBin, int endBin);
        
    void findMinMax(const vector<BL_FLOAT> &values,
                    int startBin, int endBin,
                    BL_FLOAT *minVal, BL_FLOAT *maxVal);

    //
    int _reso;
};

#endif
