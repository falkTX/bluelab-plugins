#ifdef WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

#include "GLWrapper_win.h"

// Declaration of BL function pointers
void(*BL_glAccum) (GLenum op, GLfloat value) = NULL;
void(*BL_glAlphaFunc) (GLenum func, GLclampf ref) = NULL;
GLboolean(*BL_glAreTexturesResident) (GLsizei n, const GLuint *textures, GLboolean *residences) = NULL;
void(*BL_glArrayElement) (GLint i) = NULL;
void(*BL_glBegin) (GLenum mode) = NULL;
void(*BL_glBindTexture) (GLenum target, GLuint texture) = NULL;
void(*BL_glBitmap) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) = NULL;
void(*BL_glBlendColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) = NULL;
void(*BL_glBlendEquation) (GLenum mode) = NULL;
void(*BL_glBlendEquationSeparate) (GLenum modeRGB, GLenum modeAlpha) = NULL;
void(*BL_glBlendFunc) (GLenum sfactor, GLenum dfactor) = NULL;
void(*BL_glCallList) (GLuint list) = NULL;
void(*BL_glCallLists) (GLsizei n, GLenum type, const GLvoid *lists) = NULL;
void(*BL_glClear) (GLbitfield mask) = NULL;
void(*BL_glClearAccum) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
void(*BL_glClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) = NULL;
void(*BL_glClearDepth) (GLclampd depth) = NULL;
void(*BL_glClearIndex) (GLfloat c) = NULL;
void(*BL_glClearStencil) (GLint s) = NULL;
void(*BL_glClipPlane) (GLenum plane, const GLdouble *equation) = NULL;
void(*BL_glColor3b) (GLbyte red, GLbyte green, GLbyte blue) = NULL;
void(*BL_glColor3bv) (const GLbyte *v) = NULL;
void(*BL_glColor3d) (GLdouble red, GLdouble green, GLdouble blue) = NULL;
void(*BL_glColor3dv) (const GLdouble *v) = NULL;
void(*BL_glColor3f) (GLfloat red, GLfloat green, GLfloat blue) = NULL;
void(*BL_glColor3fv) (const GLfloat *v) = NULL;
void(*BL_glColor3i) (GLint red, GLint green, GLint blue) = NULL;
void(*BL_glColor3iv) (const GLint *v) = NULL;
void(*BL_glColor3s) (GLshort red, GLshort green, GLshort blue) = NULL;
void(*BL_glColor3sv) (const GLshort *v) = NULL;
void(*BL_glColor3ub) (GLubyte red, GLubyte green, GLubyte blue) = NULL;
void(*BL_glColor3ubv) (const GLubyte *v) = NULL;
void(*BL_glColor3ui) (GLuint red, GLuint green, GLuint blue) = NULL;
void(*BL_glColor3uiv) (const GLuint *v) = NULL;
void(*BL_glColor3us) (GLushort red, GLushort green, GLushort blue) = NULL;
void(*BL_glColor3usv) (const GLushort *v) = NULL;
void(*BL_glColor4b) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) = NULL;
void(*BL_glColor4bv) (const GLbyte *v) = NULL;
void(*BL_glColor4d) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) = NULL;
void(*BL_glColor4dv) (const GLdouble *v) = NULL;
void(*BL_glColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
void(*BL_glColor4fv) (const GLfloat *v) = NULL;
void(*BL_glColor4i) (GLint red, GLint green, GLint blue, GLint alpha) = NULL;
void(*BL_glColor4iv) (const GLint *v) = NULL;
void(*BL_glColor4s) (GLshort red, GLshort green, GLshort blue, GLshort alpha) = NULL;
void(*BL_glColor4sv) (const GLshort *v) = NULL;
void(*BL_glColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) = NULL;
void(*BL_glColor4ubv) (const GLubyte *v) = NULL;
void(*BL_glColor4ui) (GLuint red, GLuint green, GLuint blue, GLuint alpha) = NULL;
void(*BL_glColor4uiv) (const GLuint *v) = NULL;
void(*BL_glColor4us) (GLushort red, GLushort green, GLushort blue, GLushort alpha) = NULL;
void(*BL_glColor4usv) (const GLushort *v) = NULL;
void(*BL_glColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = NULL;
void(*BL_glColorMaterial) (GLenum face, GLenum mode) = NULL;
void(*BL_glColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
void(*BL_glColorSubTable) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data) = NULL;
void(*BL_glColorTable) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table) = NULL;
void(*BL_glColorTableParameterfv) (GLenum target, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glColorTableParameteriv) (GLenum target, GLenum pname, const GLint *params) = NULL;
void(*BL_glConvolutionFilter1D) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image) = NULL;
void(*BL_glConvolutionFilter2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image) = NULL;
void(*BL_glConvolutionParameterf) (GLenum target, GLenum pname, GLfloat params) = NULL;
void(*BL_glConvolutionParameterfv) (GLenum target, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glConvolutionParameteri) (GLenum target, GLenum pname, GLint params) = NULL;
void(*BL_glConvolutionParameteriv) (GLenum target, GLenum pname, const GLint *params) = NULL;
void(*BL_glCopyColorSubTable) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width) = NULL;
void(*BL_glCopyColorTable) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) = NULL;
void(*BL_glCopyConvolutionFilter1D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) = NULL;
void(*BL_glCopyConvolutionFilter2D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
void(*BL_glCopyPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) = NULL;
void(*BL_glCopyTexImage1D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) = NULL;
void(*BL_glCopyTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) = NULL;
void(*BL_glCopyTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) = NULL;
void(*BL_glCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
void(*BL_glCopyTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
void(*BL_glCullFace) (GLenum mode) = NULL;
void(*BL_glDeleteLists) (GLuint list, GLsizei range) = NULL;
void(*BL_glDeleteTextures) (GLsizei n, const GLuint *textures) = NULL;
void(*BL_glDepthFunc) (GLenum func) = NULL;
void(*BL_glDepthMask) (GLboolean flag) = NULL;
void(*BL_glDepthRange) (GLclampd zNear, GLclampd zFar) = NULL;
void(*BL_glDisable) (GLenum cap) = NULL;
void(*BL_glDisableClientState) (GLenum array) = NULL;
void(*BL_glDrawArrays) (GLenum mode, GLint first, GLsizei count) = NULL;
void(*BL_glDrawBuffer) (GLenum mode) = NULL;
void(*BL_glDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) = NULL;
void(*BL_glDrawPixels) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glDrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) = NULL;
void(*BL_glEdgeFlag) (GLboolean flag) = NULL;
void(*BL_glEdgeFlagPointer) (GLsizei stride, const GLvoid *pointer) = NULL;
void(*BL_glEdgeFlagv) (const GLboolean *flag) = NULL;
void(*BL_glEnable) (GLenum cap) = NULL;
void(*BL_glEnableClientState) (GLenum array) = NULL;
void(*BL_glEnd) (void) = NULL;
void(*BL_glEndList) (void) = NULL;
void(*BL_glEvalCoord1d) (GLdouble u) = NULL;
void(*BL_glEvalCoord1dv) (const GLdouble *u) = NULL;
void(*BL_glEvalCoord1f) (GLfloat u) = NULL;
void(*BL_glEvalCoord1fv) (const GLfloat *u) = NULL;
void(*BL_glEvalCoord2d) (GLdouble u, GLdouble v) = NULL;
void(*BL_glEvalCoord2dv) (const GLdouble *u) = NULL;
void(*BL_glEvalCoord2f) (GLfloat u, GLfloat v) = NULL;
void(*BL_glEvalCoord2fv) (const GLfloat *u) = NULL;
void(*BL_glEvalMesh1) (GLenum mode, GLint i1, GLint i2) = NULL;
void(*BL_glEvalMesh2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) = NULL;
void(*BL_glEvalPoint1) (GLint i) = NULL;
void(*BL_glEvalPoint2) (GLint i, GLint j) = NULL;
void(*BL_glFeedbackBuffer) (GLsizei size, GLenum type, GLfloat *buffer) = NULL;
void(*BL_glFinish) (void) = NULL;
void(*BL_glFlush) (void) = NULL;
void(*BL_glFogf) (GLenum pname, GLfloat param) = NULL;
void(*BL_glFogfv) (GLenum pname, const GLfloat *params) = NULL;
void(*BL_glFogi) (GLenum pname, GLint param) = NULL;
void(*BL_glFogiv) (GLenum pname, const GLint *params) = NULL;
void(*BL_glFrontFace) (GLenum mode) = NULL;
void(*BL_glFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) = NULL;
GLuint (*BL_glGenLists) (GLsizei range) = NULL;
void(*BL_glGenTextures) (GLsizei n, GLuint *textures) = NULL;
void(*BL_glGetBooleanv) (GLenum pname, GLboolean *params) = NULL;
void(*BL_glGetClipPlane) (GLenum plane, GLdouble *equation) = NULL;
void(*BL_glGetColorTable) (GLenum target, GLenum format, GLenum type, GLvoid *table) = NULL;
void(*BL_glGetColorTableParameterfv) (GLenum target, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetColorTableParameteriv) (GLenum target, GLenum pname, GLint *params) = NULL;
void(*BL_glGetConvolutionFilter) (GLenum target, GLenum format, GLenum type, GLvoid *image) = NULL;
void(*BL_glGetConvolutionParameterfv) (GLenum target, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetConvolutionParameteriv) (GLenum target, GLenum pname, GLint *params) = NULL;
void(*BL_glGetDoublev) (GLenum pname, GLdouble *params) = NULL;
GLenum (*BL_glGetError) (void) = NULL;
void(*BL_glGetFloatv) (GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetHistogram) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values) = NULL;
void(*BL_glGetHistogramParameterfv) (GLenum target, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetHistogramParameteriv) (GLenum target, GLenum pname, GLint *params) = NULL;
void(*BL_glGetIntegerv) (GLenum pname, GLint *params) = NULL;
void(*BL_glGetLightfv) (GLenum light, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetLightiv) (GLenum light, GLenum pname, GLint *params) = NULL;
void(*BL_glGetMapdv) (GLenum target, GLenum query, GLdouble *v) = NULL;
void(*BL_glGetMapfv) (GLenum target, GLenum query, GLfloat *v) = NULL;
void(*BL_glGetMapiv) (GLenum target, GLenum query, GLint *v) = NULL;
void(*BL_glGetMaterialfv) (GLenum face, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetMaterialiv) (GLenum face, GLenum pname, GLint *params) = NULL;
void(*BL_glGetMinmax) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values) = NULL;
void(*BL_glGetMinmaxParameterfv) (GLenum target, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetMinmaxParameteriv) (GLenum target, GLenum pname, GLint *params) = NULL;
void(*BL_glGetPixelMapfv) (GLenum map, GLfloat *values) = NULL;
void(*BL_glGetPixelMapuiv) (GLenum map, GLuint *values) = NULL;
void(*BL_glGetPixelMapusv) (GLenum map, GLushort *values) = NULL;
void(*BL_glGetPointerv) (GLenum pname, GLvoid **params) = NULL;
void(*BL_glGetPolygonStipple) (GLubyte *mask) = NULL;
void(*BL_glGetSeparableFilter) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span) = NULL;
GLubyte *const (*BL_glGetString) (GLenum name) = NULL;
void(*BL_glGetTexEnvfv) (GLenum target, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetTexEnviv) (GLenum target, GLenum pname, GLint *params) = NULL;
void(*BL_glGetTexGendv) (GLenum coord, GLenum pname, GLdouble *params) = NULL;
void(*BL_glGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetTexGeniv) (GLenum coord, GLenum pname, GLint *params) = NULL;
void(*BL_glGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) = NULL;
void(*BL_glGetTexLevelParameterfv) (GLenum target, GLint level, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params) = NULL;
void(*BL_glGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params) = NULL;
void(*BL_glGetTexParameteriv) (GLenum target, GLenum pname, GLint *params) = NULL;
void(*BL_glHint) (GLenum target, GLenum mode) = NULL;
void(*BL_glHistogram) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink) = NULL;
void(*BL_glIndexMask) (GLuint mask) = NULL;
void(*BL_glIndexPointer) (GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
void(*BL_glIndexd) (GLdouble c) = NULL;
void(*BL_glIndexdv) (const GLdouble *c) = NULL;
void(*BL_glIndexf) (GLfloat c) = NULL;
void(*BL_glIndexfv) (const GLfloat *c) = NULL;
void(*BL_glIndexi) (GLint c) = NULL;
void(*BL_glIndexiv) (const GLint *c) = NULL;
void(*BL_glIndexs) (GLshort c) = NULL;
void(*BL_glIndexsv) (const GLshort *c) = NULL;
void(*BL_glIndexub) (GLubyte c) = NULL;
void(*BL_glIndexubv) (const GLubyte *c) = NULL;
void(*BL_glInitNames) (void) = NULL;
void(*BL_glInterleavedArrays) (GLenum format, GLsizei stride, const GLvoid *pointer) = NULL;
GLboolean(*BL_glIsEnabled) (GLenum cap) = NULL;
GLboolean(*BL_glIsList) (GLuint list) = NULL;
GLboolean(*BL_glIsTexture) (GLuint texture) = NULL;
void(*BL_glLightModelf) (GLenum pname, GLfloat param) = NULL;
void(*BL_glLightModelfv) (GLenum pname, const GLfloat *params) = NULL;
void(*BL_glLightModeli) (GLenum pname, GLint param) = NULL;
void(*BL_glLightModeliv) (GLenum pname, const GLint *params) = NULL;
void(*BL_glLightf) (GLenum light, GLenum pname, GLfloat param) = NULL;
void(*BL_glLightfv) (GLenum light, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glLighti) (GLenum light, GLenum pname, GLint param) = NULL;
void(*BL_glLightiv) (GLenum light, GLenum pname, const GLint *params) = NULL;
void(*BL_glLineStipple) (GLint factor, GLushort pattern) = NULL;
void(*BL_glLineWidth) (GLfloat width) = NULL;
void(*BL_glListBase) (GLuint base) = NULL;
void(*BL_glLoadIdentity) (void) = NULL;
void(*BL_glLoadMatrixd) (const GLdouble *m) = NULL;
void(*BL_glLoadMatrixf) (const GLfloat *m) = NULL;
void(*BL_glLoadName) (GLuint name) = NULL;
void(*BL_glLogicOp) (GLenum opcode) = NULL;
void(*BL_glMap1d) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) = NULL;
void(*BL_glMap1f) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) = NULL;
void(*BL_glMap2d) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) = NULL;
void(*BL_glMap2f) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) = NULL;
void(*BL_glMapGrid1d) (GLint un, GLdouble u1, GLdouble u2) = NULL;
void(*BL_glMapGrid1f) (GLint un, GLfloat u1, GLfloat u2) = NULL;
void(*BL_glMapGrid2d) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) = NULL;
void(*BL_glMapGrid2f) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) = NULL;
void(*BL_glMaterialf) (GLenum face, GLenum pname, GLfloat param) = NULL;
void(*BL_glMaterialfv) (GLenum face, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glMateriali) (GLenum face, GLenum pname, GLint param) = NULL;
void(*BL_glMaterialiv) (GLenum face, GLenum pname, const GLint *params) = NULL;
void(*BL_glMatrixMode) (GLenum mode) = NULL;
void(*BL_glMinmax) (GLenum target, GLenum internalformat, GLboolean sink) = NULL;
void(*BL_glMultMatrixd) (const GLdouble *m) = NULL;
void(*BL_glMultMatrixf) (const GLfloat *m) = NULL;
void(*BL_glNewList) (GLuint list, GLenum mode) = NULL;
void(*BL_glNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz) = NULL;
void(*BL_glNormal3bv) (const GLbyte *v) = NULL;
void(*BL_glNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz) = NULL;
void(*BL_glNormal3dv) (const GLdouble *v) = NULL;
void(*BL_glNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz) = NULL;
void(*BL_glNormal3fv) (const GLfloat *v) = NULL;
void(*BL_glNormal3i) (GLint nx, GLint ny, GLint nz) = NULL;
void(*BL_glNormal3iv) (const GLint *v) = NULL;
void(*BL_glNormal3s) (GLshort nx, GLshort ny, GLshort nz) = NULL;
void(*BL_glNormal3sv) (const GLshort *v) = NULL;
void(*BL_glNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
void(*BL_glOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) = NULL;
void(*BL_glPassThrough) (GLfloat token) = NULL;
void(*BL_glPixelMapfv) (GLenum map, GLint mapsize, const GLfloat *values) = NULL;
void(*BL_glPixelMapuiv) (GLenum map, GLint mapsize, const GLuint *values) = NULL;
void(*BL_glPixelMapusv) (GLenum map, GLint mapsize, const GLushort *values) = NULL;
void(*BL_glPixelStoref) (GLenum pname, GLfloat param) = NULL;
void(*BL_glPixelStorei) (GLenum pname, GLint param) = NULL;
void(*BL_glPixelTransferf) (GLenum pname, GLfloat param) = NULL;
void(*BL_glPixelTransferi) (GLenum pname, GLint param) = NULL;
void(*BL_glPixelZoom) (GLfloat xfactor, GLfloat yfactor) = NULL;
void(*BL_glPointSize) (GLfloat size) = NULL;
void(*BL_glPolygonMode) (GLenum face, GLenum mode) = NULL;
void(*BL_glPolygonOffset) (GLfloat factor, GLfloat units) = NULL;
void(*BL_glPolygonStipple) (const GLubyte *mask) = NULL;
void(*BL_glPopAttrib) (void) = NULL;
void(*BL_glPopClientAttrib) (void) = NULL;
void(*BL_glPopMatrix) (void) = NULL;
void(*BL_glPopName) (void) = NULL;
void(*BL_glPrioritizeTextures) (GLsizei n, const GLuint *textures, const GLclampf *priorities) = NULL;
void(*BL_glPushAttrib) (GLbitfield mask) = NULL;
void(*BL_glPushClientAttrib) (GLbitfield mask) = NULL;
void(*BL_glPushMatrix) (void) = NULL;
void(*BL_glPushName) (GLuint name) = NULL;
void(*BL_glRasterPos2d) (GLdouble x, GLdouble y) = NULL;
void(*BL_glRasterPos2dv) (const GLdouble *v) = NULL;
void(*BL_glRasterPos2f) (GLfloat x, GLfloat y) = NULL;
void(*BL_glRasterPos2fv) (const GLfloat *v) = NULL;
void(*BL_glRasterPos2i) (GLint x, GLint y) = NULL;
void(*BL_glRasterPos2iv) (const GLint *v) = NULL;
void(*BL_glRasterPos2s) (GLshort x, GLshort y) = NULL;
void(*BL_glRasterPos2sv) (const GLshort *v) = NULL;
void(*BL_glRasterPos3d) (GLdouble x, GLdouble y, GLdouble z) = NULL;
void(*BL_glRasterPos3dv) (const GLdouble *v) = NULL;
void(*BL_glRasterPos3f) (GLfloat x, GLfloat y, GLfloat z) = NULL;
void(*BL_glRasterPos3fv) (const GLfloat *v) = NULL;
void(*BL_glRasterPos3i) (GLint x, GLint y, GLint z) = NULL;
void(*BL_glRasterPos3iv) (const GLint *v) = NULL;
void(*BL_glRasterPos3s) (GLshort x, GLshort y, GLshort z) = NULL;
void(*BL_glRasterPos3sv) (const GLshort *v) = NULL;
void(*BL_glRasterPos4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w) = NULL;
void(*BL_glRasterPos4dv) (const GLdouble *v) = NULL;
void(*BL_glRasterPos4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
void(*BL_glRasterPos4fv) (const GLfloat *v) = NULL;
void(*BL_glRasterPos4i) (GLint x, GLint y, GLint z, GLint w) = NULL;
void(*BL_glRasterPos4iv) (const GLint *v) = NULL;
void(*BL_glRasterPos4s) (GLshort x, GLshort y, GLshort z, GLshort w) = NULL;
void(*BL_glRasterPos4sv) (const GLshort *v) = NULL;
void(*BL_glReadBuffer) (GLenum mode) = NULL;
void(*BL_glReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) = NULL;
void(*BL_glRectd) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) = NULL;
void(*BL_glRectdv) (const GLdouble *v1, const GLdouble *v2) = NULL;
void(*BL_glRectf) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) = NULL;
void(*BL_glRectfv) (const GLfloat *v1, const GLfloat *v2) = NULL;
void(*BL_glRecti) (GLint x1, GLint y1, GLint x2, GLint y2) = NULL;
void(*BL_glRectiv) (const GLint *v1, const GLint *v2) = NULL;
void(*BL_glRects) (GLshort x1, GLshort y1, GLshort x2, GLshort y2) = NULL;
void(*BL_glRectsv) (const GLshort *v1, const GLshort *v2) = NULL;
GLint (*BL_glRenderMode) (GLenum mode) = NULL;
void(*BL_glResetHistogram) (GLenum target) = NULL;
void(*BL_glResetMinmax) (GLenum target) = NULL;
void(*BL_glRotated) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z) = NULL;
void(*BL_glRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z) = NULL;
void(*BL_glScaled) (GLdouble x, GLdouble y, GLdouble z) = NULL;
void(*BL_glScalef) (GLfloat x, GLfloat y, GLfloat z) = NULL;
void(*BL_glScissor) (GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
void(*BL_glSelectBuffer) (GLsizei size, GLuint *buffer) = NULL;
void(*BL_glSeparableFilter2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column) = NULL;
void(*BL_glShadeModel) (GLenum mode) = NULL;
void(*BL_glStencilFunc) (GLenum func, GLint ref, GLuint mask) = NULL;
void(*BL_glStencilMask) (GLuint mask) = NULL;
void(*BL_glStencilOp) (GLenum fail, GLenum zfail, GLenum zpass) = NULL;
void(*BL_glTexCoord1d) (GLdouble s) = NULL;
void(*BL_glTexCoord1dv) (const GLdouble *v) = NULL;
void(*BL_glTexCoord1f) (GLfloat s) = NULL;
void(*BL_glTexCoord1fv) (const GLfloat *v) = NULL;
void(*BL_glTexCoord1i) (GLint s) = NULL;
void(*BL_glTexCoord1iv) (const GLint *v) = NULL;
void(*BL_glTexCoord1s) (GLshort s) = NULL;
void(*BL_glTexCoord1sv) (const GLshort *v) = NULL;
void(*BL_glTexCoord2d) (GLdouble s, GLdouble t) = NULL;
void(*BL_glTexCoord2dv) (const GLdouble *v) = NULL;
void(*BL_glTexCoord2f) (GLfloat s, GLfloat t) = NULL;
void(*BL_glTexCoord2fv) (const GLfloat *v) = NULL;
void(*BL_glTexCoord2i) (GLint s, GLint t) = NULL;
void(*BL_glTexCoord2iv) (const GLint *v) = NULL;
void(*BL_glTexCoord2s) (GLshort s, GLshort t) = NULL;
void(*BL_glTexCoord2sv) (const GLshort *v) = NULL;
void(*BL_glTexCoord3d) (GLdouble s, GLdouble t, GLdouble r) = NULL;
void(*BL_glTexCoord3dv) (const GLdouble *v) = NULL;
void(*BL_glTexCoord3f) (GLfloat s, GLfloat t, GLfloat r) = NULL;
void(*BL_glTexCoord3fv) (const GLfloat *v) = NULL;
void(*BL_glTexCoord3i) (GLint s, GLint t, GLint r) = NULL;
void(*BL_glTexCoord3iv) (const GLint *v) = NULL;
void(*BL_glTexCoord3s) (GLshort s, GLshort t, GLshort r) = NULL;
void(*BL_glTexCoord3sv) (const GLshort *v) = NULL;
void(*BL_glTexCoord4d) (GLdouble s, GLdouble t, GLdouble r, GLdouble q) = NULL;
void(*BL_glTexCoord4dv) (const GLdouble *v) = NULL;
void(*BL_glTexCoord4f) (GLfloat s, GLfloat t, GLfloat r, GLfloat q) = NULL;
void(*BL_glTexCoord4fv) (const GLfloat *v) = NULL;
void(*BL_glTexCoord4i) (GLint s, GLint t, GLint r, GLint q) = NULL;
void(*BL_glTexCoord4iv) (const GLint *v) = NULL;
void(*BL_glTexCoord4s) (GLshort s, GLshort t, GLshort r, GLshort q) = NULL;
void(*BL_glTexCoord4sv) (const GLshort *v) = NULL;
void(*BL_glTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
void(*BL_glTexEnvf) (GLenum target, GLenum pname, GLfloat param) = NULL;
void(*BL_glTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glTexEnvi) (GLenum target, GLenum pname, GLint param) = NULL;
void(*BL_glTexEnviv) (GLenum target, GLenum pname, const GLint *params) = NULL;
void(*BL_glTexGend) (GLenum coord, GLenum pname, GLdouble param) = NULL;
void(*BL_glTexGendv) (GLenum coord, GLenum pname, const GLdouble *params) = NULL;
void(*BL_glTexGenf) (GLenum coord, GLenum pname, GLfloat param) = NULL;
void(*BL_glTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glTexGeni) (GLenum coord, GLenum pname, GLint param) = NULL;
void(*BL_glTexGeniv) (GLenum coord, GLenum pname, const GLint *params) = NULL;
void(*BL_glTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glTexImage3D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glTexParameterf) (GLenum target, GLenum pname, GLfloat param) = NULL;
void(*BL_glTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params) = NULL;
void(*BL_glTexParameteri) (GLenum target, GLenum pname, GLint param) = NULL;
void(*BL_glTexParameteriv) (GLenum target, GLenum pname, const GLint *params) = NULL;
void(*BL_glTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
void(*BL_glTranslated) (GLdouble x, GLdouble y, GLdouble z) = NULL;
void(*BL_glTranslatef) (GLfloat x, GLfloat y, GLfloat z) = NULL;
void(*BL_glVertex2d) (GLdouble x, GLdouble y) = NULL;
void(*BL_glVertex2dv) (const GLdouble *v) = NULL;
void(*BL_glVertex2f) (GLfloat x, GLfloat y) = NULL;
void(*BL_glVertex2fv) (const GLfloat *v) = NULL;
void(*BL_glVertex2i) (GLint x, GLint y) = NULL;
void(*BL_glVertex2iv) (const GLint *v) = NULL;
void(*BL_glVertex2s) (GLshort x, GLshort y) = NULL;
void(*BL_glVertex2sv) (const GLshort *v) = NULL;
void(*BL_glVertex3d) (GLdouble x, GLdouble y, GLdouble z) = NULL;
void(*BL_glVertex3dv) (const GLdouble *v) = NULL;
void(*BL_glVertex3f) (GLfloat x, GLfloat y, GLfloat z) = NULL;
void(*BL_glVertex3fv) (const GLfloat *v) = NULL;
void(*BL_glVertex3i) (GLint x, GLint y, GLint z) = NULL;
void(*BL_glVertex3iv) (const GLint *v) = NULL;
void(*BL_glVertex3s) (GLshort x, GLshort y, GLshort z) = NULL;
void(*BL_glVertex3sv) (const GLshort *v) = NULL;
void(*BL_glVertex4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w) = NULL;
void(*BL_glVertex4dv) (const GLdouble *v) = NULL;
void(*BL_glVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
void(*BL_glVertex4fv) (const GLfloat *v) = NULL;
void(*BL_glVertex4i) (GLint x, GLint y, GLint z, GLint w) = NULL;
void(*BL_glVertex4iv) (const GLint *v) = NULL;
void(*BL_glVertex4s) (GLshort x, GLshort y, GLshort z, GLshort w) = NULL;
void(*BL_glVertex4sv) (const GLshort *v) = NULL;
void(*BL_glVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
void(*BL_glViewport) (GLint x, GLint y, GLsizei width, GLsizei height) = NULL;

// Declaration of BL function pointers - WGl
BOOL (*BL_wglCopyContext)(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask);
HGLRC(*BL_wglCreateContext)(HDC hDc);
HGLRC(*BL_wglCreateLayerContext)(HDC hDc, int level);
BOOL(*BL_wglDeleteContext)(HGLRC oldContext);
BOOL(*BL_wglDescribeLayerPlane)(HDC hDc, int pixelFormat, int layerPlane, UINT nBytes, const LAYERPLANEDESCRIPTOR *plpd);
HGLRC(*BL_wglGetCurrentContext)(void);
HDC(*BL_wglGetCurrentDC)(void);
int(*BL_wglGetLayerPaletteEntries)(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr);
PROC(*BL_wglGetProcAddress)(LPCSTR lpszProc);
BOOL(*BL_wglMakeCurrent)(HDC hDc, HGLRC newContext);
BOOL(*BL_wglRealizeLayerPalette)(HDC hdc, int iLayerPlane, BOOL bRealize);
int(*BL_wglSetLayerPaletteEntries)(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr);
BOOL(*BL_wglShareLists)(HGLRC hrcSrvShare, HGLRC hrcSrvSource);
BOOL(*BL_wglSwapLayerBuffers)(HDC hdc, UINT fuFlags);
BOOL(*BL_wglUseFontBitmaps)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
BOOL(*BL_wglUseFontBitmapsA)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
BOOL(*BL_wglUseFontBitmapsW)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
BOOL(*BL_wglUseFontOutlines)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf);
BOOL(*BL_wglUseFontOutlinesA)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf);
BOOL(*BL_wglUseFontOutlinesW)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf);

// Functions indirections
void glAccum(GLenum op, GLfloat value)
{
	BL_glAccum(op, value);
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
	BL_glAlphaFunc(func, ref);
}

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
	return BL_glAreTexturesResident(n, textures, residences);
}

void glArrayElement(GLint i)
{
	BL_glArrayElement(i);
}

void glBegin(GLenum mode)
{
	BL_glBegin(mode);
}

void glBindTexture(GLenum target, GLuint texture)
{
	BL_glBindTexture(target, texture);
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	BL_glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	BL_glBlendColor(red, green, blue, alpha);
}

void glBlendEquation(GLenum mode)
{
	BL_glBlendEquation(mode);
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	BL_glBlendEquationSeparate(modeRGB, modeAlpha);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	BL_glBlendFunc(sfactor, dfactor);
}

void glCallList(GLuint list)
{
	BL_glCallList(list);
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
	BL_glCallLists(n, type, lists);
}

void glClear(GLbitfield mask)
{
	BL_glClear(mask);
}

void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	BL_glClearAccum(red, green, blue, alpha);
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	BL_glClearColor(red, green, blue, alpha);
}

