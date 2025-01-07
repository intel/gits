// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// When defined, _PERF_MODE_ assumes the existence of file gitsData.raw and preloads it into memory before use.
//#define _PERF_MODE_

// When defined, _TIMING_MASK_AND_FRAME_LOOP_ allows passing parameters to time GLblocks and loop the rendered image.
//#define _TIMING_MASK_AND_FRAME_LOOP_

#include "helper.h"

#ifdef GITS_API_OGL
#include "openglEnums.h"
#include "openglTypes.h"

#include "windowing.h"
#include "windowContextState.h"
#endif

#include "mapping.h"
#include "bit_range.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gits::OpenGL;
using gits::OpenGL::CGlDriver;
using gits::OpenGL::drv;

struct MappingContext {
  uint64_t key;
  uint64_t value;
  bool present;
  MappingContext() : key(0), value(0), present(false) {}
};

void handle_mapping_get(int kind, MappingContext& ctx);
void handle_mapping_add(int kind, MappingContext& ctx);
void handle_mapping_check(int kind, MappingContext& ctx);
void handle_mapping_remove(int kind, MappingContext& ctx);

struct Through64 {
  template <class T>
  Through64(T value) {
    value_ = (uintptr_t)value;
  }
  template <class T>
  operator T() const {
    return (T)(uintptr_t)value_;
  }

private:
  uint64_t value_;
};

template <int Id>
struct GetMappingOp {
  static void perform(MappingContext& ctx) {
    typedef typename gits::OpenGL::IdMappedArgMap<Id>::type CMappedArg;
    ctx.value = Through64(CMappedArg::GetMapping(Through64(ctx.key)));
  }
};

template <int Id>
struct AddMappingOp {
  static void perform(MappingContext& ctx) {
    typedef typename gits::OpenGL::IdMappedArgMap<Id>::type CMappedArg;
    CMappedArg::AddMapping(Through64(ctx.key), Through64(ctx.value));
  }
};

template <int Id>
struct CheckMappingOp {
  static void perform(MappingContext& ctx) {
    typedef typename gits::OpenGL::IdMappedArgMap<Id>::type CMappedArg;
    ctx.present = CMappedArg::CheckMapping(Through64(ctx.key));
  }
};

template <int Id>
struct RemoveMappingOp {
  static void perform(MappingContext& ctx) {
    typedef typename gits::OpenGL::IdMappedArgMap<Id>::type CMappedArg;
    CMappedArg::RemoveMapping(Through64(ctx.key));
  }
};

struct ObjName {
  ObjName(uint64_t a) {
    /*empty, should not be used*/
    assert(a == 0);
    kind_ = -1;
  }

  ObjName(uint64_t a, int kind) : a_(a), kind_(kind) {}

  uint64_t org() const {
    return a_;
  }
  int kind() const {
    return kind_;
  }

  template <class T>
  operator T() const {
    MappingContext ctx;
    ctx.key = a_;
    handle_mapping_get(kind_, ctx);
    return Through64(ctx.value);
  }

  template <class T>
  void operator=(T mapped) {
    MappingContext ctx;
    ctx.key = a_;
    ctx.value = Through64(mapped);
    handle_mapping_add(kind_, ctx);
  }

  bool has_mapping() const {
    MappingContext ctx;
    ctx.key = a_;
    handle_mapping_check(kind_, ctx);
    return ctx.present;
  }

  void remove_mapping() const {
    MappingContext ctx;
    ctx.key = a_;
    handle_mapping_remove(kind_, ctx);
  }

private:
  uint64_t a_;
  int kind_;
};

template <int Kind>
ObjName name(uint64_t n) {
  return ObjName(n, Kind);
}

inline GLint uloc(GLuint programRec, GLint locationRec) {
  gits::OpenGL::CGLUniformLocation loc(programRec, locationRec);
  return *loc;
}

inline GLint bidx(GLuint programRec, GLint indexRec) {
  gits::OpenGL::CGLUniformBlockIndex idx(programRec, indexRec);
  return *idx;
}

