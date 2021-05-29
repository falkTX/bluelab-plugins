#include <IPlugPaths.h>
#include <GUIHelper12.h>
#include <AudioFile.h>

#include "BLUtilsFile.h"

char *
BLUtilsFile::GetFileExtension(const char *fileName)
{
	char *ext = (char *)strrchr(fileName, '.');
    
    // Here, we have for example ".wav"
    if (ext != NULL)
    {
        if (strlen(ext) > 0)
            // Skip the dot
            ext = &ext[1];
    }
        
    return ext;
}

char *
BLUtilsFile::GetFileName(const char *path)
{
    if (path == NULL)
        return NULL;
    
	char *fileName = (char *)strrchr(path, '/');

	// Here, we have for example "/file.wav"
	if (fileName != NULL)
	{
		if (strlen(fileName) > 0)
			// Skip the dot
			fileName = &fileName[1];
	}
	else
	{
		// There were no "/" in the path,
		// we already had the correct file name
		return (char *)path;
	}

	return fileName;
}

void
BLUtilsFile::GetFilePath(const char *fullPath, char path[FILENAME_SIZE],
                         bool keepLastSlash)
{
    memset(path, '\0', FILENAME_SIZE);
    
    char *fileName = GetFileName(fullPath);

    int pathSize = fileName - fullPath;
    if (!keepLastSlash)
        pathSize--;
    if (pathSize <= 0)
        return;
    
    strncpy(path, fullPath, pathSize);
}

long
BLUtilsFile::GetFileSize(const char *fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
        return 0;
    
    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    fclose(fp);
    
    return sz;
}

void
BLUtilsFile::GetPreferencesFileName(const char *bundleName,
                                    char resultFileName[FILENAME_SIZE])
{
    // Path
    //
    // See: IPlug/IPlugPaths.h
    WDL_String iniPathStr;
    INIPath(iniPathStr, bundleName);

#if 0
    // Get the path witthout "settings.ini"
    const char *iniFullPath = iniPathStr.Get();
    int iniFullPathLen = iniPathStr.GetLength();

    int fileNameSize = strlen("settings.ini");
    if (iniFullPathLen <= fileNameSize)
        return;
    
    char iniPath[512];
    memset(iniPath, '\0', 512);
    strncpy(iniPath, iniFullPath, iniFullPathLen - fileNameSize);

    // Result
    sprintf(resultFileName, "%s%s.prf", iniPath, BUNDLE_NAME);
#endif

    // Result
    const char *iniPath = iniPathStr.Get();
#ifdef WIN32
    sprintf(resultFileName, "%s\\%s.prf", iniPath, BUNDLE_NAME);
#else // Linux or Mac
    sprintf(resultFileName, "%s/%s.prf", iniPath, BUNDLE_NAME);
#endif
}

template <typename FLOAT_TYPE>
void
BLUtilsFile::AppendValuesFile(const char *fileName,
                              const WDL_TypedBuf<FLOAT_TYPE> &values, char delim)
{
    // Compute the file size
    FILE *fileSz = fopen(fileName, "a+");
    if (fileSz == NULL)
        return;
    
    fseek(fileSz, 0L, SEEK_END);
    long size = ftell(fileSz);
    
    fseek(fileSz, 0L, SEEK_SET);
    fclose(fileSz);
    
    // Write
    FILE *file = fopen(fileName, "a+");
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        if ((i == 0) && (size == 0))
            fprintf(file, "%g", values.Get()[i]);
        else
            fprintf(file, "%c%g", delim, values.Get()[i]);
    }
    
    //fprintf(file, "\n");
           
    fclose(file);
}
template void BLUtilsFile::AppendValuesFile(const char *fileName,
                                            const WDL_TypedBuf<float> &values,
                                            char delim);
template void BLUtilsFile::AppendValuesFile(const char *fileName,
                                            const WDL_TypedBuf<double> &values,
                                            char delim);

template <typename FLOAT_TYPE>
void
BLUtilsFile::AppendValuesFileBin(const char *fileName,
                                 const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    // Write
    FILE *file = fopen(fileName, "ab+");
    
    fwrite(values.Get(), sizeof(FLOAT_TYPE), values.GetSize(), file);
    
    fclose(file);
}
template void BLUtilsFile::AppendValuesFileBin(const char *fileName,
                                               const WDL_TypedBuf<float> &values);
template void BLUtilsFile::AppendValuesFileBin(const char *fileName,
                                               const WDL_TypedBuf<double> &values);

template <typename FLOAT_TYPE>
void
BLUtilsFile::AppendValuesFileBinFloat(const char *fileName,
                                      const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    WDL_TypedBuf<float> floatBuf;
    floatBuf.Resize(values.GetSize());
    for (int i = 0; i < floatBuf.GetSize(); i++)
    {
        FLOAT_TYPE val = values.Get()[i];
        floatBuf.Get()[i] = val;
    }
    
    // Write
    FILE *file = fopen(fileName, "ab+");
    
    fwrite(floatBuf.Get(), sizeof(float), floatBuf.GetSize(), file);
    
    fclose(file);
}
template void
BLUtilsFile::AppendValuesFileBinFloat(const char *fileName,
                                      const WDL_TypedBuf<float> &values);
template void
BLUtilsFile::AppendValuesFileBinFloat(const char *fileName,
                                      const WDL_TypedBuf<double> &values);

void *
BLUtilsFile::AppendValuesFileBinFloatInit(const char *fileName)
{
    // Write
    FILE *file = fopen(fileName, "ab+");
 
    return file;
}