void glClearDepth(GLclampd depth)
{
	BL_glClearDepth(depth);
}

void glClearIndex(GLfloat c)
{
	BL_glClearIndex(c);
}

void glClearStencil(GLint s)
{
	BL_glClearStencil(s);
}

void glClipPlane(GLenum plane, const GLdouble *equation)
{
	BL_glClipPlane(plane, equation);
}

void glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
	BL_glColor3b(red, green, blue);
}

void glColor3bv(const GLbyte *v)
{
	BL_glColor3bv(v);
}

void glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
	BL_glColor3d(red, green, blue);
}

void glColor3dv(const GLdouble *v)
{
	BL_glColor3dv(v);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	BL_glColor3f(red, green, blue);
}

void glColor3fv(const GLfloat *v)
{
	BL_glColor3fv(v);
}

void glColor3i(GLint red, GLint green, GLint blue)
{
	BL_glColor3i(red, green, blue);
}

void glColor3iv(const GLint *v)
{
	BL_glColor3iv(v);
}

void glColor3s(GLshort red, GLshort green, GLshort blue)
{
	BL_glColor3s(red, green, blue);
}

void glColor3sv(const GLshort *v)
{
	BL_glColor3sv(v);
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
	BL_glColor3ub(red, green, blue);
}

void glColor3ubv(const GLubyte *v)
{
	BL_glColor3ubv(v);
}