inline GLuint sidx(GLuint programRec, GLenum type, GLuint idx) {
  return gits::OpenGL::CGLSubroutineIndex::GetMapping(programRec, type, idx);
}

struct CRecUniformLocation {
  GLint loc_, arrs_, arri_;
  CRecUniformLocation(GLint location, GLint array_size, GLint array_index)
      : loc_(location), arrs_(array_size), arri_(array_index) {}
  // We need an operator= accepting a GLint, because sometimes return values of
  // API calls are of type CRecUniformLocation. Then, we generate CCode like:
  // CRecUniformLocation ret = CRecUniformLocation(5, 8, 0);
  // ret = glGetUniformLocation_wrap(prog, name, CRecUniformLocation(5, 8, 0));
  // TODO: It should be implemented properly, but I have no time to test it, so
  // I'm just leaving a comment for now.
  CRecUniformLocation& operator=(GLint) {
    return *this;
  }
};

GLint glGetUniformLocation_wrap(ObjName prog,
                                const char* name,
                                const CRecUniformLocation& location);
GLint glGetUniformLocationARB_wrap(ObjName prog,
                                   const char* name,
                                   const CRecUniformLocation& location);
GLint glGetUniformBlockIndex_wrap(ObjName prog, const char* name, GLint index);
GLuint glGetProgramResourceIndex_wrap(ObjName prog,
                                      GLenum programInterface,
                                      const char* name,
                                      GLuint index);
GLint glGetSubroutineIndex_wrap(ObjName prog, GLenum type, const char* name, GLuint index);

namespace api {
#define DECLARE_VARIABLE_GL_FUNCTION(a, b, c, e) extern a(STDCALL*& b) c;
GL_FUNCTIONS(DECLARE_VARIABLE_)
#define DECLARE_VARIABLE_GL_DRAW_FUNCTION(a, b, c, e) DECLARE_VARIABLE_GL_FUNCTION(a, b, c, e)
DRAW_FUNCTIONS(DECLARE_VARIABLE_)
#define DECLARE_VARIABLE_EGL_FUNCTION(a, b, c, e) extern a(STDCALL*& b) c;
EGL_FUNCTIONS(DECLARE_VARIABLE_)

#if defined GITS_PLATFORM_WINDOWS
#define DECLARE_VARIABLE_WGL_EXT_FUNCTION(a, b, c, d) extern a(STDCALL*& b) c;
WGL_EXT_FUNCTIONS(DECLARE_VARIABLE_)
#define DECLARE_VARIABLE_WGL_FUNCTION(a, b, c, d) extern a(STDCALL*& b) c;
WGL_FUNCTIONS(DECLARE_VARIABLE_)
#elif defined GITS_PLATFORM_X11
#define DECLARE_VARIABLE_GLX_FUNCTION(a, b, c, d) extern a(STDCALL*& b) c;
GLX_FUNCTIONS(DECLARE_VARIABLE_)
#endif
} // namespace api
namespace gits {}
using namespace gits;
using namespace api;

void FrameBufferSave(unsigned frameNumber);
void DrawBufferSave(unsigned drawNumber);

// ***************************** OpenGL API wrappers ***************************
// Helpers
template <typename T>
std::vector<T> ObjNameArrayToTypeArray(const ObjName* objNameArray, size_t count) {
  std::vector<T> typeArray(count);
  for (size_t i = 0; i < count; ++i) {
    typeArray[i] = (T)objNameArray[i];
  }
  return typeArray;
}

// Actual wrappers
void glBindSamplers_wrap(GLuint first, GLsizei count, const ObjName* samplerObjNames);
void glBindTextures_wrap(GLuint first, GLsizei count, const ObjName* textureObjNames);
void glFinish_wrap(void);
void glFlush_wrap(void);
void glGetProgramResourceiv_wrap(ObjName program,
                                 GLenum programInterface,
                                 GLuint index,
                                 GLsizei propCount,
                                 const GLenum* props,
                                 GLsizei count,
                                 GLsizei* length,
                                 GLint* params,
                                 GLchar* resource_name,
                                 std::vector<CRecUniformLocation> locations);
void glGetTexImage_wrap(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels);
void glReadPixels_wrap(
    GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data);

