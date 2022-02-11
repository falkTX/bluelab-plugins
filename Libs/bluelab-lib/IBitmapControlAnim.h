//
//  IBitmapControlAnim.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#ifndef IBitmapControlAnim_h
#define IBitmapControlAnim_h

#include <IControl.h>

using namespace iplug::igraphics;

// From IBitmapControlAnim2
class IBitmapControlAnim : public IBitmapControl
{
public:
    IBitmapControlAnim(int x, int y, const IBitmap &bitmap, int paramIdx,
                        double speed, bool pingPong, bool sinAnim,
                        EBlend blend = EBlend::Default)
    : IBitmapControl(x, y, bitmap, paramIdx, blend)
    {
        mSpeed = speed;
        mPingPong = pingPong;
        mSinAnim = sinAnim;
        
        mBitmapNum = 0;
        mPrevBitmapNum = 0;
    }
    
    virtual ~IBitmapControlAnim() {}
    
    void Draw(IGraphics &g) override;
    
    bool IsDirty() override;
    
protected:
    void UpdateBitmapNum();
    
    //
    double mSpeed;
    bool mPingPong;
    bool mSinAnim;
    
    int mBitmapNum;
    int mPrevBitmapNum;
};


#endif /* IBitmapControlAnim */
