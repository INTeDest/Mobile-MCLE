#pragma once

#if defined(__ANDROID__) || defined(GLES)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
#include <GL/glew.h>
#endif

#include <stdio.h>
#include <stdbool.h>

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

static inline void glColorMaterial(GLenum face, GLenum mode) { (void)face; (void)mode; }
static inline void glTexGeni(GLenum coord, GLenum pname, int param) { (void)coord; (void)pname; (void)param; }
static inline void glTexGenfv(GLenum coord, GLenum pname, const float* params) { (void)coord; (void)pname; (void)params; }
static inline void glTexCoordPointer(int size, GLenum type, int stride, const void* pointer) { (void)size; (void)type; (void)stride; (void)pointer; }
static inline void glNormalPointer(GLenum type, int stride, const void* pointer) { (void)type; (void)stride; (void)pointer; }
static inline void glColorPointer(int size, GLenum type, int stride, const void* pointer) { (void)size; (void)type; (void)stride; (void)pointer; }
static inline void glVertexPointer(int size, GLenum type, int stride, const void* pointer) { (void)size; (void)type; (void)stride; (void)pointer; }
static inline void glEndList(void) {}
static inline void glCallLists(int n, GLenum type, const void* lists) { (void)n; (void)type; (void)lists; }
static inline void glShadeModel(GLenum mode) { (void)mode; }
static inline void glAlphaFunc(GLenum func, float ref) { (void)func; (void)ref; }
static inline void glFogf(GLenum pname, float param) { (void)pname; (void)param; }
static inline void glFogi(GLenum pname, int param) { (void)pname; (void)param; }
static inline void glFogfv(GLenum pname, const float* params) { (void)pname; (void)params; }
static inline void glLightfv(GLenum light, GLenum pname, const float* params) { (void)light; (void)pname; (void)params; }
static inline void glLightf(GLenum light, GLenum pname, float param) { (void)light; (void)pname; (void)param; }
static inline void glLighti(GLenum light, GLenum pname, int param) { (void)light; (void)pname; (void)param; }
static inline void glLightModelfv(GLenum pname, const float* params) { (void)pname; (void)params; }
static inline void glLightModeli(GLenum pname, int param) { (void)pname; (void)param; }
static inline void glLightModelf(GLenum pname, float param) { (void)pname; (void)param; }
static inline void glMaterialfv(GLenum face, GLenum pname, const float* params) { (void)face; (void)pname; (void)params; }
static inline void glMaterialf(GLenum face, GLenum pname, float param) { (void)face; (void)pname; (void)param; }
static inline void glOrtho(double l, double r, double b, double t, double zn, double zf) { (void)l; (void)r; (void)b; (void)t; (void)zn; (void)zf; }
static inline void glFrustum(double l, double r, double b, double t, double zn, double zf) { (void)l; (void)r; (void)b; (void)t; (void)zn; (void)zf; }
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

#ifndef GL_LUMINANCE8_ALPHA8
#define GL_LUMINANCE8_ALPHA8 0x8048
#endif
#ifndef GL_LUMINANCE4_ALPHA4
#define GL_LUMINANCE4_ALPHA4 0x8043
#endif
#ifndef GL_INTENSITY8
#define GL_INTENSITY8 0x804B
#endif
#ifndef GL_INTENSITY4
#define GL_INTENSITY4 0x804A
#endif
#ifndef GL_LUMINANCE8
#define GL_LUMINANCE8 0x8040
#endif
#ifndef GL_LUMINANCE4
#define GL_LUMINANCE4 0x803F
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

// ============================================================================
// PlatformRenderer-based implementation (from gl_compat.h)
// ============================================================================

#ifdef __cplusplus

#include "../IPlatformRenderer.h"
#include "../renderer.h"

// Undefine and redefine GL functions to use PlatformRenderer
#undef glNewList
#define glNewList(_list, _mode) PlatformRenderer.CBuffStart(_list)
#undef glEndList
#define glEndList() PlatformRenderer.CBuffEnd()
#undef glCallList
#define glCallList(_list) ((void)PlatformRenderer.CBuffCall(_list))

#undef glGenLists
#define glGenLists(range) PlatformRenderer.CBuffCreate(range)
#undef glDeleteLists
#define glDeleteLists(list, range) PlatformRenderer.CBuffDelete(list, range)

#ifndef GL_SHADEMODEL_IS_FUNCTION
#undef glShadeModel
#define GL_SHADEMODEL_IS_FUNCTION
static inline void glShadeModel(int mode) { (void)mode; }
#endif

