//
//  USTMultiBandDisplay.h
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#ifndef __UST__USTMultiBandDisplay__
#define __UST__USTMultiBandDisplay__

class GraphControl11;
class FilterFreqResp;

#define SPLITTER_N_BANDS 1 //0

#if !SPLITTER_N_BANDS
class CrossoverSplitter5Bands;
#else
class CrossoverSplitterNBands4;
#endif

class USTMultiBandDisplay
{
public:
#if !SPLITTER_N_BANDS
    USTMultiBandDisplay(CrossoverSplitter5Bands *splitter,
                        BL_FLOAT sampleRate);
#else
    USTMultiBandDisplay(CrossoverSplitterNBands4 *splitter,
                        BL_FLOAT sampleRate);
#endif
    
    virtual ~USTMultiBandDisplay();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetGraph(GraphControl11 *graph);
    
    void Update(int filterNum);
    
protected:
    GraphControl11 *mGraph;
    
#if !SPLITTER_N_BANDS
    CrossoverSplitter5Bands *mSplitter;
#else
    CrossoverSplitterNBands4 *mSplitter;
#endif
    
    FilterFreqResp *mFilterResp;
    
    BL_FLOAT mSampleRate;
};

#endif /* defined(__UST__USTMultiBandDisplay__) */
