#ifndef GETTIMEOFDAY_H
#define GETTIMEOFDAY_H

#ifdef WIN32
extern int gettimeofday(struct timeval* p, void* tz);
#endif

#endif