#undef glTranslatef
#define glTranslatef(x, y, z)                      \
    do {                                           \
        PlatformRenderer.MatrixTranslate(x, y, z); \
    } while (0)

#undef glRotatef
#define glRotatef(a, x, y, z)                                                  \
    do {                                                                       \
        PlatformRenderer.MatrixRotate((a) * (3.14159265358979f / 180.f), x, y, \
                                      z);                                      \
    } while (0)

#undef glScalef
#define glScalef(x, y, z)                      \
    do {                                       \
        PlatformRenderer.MatrixScale(x, y, z); \
    } while (0)

#undef glScaled
#define glScaled(x, y, z)                                                 \
    do {                                                                  \
        PlatformRenderer.MatrixScale((float)(x), (float)(y), (float)(z)); \
    } while (0)

#undef glPushMatrix
#define glPushMatrix()                 \
    do {                               \
        PlatformRenderer.MatrixPush(); \
    } while (0)

#undef glPopMatrix
#define glPopMatrix()                 \
    do {                              \
        PlatformRenderer.MatrixPop(); \
    } while (0)

#undef glLoadIdentity
#define glLoadIdentity()                      \
    do {                                      \
        PlatformRenderer.MatrixSetIdentity(); \
    } while (0)

#undef glMatrixMode
#define glMatrixMode(mode)                 \
    do {                                   \
        PlatformRenderer.MatrixMode(mode); \
    } while (0)

#undef glMultMatrixf
#define glMultMatrixf(m)                \
    do {                                \
        PlatformRenderer.MatrixMult(m); \
    } while (0)

#undef glColor4f
#define glColor4f(r, g, b, a)                        \
    do {                                             \
        PlatformRenderer.StateSetColour(r, g, b, a); \
    } while (0)

#undef glColor3f
#define glColor3f(r, g, b)                              \
    do {                                                \
        PlatformRenderer.StateSetColour(r, g, b, 1.0f); \
    } while (0)

#undef glAlphaFunc
#define glAlphaFunc(func, ref)                         \
    do {                                               \
        PlatformRenderer.StateSetAlphaFunc(func, ref); \
    } while (0)

#undef glEnable
#define glEnable(cap)                                                   \
    do {                                                                \
        if ((cap) == 0x0B60 /*GL_FOG*/)                                 \
            PlatformRenderer.StateSetFogEnable(true);                   \
        else if ((cap) == 0x0B50 /*GL_LIGHTING*/)                       \
            PlatformRenderer.StateSetLightingEnable(true);              \
        else if ((cap) == 0x0BC0 /*GL_ALPHA_TEST*/)                     \
            PlatformRenderer.StateSetAlphaTestEnable(true);             \
        else if ((cap) == 0x0DE1 /*GL_TEXTURE_2D*/)                     \
            PlatformRenderer.StateSetTextureEnable(true);               \
        else if ((cap) == 0x0BE2 /*GL_BLEND*/)                          \
            PlatformRenderer.StateSetBlendEnable(true);                 \
        else if ((cap) == 0x0B44 /*GL_CULL_FACE*/)                      \
            PlatformRenderer.StateSetFaceCull(true);                    \
        else if ((cap) == 0x0B71 /*GL_DEPTH_TEST*/)                     \
            PlatformRenderer.StateSetDepthTestEnable(true);             \
        else if ((cap) == 0x4000 /*GL_LIGHT0*/)                         \
            PlatformRenderer.StateSetLightEnable(0, true);              \
        else if ((cap) == 0x4001 /*GL_LIGHT1*/)                         \
            PlatformRenderer.StateSetLightEnable(1, true);              \
        else if ((cap) == 0x0B57    /*GL_COLOR_MATERIAL*/               \
                 || (cap) == 0x0BA1 /*GL_NORMALIZE*/                    \
                 || (cap) == 0x803A /*GL_RESCALE_NORMAL*/               \
                 || (cap) == 0x0C60 /*GL_TEXTURE_GEN_S*/                \
                 || (cap) == 0x0C61 /*GL_TEXTURE_GEN_T*/                \
                 || (cap) == 0x0C62 /*GL_TEXTURE_GEN_R*/                \
                 || (cap) == 0x0C63 /*GL_TEXTURE_GEN_Q*/) { /* empty */ \
        } else                                                          \
            ::glEnable(cap);                                            \
    } while (0)

