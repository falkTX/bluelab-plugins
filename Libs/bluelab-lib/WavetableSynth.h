//
//  WavetableSynth.h
//  BL-SASViewer
//
//  Created by applematuer on 3/1/19.
//
//

#ifndef __BL_SASViewer__WavetableSynth__
#define __BL_SASViewer__WavetableSynth__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class WavetableSynth
{
public:
    WavetableSynth(int bufferSize,
                   int overlapping,
                   BL_FLOAT sampleRate,
                   int precision,
                   BL_FLOAT minFreq);
    
    virtual ~WavetableSynth();
    
    void Reset(BL_FLOAT sampleRate);
    
    // Get a whole buffer
    void GetSamplesNearest(WDL_TypedBuf<BL_FLOAT> *buffer,
                           BL_FLOAT freq,
                           BL_FLOAT amp = 1.0);
    
    // Linear: makes wobble with pure frequencies
    // (which are not aligned to an already synthetized table)
    void GetSamplesLinear(WDL_TypedBuf<BL_FLOAT> *buffer,
                          BL_FLOAT freq,
                          BL_FLOAT amp = 1.0);
    
    // Get one sample
    BL_FLOAT GetSampleNearest(int idx, BL_FLOAT freq, BL_FLOAT amp = 1.0);
    
    // Linear: makes wobble with pure frequencies
    // (which are not aligned to an already synthetized table)
    BL_FLOAT GetSampleLinear(int idx, BL_FLOAT freq, BL_FLOAT amp = 1.0);
    
    void NextBuffer();
    
protected:
    class Table
    {
    public:
        Table() { mCurrentPos = 0.0; mFrequency = 0.0; }
        virtual ~Table() {}
        
        //
        WDL_TypedBuf<BL_FLOAT> mBuffer;
        //long mCurrentPos;
        BL_FLOAT mCurrentPos;
        
        BL_FLOAT mFrequency;
    };
    
    void ComputeTables();
    
    int mBufferSize;
    int mOverlapping;
    BL_FLOAT mSampleRate;
    int mPrecision;
    BL_FLOAT mMinFreq;
    
    vector<Table> mTables;
};

#endif /* defined(__BL_SASViewer__WavetableSynth__) */
