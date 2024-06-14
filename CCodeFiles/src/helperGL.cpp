// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#include "tools_windows.h"
#endif

#include "helperGL.h"
#include "pfattribs.h"

template <template <int> class T>
void handle_mapping(int kind, MappingContext& ctx) {
  using namespace gits::OpenGL;
#define HANDLE_CASE(id)                                                                            \
  case id:                                                                                         \
    T<id>::perform(ctx);                                                                           \
    break;

  switch (kind) {
    HANDLE_CASE(OGL_PROGRAM_SHADER_MAP)
    HANDLE_CASE(OGL_VERTEXARRAY_MAP)
    // HANDLE_CASE(OGL_VERTEXARRAY_EXT_MAP)
    HANDLE_CASE(OGL_FRAME_BUFFER_MAP)
    HANDLE_CASE(OGL_FRAME_BUFFER_EXT_MAP)
    HANDLE_CASE(OGL_RENDER_BUFFER_MAP)
    HANDLE_CASE(OGL_RENDER_BUFFER_EXT_MAP)
    HANDLE_CASE(OGL_QUERY_MAP)
    HANDLE_CASE(OGL_TEXTURE_MAP)
    HANDLE_CASE(OGL_BUFFER_MAP)
    // HANDLE_CASE(OGL_UNIFORM_LOCATION_MAP)
    // HANDLE_CASE(OGL_UNIFORM_SUBROUTINE_LOCATION_MAP)
    // HANDLE_CASE(OGL_SUBROUTINE_INDEX_MAP)
    // HANDLE_CASE(OGL_UNIFORM_INDEX_MAP)
    // HANDLE_CASE(OGL_UNIFORM_BLOCK_INDEX_MAP)
    HANDLE_CASE(OGL_SAMPLER_MAP)
    HANDLE_CASE(OGL_SYNC_MAP)
    HANDLE_CASE(OGL_PIPELINE_MAP)
    HANDLE_CASE(WGL_HDC_MAP)
    HANDLE_CASE(WGL_HGLRC_MAP)
    HANDLE_CASE(WGL_HANDLE_MAP)
    HANDLE_CASE(WGL_HWND_MAP)
    HANDLE_CASE(WGL_HPBUFFERARB_MAP)
    HANDLE_CASE(WGL_HPBUFFEREXT_MAP)
    HANDLE_CASE(EGL_CONTEXT_MAP)
    HANDLE_CASE(EGL_DISPLAY_MAP)
    HANDLE_CASE(EGL_SURFACE_MAP)
    HANDLE_CASE(EGL_CLIENTBUFFER_MAP)
    HANDLE_CASE(EGL_NATIVEDISPLAYTYPE_MAP)
    HANDLE_CASE(EGL_NATIVEPIXMAPTYPE_MAP)
    HANDLE_CASE(EGL_NATIVEWINDOWTYPE_MAP)
    HANDLE_CASE(EGL_IMAGE_KHR_MAP)
    HANDLE_CASE(EGL_SYNC_KHR_MAP)
    HANDLE_CASE(X_DISPLAY_MAP)
    HANDLE_CASE(X_VISUALINFO_MAP)
    // HANDLE_CASE(GLX_FBCONFIG_MAP)
    HANDLE_CASE(GLX_CONTEXT_MAP)
    HANDLE_CASE(GLX_DRAWABLE_MAP)
  default:
    break;
  }
#undef HANDLE_CASE
}

void handle_mapping_get(int kind, MappingContext& ctx) {
  handle_mapping<GetMappingOp>(kind, ctx);
}
void handle_mapping_add(int kind, MappingContext& ctx) {
  handle_mapping<AddMappingOp>(kind, ctx);
}
void handle_mapping_check(int kind, MappingContext& ctx) {
  handle_mapping<CheckMappingOp>(kind, ctx);
}
void handle_mapping_remove(int kind, MappingContext& ctx) {
  handle_mapping<RemoveMappingOp>(kind, ctx);
}

#define GENERATE_CAPTURE_FRAME_CODE(option)                                                        \
  if (!Config::Get().opengl.player.option.empty()) {                                               \
    static unsigned frame_num = 1;                                                                 \
    const auto& path = Config::Get().ccode.outputPath;                                             \
    const std::string fileName = getDumpFrameFileName(frame_num);                                  \
    gits::OpenGL::capture_drawbuffer(path, fileName, true);                                        \
    frame_num++;                                                                                   \
  }

