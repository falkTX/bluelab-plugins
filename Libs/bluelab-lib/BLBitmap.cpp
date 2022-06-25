/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#ifdef IGRAPHICS_NANOVG

#include <string.h>

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// From IGraphics dependencies
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <BLBitmap.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsFile.h>

#if 0
TODO: for resizing think about using https://raw.githubusercontent.com/nothings/stb/master/stb_image_resize.h
(it is currently used by #bluela bin darknet)
Because the current implementation is hackish and was copied/pasted from somewhere else,
with risky modifications.
#endif

BLBitmap::BLBitmap(int width, int height, int bpp,
                   unsigned char *data)
{
    mWidth = width;
    mHeight = height;
    mBpp = bpp;
    
    mData = data;
    
    if (mData == NULL)
    {
        //mData = new unsigned char[width*height*bpp];
        // stbi uses malloc ?
        mData = (unsigned char *)malloc(width*height*bpp*sizeof(unsigned char));
        Clear();
    }
}

BLBitmap::BLBitmap(const BLBitmap &other)
{
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mBpp = other.mBpp;
    
    mData = NULL;
    if (other.mData != NULL)
    {
        //mData = new unsigned char[mWidth*mHeight*mBpp];
        // stbi uses malloc ?
        mData = (unsigned char *)malloc(mWidth*mHeight*mBpp*sizeof(unsigned char));
        memcpy(mData, other.mData, mWidth*mHeight*mBpp*sizeof(unsigned char));
    }
}

BLBitmap::~BLBitmap()
{
    // stbi uses malloc ?
    //delete mData;
    free(mData);
}

void
BLBitmap::Clear()
{
    if (mData != NULL)
        memset(mData, 0, mWidth*mHeight*mBpp);
}

int
BLBitmap::GetWidth() const
{
    return mWidth;
}

int
BLBitmap::GetHeight() const
{
    return mHeight;
}

int
BLBitmap::GetBpp() const
{
    return mBpp;
}

const unsigned char *
BLBitmap::GetData() const
{
    return mData;
}

BLBitmap *
BLBitmap::Load(const char *fileName)
{
    int w, h, n;
    unsigned char *data;
    
    int reqComp = 4;
    
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);
    data = stbi_load(fileName, &w, &h, &n, reqComp);
    if (data == NULL)
    {
        // printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
       
        return NULL;
    }
    
    BLBitmap *bmp = new BLBitmap(w, h, reqComp, data);
    return bmp;
}

void
BLBitmap::Save(const BLBitmap *bmp, const char *fileName)
{
    int w = bmp->GetWidth();
    int h = bmp->GetHeight();
    int comp = bmp->GetBpp();
    const void *data = bmp->GetData();
    
    char *ext = BLUtilsFile::GetFileExtension(fileName);
    if (strcmp(ext, "png") == 0)
    {
        int stride_in_bytes = comp*w;
        stbi_write_png(fileName, w, h, comp, data, stride_in_bytes);
    }
    else if (strcmp(ext, "bmp") == 0)
    {
        stbi_write_bmp(fileName, w, h, comp, data);
    }
    else if (strcmp(ext, "tga") == 0)
    {
        stbi_write_tga(fileName, w, h, comp, data);
    }
    else if (strcmp(ext, "jpg") == 0)
    {
        int quality = 90;
        stbi_write_jpg(fileName, w, h, comp, data, quality);
    }
}

void
BLBitmap::FillAlpha(BLBitmap *bmp, unsigned char val)
{
    if (bmp->mBpp < 4)
        return;
    
    for (int i = 3; i < bmp->mWidth*bmp->mHeight*bmp->mBpp; i += 4)
        bmp->mData[i] = val;
    //for (int i = 0; i < bmp->mWidth*bmp->mHeight; i++)
    //    bmp->mData[i*bmp->mBpp + 3] = val;
}

