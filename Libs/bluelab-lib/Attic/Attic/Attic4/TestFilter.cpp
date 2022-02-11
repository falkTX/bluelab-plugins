//
//  TestFilter.cpp
//  UST
//
//  Created by applematuer on 8/26/19.
//
//

#include <FftProcessObj16.h>

#include <BLUtils.h>
#include <BLDebug.h>

// TEST 0
#include <CrossoverSplitterNBands2.h>

// TEST 2
#include <CrossoverSplitter3Bands.h>

// TEST 3
#include <CrossoverSplitterNBands.h>

// TEST 5
#include <CrossoverSplitterNBands3.h>

// TEST 6
#include <FilterRBJNX.h>

#include "TestFilter.h"


#define PLAY_TEST0 0
#define PLAY_TEST1 0
#define PLAY_TEST2 0
#define PLAY_TEST3 0
#define PLAY_TEST4 1
#define PLAY_TEST5 0
#define PLAY_TEST6 0

#define DUMP_DATA 1


TestFilter::TestFilter(BL_FLOAT sampleRate)
{
    FftProcessObj16::Init();
    
    mSampleRate = sampleRate;
    
#if PLAY_TEST0
    InitTest0();
#endif
    
#if PLAY_TEST1
    InitTest1();
#endif
    
#if PLAY_TEST2
    InitTest2();
#endif
    
#if PLAY_TEST3
    InitTest3();
#endif

#if PLAY_TEST4
    InitTest4();
#endif
    
#if PLAY_TEST5
    InitTest5();
#endif

#if PLAY_TEST6
    InitTest6();
#endif
}

TestFilter::~TestFilter()
{
#if PLAY_TEST0
    ShutdownTest0();
#endif
    
#if PLAY_TEST1
    ShutdownTest1();
#endif
    
#if PLAY_TEST2
    ShutdownTest2();
#endif
    
#if PLAY_TEST3
    ShutdownTest3();
#endif
    
#if PLAY_TEST4
    ShutdownTest4();
#endif
    
#if PLAY_TEST5
    ShutdownTest5();
#endif
    
#if PLAY_TEST6
    ShutdownTest6();
#endif
}

void
TestFilter::Test0()
{
#define BUFFER_SIZE 1024
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(BUFFER_SIZE*2);
    
    GenImpulse(&impulse);
    
    WDL_TypedBuf<BL_FLOAT> bands[3];
    mTest0Splitter->Split(impulse, bands);

    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(bands[0], &band0);
    
    WDL_TypedBuf<BL_FLOAT> band1;
    GetFft(bands[1], &band1);
    
    WDL_TypedBuf<BL_FLOAT> band2;
    GetFft(bands[2], &band2);
    
#if DUMP_DATA
    BLDebug::DumpData("band0.txt", band0);
    BLDebug::DumpData("band1.txt", band1);
    BLDebug::DumpData("band2.txt", band2);
#endif
}

void
TestFilter::Test1()
{
    WDL_TypedBuf<BL_FLOAT> whiteNoise;
    whiteNoise.Resize(BUFFER_SIZE*2);
    
    BLUtils::GenNoise(&whiteNoise);
    BLUtils::MultValues(&whiteNoise, (BL_FLOAT)2.0);
    BLUtils::AddValues(&whiteNoise, (BL_FLOAT)-1.0);
    
    WDL_TypedBuf<BL_FLOAT> bands[3];
    mTest1Splitter->Split(whiteNoise, bands);
    
    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(bands[0], &band0);
    
    WDL_TypedBuf<BL_FLOAT> band1;
    GetFft(bands[1], &band1);
    
    WDL_TypedBuf<BL_FLOAT> band2;
    GetFft(bands[2], &band2);
    
    WDL_TypedBuf<BL_FLOAT> noiseFft;
    GetFft(whiteNoise, &noiseFft);
    
#if DUMP_DATA
    BLDebug::DumpData("noise.txt", noiseFft);
    
    BLDebug::DumpData("band0.txt", band0);
    BLDebug::DumpData("band1.txt", band1);
    BLDebug::DumpData("band2.txt", band2);
#endif
}

void
TestFilter::Test2()
{
#define BUFFER_SIZE 1024
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(BUFFER_SIZE*2);
    
    GenImpulse(&impulse);
    
    WDL_TypedBuf<BL_FLOAT> bands[3];
    mTest2Splitter->Split(impulse, bands);
    
    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(bands[0], &band0);
    
    WDL_TypedBuf<BL_FLOAT> band1;
    GetFft(bands[1], &band1);
    
    WDL_TypedBuf<BL_FLOAT> band2;
    GetFft(bands[2], &band2);
    
#if DUMP_DATA
    BLDebug::DumpData("band0.txt", band0);
    BLDebug::DumpData("band1.txt", band1);
    BLDebug::DumpData("band2.txt", band2);
#endif
}

void
TestFilter::Test3()
{
#define BUFFER_SIZE 1024
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(BUFFER_SIZE*2);
    
    GenImpulse(&impulse);
    
    WDL_TypedBuf<BL_FLOAT> bands[3];
    mTest3Splitter->Split(impulse, bands);
    
    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(bands[0], &band0);
    
    WDL_TypedBuf<BL_FLOAT> band1;
    GetFft(bands[1], &band1);
    
    WDL_TypedBuf<BL_FLOAT> band2;
    GetFft(bands[2], &band2);
    
#if DUMP_DATA
    BLDebug::DumpData("band0.txt", band0);
    BLDebug::DumpData("band1.txt", band1);
    BLDebug::DumpData("band2.txt", band2);
#endif
}