namespace api {
#define DEFINE_VARIABLE_GL_FUNCTION(a, b, c, e) a(STDCALL*& b) c = gits::OpenGL::drv.gl.b;
GL_FUNCTIONS(DEFINE_VARIABLE_)
#define DEFINE_VARIABLE_GL_DRAW_FUNCTION(a, b, c, e) DEFINE_VARIABLE_GL_FUNCTION(a, b, c, e)
DRAW_FUNCTIONS(DEFINE_VARIABLE_)
#define DEFINE_VARIABLE_EGL_FUNCTION(a, b, c, e) a(STDCALL*& b) c = gits::OpenGL::drv.egl.b;
EGL_FUNCTIONS(DEFINE_VARIABLE_)

#if defined GITS_PLATFORM_WINDOWS
#define DEFINE_VARIABLE_WGL_EXT_FUNCTION(a, b, c, d) a(STDCALL*& b) c = gits::OpenGL::drv.wgl.b;
WGL_EXT_FUNCTIONS(DEFINE_VARIABLE_)
#define DEFINE_VARIABLE_WGL_FUNCTION(a, b, c, d) a(STDCALL*& b) c = gits::OpenGL::drv.wgl.b;
WGL_FUNCTIONS(DEFINE_VARIABLE_)
#elif defined GITS_PLATFORM_X11
#define DEFINE_VARIABLE_GLX_FUNCTION(a, b, c, d) a(STDCALL*& b) c = gits::OpenGL::drv.glx.b;
GLX_FUNCTIONS(DEFINE_VARIABLE_)
#endif
} // namespace api

namespace {
class CUniformLocation {
  GLuint _program;
  GLint _location;

public:
  CUniformLocation(GLuint program, GLint location) : _program(program), _location(location) {}
  bool operator<(const CUniformLocation& rhs) const {
    if (_program == rhs._program) {
      return _location < rhs._location;
    }
    return _program < rhs._program;
  }
  GLint Location() const {
    return _location;
  }
  GLuint Program() const {
    return _program;
  }
};

std::map<GLuint, GLuint> objectMap;
std::map<CUniformLocation, GLint> uniformMap;
} // namespace

//**************************** OpenGL API wrappers *********************************

void glBindSamplers_wrap(GLuint first, GLsizei count, const ObjName* samplerObjNames) {
  std::vector<GLuint> samplers = ObjNameArrayToTypeArray<GLuint>(samplerObjNames, count);
  glBindSamplers(first, count, samplers.data());
}

void glBindTextures_wrap(GLuint first, GLsizei count, const ObjName* textureObjNames) {
  std::vector<GLuint> textures = ObjNameArrayToTypeArray<GLuint>(textureObjNames, count);
  glBindTextures(first, count, textures.data());
}

void glViewport_wrap(GLint x, GLint y, GLsizei width, GLsizei height) {
  ::glViewport(x, y, width, height);
}

GLint glGetUniformLocation_wrap(ObjName prog,
                                const char* name,
                                const CRecUniformLocation& location) {
  auto loc = drv.gl.glGetUniformLocation(prog, name);
  gits::OpenGL::CGLUniformLocation::AddMapping((GLint)prog.org(), location.loc_, 1, loc);
  return loc;
}

GLint glGetUniformLocationARB_wrap(ObjName prog,
                                   const char* name,
                                   const CRecUniformLocation& location) {
  auto loc = drv.gl.glGetUniformLocationARB(prog, name);
  gits::OpenGL::CGLUniformLocation::AddMapping((GLint)prog.org(), location.loc_, 1, loc);
  return loc;
}

GLint glGetUniformBlockIndex_wrap(ObjName prog, const char* name, GLint index) {
  auto idx = drv.gl.glGetUniformBlockIndex(prog, name);
  gits::OpenGL::CGLUniformBlockIndex::AddMapping((GLint)prog.org(), index, idx);
  return idx;
}

GLuint glGetProgramResourceIndex_wrap(ObjName prog,
                                      GLenum programInterface,
                                      const char* name,
                                      GLuint index) {
  GLuint currentIndex = drv.gl.glGetProgramResourceIndex(prog, programInterface, name);
  if (programInterface == GL_SHADER_STORAGE_BLOCK) {
    gits::OpenGL::CGLStorageBlockIndex::AddMapping((GLint)prog.org(), index, currentIndex);
  }
  return currentIndex;
}

GLint glGetSubroutineIndex_wrap(ObjName prog, GLenum type, const char* name, GLuint index) {
  auto idx = drv.gl.glGetSubroutineIndex(prog, type, name);
  gits::OpenGL::CGLSubroutineIndex::AddMapping(prog, type, index, idx);
  return idx;
}

