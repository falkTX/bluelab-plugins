//
//  GLContext.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

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
		glfwDestroyWindow((GLFWwindow *)mGLContext);
#else
    NOT IMLEMENTED
#endif
#endif
    
    glfwTerminate();

	mInstance = NULL;
}

void 
GLContext::Ref()
{
	if (mInstance == NULL)
		GLContext::Get();

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

GLContext::GLContext()
{
	mGLContext = NULL;
}

static void
glfwErrorCb(int error, const char* desc)
{
	fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

bool 
GLContext::Init()
{
	// Debug
	glfwSetErrorCallback(glfwErrorCb);
    
    if (!glfwInit())
		return false;
    
#ifndef _WIN32 // don't require this on win32, and works with more cards
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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
	
	//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
	
	//
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	
	mGLContext = glfwCreateWindow(640, 480, "BlueLabAudio", NULL, NULL);
    
	if (mGLContext == NULL)
	{
		glfwTerminate();
		
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