void glColor3ui(GLuint red, GLuint green, GLuint blue)
{
	BL_glColor3ui(red, green, blue);
}

void glColor3uiv(const GLuint *v)
{
	BL_glColor3uiv(v);
}

void glColor3us(GLushort red, GLushort green, GLushort blue)
{
	BL_glColor3us(red, green, blue);
}

void glColor3usv(const GLushort *v)
{
	BL_glColor3usv(v);
}

void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
	BL_glColor4b(red, green, blue, alpha);
}

void glColor4bv(const GLbyte *v)
{
	BL_glColor4bv(v);
}

void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
	BL_glColor4d(red, green, blue, alpha);
}

void glColor4dv(const GLdouble *v)
{
	BL_glColor4dv(v);
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	BL_glColor4f(red, green, blue, alpha);
}

void glColor4fv(const GLfloat *v)
{
	BL_glColor4fv(v);
}

void glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
	BL_glColor4i(red, green, blue, alpha);
}

void glColor4iv(const GLint *v)
{
	BL_glColor4iv(v);
}

void glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
	BL_glColor4s(red, green, blue, alpha);
}

void glColor4sv(const GLshort *v)
{
	BL_glColor4sv(v);
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	BL_glColor4ub(red, green, blue, alpha);
}

void glColor4ubv(const GLubyte *v)
{
	BL_glColor4ubv(v);
}

void glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
	BL_glColor4ui(red, green, blue, alpha);
}

void glColor4uiv(const GLuint *v)
{
	BL_glColor4uiv(v);
}

void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
	BL_glColor4us(red, green, blue, alpha);
}

void glColor4usv(const GLushort *v)
{
	BL_glColor4usv(v);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	BL_glColorMask(red, green, blue, alpha);
}

void glColorMaterial(GLenum face, GLenum mode)
{
	BL_glColorMaterial(face, mode);
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	BL_glColorPointer(size, type, stride, pointer);
}

void glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
	BL_glColorSubTable(target, start, count, format, type, data);
}

void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
	BL_glColorTable(target, internalformat, width, format, type, table);
}

void glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	BL_glColorTableParameterfv(target, pname, params);
}

void glColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	BL_glColorTableParameteriv(target, pname, params);
}

void glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
	BL_glConvolutionFilter1D(target, internalformat, width, format, type, image);
}

void glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
	BL_glConvolutionFilter2D(target, internalformat, width, height, format, type, image);
}

void glConvolutionParameterf(GLenum target, GLenum pname, GLfloat params)
{
	BL_glConvolutionParameterf(target, pname, params);
}

void glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	BL_glConvolutionParameterfv(target, pname, params);
}

void glConvolutionParameteri(GLenum target, GLenum pname, GLint params)
{
	BL_glConvolutionParameteri(target, pname, params);
}

void glConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	BL_glConvolutionParameteriv(target, pname, params);
}

void glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
	BL_glCopyColorSubTable(target, start, x, y, width);
}

void glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	BL_glCopyColorTable(target, internalformat, x, y, width);
}

void glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	BL_glCopyConvolutionFilter1D(target, internalformat, x, y, width);
}

void glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
	BL_glCopyConvolutionFilter2D(target, internalformat, x, y, width, height);
}

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	BL_glCopyPixels(x, y, width, height, type);
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
	BL_glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	BL_glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	BL_glCopyTexSubImage1D(target, level, xoffset, x, y, width);
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	BL_glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	BL_glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

void glCullFace(GLenum mode)
{
	BL_glCullFace(mode);
}

void glDeleteLists(GLuint list, GLsizei range)
{
	BL_glDeleteLists(list, range);
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
	BL_glDeleteTextures(n, textures);
}

void glDepthFunc(GLenum func)
{
	BL_glDepthFunc(func);
}

void glDepthMask(GLboolean flag)
{
	BL_glDepthMask(flag);
}

void glDepthRange(GLclampd zNear, GLclampd zFar)
{
	BL_glDepthRange(zNear, zFar);
}

void glDisable(GLenum cap)
{
	BL_glDisable(cap);
}

void glDisableClientState(GLenum array)
{
	BL_glDisableClientState(array);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	BL_glDrawArrays(mode, first, count);
}

void glDrawBuffer(GLenum mode)
{
	BL_glDrawBuffer(mode);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	BL_glDrawElements(mode, count, type, indices);
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glDrawPixels(width, height, format, type, pixels);
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
	BL_glDrawRangeElements(mode, start, end, count, type, indices);
}

void glEdgeFlag(GLboolean flag)
{
	BL_glEdgeFlag(flag);
}

void glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
	BL_glEdgeFlagPointer(stride, pointer);
}

void glEdgeFlagv(const GLboolean *flag)
{
	BL_glEdgeFlagv(flag);
}

void glEnable(GLenum cap)
{
	BL_glEnable(cap);
}

void glEnableClientState(GLenum array)
{
	BL_glEnableClientState(array);
}

void glEnd(void)
{
	BL_glEnd();
}

void glEndList(void)
{
	BL_glEndList();
}

void glEvalCoord1d(GLdouble u)
{
	BL_glEvalCoord1d(u);
}

