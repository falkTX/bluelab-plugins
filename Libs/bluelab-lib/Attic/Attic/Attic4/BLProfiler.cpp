#include "BLProfiler.h"

#if BL_PROFILE

void __ProfilerAppendMessage__(const char *filename, const char *message)
{
    char fullFilename[512];
    sprintf(fullFilename, BASE_FILE"%s", filename);
    FILE *file = fopen(fullFilename, "a+");
	if (file != NULL)
		fprintf(file, "%s\n", message);
    fclose(file);
}

#endif