void glFinish_wrap(void) {
  glFinish();

  GENERATE_CAPTURE_FRAME_CODE(captureFinishFrame)
}

void glFlush_wrap(void) {
  glFlush();

  GENERATE_CAPTURE_FRAME_CODE(captureFlushFrame)
}

void glReadPixels_wrap(
    GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data) {
  GLint bufferBound = 0;
  ::glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &bufferBound);

  if (bufferBound != 0) {
    ::glReadPixels(x, y, width, height, format, type, reinterpret_cast<void*>(data));
  } else {
    // reserve space only if initialization is costly
    GLuint bufferSize = 0;
    static std::vector<unsigned char> dataSink;

    bufferSize = gits::OpenGL::getTexImageSize(width, height, 1, format, type);
    dataSink.resize(std::max<size_t>(dataSink.size(), bufferSize));

    ::glReadPixels(x, y, width, height, format, type, &dataSink[0]);
  }

  GENERATE_CAPTURE_FRAME_CODE(captureReadPixels)
}

void HandleUniformLocationMapping(const CRecUniformLocation& retVal,
                                  GLint actualLocation,
                                  ObjName program,
                                  const GLchar* name) {
  // Location -1 will be mapped the same way as other locations,
  // not present in mapping structures - using identity functions
  if (retVal.loc_ == -1) {
    return;
  }

  // We assume that there are no stream that during recording had optimized
  // all but the very first uniform array element - we assume that all uniform
  // arrays are at least 2 elements. Whole inactive arrays are also not handled well ...
  if (retVal.arrs_ == 1) {
    CGLUniformLocation::AddMapping(program.org(), retVal.loc_, 1, actualLocation);
  } else {
    // Player array index location may be -1 if we have a smaller array in
    // player than in recorder. In this case evaluation and mapping of runtime
    // array is impossible. We return there. Hopefully there should be at least
    // one location queried which is valid for player array size.
    if (actualLocation == -1) {
      return;
    }

    // For arrays we need to check if some part of it are inactive or is there more.
    // active parts than there were during recording. To do so, we can compare array
    // size against runtime array size. Inactive parts will be mapped to -1, while
    // active to correct value.
    GLint array_size, offset;
    GetUniformArraySizeAndOffset(program, name, actualLocation, array_size, offset);

    auto arrayOrigLocation = retVal.loc_ - retVal.arri_;
    auto arrayCurrLocation = actualLocation - retVal.arri_;

    // Add common part of arrays.
    CGLUniformLocation::AddMapping(program.org(), arrayOrigLocation,
                                   std::min(array_size, retVal.arrs_), arrayCurrLocation);

    if (retVal.arrs_ > array_size) {
      // We had bigger array at record time, map rest of the array to -1,
      // so that setting those elements doesn't interfere with other uniforms.
      auto rest = retVal.arrs_ - array_size;
      CGLUniformLocation::AddMapping(program.org(), arrayOrigLocation + array_size, rest, -1);
    }

    if (retVal.arrs_ < array_size) {
      // We have bigger array now (this means some uniforms didn't get optimized away, but are unused in the shader).
      // This can cause problems, if application was setting values of optimized away uniforms in original stream
      // by computing their locations.
      CALL_ONCE[] {
        Log(WARN) << "Uniform array size during recording was smaller than it is now - corruptions "
                     "may appear if application was setting inactive uniform array elements";
      };
    }
  }
}

void glGetProgramResourceiv_wrap(ObjName program,
                                 GLenum programInterface,
                                 GLuint index,
                                 GLsizei propCount,
                                 const GLenum* props,
                                 GLsizei count,
                                 GLsizei* length,
                                 GLint* params,
                                 GLchar* resource_name,
                                 std::vector<CRecUniformLocation> locations) {
  // TODO: The min(propCount, count) length calculations are wrong.
  // Same for the assumption that params[i] corresponds to props[i].
  // One property might result in multiple parameters being written to params.
  GLsizei totalPropsCount = std::min(propCount, count);
  glGetProgramResourceiv(program, programInterface, index, propCount, props, count, length, params);

  if (programInterface == GL_UNIFORM) {
    for (GLsizei i = 0; i < totalPropsCount; ++i) {
      if (props[i] == GL_LOCATION) {
        HandleUniformLocationMapping(locations[i], params[i], program, resource_name);
      }
    }
  }
}

