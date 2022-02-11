//
//  Axis3DFactory2.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/5/20.
//
//

#ifndef __BL_SoundMetaViewer__Axis3DFactory2__
#define __BL_SoundMetaViewer__Axis3DFactory2__

#include <BLTypes.h>

// Axis3DFactory2: from Axis3DFactory
// - removed static (this is very dangerous for plugins)

class Axis3D;
class Axis3DFactory2
{
public:
    enum Orientation
    {
        ORIENTATION_X,
        ORIENTATION_Y,
        ORIENTATION_Z
    };
    
    Axis3DFactory2();
    virtual ~Axis3DFactory2();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
    Axis3D *CreateAmpAxis(Orientation orientation);
    Axis3D *CreateAmpDbAxis(Orientation orientation);
    
    Axis3D *CreateFreqAxis(Orientation orientation);
    
    Axis3D *CreateChromaAxis(Orientation orientation);
    
    Axis3D *CreateAngleAxis(Orientation orientation);
    
    Axis3D *CreateLeftRightAxis(Orientation orientation);
    
    Axis3D *CreateMinusOneOneAxis(Orientation orientation);
    
    Axis3D *CreatePercentAxis(Orientation orientation);
    
    Axis3D *CreateEmptyAxis(Orientation orientation);
    
protected:
    void ComputeExtremities(Orientation orientation, BL_FLOAT p0[3], BL_FLOAT p1[3]);
    
    static BL_FLOAT FreqToMelNorm(BL_FLOAT freq, int bufferSize, BL_FLOAT sampleRate);
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SoundMetaViewer__Axis3DFactory2__) */
