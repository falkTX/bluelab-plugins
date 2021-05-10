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
        clear();
    }
    
    void setSize(int numChans, int bufferSize)
    {
        clear();
        
        mChannels.resize(numChans);
        
        for (int k = 0; k < numChans; k++)
            mChannels[i] = malloc(bufferSize*sizeof(DataType));

        mNumSamples = bufferSize;
    }

    void clear()
    {
        for (int k = 0; k < numChans; k++)
            free(mChannels[i]);
        mChannels.clear();

        mNumSamples = 0;
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
        
    protected;      
    vector<DataType *> mChannels;
    int mNumSamples;
};