void glGetTexImage_wrap(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels) {
  GLint bufferBound = 0;
  ::glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &bufferBound);

  if (bufferBound != 0) {
    ::glGetTexImage(target, level, format, type, pixels);
  } else {
    GLuint bufferSize = 0;
    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei depth = 0;

    ::glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    ::glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);
    ::glGetTexLevelParameteriv(target, level, GL_TEXTURE_DEPTH, &depth);

    bufferSize = gits::OpenGL::getTexImageSize(width, height, depth, format, type);

    std::vector<GLchar> pixels(bufferSize);
    ::glGetTexImage(target, level, format, type, &pixels[0]);
  }
}

#ifdef _WIN32
void* glMapTexture2DINTEL_wrap(GLuint texture,
                               GLuint level,
                               GLbitfield bietfield,
                               int* outStride,
                               int* outLayout,
                               void* returnValue) {
  GLint stride = 0;
  GLenum tiling = 0;
  GLint compressed = 0;
  GLint size = 0;

  void* pointer = glMapTexture2DINTEL(
      texture, level, /**_access*/ GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, &stride, &tiling);

  glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED, &compressed);
  if (compressed == GL_FALSE) {
    GLint height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &height);
    size = stride * height;
  } else {
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
  }
  CMappedTextures::getInstance().MapTexture(texture, level, pointer, size,
                                            GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, stride, tiling);

  return pointer;
}

void glUnmapTexture2DINTEL_wrap(GLuint texture, GLint level) {
  glUnmapTexture2DINTEL(texture, level);
  CMappedTextures::getInstance().UnmapTexture(texture, level);
}
#endif

// ************************** WGL API wrappers *************************************
Window_* WindowsMap::FindWindowPlayer(win_handle_t winhandle) {
  CWinMapPlay::iterator iter = getMap().find(winhandle);
  if (iter != getMap().end()) {
    return iter->second;
  } else {
    throw std::runtime_error("Unknown HWND");
  }
}

#ifdef _WIN32
HDC UpdateHDC(ObjName hdc, ObjName hwnd, bool zeroHwndToUse /*= false*/) {
  if (hdc.has_mapping()) {
    return hdc;
  } else {
    if (zeroHwndToUse) {
      hdc = GetDC(0);
      return hdc;
    } else {
      /*
      HDC hdcNew = GetDC(hwnd);
      HwndHdcMap::AddHDC((void*)hwnd, (void*)hdcNew);
      hdc = hdcNew;
      return hdcNew;
      */
      if (hwnd.has_mapping()) {
        HDC hdcOld = (HDC)HwndHdcMap::GetHDC(hwnd);
        HDC hdcNew = GetDC(hwnd);

        if (hdcOld != hdcNew) {
          HwndHdcMap::AddHDC(hwnd, hdcNew);
          hdc = hdcNew; // overwrite previous mapping
          ReleaseDC(hwnd, hdcOld);
        }
        return hdcNew;
      } else {
        hdc = GetDC(0);
        return hdc;
      }
    }
  }
}

#endif

// CREATE WINDOW
HWND CreateWindow_(int winparams[], size_t size) {
  if (size < 5) {
    throw std::runtime_error("Too few window parameters for window creation");
  }

  // Create a window.
  Window_* window =
      new Window_(winparams[2], winparams[3], winparams[0], winparams[1], winparams[4] != 0);
  window->set_title("gitsPlayer");
  // Add mapping.
  WindowsMap::AddWindowPlayer(window->handle(), window);

  return (HWND)window->handle();
}
#if defined GITS_PLATFORM_WINDOWS

