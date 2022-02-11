//
//  View3DPluginInterface.h
//  UST-macOS
//
//  Created by applematuer on 9/26/20.
//
//

#ifndef View3DPluginInterface_h
#define View3DPluginInterface_h

#include <BLTypes.h>

class View3DPluginInterface
{
public:
    virtual void SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1) = 0;
    virtual void SetCameraFov(BL_FLOAT angle) = 0;
};

#endif /* View3DPluginInterface_h */
