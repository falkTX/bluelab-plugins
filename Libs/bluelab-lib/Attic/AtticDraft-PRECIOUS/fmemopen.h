//
//  fmemopen.h
//  StereoPan
//
//  Created by Apple m'a Tuer on 14/04/17.
//
//

#ifndef StereoPan_fmemopen_h
#define StereoPan_fmemopen_h

#if defined __cplusplus
extern "C" {
#endif
    FILE *fmemopen(void *buf, size_t size, const char *mode);
    
#ifdef __cplusplus
}
#endif

#endif