// SELECT PIXEL FORMAT
int SelectPixelFormat(HDC dc, int attribs[], int values[], size_t size) {
#ifdef GITS_PLATFORM_WINDOWS
  int piFormat = 0;
  UINT numFormats;

  if (api::wglGetProcAddress("wglChoosePixelFormatARB") == 0) {
    // CHOOSE PIXEL FORMAT USING WIN GDI FUNCTION
    PIXELFORMATDESCRIPTOR pfd_current = {};
    pfd_current.nSize = sizeof(pfd_current);
    pfd_current.nVersion = 1;
    GetPFDFromAttribs(size, &attribs[0], &values[0], &pfd_current);
    piFormat = ChoosePixelFormat(dc, &pfd_current);
    Log(WARN) << "Pixel format being picked by ChoosePixelFormat not by wglChoosePixelFormatARB!";
  } else {
    // CHOOSE PIXEL FORMAT USING ARB EXTENSION FUNCTION
    std::vector<int> attribsvalues;
    // Merge attribs and values
    for (size_t i_attribs = 0, ivalues = 0; i_attribs < size; i_attribs++, ivalues++) {
      attribsvalues.push_back(attribs[ivalues]);
      attribsvalues.push_back(values[ivalues]);
    }
    attribsvalues.push_back(0);

    wglChoosePixelFormatARB(dc, &attribsvalues[0], NULL, 1, &piFormat, &numFormats);
  }

  return piFormat;
#else
  // this function should not be used in case when wgl is not used.
  return 0;
#endif
}
#endif
void UpdateWindows_(ObjName hwnd,
                    const int winparams[],
                    size_t w_size,
                    const ObjName hwnd_del_list[],
                    size_t h_size) {
  // Update current window parameters.
  if (w_size >= 5) {
    Window_* window = WindowsMap::FindWindowPlayer(hwnd);
    window->set_size(winparams[2], winparams[3]);
    window->set_position(winparams[0], winparams[1]);
    window->set_visibility(winparams[4] != 0);
  }
#ifdef _WIN32
  // Delete unneeded windows.
  if (h_size > 1) {
    for (size_t hwnd_i = 0; hwnd_i < h_size; hwnd_i++) {
      ReleaseDC(hwnd_del_list[hwnd_i], (HDC)HwndHdcMap::GetHDC(hwnd_del_list[hwnd_i]));
      HwndHdcMap::DelHWND(hwnd_del_list[hwnd_i]);
      WindowsMap::DeleteWindowPlayer(hwnd_del_list[hwnd_i]);
    }
  }
#endif
}
#ifdef _WIN32
BOOL wglSetPixelFormat_wrap(ObjName org_hdc,
                            ObjName org_hwnd,
                            int format,
                            int* attribs,
                            int* values,
                            size_t att_size,
                            int* winparams,
                            size_t wp_size,
                            bool ret_val) {
  drv.gl.Initialize(gits::OpenGL::CGlDriver::API_GL);
  HWND hwnd = CreateWindow_(winparams, wp_size);
  HDC hdc = GetDC(hwnd);
  int pformat = SelectPixelFormat(hdc, attribs, values, att_size);
  PIXELFORMATDESCRIPTOR pfd = {};
  auto ret = SetPixelFormat(hdc, pformat, &pfd);
  HwndHdcMap::AddHDC((void*)hwnd, (void*)hdc);
  org_hdc = hdc;
  org_hwnd = hwnd;
  return ret;
}

int wglGetPixelFormat_wrap(ObjName hdc, ObjName hwnd, int ret_val) {
  return wglGetPixelFormat(UpdateHDC(hdc, hwnd, true));
}

void wglGetExtensionsStringARB_wrap(ObjName hdc, ObjName hwnd) {
  wglGetExtensionsStringARB(UpdateHDC(hdc, hwnd, true));
}

BOOL wglSwapBuffers_wrap(ObjName org_hdc,
                         ObjName org_hwnd,
                         int* winparams,
                         size_t wp_size,
                         ObjName* wnd_del,
                         size_t dw_size,
                         bool ret_val) {
  UpdateWindows_(org_hwnd, winparams, wp_size, wnd_del, dw_size);
  OnFrameEnd();
  if (Config::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()]) {
    const std::string fileName = getDumpFrameFileName(CGits::Instance().CurrentFrame());
    gits::OpenGL::capture_drawbuffer(Config::Get().ccode.outputPath, fileName, true);
  }
  auto ret = SwapBuffers(UpdateHDC(org_hdc, org_hwnd));
  return ret;
}

BOOL wglMakeCurrent_wrap(ObjName org_hdc,
                         ObjName ctx,
                         ObjName org_hwnd,
                         int* winparams,
                         size_t wp_size,
                         ObjName* hwnd_del_list,
                         size_t dw_size,
                         bool ret_val) {
  UpdateWindows_(org_hwnd, winparams, wp_size, hwnd_del_list, dw_size);
  auto ret = api::wglMakeCurrent(UpdateHDC(org_hdc, org_hwnd), ctx);
  if (ret == 0) {
    Log(WARN) << "wglMakeCurrent failed, GetLastError() == " << Win32ErrorToString(GetLastError());
  } else {
    gits::OpenGL::SD().SetCurrentContext(ctx);
  }
  return ret;
}

