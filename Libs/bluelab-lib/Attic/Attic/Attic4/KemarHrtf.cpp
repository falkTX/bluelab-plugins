//
//  HRTF.cpp
//  Spatializer
//
//  Created by Pan on 16/11/17.
//
//

#include <limits.h>

#include "portable_endian.h"
#include "Hrtf.h"
#include <BLUtils.h>

#include "KemarHRTF.h"

#define IMPULSE_SIZE 128

#define DEBUG_RESPONSES 0

bool
KemarHRTF::Load(IGraphics* pGraphics,
				const char *resourcesDir, HRTF **outHrtf)
{
#ifndef WIN32 // Mac
	return LoadMac(resourcesDir, outHrtf);
#else
	return LoadWin(pGraphics, outHrtf);
#endif
}

bool
KemarHRTF::LoadMac(const char *resourcesDir, HRTF **outHrtf)
{
#if DEBUG_RESPONSES
    BL_FLOAT debugAvg[2][360];
    for (int i = 0; i < 360; i++)
    {
        debugAvg[0][i] = 0.0;
        debugAvg[1][i] = 0.0;
    }
#endif
    
    *outHrtf = new HRTF(HRTF_SAMPLERATE);
    
    for (int i = 0; i < MAX_ELEV - MIN_ELEV + 1; i += STEP_ELEV)
    {
        int elev = MIN_ELEV + i;
        
        for (int j = 0; j < MAX_AZIM - MIN_AZIM + 1; j += STEP_AZIM)
        {
            int azim = MIN_AZIM + j;
        
            char fileName[512];
            sprintf(fileName, "%s/H%de%03da.%s", resourcesDir, elev, azim, RESP_EXT);
            
            WDL_TypedBuf<BL_FLOAT> *outImpulseResponses[2];
            ReadOneFile(outImpulseResponses, fileName);
            
            // Deduce the values we can deduce
            // (we have the front at 0 degrees)
            
            int opposite = 360 - j;
            opposite = opposite % 360;
            
#if DEBUG_RESPONSES
            BL_FLOAT avg0 = BLUtils::ComputeAbsAvg(outImpulseResponses[0]->Get(),
                                               outImpulseResponses[0]->GetSize());
            
            BL_FLOAT avg1 = BLUtils::ComputeAbsAvg(outImpulseResponses[1]->Get(),
                                               outImpulseResponses[1]->GetSize());
            
            debugAvg[0][j] += avg0;
            debugAvg[1][j] += avg1;
            
            if (j < 180)
            {
                debugAvg[1][opposite] += avg0;
                debugAvg[0][opposite] += avg1;
            }
#endif
            
            // Two ears
            (*outHrtf)->SetImpulseResponse(outImpulseResponses[0], i/STEP_ELEV, j, 0);
            (*outHrtf)->SetImpulseResponse(outImpulseResponses[1], i/STEP_ELEV, j, 1);
            
            // Two ears, symetrical
            if (j < 180)
            {
                (*outHrtf)->SetImpulseResponse(outImpulseResponses[0], i/STEP_ELEV, opposite, 1);
                (*outHrtf)->SetImpulseResponse(outImpulseResponses[1], i/STEP_ELEV, opposite, 0);
            }
        }
    }
    
#if DEBUG_RESPONSES
    BLDebug::DumpData("avg0.txt", debugAvg[0], 360);
    BLDebug::DumpData("avg1.txt", debugAvg[1], 360);
#endif
    
    return true;
}