template <typename FLOAT_TYPE>
void
BLUtilsFile::AppendValuesFileBinFloat(void *cookie,
                                      const WDL_TypedBuf<FLOAT_TYPE> &values)
{
    // Write
    FILE *file = (FILE *)cookie;
    
    WDL_TypedBuf<float> floatBuf;
    floatBuf.Resize(values.GetSize());
    for (int i = 0; i < floatBuf.GetSize(); i++)
    {
        FLOAT_TYPE val = values.Get()[i];
        floatBuf.Get()[i] = val;
    }
    
    fwrite(floatBuf.Get(), sizeof(float), floatBuf.GetSize(), file);
    
    // TEST
    fflush(file);
}
template void
BLUtilsFile::AppendValuesFileBinFloat(void *cookie,
                                      const WDL_TypedBuf<float> &values);
template void
BLUtilsFile::AppendValuesFileBinFloat(void *cookie,
                                      const WDL_TypedBuf<double> &values);

void
BLUtilsFile::AppendValuesFileBinFloatShutdown(void *cookie)
{
    fclose((FILE *)cookie);
}

bool
BLUtilsFile::PromptForFileOpenAudio(Plugin *plug,
                                    const char currentLoadPath[FILENAME_SIZE],
                                    WDL_String *resultFileName)
{
    // Get the location where we will open the file selector
    char *dir = "";

#ifdef __linux__
    // By default, open the desktop
    // (necessary to set "~" to open on the Desktop,
    // in addition to "cd" just before zenity)
    dir = "~";
#endif
    
    if (strlen(currentLoadPath) > 0)
        dir = (char *)currentLoadPath;

    // Open the file selector
    //
    
    bool fileOk = GUIHelper12::PromptForFile(plug, EFileAction::Open,
                                             resultFileName,
                                             //"",
                                             dir,
#ifndef __linux__
                                                   
#if AUDIOFILE_USE_FLAC
                                             "wav aif aiff flac"
#else
                                             "wav aif aiff"
#endif
                                                   
#else // __linux__
                                                   
#if AUDIOFILE_USE_FLAC
                                             "*.wav *.aif *.aiff *.flac"
#else
                                             "*.wav *.aif *.aiff"
#endif
                                                   
#endif
                                             );

    return fileOk;
}

bool
BLUtilsFile::PromptForFileSaveAsAudio(Plugin *plug,
                                      const char currentLoadPath[FILENAME_SIZE],
                                      const char currentSavePath[FILENAME_SIZE],
                                      const char *currentFileName,
                                      WDL_String *resultFileName)
{
    // Get the location we will open
    //
    
    char fullDir[FILENAME_SIZE];
    memset(fullDir, '\0', FILENAME_SIZE);
    
    char *dir = NULL;
    if (strlen(currentSavePath) > 0)
        // Save path is defined => use it
    {
        dir = (char *)currentSavePath;
    }
    else
        // Save path is not defined
        // => use load path if possible
    {
        if (strlen(currentLoadPath) > 0)
        {
            dir = (char *)currentLoadPath;
        }
    }

    // Append the current file name
    // So e.g when "save as", we will have the current file name already filled
    if (dir != NULL)
    {
        if (currentFileName != NULL)
            sprintf(fullDir, "%s%s", dir, currentFileName);
        else
            strcpy(fullDir, dir);
    }

#ifdef __linux__
    // By default, open the desktop
    // (necessary to set "~" to open on the Desktop,
    // in addition to "cd" just before zenity)
    if (dir == NULL)
        dir = "~";
#endif

    // Open file selector
    //
    
    bool fileOk = GUIHelper12::PromptForFile(plug, EFileAction::Save,
                                             resultFileName,
                                             //"",
                                             fullDir,
#ifndef __linux__
                                                   
#if AUDIOFILE_USE_FLAC
                                             "wav aif aiff flac"
#else
                                             "wav aif aiff"
#endif

#else // __linux__

#if AUDIOFILE_USE_FLAC
                                             "*.wav *.aif *.aiff *.flac"
#else
                                             "*.wav *.aif *.aiff"
#endif
                                                   
#endif
                                                   
                                             );

    return fileOk;
}

#if 0
bool
BLUtilsFile::PromptForFileSaveNoDirAudio(Plugin *plug,
                                         WDL_String *resultFileName)
{
    bool fileOk = GUIHelper12::PromptForFile(plug, EFileAction::Save,
                                             resultFileName, "",
#ifndef __linux__
                                                   
#if AUDIOFILE_USE_FLAC
                                             "wav aif aiff flac"
#else
                                             "wav aif aiff"
#endif

#else // __linux__
                                                   
#if AUDIOFILE_USE_FLAC
                                             "*.wav *.aif *.aiff *.flac"
#else
                                             "*.wav *.aif *.aiff"
#endif
                                                   
#endif
                                             );

    return fileOk;
}
#endif

bool
BLUtilsFile::PromptForFileOpenImage(Plugin *plug,
                                    const char currentLoadPath[FILENAME_SIZE],
                                    WDL_String *resultFileName)
{
    // Get the location where we will open the file selector
    char *dir = "";

#ifdef __linux__
    // By default, open the desktop
    // (necessary to set "~" to open on the Desktop,
    // in addition to "cd" just before zenity)
    dir = "~";
#endif
    
    if (strlen(currentLoadPath) > 0)
        dir = (char *)currentLoadPath;

    // Open the file selector
    //
    
    bool fileOk = GUIHelper12::PromptForFile(plug, EFileAction::Open,
                                             resultFileName,
                                             //"",
                                             dir,
#ifndef __linux__                                                  
                                             "png jpg jpeg"
#else // __linux__
                                             "*.png *.jpg *.jpeg"
#endif
                                             );

    return fileOk;
}
