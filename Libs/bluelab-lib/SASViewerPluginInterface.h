//
//  SASViewerPluginInterface.h
//  UST-macOS
//
//  Created by applematuer on 9/26/20.
//
//

#ifndef SASViewerPluginInterface_h
#define SASViewerPluginInterface_h

#include <BLTypes.h>

class SASViewerPluginInterface
{
public:
    virtual void SetCameraAngles(BL_GUI_FLOAT angle0, BL_GUI_FLOAT angle1) = 0;
    virtual void SetCameraFov(BL_GUI_FLOAT angle) = 0;

};
#endif /* SASViewerPluginInterface_h */
