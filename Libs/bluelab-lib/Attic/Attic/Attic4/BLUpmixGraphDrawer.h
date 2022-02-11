//
//  BLUpmixGraphDrawer.h
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#ifndef __UST__BLUpmixGraphDrawer__
#define __UST__BLUpmixGraphDrawer__

#include <GraphControl11.h>

class BLVectorscopePlug;

// BLUpmixGraphDrawer.h: from USTUpmixGraphDrawer.h
//
class BLUpmixGraphDrawer : public GraphCustomDrawer,
                            public GraphCustomControl
{
public:
    BLUpmixGraphDrawer(BLVectorscopePlug *plug, GraphControl11 *graph,
                       const char *title = NULL);
    
    virtual ~BLUpmixGraphDrawer();
    
    // GraphCustomDrawer
    virtual void PreDraw(NVGcontext *vg, int width, int height);
    
    // GraphCustomControl
    void OnMouseDown(int x, int y, IMouseMod* pMod);
    void OnMouseUp(int x, int y, IMouseMod* pMod);
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    
    //
    void SetGain(BL_FLOAT gain);
    void SetPan(BL_FLOAT pan);
    void SetDepth(BL_FLOAT depth);
    void SetBrillance(BL_FLOAT brillance);
    
protected:
    void DrawSource(NVGcontext *vg, int width, int height);
    
    void ComputeSourceCenter(BL_FLOAT center[2], int width, int height);
    BL_FLOAT ComputeRad0(int height);
    BL_FLOAT ComputeRad1(int height);
    
    void SourceCenterToPanDepth(const BL_FLOAT center[2],
                                int width, int height,
                                BL_FLOAT *outPan, BL_FLOAT *outDepth);

    //
    BLVectorscopePlug *mPlug;
    GraphControl11 *mGraph;
    
    //
    BL_FLOAT mGain;
    BL_FLOAT mPan;
    BL_FLOAT mDepth;
    BL_FLOAT mBrillance;
    
    // GraphCustomControl
    int mWidth;
    int mHeight;
    
    bool mSourceIsSelected;
    bool mPrevMouseDrag;
    
    // Title
    bool mTitleSet;
    char mTitleText[256];
};

#endif /* defined(__UST__BLUpmixGraphDrawer__) */
