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

#include "../../WDL/fastqueue.h"

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
    
    void SetInputs(const vector<WDL_TypedBuf<BL_FLOAT> > &buffers);
    bool GetInputs(vector<WDL_TypedBuf<BL_FLOAT> > *buffers);
    
    void ResizeOutputs(vector<WDL_TypedBuf<BL_FLOAT> > *buffers);
                    
    void SetOutputs(const vector<WDL_TypedBuf<BL_FLOAT> > &buffers);
    bool GetOutputs(vector<WDL_TypedBuf<BL_FLOAT> > *buffers, int nFrames);
    
protected:
    int mBufferSize;
    int mCurrentLatency;
    
    //vector<WDL_TypedBuf<BL_FLOAT> > mInputs;
    //vector<WDL_TypedBuf<BL_FLOAT> > mOutputs;
    vector<WDL_TypedFastQueue<BL_FLOAT> > mInputs;
    vector<WDL_TypedFastQueue<BL_FLOAT> > mOutputs;
};

#endif /* defined(__BL_Spatializer__FixedBuffedObj__) */
