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

#include "Debug.h"

GLContext2 *GLContext2::mInstance = NULL;
int GLContext2::mRefCount = 0;

void
GLContext2::Catch()
{
    if (mInstance == NULL)
        mInstance = new GLContext2();
    
    mRefCount++;
}

void
GLContext2::Release()
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

GLContext2 *
GLContext2::Get()
{
    return mInstance;
}

GLContext2::~GLContext2()
{
    // This crashes on Mac with Logic but not with Protools...
    
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
}

bool
GLContext2::Bind()
{
    glfwMakeContextCurrent((GLFWwindow *)mGLContext);
    
    return true;
}

void
GLContext2::Unbind()
{
    glfwMakeContextCurrent(NULL);
}

void
GLContext2::DebugPrintGLError()
{
    GLenum error = glGetError();
    
    if (error != GL_NO_ERROR)
    {
        const GLubyte *errorStr = gluErrorString(error);
    
        fprintf(stderr, "GLError: %s\n", errorStr);
    }
}

void
GLContext2::DebugDumpGLError()
{
    GLenum error = glGetError();
    const GLubyte *errorStr = gluErrorString(error);
    
    char message[1024];
    sprintf(message, "GLError: %s\n", errorStr);
    
    Debug::DumpMessage("GLErrors.txt", message);
}

GLContext2::GLContext2()
{
	mGLContext = NULL;
    
    InitGL();
}

static void
glfwErrorCb(int error, const char* desc)
{
	fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

bool 
GLContext2::InitGL()
{
	// Debug
	glfwSetErrorCallback(glfwErrorCb);
    
    if (!glfwInit())
		return false;
    
    // Create the context
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

	return true;
}
