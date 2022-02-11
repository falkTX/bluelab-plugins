//
//  Axis3DFactory2.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/5/20.
//
//

#include <BLUtils.h>

#include <Axis3D.h>

#include "Axis3DFactory2.h"

#define AXIS_OFFSET_Z 0.06

// Artificially modify the coeff, to increase the spread on the grid
#define MEL_COEFF 4.0


Axis3DFactory2::Axis3DFactory2()
{
    mBufferSize = 2048;
    mSampleRate = 44100.0;
}

Axis3DFactory2::~Axis3DFactory2() {}

void
Axis3DFactory2::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
}

Axis3D *
Axis3DFactory2::CreateAmpAxis(Orientation orientation)
{
    // Linear scale
#define NUM_AXIS_DATA 5
    char *labels[NUM_AXIS_DATA] =
    {
        "-20dB", "-15dB", "-10dB", "-5dB", "0dB"
    };
    
    // Scale the axis normalized values
    BL_FLOAT ampsDB[NUM_AXIS_DATA] =
    {
        -20.0, -15.0, -10.0, -5.0, 0.0
    };
    
    BL_FLOAT normPos[NUM_AXIS_DATA];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        BL_FLOAT ampDB = ampsDB[i];
        
        // ORIG
        BL_FLOAT amp = BLUtils::DBToAmp(ampDB);
        
        // TEST
        //BL_FLOAT amp = ampDB/20.0 + 1.0;
        
        normPos[i] = amp;
    }
    
    // 3D extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    // Create the axis
    Axis3D *axis = new Axis3D(labels, normPos, NUM_AXIS_DATA, p0, p1);
        
    return axis;
}

