/*
	File: setfileicon.m
	Version: 1.0
	Author: Damien Bobillot (damien.bobillot.2002_setfileicon@m4x.org)
	Licence: GNU GPL
	Compatibility: Mac OS X 10.4 (may work on MacOS X 10.0)

	TODO.

	Syntaxe : setfileicon <icon> <file>

	06/06/2006 - 1.0 - First version
*/

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

OSStatus SetCustomIconForFileOrFolder(NSString* icon_path, NSString* file_path);
OSStatus DeleteCustomIconForFileOrFolder(NSString* file_path);

int main(int argc, const char* argv[]) {
	NSAutoreleasePool* pool = [NSAutoreleasePool new];
	
	// Parse arguments
	BOOL useNSWorkspace = NO;
	BOOL deleteCustomIcon = NO;	
	if(argc != 3) {
		fprintf(stderr, "usage: setfileicon <icon> <file>\n");
		return 1;
	}
	deleteCustomIcon = strcmp(argv[1], "-d") == 0;
	
	NSString* icon_path = [NSString stringWithUTF8String:argv[1]];
	NSString* file_path = [NSString stringWithUTF8String:argv[2]];
	
	// Delete the customized icon
	if(deleteCustomIcon) {
		DeleteCustomIconForFileOrFolder(file_path);
		
	// Use NSWorkspace method (for any image type, but seems to have a bug)
	} else if(useNSWorkspace) {
		NSImage* icon_image = [[NSImage alloc] initWithContentsOfFile:icon_path];
		if(icon_image == nil) { fprintf(stderr, "setfileicon: cannot load image file\n"); return 1; }
		
		BOOL succeded = [[NSWorkspace sharedWorkspace] setIcon:icon_image forFile:file_path options:NSExclude10_4ElementsIconCreationOption];
		if(!succeded) { fprintf(stderr, "setfileicon: cannot set file icon\n"); return 1; }
		
	// Use manual method (only work with .icns files)
	} else {
		SetCustomIconForFileOrFolder(icon_path, file_path);
	}
	
	[pool release];
	return 0;
}

OSStatus DeleteCustomIconForFileOrFolder(NSString* file_path) {
	OSStatus err;
	FSRef resFile;
	err = FSPathMakeRef((UInt8*)[file_path fileSystemRepresentation], &resFile, false);
	if( err != noErr ) {
		fprintf(stderr,"Cannot find file %s : error #%d\n",[file_path fileSystemRepresentation],(int)err);
		return err;
	}
	
	FSCatalogInfo catInfo;
	err = FSGetCatalogInfo(&resFile, kFSCatInfoFinderInfo, &catInfo, NULL, NULL, NULL);
	if(err == noErr)
        ((FileInfo*)&catInfo.finderInfo)->finderFlags &= ~kHasCustomIcon;
	if(err == noErr)
        err = FSSetCatalogInfo(&resFile, kFSCatInfoFinderInfo, &catInfo);
	if( err != noErr ) {
		fprintf(stderr,"Cannot suppress custom icon flag for %s : error #%d\n",[file_path fileSystemRepresentation],(int)err);
		return err;
	}
	
	return noErr;
}

