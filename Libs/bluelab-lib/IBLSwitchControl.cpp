#include "IBLSwitchControl.h"

IBLSwitchControl::IBLSwitchControl(float x, float y, const IBitmap& bitmap,
                                   int paramIdx)
: IBSwitchControl(x, y, bitmap, paramIdx)
{
    mClickToggleOff = true;
}

IBLSwitchControl::IBLSwitchControl(const IRECT& bounds, const IBitmap& bitmap,
                                   int paramIdx)
: IBSwitchControl(bounds, bitmap, paramIdx)
{
    mClickToggleOff = true;
}

void
IBLSwitchControl::SetClickToggleOff(bool flag)
{
    mClickToggleOff = flag;
}

void
IBLSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
    if (!mClickToggleOff)
    {
        double value = GetValue();
        if (value > 0.5)
            return;
    }
    
    IBSwitchControl::OnMouseDown(x, y, mod);
}