void
BLBitmap::Blit(BLBitmap *dst, const BLBitmap *src)
{
    if (src->mWidth != dst->mWidth)
        return;
    if (src->mHeight != dst->mHeight)
        return;
    
    if ((src->mWidth == dst->mWidth) &&
        (src->mHeight == dst->mHeight) &&
        (src->mBpp == dst->mBpp))
        memcpy(dst->mData, src->mData,
               src->mWidth*src->mHeight*src->mBpp*sizeof(unsigned char));
    else
    {
        for (int i = 0; i < dst->mWidth*dst->mHeight; i++)
        {
            for (int k = 0; k < dst->mBpp; k++)
            {
                if (k < src->mBpp)
                    dst->mData[i*dst->mBpp + k] = src->mData[i*src->mBpp + k];
            }
        }
    }
}

// From Lice
// this is only designed for filtering down an image approx 2:1 to 4:1 or so.. it'll work (poortly) for higher, and for less it's crap too.
static void
scaleBlitFilterDown(unsigned char *dest, const unsigned char *src,
                    int w, int h,
                    int icurx, int icury, int idx, int idy, int clipright, int clipbottom,
                    int src_span, int dest_span,
                    const int *filter, int filt_start, int filtsz)
{
    
    while (h--)
    {
        int cury = icury >> 16;
        int curx=icurx;
        int n=w;
        if (cury >= 0 && cury < clipbottom)
        {
            const unsigned char *inptr=src + (cury+filt_start) * src_span;
            unsigned char *pout=dest;
            while (n--)
            {
                int offs=curx >> 16;
                if (offs>=0 && offs<clipright)
                {
                    int r=0,g=0,b=0,a=0;
                    int sc=0;
                    int fy=filtsz;
                    int ypos=cury+filt_start;
                    const unsigned char *rdptr  = inptr + (offs + filt_start)*(int) (sizeof(int)/sizeof(unsigned char));
                    const int *scaletab = filter;
                    while (fy--)
                    {
                        if (ypos >= clipbottom)
                            break;
                        
                        if (ypos >= 0)
                        {
                            int xpos=offs + filt_start;
                            const unsigned char *pin = rdptr;
                            int fx=filtsz;
                            while (fx--)
                            {
                                int tsc = *scaletab++;
                                if (xpos >= 0 && xpos < clipright)
                                {
                                    r+=pin[0]*tsc;
                                    g+=pin[1]*tsc;
                                    b+=pin[2]*tsc;
                                    a+=pin[2]*tsc;
                                    sc+=tsc;
                                }
                                xpos++;
                                pin+=sizeof(int)/sizeof(unsigned char);
                            }
                        }
                        else
                            scaletab += filtsz;
                        
                        ypos++;
                        rdptr+=src_span;
                    }
                    if (sc>0)
                    {
                        pout[0] = r/sc;
                        pout[1] = g/sc;
                        pout[2] = b/sc;
                        pout[3] = a/sc;
                    }
                }
                
                pout += sizeof(int)/sizeof(unsigned char);
                curx+=idx;
            }
        }
        dest+=dest_span;
        icury+=idy;
    }
}

static void
LinearFilterI(int *r, int *g, int *b, int *a,
              const unsigned char *pin,
              const unsigned char *pinnext,
              unsigned int frac)
{
    const unsigned int f=65536-frac;
    *r=(pin[0]*f + pinnext[0]*frac)>>16;
    *g=(pin[1]*f + pinnext[1]*frac)>>16;
    *b=(pin[2]*f + pinnext[2]*frac)>>16;
    *a=(pin[3]*f + pinnext[3]*frac)>>16;
}

static void
BilinearFilterI(int *r, int *g, int *b, int *a,
                const unsigned char *pin, const unsigned char *pinnext,
                unsigned int xfrac, unsigned int yfrac)
{
    const unsigned int f4=(xfrac*yfrac)>>16;
    const unsigned int f3=yfrac-f4; // (1.0-xfrac)*yfrac;
    const unsigned int f2=xfrac-f4; // xfrac*(1.0-yfrac);
    const unsigned int f1=65536-yfrac-xfrac+f4; // (1.0-xfrac)*(1.0-yfrac);
#define DOCHAN(output, inchan) \
(output)=(pin[(inchan)]*f1 + pin[4+(inchan)]*f2 + pinnext[(inchan)]*f3 + pinnext[4+(inchan)]*f4)>>16;
    DOCHAN(*r,0)
    DOCHAN(*g,1)
    DOCHAN(*b,2)
    DOCHAN(*a,3)
#undef DOCHAN
}