void glEvalCoord1dv(const GLdouble *u)
{
	BL_glEvalCoord1dv(u);
}

void glEvalCoord1f(GLfloat u)
{
	BL_glEvalCoord1f(u);
}

void glEvalCoord1fv(const GLfloat *u)
{
	BL_glEvalCoord1fv(u);
}

void glEvalCoord2d(GLdouble u, GLdouble v)
{
	BL_glEvalCoord2d(u, v);
}

void glEvalCoord2dv(const GLdouble *u)
{
	BL_glEvalCoord2dv(u);
}

void glEvalCoord2f(GLfloat u, GLfloat v)
{
	BL_glEvalCoord2f(u, v);
}

void glEvalCoord2fv(const GLfloat *u)
{
	BL_glEvalCoord2fv(u);
}

void glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
	BL_glEvalMesh1(mode, i1, i2);
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
	BL_glEvalMesh2(mode, i1, i2, j1, j2);
}

void glEvalPoint1(GLint i)
{
	BL_glEvalPoint1(i);
}

void glEvalPoint2(GLint i, GLint j)
{
	BL_glEvalPoint2(i, j);
}

void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
	BL_glFeedbackBuffer(size, type, buffer);
}

void glFinish(void)
{
	BL_glFinish();
}

void glFlush(void)
{
	BL_glFlush();
}

void glFogf(GLenum pname, GLfloat param)
{
	BL_glFogf(pname, param);
}

void glFogfv(GLenum pname, const GLfloat *params)
{
	BL_glFogfv(pname, params);
}

void glFogi(GLenum pname, GLint param)
{
	BL_glFogi(pname, param);
}

void glFogiv(GLenum pname, const GLint *params)
{
	BL_glFogiv(pname, params);
}

void glFrontFace(GLenum mode)
{
	BL_glFrontFace(mode);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	BL_glFrustum(left, right, bottom, top, zNear, zFar);
}

GLuint glGenLists(GLsizei range)
{
	return BL_glGenLists(range);
}

void glGenTextures(GLsizei n, GLuint *textures)
{
	BL_glGenTextures(n, textures);
}

void glGetBooleanv(GLenum pname, GLboolean *params)
{
	BL_glGetBooleanv(pname, params);
}

void glGetClipPlane(GLenum plane, GLdouble *equation)
{
	BL_glGetClipPlane(plane, equation);
}

void glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
	BL_glGetColorTable(target, format, type, table);
}

void glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	BL_glGetColorTableParameterfv(target, pname, params);
}

void glGetColorTableParameteriv(GLenum target, GLenum pname, GLint *params)
{
	BL_glGetColorTableParameteriv(target, pname, params);
}

void glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
	BL_glGetConvolutionFilter(target, format, type, image);
}

void glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	BL_glGetConvolutionParameterfv(target, pname, params);
}

void glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
	BL_glGetConvolutionParameteriv(target, pname, params);
}

void glGetDoublev(GLenum pname, GLdouble *params)
{
	BL_glGetDoublev(pname, params);
}

GLenum glGetError(void)
{
	return BL_glGetError();
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
	BL_glGetFloatv(pname, params);
}

void glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
	BL_glGetHistogram(target, reset, format, type, values);
}

void glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	BL_glGetHistogramParameterfv(target, pname, params);
}

void glGetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
	BL_glGetHistogramParameteriv(target, pname, params);
}

void glGetIntegerv(GLenum pname, GLint *params)
{
	BL_glGetIntegerv(pname, params);
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	BL_glGetLightfv(light, pname, params);
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
	BL_glGetLightiv(light, pname, params);
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
	BL_glGetMapdv(target, query, v);
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
	BL_glGetMapfv(target, query, v);
}

void glGetMapiv(GLenum target, GLenum query, GLint *v)
{
	BL_glGetMapiv(target, query, v);
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	BL_glGetMaterialfv(face, pname, params);
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
	BL_glGetMaterialiv(face, pname, params);
}

void glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
	BL_glGetMinmax(target, reset, format, type, values);
}

void glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	BL_glGetMinmaxParameterfv(target, pname, params);
}

void glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
	BL_glGetMinmaxParameteriv(target, pname, params);
}

void glGetPixelMapfv(GLenum map, GLfloat *values)
{
	BL_glGetPixelMapfv(map, values);
}

void glGetPixelMapuiv(GLenum map, GLuint *values)
{
	BL_glGetPixelMapuiv(map, values);
}

void glGetPixelMapusv(GLenum map, GLushort *values)
{
	BL_glGetPixelMapusv(map, values);
}

void glGetPointerv(GLenum pname, GLvoid **params)
{
	BL_glGetPointerv(pname, params);
}

void glGetPolygonStipple(GLubyte *mask)
{
	BL_glGetPolygonStipple(mask);
}

void glGetSeparableFilter(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
	BL_glGetSeparableFilter(target, format, type, row, column, span);
}

const GLubyte *glGetString(GLenum name)
{
	return BL_glGetString(name);
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
	BL_glGetTexEnvfv(target, pname, params);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
	BL_glGetTexEnviv(target, pname, params);
}

void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
	BL_glGetTexGendv(coord, pname, params);
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
	BL_glGetTexGenfv(coord, pname, params);
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
	BL_glGetTexGeniv(coord, pname, params);
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
	BL_glGetTexImage(target, level, format, type, pixels);
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
	BL_glGetTexLevelParameterfv(target, level, pname, params);
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
	BL_glGetTexLevelParameteriv(target, level, pname, params);
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	BL_glGetTexParameterfv(target, pname, params);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
	BL_glGetTexParameteriv(target, pname, params);
}

void glHint(GLenum target, GLenum mode)
{
	BL_glHint(target, mode);
}

void glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
	BL_glHistogram(target, width, internalformat, sink);
}

void glIndexMask(GLuint mask)
{
	BL_glIndexMask(mask);
}

void glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	BL_glIndexPointer(type, stride, pointer);
}

void glIndexd(GLdouble c)
{
	BL_glIndexd(c);
}

void glIndexdv(const GLdouble *c)
{
	BL_glIndexdv(c);
}

void glIndexf(GLfloat c)
{
	BL_glIndexf(c);
}

void glIndexfv(const GLfloat *c)
{
	BL_glIndexfv(c);
}

void glIndexi(GLint c)
{
	BL_glIndexi(c);
}

void glIndexiv(const GLint *c)
{
	BL_glIndexiv(c);
}

void glIndexs(GLshort c)
{
	BL_glIndexs(c);
}

void glIndexsv(const GLshort *c)
{
	BL_glIndexsv(c);
}

void glIndexub(GLubyte c)
{
	BL_glIndexub(c);
}

void glIndexubv(const GLubyte *c)
{
	BL_glIndexubv(c);
}

void glInitNames(void)
{
	BL_glInitNames();
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
	BL_glInterleavedArrays(format, stride, pointer);
}

GLboolean glIsEnabled(GLenum cap)
{
	return BL_glIsEnabled(cap);
}

GLboolean glIsList(GLuint list)
{
	return BL_glIsList(list);
}

GLboolean glIsTexture(GLuint texture)
{
	return BL_glIsTexture(texture);
}

void glLightModelf(GLenum pname, GLfloat param)
{
	BL_glLightModelf(pname, param);
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
	BL_glLightModelfv(pname, params);
}

void glLightModeli(GLenum pname, GLint param)
{
	BL_glLightModeli(pname, param);
}

void glLightModeliv(GLenum pname, const GLint *params)
{
	BL_glLightModeliv(pname, params);
}

void glLightf(GLenum light, GLenum pname, GLfloat param)
{
	BL_glLightf(light, pname, param);
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	BL_glLightfv(light, pname, params);
}

void glLighti(GLenum light, GLenum pname, GLint param)
{
	BL_glLighti(light, pname, param);
}

void glLightiv(GLenum light, GLenum pname, const GLint *params)
{
	BL_glLightiv(light, pname, params);
}

void glLineStipple(GLint factor, GLushort pattern)
{
	BL_glLineStipple(factor, pattern);
}

void glLineWidth(GLfloat width)
{
	BL_glLineWidth(width);
}

void glListBase(GLuint base)
{
	BL_glListBase(base);
}

void glLoadIdentity(void)
{
	BL_glLoadIdentity();
}

void glLoadMatrixd(const GLdouble *m)
{
	BL_glLoadMatrixd(m);
}

void glLoadMatrixf(const GLfloat *m)
{
	BL_glLoadMatrixf(m);
}

void glLoadName(GLuint name)
{
	BL_glLoadName(name);
}

void glLogicOp(GLenum opcode)
{
	BL_glLogicOp(opcode);
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
	BL_glMap1d(target, u1, u2, stride, order, points);
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
	BL_glMap1f(target, u1, u2, stride, order, points);
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
	BL_glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
	BL_glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
	BL_glMapGrid1d(un, u1, u2);
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
	BL_glMapGrid1f(un, u1, u2);
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
	BL_glMapGrid2d(un, u1, u2, vn, v1, v2);
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
	BL_glMapGrid2f(un, u1, u2, vn, v1, v2);
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	BL_glMaterialf(face, pname, param);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	BL_glMaterialfv(face, pname, params);
}

void glMateriali(GLenum face, GLenum pname, GLint param)
{
	BL_glMateriali(face, pname, param);
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
	BL_glMaterialiv(face, pname, params);
}

void glMatrixMode(GLenum mode)
{
	BL_glMatrixMode(mode);
}

void glMinmax(GLenum target, GLenum internalformat, GLboolean sink)
{
	BL_glMinmax(target, internalformat, sink);
}

void glMultMatrixd(const GLdouble *m)
{
	BL_glMultMatrixd(m);
}

void glMultMatrixf(const GLfloat *m)
{
	BL_glMultMatrixf(m);
}

void glNewList(GLuint list, GLenum mode)
{
	BL_glNewList(list, mode);
}

void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
	BL_glNormal3b(nx, ny, nz);
}

void glNormal3bv(const GLbyte *v)
{
	BL_glNormal3bv(v);
}

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
	BL_glNormal3d(nx, ny, nz);
}

void glNormal3dv(const GLdouble *v)
{
	BL_glNormal3dv(v);
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	BL_glNormal3f(nx, ny, nz);
}

void glNormal3fv(const GLfloat *v)
{
	BL_glNormal3fv(v);
}

void glNormal3i(GLint nx, GLint ny, GLint nz)
{
	BL_glNormal3i(nx, ny, nz);
}

void glNormal3iv(const GLint *v)
{
	BL_glNormal3iv(v);
}

void glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
	BL_glNormal3s(nx, ny, nz);
}

void glNormal3sv(const GLshort *v)
{
	BL_glNormal3sv(v);
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	BL_glNormalPointer(type, stride, pointer);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	BL_glOrtho(left, right, bottom, top, zNear, zFar);
}

void glPassThrough(GLfloat token)
{
	BL_glPassThrough(token);
}

void glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
	BL_glPixelMapfv(map, mapsize, values);
}

void glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
	BL_glPixelMapuiv(map, mapsize, values);
}

void glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
	BL_glPixelMapusv(map, mapsize, values);
}

void glPixelStoref(GLenum pname, GLfloat param)
{
	BL_glPixelStoref(pname, param);
}

void glPixelStorei(GLenum pname, GLint param)
{
	BL_glPixelStorei(pname, param);
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
	BL_glPixelTransferf(pname, param);
}

void glPixelTransferi(GLenum pname, GLint param)
{
	BL_glPixelTransferi(pname, param);
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
	BL_glPixelZoom(xfactor, yfactor);
}

void glPointSize(GLfloat size)
{
	BL_glPointSize(size);
}

void glPolygonMode(GLenum face, GLenum mode)
{
	BL_glPolygonMode(face, mode);
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
	BL_glPolygonOffset(factor, units);
}

void glPolygonStipple(const GLubyte *mask)
{
	BL_glPolygonStipple(mask);
}

void glPopAttrib(void)
{
	BL_glPopAttrib();
}

void glPopClientAttrib(void)
{
	BL_glPopClientAttrib();
}

void glPopMatrix(void)
{
	BL_glPopMatrix();
}

void glPopName(void)
{
	BL_glPopName();
}

void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	BL_glPrioritizeTextures(n, textures, priorities);
}

void glPushAttrib(GLbitfield mask)
{
	BL_glPushAttrib(mask);
}

