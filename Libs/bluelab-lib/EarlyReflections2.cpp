//
//  EarlyReflections2.cpp
//  BL-ReverbDepth
//
//  Created by applematuer on 9/1/20.
//
//

#include <DelayObj4.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "EarlyReflections2.h"

// For early reflections reverb (in meters)
#define DEFAULT_ROOM_SIZE 10.0
#define DEFAULT_INTERMIC_DIST 0.1

#define SOUND_SPEED 343.0
#define DEPTH_COEFF 10.0 //2.0

#define DEFAULT_DELAY 100

#if 0
TODO: add new parameters to knob (order, material absorption)
TODO: rotate/un-center sources, to avoid symetry
OPTIM: for the moment, with order 2, we have 2x12 delays
=> to avoid this, try to remove the nearest wall for example
OPTIM: read the "optimization" section of the article
#endif

//
EarlyReflections2::Ray::Ray()
{
    mDelay = new DelayObj4(1);

    mPoint[0] = 0.0;
    mPoint[1] = 0.0;
    
    mAttenCoeff = 1.0;
}

EarlyReflections2::Ray::Ray(const Ray &other)
{
    mDelay = new DelayObj4(*other.mDelay);
    
    mAttenCoeff = other.mAttenCoeff;
    
    mPoint[0] = other.mPoint[0];
    mPoint[1] = other.mPoint[1];
}

EarlyReflections2::Ray::~Ray()
{
    delete mDelay;
}

//
EarlyReflections2::EarlyReflections2(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mOrder = 2;
    
    mRoomSize = DEFAULT_ROOM_SIZE;
    mIntermicDist = DEFAULT_INTERMIC_DIST;
    mNormDepth = 1.0;
    mReflectCoeff = 1.0;
    
    Init();
}

EarlyReflections2::EarlyReflections2(const EarlyReflections2 &other)
{
    mSampleRate = other.mSampleRate;
    
    mOrder = other.mOrder;
    
    mRoomSize = other.mRoomSize;
    mIntermicDist = other.mIntermicDist;
    mNormDepth = other.mNormDepth;
    mReflectCoeff = other.mReflectCoeff;
    
    // mDelays
    
    Init();
}

EarlyReflections2::~EarlyReflections2() {}

void
EarlyReflections2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < mRays[i].size(); j++)
            mRays[i][j].mDelay->Reset();
    }
}

void
EarlyReflections2::Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                          WDL_TypedBuf<BL_FLOAT> outRevSamples[2])
{
    for (int i = 0; i < 2; i++)
    {
        outRevSamples[i].Resize(samples.GetSize());
        BLUtils::FillAllZero(&outRevSamples[i]);
    }
    
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < mRays[i].size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> revSamples = samples;
            mRays[i][j].mDelay->ProcessSamples(&revSamples);
            
            BLUtils::MultValues(&revSamples, mRays[i][j].mAttenCoeff);
            BLUtils::AddValues(&outRevSamples[i], revSamples);
        }
    }
    
    // Ponderate with depth
    for (int i = 0; i < 2; i++)
        BLUtils::MultValues(&outRevSamples[i], mNormDepth*(BL_FLOAT)DEPTH_COEFF);
}

void
EarlyReflections2::Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                          WDL_TypedBuf<BL_FLOAT> *outRevSamplesL,
                          WDL_TypedBuf<BL_FLOAT> *outRevSamplesR)
{
    WDL_TypedBuf<BL_FLOAT> outRevSamples[2];
    Process(samples, outRevSamples);
    
    *outRevSamplesL = outRevSamples[0];
    *outRevSamplesR = outRevSamples[1];
}

void
EarlyReflections2::SetOrder(int order)
{
    mOrder = order;
    
    Init();
}

void
EarlyReflections2::SetRoomSize(BL_FLOAT roomSize)
{
    mRoomSize = roomSize;
    
    Init();
}

void
EarlyReflections2::SetIntermicDist(BL_FLOAT dist)
{
    mIntermicDist = dist;
    
    Init();
}

void
EarlyReflections2::SetNormDepth(BL_FLOAT depth)
{
    mNormDepth = depth;
}

void
EarlyReflections2::SetReflectCoeff(BL_FLOAT coeff)
{
    mReflectCoeff = coeff;
    
    Init();
}

// May have mistakes (see the picture in UST/Images)
void
EarlyReflections2::Init()
{
    for (int i = 0; i < 2; i++)
        mRays[i].clear();
    
    // Room corners
    BL_FLOAT roomCorners[4][2] = {  { 0.0, 0.0 },
                                    { mRoomSize, 0.0 },
                                    { mRoomSize, mRoomSize },
                                    { 0.0, mRoomSize }
                                 };
    
    // Source coordinate
    BL_FLOAT S[2] = { (BL_FLOAT)(mRoomSize/2.0), (BL_FLOAT)(mRoomSize/2.0) };
    
    // Mics coordinates
    
    // Good to offset a little, to avoid symetries
#define MIC_OFFSET_LX mRoomSize*0.01
#define MIC_OFFSET_RX -mRoomSize*0.02
#define MIC_Y mRoomSize/4.0

    BL_FLOAT micL[2];
    micL[0] = mRoomSize/2.0 - mIntermicDist/2.0 + MIC_OFFSET_LX;
    micL[1] = MIC_Y;
    
    BL_FLOAT micR[2];
    micR[0] = mRoomSize/2.0 + mIntermicDist/2.0 + MIC_OFFSET_RX;
    micR[1] = MIC_Y;

    
    // Make the reflections
    vector<Ray> rays[2];
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            GenerateRays(&rays[i], roomCorners, S, micL);
        else
            GenerateRays(&rays[i], roomCorners, S, micR);
    }
    
    for (int i = 0; i < 2; i++)
    {
        mRays[i] = rays[i];
    }
    
    // Debug
