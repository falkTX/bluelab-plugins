//
//  GLContext.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#include "resource.h"

#include "GLContext.h"

#include <stdio.h>

#include <GL/glew.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#endif

GLContext *GLContext::mInstance = NULL;
int GLContext::mRefCount = 0;

GLContext *
GLContext::Get()
{
	if (mInstance == NULL)
	{
		mInstance = new GLContext();

		if (!mInstance->Init())
		{
			delete mInstance;

			mInstance = NULL;
		}
	}

	return mInstance;
}

GLContext::~GLContext()
{
	if (mGLContext != NULL)
#ifdef __APPLE__
		CGLDestroyContext((CGLContextObj)mGLContext);
#else
#ifdef WIN32
	{
		glfwDestroyWindow((GLFWwindow *)mGLContext);
    
		// Just in case
		// Added for crash Mixcraft on Windows
		mGLContext = NULL;

		// FIX: Ableton 10 win 10: insert an Impulses, quit Ableton => it crashes while quitting
		// (not fixed)
		
		// FIX: Ableton 10 Win 10 : fixes crash when quitting
		// Comment terminate to avoid init(), terminate(), init()
		//glfwTerminate();
	}
#else
    NOT IMLEMENTED
#endif
#endif

	mInstance = NULL;
}

void 
GLContext::Ref()
{
	if (mInstance == NULL)
		GLContext::Get();

	// FIX: Ableton 10 win 10: vst crash at scan
	//
	// If instantiation failed, to not increase ref count
	if (mInstance == NULL)
		return;

	mRefCount++;
}

void 
GLContext::Unref()
{
	mRefCount--;

	if (mRefCount <= 0)
	{
		if (mInstance != NULL)
			delete mInstance;

		mInstance = NULL;

		mRefCount = 0;
	}
}

bool
GLContext::Bind()
{
	// FIX: Ableton 10 Windows 10: crash during plug scan (plug not recognized)
	if (mGLContext == NULL)
		return false;

#ifdef __APPLE__
    if (CGLSetCurrentContext((CGLContextObj)mGLContext))
        return false;
#else
#ifdef WIN32
	glfwMakeContextCurrent((GLFWwindow *)mGLContext);
#else
    NOT IMPLEMENTED
#endif
#endif

    return true;
}

void
GLContext::Unbind()
{
#ifdef __APPLE__
    CGLSetCurrentContext(NULL);
#else
#ifdef WIN32
   glfwMakeContextCurrent(NULL);
#else
    NOT IMPLEMENTED
#endif
#endif
}

void
GLContext::SwapBuffers()
{
	// FIX: Ableton 10 Windows 10: crash during plug scan (plug not recognized)
	if (mGLContext == NULL)
		return;

#ifdef WIN32
	glfwSwapBuffers((GLFWwindow *)mGLContext);
#endif
}

void
GLContext::DebugPrintGLError()
{
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
		const GLubyte *errorStr = gluErrorString(error);

		fprintf(stderr, "GLError: %s\n", errorStr);
	}
}

GLContext::GLContext()
{
	mGLContext = NULL;
}

static void
glfwErrorCb(int error, const char* desc)
{
	char buf[1024];
	sprintf(buf, "GLFW error %d: %s\n", error, desc);

	fprintf(stderr, "%s", buf);
}

bool
GLContext::Init()
{
#ifdef WIN32
	//glfwSetErrorCallback(glfwErrorCb);

	if (!glfwInit())
	{
		return false;
	}
#endif

	//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    
#ifdef DEMO_MSAA
	glfwWindowHint(GLFW_SAMPLES, 4);
#endif
    
    // Create the context
#ifdef __APPLE__
    const GLubyte *glstring;
    
    GLint npix;
    CGLPixelFormatObj PixelFormat;
    
    const CGLPixelFormatAttribute attributes[] =
    {
        //kCGLPFAOffScreen,
        //      kCGLPFAColorSize, (CGLPixelFormatAttribute)8,
        //      kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
        kCGLPFAAccelerated, (CGLPixelFormatAttribute)0
    };
    
    // Create context if none exists
    CGLChoosePixelFormat(attributes, &PixelFormat, &npix);
    
    if (PixelFormat == NULL)
    {
        return false;
    }
    
    CGLCreateContext(PixelFormat, NULL, (CGLContextObj *)&mGLContext);
    
    if (mGLContext == NULL)
    {
        return false;
    }
    
    // Set the current context
    
    if (!Bind())
        return false;
    
    // Check OpenGL functionality:
    glstring = glGetString(GL_EXTENSIONS);
    
    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring))
    {
        return false;
    }
#else
#ifdef WIN32
	// pixel format ? 

	// TEST
	//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	//mGLContext = glfwCreateWindow(640, 480, "BlueLabAudio", NULL, NULL);
	
	// FIX: Mixcraft, Windows: create an AutoGain, play, create a Denoiser, play, delete the Denoiser -> it crashes
	// (tested with VST3 format for all)
	//
	// Add the plug bundle name to the window name
	// This avoids deleting the window of a plug A when a plug B is deleted
	//
	// The singleton and ref count mechanism works only between plugins of same type
	// (e.g several AutoGains). It doesn't work on windows between plugs with different names
	// Moreover, glfw can be instantiated only one time in the same "process".
	// and between several plugs with the same name, we are on the same "process" (in Mixcraft at least

	mGLContext = glfwCreateWindow(256, 256, "BL-OGL-Win-"BUNDLE_NAME, NULL, NULL);

	if (mGLContext == NULL)
	{
		// FIX: Ableton 10 Win 10 : fixes crash when quitting
		// Comment terminate to avoid init(), terminate(), init()
		//glfwTerminate();

		return false;
	}

	if (!Bind())
		return false;

	// Check OpenGL functionality:
	if (!glfwExtensionSupported("GL_EXT_framebuffer_object"))
	{
		return false;
	}

#else
    NOT IMPLEMENTED
#endif
#endif
    
//#ifdef NANOVG_GLEW
	glewExperimental = GL_TRUE;

	if(glewInit() != GLEW_OK)
		return false;

	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();
//#endif

	return true;
}
