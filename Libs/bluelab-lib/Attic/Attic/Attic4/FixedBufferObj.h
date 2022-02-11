//
//  FixedBuffedObj.h
//  BL-Spatializer
//
//  Created by applematuer on 5/23/19.
//
//

#ifndef __BL_Spatializer__FixedBuffedObj__
#define __BL_Spatializer__FixedBuffedObj__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"


// Used to force using a fixed buffer size, not nFrames
class FixedBufferObj
{
public:
    FixedBufferObj(int bufferSize);
    
    virtual ~FixedBufferObj();
    
    void Reset();
    
    // NEW
    void Reset(int bufferSize);
    
    void SetInputs(const vector<WDL_TypedBuf<double> > &buffers);
    bool GetInputs(vector<WDL_TypedBuf<double> > *buffers);
    
    void ResizeOutputs(vector<WDL_TypedBuf<double> > *buffers);
                    
    void SetOutputs(const vector<WDL_TypedBuf<double> > &buffers);
    bool GetOutputs(vector<WDL_TypedBuf<double> > *buffers, int nFrames);
    
protected:
    int mBufferSize;
    int mCurrentLatency;
    
    vector<WDL_TypedBuf<double> > mInputs;
    vector<WDL_TypedBuf<double> > mOutputs;
};

#endif /* defined(__BL_Spatializer__FixedBuffedObj__) */