void glUseProgram_wrap(GLuint program);
void glUseProgramObjectARB_wrap(GLuint program);
void glViewport_wrap(GLint x, GLint y, GLsizei width, GLsizei height);

void* glMapTexture2DINTEL_wrap(GLuint texture,
                               GLuint level,
                               GLbitfield bietfield,
                               int* outStride,
                               int* outLayout,
                               void* returnValue);
void glUnmapTexture2DINTEL_wrap(GLuint texture, GLint level);

// *************************** WGL API wrappers ***********************************
int wglGetPixelFormat_wrap(ObjName hdc, ObjName hwnd, int ret_val);

BOOL wglSetPixelFormat_wrap(ObjName org_hdc,
                            ObjName org_hwnd,
                            int format,
                            int* attribs,
                            int* values,
                            size_t att_size,
                            int* winparams,
                            size_t wp_size,
                            bool ret_val);
template <size_t S0, size_t S1>
BOOL wglSetPixelFormat_wrap(ObjName org_hdc,
                            ObjName org_hwnd,
                            int format,
                            int (&attribs)[S0],
                            int (&values)[S0],
                            int (&winparams)[S1],
                            bool ret_val) {
  return wglSetPixelFormat_wrap(org_hdc, org_hwnd, format, attribs, values, S0, winparams, S1,
                                ret_val);
}
template <class T>
BOOL wglSetPixelFormat_wrap(ObjName org_hdc,
                            ObjName org_hwnd,
                            int format,
                            T attribs,
                            T values,
                            T winparams,
                            bool ret_val,
                            typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  int attribs_[] = {0};
  int values_[] = {0};
  int winparams_[] = {0};
  return wglSetPixelFormat_wrap(org_hdc, org_hwnd, format, attribs_, values_, 1, winparams_, 1,
                                ret_val);
}
template <class T, size_t S0>
BOOL wglSetPixelFormat_wrap(ObjName org_hdc,
                            ObjName org_hwnd,
                            int format,
                            T attribs,
                            T values,
                            int (&winparams)[S0],
                            bool ret_val,
                            typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  int attribs_[] = {0};
  int values_[] = {0};
  return wglSetPixelFormat_wrap(org_hdc, org_hwnd, format, attribs_, values_, 1, winparams, S0,
                                ret_val);
}
template <class T, size_t S0>
BOOL wglSetPixelFormat_wrap(ObjName org_hdc,
                            ObjName org_hwnd,
                            int format,
                            int (&attribs)[S0],
                            int (&values)[S0],
                            T winparams,
                            bool ret_val,
                            typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  int winparams_[] = {0};
  return wglSetPixelFormat_wrap(org_hdc, org_hwnd, format, attribs, values, S0, winparams_, 1,
                                ret_val);
}

inline int ObjArraySize(ObjName* a, int size) {
  if (a[0].kind() == -1) {
    return 0;
  }
  return size;
}

BOOL wglSwapBuffers_wrap(ObjName org_hdc,
                         ObjName org_hwnd,
                         int* winparams,
                         size_t wp_size,
                         ObjName* wnd_del,
                         size_t dw_size,
                         bool ret_val);
template <size_t S0, size_t S1>
BOOL wglSwapBuffers_wrap(
    ObjName org_hdc, ObjName org_hwnd, int (&winparams)[S0], ObjName (&wnd_del)[S1], bool ret_val) {
  return wglSwapBuffers_wrap(org_hdc, org_hwnd, winparams, S0, wnd_del, ObjArraySize(wnd_del, S1),
                             ret_val);
}
template <class T, class U>
BOOL wglSwapBuffers_wrap(
    ObjName org_hdc,
    ObjName org_hwnd,
    T winparams,
    U wnd_del,
    bool ret_val,
    typename std::enable_if<std::is_pointer<T>::value && std::is_pointer<U>::value>::type* = 0) {
  int winparams_[] = {0};
  ObjName wnd_del_[] = {0};
  return wglSwapBuffers_wrap(org_hdc, org_hwnd, winparams_, 1, wnd_del_, ObjArraySize(wnd_del_, 1),
                             ret_val);
}
template <class T, size_t S0>
BOOL wglSwapBuffers_wrap(ObjName org_hdc,
                         ObjName org_hwnd,
                         int (&winparams)[S0],
                         T wnd_del,
                         bool ret_val,
                         typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  ObjName wnd_del_[] = {0};
  return wglSwapBuffers_wrap(org_hdc, org_hwnd, winparams, S0, wnd_del_, ObjArraySize(wnd_del_, 1),
                             ret_val);
}
template <class T, size_t S0>
BOOL wglSwapBuffers_wrap(ObjName org_hdc,
                         ObjName org_hwnd,
                         T winparams,
                         ObjName (&wnd_del)[S0],
                         bool ret_val,
                         typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  int winparams_[] = {0};
  return wglSwapBuffers_wrap(org_hdc, org_hwnd, winparams_, 1, wnd_del, ObjArraySize(wnd_del, S0),
                             ret_val);
}

