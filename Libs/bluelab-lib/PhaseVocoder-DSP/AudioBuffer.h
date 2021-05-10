#pragma once

// #bluelab
template <typename DataType> class AudioBuffer
{
 public:
    AudioBuffer(int numChans, int bufferSize)
    {
        setSize(numChans, bufferSize);
    }
    
    virtual ~AudioBuffer()
    {
        reset();
    }
    
    void setSize(int numChans, int bufferSize)
    {
        mChannels.resize(numChans);
        
        for (int k = 0; k < numChans; k++)
            mChannels[i] = realloc(mChannels[i], bufferSize*sizeof(DataType));

        mNumSamples = bufferSize;
    }

    void clear()
    {
        for (int k = 0; k < numChans; k++)
            memset(mChannels[i], 0, mNumSamples*sizeof(DataType));
    }

    DataType *getWritePointer(int chanNum)
    {
        if (chanNum < mChannels.size())
            return mChannels[chanNum];

        return NULL;
    }

    int getNumSamples()
    {
        return mNumSamples;
    }
        
 protected:
    void reset()
    {
        for (int k = 0; k < numChans; k++)
            free(mChannels[i]);
        mChannels.clear();

        mNumSamples = 0;
    }
    
    vector<DataType *> mChannels;
    int mNumSamples;
};
