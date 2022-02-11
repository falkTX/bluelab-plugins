#ifndef GETTIMEOFDAY_H
#define GETTIMEOFDAY_H

#ifdef WIN32
//#ifndef _TIMEVAL_DEFINED /* also in winsock[2].h */
//#define _TIMEVAL_DEFINED
//struct timeval {
//    long tv_sec;
//    long tv_usec;
//};
//#endif /* _TIMEVAL_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

	extern int gettimeofday(struct timeval* p, void* tz);

#ifdef __cplusplus
}
#endif

#endif

#endif