BOOL wglUseFontBitmapsA_wrap(ObjName hdcOut,
                             DWORD first,
                             DWORD count,
                             DWORD listBase,
                             ObjName hwnd,
                             GLchar* strarr,
                             int* fontArgs,
                             bool ret_val) {
  HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
  HFONT hfont = CreateFontA(fontArgs[0], fontArgs[1], fontArgs[2], fontArgs[3], fontArgs[4],
                            fontArgs[5], fontArgs[6], fontArgs[7], fontArgs[8], fontArgs[9],
                            fontArgs[10], fontArgs[11], fontArgs[12], strarr);
  HGDIOBJ last = SelectObject(hdc, hfont);
  BOOL ret = drv.wgl.wglUseFontBitmapsA(GetDC(0), first, count, listBase);
  SelectObject(hdc, last);
  DeleteObject(hfont);
  return ret;
}

BOOL wglUseFontBitmapsW_wrap(ObjName hdcOut,
                             DWORD first,
                             DWORD count,
                             DWORD listBase,
                             ObjName hwnd,
                             GLchar* strarr,
                             int* fontArgs,
                             bool ret_val) {
  HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
  HFONT hfont = CreateFontA(fontArgs[0], fontArgs[1], fontArgs[2], fontArgs[3], fontArgs[4],
                            fontArgs[5], fontArgs[6], fontArgs[7], fontArgs[8], fontArgs[9],
                            fontArgs[10], fontArgs[11], fontArgs[12], strarr);
  HGDIOBJ last = SelectObject(hdc, hfont);
  BOOL ret = drv.wgl.wglUseFontBitmapsW(GetDC(0), first, count, listBase);
  SelectObject(hdc, last);
  DeleteObject(hfont);
  return ret;
}

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
                              bool ret_val) {
  HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
  HFONT hfont = CreateFontA(fontArgs[0], fontArgs[1], fontArgs[2], fontArgs[3], fontArgs[4],
                            fontArgs[5], fontArgs[6], fontArgs[7], fontArgs[8], fontArgs[9],
                            fontArgs[10], fontArgs[11], fontArgs[12], strarr);
  HGDIOBJ last = SelectObject(hdc, hfont);
  BOOL ret = drv.wgl.wglUseFontOutlinesA(GetDC(0), first, count, listBase, deviation, extrusion,
                                         format, 0);
  SelectObject(hdc, last);
  DeleteObject(hfont);
  return ret;
}

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
                              bool ret_val) {
  HDC hdc = (HDC)drv.wgl.wglGetCurrentDC();
  HFONT hfont = CreateFontW(fontArgs[0], fontArgs[1], fontArgs[2], fontArgs[3], fontArgs[4],
                            fontArgs[5], fontArgs[6], fontArgs[7], fontArgs[8], fontArgs[9],
                            fontArgs[10], fontArgs[11], fontArgs[12], (LPCWSTR)strarr);
  HGDIOBJ last = SelectObject(hdc, hfont);
  BOOL ret = drv.wgl.wglUseFontOutlinesW(GetDC(0), first, count, listBase, deviation, extrusion,
                                         format, 0);
  SelectObject(hdc, last);
  DeleteObject(hfont);
  return ret;
}

HPBUFFERARB wglCreatePbufferARB_wrap(ObjName hdc,
                                     int pformat,
                                     int width,
                                     int height,
                                     int* piAttribsList,
                                     int* attribs,
                                     int* values,
                                     int attsize,
                                     HPBUFFERARB ret_val) {
  if (!hdc.has_mapping()) {
    hdc = GetDC(0);
  }

  return wglCreatePbufferARB(hdc, SelectPixelFormat(hdc, attribs, values, attsize), width, height,
                             piAttribsList);
}

void wglChoosePixelFormat_wrap(ObjName hdc, ObjName hwnd, int* pdf_temp) {
  drv.gl.Initialize(gits::OpenGL::CGlDriver::API_GL);
  if (!hdc.has_mapping()) {
    hdc = GetDC(0);
  }

  wglChoosePixelFormat(UpdateHDC(hdc, hwnd), (const PIXELFORMATDESCRIPTOR*)pdf_temp);
}

HGLRC wglCreateContext_wrap(ObjName hdc, ObjName hwnd, int ret_val) {
  auto ctx = ::wglCreateContext(UpdateHDC(hdc, hwnd));
  gits::OpenGL::SD().AddContext(ctx, 0);
  return ctx;
}

HGLRC wglCreateContextAttribsARB_wrap(
    ObjName hdc, ObjName shared_ctx, const int* attribs, ObjName hwnd, int ret_val) {
  auto ctx = ::wglCreateContextAttribsARB(UpdateHDC(hdc, hwnd), shared_ctx, attribs);
  gits::OpenGL::SD().AddContext(ctx, shared_ctx);
  return ctx;
}

