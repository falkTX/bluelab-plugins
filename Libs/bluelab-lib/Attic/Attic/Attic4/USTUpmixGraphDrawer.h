//
//  USTUpmixGraphDrawer.h
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#ifndef __UST__USTUpmixGraphDrawer__
#define __UST__USTUpmixGraphDrawer__

#include <GraphControl11.h>

class UST;

class USTUpmixGraphDrawer : public GraphCustomDrawer,
                            public GraphCustomControl
{
public:
    USTUpmixGraphDrawer(UST *plug, GraphControl11 *graph);
    
    virtual ~USTUpmixGraphDrawer();
    
    // GraphCustomDrawer
    virtual void PreDraw(NVGcontext *vg, int width, int height);
    
    //
    void SetGain(BL_FLOAT gain);
    void SetPan(BL_FLOAT pan);
    void SetDepth(BL_FLOAT depth);
    void SetBrillance(BL_FLOAT brillance);
    
    // GraphCustomControl
    void OnMouseDown(int x, int y, IMouseMod* pMod);
    void OnMouseUp(int x, int y, IMouseMod* pMod);
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    
protected:
    void DrawSource(NVGcontext *vg, int width, int height);
    
    void ComputeSourceCenter(BL_FLOAT center[2], int width, int height);
    BL_FLOAT ComputeRad0(int height);
    BL_FLOAT ComputeRad1(int height);
    
    void SourceCenterToPanDepth(const BL_FLOAT center[2],
                                int width, int height,
                                BL_FLOAT *outPan, BL_FLOAT *outDepth);

    
    BL_FLOAT mGain;
    BL_FLOAT mPan;
    BL_FLOAT mDepth;
    BL_FLOAT mBrillance;
    
    // GraphCustomControl
    int mWidth;
    int mHeight;
    
    bool mSourceIsSelected;
    
    UST *mPlug;
    GraphControl11 *mGraph;
};

#endif /* defined(__UST__USTUpmixGraphDrawer__) */
