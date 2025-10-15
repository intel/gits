// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openglDrivers.h"
#include "log.h"
#include "platform.h"
#include "openglEnums.h"
#include "exception.h"
#include "gits.h"
#ifndef BUILD_FOR_CCODE
#include "lua_bindings.h"
#include <tuple>
#endif

#include <sstream>
#include <algorithm>

namespace gits {
namespace OpenGL {

// Helper functions to use inside gl/native functions generated for CDriver.
namespace {

NOINLINE bool load_gl_function_generic(void*& func, const char* name) {
  const auto lib = drv.gl.Library();
  if (lib == nullptr) {
    return false;
  }

  func = dl::load_symbol(lib, name);
  if (func == nullptr) {
    func = load_egl_or_native(name);
  }

  return (bool)func;
}

template <class T>
bool load_gl_function(T& func, const char* name) {
  return load_gl_function_generic(reinterpret_cast<void*&>(func), name);
}

NOINLINE bool load_egl_function_generic(void*& func, const char* name) {
  func = dl::load_symbol(drv.egl.Library(), name);
  if (func == nullptr) {
    func = drv.egl.eglGetProcAddress(name);
  }
  return (bool)func;
}

template <class T>
bool load_egl_function(T& func, const char* name) {
  return load_egl_function_generic(reinterpret_cast<void*&>(func), name);
}

#ifdef GITS_PLATFORM_WINDOWS

// `already_loaded` is not local to wglGetProcAddress_proxy,
// as a workaround for existing streams that do not have
// wglGetProcAddress recorded and issue extension functions
// when no context is current. Special wglMakeCurrent proxy
// is used to load key functions needed to support those streams.
static std::map<std::string, void*> already_loaded;

BOOL STDCALL wglMakeCurrent_proxy(HDC hdc, HGLRC hglrc) {
  static auto realMakeCurrent =
      (decltype(&wglMakeCurrent_proxy))dl::load_symbol(drv.wgl.Library(), "wglMakeCurrent");

  static bool done;
  auto retVal = realMakeCurrent(hdc, hglrc);
  if (!done && retVal && hdc && hglrc) {
#define WGL_LOAD_EXTENSION(fname) already_loaded[fname] = drv.wgl.wglGetProcAddress(fname)
    WGL_LOAD_EXTENSION("wglCreateContextAttribsARB");
    WGL_LOAD_EXTENSION("wglChoosePixelFormatARB");
    WGL_LOAD_EXTENSION("wglGetPixelFormatAttribivARB");
    WGL_LOAD_EXTENSION("wglCreatePbufferARB");
    WGL_LOAD_EXTENSION("wglGetPbufferDCARB");
    WGL_LOAD_EXTENSION("wglReleasePbufferDCARB");
    WGL_LOAD_EXTENSION("wglDestroyPbufferARB");
    WGL_LOAD_EXTENSION("wglGetExtensionsStringARB");
#undef WGL_LOAD_EXTENSION
    done = true;
  }
  return retVal;
}

// We need to keep track of functions loaded beforehand in WGL.
// This is because application may load the pointer when it had
// a context current, but call function when the context is
// already destroyed (so not current). GITS defers actual function
// discovery to call site of application so we need to remember
// the pointers here.
void* STDCALL wglGetProcAddress_proxy(const char* name) {
  static auto getProc =
      (decltype(&wglGetProcAddress_proxy))dl::load_symbol(drv.wgl.Library(), "wglGetProcAddress");

  // This function is cached, return it.
  auto func = already_loaded[name];
  if (func) {
    return func;
  }

  // This function is not cached, need to resolve now.
  func = getProc(name);
  already_loaded[name] = func;
  return func;
}

template <class T>
NOINLINE bool load_wgl_function(T& func, const char* name) {
  if (strncmp(name, "wglGetProcAddress", 17) == 0) {
    func = (T)wglGetProcAddress_proxy;
  } else if (strncmp(name, "wglMakeCurrent", 14) == 0) {
    func = (T)wglMakeCurrent_proxy;
  } else {
    func = (T)dl::load_symbol(drv.wgl.Library(), name);
  }
  return func;
}

template <class T>
NOINLINE bool load_wglext_function(T& func, const char* name) {
  func = (T)drv.wgl.wglGetProcAddress(name);
  return func;
}
#endif

#ifdef GITS_PLATFORM_X11
template <class T>
NOINLINE bool load_glx_function(T& func, const char* name) {
  func = (T)dl::load_symbol(drv.glx.Library(), name);
  if (func == 0) {
    if (drv.glx.glXGetProcAddress == nullptr) {
      drv.glx.glXGetProcAddress =
          (void*(STDCALL*)(const GLubyte*))dl::load_symbol(drv.glx.Library(), "glXGetProcAddress");
    }
    func = (T)drv.glx.glXGetProcAddress((GLubyte*)(name));
  }
  return func;
}
#endif

NOINLINE void GLErrorLog() {
  if (!Configurator::Get().opengl.shared.traceGLError) {
    return;
  }

  GLint error = 0;

  static GLenum (*glGetErrorLocal)() = nullptr;
  if (glGetErrorLocal == nullptr) {
    load_gl_function(glGetErrorLocal, "glGetError");
  }

  if (glGetErrorLocal != nullptr) {
    error = glGetErrorLocal();
  } else {
    LOG_ERROR << "Logging failed - can't load glGetError";
  }

  if (error != 0) {
    LOG_TRACE << "GL Error: " << ToStr(static_cast<GLenum>(error));
  }
}

void noop() {}

#if !defined BUILD_FOR_CCODE
using namespace lua;

#define LUA_FUNCTION(RET, NAME, ARGS_DECL, DRV)                                                    \
  int lua_##NAME(lua_State* L) {                                                                   \
    const int top = lua_gettop(L);                                                                 \
    if (top != Argnum<RET ARGS_DECL>::value)                                                       \
      luaL_error(L, "invalid number of parameters");                                               \
                                                                                                   \
    FuncToTuple<RET ARGS_DECL>::type args;                                                         \
    fill_tuple(L, args);                                                                           \
    bypass_luascript = true;                                                                       \
    RET ret = call_tuple<RET>(gits::OpenGL::DRV.NAME, args);                                       \
    bypass_luascript = false;                                                                      \
    gits::lua::lua_push(L, ret);                                                                   \
                                                                                                   \
    return 1;                                                                                      \
  }

#else

#define LUA_FUNCTION(RET, NAME, ARGS_DECL, DRV)                                                    \
  int lua_##NAME(struct lua_State* L) {                                                            \
    return 1;                                                                                      \
  }

#endif

static bool bypass_luascript;

#define LUA_GL_FUNCTION(b, c, d, e)      LUA_FUNCTION(b, c, d, drv.gl)
#define LUA_GL_DRAW_FUNCTION(b, c, d, e) LUA_FUNCTION(b, c, d, drv.gl)
#define LUA_EGL_FUNCTION(b, c, d, e)     LUA_FUNCTION(b, c, d, drv.egl)
#define LUA_WGL_FUNCTION(b, c, d, e)     LUA_FUNCTION(b, c, d, drv.wgl)
#define LUA_WGL_EXT_FUNCTION             LUA_WGL_FUNCTION
#define LUA_GLX_FUNCTION(b, c, d, e)     LUA_FUNCTION(b, c, d, drv.glx)

} // namespace

