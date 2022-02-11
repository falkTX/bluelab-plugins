//
//  AudioFile.cpp
//  BL-Ghost
//
//  Created by Pan on 31/05/18.
//
//

#include <BLTypes.h>

#include "Resampler2.h"
#include <BLUtils.h>
#include "AudioFile.h"

#define	BLOCK_SIZE 4096


AudioFile::AudioFile(int numChannels, BL_FLOAT sampleRate, int format,
                     vector<WDL_TypedBuf<BL_FLOAT> > *data)
{
    mNumChannels = numChannels;
    
    if (data != NULL)
    {
        // Improved behaviour, to avoid having the data 2 times in memory
        // The data pointer is provided from outside the function
        mData = data;
        mDataExtRef = true;
    }
    else
    {
        mData = new vector<WDL_TypedBuf<BL_FLOAT> >();
        mDataExtRef = false;
    }
    
    mData->resize(numChannels);
    
    mSampleRate = sampleRate;
    
    mInternalFormat = format;
}

AudioFile::~AudioFile()
{
    if (!mDataExtRef)
    {
        delete mData;
    }
}

// NOTE: Would crash when opening very large files
// under the debugger, and using Scheme -> Guard Edge/ Guard Malloc
AudioFile *
AudioFile::Load(const char *fileName, vector<WDL_TypedBuf<BL_FLOAT> > *data)
{
	AudioFile *result = NULL;

#if !DISABLE_LIBSNDFILE
    SF_INFO	sfinfo;
    memset(&sfinfo, 0, sizeof (sfinfo)) ;
    
    SNDFILE	*file = sf_open(fileName, SFM_READ, &sfinfo);
    if (file == NULL)
        return NULL;
    
    // Keep the loaded format, in case we would like to save to the same format
    result = new AudioFile(sfinfo.channels, (BL_FLOAT)sfinfo.samplerate, sfinfo.format, data);
    
    // Avoid duplicating data
    //vector<WDL_TypedBuf<BL_FLOAT> > channels;
    //channels.resize(sfinfo.channels);
    
    // Use directly mData, without temporary buffer
    result->mData->resize(sfinfo.channels);
    
    //if (!channels.empty())
    if (!result->mData->empty())
    {
        double buf[BLOCK_SIZE];
        sf_count_t frames = BLOCK_SIZE / sfinfo.channels;
    
        int readcount;
        while ((readcount = sf_readf_double(file, buf, frames)) > 0)
        {
            //int prevSize = channels[0].GetSize();
            int prevSize = (*result->mData)[0].GetSize();
            for (int m = 0; m < sfinfo.channels; m++)
                //channels[m].Resize(prevSize + readcount);
                (*result->mData)[m].Resize(prevSize + readcount);
        
            for (int k = 0 ; k < readcount ; k++)
            {
                for (int m = 0; m < sfinfo.channels; m++)
                {
                    double val = buf[k*sfinfo.channels + m];
                    //channels[m].Add(val); // Crashes
                
                    //channels[m].Get()[prevSize + k] = val;
                    (*result->mData)[m].Get()[prevSize + k] = val;
                }
            }
        }
    }
    
    //for (int i = 0; i < result->mData->size(); i++)
    //    (*result->mData)[i] = channels[i];
    
    sf_close(file);
#endif

    return result;
}

bool
AudioFile::Save(const char *fileName)
{
#if !DISABLE_LIBSNDFILE

#define USE_BL_FLOAT 0
	SF_INFO	sfinfo;
    
    memset(&sfinfo, 0, sizeof (sfinfo));
    
	sfinfo.samplerate = (int)mSampleRate;
	sfinfo.frames = (mData->size() > 0) ? (*mData)[0].GetSize() : 0;
	sfinfo.channels	= mNumChannels;
    
    char *ext = BLUtils::GetFileExtension(fileName);
    if (ext == NULL)
        ext = "wav";
    
    // Set the format to a default format
    
#if USE_BL_FLOAT // For 64 bit (not supported everywhere)
    if (strcmp(ext, "wav") == 0)
        sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_BL_FLOAT/*SF_FORMAT_PCM_24*/);
    
    if ((strcmp(ext, "aif") == 0) || (strcmp(ext, "aiff") == 0))
        sfinfo.format = (SF_FORMAT_AIFF | SF_FORMAT_BL_FLOAT/*SF_FORMAT_PCM_24*/);
    
