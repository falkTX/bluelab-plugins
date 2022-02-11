//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef GraphSwapColor_h
#define GraphSwapColor_h

#ifdef WIN32
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[0]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[2]; \
__RGBA__[3] = tmp[3]; }
#endif

#ifdef __APPLE__
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[0]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[2]; \
__RGBA__[3] = tmp[3]; }
#endif

#ifdef __linux__
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[0]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[2]; \
__RGBA__[3] = tmp[3]; }
#endif

#endif