void
TestFilter::Test4()
{
#define BUFFER_SIZE 1024
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(BUFFER_SIZE*2);
    
    GenImpulse(&impulse);
    
    WDL_TypedBuf<BL_FLOAT> bands[5];
    mTest4Splitter->Split(impulse, bands);
    
    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(bands[0], &band0);
    
    WDL_TypedBuf<BL_FLOAT> band1;
    GetFft(bands[1], &band1);
    
    WDL_TypedBuf<BL_FLOAT> band2;
    GetFft(bands[2], &band2);
    
    WDL_TypedBuf<BL_FLOAT> band3;
    GetFft(bands[3], &band3);
    
    WDL_TypedBuf<BL_FLOAT> band4;
    GetFft(bands[4], &band4);
    
#if DUMP_DATA
    BLDebug::DumpData("band0.txt", band0);
    BLDebug::DumpData("band1.txt", band1);
    BLDebug::DumpData("band2.txt", band2);
    BLDebug::DumpData("band3.txt", band3);
    BLDebug::DumpData("band4.txt", band4);
#endif
}

void
TestFilter::Test5()
{
#define BUFFER_SIZE 1024
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(BUFFER_SIZE*2);
    
    GenImpulse(&impulse);
    
    WDL_TypedBuf<BL_FLOAT> bands[5];
    mTest5Splitter->Split(impulse, bands);
    
    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(bands[0], &band0);
    
    WDL_TypedBuf<BL_FLOAT> band1;
    GetFft(bands[1], &band1);
    
#if DUMP_DATA
    BLDebug::DumpData("band0.txt", band0);
    BLDebug::DumpData("band1.txt", band1);
#endif
}

void
TestFilter::Test6()
{
#define BUFFER_SIZE 1024
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(BUFFER_SIZE*2);
    
    GenImpulse(&impulse);
    
    //WDL_TypedBuf<BL_FLOAT> band;
    //mTest6Filter->Process(&band, impulse);
    WDL_TypedBuf<BL_FLOAT> band = impulse;
    mTest6Filter->Process(&band);
    
    WDL_TypedBuf<BL_FLOAT> band0;
    GetFft(band, &band0);
    
#if DUMP_DATA
    BLDebug::DumpData("band0.txt", band0);
#endif
}

void
TestFilter::InitTest0()
{
    BL_FLOAT cutoffFreqs[2] = {  8192.0, 16384.0 };
    mTest0Splitter = new CrossoverSplitterNBands2(3, cutoffFreqs, mSampleRate);
}

void
TestFilter::ShutdownTest0()
{
    delete mTest0Splitter;
}

void
TestFilter::InitTest1()
{
    BL_FLOAT cutoffFreqs[2] = {  8192.0, 16384.0 };
    mTest1Splitter = new CrossoverSplitterNBands2(3, cutoffFreqs, mSampleRate);
}

void
TestFilter::ShutdownTest1()
{
    delete mTest1Splitter;
}

void
TestFilter::InitTest2()
{
    BL_FLOAT cutoffFreqs[2] = {  8192.0, 16384.0 };
    mTest2Splitter = new CrossoverSplitter3Bands(cutoffFreqs, mSampleRate);
}

void
TestFilter::ShutdownTest2()
{
    delete mTest2Splitter;
}

void
TestFilter::InitTest3()
{
    BL_FLOAT cutoffFreqs[2] = {  8192.0, 16384.0 };
    mTest3Splitter = new CrossoverSplitterNBands(3, cutoffFreqs, mSampleRate);
}

void
TestFilter::ShutdownTest3()
{
    delete mTest3Splitter;
}

void
TestFilter::InitTest4()
{
    BL_FLOAT cutoffFreqs[4] = {  90.0, 500.0, 2000.0, 10000.0 };
    mTest4Splitter = new CrossoverSplitterNBands3(5, cutoffFreqs, mSampleRate);
}

void
TestFilter::ShutdownTest4()
{
    delete mTest4Splitter;
}

void
TestFilter::InitTest5()
{
    // NOTE: think to put the gain > 0 in FilterRBJNX
    BL_FLOAT cutoffFreqs[1] = {  5000.0 };
    mTest5Splitter = new CrossoverSplitterNBands3(2, cutoffFreqs, mSampleRate);
}

void
TestFilter::ShutdownTest5()
{
    delete mTest5Splitter;
}

void
TestFilter::InitTest6()
{
    // NOTE: think to put the gain > 0 in FilterRBJNX
    mTest6Filter = new FilterRBJNX(1,
                                  //FILTER_TYPE_LOWPASS,
                                  FILTER_TYPE_LOWSHELF,
                                  mSampleRate, 5000.0);
}

void
TestFilter::ShutdownTest6()
{
    delete mTest6Filter;
}

void
TestFilter::GenImpulse(WDL_TypedBuf<BL_FLOAT> *impulse)
{
    if (impulse->GetSize() == 0)
        return;
    
    BLUtils::FillAllZero(impulse);
    impulse->Get()[0] = 1.0;
}

void
TestFilter::GetFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                   WDL_TypedBuf<BL_FLOAT> *fft)
{
    WDL_TypedBuf<BL_FLOAT> phases;
    FftProcessObj16::SamplesToHalfMagnPhases(samples, fft, &phases);
}