#ifdef BUILD_FOR_CCODE
#define LUA_SCRIPTING_INSTRUMENTATION(b, c, d, e)
#define LUA_FUNCTION_EXISTS(function) false
#else
static NOINLINE lua_State* GetLuaState() {
  return CGits::Instance().GetLua().get();
}

#define LUA_SCRIPTING_INSTRUMENTATION(b, c, d, e)

#define LUA_FUNCTION_EXISTS(function) lua::FunctionExists(function, GetLuaState())

#endif

// logging_`function` is a function that is called from
// default_`function` when configured to do so. It assumes that
// drv.gl.`function` is properly initialized and so stores the value
// of it upon first call. Next it outputs its name and list of
// parameters to GITS Log(), installs itself as drv.gl.`function`
// and calls original function.
#ifndef BUILD_FOR_CCODE
#define LOGGING_FUNCTION(b, c, d, e, err_fun, drv_name)                                            \
  b STDCALL logging_##c d {                                                                        \
    err_fun();                                                                                     \
    const Config& gits_cfg = Configurator::Get();                                                  \
    const bool doTrace = ShouldLog(LogLevel::TRACE);                                               \
    if (doTrace && (!drv.traceGLAPIBypass || gits_cfg.opengl.player.traceGitsInternal)) {          \
      Tracer(#c).trace e;                                                                          \
    }                                                                                              \
    b gits_ret = (b)0;                                                                             \
    bool call_shd = true;                                                                          \
    if (gits_cfg.common.shared.useEvents && !bypass_luascript) {                                   \
      const auto L = GetLuaState();                                                                \
      const bool exists = !doTrace || lua::FunctionExists(#c, L);                                  \
      if (exists) {                                                                                \
        LUA_CALL_FUNCTION(L, #c, e, d)                                                             \
        call_shd = false;                                                                          \
        const int top_ = lua_gettop(L);                                                            \
        gits_ret = lua::lua_to<b>(L, top_);                                                        \
        lua_pop(L, top_);                                                                          \
      }                                                                                            \
    }                                                                                              \
    if (call_shd) {                                                                                \
      gits_ret = drv_name.shd_##c e;                                                               \
    }                                                                                              \
    if (doTrace && (!drv.traceGLAPIBypass || gits_cfg.opengl.player.traceGitsInternal)) {          \
      Tracer(#c).trace_ret(gits_ret);                                                              \
    }                                                                                              \
    return gits_ret;                                                                               \
  }
#else
#define LOGGING_FUNCTION(b, c, d, e, err_fun, drv_name)                                            \
  b STDCALL logging_##c d {                                                                        \
    return (b)0;                                                                                   \
  }
#endif

#define LOGGING_GL_FUNCTION(b, c, d, e)      LOGGING_FUNCTION(b, c, d, e, GLErrorLog, drv.gl)
#define LOGGING_GL_DRAW_FUNCTION(b, c, d, e) LOGGING_FUNCTION(b, c, d, e, GLErrorLog, drv.gl)
#define LOGGING_EGL_FUNCTION(b, c, d, e)     LOGGING_FUNCTION(b, c, d, e, noop, drv.egl)
#define LOGGING_WGL_FUNCTION(b, c, d, e)     LOGGING_FUNCTION(b, c, d, e, noop, drv.wgl)
#define LOGGING_WGL_EXT_FUNCTION             LOGGING_WGL_FUNCTION
#define LOGGING_GLX_FUNCTION(b, c, d, e)     LOGGING_FUNCTION(b, c, d, e, noop, drv.glx)

// If the proxy library loads itself instead of the original one, the function logs the message and exits.
void CheckForRecursion(dl::SharedObject library) {
  if (Configurator::IsRecorder() && Configurator::Get().opengl.recorder.detectRecursion &&
      (dl::load_symbol(library, "GITSIdentificationToken") !=
       nullptr)) { // This token is exposed only by proxy i.e. gits library
    LOG_ERROR << "Recursion detected while loading the library.";
    LOG_ERROR << "LibDirectory value not set correctly in gits.ini file. Please refer to the "
                 "\"OpenGL Options\" section in gits.ini for more details.";
    exit(EXIT_FAILURE);
  }
}

NOINLINE bool UseTracing(const char* func) {
  const auto& cfg = Configurator::Get();
  return ShouldLog(LogLevel::TRACE) || (cfg.common.shared.useEvents && LUA_FUNCTION_EXISTS(func)) ||
         (!cfg.common.player.traceSelectedFrames.empty());
}

NOINLINE void LogFunctionNotFoundShutdown(const char* func) {
  LOG_ERROR << "Function " << func << " couldn't be loaded";
  drv.trigger_terminate_event();
  exit(1);
}

// default_`function` is a function that, when called, checks whether
// there is a GL context. If no context is current, function returns
// 0. If the context is current it looks up `function` in appropriate
// library using dlsym or native GL api, and updates drv.gl structure
// with it. If drv.gl.`function` is 0, we exit(1). If its non-0, we
// call logging_`function` or drv.gl.`function`. As drv.gl.`function`
// got overwritten, we won't get here next time.
#define DEFAULT_FUNCTION(b, c, d, e, load_func, drv_name)                                          \
  b STDCALL default_##c d {                                                                        \
    if (!load_func(drv_name.c, #c)) {                                                              \
      LOG_ERROR << "Application called the function " << #c << ", but it "                         \
                << "couldn't be loaded. The driver call will be skipped.";                         \
      if (Configurator::Get().opengl.recorder.retryFunctionLoads) {                                \
        LOG_ERROR << "Function load will be reattempted the next time "                            \
                     "application calls this function.";                                           \
        drv_name.c = default_##c;                                                                  \
      } else {                                                                                     \
        LOG_ERROR << "If application calls this function again, "                                  \
                     "there will be a crash, caused by the nullptr call.";                         \
      }                                                                                            \
      return (b)0;                                                                                 \
    }                                                                                              \
                                                                                                   \
    drv_name.shd_##c = drv_name.c;                                                                 \
                                                                                                   \
    if (drv_name.c == 0)                                                                           \
      LogFunctionNotFoundShutdown(#c);                                                             \
                                                                                                   \
    if (UseTracing(#c))                                                                            \
      drv_name.c = logging_##c;                                                                    \
                                                                                                   \
    return drv_name.c e;                                                                           \
  }

#ifdef BUILD_FOR_CCODE

void DumpPreDrawCall() {
  static int PreDrawCallCnt = 0;
  if (Configurator::Get().opengl.player.captureDraws[++PreDrawCallCnt] &&
      Configurator::Get().opengl.player.captureDrawsPre) {
    capture_drawbuffer(Configurator::Get().ccode.outputPath,
                       "drawcall-" + std::to_string(PreDrawCallCnt) + "-pre", false);
  }
}

void DumpPostDrawCall() {
  static int PostDrawCallCnt = 0;
  if (Configurator::Get().opengl.player.captureDraws[++PostDrawCallCnt]) {
    capture_drawbuffer(Configurator::Get().ccode.outputPath,
                       "drawcall-" + std::to_string(PostDrawCallCnt) + "-post", false);
  }
}

#define DEFAULT_DRAW_FUNCTION(b, c, d, e, load_func, drv_name)                                     \
  b STDCALL draw_##c d {                                                                           \
    static b(STDCALL* drawFunc) d;                                                                 \
    bool isFunLoaded = false;                                                                      \
    if (!Configurator::Get().opengl.player.captureDraws.empty()) {                                 \
      isFunLoaded = load_func(drawFunc, #c);                                                       \
    } else {                                                                                       \
      isFunLoaded = load_func(drv_name.c, #c);                                                     \
    }                                                                                              \
    if (!isFunLoaded) {                                                                            \
      LOG_WARNING << "Function " #c " called before any context creation";                         \
      return (b)0;                                                                                 \
    } else {                                                                                       \
      if (drawFunc == 0 && drv_name.c == 0) {                                                      \
        LOG_ERROR << "Function " #c " couldn't be loaded";                                         \
        drv.trigger_terminate_event();                                                             \
        exit(1);                                                                                   \
      } else {                                                                                     \
        if (ShouldLog(LogLevel::TRACE) || Configurator::Get().common.shared.useEvents)             \
          return logging_##c e;                                                                    \
        if (!Configurator::Get().opengl.player.captureDraws.empty()) {                             \
          DumpPreDrawCall();                                                                       \
          drawFunc e;                                                                              \
          DumpPostDrawCall();                                                                      \
          return 0;                                                                                \
        } else {                                                                                   \
          return drv_name.c e;                                                                     \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  }
#endif

#define DEFAULT_GL_FUNCTION(b, c, d, e)  DEFAULT_FUNCTION(b, c, d, e, load_gl_function, drv.gl)
#define DEFAULT_EGL_FUNCTION(b, c, d, e) DEFAULT_FUNCTION(b, c, d, e, load_egl_function, drv.egl)
#define DEFAULT_WGL_FUNCTION(b, c, d, e) DEFAULT_FUNCTION(b, c, d, e, load_wgl_function, drv.wgl)
#define DEFAULT_WGL_EXT_FUNCTION(b, c, d, e)                                                       \
  DEFAULT_FUNCTION(b, c, d, e, load_wglext_function, drv.wgl)
#define DEFAULT_GLX_FUNCTION(b, c, d, e) DEFAULT_FUNCTION(b, c, d, e, load_glx_function, drv.glx)

#ifndef BUILD_FOR_CCODE
#define DEFAULT_GL_DRAW_FUNCTION(b, c, d, e) DEFAULT_FUNCTION(b, c, d, e, load_gl_function, drv.gl)
#else
#define DEFAULT_GL_DRAW_FUNCTION(b, c, d, e)                                                       \
  DEFAULT_DRAW_FUNCTION(b, c, d, e, load_gl_function, drv.gl)
#endif

namespace {
GL_FUNCTIONS(LOGGING_)
GL_FUNCTIONS(DEFAULT_)
GL_FUNCTIONS(LUA_)

DRAW_FUNCTIONS(LOGGING_)
DRAW_FUNCTIONS(DEFAULT_)
DRAW_FUNCTIONS(LUA_)

EGL_FUNCTIONS(LOGGING_)
EGL_FUNCTIONS(DEFAULT_)
EGL_FUNCTIONS(LUA_)

#ifdef GITS_PLATFORM_WINDOWS
WGL_FUNCTIONS(LOGGING_)
WGL_FUNCTIONS(DEFAULT_)
WGL_FUNCTIONS(LUA_)
WGL_EXT_FUNCTIONS(LOGGING_)
WGL_EXT_FUNCTIONS(DEFAULT_)
WGL_EXT_FUNCTIONS(LUA_)
#endif

#ifdef GITS_PLATFORM_X11
GLX_FUNCTIONS(LOGGING_)
GLX_FUNCTIONS(DEFAULT_)
GLX_FUNCTIONS(LUA_)
#endif
} // namespace

#define LOGGING_GL_FUNCTION(b, c, d, e)      LOGGING_FUNCTION(b, c, d, e, GLErrorLog, drv.gl)
#define LOGGING_GL_DRAW_FUNCTION(b, c, d, e) LOGGING_FUNCTION(b, c, d, e, GLErrorLog, drv.gl)
#define LOGGING_EGL_FUNCTION(b, c, d, e)     LOGGING_FUNCTION(b, c, d, e, noop, drv.egl)
#define LOGGING_WGL_FUNCTION(b, c, d, e)     LOGGING_FUNCTION(b, c, d, e, noop, drv.wgl)
#define LOGGING_WGL_EXT_FUNCTION             LOGGING_WGL_FUNCTION
#define LOGGING_GLX_FUNCTION(b, c, d, e)     LOGGING_FUNCTION(b, c, d, e, noop, drv.glx)

namespace {
#define LUA_EXPORT_GL_FUNCTION(b, c, d, e)      {#c, lua_##c},
#define LUA_EXPORT_GL_DRAW_FUNCTION(b, c, d, e) LUA_EXPORT_GL_FUNCTION(b, c, d, e)
#define LUA_EXPORT_EGL_FUNCTION(b, c, d, e)     LUA_EXPORT_GL_FUNCTION(b, c, d, e)
#define LUA_EXPORT_WGL_FUNCTION(b, c, d, e)     LUA_EXPORT_GL_FUNCTION(b, c, d, e)
#define LUA_EXPORT_WGL_EXT_FUNCTION(b, c, d, e) LUA_EXPORT_GL_FUNCTION(b, c, d, e)
#define LUA_EXPORT_GLX_FUNCTION(b, c, d, e)     LUA_EXPORT_GL_FUNCTION(b, c, d, e)

#ifndef BUILD_FOR_CCODE
const luaL_Reg exports[] = {GL_FUNCTIONS(LUA_EXPORT_) DRAW_FUNCTIONS(LUA_EXPORT_)
                                EGL_FUNCTIONS(LUA_EXPORT_)

#ifdef GITS_PLATFORM_WINDOWS
                                    WGL_FUNCTIONS(LUA_EXPORT_) WGL_EXT_FUNCTIONS(LUA_EXPORT_)
#endif

#ifdef GITS_PLATFORM_X11
                                        GLX_FUNCTIONS(LUA_EXPORT_)
#endif
                                            {nullptr, nullptr}};
#endif
} // namespace

void RegisterLuaDriverFunctions() {
#ifndef BUILD_FOR_CCODE
  const auto L = CGits::Instance().GetLua();
  luaL_newlib(L.get(), exports);
  lua_setglobal(L.get(), "drv");
#endif
}

#undef DEFAULT_GL_FUNCTION
#undef DEFAULT_EGL_FUNCTION
#undef DEFAULT_FUNCTION
#undef DEFAULT_WGL_FUNCTION
#undef DEFAULT_WGL_EXT_FUNCTION
#undef DEFAULT_GLX_FUNCTIONS

#undef LOGGING_GL_FUNCTION
#undef LOGGING_EGL_FUNCTION
#undef LOGGING_FUNCTION
#undef LOGGING_WGL_FUNCTION
#undef LOGGING_WGL_EXT_FUNCTION
#undef LOGGING_GLX_FUNCTION

CEglDriver::CEglDriver() : _initialized(false), _lib_egl(nullptr), _egl_used(false) {
#define INITIALIZE_EGL_FUNCTION(b, c, d, e)                                                        \
  c = default_##c;                                                                                 \
  shd_##c = nullptr;
  EGL_FUNCTIONS(INITIALIZE_)
#undef INITIALIZE_EGL_FUNCTION
}

CEglDriver::~CEglDriver() {
  dl::close_library(_lib_egl);
}

dl::SharedObject CEglDriver::Library() {
  if (_initialized) {
    return _lib_egl;
  }

  _initialized = true;

  //EGL is loaded into global namespace so its easier on
  //frame capturing tools to perform their job
  std::filesystem::path path = Configurator::Get().common.shared.libEGL;
  _lib_egl = dl::open_library(path.string().c_str());

  if (_lib_egl == nullptr) {
    LOG_ERROR << "Couldn't open EGL library: " << path.string();
    exit(1);
  }

  CheckForRecursion(_lib_egl);
  return _lib_egl;
}

#if defined GITS_PLATFORM_X11
//#define LOAD_PTR_GLX_FUNCTION(a, b, c) b = (a (STDCALL *) c) dl::load_symbol(lib_gl, #b); if (b == 0) b= (a (STDCALL *) c) drv.glx.glXGetProcAddress((GLubyte*)(#b));

CGlxDriver::CGlxDriver() : _initialized(false), _lib(nullptr) {
#define INITIALIZE_GLX_FUNCTION(b, c, d, e)                                                        \
  c = default_##c;                                                                                 \
  shd_##c = nullptr;
  GLX_FUNCTIONS(INITIALIZE_)
#undef INITIALIZE_GLX_FUNCTION
}

dl::SharedObject CGlxDriver::Library() {
  if (_initialized) {
    return _lib;
  }
  _initialized = true;
  const auto& libGL = Configurator::Get().common.shared.libGL;
  printf("library path: %s\n", libGL.string().c_str());

  _lib = dl::open_library(libGL.string().c_str());

  if (_lib == nullptr) {
    LOG_ERROR << "Couldn't open GLX library: " << libGL.string();
    exit(1);
  }

  CheckForRecursion(_lib);
  return _lib;
}

#undef LOAD_PTR_GLX_FUNCTION
#endif

void* load_egl_or_native(const char* name) {
  void* return_value;
  drv.traceGLAPIBypass = true;
  if (drv.egl.Used()) {
    return_value = drv.egl.eglGetProcAddress(name);
  } else {
#if defined GITS_PLATFORM_WINDOWS
    return_value = drv.wgl.wglGetProcAddress(name);
#elif defined GITS_PLATFORM_X11
    return_value = (void*)drv.glx.glXGetProcAddress((GLubyte*)name);
#else
#error "Unsupported OS"
#endif
  }
  drv.traceGLAPIBypass = false;
  return return_value;
}

bool check_gl_function_availability(const char* name) {
  void* ptr = nullptr;
  bool function_availability = load_gl_function_generic(ptr, name);
  return function_availability;
}

#if defined GITS_PLATFORM_WINDOWS

CWglDriver::CWglDriver() : _initialized(false), _lib(nullptr) {
#define INITIALIZE_WGL_FUNCTION(b, c, d, e)                                                        \
  c = default_##c;                                                                                 \
  shd_##c = nullptr;
#define INITIALIZE_WGL_EXT_FUNCTION(b, c, d, e)                                                    \
  c = default_##c;                                                                                 \
  shd_##c = nullptr;
  WGL_FUNCTIONS(INITIALIZE_) WGL_EXT_FUNCTIONS(INITIALIZE_)
#undef INITIALIZE_WGL_EXT_FUNCTION
#undef INITIALIZE_WGL_FUNCTION
}

CWglDriver::~CWglDriver() {
  dl::close_library(_lib);
}

dl::SharedObject CWglDriver::Library() {
  if (_initialized) {
    return _lib;
  }
  _initialized = true;

  const auto& libGL = Configurator::Get().common.shared.libGL;

  _lib = dl::open_library(libGL.string().c_str());

  if (_lib == nullptr) {
    LOG_ERROR << "Couldn't open WGL library: " << libGL.string();
    exit(1);
  }

  CheckForRecursion(_lib);
  return _lib;
}

#endif

CGlDriver::CGlDriver() : _api(API_NULL), _initialized(false), _lib(nullptr) {
#define INITIALIZE_GL_FUNCTION(b, c, d, e)                                                         \
  c = default_##c;                                                                                 \
  shd_##c = nullptr;
#define INITIALIZE_GL_DRAW_FUNCTION(b, c, d, e)                                                    \
  c = draw_##c;                                                                                    \
  shd_##c = nullptr;
  GL_FUNCTIONS(INITIALIZE_)
  DRAW_FUNCTIONS(INITIALIZE_)
#undef INITIALIZE_GL_FUNCTION
}

void CGlDriver::Initialize(TApiType api) {
  if (Configurator::Get().common.mode == GITSMode::MODE_RECORDER &&
      !Configurator::Get().opengl.recorder.multiApiProtectBypass && _initialized && api != _api) {
    LOG_ERROR << "Multiple OGL API types applications not supported";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (_initialized) {
    return;
  }

  // Use defaults if no library options are given
  std::filesystem::path path;
  switch (api) {
  case API_GL:
    path = Configurator::Get().common.shared.libGL;
    LOG_INFO << "Initializing OpenGL API";
    break;
  case API_GLES1:
    path = Configurator::Get().common.shared.libGLESv1;
    LOG_INFO << "Initializing OpenGLES 1.1 API";
    break;
  case API_GLES2:
    path = Configurator::Get().common.shared.libGLESv2;
    LOG_INFO << "Initializing OpenGLES 2.0 API";
    break;
  default:
    throw std::runtime_error("bad api specified for GL function loader");
  }

  _lib = dl::open_library(path.string().c_str());
  _api = api;
  _initialized = true;
}

CGlDriver::~CGlDriver() {
  dl::close_library(_lib);
}

bool CGlDriver::HasExtension(const std::string& extension) const {
  static bool isInitalized = false;
  static std::string extensions;

  if (!isInitalized) {
    const char* ext = (const char*)drv.gl.glGetString(GL_EXTENSIONS);
    if (ext != nullptr) {
      extensions.append(ext);
    } else {
      GLint n = 0, i = 0;
      drv.gl.glGetError();
      drv.gl.glGetIntegerv(GL_NUM_EXTENSIONS, &n);
      for (i = 0; i < n; i++) {
        extensions.append((const char*)drv.gl.glGetStringi(GL_EXTENSIONS, i));
      }
    }

    isInitalized = true;
  }

  return extensions.find(extension) != std::string::npos;
}

bool CGlDriver::CanReadWriteonlyMappings() const {
  static bool can_read = false;

  GLuint buffer = 0;
  GLint old = 0;
  drv.gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old);
  drv.gl.glGenBuffers(1, &buffer);
  drv.gl.glBindBuffer(GL_ARRAY_BUFFER, buffer);
  const char* data = "GITS";
  drv.gl.glBufferData(GL_ARRAY_BUFFER, 5, data, GL_STATIC_DRAW);

  auto func_map = drv.gl.glMapBuffer;
  auto func_unmap = drv.gl.glUnmapBuffer;
  if (drv.gl.HasExtension("GL_OES_mapbuffer")) {
    func_map = drv.gl.glMapBufferOES;
    func_unmap = drv.gl.glUnmapBufferOES;
  }

  const char* const ptr = (char*)func_map(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  can_read = (strcmp(data, ptr) == 0);
  LOG_WARNING << "Reading from writeonly buffer mappings " << (can_read ? "" : "im") << "possible";
  func_unmap(GL_ARRAY_BUFFER);

  drv.gl.glDeleteBuffers(1, &buffer);
  drv.gl.glBindBuffer(GL_ARRAY_BUFFER, old);
  return can_read;
}

CDrivers drv;

} // namespace OpenGL
} // namespace gits
