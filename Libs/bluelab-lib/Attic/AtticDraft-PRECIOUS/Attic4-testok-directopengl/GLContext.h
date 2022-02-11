//
//  GLContext.h
//  Transient
//
//  Created by Apple m'a Tuer on 29/09/17.
//
//

#ifndef Transient_GLContext_h
#define Transient_GLContext_h

class GLContext
{
public:
    static GLContext *Get();
    
	virtual ~GLContext();

	// Reference counter
	static void Ref();
	static void Unref();
    
    bool Bind();
    
    void Unbind();
    
protected:
    GLContext();
 
	bool Init();

    void *mGLContext;
    
	static int mRefCount;

private:
    static GLContext *mInstance;
};

#endif
