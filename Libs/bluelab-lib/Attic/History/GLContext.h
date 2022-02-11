//
//  GLContext.h
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#ifndef Transient_GLContext_h
#define Transient_GLContext_h

// This is the first version of GLContext
//
// With version Mac only, this one is better than GLContext + glfw
// GraphControl6 bench: ~120
// (GLContext2 + glfw was ~180)
//
// History: after port to Windows, GLContext2 + glfw was used
// It was optimised to do "direct rendering" i.e not copying
// too much memory between CPU and GPU
// After going back to Mac only, GLContext is better
// (avoids crashes, hack with ghost window,
// problems of "static" with several plugs at the same time, faster

// For Mixcraft on Windows for example, the singleton
// doesn't work either between plugins
//
// NOTE: not sure it is really efficient...
//
#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

class EXPORT GLContext
{
public:
    static GLContext *Get();
    
	virtual ~GLContext();

	// Reference counter
	static void Ref();
	static void Unref();
    
    bool Bind();
    
    void Unbind();
    
	void SwapBuffers();

	static void DebugPrintGLError();

protected:
    GLContext();
 
	bool Init();

    void *mGLContext;
    
	static int mRefCount;

private:
    static GLContext *mInstance;
};

#endif
