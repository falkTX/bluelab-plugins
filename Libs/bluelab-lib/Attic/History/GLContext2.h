//
//  GLContext.h
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#ifndef Transient_GLContext2_h
#define Transient_GLContext2_h

// Use the same implementation for Mac and Windows
// (Uses glfw for context).
class GLContext2
{
public:
    static void Catch();
    
    static void Release();
    
    static GLContext2 *Get();
    
	virtual ~GLContext2();
    
    bool Bind();
    
    void Unbind();
    
    static void DebugPrintGLError();
    
    static void DebugDumpGLError();
    
protected:
    GLContext2();
    
	bool InitGL();

    void *mGLContext;
    
private:
    static GLContext2 *mInstance;
    static int mRefCount;
};

#endif