#define __DOPIX(pout,r,g,b,a) \
pout[0] = r; \
pout[1] = g; \
pout[2] = b; \
pout[3] = a;

static void
scaleBlit(unsigned char *dest, const unsigned char *src,
          int w, int h,
          int icurx, int icury, int idx, int idy,
          unsigned int clipright, unsigned int clipbottom,
          int src_span, int dest_span)
{
    // Bilinear
    while (h--)
    {
        const unsigned int cury = icury >> 16;
        const int yfrac=icury&65535;
        int curx=icurx;
        const unsigned char *inptr=src + cury * src_span;
        unsigned char *pout=dest;
        int n=w;
        if (cury < clipbottom-1)
        {
            while (n--)
            {
                const unsigned int offs=curx >> 16;
                const unsigned char *pin = inptr + offs*sizeof(int);
                if (offs<clipright-1)
                {
                    int r,g,b,a;
                    BilinearFilterI(&r,&g,&b,&a,pin,pin+src_span,curx&0xffff,yfrac);
                    __DOPIX(pout,r,g,b,a)
                }
                else if (offs==clipright-1)
                {
                    int r,g,b,a;
                    LinearFilterI(&r,&g,&b,&a,pin,pin+src_span,yfrac);
                    __DOPIX(pout,r,g,b,a)
                }
                    
                pout += sizeof(int)/sizeof(unsigned char);
                curx+=idx;
            }
        }
        else if (cury == clipbottom-1)
        {
            while (n--)
            {
                const unsigned int offs=curx >> 16;
                const unsigned char *pin = inptr + offs*sizeof(int);
                if (offs<clipright-1)
                {
                    int r,g,b,a;
                    LinearFilterI(&r,&g,&b,&a,pin,
                                  pin+sizeof(int)/sizeof(unsigned char),
                                  curx&0xffff);
                    __DOPIX(pout,r,g,b,a)
                }
                else if (offs==clipright-1)
                {
                    __DOPIX(pout,pin[0],pin[1],pin[2],pin[3])
                }
                    
                pout += sizeof(int)/sizeof(unsigned char);
                curx+=idx;
            }
        }
        dest+=dest_span;
        icury+=idy;
    }
}

