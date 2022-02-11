//
//  BLTypes.h
//  UST
//
//  Created by applematuer on 8/18/20.
//
//

#ifndef UST_BLTypes_h
#define UST_BLTypes_h

#define BL_GUI_TYPE_FLOAT 1

#if BL_GUI_TYPE_FLOAT
#define BL_GUI_FLOAT float
/*#define COS cosf
#define SIN sinf
#define TAN tanf
#define ATAN2 atan2f
#define SQRT sqrtf
#define FMOD fmodf*/
#else
#define BL_GUI_FLOAT double
/*#define SIN sin
#define TAN tan
#define ATAN2 atan2
#define SQRTF sqrt
#define FMOD fmod*/
#endif

//
#define BL_TYPE_FLOAT 0

#if !BL_TYPE_FLOAT
#define BL_FLOAT double
#endif

#endif