Axis3D *
Axis3DFactory2::CreateAmpDbAxis(Orientation orientation)
{
    // Db scale
#define NUM_AXIS_DATA_DBSCALE 7
    char *labelsDBScale[NUM_AXIS_DATA_DBSCALE] =
    {
        ""/*"-60dB"*/, "-50dB", "-40dB", "-30dB", "-20dB", "-10dB", "0dB"
    };
    
    // Scale the axis normalized values
    BL_FLOAT ampsDBDBScale[NUM_AXIS_DATA_DBSCALE] =
    {
        -60.0, -50.0, -40.0, -30.0, -20.0, -10.0, 0.0
    };
    
    BL_FLOAT normPosDBScale[NUM_AXIS_DATA_DBSCALE];
    for (int i = 0; i < NUM_AXIS_DATA_DBSCALE; i++)
    {
        BL_FLOAT ampDB = ampsDBDBScale[i];
        
        // ORIG
        //BL_FLOAT amp = DBToAmp(ampDB);
        
        // NEW: Looks better !
        BL_FLOAT amp = ampDB/60.0 + 1.0;
        
        normPosDBScale[i] = amp;
    }
    
    // 3D extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    // Create the axis
    Axis3D *axis = new Axis3D(labelsDBScale, normPosDBScale, NUM_AXIS_DATA_DBSCALE, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreateFreqAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 9
    char *labels[NUM_AXIS_DATA] =
    {
        /*"1Hz",*/ "", "100Hz", "500Hz", "1KHz", "2KHz", "5KHz", "10KHz", "20KHz", ""
    };
    
    // Scale the axis normalized values
    BL_FLOAT freqs[NUM_AXIS_DATA] =
    {
        1.0, 100.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0, 40000.0
    };
    
    BL_FLOAT normPos[NUM_AXIS_DATA];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        BL_FLOAT freq = freqs[i];
        freq = FreqToMelNorm(freq, mBufferSize, mSampleRate);
        
        normPos[i] = freq;
    }
    
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, normPos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreateChromaAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 13 //12
    char *labels[NUM_AXIS_DATA] =
    {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C"
    };
    
    BL_FLOAT notesPos[NUM_AXIS_DATA] =
    {
        0.0/12.0, 1.0/12.0, 2.0/12.0, 3.0/12.0, 4.0/12.0, 5.0/12.0,
        6.0/12.0, 7.0/12.0, 8.0/12.0, 9.0/12.0, 10.0/12.0, 11.0/12.0,
        12.0/12.0
    };
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, notesPos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreateAngleAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 7
    char *labels[NUM_AXIS_DATA] =
    {
        "-90°", "-60°", "-45°", "0°", "+45°", "+60°", "+90°"
    };
    
    BL_FLOAT anglesPos[NUM_AXIS_DATA] =
    {
        -90.0/180.0 + 0.5,
        -60.0/180.0 + 0.5,
        -45.0/180.0 + 0.5,
          0.0/180.0 + 0.5,
         45.0/180.0 + 0.5,
         60.0/180.0 + 0.5,
         90.0/180.0 + 0.5
    };
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, anglesPos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreateLeftRightAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 3
    char *labels[NUM_AXIS_DATA] =
    {
        "L", ".", "R"
    };
    
    BL_FLOAT lRPos[NUM_AXIS_DATA] =
    {
        0.0, 0.5, 1.0
    };
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, lRPos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreateMinusOneOneAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 3
    char *labels[NUM_AXIS_DATA] =
    {
        "-1", "0", "1"
    };
    
    BL_FLOAT pos[NUM_AXIS_DATA] =
    {
        0.0, 0.5, 1.0
    };
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, pos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreatePercentAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 11
    char *labels[NUM_AXIS_DATA] =
    {
        "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"
    };
    
    BL_FLOAT pos[NUM_AXIS_DATA] =
    {
        0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
    };
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, pos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

Axis3D *
Axis3DFactory2::CreateEmptyAxis(Orientation orientation)
{
    // Create axis
#define NUM_AXIS_DATA 2
    char *labels[NUM_AXIS_DATA] =
    {
        "", ""
    };
    
    BL_FLOAT pos[NUM_AXIS_DATA] =
    {
        0.0, 1.0
    };
    
    // 3d extremities of the axis
    BL_FLOAT p0[3];
    BL_FLOAT p1[3];
    ComputeExtremities(orientation, p0, p1);
    
    Axis3D *axis = new Axis3D(labels, pos, NUM_AXIS_DATA, p0, p1);
    
    return axis;
}

void
Axis3DFactory2::ComputeExtremities(Orientation orientation, BL_FLOAT p0[3], BL_FLOAT p1[3])
{
    if (orientation == ORIENTATION_X)
    {
        //p0 = { -0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
        //p1 = { 0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
        
        p0[0] = -0.5;
        p0[1] = 0.0;
        p0[2] = 0.5 + AXIS_OFFSET_Z;
        
        p1[0] = 0.5;
        p1[1] = 0.0;
        p1[2] = 0.5 + AXIS_OFFSET_Z;
    }
    
    if (orientation == ORIENTATION_Y)
    {
        //p0 = { -0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
        //p1 = { -0.5, 0.5, 0.5 + AXIS_OFFSET_Z };
        p0[0] = -0.5;
        p0[1] = 0.0;
        p0[2] = 0.5 + AXIS_OFFSET_Z;
        
        p1[0] = -0.5;
        p1[1] = 1.0; //0.5;
        p1[2] = 0.5 + AXIS_OFFSET_Z;
    }
    
    if (orientation == ORIENTATION_Z)
    {
        //p0 = { -0.5, 0.0, -0.5 + AXIS_OFFSET_Z };
        //p1 = { -0.5, 0.0, 0.5 + AXIS_OFFSET_Z };
        p0[0] = -0.5;
        p0[1] = 0.0;
        p0[2] = -0.5 + AXIS_OFFSET_Z;
        
        p1[0] = -0.5;
        p1[1] = 0.0;
        p1[2] = 0.5 + AXIS_OFFSET_Z;
    }
}

BL_FLOAT
Axis3DFactory2::FreqToMelNorm(BL_FLOAT freq, int bufferSize, BL_FLOAT sampleRate)
{
    // Convert to Mel
    BL_FLOAT hzPerBin = sampleRate/(bufferSize/2.0);
    hzPerBin *= MEL_COEFF;
    
    // Hack: something is not really correct here...
    freq *= 2.0;
    
    BL_FLOAT result = BLUtils::FreqToMelNorm((BL_FLOAT)(freq*MEL_COEFF), hzPerBin, bufferSize);
    
    return result;
}

