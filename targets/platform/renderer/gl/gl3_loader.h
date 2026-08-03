#pragma once

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <stdio.h>

#ifdef __cplusplus
#include <atomic>

namespace std {
#if !defined(__cpp_lib_atomic_ref)
template <typename T>
struct atomic_ref {
    T& ptr;
    explicit atomic_ref(T& ref) : ptr(ref) {}

    bool compare_exchange_strong(T& expected, T desired,
                                 std::memory_order = std::memory_order_seq_cst,
                                 std::memory_order = std::memory_order_seq_cst) noexcept {
        return __atomic_compare_exchange_n(&ptr, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }

    bool compare_exchange_weak(T& expected, T desired,
                               std::memory_order = std::memory_order_seq_cst,
                               std::memory_order = std::memory_order_seq_cst) noexcept {
        return __atomic_compare_exchange_n(&ptr, &expected, desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }

    T load(std::memory_order = std::memory_order_seq_cst) const noexcept {
        return __atomic_load_n(&ptr, __ATOMIC_SEQ_CST);
    }

    void store(T desired, std::memory_order = std::memory_order_seq_cst) noexcept {
        __atomic_store_n(&ptr, desired, __ATOMIC_SEQ_CST);
    }

    T exchange(T desired, std::memory_order = std::memory_order_seq_cst) noexcept {
        return __atomic_exchange_n(&ptr, desired, __ATOMIC_SEQ_CST);
    }
};
#endif
}
#endif

static inline bool gl3_load(void) {
    return true;
}

// Inline compatibility functions for legacy GL calls in C and C++
#ifdef __cplusplus
extern "C" {
#endif

static inline void glBegin(GLenum mode) { (void)mode; }
static inline void glEnd(void) {}

static inline void glVertex2f(float x, float y) { (void)x; (void)y; }
static inline void glVertex2i(int x, int y) { (void)x; (void)y; }
static inline void glVertex3f(float x, float y, float z) { (void)x; (void)y; (void)z; }
static inline void glVertex3d(double x, double y, double z) { (void)x; (void)y; (void)z; }
static inline void glVertex3fv(const float* v) { (void)v; }

static inline void glNormal3f(float nx, float ny, float nz) { (void)nx; (void)ny; (void)nz; }
static inline void glNormal3fv(const float* v) { (void)v; }
static inline void glNormal3b(signed char nx, signed char ny, signed char nz) { (void)nx; (void)ny; (void)nz; }

static inline void glTexCoord2f(float s, float t) { (void)s; (void)t; }
static inline void glTexCoord2d(double s, double t) { (void)s; (void)t; }
static inline void glTexCoord2fv(const float* v) { (void)v; }

static inline void glColor3f(float r, float g, float b) { (void)r; (void)g; (void)b; }
static inline void glColor3ub(unsigned char r, unsigned char g, unsigned char b) { (void)r; (void)g; (void)b; }
static inline void glColor4f(float r, float g, float b, float a) { (void)r; (void)g; (void)b; (void)a; }
static inline void glColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { (void)r; (void)g; (void)b; (void)a; }
static inline void glColor4fv(const float* v) { (void)v; }

static inline void glRectf(float x1, float y1, float x2, float y2) { (void)x1; (void)y1; (void)x2; (void)y2; }

static inline void glClearDepth(double depth) {
    glClearDepthf((float)depth);
}

static inline void glColorMaterial(GLenum face, GLenum mode) {
    (void)face;
    (void)mode;
}

static inline void glTexGeni(GLenum coord, GLenum pname, int param) {
    (void)coord; (void)pname; (void)param;
}

static inline void glTexGenfv(GLenum coord, GLenum pname, const float* params) {
    (void)coord; (void)pname; (void)params;
}

static inline void glTexCoordPointer(int size, GLenum type, int stride, const void* pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}

static inline void glNormalPointer(GLenum type, int stride, const void* pointer) {
    (void)type; (void)stride; (void)pointer;
}

static inline void glColorPointer(int size, GLenum type, int stride, const void* pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}

static inline void glVertexPointer(int size, GLenum type, int stride, const void* pointer) {
    (void)size; (void)type; (void)stride; (void)pointer;
}

static inline void glEndList(void) {}

static inline void glCallLists(int n, GLenum type, const void* lists) {
    (void)n; (void)type; (void)lists;
}

static inline void glShadeModel(GLenum mode) {
    (void)mode;
}

static inline void glAlphaFunc(GLenum func, float ref) {
    (void)func; (void)ref;
}

static inline void glFogf(GLenum pname, float param) {
    (void)pname; (void)param;
}

static inline void glFogi(GLenum pname, int param) {
    (void)pname; (void)param;
}

static inline void glFogfv(GLenum pname, const float* params) {
    (void)pname; (void)params;
}

static inline void glLightfv(GLenum light, GLenum pname, const float* params) {
    (void)light; (void)pname; (void)params;
}

static inline void glLightf(GLenum light, GLenum pname, float param) {
    (void)light; (void)pname; (void)param;
}

static inline void glLighti(GLenum light, GLenum pname, int param) {
    (void)light; (void)pname; (void)param;
}

static inline void glLightModelfv(GLenum pname, const float* params) {
    (void)pname; (void)params;
}

static inline void glLightModeli(GLenum pname, int param) {
    (void)pname; (void)param;
}

static inline void glLightModelf(GLenum pname, float param) {
    (void)pname; (void)param;
}

static inline void glMaterialfv(GLenum face, GLenum pname, const float* params) {
    (void)face; (void)pname; (void)params;
}

static inline void glMaterialf(GLenum face, GLenum pname, float param) {
    (void)face; (void)pname; (void)param;
}

static inline void glOrtho(double l, double r, double b, double t, double zn, double zf) {
    (void)l; (void)r; (void)b; (void)t; (void)zn; (void)zf;
}

static inline void glFrustum(double l, double r, double b, double t, double zn, double zf) {
    (void)l; (void)r; (void)b; (void)t; (void)zn; (void)zf;
}

static inline void glPushMatrix(void) {}
static inline void glPopMatrix(void) {}
static inline void glLoadIdentity(void) {}
static inline void glMatrixMode(GLenum mode) { (void)mode; }
static inline void glTranslatef(float x, float y, float z) { (void)x; (void)y; (void)z; }
static inline void glTranslated(double x, double y, double z) { (void)x; (void)y; (void)z; }
static inline void glRotatef(float angle, float x, float y, float z) { (void)angle; (void)x; (void)y; (void)z; }
static inline void glRotated(double angle, double x, double y, double z) { (void)angle; (void)x; (void)y; (void)z; }
static inline void glScalef(float x, float y, float z) { (void)x; (void)y; (void)z; }
static inline void glScaled(double x, double y, double z) { (void)x; (void)y; (void)z; }
static inline void glLoadMatrixf(const float* m) { (void)m; }
static inline void glLoadMatrixd(const double* m) { (void)m; }
static inline void glMultMatrixf(const float* m) { (void)m; }
static inline void glMultMatrixd(const double* m) { (void)m; }
static inline void glEnableClientState(GLenum array) { (void)array; }
static inline void glDisableClientState(GLenum array) { (void)array; }
static inline void glClientActiveTexture(GLenum texture) { (void)texture; }
static inline GLuint glGenLists(int range) { (void)range; return 1; }
static inline void glNewList(GLuint list, GLenum mode) { (void)list; (void)mode; }
static inline void glCallList(GLuint list) { (void)list; }
static inline void glDeleteLists(GLuint list, int range) { (void)list; (void)range; }

#ifdef __cplusplus
}
#endif

// Fixed-Function OpenGL constants missing in GLES3
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88B4
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif

#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif

#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_TEXTURE_MATRIX
#define GL_TEXTURE_MATRIX 0x0BA8
#endif
#ifndef GL_MATRIX_MODE
#define GL_MATRIX_MODE 0x0BA0
#endif

#ifndef GL_COLOR_MATERIAL
#define GL_COLOR_MATERIAL 0x0B57
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE 0x0BA1
#endif
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 0x803A
#endif

#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LIGHT0
#define GL_LIGHT0 0x4000
#endif
#ifndef GL_LIGHT1
#define GL_LIGHT1 0x4001
#endif

#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif

#ifndef GL_EXP
#define GL_EXP 0x0800
#endif
#ifndef GL_EXP2
#define GL_EXP2 0x0801
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif

#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif

#ifndef GL_POLYGON_OFFSET_POINT
#define GL_POLYGON_OFFSET_POINT 0x2A01
#endif
#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE 0x2A02
#endif
#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif

#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif

#ifndef GL_AMBIENT_AND_DIFFUSE
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#endif
#ifndef GL_EMISSION
#define GL_EMISSION 0x1600
#endif
#ifndef GL_SHININESS
#define GL_SHININESS 0x1601
#endif

#ifndef GL_SMOOTH
#define GL_SMOOTH 0x1D01
#endif
#ifndef GL_FLAT
#define GL_FLAT 0x1D00
#endif

#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif
#ifndef GL_POSITION
#define GL_POSITION 0x1203
#endif
#ifndef GL_LIGHT_MODEL_AMBIENT
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#endif

#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S 0x0C60
#endif
#ifndef GL_TEXTURE_GEN_T
#define GL_TEXTURE_GEN_T 0x0C61
#endif
#ifndef GL_TEXTURE_GEN_Q
#define GL_TEXTURE_GEN_Q 0x0C63
#endif
#ifndef GL_TEXTURE_GEN_R
#define GL_TEXTURE_GEN_R 0x0C62
#endif