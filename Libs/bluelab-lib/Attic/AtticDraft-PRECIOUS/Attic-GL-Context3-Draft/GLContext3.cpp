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

#include <GL/glew.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#endif

#include <pugl.h>

#include <Debug.h>

static void
onEvent(PuglView* view, const PuglEvent* event)
{
    
}

GLContext3::GLContext3(const char *plugName, IGraphics *pGraphics)
{
    mContext = NULL;
    
    // TODO:
    // - very small window
    // - hide window
    
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
    
    PuglView* view = puglInit(NULL, NULL);
    puglInitWindowClass(view, windowClass);
    
    puglInitWindowSize(view, 512, 512);
    puglInitWindowMinSize(view, 256, 256);
    
    puglInitContextType(view, PUGL_GL);
    
    //puglIgnoreKeyRepeat(view, ignoreKeyRepeat);
    
    puglSetEventFunc(view, onEvent);
    
    // TEST
    puglInitTransientFor(view, parent);
    
    puglCreateWindow(view, windowName);
    
    // TEST
    //puglShowWindow(view);
    
    puglEnterContext(view);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    puglLeaveContext(view, false);
    
    puglShowWindow(view);
    
   // while (true)
   // {
        //puglWaitForEvent(view);
        //puglProcessEvents(view);
   // }
    
    if(glewInit() != GLEW_OK)
		return;
    
    // TEST
    //puglHideWindow(view);
    mContext = view;
}

GLContext3::~GLContext3()
{
    if (mContext != NULL)
    {
        PuglView* view = (PuglView*)mContext;
        
        puglDestroy(view);
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
