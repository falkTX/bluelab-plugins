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

#include "../../WDL/IPlug/Containers.h"

class Spectrum
{
public:
    Spectrum(int width);
    
    virtual ~Spectrum();
    
    void SetInputMultiplier(double mult);
    
    static Spectrum *Load(const char *fileName);
    
    void AddLine(const WDL_TypedBuf<double> &newLine);
    
    const WDL_TypedBuf<double> *GetLine(int index);
    
    void Save(const char *filename);
    
protected:
    int mWidth;
    double mInputMultiplier;
    
    vector<WDL_TypedBuf<double> > mLines;
};

#endif /* defined(__Denoiser__Spectrum__) */
