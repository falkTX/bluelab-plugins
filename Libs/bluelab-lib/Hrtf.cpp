//
//  Hrtf.cpp
//  Spatializer
//
//  Created by Pan on 02/12/17.
//
//

#include <BLUtils.h>
#include <DebugGraph.h>

#include "Hrtf.h"

#define DEBUG_GRAPH 0

HRTF::HRTF(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int k = 0; k < 2; k++)
        for (int j = 0; j < HRTF_NUM_AZIM; j++)
            for (int i = 0; i < NUM_ELEV; i++)
                mImpulseResponses[i][j][k] = NULL;
}

HRTF::~HRTF()
{
    for (int k = 0; k < 2; k++)
        for (int j = 0; j < HRTF_NUM_AZIM; j++)
            for (int i = 0; i < NUM_ELEV; i++)
                delete mImpulseResponses[i][j][k];
}

BL_FLOAT
HRTF::GetSampleRate()
{
    return mSampleRate;
}

bool
HRTF::GetImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                         BL_FLOAT elev, BL_FLOAT azim, int chan)
{
    int elevId = (int)round(elev - MIN_ELEV)/STEP_ELEV;
    int azimId = (int)azim;
    
    bool res = GetNearestImpulseResponse(outImpulseResponse, elevId, azimId, chan);
    
    return res;
}

bool
HRTF::GetImpulseResponseInterp(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                               BL_FLOAT elev, BL_FLOAT azim, int chan,
                               BL_FLOAT delay, int respSize, bool *overFlag)

{
    int elevId = (int)round(elev - MIN_ELEV)/STEP_ELEV;
    int azimId = (int)azim;
    
    int elevIds[2] = { elevId, elevId + 1 };
    for (int i = 0; i < 2; i++)
    {
        if (elevIds[i] < 0)
            elevIds[i] = 0;
        
        if (elevIds[i] > NUM_ELEV - 1)
            elevIds[i] = NUM_ELEV - 1;
    }
    
    int azimIds[2] = { azimId, azimId + 1 };
    for (int i = 0; i < 2; i++)
    {
        if (azimIds[i] < 0)
            azimIds[i] += HRTF_NUM_AZIM;
        
        if (azimIds[i] >= HRTF_NUM_AZIM)
            azimIds[i] -= HRTF_NUM_AZIM;
    }
    
    // ((e-, a-), (e+, a-)) ((e-, a+), (e+, a+))
    WDL_TypedBuf<BL_FLOAT> impulseResponses[2][2];
    
    int elevIdFound[2];
    int azimIdFound[2];
    bool res = true;
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++)
    {
        int elevDir = (i == 0) ? - 1 : 1;
        int azimDir = (j == 0) ? - 1 : 1;
        bool res2 = GetNearestImpulseResponse(&impulseResponses[i][j],
                                              elevIds[i], azimIds[j],
                                              &elevIdFound[i], &azimIdFound[j],
                                              elevDir, azimDir, chan);
            
        res = res && res2;
    }
    
    BL_FLOAT elevs[2];
    for (int i = 0; i < 2; i++)
        elevs[i] = ((BL_FLOAT)elevIdFound[i])*STEP_ELEV + MIN_ELEV;
    
    
    BL_FLOAT azims[2];
    for (int i = 0; i < 2; i++)
        azims[i] = (BL_FLOAT)azimIdFound[i];
    
    BL_FLOAT u = 0.0;
    if (elevIdFound[0] != elevIdFound[1])
        u = (elev - elevs[0])/(elevs[1] - elevs[0]);
    
    // Reorder
    if (azims[0] > azims[1])
    {
        azims[0] -= HRTF_NUM_AZIM;
    }
    
    // v
    BL_FLOAT v = 0.0;
    if (azimIdFound[0] != azimIdFound[1] != 0)
        v = (azim - azims[0])/(azims[1] - azims[0]);
    
    WDL_TypedBuf<BL_FLOAT> resp;
    BLUtils::Interp2D(&resp, impulseResponses, u, v);
    
    ApplyDelay(outImpulseResponse, &resp,
               delay, respSize, overFlag);
    