HANDLE wglCreateBufferRegionARB_wrap(
    ObjName hdc, int iLayerPlane, UINT uType, ObjName hwnd, HANDLE ret_val) {
  auto ret = wglCreateBufferRegionARB(UpdateHDC(hdc, hwnd), iLayerPlane, uType);
  return ret;
}

BOOL wglShareLists_wrap(ObjName lhs, ObjName rhs, bool ret_val) {
  auto ret = ::wglShareLists(lhs, rhs);
  if (ret == TRUE) {
    gits::OpenGL::SD().ShareContext(lhs, rhs);
  }
  return ret;
}

#endif
// ************************** EGL API wrappers ****************************************
void eglSwapBuffers_wrap(ObjName org_dpy, ObjName org_surf) {
  OnFrameEnd();
  if (Config::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()]) {
    const auto& path = Config::Get().ccode.outputPath;
    const std::string fileName = getDumpFrameFileName(CGits::Instance().CurrentFrame());
    gits::OpenGL::capture_drawbuffer(path, fileName, true);
  }
  eglSwapBuffers(org_dpy, (EGLSurface)org_surf);
}

EGLBoolean eglMakeCurrent_wrap(
    EGLDisplay dpy, EGLSurface read, EGLSurface draw, EGLContext ctx, bool ret_val) {
  auto ret = eglMakeCurrent(dpy, read, draw, ctx);
  if (ret == EGL_TRUE) {
    gits::OpenGL::SD().SetCurrentContext(ctx);
  }
  return ret;
}

EGLContext eglCreateContext_wrap(EGLDisplay dpy,
                                 EGLConfig cfg,
                                 EGLContext shared,
                                 const EGLint* attribs) {
  auto ret = eglCreateContext(dpy, cfg, shared, attribs);
  gits::OpenGL::SD().AddContext(ret, shared);
  return ret;
}

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
                                   const GLvoid* anb_data) {
  EGLImageKHR eglImage;
  GLuint bufferOrig = (GLuint)(uintptr_t)(buffer);
  GLuint bufferRun = 0;
  if (target == EGL_GL_TEXTURE_2D_KHR || target == EGL_GL_TEXTURE_3D_KHR ||
      (target >= EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR &&
       target <= EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR)) {
    if (!CGLTexture::CheckMapping(bufferOrig)) {
      // After implementing ES texture restoration this texture will need to be
      // restored in this token
      Log(WARN) << "Texture used to create eglImage not restored - eglCreateImageKHR will fail - "
                   "shouldn't influence rendered image.";
    } else {
      bufferRun = CGLTexture::GetMapping(bufferOrig);
    }
  } else if (target == EGL_GL_RENDERBUFFER_KHR) {
    if (!CGLRenderbuffer::CheckMapping(bufferOrig)) {
      // After implementing ES texture restoration this texture will need to be
      // restored in this token
      Log(WARN) << "Renderbuffer used to create eglImage not restored - eglCreateImageKHR will "
                   "fail - shouldn't influence rendered image.";
    } else {
      bufferRun = CGLRenderbuffer::GetMapping(bufferOrig);
    }
  } else {
    Log(ERR) << "Not supported eglCreateImageKHR target: " << target;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  eglImage = drv.egl.eglCreateImageKHR(dpy, ctx, target, (EGLClientBuffer)bufferRun, attrib_list);
  return eglImage;
}

unsigned getTexImageSize(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type) {
  unsigned texImageSize = 0;
  GLint lineLength = 0;
  GLint imageHeight = 0;
  GLint alignment = 0;
  GLint skipPixels = 0;
  GLint skipLines = 0;
  GLint skipImages = 0;
  GLint padding = 0;
  GLdouble groupSize = 0;
  GLint rowSize = 0;
  GLint imageSize = 0;

  // When height = 1, buffer is 2D. Hence depth should also be 1.
  if (width <= 0 || height <= 0 || depth <= 0 || (height == 1 && depth != 1)) {
    throw std::runtime_error("Dimension(s) is not set correctly");
  }

  // Get pixel pack params
  // Common param for all dimensions
  glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);

  // Get 1D params
  glGetIntegerv(GL_PACK_ROW_LENGTH, &lineLength);
  glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);

  // Get 2D params
  if (height > 1) {
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skipLines);
    glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &imageHeight);
  }

  // Get 3D param
  if (depth > 1) {
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &skipImages);
  }

  // Calculate image size
  GLint groupsPerLine = (lineLength > 0) ? lineLength : width;
  GLint rowsPerImage = (imageHeight > 0) ? imageHeight : height;

  if (type == GL_BITMAP) {
    groupSize = 1 / 8.0;
    // groupsPerLine % 8 bits will require another byte
    rowSize = static_cast<GLint>((groupsPerLine + 7) * groupSize);
  } else {
    groupSize = texelSize(format, type);
    rowSize = static_cast<GLint>(groupsPerLine * groupSize);
  }

  padding = rowSize & (alignment - 1);
  if (padding) {
    rowSize += alignment - padding;
  }
  imageSize = rowsPerImage * rowSize;

  texImageSize += imageSize * (skipImages + depth - 1);
  texImageSize += rowSize * (skipLines + height - 1);
  texImageSize += static_cast<GLsizei>(groupSize * (skipPixels + width));

  // add bitmap adjustment offset
  if (type == GL_BITMAP) {
    texImageSize +=
        static_cast<GLsizei>((skipPixels + width + 7) * groupSize) -
        (static_cast<GLsizei>(skipPixels * groupSize) + static_cast<GLsizei>(width * groupSize));
  }

  return texImageSize;
}