void
BLBitmap::ScaledBlit(BLBitmap *dest, const BLBitmap *src,
                     int dstx, int dsty, int dstw, int dsth,
                     float srcx, float srcy, float srcw, float srch)
{
    if (!dest || !src || !dstw || !dsth)
        return;
    
    if (dstw<0)
    {
        dstw=-dstw;
        dstx-=dstw;
        srcx+=srcw;
        srcw=-srcw;
    }
    if (dsth<0)
    {
        dsth=-dsth;
        dsty-=dsth;
        srcy+=srch;
        srch=-srch;
    }
    
    double xadvance = srcw / dstw;
    double yadvance = srch / dsth;
    
    if (dstx < 0)
    {
        srcx -= (float) (dstx*xadvance);
        dstw+=dstx;
        dstx=0;
    }
    if (dsty < 0)
    {
        srcy -= (float) (dsty*yadvance);
        dsth+=dsty;
        dsty=0;
    }
    
    const int destbm_w = dest->GetWidth();
    const int destbm_h = dest->GetHeight();
    if (dstw < 1 || dsth < 1 ||
        dstx >= destbm_w || dsty >= destbm_h)
        return;
    
    if (dstw > destbm_w-dstx)
        dstw=destbm_w-dstx;
    if (dsth > destbm_h-dsty)
        dsth=destbm_h-dsty;
    
    int idx=(int)(xadvance*65536.0);
    int idy=(int)(yadvance*65536.0);
    int icurx=(int) (srcx*65536.0);
    int icury=(int) (srcy*65536.0);
    
#if 1
    // the clip area calculations need to be done fixed point so the results match runtime
    
    if (idx>0)
    {
        if (icurx < 0) // increase dstx, decrease dstw
        {
            int n = (idx-1-icurx)/idx;
            dstw-=n;
            dstx+=n;
            icurx+=idx*n;
        }
        if ((icurx + idx*(dstw-1))/65536 >= src->GetWidth())
        {
            int neww = ((src->GetWidth()-1)*65536 - icurx)/idx;
            if (neww < dstw) dstw=neww;
        }
    }
    else if (idx<0)
    {
        // todo: optimize source-clipping with reversed X axis
    }
    
    if (idy > 0)
    {
        if (icury < 0) // increase dsty, decrease dsth
        {
            int n = (idy-1-icury)/idy;
            dsth-=n;
            dsty+=n;
            icury+=idy*n;
        }
        if ((icury + idy*(dsth-1))/65536 >= src->GetHeight())
        {
            int newh = ((src->GetHeight()-1)*65536 - icury)/idy;
            if (newh < dsth)
                dsth=newh;
        }
    }
    else if (idy<0)
    {
        // todo: optimize source-clipping with reversed Y axis (check icury against src->getHeight(), etc)
    }
    if (dstw<1 || dsth<1)
        return;
#endif
    
    //int dest_span=dest->getRowSpan()*sizeof(LICE_pixel);
    //int src_span=src->getRowSpan()*sizeof(LICE_pixel);
    
    // Looks good
    // NOTE: when trying to do like lice, with line_align=4,
    // span = (m_width+m_linealign)&~m_linealign
    // this is not good (crash).
    int dest_span=dest->GetWidth()*sizeof(int);
    int src_span=src->GetWidth()*sizeof(int);
    
    const unsigned char *psrc = (const unsigned char*)src->GetData();
    unsigned char *pdest = (unsigned char*)dest->GetData();
    if (!psrc || !pdest)
        return;
    
    pdest += dsty*dest_span;
    pdest+=dstx*sizeof(int);
    
    int clip_r=(int)(srcx+MAX(srcw,0)+0.999999);
    int clip_b=(int)(srcy+MAX(srch,0)+0.999999);
    if (clip_r>src->GetWidth())
        clip_r=src->GetWidth();
    if (clip_b>src->GetHeight())
        clip_b=src->GetHeight();
    
    if (clip_r<1||clip_b<1)
        return;
    
    // Bilinear
    if (xadvance>=1.7 && yadvance >=1.7)
    {
        int msc = MAX(idx,idy);
        const int filtsz=msc>(3<<16) ? 5 : 3;
        const int filt_start = - (filtsz/2);
            
        int filter[25]; // 5x5 max
        
        int y;
            
        int *p=filter;
        for(y=0;y<filtsz;y++)
        {
            int x;
            for(x=0;x<filtsz;x++)
            {
                if (x==y && x==filtsz/2)
                    *p++ = 65536; // src pix is always valued at 1.
                else
                {
                    double dx=x+filt_start;
                    double dy=y+filt_start;
                    double v = (msc-1.0) / sqrt(dx*dx+dy*dy); // this needs serious tweaking...
                            
                    if(v<0.0)
                        *p++=0;
                    else if (v>1.0)
                        *p++=65536;
                    else
                        *p++=(int)(v*65536.0);
                }
            }
        }
        
        scaleBlitFilterDown(pdest,psrc,
                            dstw,dsth,
                            icurx,icury,idx,idy,
                            clip_r,clip_b,src_span,dest_span,
                            (const int *)filter,filt_start,filtsz);
    }
    else
    {
        scaleBlit(pdest,psrc,dstw,dsth,icurx,icury,idx,idy,
                  clip_r,clip_b,src_span,dest_span);
    }
}

#endif
