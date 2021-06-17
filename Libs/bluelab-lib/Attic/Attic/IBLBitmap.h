#ifndef IBL_BITMAP_H
#define IBL_BITMAP_H

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IBLBitmap : public IBitmap
{
 public:
    IBLBitmap(APIBitmap* pAPIBitmap, int n, bool framesAreHorizontal,
              const char* name = "");
        
    IBLBitmap();
};

#endif