#if 0 // 1
    // Reset the file
    BLDebug::DumpValue("left-x.txt", 0.0);
    BLDebug::DumpValue("left-y.txt", 0.0);
    for (int i = 0; i < mRays[0].size(); i++)
    {
        const Ray &r = mRays[0][i];
        
        BLDebug::AppendValue("left-x.txt", micL[0]);
        BLDebug::AppendValue("left-x.txt", r.mPoint[0]);
        
        BLDebug::AppendValue("left-y.txt", micL[1]);
        BLDebug::AppendValue("left-y.txt", r.mPoint[1]);
    }
    
    BLDebug::DumpValue("right-x.txt", 0.0);
    BLDebug::DumpValue("right-y.txt", 0.0);
    for (int i = 0; i < mRays[1].size(); i++)
    {
        const Ray &r = mRays[1][i];
        
        BLDebug::AppendValue("right-x.txt", micR[0]);
        BLDebug::AppendValue("right-x.txt", r.mPoint[0]);
        
        BLDebug::AppendValue("right-y.txt", micR[1]);
        BLDebug::AppendValue("right-y.txt", r.mPoint[1]);
    }
#endif
}

void
EarlyReflections2::GenerateRays(vector<Ray> *rays,
                                BL_FLOAT roomCorners[4][2],
                                BL_FLOAT S[2], BL_FLOAT R[2])
{
    vector<Ray> prevRays;
    for (int i = 0; i < mOrder; i++)
    {
        vector<Ray> newRays;
        GenerateRays(&newRays, roomCorners, S, R, prevRays, i + 1);
        
        prevRays = newRays;
        
        for (int j = 0; j < newRays.size(); j++)
            rays->push_back(newRays[j]);
    }
}

void
EarlyReflections2::GenerateRays(vector<Ray> *newRays,
                                BL_FLOAT roomCorners[4][2],
                                BL_FLOAT S[2], BL_FLOAT R[2],
                                const vector<Ray> &prevRays,
                                int order)
{
#define EPS 1e-15
    
    BL_FLOAT directDist = BLUtils::ComputeDist(S, R);
    
    // Get the sources
    vector<Point> sources;
    if (prevRays.empty())
    {
        Point p;
        p.mValues[0] = S[0];
        p.mValues[1] = S[1];
        
        sources.push_back(p);
    }
    else
    {
        for (int i = 0; i < prevRays.size(); i++)
        {
            Point p;
            p.mValues[0] = prevRays[i].mPoint[0];
            p.mValues[1] = prevRays[i].mPoint[1];
            
            sources.push_back(p);
        }
    }
    
    for (int i = 0; i < sources.size(); i++)
    {
        BL_FLOAT source[2];
        source[0] = sources[i].mValues[0];
        source[1] = sources[i].mValues[1];
        
        // 4 sides in the room
        int startSide = 0; // Take 4 sides
        ///int startSide = 1; // Take only 3 sides (remove the nearest one)
        for (int j = startSide; j < 4; j++)
        {
            // HACK: consider the room walls are aligned with axes
            
            // Corners
            BL_FLOAT rc0[2];
            rc0[0] = roomCorners[j][0];
            rc0[1] = roomCorners[j][1];
            
            BL_FLOAT rc1[2];
            rc1[0] = roomCorners[(j + 1) % 4][0];
            rc1[1] = roomCorners[(j + 1) % 4][1];
            
            // dx and dy
            BL_FLOAT dx = rc1[0] - rc0[0];
            BL_FLOAT dy = rc1[1] - rc0[1];
            
            BL_FLOAT sourceImg[2];
            if (std::fabs(dx) < EPS)
            {
                sourceImg[0] = source[0] + 2.0*(rc0[0] - source[0]);
                sourceImg[1] = source[1];
            }
            else if (std::fabs(dy) < EPS)
            {
                sourceImg[0] = source[0];
                sourceImg[1] = source[1] + 2.0*(rc0[1] - source[1]);
            }
            
            // Check if this source is valid
            // "Audibility checking"
            BL_FLOAT seg0[2][2];
            seg0[0][0] = rc0[0];
            seg0[0][1] = rc0[1];
            seg0[1][0] = rc1[0];
            seg0[1][1] = rc1[1];
            
            BL_FLOAT seg1[2][2];
            seg1[0][0] = R[0];
            seg1[0][1] = R[1];
            seg1[1][0] = sourceImg[0];
            seg1[1][1] = sourceImg[1];
            
            bool intersect = BLUtilsMath::SegSegIntersect(seg0, seg1);
            if (!intersect)
                continue;
            
            // New ray
            Ray r;
            r.mPoint[0] = sourceImg[0];
            r.mPoint[1] = sourceImg[1];
            
            BL_FLOAT dist = BLUtils::ComputeDist(sourceImg, R);
            
            // Time
            BL_FLOAT ts = dist/SOUND_SPEED;
            
            // Delay
            BL_FLOAT ds = ts*mSampleRate;
            r.mDelay->SetDelay(ds);
            
            // Reflectance
            r.mAttenCoeff = (BL_FLOAT)1.0;
            r.mAttenCoeff *= std::pow(mReflectCoeff, order);
            
            // Distance attenuation
            BL_FLOAT distCoeff = 1.0;
            BL_FLOAT ratio = dist/directDist;
            if (std::fabs(ratio) > EPS)
            {
                distCoeff = 1.0/(ratio*ratio);
            }
            r.mAttenCoeff *= distCoeff;
            
            newRays->push_back(r);
        }
    }
}
