#include <IPlugPaths.h>

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