void glPushClientAttrib(GLbitfield mask)
{
	BL_glPushClientAttrib(mask);
}

void glPushMatrix(void)
{
	BL_glPushMatrix();
}

void glPushName(GLuint name)
{
	BL_glPushName(name);
}

void glRasterPos2d(GLdouble x, GLdouble y)
{
	BL_glRasterPos2d(x, y);
}

void glRasterPos2dv(const GLdouble *v)
{
	BL_glRasterPos2dv(v);
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
	BL_glRasterPos2f(x, y);
}

void glRasterPos2fv(const GLfloat *v)
{
	BL_glRasterPos2fv(v);
}

void glRasterPos2i(GLint x, GLint y)
{
	BL_glRasterPos2i(x, y);
}

void glRasterPos2iv(const GLint *v)
{
	BL_glRasterPos2iv(v);
}

void glRasterPos2s(GLshort x, GLshort y)
{
	BL_glRasterPos2s(x, y);
}

void glRasterPos2sv(const GLshort *v)
{
	BL_glRasterPos2sv(v);
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
	BL_glRasterPos3d(x, y, z);
}

void glRasterPos3dv(const GLdouble *v)
{
	BL_glRasterPos3dv(v);
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	BL_glRasterPos3f(x, y, z);
}

void glRasterPos3fv(const GLfloat *v)
{
	BL_glRasterPos3fv(v);
}

void glRasterPos3i(GLint x, GLint y, GLint z)
{
	BL_glRasterPos3i(x, y, z);
}

void glRasterPos3iv(const GLint *v)
{
	BL_glRasterPos3iv(v);
}

void glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
	BL_glRasterPos3s(x, y, z);
}

void glRasterPos3sv(const GLshort *v)
{
	BL_glRasterPos3sv(v);
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	BL_glRasterPos4d(x, y, z, w);
}

void glRasterPos4dv(const GLdouble *v)
{
	BL_glRasterPos4dv(v);
}

void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	BL_glRasterPos4f(x, y, z, w);
}

void glRasterPos4fv(const GLfloat *v)
{
	BL_glRasterPos4fv(v);
}

void glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
	BL_glRasterPos4i(x, y, z, w);
}

void glRasterPos4iv(const GLint *v)
{
	BL_glRasterPos4iv(v);
}

void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	BL_glRasterPos4s(x, y, z, w);
}

void glRasterPos4sv(const GLshort *v)
{
	BL_glRasterPos4sv(v);
}

void glReadBuffer(GLenum mode)
{
	BL_glReadBuffer(mode);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	BL_glReadPixels(x, y, width, height, format, type, pixels);
}

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	BL_glRectd(x1, y1, x2, y2);
}

void glRectdv(const GLdouble *v1, const GLdouble *v2)
{
	BL_glRectdv(v1, v2);
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	BL_glRectf(x1, y1, x2, y2);
}

void glRectfv(const GLfloat *v1, const GLfloat *v2)
{
	BL_glRectfv(v1, v2);
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
	BL_glRecti(x1, y1, x2, y2);
}

void glRectiv(const GLint *v1, const GLint *v2)
{
	BL_glRectiv(v1, v2);
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
	BL_glRects(x1, y1, x2, y2);
}

void glRectsv(const GLshort *v1, const GLshort *v2)
{
	BL_glRectsv(v1, v2);
}

GLint glRenderMode(GLenum mode)
{
	return BL_glRenderMode(mode);
}

void glResetHistogram(GLenum target)
{
	BL_glResetHistogram(target);
}

void glResetMinmax(GLenum target)
{
	BL_glResetMinmax(target);
}

void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	BL_glRotated(angle, x, y, z);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	BL_glRotatef(angle, x, y, z);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
	BL_glScaled(x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	BL_glScalef(x, y, z);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	BL_glScissor(x, y, width, height);
}

void glSelectBuffer(GLsizei size, GLuint *buffer)
{
	BL_glSelectBuffer(size, buffer);
}

void glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
	BL_glSeparableFilter2D(target, internalformat, width, height, format, type, row, column);
}

void glShadeModel(GLenum mode)
{
	BL_glShadeModel(mode);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	BL_glStencilFunc(func, ref, mask);
}

void glStencilMask(GLuint mask)
{
	BL_glStencilMask(mask);
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	BL_glStencilOp(fail, zfail, zpass);
}

void glTexCoord1d(GLdouble s)
{
	BL_glTexCoord1d(s);
}

void glTexCoord1dv(const GLdouble *v)
{
	BL_glTexCoord1dv(v);
}

void glTexCoord1f(GLfloat s)
{
	BL_glTexCoord1f(s);
}

void glTexCoord1fv(const GLfloat *v)
{
	BL_glTexCoord1fv(v);
}

void glTexCoord1i(GLint s)
{
	BL_glTexCoord1i(s);
}

void glTexCoord1iv(const GLint *v)
{
	BL_glTexCoord1iv(v);
}

void glTexCoord1s(GLshort s)
{
	BL_glTexCoord1s(s);
}

void glTexCoord1sv(const GLshort *v)
{
	BL_glTexCoord1sv(v);
}

void glTexCoord2d(GLdouble s, GLdouble t)
{
	BL_glTexCoord2d(s, t);
}

void glTexCoord2dv(const GLdouble *v)
{
	BL_glTexCoord2dv(v);
}

void glTexCoord2f(GLfloat s, GLfloat t)
{
	BL_glTexCoord2f(s, t);
}

void glTexCoord2fv(const GLfloat *v)
{
	BL_glTexCoord2fv(v);
}

void glTexCoord2i(GLint s, GLint t)
{
	BL_glTexCoord2i(s, t);
}

void glTexCoord2iv(const GLint *v)
{
	BL_glTexCoord2iv(v);
}

void glTexCoord2s(GLshort s, GLshort t)
{
	BL_glTexCoord2s(s, t);
}

void glTexCoord2sv(const GLshort *v)
{
	BL_glTexCoord2sv(v);
}

void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
	BL_glTexCoord3d(s, t, r);
}

void glTexCoord3dv(const GLdouble *v)
{
	BL_glTexCoord3dv(v);
}

void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
	BL_glTexCoord3f(s, t, r);
}

void glTexCoord3fv(const GLfloat *v)
{
	BL_glTexCoord3fv(v);
}

void glTexCoord3i(GLint s, GLint t, GLint r)
{
	BL_glTexCoord3i(s, t, r);
}

void glTexCoord3iv(const GLint *v)
{
	BL_glTexCoord3iv(v);
}

void glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
	BL_glTexCoord3s(s, t, r);
}

void glTexCoord3sv(const GLshort *v)
{
	BL_glTexCoord3sv(v);
}

void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
	BL_glTexCoord4d(s, t, r, q);
}

void glTexCoord4dv(const GLdouble *v)
{
	BL_glTexCoord4dv(v);
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	BL_glTexCoord4f(s, t, r, q);
}

void glTexCoord4fv(const GLfloat *v)
{
	BL_glTexCoord4fv(v);
}

void glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
	BL_glTexCoord4i(s, t, r, q);
}

void glTexCoord4iv(const GLint *v)
{
	BL_glTexCoord4iv(v);
}

void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
	BL_glTexCoord4s(s, t, r, q);
}

void glTexCoord4sv(const GLshort *v)
{
	BL_glTexCoord4sv(v);
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	BL_glTexCoordPointer(size, type, stride, pointer);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	BL_glTexEnvf(target, pname, param);
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	BL_glTexEnvfv(target, pname, params);
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	BL_glTexEnvi(target, pname, param);
}

void glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	BL_glTexEnviv(target, pname, params);
}

void glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
	BL_glTexGend(coord, pname, param);
}

void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
	BL_glTexGendv(coord, pname, params);
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
	BL_glTexGenf(coord, pname, param);
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
	BL_glTexGenfv(coord, pname, params);
}

void glTexGeni(GLenum coord, GLenum pname, GLint param)
{
	BL_glTexGeni(coord, pname, param);
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
	BL_glTexGeniv(coord, pname, params);
}

void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	BL_glTexParameterf(target, pname, param);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	BL_glTexParameterfv(target, pname, params);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	BL_glTexParameteri(target, pname, param);
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	BL_glTexParameteriv(target, pname, params);
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
	BL_glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	BL_glTranslated(x, y, z);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	BL_glTranslatef(x, y, z);
}

void glVertex2d(GLdouble x, GLdouble y)
{
	BL_glVertex2d(x, y);
}

void glVertex2dv(const GLdouble *v)
{
	BL_glVertex2dv(v);
}

void glVertex2f(GLfloat x, GLfloat y)
{
	BL_glVertex2f(x, y);
}

void glVertex2fv(const GLfloat *v)
{
	BL_glVertex2fv(v);
}

void glVertex2i(GLint x, GLint y)
{
	BL_glVertex2i(x, y);
}

void glVertex2iv(const GLint *v)
{
	BL_glVertex2iv(v);
}

void glVertex2s(GLshort x, GLshort y)
{
	BL_glVertex2s(x, y);
}

void glVertex2sv(const GLshort *v)
{
	BL_glVertex2sv(v);
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
	BL_glVertex3d(x, y, z);
}

void glVertex3dv(const GLdouble *v)
{
	BL_glVertex3dv(v);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	BL_glVertex3f(x, y, z);
}

void glVertex3fv(const GLfloat *v)
{
	BL_glVertex3fv(v);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
	BL_glVertex3i(x, y, z);
}

void glVertex3iv(const GLint *v)
{
	BL_glVertex3iv(v);
}

void glVertex3s(GLshort x, GLshort y, GLshort z)
{
	BL_glVertex3s(x, y, z);
}

void glVertex3sv(const GLshort *v)
{
	BL_glVertex3sv(v);
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	BL_glVertex4d(x, y, z, w);
}

void glVertex4dv(const GLdouble *v)
{
	BL_glVertex4dv(v);
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	BL_glVertex4f(x, y, z, w);
}

void glVertex4fv(const GLfloat *v)
{
	BL_glVertex4fv(v);
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
	BL_glVertex4i(x, y, z, w);
}

void glVertex4iv(const GLint *v)
{
	BL_glVertex4iv(v);
}

void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	BL_glVertex4s(x, y, z, w);
}

void glVertex4sv(const GLshort *v)
{
	BL_glVertex4sv(v);
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	BL_glVertexPointer(size, type, stride, pointer);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	BL_glViewport(x, y, width, height);
}

// Functions indirection - WGL
BOOL wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
	return BL_wglCopyContext(hglrcSrc, hglrcDst, mask);
}

HGLRC wglCreateContext(HDC hDc)
{
	return BL_wglCreateContext(hDc);
}

HGLRC wglCreateLayerContext(HDC hDc, int level)
{
	return BL_wglCreateLayerContext(hDc, level);
}

BOOL wglDeleteContext(HGLRC oldContext)
{
	return BL_wglDeleteContext(oldContext);
}

BOOL wglDescribeLayerPlane(HDC hDc, int pixelFormat, int layerPlane, UINT nBytes, const LAYERPLANEDESCRIPTOR *plpd)
{
	return BL_wglDescribeLayerPlane(hDc, pixelFormat, layerPlane, nBytes, plpd);
}

HGLRC wglGetCurrentContext(void)
{
	return BL_wglGetCurrentContext();
}

HDC wglGetCurrentDC(void)
{
	return BL_wglGetCurrentDC();
}

int wglGetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr)
{
	return BL_wglGetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);
}

PROC wglGetProcAddress(LPCSTR lpszProc)
{
	return BL_wglGetProcAddress(lpszProc);
}

BOOL wglMakeCurrent(HDC hDc, HGLRC newContext)
{
	return BL_wglMakeCurrent(hDc, newContext);
}

BOOL wglRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize)
{
	return BL_wglRealizeLayerPalette(hdc, iLayerPlane, bRealize);
}

int wglSetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr)
{
	return BL_wglSetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);
}

BOOL wglShareLists(HGLRC hrcSrvShare, HGLRC hrcSrvSource)
{
	return BL_wglShareLists(hrcSrvShare, hrcSrvSource);
}

BOOL wglSwapLayerBuffers(HDC hdc, UINT fuFlags)
{
	return BL_wglSwapLayerBuffers(hdc, fuFlags);
}

BOOL wglUseFontBitmaps(HDC hDC, DWORD first, DWORD count, DWORD listBase)
{
	return BL_wglUseFontBitmaps(hDC, first, count, listBase);
}

