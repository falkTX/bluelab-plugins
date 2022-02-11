//
//  USTUpmix.h
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#ifndef __UST__USTUpmix__
#define __UST__USTUpmix__

#define SPLITTER_N_BANDS 1

class GraphControl11;
class USTUpmixGraphDrawer;
class CrossoverSplitter2Bands;
class DelayObj4;
class UST;
class CrossoverSplitter3Bands;
//class CrossoverSplitterNBands;
class CrossoverSplitterNBands3;

class revmodel;

class USTUpmix
{
public:
    USTUpmix(BL_FLOAT sampleRate);
    
    virtual ~USTUpmix();
    
    void Reset(BL_FLOAT sampleRate);

    
    void SetEnabled(bool flag);
    
    int GetNumCurves();
    
    int GetNumPoints();
    
    void SetGraph(UST *plug, GraphControl11 *graph);
    
    //
    void SetGain(BL_FLOAT gain);
    void SetPan(BL_FLOAT pan);
    void SetDepth(BL_FLOAT depth);
    void SetBrillance(BL_FLOAT brillance);
    
    //
    
    // Cooking to try to have the same result as Waves
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
    // Simple version, to test Depth
    void ProcessSimple(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
    void ProcessSimpleStereo(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
protected:
    void SplitDiffuse(const WDL_TypedBuf<BL_FLOAT> &samples,
                      WDL_TypedBuf<BL_FLOAT> *defined,
                      WDL_TypedBuf<BL_FLOAT> *diffuse);

    
    void BoostDiffuse(WDL_TypedBuf<BL_FLOAT> *diffuse, BL_FLOAT normGain);

    void GenerateEarlyReflect(const WDL_TypedBuf<BL_FLOAT> &samples,
                              WDL_TypedBuf<BL_FLOAT> outRevSamples[2],
                              BL_FLOAT normDepth);

    void GenerateEarlyReflect2(const WDL_TypedBuf<BL_FLOAT> &samples,
                               WDL_TypedBuf<BL_FLOAT> outRevSamples[2],
                               BL_FLOAT normDepth);
    
    void ApplyBrillance(WDL_TypedBuf<BL_FLOAT> *ioSamples);

    void ApplyBrillanceStereo(int chan, WDL_TypedBuf<BL_FLOAT> *ioSamples);
    
    void InitRevModel();
    
    
    bool mIsEnabled;
    
    GraphControl11 *mGraph;
    
    USTUpmixGraphDrawer *mUpmixDrawer;
    
    BL_FLOAT mGain;
    BL_FLOAT mPan;
    BL_FLOAT mDepth;
    BL_FLOAT mBrillance;

#if !SPLITTER_N_BANDS
    CrossoverSplitter2Bands *mBassSplitter;
#else
    CrossoverSplitterNBands3 *mBassSplitter;
    
    // For stereo
    CrossoverSplitterNBands3 *mBassSplitters[2];
#endif
    
    DelayObj4 *mSplitSDelay;
    DelayObj4 *mEarlyReflectDelays[4];
    
    DelayObj4 *mCompDelay;
    DelayObj4 *mCompDelays2[3];
    
#if !SPLITTER_N_BANDS
    CrossoverSplitter3Bands *mBrillanceSplitter;
#else
    //CrossoverSplitterNBands *mBrillanceSplitter;
    CrossoverSplitterNBands3 *mBrillanceSplitter;
    
    // For stereo
    CrossoverSplitterNBands3 *mBrillanceSplitters[2];
#endif
    
    BL_FLOAT mSampleRate;
    
    revmodel *mRevModel;
};


#endif /* defined(__UST__USTUpmix__) */