bool
KemarHRTF::ReadOneFile(WDL_TypedBuf<BL_FLOAT> *outImpulseResponses[2], const char *fileName)
{
    // Open the file
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
        return false;
    
    // As recommended, choose the maximum size of 128
    // even if the data is smaller
    for (int j = 0; j < 2; j++)
    {
        outImpulseResponses[j] = new WDL_TypedBuf<BL_FLOAT>();
        outImpulseResponses[j]->Resize(IMPULSE_SIZE);
        for (int i = 0; i < IMPULSE_SIZE; i++)
            outImpulseResponses[j]->Get()[i] = 0.0;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    long end = ftell(file);
    
    // Do SEEK_SET at the last, so the file is "rewinded"
    fseek(file, 0, SEEK_SET);
    long start = ftell(file);
    
    long fileSize = (end - start)/sizeof(short);
    
    // Read the file
    short *buf = (short *)malloc(fileSize*sizeof(short));
    fread(buf, sizeof(short), fileSize, file);
    fclose(file);
    
    // Not sure it is useful...
#if 0 // First half: Left, second half: right
    // Get the left and right buffer
    for (int i = 0; i < fileSize/2; i++)
    {
        short dat = be16toh(buf[i]);
        outImpulseResponses[0]->Get()[i] = ((BL_FLOAT)dat)/SHRT_MAX;
    }
    
    for (int i = 0; i < fileSize/2; i++)
    {
        short dat = be16toh(buf[i + fileSize/2]);
        outImpulseResponses[1]->Get()[i] = ((BL_FLOAT)dat)/SHRT_MAX;
    }
#endif
    
    // Interleaved
    // Ok for compact !
    for (int i = 0; i < fileSize/2; i++)
    {
        short dat0 = be16toh(buf[i*2]);
        outImpulseResponses[0]->Get()[i] = ((BL_FLOAT)dat0)/SHRT_MAX;
        
        short dat1 = be16toh(buf[i*2 + 1]);
        outImpulseResponses[1]->Get()[i] = ((BL_FLOAT)dat1)/SHRT_MAX;
    }
    
    free(buf);
    
    return true;
}

#ifdef WIN32

bool
KemarHRTF::LoadWin(IGraphics* pGraphics, HRTF **outHrtf)
{
#define START_RC_ID 1000 

	int rcId = START_RC_ID;

	*outHrtf = new HRTF(HRTF_SAMPLERATE);
	for (int i = 0; i < MAX_ELEV - MIN_ELEV + 1; i += STEP_ELEV)
	{
		int elev = MIN_ELEV + i;

		for (int j = 0; j < MAX_AZIM - MIN_AZIM + 1; j += STEP_AZIM)
		{
			int azim = MIN_AZIM + j;

			WDL_TypedBuf<BL_FLOAT> *outImpulseResponses[2];
			ReadOneFileWin((IGraphicsWin *)pGraphics, outImpulseResponses, rcId);

			// Deduce the values we can deduce
			// (we have the front at 0 degrees)

			int opposite = 360 - j;
			opposite = opposite % 360;

			// Two ears
			(*outHrtf)->SetImpulseResponse(outImpulseResponses[0], i / STEP_ELEV, j, 0);
			(*outHrtf)->SetImpulseResponse(outImpulseResponses[1], i / STEP_ELEV, j, 1);

			// Two ears, symetrical
			if (j < 180)
			{
				(*outHrtf)->SetImpulseResponse(outImpulseResponses[0], i / STEP_ELEV, opposite, 1);
				(*outHrtf)->SetImpulseResponse(outImpulseResponses[1], i / STEP_ELEV, opposite, 0);
			}

			rcId++;
		}
	}

	return true;
}

bool
KemarHRTF::ReadOneFileWin(IGraphicsWin *pGraphics, 
						  WDL_TypedBuf<BL_FLOAT> *outImpulseResponses[2], int rcId)
{
	void *rcBuf;
	long rcSize;
	bool loaded = pGraphics->LoadWindowsResource(rcId, "RCDATA", &rcBuf, &rcSize);
	if (!loaded)
		// There was no file in the resources corresponding to this given impulse response
		// This is normal: in Kemar, we don't have every measurements, and here, we fill by interpolation
		return false;

	// Allocate only if the file exists in the resources
	// Otherwise, keep the null value, which is good for further interpolation

	// As recommended, choose the maximum size of 128
	// even if the data is smaller
	for (int j = 0; j < 2; j++)
	{
		outImpulseResponses[j] = new WDL_TypedBuf<BL_FLOAT>();
		outImpulseResponses[j]->Resize(IMPULSE_SIZE);
		for (int i = 0; i < IMPULSE_SIZE; i++)
			outImpulseResponses[j]->Get()[i] = 0.0;
	}

	long fileSize = rcSize / sizeof(short);

	// Read the file
	short *buf = (short *)rcBuf;

	// Interleaved
	// Ok for compact !
	for (int i = 0; i < fileSize / 2; i++)
	{
		short dat0 = be16toh(buf[i * 2]);
		//short dat0 = buf[i * 2];
		outImpulseResponses[0]->Get()[i] = ((BL_FLOAT)dat0) / SHRT_MAX;

		short dat1 = be16toh(buf[i * 2 + 1]);
		//short dat1 = buf[i * 2 + 1];
		outImpulseResponses[1]->Get()[i] = ((BL_FLOAT)dat1) / SHRT_MAX;
	}

	free(buf);

	return true;
}

#endif // WIN32

void
KemarHRTF::DumpWinRcFile(const char *dumpRcFileName, int startRcId)
{
	const char *resDir = "./resources/hrtfs/KEMAR";

	FILE *dumpFile = fopen(dumpRcFileName, "wb");

	int rcId = startRcId;
	for (int i = 0; i < MAX_ELEV - MIN_ELEV + 1; i += STEP_ELEV)
	{
		int elev = MIN_ELEV + i;

		for (int j = 0; j < MAX_AZIM - MIN_AZIM + 1; j += STEP_AZIM)
		{
			int azim = MIN_AZIM + j;

			char fileName[512];
			sprintf(fileName, "%s/H%de%03da.%s", resDir, elev, azim, RESP_EXT);

			FILE *file = fopen(fileName, "rb");
			if (file == NULL)
				// This file does not exist
			{
				// But increment either, to be able to  locate non-exixsten hrtf 
				// later when loading the resource
				rcId++;
			
				continue;
			}

			fclose(file);

			fprintf(dumpFile, "%d RCDATA \"%s\"\n", rcId, fileName);

			rcId++;
		}
	}

	fclose(dumpFile);
}
