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
    GLContext2();
    
	virtual ~GLContext2();
    
    bool Bind();
    
    void Unbind();
    
protected:
	bool Init();

    void *mGLContext;
};

#endif
