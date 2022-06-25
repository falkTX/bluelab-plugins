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
//  AudioFile.h
//  BL-Ghost
//
//  Created by Pan on 31/05/18.
//
//

#ifndef __BL_Ghost__AudioFile__
#define __BL_Ghost__AudioFile__

#include <vector>
using namespace std;

// Disable libsndfile ?
#define DISABLE_LIBSNDFILE 0

#if !DISABLE_LIBSNDFILE
#include <sndfile.h>
#endif

#include <BLTypes.h>

#define AUDIOFILE_USE_FLAC 1

#include "IPlug_include_in_plug_hdr.h"

class AudioFile
{
public:
    // Format: internal snd file format
    // Set to -1 for default
    //
    // Possibility to provide the data buffer
    // This will avoid duplicate data when loading
    // (load in a buffer, then get the data in a second buffer = 2x the data in memory
    // for a while)
    AudioFile(int numChannels, BL_FLOAT sampleRate, int internalFormat = -1,
              vector<WDL_TypedBuf<BL_FLOAT> > *data = NULL);
    
    virtual ~AudioFile();
    
    // NOTE: Would crash when opening very large files
    // under the debugger, and using Scheme -> Guard Edge/ Guard Malloc
    static AudioFile *Load(const char *fileName, vector<WDL_TypedBuf<BL_FLOAT> > *data = NULL);
    
    bool Save(const char *fileName);
    
    void Resample(BL_FLOAT newSampleRate);
    
    // Do not fill with zeros of we don't get exactly the expected number of data
    void Resample2(BL_FLOAT newSampleRate);
    
    int GetNumChannels();
    BL_FLOAT GetSampleRate();
    
    int GetInternalFormat();
    
    void GetData(int channelNum, WDL_TypedBuf<BL_FLOAT> **data);
    
    void SetData(int channelNum, const WDL_TypedBuf<BL_FLOAT> &data,
                 long dataSize = -1);

    // Rescale is some samples are > 1.0 or < -1.0
    void FixDataBounds();
    
protected:
    int mNumChannels;
    BL_FLOAT mSampleRate;
    
    int mInternalFormat;
    
    // Data pointer provided externally ?
    bool mDataExtRef;
    
    vector<WDL_TypedBuf<BL_FLOAT> > *mData;
};

#endif /* defined(__BL_Ghost__AudioFile__) */
