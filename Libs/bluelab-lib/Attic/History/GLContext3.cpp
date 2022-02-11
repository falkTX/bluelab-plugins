//
//  GLContext3.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#include "resource.h"

#include "GLContext3.h"

#include <stdio.h>
#include <time.h>

#include <GL/glew.h>

//#define GLFW_INCLUDE_GLEXT
//#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#endif

#include <pugl/pugl.h>

// TEST
//#ifdef WIN32
//extern "C" {
//#include <GLWrapper_win.h>
//}
//#endif

// Studio One 4, Windows 7
// - open a GL plugin
// - close it
// => it crashes when calling wglDeleteContext
//
// So don't delete the context...
#define FIX_STUDIO_ONE_WIN_CLOSE_CRASH 1

#ifdef WIN32
static GLboolean
gluCheckExtension(const GLubyte *extName, const GLubyte *extString)
{
	GLboolean flag = GL_FALSE;
	char *word;
	char *lookHere;
	char *deleteThis;

	if (extString == NULL) return GL_FALSE;

	deleteThis = lookHere = (char *)malloc(strlen((const char *)extString) + 1);
	if (lookHere == NULL)
		return GL_FALSE;
	/* strtok() will modify string, so copy it somewhere */
	strcpy(lookHere, (const char *)extString);

	while ((word = strtok(lookHere, " ")) != NULL) {
		if (strcmp(word, (const char *)extName) == 0) {
			flag = GL_TRUE;
			break;
		}
		lookHere = NULL;		/* get next token */
	}
	free((void *)deleteThis);
	return flag;
}
#endif

static void
onEvent(PuglView* view, const PuglEvent* event)
{
    
}

GLContext3::GLContext3(const char *plugName, IGraphics *pGraphics)
{
  mGraphics = pGraphics;

  mContext = NULL;
  
//#ifdef WIN32
//	bool wrapperSuccess = GLWrapperInit();
//	if (!wrapperSuccess)
//		return;
//#endif
    
    // Generate a unique class name and window name
    // (to be sure to not have interference between plugins)
    clock_t c = clock();
    srand(c);
    
    char windowClass[1024];
    long rnd0 = rand();
    sprintf(windowClass, "%s-c-%ld", plugName, rnd0);
    
    char windowName[1024];
    long rnd1 = rand();
    sprintf(windowName, "%s-%ld", plugName, rnd1);
    
    uintptr_t parent = (uintptr_t)pGraphics->GetWindow();
    
    // Init pugl
    
    PuglView* view = puglInit(NULL, NULL);
    puglInitWindowClass(view, windowClass);
    
    puglInitWindowSize(view, 1, 1);
    puglInitWindowMinSize(view, 1, 1);
    
    puglInitContextType(view, PUGL_GL);
    
    // Necessary, otherwise it crashes
    puglSetEventFunc(view, onEvent);
    
    // Niko
    puglInitTransientFor(view, parent);
    
    puglCreateWindow(view, windowName);
    
    // Hide window
    puglHideWindow(view);

    if(glewInit() != GLEW_OK)
	{
		puglDestroy(view);
	
		return;
	}

    // GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();

	// Check that version is 2 or greater
	// (this avoids a crash if the version is 1.1)
	const GLubyte *version = glGetString(GL_VERSION);
	if (version == NULL)
	{
		puglDestroy(view);

		return;
	}

	int major;
	int minor;
	sscanf((const char *)version, "%d.%d", &major, &minor);

	if (major < 2)
	{
		puglDestroy(view);

		return;
	}

    // Check OpenGL functionality:
    const GLubyte *glstring = glGetString(GL_EXTENSIONS);
    
    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring))
    {
        return;
    }

    mContext = view;
}

GLContext3::~GLContext3()
{
    if (mContext != NULL)
    {
        PuglView* view = (PuglView*)mContext;
     
	bool customDestroy = false;

#if FIX_STUDIO_ONE_WIN_CLOSE_CRASH
#ifdef WIN32
	if (mGraphics != NULL)
	  {
	    IPlugBase *plug = mGraphics->GetPlug();
	    if (plug != NULL)
	      {
		if (plug->GetHost() == kHostStudioOne)
		  customDestroy = true;
	      }
	  }
#endif
#endif
        
	if (!customDestroy)
	  // Normal behaviour
	  puglDestroy(view);
	else
	  puglDestroyNiko(view, false);
    }
}

bool
GLContext3::IsInitialized()
{
    bool res = (mContext != NULL);
    
    return res;
}

bool
GLContext3::Enter()
{
    if (mContext == NULL)
        return false;
    
    PuglView* view = (PuglView*)mContext;
    puglEnterContext(view);
    
    return true;
}

void
GLContext3::Leave()
{
    if (mContext == NULL)
        return;
    
    PuglView* view = (PuglView*)mContext;
    puglLeaveContext(view, true/*false*/);
}

void
GLContext3::DebugPrintGLError()
{
    GLenum error = glGetError();
    
	if (error != GL_NO_ERROR)
	{
		const GLubyte *errorStr = gluErrorString(error);
        
		fprintf(stderr, "GLError: %s\n", errorStr);
	}
}