#undef glDisable
#define glDisable(cap)                                                  \
    do {                                                                \
        if ((cap) == 0x0B60 /*GL_FOG*/)                                 \
            PlatformRenderer.StateSetFogEnable(false);                  \
        else if ((cap) == 0x0B50 /*GL_LIGHTING*/)                       \
            PlatformRenderer.StateSetLightingEnable(false);             \
        else if ((cap) == 0x0BC0 /*GL_ALPHA_TEST*/)                     \
            PlatformRenderer.StateSetAlphaTestEnable(false);            \
        else if ((cap) == 0x0DE1 /*GL_TEXTURE_2D*/)                     \
            PlatformRenderer.StateSetTextureEnable(false);              \
        else if ((cap) == 0x0BE2 /*GL_BLEND*/)                          \
            PlatformRenderer.StateSetBlendEnable(false);                \
        else if ((cap) == 0x0B44 /*GL_CULL_FACE*/)                      \
            PlatformRenderer.StateSetFaceCull(false);                   \
        else if ((cap) == 0x0B71 /*GL_DEPTH_TEST*/)                     \
            PlatformRenderer.StateSetDepthTestEnable(false);            \
        else if ((cap) == 0x4000 /*GL_LIGHT0*/)                         \
            PlatformRenderer.StateSetLightEnable(0, false);             \
        else if ((cap) == 0x4001 /*GL_LIGHT1*/)                         \
            PlatformRenderer.StateSetLightEnable(1, false);             \
        else if ((cap) == 0x0B57    /*GL_COLOR_MATERIAL*/               \
                 || (cap) == 0x0BA1 /*GL_NORMALIZE*/                    \
                 || (cap) == 0x803A /*GL_RESCALE_NORMAL*/               \
                 || (cap) == 0x0C60 /*GL_TEXTURE_GEN_S*/                \
                 || (cap) == 0x0C61 /*GL_TEXTURE_GEN_T*/                \
                 || (cap) == 0x0C62 /*GL_TEXTURE_GEN_R*/                \
                 || (cap) == 0x0C63 /*GL_TEXTURE_GEN_Q*/) { /* empty */ \
        } else                                                          \
            ::glDisable(cap);                                           \
    } while (0)

#undef glFogi
#define glFogi(pname, param)                         \
    do {                                             \
        if ((pname) == 0x0B65 /*GL_FOG_MODE*/)       \
            PlatformRenderer.StateSetFogMode(param); \
    } while (0)

#undef glFogf
#define glFogf(pname, param)                                 \
    do {                                                     \
        if ((pname) == 0x0B63 /*GL_FOG_START*/)              \
            PlatformRenderer.StateSetFogNearDistance(param); \
        else if ((pname) == 0x0B64 /*GL_FOG_END*/)           \
            PlatformRenderer.StateSetFogFarDistance(param);  \
        else if ((pname) == 0x0B62 /*GL_FOG_DENSITY*/)       \
            PlatformRenderer.StateSetFogDensity(param);      \
    } while (0)

#undef glOrtho
#define glOrtho(left, right, bottom, top, zNear, zFar)                     \
    do {                                                                   \
        PlatformRenderer.MatrixOrthogonal(left, right, bottom, top, zNear, \
                                          zFar);                           \
    } while (0)

#undef glMultiTexCoord2f
#define glMultiTexCoord2f(tex, u, v)                        \
    do {                                                    \
        if ((tex) == 0x84C1 /*GL_TEXTURE1*/)                \
            PlatformRenderer.StateSetVertexTextureUV(u, v); \
    } while (0)

#undef glActiveTexture
#define glActiveTexture(tex)                         \
    do {                                             \
        PlatformRenderer.StateSetActiveTexture(tex); \
    } while (0)

#undef glClientActiveTexture
#define glClientActiveTexture(tex)                   \
    do {                                             \
        PlatformRenderer.StateSetActiveTexture(tex); \
    } while (0)

// Template functions originally from gl_compat.h
int glGenTextures_4J();
void glGenTextures_4J(int n, unsigned int* textures);
void glDeleteTextures_4J(int id);
void glDeleteTextures_4J(int n, const unsigned int* textures);
void glTexImage2D_4J(int target, int level, int internalformat, int width,
                     int height, int border, int format, int type,
                     void* pixels);

template <typename T>
inline void glGenTextures_4J(T* buf) {
    unsigned int id = 0;
    ::glGenTextures(1, &id);
    buf->put((int)id);
    buf->flip();
}

template <typename T>
inline void glDeleteTextures_4J(T* buf) {
    if (buf->limit() > 0) {
        unsigned int id = (unsigned int)buf->get(0);
        ::glDeleteTextures(1, &id);
    }
}