#if DEBUG_GRAPH
#define GRAPH_SCALE 0.25
    
    fprintf(stderr, "input elev: %d azim: %d\n", elevId, azimId);
    fprintf(stderr, "result\n");
    fprintf(stderr, "elev ids: (%g %g) \n", elevs[0], elevs[1]);
    fprintf(stderr, "azim ids: (%g %g)\n", azims[0], azims[1]);
    fprintf(stderr, "(u, v) : (%g %g)\n", u, v);
    
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
        {
            WDL_TypedBuf<BL_FLOAT> debugImpulse = impulseResponses[i][j];
            DebugGraph::SetCurveValues(debugImpulse, 1 + j + i*2, -GRAPH_SCALE, GRAPH_SCALE, 1.0,
                                       160, 160, 160);
                                       //true, 0.2);
        }
    
    WDL_TypedBuf<BL_FLOAT> debugAvg;
    vector<WDL_TypedBuf<BL_FLOAT> > debugCurves;
    debugCurves.push_back(impulseResponses[0][0]);
    debugCurves.push_back(impulseResponses[1][0]);
    debugCurves.push_back(impulseResponses[0][1]);
    debugCurves.push_back(impulseResponses[1][1]);
    BLUtils::ComputeAvg(&debugAvg, debugCurves);
    DebugGraph::SetCurveValues(debugAvg, 5, -GRAPH_SCALE, GRAPH_SCALE, 1.0, 255, 0, 0);
    
    DebugGraph::SetCurveValues(resp, 6, -GRAPH_SCALE, GRAPH_SCALE, 2.0, 0, 255, 0);
#endif

    return res;
}

bool
HRTF::GetImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                         BL_FLOAT elev, BL_FLOAT azim, int chan,
                         BL_FLOAT delay, int respSize, bool *overFlag)
{
    *overFlag = false;
    
    WDL_TypedBuf<BL_FLOAT> response;
    bool res = GetImpulseResponse(&response, elev, azim, chan);
    
    ApplyDelay(outImpulseResponse, &response,
               delay, respSize, overFlag);
    
    return res;
}

void
HRTF::SetImpulseResponse(const WDL_TypedBuf<BL_FLOAT> *response,
                         int elevId, int azimId, int chan)
{
    if (elevId >= NUM_ELEV)
        return;
    if (azimId >= HRTF_NUM_AZIM)
        return;
    
    mImpulseResponses[elevId][azimId][chan] = new WDL_TypedBuf<BL_FLOAT>();
    mImpulseResponses[elevId][azimId][chan]->Resize(response->GetSize());
                                                   
    *mImpulseResponses[elevId][azimId][chan] = *response;
}

bool
HRTF::GetNearestImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                                int elevId, int azimId, int chan)
{
    bool found = false;
    while(!found && (azimId < HRTF_NUM_AZIM))
    {
        found = GetImpulseResponse(outImpulseResponse, elevId, azimId, chan);
        
        azimId++;
    }
    
    return found;
}

bool
HRTF::GetNearestImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                                int elevId, int azimId,
                                int *elevIdFound, int *azimIdFound,
                                int elevDir, int azimDir, int chan)
{
    bool found = false;
    *elevIdFound = -1;
    *azimIdFound = -1;
    
    while(!found && (azimId >= 0) && (azimId < HRTF_NUM_AZIM))
    {
        found = GetImpulseResponse(outImpulseResponse, elevId, azimId, chan);
        
        if (found)
        {
            *elevIdFound = elevId;
            *azimIdFound = azimId;
        }
        else
            azimId += azimDir;
    }
    
    return found;
}

bool
HRTF::GetImpulseResponse(WDL_TypedBuf<BL_FLOAT> *outImpulseResponse,
                         int elevId, int azimId, int chan)
{
    if (elevId < 0)
        return false;
    if (elevId >= NUM_ELEV)
        return false;
    
    if (azimId < 0)
        return false;
    if (azimId >= HRTF_NUM_AZIM)
        return false;
    
    if (mImpulseResponses[elevId][azimId][chan] == NULL)
        return false;
    
    int size = mImpulseResponses[elevId][azimId][chan]->GetSize();
    outImpulseResponse->Resize(size);
    
    *outImpulseResponse = *mImpulseResponses[elevId][azimId][chan];
    
    return true;
}

long
HRTF::FindZeroPhaseIndex(const WDL_TypedBuf<BL_FLOAT> *response)
{
    // For the moment, static threshold
    // For Kemar
    // Could be improved...
#define THRS 0.002
//#define THRS 0.02
    
    long result = 0;
    for (int i = 0; i < response->GetSize(); i++)
    {
        BL_FLOAT val = response->Get()[i];
        
        if (std::fabs(val) >= THRS)
        {
            result = i;
            break;
        }
    }
    
    return result;
}

void
HRTF::ApplyDelay(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> *response,
                 BL_FLOAT delay, int respSize, bool *overFlag)
{
    int zeroIndex = FindZeroPhaseIndex(response);
    
    // Keep the previous 0
    zeroIndex = zeroIndex - 1;
    if (zeroIndex < 0)
        zeroIndex = 0;
    
    int delaySample = (int)(delay*mSampleRate);
    
    result->Resize(respSize);
    BLUtils::FillAllZero(result);
    
    for (int i = zeroIndex; i < response->GetSize(); i++)
    {
        BL_FLOAT val = response->Get()[i];
        
        //int index = offset + i;
        int index = delaySample + i - zeroIndex;
        if (index >= result->GetSize())
            break;
        
        result->Get()[index] = val;
    }
}
