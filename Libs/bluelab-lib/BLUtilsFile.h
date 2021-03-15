#ifndef BL_UTILS_FILE_H
#define BL_UTILS_FILE_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class BLUtilsFile
{
 public:
    // File extension
    static char *GetFileExtension(const char *fileName);

	// Get file name from full path
	static char *GetFileName(const char *path);
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFile(const char *fileName,
                                 const WDL_TypedBuf<FLOAT_TYPE> &values,
                                 char delim = ' ');
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFile(const char *fileName,
                                 const WDL_TypedBuf<float> &values,
                                 char delim = ' ');
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFileBin(const char *fileName,
                                    const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    //static void AppendValuesFileBin(const char *fileName, const WDL_TypedBuf<float> &values);
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFileBinFloat(const char *fileName,
                                         const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    // Optimized version (3 methods)
    static void *AppendValuesFileBinFloatInit(const char *fileName);
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFileBinFloat(void *cookie,
                                         const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    static void AppendValuesFileBinFloatShutdown(void *cookie);
};

#endif