template <typename T>
inline void glTexCoordPointer_4J(int size, int type, T* pointer) { (void)size; (void)type; (void)pointer; }
template <typename T>
inline void glNormalPointer_4J(int type, T* pointer) { (void)type; (void)pointer; }
template <typename T>
inline void glColorPointer_4J(int size, bool normalized, int stride, T* pointer) { (void)size; (void)normalized; (void)stride; (void)pointer; }
template <typename T>
inline void glVertexPointer_4J(int size, int type, T* pointer) { (void)size; (void)type; (void)pointer; }

template <typename T>
inline void glTexImage2D_4J(int target, int level, int internalformat,
                            int width, int height, int border, int format,
                            int type, T* pixels) {
    void* data = pixels ? pixels->getBuffer() : nullptr;
    ::glTexImage2D((unsigned int)target, level, internalformat, width, height,
                   border, (unsigned int)format, (unsigned int)type, data);
}

template <typename T>
inline void glCallLists_4J(T* lists) {
    int base = lists->position();
    int count = lists->limit() - base;
    for (int i = 0; i < count; i++) {
        PlatformRenderer.CBuffCall(lists->get(base + i));
    }
}

template <typename T>
inline void glFog_4J(int pname, T* params) {
    float* p = params->_getDataPointer();
    if (pname == 0x0B66 /* GL_FOG_COLOR */)
        PlatformRenderer.StateSetFogColour(p[0], p[1], p[2]);
}

template <typename T>
inline void glLight_4J(int light, int pname, T* params) {
    float* p = params->_getDataPointer();
    if (pname == 0x1203 /* GL_POSITION */)
        PlatformRenderer.StateSetLightDirection(light == 0x4000 ? 0 : 1, p[0],
                                                p[1], p[2]);
    else if (pname == 0x1200 /* GL_AMBIENT */)
        PlatformRenderer.StateSetLightAmbientColour(p[0], p[1], p[2]);
    else if (pname == 0x1201 /* GL_DIFFUSE */)
        PlatformRenderer.StateSetLightColour(light == 0x4000 ? 0 : 1, p[0],
                                             p[1], p[2]);
}

template <typename T>
inline void glLightModel_4J(int pname, T* params) {
    float* p = params->_getDataPointer();
    if (pname == 0x0B53 /* GL_LIGHT_MODEL_AMBIENT */)
        PlatformRenderer.StateSetLightAmbientColour(p[0], p[1], p[2]);
}

template <typename T>
inline void glTexGen_4J(int coord, int pname, T* params) { (void)coord; (void)pname; (void)params; }

inline void glReadPixels_4J(int x, int y, int width, int height, int format,
                            int type, void* pixels) {
    ::glReadPixels(x, y, width, height, (unsigned int)format,
                   (unsigned int)type, pixels);
}

inline void glReadPixels_4J(int x, int y, int width, int height, int format,
                            int type, unsigned char* pixels) {
    ::glReadPixels(x, y, width, height, (unsigned int)format,
                   (unsigned int)type, (void*)pixels);
}

template <typename T>
inline void glReadPixels_4J(int x, int y, int width, int height, int format,
                            int type, T* pixels) {
    ::glReadPixels(x, y, width, height, (unsigned int)format,
                   (unsigned int)type, pixels->getBuffer());
}

#define glGenTextures(...) glGenTextures_4J(__VA_ARGS__)
#define glDeleteTextures(...) glDeleteTextures_4J(__VA_ARGS__)
#define glTexCoordPointer(a, b, c) glTexCoordPointer_4J(a, b, c)
#define glNormalPointer(a, b) glNormalPointer_4J(a, b)
#define glColorPointer(a, b, c, d) glColorPointer_4J(a, b, c, d)
#define glVertexPointer(a, b, c) glVertexPointer_4J(a, b, c)
#define glTexImage2D(a, b, c, d, e, f, g, h, i) \
    glTexImage2D_4J(a, b, c, d, e, f, g, h, i)
#define glCallLists(x) glCallLists_4J(x)
#define glReadPixels(a, b, c, d, e, f, g) glReadPixels_4J(a, b, c, d, e, f, g)
#define glFog(a, b) glFog_4J(a, b)
#define glLight(a, b, c) glLight_4J(a, b, c)
#define glLightModel(a, b) glLightModel_4J(a, b)
#define glTexGen(a, b, c) glTexGen_4J(a, b, c)

#endif // __cplusplus