OSStatus SetCustomIconForFileOrFolder(NSString* icon_path, NSString* file_path) {
	OSStatus err = noErr;
	
	// Load icon data
	NSData* icns_data = [NSData dataWithContentsOfFile:icon_path];
	if(icns_data == nil) {
		fprintf(stderr, "setfileicon: cannot load icon file\n");
		return fnfErr;
	}
	
	// Compute where the icon will by added
	NSString* dst_icon_file = file_path;
	BOOL isFolder;
	if(![[NSFileManager defaultManager] fileExistsAtPath:file_path isDirectory:&isFolder]) {
		fprintf(stderr,"Cannot find file %s : error #%d\n",[file_path fileSystemRepresentation],(int)err);
		return fnfErr;
	}
	if(isFolder) {
		dst_icon_file = [file_path stringByAppendingPathComponent:@"Icon\r"];
		if(![[NSFileManager defaultManager] createFileAtPath:dst_icon_file contents:[NSData data] attributes:NULL]) {
			fprintf(stderr,"Cannot create icon file %s : error #%d\n",[dst_icon_file fileSystemRepresentation],(int)err);
			return paramErr;
		}
	}
	
    // Get new and old file spectification records for the icon file
    FSSpec resFileOld;
    FSRef resFile;
    err = FSPathMakeRef((UInt8*)[dst_icon_file fileSystemRepresentation], &resFile, false);
    if(err == noErr)
        err = FSGetCatalogInfo(&resFile, 0, NULL, NULL, &resFileOld, NULL);
    if( err != noErr ) {
        fprintf(stderr,"Cannot find file %s : error #%d\n",[dst_icon_file fileSystemRepresentation],(int)err);
        return err;
    }
    
    // #bluelab
#if 0 // Origin
	// Create the resource fork (only if needed)
	//FSpCreateResFile(&resFileOld, '\?\?\?\?', '\?\?\?\?', smRoman);
    FSpCreateResFile(&resFileOld, '\?\?\?\?', '\?\?\?\?', smRoman);
	if((err = ResError()) != noErr ) {
		fprintf(stderr,"Cannot create resource fork at %s : error #%d\n",[dst_icon_file fileSystemRepresentation],(int)err);
		return err;
	}
	
	// Open the resource fork
	SInt16 refnum;
	HFSUniStr255 forkName;
	FSGetResourceForkName(&forkName);
	err = FSOpenResourceFile(&resFile,forkName.length,forkName.unicode,fsRdWrPerm,&refnum);
	if( err != noErr ) {
		fprintf(stderr,"Cannot open resource fork at %s : error #%d\n",[dst_icon_file fileSystemRepresentation],(int)err);
		return err;
	}
#endif
    
#if 1 // New
    // See: https://github.com/BOINC/boinc/blob/master/api/mac_icon.cpp
    HFSUniStr255 forkName;
    
    SInt16 refnum = FSOpenResFile(&resFile, fsRdWrPerm);
    err = ResError();
    
    if (err == eofErr) { /* EOF, resource fork/file not found */
        // If we set file type and signature to non-NULL, it makes OS mistakenly
        // identify file as a classic application instead of a UNIX executable.
        //            FSpCreateResFile(&fsref, 0, 0, smRoman);
        err = FSGetResourceForkName(&forkName);
        if (err == noErr) {
            err = FSCreateResourceFork(&resFile, forkName.length, forkName.unicode, 0);
        }
        //err = ResError(); // #bluelab comment (prevent failure when targetting a folder (e.g AU plugin)
        if (err == noErr) {
            //                rref = FSpOpenResFile(&fsspec, fsRdWrPerm);
            refnum = FSOpenResFile(&resFile, fsRdWrPerm);
            err = ResError();
        }
    }
#endif

	// Add the icns resource #-16455
	Handle icnsResource = NewHandle([icns_data length]);
	HLock(icnsResource);
	memcpy(*icnsResource, [icns_data bytes], [icns_data length]);
	HUnlock(icnsResource);
	AddResource(icnsResource,'icns',-16455,"\p");
	if((err = ResError()) == noErr )
        CloseResFile(refnum);
	if((err = ResError()) != noErr ) {
		fprintf(stderr,"Cannot write resource fork at %s : error #%d\n",[dst_icon_file fileSystemRepresentation],(int)err);
		return err;
	}
    
	// Setup the "hidden" flag for folder
	FSCatalogInfo catInfo;
	if(isFolder) {
		err = FSGetCatalogInfo(&resFile, kFSCatInfoFinderInfo, &catInfo, NULL, NULL, NULL);
		if(err == noErr)
            ((FileInfo*)&catInfo.finderInfo)->finderFlags |= kIsInvisible;
		if(err == noErr)
            err = FSSetCatalogInfo(&resFile, kFSCatInfoFinderInfo, &catInfo);
		if( err != noErr ) {
			fprintf(stderr,"Cannot set hidden flag for %s : error #%d\n",[dst_icon_file fileSystemRepresentation],(int)err);
			return err;
		}
	}
	
	// Setup the "custom icon" flag
	if(isFolder) {
		err = FSPathMakeRef((UInt8*)[file_path fileSystemRepresentation], &resFile, false);
		if( err != noErr ) {
			fprintf(stderr,"Cannot find file %s : error #%d\n",[file_path fileSystemRepresentation],(int)err);
			return err;
		}
	}
	
	err = FSGetCatalogInfo(&resFile, kFSCatInfoFinderInfo, &catInfo, NULL, NULL, NULL);
	if(err == noErr)
        ((FileInfo*)&catInfo.finderInfo)->finderFlags |= kHasCustomIcon;
	if(err == noErr)
        err = FSSetCatalogInfo(&resFile, kFSCatInfoFinderInfo, &catInfo);
	if( err != noErr ) {
		fprintf(stderr,"Cannot set custom icon flag for %s : error #%d\n",[file_path fileSystemRepresentation],(int)err);
		return err;
	}

	return noErr;
}
