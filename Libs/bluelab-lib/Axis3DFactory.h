//
//  Axis3DFactory.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/5/20.
//
//

#ifndef __BL_SoundMetaViewer__Axis3DFactory__
#define __BL_SoundMetaViewer__Axis3DFactory__

#ifdef IGRAPHICS_NANOVG

class Axis3D;

class Axis3DFactory
{
public:
    enum Orientation
    {
        ORIENTATION_X,
        ORIENTATION_Y,
        ORIENTATION_Z
    };
    
    static Axis3DFactory *Get();
    virtual ~Axis3DFactory();
    
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
    
    static Axis3DFactory *mInstance;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
private:
    Axis3DFactory();
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SoundMetaViewer__Axis3DFactory__) */
