//
//  GLContext3.h
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#ifndef Transient_GLContext3_h
#define Transient_GLContext3_h

#include "IPlug_include_in_plug_hdr.h"

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
//
// GLContext3: use pugl and no static
//

class GLContext3
{
public:
    GLContext3(const char *plugName, IGraphics *pGraphics);
    
	virtual ~GLContext3();

    bool IsInitialized();
    
    bool Enter();
    
    void Leave();

	void DebugPrintGLError();

protected:
    void *mContext;
};

#endif
