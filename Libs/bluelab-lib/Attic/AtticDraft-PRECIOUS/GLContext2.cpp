//
//  GLContext.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#include "GLContext2.h"

#include <stdio.h>

#include <GL/glew.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#endif

// Finally, on mac, use CGL...

GLContext2::GLContext2()
{
	mGLContext = NULL;
    
    Init();
}


GLContext2::~GLContext2()
{
	// On Windows, this is important to clean, otherwise when we reload a plugin 
	// with the same host session, this will crash
	// (due to RegisterWindowsClass and so on)
#ifdef WIN32
    // Crashed on Mac
    glfwDestroyWindow((GLFWwindow *)mGLContext);
    
    // Dummy comment, but otherwise, makes crash when
    // quitting plugin or application on Mac
    glfwTerminate();
#endif
    
#ifdef __APPLE__
    CGLDestroyContext((CGLContextObj)mGLContext);
    
    //glfwTerminate();
#endif
}

bool
GLContext2::Bind()
{
#ifdef WIN32
    glfwMakeContextCurrent((GLFWwindow *)mGLContext);
#endif
    
#ifdef __APPLE__
    if (CGLSetCurrentContext((CGLContextObj)mGLContext))
        return false;
#endif
    
    return true;
}

void
GLContext2::Unbind()
{
#ifdef WIN32
    glfwMakeContextCurrent(NULL);
#endif
    
#ifdef __APPLE__
    CGLSetCurrentContext(NULL);
#endif
}

static void
glfwErrorCb(int error, const char* desc)
{
	fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

bool 
GLContext2::Init()
{
#ifdef WIN32
	// Debug
	glfwSetErrorCallback(glfwErrorCb);
    
    if (!glfwInit())
		return false;
    
    // Create the context
	
    // TODO: check pixel format !!!!!
	
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	
	mGLContext = glfwCreateWindow(640, 480, "BlueLabAudio", NULL, NULL);
    
	if (mGLContext == NULL)
	{
		glfwTerminate();
		
		return false;
	}
    
	if (!Bind())
		return false;
    
    glewExperimental = GL_TRUE;
    
	if(glewInit() != GLEW_OK)
		return false;
    
	// Check OpenGL functionality
	if (!glfwExtensionSupported("GL_EXT_framebuffer_object"))
	{
		return false;
	}
	
	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();
#endif
    
#ifdef __APPLE__
    ////
    // Debug
	glfwSetErrorCallback(glfwErrorCb);
    
    if (!glfwInit())
		return false;
    
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create the context
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
    
    glewExperimental = GL_TRUE;
    
	if(glewInit() != GLEW_OK)
		return false;
    
    // Check OpenGL functionality:
    glstring = glGetString(GL_EXTENSIONS);
    
    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring))
    {
        return false;
    }
	
	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();
#endif
    
	return true;
}
