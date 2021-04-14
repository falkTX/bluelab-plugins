#ifndef BL_BITMAP_H
#define BL_BITMAP_H

#include <stdlib.h>

#include "IPlug_include_in_plug_hdr.h"

#ifdef IGRAPHICS_NANOVG

class BLBitmap
{
public:
    BLBitmap(int width, int height, int bpp,
             unsigned char *data = NULL);

    BLBitmap(const BLBitmap &other);
    
    virtual ~BLBitmap();

    void Clear();
    
    int GetWidth() const;
    int GetHeight() const;
    int GetBpp() const;
    
    const unsigned char *GetData() const;
    
    // Load from file
    static BLBitmap *Load(const char *fileName);
    // Save
    static void Save(const BLBitmap *bmp, const char *fileName);
    
    static void FillAlpha(BLBitmap *bmp, unsigned char val);
    
    static void Blit(BLBitmap *dst, const BLBitmap *src);
    
    static void ScaledBlit(BLBitmap *dest, const BLBitmap *src,
                           int dstx, int dsty, int dstw, int dsth,
                           float srcx, float srcy, float srcw, float srch);

protected:
    int mWidth;
    int mHeight;
    int mBpp;
    
    unsigned char *mData;
};

#endif

#endif