#if AUDIOFILE_USE_FLAC
    if (strcmp(ext, "flac") == 0)
        sfinfo.format = (SF_FORMAT_FLAC | SF_FORMAT_BL_FLOAT/*SF_FORMAT_PCM_24*/);
#endif
    
#else // More standard bit depth
    if (strcmp(ext, "wav") == 0)
        sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT/*SF_FORMAT_PCM_24*/);
    
    if ((strcmp(ext, "aif") == 0) || (strcmp(ext, "aiff") == 0))
        sfinfo.format = (SF_FORMAT_AIFF | SF_FORMAT_FLOAT/*SF_FORMAT_PCM_24*/);
    
#if AUDIOFILE_USE_FLAC
    if (strcmp(ext, "flac") == 0)
        //sfinfo.format = (SF_FORMAT_FLAC | SF_FORMAT_FLOAT/*SF_FORMAT_PCM_24*/);
        sfinfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_24);
#endif
    
#endif
    
    if (mInternalFormat != -1)
        // Internal format is defined, overwrite the default with it
        sfinfo.format = mInternalFormat;
    
    SNDFILE *file = sf_open(fileName, SFM_WRITE, &sfinfo);
    if (file == NULL)
        return false;
    
    int numData = (mData->size() > 0) ? mNumChannels*(*mData)[0].GetSize() : 0;
    
#if USE_BL_FLOAT
    BL_FLOAT *buffer = (BL_FLOAT *)malloc(numData*sizeof(BL_FLOAT));
#else
    float *buffer = (float *)malloc(numData*sizeof(float));
#endif
    
    if (mData->size() > 0)
    {
        for (int j = 0; j < (*mData)[0].GetSize(); j++)
        {
            for (int i = 0; i < mNumChannels; i++)
            {
                buffer[j*mNumChannels + i] = (*mData)[i].Get()[j];
            }
        }
    
#if USE_BL_FLOAT
        sf_write_BL_FLOAT(file, buffer, numData);
#else
        sf_write_float(file, buffer, numData);
#endif
        
    }
    
    sf_close(file);
                  
    free(buffer);
#endif

    return true;
}

void
AudioFile::Resample(BL_FLOAT newSampleRate)
{
    if (newSampleRate == mSampleRate)
    {
        // Nothing to do
        // And will save having the audio buffer 2 times in memory
        return;
    }
    
    for (int i = 0; i < mNumChannels; i++)
    {
        WDL_TypedBuf<BL_FLOAT> &chan = (*mData)[i];
        WDL_TypedBuf<BL_FLOAT> resamp;
        
        Resampler2::Resample(&chan,
                             &resamp,
                             mSampleRate, newSampleRate);
        
        chan = resamp;
    }

    mSampleRate = newSampleRate;
}

void
AudioFile::Resample2(BL_FLOAT newSampleRate)
{
    if (newSampleRate == mSampleRate)
    {
        // Nothing to do
        // And will save having the audio buffer 2 times in memory
        return;
    }
    
    for (int i = 0; i < mNumChannels; i++)
    {
        WDL_TypedBuf<BL_FLOAT> &chan = (*mData)[i];
        WDL_TypedBuf<BL_FLOAT> resamp;
        
        Resampler2::Resample2(&chan,
                              &resamp,
                              mSampleRate, newSampleRate);
        
        chan = resamp;
    }
    
    mSampleRate = newSampleRate;
}

int
AudioFile::GetNumChannels()
{
    return mNumChannels;
}

BL_FLOAT
AudioFile::GetSampleRate()
{
    return mSampleRate;
}

int
AudioFile::GetInternalFormat()
{
    return mInternalFormat;
}

void
AudioFile::GetData(int channelNum, WDL_TypedBuf<BL_FLOAT> **data)
{
    if (channelNum >= mData->size())
        return;
    
    *data = &((*mData)[channelNum]);
}

void
AudioFile::SetData(int channelNum, const WDL_TypedBuf<BL_FLOAT> &data, long dataSize)
{
    if (channelNum >= mData->size())
        return;
    
    (*mData)[channelNum] = data;
    
    // dataSize provided: we don't want to save all the data
    if (dataSize > 0)
        BLUtils::ResizeFillZeros(&(*mData)[channelNum], dataSize);
}

