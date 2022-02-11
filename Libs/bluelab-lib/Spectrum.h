//
//  Spectrum.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#ifndef __Denoiser__Spectrum__
#define __Denoiser__Spectrum__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

class Spectrum
{
public:
    Spectrum(int width);
    
    virtual ~Spectrum();
    
    void SetInputMultiplier(BL_FLOAT mult);
    
    static Spectrum *Load(const char *fileName);
    
    void AddLine(const WDL_TypedBuf<BL_FLOAT> &newLine);
    
    const WDL_TypedBuf<BL_FLOAT> *GetLine(int index);
    
    void Save(const char *filename);
    
protected:
    int mWidth;
    BL_FLOAT mInputMultiplier;
    
    vector<WDL_TypedBuf<BL_FLOAT> > mLines;
};

#endif /* defined(__Denoiser__Spectrum__) */