BOOL wglMakeCurrent_wrap(ObjName org_hdc,
                         ObjName ctx,
                         ObjName org_hwnd,
                         int* winparams,
                         size_t wp_size,
                         ObjName* hwnd_del_list,
                         size_t dw_size,
                         bool ret_val);

template <size_t S0, size_t S1>
BOOL wglMakeCurrent_wrap(ObjName org_hdc,
                         ObjName ctx,
                         ObjName org_hwnd,
                         int (&winparams)[S0],
                         ObjName (&hwnd_del_list)[S1],
                         bool ret_val) {
  return wglMakeCurrent_wrap(org_hdc, ctx, org_hwnd, winparams, S0, hwnd_del_list,
                             ObjArraySize(hwnd_del_list, S1), ret_val);
}
template <class T, class U>
BOOL wglMakeCurrent_wrap(
    ObjName org_hdc,
    ObjName ctx,
    ObjName org_hwnd,
    T winparams,
    U hwnd_del_list,
    bool ret_val,
    typename std::enable_if<std::is_pointer<T>::value && std::is_pointer<U>::value>::type* = 0) {
  int winparams_[] = {0};
  ObjName hwnd_del_list_[] = {0};
  return wglMakeCurrent_wrap(org_hdc, ctx, org_hwnd, winparams_, 1, hwnd_del_list_,
                             ObjArraySize(hwnd_del_list_, 1), ret_val);
}
template <class T, size_t S0>
BOOL wglMakeCurrent_wrap(ObjName org_hdc,
                         ObjName ctx,
                         ObjName org_hwnd,
                         int (&winparams)[S0],
                         T hwnd_del_list,
                         bool ret_val,
                         typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  ObjName hwnd_del_list_[] = {0};
  return wglMakeCurrent_wrap(org_hdc, ctx, org_hwnd, winparams, S0, hwnd_del_list_,
                             ObjArraySize(hwnd_del_list_, 1), ret_val);
}
template <class T, size_t S0>
BOOL wglMakeCurrent_wrap(ObjName org_hdc,
                         ObjName ctx,
                         ObjName org_hwnd,
                         T winparams,
                         ObjName (&hwnd_del_list)[S0],
                         bool ret_val,
                         typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  int winparams_[] = {0};
  return wglMakeCurrent_wrap(org_hdc, ctx, org_hwnd, winparams_, 1, hwnd_del_list,
                             ObjArraySize(hwnd_del_list, S0), ret_val);
}

HGLRC wglCreateContext_wrap(ObjName hdc, ObjName hwnd, int ret_val);
HGLRC wglCreateContextAttribsARB_wrap(
    ObjName hdc, ObjName shared_ctx, const int* attribs, ObjName hwnd, int ret_val);
HANDLE wglCreateBufferRegionARB_wrap(
    ObjName hdc, int iLayerPlane, UINT uType, ObjName hwnd, HANDLE ret_val);
BOOL wglShareLists_wrap(ObjName lhs, ObjName rhs, bool ret_val);
void wglGetExtensionsStringARB_wrap(ObjName hdc, ObjName hwnd);
BOOL wglUseFontBitmapsA_wrap(ObjName hdcOut,
                             DWORD first,
                             DWORD count,
                             DWORD listBase,
                             ObjName hwnd,
                             GLchar* strarr,
                             int* fontArgs,
                             bool ret_val);
