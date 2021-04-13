#ifndef BL_UTILS_FILE_H
#define BL_UTILS_FILE_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#define FILENAME_SIZE 2048 //1024

using namespace iplug;


class BLUtilsFile
{
 public:
    // File extension
    static char *GetFileExtension(const char *fileName);

	// Get file name from full path
	static char *GetFileName(const char *path);

    // Squeeze the file bane
    static void GetFilePath(const char *fullPath, char path[FILENAME_SIZE],
                            bool keepLastSlash = false);
    
    static long GetFileSize(const char *fileName);
        
    static void GetPreferencesFileName(const char *bundleName,
                                       char resultFileName[FILENAME_SIZE]);
                                              
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

    // Prompt for file
    static bool PromptForFileOpenAudio(Plugin *plug,
                                       const char currentLoadPath[FILENAME_SIZE],
                                       WDL_String *resultFileName);

    static bool PromptForFileSaveAsAudio(Plugin *plug,
                                         const char currentLoadPath[FILENAME_SIZE],
                                         const char currentSavePath[FILENAME_SIZE],
                                         const char *currentFileName,
                                         WDL_String *resultFileName);

    //static bool PromptForFileSaveNoDirAudio(Plugin *plug,
    //                                        WDL_String *resultFileName);

    static bool PromptForFileOpenImage(Plugin *plug,
                                       const char currentLoadPath[FILENAME_SIZE],
                                       WDL_String *resultFileName);
};

#endif