#if defined GITS_PLATFORM_WINDOWS
void CMappedTextures::MapTexture(
    GLint name, GLint level, void* mapping, int size, int access, int stride, int layout) {
  MappedLevel mapped_level;
  mapped_level.texture = name;
  mapped_level.level = level;
  mapped_level.pointer = mapping;
  mapped_level.size = size;
  mapped_level.hash = 0;
  mapped_level.access = access;
  mapped_level.stride = stride;
  mapped_level.layout = layout;

  mapped_.insert(std::make_pair(name, mapped_level));
}

void CMappedTextures::UnmapTexture(GLint name, GLint level) {
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  for (map_t::iterator it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level) {
      mapped_.erase(it);
      break;
    }
  }
}

void CMappedTextures::UpdateTexture(GLint name, GLint level, unsigned int offset) {
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  for (map_t::iterator it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level && it->second.pointer != NULL) {
      char* mapping = LoadFileDense("gitsTextures.dat", offset, it->second.size);
      memcpy(it->second.pointer, mapping, it->second.size);
      FreeMem(mapping);
    }
  }
}
#endif

int getOpenGLVersion() {
#ifdef CCODE_FOR_EGL
  return 0;
#endif
  int minor = 0, major = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver == 0) {
    throw std::runtime_error("glGetString(GL_VERSION) returned 0");
  }

  std::string version;
  std::stringstream str(ver);

  str >> version; // consume rest

  // drop the dot
  std::string::iterator iter = std::find(version.begin(), version.end(), '.');
  if (iter != version.end()) {
    *iter = ' ';
  }

  // get major / minor version
  str.clear();
  str.str(version);
  str >> major;
  str >> minor;

  return 100 * major + minor;
}

EGLConfig find_config(EGLDisplay dpy, const std::vector<EGLint>& attribs) {
  EGLConfig config = nullptr;
  EGLint configs = 0;

  // zero terminate attrib list
  std::vector<EGLint> attributes = attribs;
  // TODO: some filtering may be required to on the attribs
  attributes = filter_config_attribs(attributes);
  attributes.push_back(EGL_NONE);

  eglChooseConfig(dpy, &attributes[0], &config, 1, &configs);
  if (configs == 0) {
    throw std::runtime_error("Couldn't select config");
  }
  return config;
}

std::vector<EGLint> filter_config_attribs(const std::vector<EGLint>& attribs) {
  std::vector<EGLint> attr;
  for (size_t i = 0; i < attribs.size(); i += 2) {
    switch (attribs[i]) {
      // case EGL_BUFFER_SIZE:
    case EGL_RED_SIZE:
    case EGL_GREEN_SIZE:
    case EGL_BLUE_SIZE:
      // case EGL_LUMINANCE_SIZE:
    case EGL_ALPHA_SIZE:
    case EGL_DEPTH_SIZE:
    case EGL_SAMPLE_BUFFERS:
    case EGL_SAMPLES:
      // case EGL_STENCIL_SIZE:
      attr.push_back(attribs[i]);
      attr.push_back(attribs[i + 1]);
      break;
    case EGL_COLOR_BUFFER_TYPE:
      // attr.push_back(attribs[i]);
      // attr.push_back(attribs[i+1]);
      break;
    case EGL_SURFACE_TYPE:
    case EGL_RENDERABLE_TYPE:
    default:
      // other attributes are ignored
      break;
    }
  }
  return attr;
}