BOOL wglUseFontBitmapsW_wrap(ObjName hdcOut,
                             DWORD first,
                             DWORD count,
                             DWORD listBase,
                             ObjName hwnd,
                             GLchar* strarr,
                             int* fontArgs,
                             bool ret_val);
BOOL wglUseFontOutlinesA_wrap(HDC hdcOut,
                              DWORD first,
                              DWORD count,
                              DWORD listBase,
                              FLOAT deviation,
                              FLOAT extrusion,
                              int format,
                              HWND hwnd,
                              GLchar* strarr,
                              int* fontArgs,
                              bool ret_val);
BOOL wglUseFontOutlinesW_wrap(HDC hdcOut,
                              DWORD first,
                              DWORD count,
                              DWORD listBase,
                              FLOAT deviation,
                              FLOAT extrusion,
                              int format,
                              HWND hwnd,
                              GLchar* strarr,
                              int* fontArgs,
                              bool ret_val);

HPBUFFERARB wglCreatePbufferARB_wrap(ObjName hdc,
                                     int pformat,
                                     int width,
                                     int height,
                                     int* piAttribsList,
                                     int* attribs,
                                     int* values,
                                     int attsize,
                                     HPBUFFERARB ret_val);
template <size_t S0>
HPBUFFERARB wglCreatePbufferARB_wrap(ObjName hdc,
                                     int pformat,
                                     int width,
                                     int height,
                                     int* piAttribsList,
                                     int (&attribs)[S0],
                                     int* values,
                                     HPBUFFERARB ret_val) {
  return wglCreatePbufferARB_wrap(hdc, pformat, width, height, piAttribsList, attribs, values, S0,
                                  ret_val);
}
template <class T>
HPBUFFERARB wglCreatePbufferARB_wrap(
    ObjName hdc,
    int pformat,
    int width,
    int height,
    int* piAttribsList,
    T attribs,
    int* values,
    HPBUFFERARB ret_val,
    typename std::enable_if<std::is_pointer<T>::value>::type* = 0) {
  int attribs_[] = {0};
  int values_[] = {0};
  return wglCreatePbufferARB_wrap(hdc, pformat, width, height, piAttribsList, attribs_, values_, 1,
                                  ret_val);
}

void wglChoosePixelFormat_wrap(ObjName hdc, ObjName hwnd, int* pdf);

//***************************************** EGL API wrappers ***************************

void eglSwapBuffers_wrap(ObjName org_dpy, ObjName org_surf);

EGLImageKHR eglCreateImageKHR_wrap(EGLDisplay dpy,
                                   EGLContext ctx,
                                   EGLenum target,
                                   EGLClientBuffer buffer,
                                   const EGLint* attrib_list,
                                   EGLint anb_width,
                                   EGLint anb_height,
                                   EGLint anb_format,
                                   EGLint anb_usage,
                                   EGLint anb_stride,
                                   const GLvoid* anb_data);
EGLBoolean eglMakeCurrent_wrap(
    EGLDisplay dpy, EGLSurface read, EGLSurface draw, EGLContext ctx, bool ret_val);
EGLContext eglCreateContext_wrap(EGLDisplay dpy,
                                 EGLConfig cfg,
                                 EGLContext shared,
                                 const EGLint* attribs);

//HWND to window Map
class WindowsMap {
public:
  typedef std::map<win_handle_t, Window_*> CWinMapPlay;

  static Window_* FindWindowPlayer(win_handle_t winhandle);
  static void AddWindowPlayer(win_handle_t winhandle, Window_* window) {
    getMap()[winhandle] = window;
  }
  static void DeleteWindowPlayer(win_handle_t winhandle) {
    getMap().erase(winhandle);
  }

private:
  static CWinMapPlay& getMap() {
    static CWinMapPlay _winMapPlay;
    return _winMapPlay;
  }
};

