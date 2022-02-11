//
//  BLProfiler.h
//  BL-Spectrogram
//
//  Created by Pan on 13/04/18.
//
//

#ifndef BL_Spectrogram_BLProfiler_h
#define BL_Spectrogram_BLProfiler_h

// Change this to activate !
#define BL_PROFILE 0 //1

#if BL_PROFILE
#include <stdio.h>

#include <BlaTimer.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifndef WIN32 // Mac
#define BASE_FILE "/Users/applematuer/Documents/BlueLabAudio-Debug/"
#else
#define BASE_FILE "C:/Tmp/BlueLabAudio-Debug/"
#endif

extern void __ProfilerAppendMessage__(const char *filename, const char *message);
#endif

#define PROFILE_GRANULARITY 50

#if BL_PROFILE
#define BL_PROFILE_DECLARE \
BlaTimer __BLTimer__; \
long __BLTimer__Count;
#else
#define BL_PROFILE_DECLARE
#endif

#if BL_PROFILE
#define BL_PROFILE_RESET \
__BLTimer__.Reset(); \
__BLTimer__Count = 0;
#else
#define BL_PROFILE_RESET
#endif

#if BL_PROFILE
#define BL_PROFILE_BEGIN \
__BLTimer__.Start();
#else
#define BL_PROFILE_BEGIN
#endif

#if BL_PROFILE
#define BL_PROFILE_END \
__BLTimer__.Stop(); \
__BLTimer__Count++; \
if (__BLTimer__Count % PROFILE_GRANULARITY == 0) { \
long t = __BLTimer__.Get(); \
char message[512]; \
sprintf(message, "%s: %g ms", __FILENAME__, ((BL_FLOAT)t)/PROFILE_GRANULARITY); \
__ProfilerAppendMessage__(BUNDLE_NAME"-PROFILE.txt", message); \
fprintf(stderr, "%s\n", message); \
__BLTimer__.Reset(); }
#else
#define BL_PROFILE_END
#endif

#endif // BLProfiler.h
