//
//  WavesPluginInterface.h
//  UST-macOS
//
//  Created by applematuer on 9/26/20.
//
//

#ifndef WavesPluginInterface_h
#define WavesPluginInterface_h

#include <BLTypes.h>

class WavesPluginInterface
{
public:
    virtual void SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1) = 0;
    virtual void SetCameraFov(BL_FLOAT angle) = 0;
};

#endif /* WavesPluginInterface_h */