void UpdateWindows_(ObjName hwnd,
                    const int winparams[],
                    size_t w_size,
                    const ObjName hwnd_del_list[],
                    size_t h_size);
#ifdef _WIN32
//HDC to HWND map
class HwndHdcMap {
public:
  typedef std::map<void*, void*> CHwndToHdc;

  static void AddHDC(void* hwnd, void* hdc) {
    getMap()[hwnd] = hdc;
  }
  static void DelHWND(void* hwnd) {
    getMap().erase(hwnd);
  }
  static void* GetHDC(void* hwnd) {
    return getMap()[hwnd];
  }

private:
  static CHwndToHdc& getMap() {
    static CHwndToHdc _hwndToHdc;
    return _hwndToHdc;
  }
};

struct MappedLevel {
  MappedLevel()
      : texture(0),
        level(0),
        size(0),
        pointer(0),
        hash(0),
        isInitialized(false),
        isRemoved(false),
        access(0),
        stride(NULL),
        layout(NULL) {}
  unsigned int texture;
  unsigned int level;
  unsigned int size;
  void* pointer;
  bool isInitialized;
  bool isRemoved;
  int access;
  int stride;
  int layout;
  uint32_t hash;
};

//Map Texture Intel EXT reletad class
class CMappedTextures {
  typedef std::multimap<GLint, MappedLevel> map_t;
  map_t mapped_;

  CMappedTextures() {}

public:
  void MapTexture(
      GLint name, GLint level, void* mapping, int size, int access, int stride, int layout);
  void UnmapTexture(GLint name, GLint level);
  void UpdateTexture(GLint name, GLint level, unsigned int offset);

  static CMappedTextures& getInstance() {
    static CMappedTextures instance;
    return instance;
  }
};

HDC UpdateHDC(ObjName hdc, ObjName hwnd, bool zeroHwndToUse = false);

//Windows / Pixelformat
unsigned getTexImageSize(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type);

#endif

//This function returns the current OGl version.
int getOpenGLVersion();
EGLConfig find_config(EGLDisplay dpy, const std::vector<EGLint>& attribs);
std::vector<EGLint> filter_config_attribs(const std::vector<EGLint>& attribs);

// Template to provide the buffer of given input size (no. of chars).
// Template argument paramIndex represents index of out param inside API call to provide unique buffer
template <int paramIndex>
void* getSinkBuffer(int charCount) {
  static std::vector<char> buffer;
  buffer.resize(std::max<size_t>(buffer.size(), charCount));
  return &buffer[0];
}

template <size_t S>
EGLint find_attrib(const EGLint (&attribs)[S], EGLint pname) {
  for (size_t i = 0; i < S; i += 2) {
    if (attribs[i] == pname) {
      return attribs[i + 1];
    }
  }
  return -1;
}

// Template to declare out param whose size (no. of elements) is given as input.
// Template argument T represents data type of the param e.g. GLint and paramIndex represents index of out param inside API call
template <class T, int paramIndex>
T* ignoredParam(int elementCount) {
  return (T*)getSinkBuffer<paramIndex>(elementCount * sizeof(T));
}

// Template for out params of APIs whose buffer size calculation (no. of elements) requires glGetIntegerv to be called
// pname represnts the common enum which can be used by different APIs in same family e.g. glGetDoublev, glGetFloatv
template <unsigned pname>
int QueryElementCount() {
  static int elements = 0;

  if (elements == 0) {
    glGetIntegerv(pname, &elements);
  }

  // In glGetInteger family APIs e.g. glGetFloatv, glGetDoublev etc., for those pname values which do not require size to be
  // queied, the max size is 16.
  // The comparison is applicable only for glGetInteger family APIs but it will not cause any problem to other families.
  return std::max<unsigned>(elements, 16);
}

// Template to declare out param whose buffer size is required to be queried.
// Template argument T represents data type of the param e.g. GLint, paramIndex represents index of out param inside API call
// and pname represents enum to be used to query the buffer size (no. of elements)
template <class T, int paramIndex, GLenum pname>
T* ignoredQueryParam() {
  return (T*)getSinkBuffer<paramIndex>(QueryElementCount<pname>() * sizeof(T));
}