//BOOL wglUseFontBitmapsA(HDC hDC, DWORD first, DWORD count, DWORD listBase)
//{
//	return BL_wglUseFontBitmapsA(hDC, first, count, listBase);
//}

BOOL wglUseFontBitmapsW(HDC hDC, DWORD first, DWORD count, DWORD listBase)
{
	return BL_wglUseFontBitmapsW(hDC, first, count, listBase);
}

BOOL wglUseFontOutlines(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
	return BL_wglUseFontOutlines(hDC, first, count, listBase, deviation, extrusion, format, lpgmf);
}

//BOOL wglUseFontOutlinesA(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
//{
//	return BL_wglUseFontOutlinesA(hDC, first, count, listBase, deviation, extrusion, format, lpgmf);
//}

BOOL wglUseFontOutlinesW(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
	return BL_wglUseFontOutlinesW(hDC, first, count, listBase, deviation, extrusion, format, lpgmf);
}



static void
LoadFunctionPointers(HMODULE module)
{
	BL_glAccum = (void *)GetProcAddress(module, "glAccum");
	BL_glAlphaFunc = (void *)GetProcAddress(module, "glAlphaFunc");
	BL_glAreTexturesResident = (void *)GetProcAddress(module, "glAreTexturesResident");
	BL_glArrayElement = (void *)GetProcAddress(module, "glArrayElement");
	BL_glBegin = (void *)GetProcAddress(module, "glBegin");
	BL_glBindTexture = (void *)GetProcAddress(module, "glBindTexture");
	BL_glBitmap = (void *)GetProcAddress(module, "glBitmap");
	BL_glBlendColor = (void *)GetProcAddress(module, "glBlendColor");
	BL_glBlendEquation = (void *)GetProcAddress(module, "glBlendEquation");
	BL_glBlendEquationSeparate = (void *)GetProcAddress(module, "glBlendEquationSeparate");
	BL_glBlendFunc = (void *)GetProcAddress(module, "glBlendFunc");
	BL_glCallList = (void *)GetProcAddress(module, "glCallList");
	BL_glCallLists = (void *)GetProcAddress(module, "glCallLists");
	BL_glClear = (void *)GetProcAddress(module, "glClear");
	BL_glClearAccum = (void *)GetProcAddress(module, "glClearAccum");
	BL_glClearColor = (void *)GetProcAddress(module, "glClearColor");
	BL_glClearDepth = (void *)GetProcAddress(module, "glClearDepth");
	BL_glClearIndex = (void *)GetProcAddress(module, "glClearIndex");
	BL_glClearStencil = (void *)GetProcAddress(module, "glClearStencil");
	BL_glClipPlane = (void *)GetProcAddress(module, "glClipPlane");
	BL_glColor3b = (void *)GetProcAddress(module, "glColor3b");
	BL_glColor3bv = (void *)GetProcAddress(module, "glColor3bv");
	BL_glColor3d = (void *)GetProcAddress(module, "glColor3d");
	BL_glColor3dv = (void *)GetProcAddress(module, "glColor3dv");
	BL_glColor3f = (void *)GetProcAddress(module, "glColor3f");
	BL_glColor3fv = (void *)GetProcAddress(module, "glColor3fv");
	BL_glColor3i = (void *)GetProcAddress(module, "glColor3i");
	BL_glColor3iv = (void *)GetProcAddress(module, "glColor3iv");
	BL_glColor3s = (void *)GetProcAddress(module, "glColor3s");
	BL_glColor3sv = (void *)GetProcAddress(module, "glColor3sv");
	BL_glColor3ub = (void *)GetProcAddress(module, "glColor3ub");
	BL_glColor3ubv = (void *)GetProcAddress(module, "glColor3ubv");
	BL_glColor3ui = (void *)GetProcAddress(module, "glColor3ui");
	BL_glColor3uiv = (void *)GetProcAddress(module, "glColor3uiv");
	BL_glColor3us = (void *)GetProcAddress(module, "glColor3us");
	BL_glColor3usv = (void *)GetProcAddress(module, "glColor3usv");
	BL_glColor4b = (void *)GetProcAddress(module, "glColor4b");
	BL_glColor4bv = (void *)GetProcAddress(module, "glColor4bv");
	BL_glColor4d = (void *)GetProcAddress(module, "glColor4d");
	BL_glColor4dv = (void *)GetProcAddress(module, "glColor4dv");
	BL_glColor4f = (void *)GetProcAddress(module, "glColor4f");
	BL_glColor4fv = (void *)GetProcAddress(module, "glColor4fv");
	BL_glColor4i = (void *)GetProcAddress(module, "glColor4i");
	BL_glColor4iv = (void *)GetProcAddress(module, "glColor4iv");
	BL_glColor4s = (void *)GetProcAddress(module, "glColor4s");
	BL_glColor4sv = (void *)GetProcAddress(module, "glColor4sv");
	BL_glColor4ub = (void *)GetProcAddress(module, "glColor4ub");
	BL_glColor4ubv = (void *)GetProcAddress(module, "glColor4ubv");
	BL_glColor4ui = (void *)GetProcAddress(module, "glColor4ui");
	BL_glColor4uiv = (void *)GetProcAddress(module, "glColor4uiv");
	BL_glColor4us = (void *)GetProcAddress(module, "glColor4us");
	BL_glColor4usv = (void *)GetProcAddress(module, "glColor4usv");
	BL_glColorMask = (void *)GetProcAddress(module, "glColorMask");
	BL_glColorMaterial = (void *)GetProcAddress(module, "glColorMaterial");
	BL_glColorPointer = (void *)GetProcAddress(module, "glColorPointer");
	BL_glColorSubTable = (void *)GetProcAddress(module, "glColorSubTable");
	BL_glColorTable = (void *)GetProcAddress(module, "glColorTable");
	BL_glColorTableParameterfv = (void *)GetProcAddress(module, "glColorTableParameterfv");
	BL_glColorTableParameteriv = (void *)GetProcAddress(module, "glColorTableParameteriv");
	BL_glConvolutionFilter1D = (void *)GetProcAddress(module, "glConvolutionFilter1D");
	BL_glConvolutionFilter2D = (void *)GetProcAddress(module, "glConvolutionFilter2D");
	BL_glConvolutionParameterf = (void *)GetProcAddress(module, "glConvolutionParameterf");
	BL_glConvolutionParameterfv = (void *)GetProcAddress(module, "glConvolutionParameterfv");
	BL_glConvolutionParameteri = (void *)GetProcAddress(module, "glConvolutionParameteri");
	BL_glConvolutionParameteriv = (void *)GetProcAddress(module, "glConvolutionParameteriv");
	BL_glCopyColorSubTable = (void *)GetProcAddress(module, "glCopyColorSubTable");
	BL_glCopyColorTable = (void *)GetProcAddress(module, "glCopyColorTable");
	BL_glCopyConvolutionFilter1D = (void *)GetProcAddress(module, "glCopyConvolutionFilter1D");
	BL_glCopyConvolutionFilter2D = (void *)GetProcAddress(module, "glCopyConvolutionFilter2D");
	BL_glCopyPixels = (void *)GetProcAddress(module, "glCopyPixels");
	BL_glCopyTexImage1D = (void *)GetProcAddress(module, "glCopyTexImage1D");
	BL_glCopyTexImage2D = (void *)GetProcAddress(module, "glCopyTexImage2D");
	BL_glCopyTexSubImage1D = (void *)GetProcAddress(module, "glCopyTexSubImage1D");
	BL_glCopyTexSubImage2D = (void *)GetProcAddress(module, "glCopyTexSubImage2D");
	BL_glCopyTexSubImage3D = (void *)GetProcAddress(module, "glCopyTexSubImage3D");
	BL_glCullFace = (void *)GetProcAddress(module, "glCullFace");
	BL_glDeleteLists = (void *)GetProcAddress(module, "glDeleteLists");
	BL_glDeleteTextures = (void *)GetProcAddress(module, "glDeleteTextures");
	BL_glDepthFunc = (void *)GetProcAddress(module, "glDepthFunc");
	BL_glDepthMask = (void *)GetProcAddress(module, "glDepthMask");
	BL_glDepthRange = (void *)GetProcAddress(module, "glDepthRange");
	BL_glDisable = (void *)GetProcAddress(module, "glDisable");
	BL_glDisableClientState = (void *)GetProcAddress(module, "glDisableClientState");
	BL_glDrawArrays = (void *)GetProcAddress(module, "glDrawArrays");
	BL_glDrawBuffer = (void *)GetProcAddress(module, "glDrawBuffer");
	BL_glDrawElements = (void *)GetProcAddress(module, "glDrawElements");
	BL_glDrawPixels = (void *)GetProcAddress(module, "glDrawPixels");
	BL_glDrawRangeElements = (void *)GetProcAddress(module, "glDrawRangeElements");
	BL_glEdgeFlag = (void *)GetProcAddress(module, "glEdgeFlag");
	BL_glEdgeFlagPointer = (void *)GetProcAddress(module, "glEdgeFlagPointer");
	BL_glEdgeFlagv = (void *)GetProcAddress(module, "glEdgeFlagv");
	BL_glEnable = (void *)GetProcAddress(module, "glEnable");
	BL_glEnableClientState = (void *)GetProcAddress(module, "glEnableClientState");
	BL_glEnd = (void *)GetProcAddress(module, "glEnd");
	BL_glEndList = (void *)GetProcAddress(module, "glEndList");
	BL_glEvalCoord1d = (void *)GetProcAddress(module, "glEvalCoord1d");
	BL_glEvalCoord1dv = (void *)GetProcAddress(module, "glEvalCoord1dv");
	BL_glEvalCoord1f = (void *)GetProcAddress(module, "glEvalCoord1f");
	BL_glEvalCoord1fv = (void *)GetProcAddress(module, "glEvalCoord1fv");
	BL_glEvalCoord2d = (void *)GetProcAddress(module, "glEvalCoord2d");
	BL_glEvalCoord2dv = (void *)GetProcAddress(module, "glEvalCoord2dv");
	BL_glEvalCoord2f = (void *)GetProcAddress(module, "glEvalCoord2f");
	BL_glEvalCoord2fv = (void *)GetProcAddress(module, "glEvalCoord2fv");
	BL_glEvalMesh1 = (void *)GetProcAddress(module, "glEvalMesh1");
	BL_glEvalMesh2 = (void *)GetProcAddress(module, "glEvalMesh2");
	BL_glEvalPoint1 = (void *)GetProcAddress(module, "glEvalPoint1");
	BL_glEvalPoint2 = (void *)GetProcAddress(module, "glEvalPoint2");
	BL_glFeedbackBuffer = (void *)GetProcAddress(module, "glFeedbackBuffer");
	BL_glFinish = (void *)GetProcAddress(module, "glFinish");
	BL_glFlush = (void *)GetProcAddress(module, "glFlush");
	BL_glFogf = (void *)GetProcAddress(module, "glFogf");
	BL_glFogfv = (void *)GetProcAddress(module, "glFogfv");
	BL_glFogi = (void *)GetProcAddress(module, "glFogi");
	BL_glFogiv = (void *)GetProcAddress(module, "glFogiv");
	BL_glFrontFace = (void *)GetProcAddress(module, "glFrontFace");
	BL_glFrustum = (void *)GetProcAddress(module, "glFrustum");
	BL_glGenLists = (void *)GetProcAddress(module, "glGenLists");
	BL_glGenTextures = (void *)GetProcAddress(module, "glGenTextures");
	BL_glGetBooleanv = (void *)GetProcAddress(module, "glGetBooleanv");
	BL_glGetClipPlane = (void *)GetProcAddress(module, "glGetClipPlane");
	BL_glGetColorTable = (void *)GetProcAddress(module, "glGetColorTable");
	BL_glGetColorTableParameterfv = (void *)GetProcAddress(module, "glGetColorTableParameterfv");
	BL_glGetColorTableParameteriv = (void *)GetProcAddress(module, "glGetColorTableParameteriv");
	BL_glGetConvolutionFilter = (void *)GetProcAddress(module, "glGetConvolutionFilter");
	BL_glGetConvolutionParameterfv = (void *)GetProcAddress(module, "glGetConvolutionParameterfv");
	BL_glGetConvolutionParameteriv = (void *)GetProcAddress(module, "glGetConvolutionParameteriv");
	BL_glGetDoublev = (void *)GetProcAddress(module, "glGetDoublev");
	BL_glGetError = (void *)GetProcAddress(module, "glGetError");
	BL_glGetFloatv = (void *)GetProcAddress(module, "glGetFloatv");
	BL_glGetHistogram = (void *)GetProcAddress(module, "glGetHistogram");
	BL_glGetHistogramParameterfv = (void *)GetProcAddress(module, "glGetHistogramParameterfv");
	BL_glGetHistogramParameteriv = (void *)GetProcAddress(module, "glGetHistogramParameteriv");
	BL_glGetIntegerv = (void *)GetProcAddress(module, "glGetIntegerv");
	BL_glGetLightfv = (void *)GetProcAddress(module, "glGetLightfv");
	BL_glGetLightiv = (void *)GetProcAddress(module, "glGetLightiv");
	BL_glGetMapdv = (void *)GetProcAddress(module, "glGetMapdv");
	BL_glGetMapfv = (void *)GetProcAddress(module, "glGetMapfv");
	BL_glGetMapiv = (void *)GetProcAddress(module, "glGetMapiv");
	BL_glGetMaterialfv = (void *)GetProcAddress(module, "glGetMaterialfv");
	BL_glGetMaterialiv = (void *)GetProcAddress(module, "glGetMaterialiv");
	BL_glGetMinmax = (void *)GetProcAddress(module, "glGetMinmax");
	BL_glGetMinmaxParameterfv = (void *)GetProcAddress(module, "glGetMinmaxParameterfv");
	BL_glGetMinmaxParameteriv = (void *)GetProcAddress(module, "glGetMinmaxParameteriv");
	BL_glGetPixelMapfv = (void *)GetProcAddress(module, "glGetPixelMapfv");
	BL_glGetPixelMapuiv = (void *)GetProcAddress(module, "glGetPixelMapuiv");
	BL_glGetPixelMapusv = (void *)GetProcAddress(module, "glGetPixelMapusv");
	BL_glGetPointerv = (void *)GetProcAddress(module, "glGetPointerv");
	BL_glGetPolygonStipple = (void *)GetProcAddress(module, "glGetPolygonStipple");
	BL_glGetSeparableFilter = (void *)GetProcAddress(module, "glGetSeparableFilter");
	BL_glGetString = (void *)GetProcAddress(module, "glGetString");
	BL_glGetTexEnvfv = (void *)GetProcAddress(module, "glGetTexEnvfv");
	BL_glGetTexEnviv = (void *)GetProcAddress(module, "glGetTexEnviv");
	BL_glGetTexGendv = (void *)GetProcAddress(module, "glGetTexGendv");
	BL_glGetTexGenfv = (void *)GetProcAddress(module, "glGetTexGenfv");
	BL_glGetTexGeniv = (void *)GetProcAddress(module, "glGetTexGeniv");
	BL_glGetTexImage = (void *)GetProcAddress(module, "glGetTexImage");
	BL_glGetTexLevelParameterfv = (void *)GetProcAddress(module, "glGetTexLevelParameterfv");
	BL_glGetTexLevelParameteriv = (void *)GetProcAddress(module, "glGetTexLevelParameteriv");
	BL_glGetTexParameterfv = (void *)GetProcAddress(module, "glGetTexParameterfv");
	BL_glGetTexParameteriv = (void *)GetProcAddress(module, "glGetTexParameteriv");
	BL_glHint = (void *)GetProcAddress(module, "glHint");
	BL_glHistogram = (void *)GetProcAddress(module, "glHistogram");
	BL_glIndexMask = (void *)GetProcAddress(module, "glIndexMask");
	BL_glIndexPointer = (void *)GetProcAddress(module, "glIndexPointer");
	BL_glIndexd = (void *)GetProcAddress(module, "glIndexd");
	BL_glIndexdv = (void *)GetProcAddress(module, "glIndexdv");
	BL_glIndexf = (void *)GetProcAddress(module, "glIndexf");
	BL_glIndexfv = (void *)GetProcAddress(module, "glIndexfv");
	BL_glIndexi = (void *)GetProcAddress(module, "glIndexi");
	BL_glIndexiv = (void *)GetProcAddress(module, "glIndexiv");
	BL_glIndexs = (void *)GetProcAddress(module, "glIndexs");
	BL_glIndexsv = (void *)GetProcAddress(module, "glIndexsv");
	BL_glIndexub = (void *)GetProcAddress(module, "glIndexub");
	BL_glIndexubv = (void *)GetProcAddress(module, "glIndexubv");
	BL_glInitNames = (void *)GetProcAddress(module, "glInitNames");
	BL_glInterleavedArrays = (void *)GetProcAddress(module, "glInterleavedArrays");
	BL_glIsEnabled = (void *)GetProcAddress(module, "glIsEnabled");
	BL_glIsList = (void *)GetProcAddress(module, "glIsList");
	BL_glIsTexture = (void *)GetProcAddress(module, "glIsTexture");
	BL_glLightModelf = (void *)GetProcAddress(module, "glLightModelf");
	BL_glLightModelfv = (void *)GetProcAddress(module, "glLightModelfv");
	BL_glLightModeli = (void *)GetProcAddress(module, "glLightModeli");
	BL_glLightModeliv = (void *)GetProcAddress(module, "glLightModeliv");
	BL_glLightf = (void *)GetProcAddress(module, "glLightf");
	BL_glLightfv = (void *)GetProcAddress(module, "glLightfv");
	BL_glLighti = (void *)GetProcAddress(module, "glLighti");
	BL_glLightiv = (void *)GetProcAddress(module, "glLightiv");
	BL_glLineStipple = (void *)GetProcAddress(module, "glLineStipple");
	BL_glLineWidth = (void *)GetProcAddress(module, "glLineWidth");
	BL_glListBase = (void *)GetProcAddress(module, "glListBase");
	BL_glLoadIdentity = (void *)GetProcAddress(module, "glLoadIdentity");
	BL_glLoadMatrixd = (void *)GetProcAddress(module, "glLoadMatrixd");
	BL_glLoadMatrixf = (void *)GetProcAddress(module, "glLoadMatrixf");
	BL_glLoadName = (void *)GetProcAddress(module, "glLoadName");
	BL_glLogicOp = (void *)GetProcAddress(module, "glLogicOp");
	BL_glMap1d = (void *)GetProcAddress(module, "glMap1d");
	BL_glMap1f = (void *)GetProcAddress(module, "glMap1f");
	BL_glMap2d = (void *)GetProcAddress(module, "glMap2d");
	BL_glMap2f = (void *)GetProcAddress(module, "glMap2f");
	BL_glMapGrid1d = (void *)GetProcAddress(module, "glMapGrid1d");
	BL_glMapGrid1f = (void *)GetProcAddress(module, "glMapGrid1f");
	BL_glMapGrid2d = (void *)GetProcAddress(module, "glMapGrid2d");
	BL_glMapGrid2f = (void *)GetProcAddress(module, "glMapGrid2f");
	BL_glMaterialf = (void *)GetProcAddress(module, "glMaterialf");
	BL_glMaterialfv = (void *)GetProcAddress(module, "glMaterialfv");
	BL_glMateriali = (void *)GetProcAddress(module, "glMateriali");
	BL_glMaterialiv = (void *)GetProcAddress(module, "glMaterialiv");
	BL_glMatrixMode = (void *)GetProcAddress(module, "glMatrixMode");
	BL_glMinmax = (void *)GetProcAddress(module, "glMinmax");
	BL_glMultMatrixd = (void *)GetProcAddress(module, "glMultMatrixd");
	BL_glMultMatrixf = (void *)GetProcAddress(module, "glMultMatrixf");
	BL_glNewList = (void *)GetProcAddress(module, "glNewList");
	BL_glNormal3b = (void *)GetProcAddress(module, "glNormal3b");
	BL_glNormal3bv = (void *)GetProcAddress(module, "glNormal3bv");
	BL_glNormal3d = (void *)GetProcAddress(module, "glNormal3d");
	BL_glNormal3dv = (void *)GetProcAddress(module, "glNormal3dv");
	BL_glNormal3f = (void *)GetProcAddress(module, "glNormal3f");
	BL_glNormal3fv = (void *)GetProcAddress(module, "glNormal3fv");
	BL_glNormal3i = (void *)GetProcAddress(module, "glNormal3i");
	BL_glNormal3iv = (void *)GetProcAddress(module, "glNormal3iv");
	BL_glNormal3s = (void *)GetProcAddress(module, "glNormal3s");
	BL_glNormal3sv = (void *)GetProcAddress(module, "glNormal3sv");
	BL_glNormalPointer = (void *)GetProcAddress(module, "glNormalPointer");
	BL_glOrtho = (void *)GetProcAddress(module, "glOrtho");
	BL_glPassThrough = (void *)GetProcAddress(module, "glPassThrough");
	BL_glPixelMapfv = (void *)GetProcAddress(module, "glPixelMapfv");
	BL_glPixelMapuiv = (void *)GetProcAddress(module, "glPixelMapuiv");
	BL_glPixelMapusv = (void *)GetProcAddress(module, "glPixelMapusv");
	BL_glPixelStoref = (void *)GetProcAddress(module, "glPixelStoref");
	BL_glPixelStorei = (void *)GetProcAddress(module, "glPixelStorei");
	BL_glPixelTransferf = (void *)GetProcAddress(module, "glPixelTransferf");
	BL_glPixelTransferi = (void *)GetProcAddress(module, "glPixelTransferi");
	BL_glPixelZoom = (void *)GetProcAddress(module, "glPixelZoom");
	BL_glPointSize = (void *)GetProcAddress(module, "glPointSize");
	BL_glPolygonMode = (void *)GetProcAddress(module, "glPolygonMode");
	BL_glPolygonOffset = (void *)GetProcAddress(module, "glPolygonOffset");
	BL_glPolygonStipple = (void *)GetProcAddress(module, "glPolygonStipple");
	BL_glPopAttrib = (void *)GetProcAddress(module, "glPopAttrib");
	BL_glPopClientAttrib = (void *)GetProcAddress(module, "glPopClientAttrib");
	BL_glPopMatrix = (void *)GetProcAddress(module, "glPopMatrix");
	BL_glPopName = (void *)GetProcAddress(module, "glPopName");
	BL_glPrioritizeTextures = (void *)GetProcAddress(module, "glPrioritizeTextures");
	BL_glPushAttrib = (void *)GetProcAddress(module, "glPushAttrib");
	BL_glPushClientAttrib = (void *)GetProcAddress(module, "glPushClientAttrib");
	BL_glPushMatrix = (void *)GetProcAddress(module, "glPushMatrix");
	BL_glPushName = (void *)GetProcAddress(module, "glPushName");
	BL_glRasterPos2d = (void *)GetProcAddress(module, "glRasterPos2d");
	BL_glRasterPos2dv = (void *)GetProcAddress(module, "glRasterPos2dv");
	BL_glRasterPos2f = (void *)GetProcAddress(module, "glRasterPos2f");
	BL_glRasterPos2fv = (void *)GetProcAddress(module, "glRasterPos2fv");
	BL_glRasterPos2i = (void *)GetProcAddress(module, "glRasterPos2i");
	BL_glRasterPos2iv = (void *)GetProcAddress(module, "glRasterPos2iv");
	BL_glRasterPos2s = (void *)GetProcAddress(module, "glRasterPos2s");
	BL_glRasterPos2sv = (void *)GetProcAddress(module, "glRasterPos2sv");
	BL_glRasterPos3d = (void *)GetProcAddress(module, "glRasterPos3d");
	BL_glRasterPos3dv = (void *)GetProcAddress(module, "glRasterPos3dv");
	BL_glRasterPos3f = (void *)GetProcAddress(module, "glRasterPos3f");
	BL_glRasterPos3fv = (void *)GetProcAddress(module, "glRasterPos3fv");
	BL_glRasterPos3i = (void *)GetProcAddress(module, "glRasterPos3i");
	BL_glRasterPos3iv = (void *)GetProcAddress(module, "glRasterPos3iv");
	BL_glRasterPos3s = (void *)GetProcAddress(module, "glRasterPos3s");
	BL_glRasterPos3sv = (void *)GetProcAddress(module, "glRasterPos3sv");
	BL_glRasterPos4d = (void *)GetProcAddress(module, "glRasterPos4d");
	BL_glRasterPos4dv = (void *)GetProcAddress(module, "glRasterPos4dv");
	BL_glRasterPos4f = (void *)GetProcAddress(module, "glRasterPos4f");
	BL_glRasterPos4fv = (void *)GetProcAddress(module, "glRasterPos4fv");
	BL_glRasterPos4i = (void *)GetProcAddress(module, "glRasterPos4i");
	BL_glRasterPos4iv = (void *)GetProcAddress(module, "glRasterPos4iv");
	BL_glRasterPos4s = (void *)GetProcAddress(module, "glRasterPos4s");
	BL_glRasterPos4sv = (void *)GetProcAddress(module, "glRasterPos4sv");
	BL_glReadBuffer = (void *)GetProcAddress(module, "glReadBuffer");
	BL_glReadPixels = (void *)GetProcAddress(module, "glReadPixels");
	BL_glRectd = (void *)GetProcAddress(module, "glRectd");
	BL_glRectdv = (void *)GetProcAddress(module, "glRectdv");
	BL_glRectf = (void *)GetProcAddress(module, "glRectf");
	BL_glRectfv = (void *)GetProcAddress(module, "glRectfv");
	BL_glRecti = (void *)GetProcAddress(module, "glRecti");
	BL_glRectiv = (void *)GetProcAddress(module, "glRectiv");
	BL_glRects = (void *)GetProcAddress(module, "glRects");
	BL_glRectsv = (void *)GetProcAddress(module, "glRectsv");
	BL_glRenderMode = (void *)GetProcAddress(module, "glRenderMode");
	BL_glResetHistogram = (void *)GetProcAddress(module, "glResetHistogram");
	BL_glResetMinmax = (void *)GetProcAddress(module, "glResetMinmax");
	BL_glRotated = (void *)GetProcAddress(module, "glRotated");
	BL_glRotatef = (void *)GetProcAddress(module, "glRotatef");
	BL_glScaled = (void *)GetProcAddress(module, "glScaled");
	BL_glScalef = (void *)GetProcAddress(module, "glScalef");
	BL_glScissor = (void *)GetProcAddress(module, "glScissor");
	BL_glSelectBuffer = (void *)GetProcAddress(module, "glSelectBuffer");
	BL_glSeparableFilter2D = (void *)GetProcAddress(module, "glSeparableFilter2D");
	BL_glShadeModel = (void *)GetProcAddress(module, "glShadeModel");
	BL_glStencilFunc = (void *)GetProcAddress(module, "glStencilFunc");
	BL_glStencilMask = (void *)GetProcAddress(module, "glStencilMask");
	BL_glStencilOp = (void *)GetProcAddress(module, "glStencilOp");
	BL_glTexCoord1d = (void *)GetProcAddress(module, "glTexCoord1d");
	BL_glTexCoord1dv = (void *)GetProcAddress(module, "glTexCoord1dv");
	BL_glTexCoord1f = (void *)GetProcAddress(module, "glTexCoord1f");
	BL_glTexCoord1fv = (void *)GetProcAddress(module, "glTexCoord1fv");
	BL_glTexCoord1i = (void *)GetProcAddress(module, "glTexCoord1i");
	BL_glTexCoord1iv = (void *)GetProcAddress(module, "glTexCoord1iv");
	BL_glTexCoord1s = (void *)GetProcAddress(module, "glTexCoord1s");
	BL_glTexCoord1sv = (void *)GetProcAddress(module, "glTexCoord1sv");
	BL_glTexCoord2d = (void *)GetProcAddress(module, "glTexCoord2d");
	BL_glTexCoord2dv = (void *)GetProcAddress(module, "glTexCoord2dv");
	BL_glTexCoord2f = (void *)GetProcAddress(module, "glTexCoord2f");
	BL_glTexCoord2fv = (void *)GetProcAddress(module, "glTexCoord2fv");
	BL_glTexCoord2i = (void *)GetProcAddress(module, "glTexCoord2i");
	BL_glTexCoord2iv = (void *)GetProcAddress(module, "glTexCoord2iv");
	BL_glTexCoord2s = (void *)GetProcAddress(module, "glTexCoord2s");
	BL_glTexCoord2sv = (void *)GetProcAddress(module, "glTexCoord2sv");
	BL_glTexCoord3d = (void *)GetProcAddress(module, "glTexCoord3d");
	BL_glTexCoord3dv = (void *)GetProcAddress(module, "glTexCoord3dv");
	BL_glTexCoord3f = (void *)GetProcAddress(module, "glTexCoord3f");
	BL_glTexCoord3fv = (void *)GetProcAddress(module, "glTexCoord3fv");
	BL_glTexCoord3i = (void *)GetProcAddress(module, "glTexCoord3i");
	BL_glTexCoord3iv = (void *)GetProcAddress(module, "glTexCoord3iv");
	BL_glTexCoord3s = (void *)GetProcAddress(module, "glTexCoord3s");
	BL_glTexCoord3sv = (void *)GetProcAddress(module, "glTexCoord3sv");
	BL_glTexCoord4d = (void *)GetProcAddress(module, "glTexCoord4d");
	BL_glTexCoord4dv = (void *)GetProcAddress(module, "glTexCoord4dv");
	BL_glTexCoord4f = (void *)GetProcAddress(module, "glTexCoord4f");
	BL_glTexCoord4fv = (void *)GetProcAddress(module, "glTexCoord4fv");
	BL_glTexCoord4i = (void *)GetProcAddress(module, "glTexCoord4i");
	BL_glTexCoord4iv = (void *)GetProcAddress(module, "glTexCoord4iv");
	BL_glTexCoord4s = (void *)GetProcAddress(module, "glTexCoord4s");
	BL_glTexCoord4sv = (void *)GetProcAddress(module, "glTexCoord4sv");
	BL_glTexCoordPointer = (void *)GetProcAddress(module, "glTexCoordPointer");
	BL_glTexEnvf = (void *)GetProcAddress(module, "glTexEnvf");
	BL_glTexEnvfv = (void *)GetProcAddress(module, "glTexEnvfv");
	BL_glTexEnvi = (void *)GetProcAddress(module, "glTexEnvi");
	BL_glTexEnviv = (void *)GetProcAddress(module, "glTexEnviv");
	BL_glTexGend = (void *)GetProcAddress(module, "glTexGend");
	BL_glTexGendv = (void *)GetProcAddress(module, "glTexGendv");
	BL_glTexGenf = (void *)GetProcAddress(module, "glTexGenf");
	BL_glTexGenfv = (void *)GetProcAddress(module, "glTexGenfv");
	BL_glTexGeni = (void *)GetProcAddress(module, "glTexGeni");
	BL_glTexGeniv = (void *)GetProcAddress(module, "glTexGeniv");
	BL_glTexImage1D = (void *)GetProcAddress(module, "glTexImage1D");
	BL_glTexImage2D = (void *)GetProcAddress(module, "glTexImage2D");
	BL_glTexImage3D = (void *)GetProcAddress(module, "glTexImage3D");
	BL_glTexParameterf = (void *)GetProcAddress(module, "glTexParameterf");
	BL_glTexParameterfv = (void *)GetProcAddress(module, "glTexParameterfv");
	BL_glTexParameteri = (void *)GetProcAddress(module, "glTexParameteri");
	BL_glTexParameteriv = (void *)GetProcAddress(module, "glTexParameteriv");
	BL_glTexSubImage1D = (void *)GetProcAddress(module, "glTexSubImage1D");
	BL_glTexSubImage2D = (void *)GetProcAddress(module, "glTexSubImage2D");
	BL_glTexSubImage3D = (void *)GetProcAddress(module, "glTexSubImage3D");
	BL_glTranslated = (void *)GetProcAddress(module, "glTranslated");
	BL_glTranslatef = (void *)GetProcAddress(module, "glTranslatef");
	BL_glVertex2d = (void *)GetProcAddress(module, "glVertex2d");
	BL_glVertex2dv = (void *)GetProcAddress(module, "glVertex2dv");
	BL_glVertex2f = (void *)GetProcAddress(module, "glVertex2f");
	BL_glVertex2fv = (void *)GetProcAddress(module, "glVertex2fv");
	BL_glVertex2i = (void *)GetProcAddress(module, "glVertex2i");
	BL_glVertex2iv = (void *)GetProcAddress(module, "glVertex2iv");
	BL_glVertex2s = (void *)GetProcAddress(module, "glVertex2s");
	BL_glVertex2sv = (void *)GetProcAddress(module, "glVertex2sv");
	BL_glVertex3d = (void *)GetProcAddress(module, "glVertex3d");
	BL_glVertex3dv = (void *)GetProcAddress(module, "glVertex3dv");
	BL_glVertex3f = (void *)GetProcAddress(module, "glVertex3f");
	BL_glVertex3fv = (void *)GetProcAddress(module, "glVertex3fv");
	BL_glVertex3i = (void *)GetProcAddress(module, "glVertex3i");
	BL_glVertex3iv = (void *)GetProcAddress(module, "glVertex3iv");
	BL_glVertex3s = (void *)GetProcAddress(module, "glVertex3s");
	BL_glVertex3sv = (void *)GetProcAddress(module, "glVertex3sv");
	BL_glVertex4d = (void *)GetProcAddress(module, "glVertex4d");
	BL_glVertex4dv = (void *)GetProcAddress(module, "glVertex4dv");
	BL_glVertex4f = (void *)GetProcAddress(module, "glVertex4f");
	BL_glVertex4fv = (void *)GetProcAddress(module, "glVertex4fv");
	BL_glVertex4i = (void *)GetProcAddress(module, "glVertex4i");
	BL_glVertex4iv = (void *)GetProcAddress(module, "glVertex4iv");
	BL_glVertex4s = (void *)GetProcAddress(module, "glVertex4s");
	BL_glVertex4sv = (void *)GetProcAddress(module, "glVertex4sv");
	BL_glVertexPointer = (void *)GetProcAddress(module, "glVertexPointer");
	BL_glViewport = (void *)GetProcAddress(module, "glViewport");

	// Load - WGL
	BL_wglCopyContext = (void *)GetProcAddress(module, "wglCopyContext");
	BL_wglCreateContext = (void *)GetProcAddress(module, "wglCreateContext");
	BL_wglCreateLayerContext = (void *)GetProcAddress(module, "wglCreateLayerContext");
	BL_wglDeleteContext = (void *)GetProcAddress(module, "wglDeleteContext");
	BL_wglDescribeLayerPlane = (void *)GetProcAddress(module, "wglDescribeLayerPlane");
	BL_wglGetCurrentContext = (void *)GetProcAddress(module, "wglGetCurrentContext");
	BL_wglGetCurrentDC = (void *)GetProcAddress(module, "wglGetCurrentDC");
	BL_wglGetLayerPaletteEntries = (void *)GetProcAddress(module, "wglGetLayerPaletteEntries");
	BL_wglGetProcAddress = (void *)GetProcAddress(module, "wglGetProcAddress");
	BL_wglMakeCurrent = (void *)GetProcAddress(module, "wglMakeCurrent");
	BL_wglRealizeLayerPalette = (void *)GetProcAddress(module, "wglRealizeLayerPalette");
	BL_wglSetLayerPaletteEntries = (void *)GetProcAddress(module, "wglSetLayerPaletteEntries");
	BL_wglShareLists = (void *)GetProcAddress(module, "wglShareLists");
	BL_wglSwapLayerBuffers = (void *)GetProcAddress(module, "wglSwapLayerBuffers");
	BL_wglUseFontBitmaps = (void *)GetProcAddress(module, "wglUseFontBitmaps");
	BL_wglUseFontBitmapsA = (void *)GetProcAddress(module, "wglUseFontBitmapsA");
	BL_wglUseFontBitmapsW = (void *)GetProcAddress(module, "wglUseFontBitmapsW");
	BL_wglUseFontOutlines = (void *)GetProcAddress(module, "wglUseFontOutlines");
	BL_wglUseFontOutlinesA = (void *)GetProcAddress(module, "wglUseFontOutlinesA");
	BL_wglUseFontOutlinesW = (void *)GetProcAddress(module, "wglUseFontOutlinesW");
}

int
GLWrapperInit()
{
	//HMODULE module = LoadLibraryA("opengl32.dll");
	HMODULE module = LoadLibrary("opengl32.dll");
	//HMODULE module = LoadLibrary("c:/Windows/System32/opengl32.dll");
	if (module == NULL)
		return 0;

	TCHAR buffer[MAX_PATH];
	GetModuleFileName(module, buffer, MAX_PATH);

	LoadFunctionPointers(module);

	return 1;
}
