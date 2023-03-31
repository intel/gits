// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   eglFunctions.cpp
*
*/

#include "eglFunctions.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "openglDrivers.h"
#include "gits.h"
#include "exception.h"
#include "library.h"
#include "log.h"
#include "pragmas.h"
#include "config.h"
#include "platform.h"
#include "ptblLibrary.h"
#include "ptbl_eglLibrary.h"
#include "opengl_apis_iface.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include <map>

#ifdef GITS_PLATFORM_X11
#include <X11/Xlib.h>
#endif
/* ***************************** EGL_GET_ERROR *************************** */

namespace {

//Flag to check whether initialize_egl() function has been executed or not.
//This function is called from eglGetDisplay API, so any egl calls made before eglGetDisplay will be skipped.
static bool isEGLInitialized = false;

//Macro to check whether initialize_egl() function is executed or not. This macro will be placed in Run method of all egl APIs.
//#define CHECK_FOR_EGL_INITIALIZATION if(!IsEGLInitialized()) return;

#define CHECK_FOR_EGL_INITIALIZATION(apiName)                                                      \
  if (!isEGLInitialized) {                                                                         \
    Log(WARN) << std::endl                                                                         \
              << "Skipped " << apiName << " execution as initialize_egl() is not yet called.";     \
    return;                                                                                        \
  }

void initialize_egl() {
  //add implicit mappings in EGL related CParameters
  //all EGL applications need to call eglGetDisplay, so this is natural place for
  //such configuration, move to some constructor of static storage duration if
  //this is troublesome
  if (!isEGLInitialized) {
    using namespace ::gits::OpenGL;
    drv.egl.Used(true);
    isEGLInitialized = true;
  }
}

void log_config_properties(EGLDisplay dpy, EGLConfig config) {
  if (gits::OpenGL::PtblNtvApi() != gits::OpenGL::PtblNtvStreamApi()) {
    return;
  }
  using namespace gits::OpenGL;
  EGLint id, buffer_size, depth_size, stencil_size, samples;
  drv.egl.eglGetConfigAttrib(dpy, config, EGL_CONFIG_ID, &id);
  drv.egl.eglGetConfigAttrib(dpy, config, EGL_BUFFER_SIZE, &buffer_size);
  drv.egl.eglGetConfigAttrib(dpy, config, EGL_DEPTH_SIZE, &depth_size);
  drv.egl.eglGetConfigAttrib(dpy, config, EGL_STENCIL_SIZE, &stencil_size);
  drv.egl.eglGetConfigAttrib(dpy, config, EGL_SAMPLES, &samples);
  Log(TRACE) << "Received config attributes: id=" << id << " color=" << buffer_size
             << " depth=" << depth_size << " stencil=" << stencil_size << " samples=" << samples;
}

void show_config_attribs(const std::vector<int>& attribs) {
  using namespace gits;
  using namespace gits::OpenGL;
  if (Config::Get().player.showOriginalPixelFormat) {
    Log(TRACE) << "Original config attributes:";
    for (size_t i = 0; i < attribs.size(); i += 2) {
      if (attribs[i] == EGL_NONE) {
        break;
      }
      GLLog(TRACE) << "    " << (GLenum)attribs[i] << " - " << attribs[i + 1];
    }
  }
}

} // namespace

gits::OpenGL::CeglGetError::CeglGetError() {}

gits::OpenGL::CeglGetError::CeglGetError(EGLint return_value) : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CeglGetError::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglGetError::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  initialize_egl();

  drv.egl.eglGetError();
}

void gits::OpenGL::CeglGetError::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream << Name() << "();\n";
}

/* ***************************** EGL_GET_DISPLAY *************************** */

gits::OpenGL::CeglGetDisplay::CeglGetDisplay() {}

gits::OpenGL::CeglGetDisplay::CeglGetDisplay(EGLDisplay return_value,
                                             EGLNativeDisplayType display_id)
    : _return_value(return_value), _display_id(display_id) {
  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
}

gits::CArgument& gits::OpenGL::CeglGetDisplay::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _display_id);
}

void gits::OpenGL::CeglGetDisplay::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() == PtblNativeAPI::Type::EGL) {
    initialize_egl();
  } else {
    isEGLInitialized = true;
  }

  EGLDisplay display = ptbl_GetEGLDisplay();
  _return_value.AddMapping(display);
}

void gits::OpenGL::CeglGetDisplay::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream.Indent() << "{" << std::endl;
  CALL_ONCE[&] {
    stream.Indent() << "drv.egl.Used(true);\n";
  };
  stream.Indent() << "EGLDisplay display = GetEGLDisplay();" << std::endl;
  stream.Indent() << _return_value << " = display;" << std::endl;
  stream.Indent() << "}" << std::endl;
}

/* ***************************** EGL_INITIALIZE *************************** */

gits::OpenGL::CeglInitialize::CeglInitialize() {}

gits::OpenGL::CeglInitialize::CeglInitialize(EGLBoolean return_value,
                                             EGLDisplay dpy,
                                             EGLint* major,
                                             EGLint* minor)
    : _return_value(return_value),
      _dpy(dpy),
      _major(major ? *major : -1),
      _minor(minor ? *minor : -1) {}

gits::CArgument& gits::OpenGL::CeglInitialize::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _major, _minor);
}

void gits::OpenGL::CeglInitialize::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLint minor = -1, major = -1;
  drv.egl.eglInitialize(*_dpy, &major, &minor);
}

void gits::OpenGL::CeglInitialize::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "EGLint minor = -1, major = -1;" << std::endl;
  stream.Indent() << "eglInitialize(" << _dpy << std::dec << ", &major, &minor);" << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

/* ***************************** EGL_TERMINATE *************************** */

gits::OpenGL::CeglTerminate::CeglTerminate() {}

gits::OpenGL::CeglTerminate::CeglTerminate(EGLBoolean return_value, EGLDisplay dpy)
    : _return_value(return_value), _dpy(dpy) {}

gits::CArgument& gits::OpenGL::CeglTerminate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy);
}

void gits::OpenGL::CeglTerminate::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglTerminate(*_dpy);
  SD().GetEglConfigs().clear();
}

/* ***************************** EGL_QUERY_STRING *************************** */

gits::OpenGL::CeglQueryString::CeglQueryString() : _return_value(1) {}

gits::OpenGL::CeglQueryString::CeglQueryString(const char* return_value,
                                               EGLDisplay dpy,
                                               EGLint name)
    : _return_value(return_value, '\0', 1), _dpy(dpy), _name(name) {}

gits::CArgument& gits::OpenGL::CeglQueryString::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _name);
}

void gits::OpenGL::CeglQueryString::Write(CCodeOStream& stream) const {
  stream.Indent() << "eglQueryString(" << _dpy << ", " << _name << ");\n";
}

void gits::OpenGL::CeglQueryString::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglQueryString(*_dpy, *_name);
}

/* ***************************** EGL_GET_CONFIGS *************************** */

gits::OpenGL::CeglGetConfigs::CeglGetConfigs() : _configs(1) {}

gits::OpenGL::CeglGetConfigs::CeglGetConfigs(EGLBoolean return_value,
                                             EGLDisplay dpy,
                                             EGLConfig* configs,
                                             EGLint config_size,
                                             EGLint* num_config)
    : _return_value(return_value),
      _dpy(dpy),
      _configs(config_size, configs),
      _config_size(config_size),
      _num_config(num_config ? *num_config : -1) {}

gits::CArgument& gits::OpenGL::CeglGetConfigs::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _configs, _config_size, _num_config);
}

void gits::OpenGL::CeglGetConfigs::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLint num_config;
  std::vector<EGLConfig> configs((std::max)(*_config_size, 1));
  drv.egl.eglGetConfigs(*_dpy, &configs[0], *_config_size, &num_config);
}

void gits::OpenGL::CeglGetConfigs::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream << "//eglGetConfigs - not implemented\n";
}

/* ***************************** EGL_CHOOSE_CONFIG *************************** */

gits::OpenGL::CeglChooseConfig::CeglChooseConfig() {}

gits::OpenGL::CeglChooseConfig::CeglChooseConfig(EGLBoolean return_value,
                                                 EGLDisplay dpy,
                                                 const EGLint* attrib_list,
                                                 EGLConfig* configs,
                                                 EGLint config_size,
                                                 EGLint* num_config)
    : _return_value(return_value),
      _dpy(dpy),
      _attrib_list(attrib_list, EGL_NONE, 2),
      _configs(config_size, configs),
      _config_size(config_size),
      _num_config(num_config ? *num_config : -1) {}

gits::CArgument& gits::OpenGL::CeglChooseConfig::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _attrib_list, _configs, _config_size, _num_config);
}

void gits::OpenGL::CeglChooseConfig::Write(CCodeOStream& stream) const {
  CScopeGenerator generateScopedBrackets(stream);
  stream.Indent() << "EGLint num_config;" << std::endl;

  _attrib_list.VariableNameRegister(stream, false);
  _attrib_list.Declare(stream);

  stream.Indent() << "std::vector<EGLConfig> configs(" << _config_size << ");" << std::endl;
  if (*_config_size == 0) {
    stream.Indent() << "eglChooseConfig(" << _dpy << ", " << _attrib_list << ", 0, " << _config_size
                    << ", &num_config);" << std::endl;
  } else {
    stream.Indent() << "eglChooseConfig(" << _dpy << ", " << _attrib_list << ", &configs[0], "
                    << _config_size << ", &num_config);" << std::endl;
  }
}

void gits::OpenGL::CeglChooseConfig::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLint num_config;
  std::vector<EGLConfig> configs(*_config_size);
  drv.egl.eglChooseConfig(*_dpy, *_attrib_list, (*_config_size == 0) ? nullptr : &configs[0],
                          *_config_size, &num_config);
}

/* ***************************** EGL_GET_CONFIG_ATTRIB *************************** */

gits::OpenGL::CeglGetConfigAttrib::CeglGetConfigAttrib() {}

gits::OpenGL::CeglGetConfigAttrib::CeglGetConfigAttrib(
    EGLBoolean return_value, EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value)
    : _return_value(return_value), _dpy(dpy), _config(config), _attribute(attribute) {}

gits::CArgument& gits::OpenGL::CeglGetConfigAttrib::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _attribute, _value);
}

void gits::OpenGL::CeglGetConfigAttrib::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.Indent() << "    EGLint value;" << std::endl;
  stream.Indent() << "    eglGetConfigAttrib(" << _dpy << ", 0, " << _attribute << ", &value);"
                  << std::endl;
  stream.Indent() << "}" << std::endl;
}

void gits::OpenGL::CeglGetConfigAttrib::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglGetConfigAttrib(*_dpy, *_config, *_attribute, *_value);
}

namespace gits {
namespace OpenGL {
namespace {
std::vector<EGLint> get_config_attribs(EGLDisplay dpy, EGLConfig cfg) {
  const EGLint attribs[] = {EGL_BUFFER_SIZE,
                            EGL_RED_SIZE,
                            EGL_GREEN_SIZE,
                            EGL_BLUE_SIZE,
                            EGL_LUMINANCE_SIZE,
                            EGL_ALPHA_SIZE,
                            EGL_ALPHA_MASK_SIZE,
                            EGL_BIND_TO_TEXTURE_RGB,
                            EGL_BIND_TO_TEXTURE_RGBA,
                            EGL_COLOR_BUFFER_TYPE,
                            EGL_CONFIG_CAVEAT,
                            EGL_CONFIG_ID,
                            EGL_CONFORMANT,
                            EGL_DEPTH_SIZE,
                            EGL_LEVEL,
                            EGL_MAX_PBUFFER_WIDTH,
                            EGL_MAX_PBUFFER_HEIGHT,
                            EGL_MAX_PBUFFER_PIXELS,
                            EGL_MAX_SWAP_INTERVAL,
                            EGL_MIN_SWAP_INTERVAL,
                            EGL_NATIVE_RENDERABLE,
                            EGL_NATIVE_VISUAL_ID,
                            EGL_NATIVE_VISUAL_TYPE,
                            EGL_RENDERABLE_TYPE,
                            EGL_SAMPLE_BUFFERS,
                            EGL_SAMPLES,
                            EGL_STENCIL_SIZE,
                            EGL_SURFACE_TYPE,
                            EGL_TRANSPARENT_TYPE,
                            EGL_TRANSPARENT_RED_VALUE,
                            EGL_TRANSPARENT_GREEN_VALUE,
                            EGL_TRANSPARENT_BLUE_VALUE};
  std::vector<EGLint> attributes;
  for (const auto& attrib : attribs) {
    EGLint value = 0;
    EGLBoolean status = drv.egl.eglGetConfigAttrib(dpy, cfg, attrib, &value);
    if (status == EGL_TRUE) {
      attributes.push_back(attrib);
      attributes.push_back(value);
    }
  }
  return attributes;
}

std::vector<EGLint> map_to_vector(const std::map<EGLint, EGLint>& m) {
  std::vector<EGLint> v;
  for (auto& elem : m) {
    v.push_back(elem.first);
    v.push_back(elem.second);
  }
  return v;
}

std::map<EGLint, EGLint> vector_to_map(const std::vector<EGLint>& v) {
  std::map<EGLint, EGLint> m;
  for (size_t i = 0; i < v.size(); i += 2) {
    m[v[i]] = v[i + 1];
  }
  return m;
}

std::vector<EGLint> filter_config_attribs(const std::vector<EGLint>& attribs) {
  std::vector<EGLint> attr;
  for (size_t i = 0; i < attribs.size(); i += 2) {
    switch (attribs[i]) {
    case EGL_RED_SIZE:
    case EGL_GREEN_SIZE:
    case EGL_BLUE_SIZE:
    case EGL_ALPHA_SIZE:
    case EGL_DEPTH_SIZE:
    case EGL_STENCIL_SIZE:
      attr.push_back(attribs[i]);
      attr.push_back(Config::Get().player.minimalConfig ? 1 : attribs[i + 1]);
      break;
    case EGL_SAMPLE_BUFFERS:
    case EGL_SAMPLES:
      if (!Config::Get().player.forceNoMSAA) {
        attr.push_back(attribs[i]);
        attr.push_back(Config::Get().player.minimalConfig ? 1 : attribs[i + 1]);
      }
      break;
    case EGL_COLOR_BUFFER_TYPE:
      //        attr.push_back(attribs[i]);
      //        attr.push_back(attribs[i+1]);
      //        break;
    case EGL_SURFACE_TYPE:
    default:
      //other attributes are not interesting for GITS
      break;
    }
  }
  return attr;
}

EGLConfig choose_config(EGLDisplay dpy, const std::vector<EGLint>& attribs) {
  auto& all_configs = SD().GetEglConfigs();
  if (all_configs.empty()) {
    EGLBoolean status;
    EGLint num_config = 0;
    status = drv.egl.eglGetConfigs(dpy, nullptr, 0, &num_config);
    if (status == EGL_FALSE) {
      throw std::runtime_error("Error when querying number of supported egl configs");
    }

    std::vector<EGLConfig> configs(num_config);
    status = drv.egl.eglGetConfigs(dpy, &configs[0], (EGLint)configs.size(), &num_config);
    if (status == EGL_FALSE) {
      throw std::runtime_error("Error when querying supported egl configs");
    }

    for (auto config : configs) {
      all_configs[config] = vector_to_map(get_config_attribs(dpy, config));
    }
  }

  auto needed = vector_to_map(filter_config_attribs(attribs));
  std::vector<EGLConfig> matches;
  for (auto& candidate : all_configs) {
    bool config_matches = true;
    for (auto& attr : needed) {
      if (attr.first == EGL_RENDERABLE_TYPE) {
        config_matches = ((candidate.second[EGL_RENDERABLE_TYPE] & attr.second) == attr.second);
        continue;
      }

      if (candidate.second[attr.first] < attr.second) {
        config_matches = false;
        break;
      }
    }
    if (config_matches) {
      matches.push_back(candidate.first);
    }
  }

  if (matches.empty()) {
    throw std::runtime_error("No matching configs found on the device");
  }

  // This makes sure that ties are resolved in the same way everywhere.
  std::sort(matches.begin(), matches.end(), [&](EGLConfig lhs, EGLConfig rhs) {
    return all_configs[lhs][EGL_CONFIG_ID] < all_configs[rhs][EGL_CONFIG_ID];
  });

  for (auto& attr : needed) {
    if (attr.first == EGL_RENDERABLE_TYPE) {
      continue;
    }

    std::stable_sort(matches.begin(), matches.end(), [&](EGLConfig lhs, EGLConfig rhs) {
      return all_configs[lhs][attr.first] < all_configs[rhs][attr.first];
    });
  }

  return matches[0];
}

std::vector<EGLint> get_surface_attribs(EGLDisplay dpy, EGLSurface surface) {
  const EGLint attribs[] = {
      EGL_VG_ALPHA_FORMAT, EGL_VG_COLORSPACE,  EGL_CONFIG_ID,      EGL_HEIGHT,
      EGL_LARGEST_PBUFFER, EGL_MIPMAP_TEXTURE, EGL_MIPMAP_LEVEL,   EGL_MULTISAMPLE_RESOLVE,
      EGL_RENDER_BUFFER,   EGL_SWAP_BEHAVIOR,  EGL_TEXTURE_FORMAT, EGL_TEXTURE_TARGET,
      EGL_WIDTH,
      /*
    EGL_PIXEL_ASPECT_RATIO,
    EGL_HORIZONTAL_RESOLUTION,
    EGL_VERTICAL_RESOLUTION,
*/
  };
  std::vector<EGLint> attributes;
  for (const auto& attrib : attribs) {
    EGLint value = 0;
    EGLBoolean status = drv.egl.eglQuerySurface(dpy, surface, attrib, &value);
    if (status == EGL_TRUE) {
      attributes.push_back(attrib);
      attributes.push_back(value);
    }
  }
  return attributes;
}

EGLint find_attrib(EGLint attrib, const std::vector<EGLint>& attrib_list) {
  for (size_t i = 0; i < attrib_list.size(); i += 2) {
    if (attrib_list[i] == attrib) {
      return attrib_list.at(i + 1);
    }
  }
  //TODO: serialize attrib, and the whole list and pass to exception
  throw std::runtime_error("Couldn't find specified attribute in attrib list");
}

} // namespace
} // namespace OpenGL
} // namespace gits

EGLConfig gits::OpenGL::FindConfigEGL(EGLDisplay dpy, const std::vector<EGLint>& attribs) {
  show_config_attribs(attribs);
  return choose_config(dpy, attribs);
}

/* ***************************** EGL_CREATE_WINDOW_SURFACE *************************** */

gits::OpenGL::CeglCreateWindowSurface::CeglCreateWindowSurface() {}

gits::OpenGL::CeglCreateWindowSurface::CeglCreateWindowSurface(EGLSurface return_value,
                                                               EGLDisplay dpy,
                                                               EGLConfig config,
                                                               EGLNativeWindowType win,
                                                               const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _win(win),
      _attrib_list(attrib_list, EGL_NONE, 2),
      _config_attribs(get_config_attribs(dpy, config)),
      _surface_attribs(get_surface_attribs(dpy, return_value)) {}

gits::CArgument& gits::OpenGL::CeglCreateWindowSurface::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _win, _attrib_list, _config_attribs,
                       _surface_attribs);
}

void gits::OpenGL::CeglCreateWindowSurface::Run() {
  CHECK_FOR_EGL_INITIALIZATION(Name())

  TODO("Add similar surface resizing code to eglSwapBuffers")

  EGLint width, height;

  try {
    float scale = Config::Get().player.scaleFactor;
    width = static_cast<EGLint>(scale * find_attrib(EGL_WIDTH, _surface_attribs.Vector()));
    height = static_cast<EGLint>(scale * find_attrib(EGL_HEIGHT, _surface_attribs.Vector()));
  } catch (...) {
    Log(WARN) << "Couldn't find surface attribs! Using default values for width and height. This "
                 "may result in undefined behavior.";
    width = 800;
    height = 600;
  }

  if (Config::Get().player.forceWindowSize) {
    width = Config::Get().player.windowSize.first;
    height = Config::Get().player.windowSize.second;
  }

  const auto& attr = _config_attribs.Vector();
  EGLConfig config = ptblFindConfigEGL(*_dpy, attr);

  std::pair<int, int> pos = std::pair<int, int>(0, 0);

  EGLSurface surface = nullptr;
  if (!Config::Get().player.renderOffscreen) {
#ifdef GITS_PLATFORM_WINDOWS
    Window_* wnd = new Window_(width, height, pos.first, pos.second, true);
#elif defined GITS_PLATFORM_X11
    Window_* wnd = new Window_(config, width, height, pos.first, pos.second, true);
#endif

    CStateDynamicNative::Get().MapAddWindowPlayer(wnd->handle(), wnd);
    // TODO: Call MapDeleteWindowPlayer on eglDestroySurface. We'll likely need a map from EGLSurface to Window_.
    log_config_properties(*_dpy, config);
    surface =
        ptbl_eglCreateWindowSurface(*_dpy, config, (EGLNativeWindowType)wnd->handle(), nullptr);
  } else {
    log_config_properties(*_dpy, config);
    EGLint attrib_list[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    surface = ptbl_eglCreatePbufferSurface(*_dpy, config, attrib_list);
  }
  _return_value.AddMapping(surface);
}

void gits::OpenGL::CeglCreateWindowSurface::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  EGLint width = find_attrib(EGL_WIDTH, _surface_attribs.Vector());
  EGLint height = find_attrib(EGL_HEIGHT, _surface_attribs.Vector());

  CScopeGenerator generateScopedBrackets(stream);
  stream.Indent() << "EGLDisplay display = " << _dpy << ";" << std::endl;
  stream.Indent() << "int winparams[] = {0, 0," << width << ", " << height << ", 1};" << std::endl;

  _config_attribs.VariableNameRegister(stream, false);
  _config_attribs.Declare(stream);

  stream.Indent() << std::endl;
  stream.Indent() << "std::vector<int> config_attribs (" << _config_attribs << ", "
                  << _config_attribs << " + sizeof(" << _config_attribs << ") / sizeof(EGLint) );"
                  << std::endl;
  stream.Indent() << "EGLConfig config = find_config(display, config_attribs);" << std::endl;

#if defined GITS_PLATFORM_WINDOWS
  stream.Indent() << "Window_* wnd = new Window_(winparams[2], winparams[3], 0, 0, true);"
                  << std::endl;
#elif defined GITS_PLATFORM_X11
  stream.Indent() << "Window_* wnd = new Window_(config, winparams[2], winparams[3], 0, 0, true);"
                  << std::endl;
#else
  stream.Indent() << "throw ENotImplemented(EXCEPTION_MESSAGE);" << std::endl;
#endif

  stream.Indent() << "WindowsMap::AddWindowPlayer(wnd->handle(), wnd);" << std::endl;
  stream.Indent() << "EGLSurface surface = eglCreateWindowSurface(display, config, "
                     "(EGLNativeWindowType)wnd->handle(), 0);"
                  << std::endl;
  stream.Indent() << _return_value << " = surface;" << std::endl;
}

/* ***************************** EGL_CREATE_PBUFFER_SURFACE *************************** */

gits::OpenGL::CeglCreatePbufferSurface::CeglCreatePbufferSurface() {}

gits::OpenGL::CeglCreatePbufferSurface::CeglCreatePbufferSurface(EGLSurface return_value,
                                                                 EGLDisplay dpy,
                                                                 EGLConfig config,
                                                                 const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _attrib_list(attrib_list, EGL_NONE, 2),
      _config_attribs(get_config_attribs(dpy, config)) {}

gits::CArgument& gits::OpenGL::CeglCreatePbufferSurface::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _attrib_list, _config_attribs);
}

void gits::OpenGL::CeglCreatePbufferSurface::Run() {
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLConfig config = ptblFindConfigEGL(*_dpy, _config_attribs.Vector());

  log_config_properties(*_dpy, config);

  EGLSurface surface = ptbl_eglCreatePbufferSurface(*_dpy, config, *_attrib_list);
  if (surface == nullptr) {
    Log(WARN) << "PBuffer creation failed.";
  }
  _return_value.AddMapping(surface);
}

void gits::OpenGL::CeglCreatePbufferSurface::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  CScopeGenerator generateScopedBrackets(stream);

  stream << "EGLDisplay display = " << _dpy << ";\n";

  _config_attribs.VariableNameRegister(stream, false);
  _config_attribs.Declare(stream);

  stream << "std::vector<int> config_attribs (" << _config_attribs << ", " << _config_attribs
         << " + sizeof(" << _config_attribs << ") / sizeof(EGLint) );\n";
  stream << "EGLConfig config = find_config(display, config_attribs);\n";

  _attrib_list.VariableNameRegister(stream, false);
  _attrib_list.Declare(stream);

  stream << _return_value << " = " << Name() << "(display, config, " << _attrib_list << ");\n";
}

/* ***************************** EGL_CREATE_PIXMAP_SURFACE *************************** */

gits::OpenGL::CeglCreatePixmapSurface::CeglCreatePixmapSurface() {}

gits::OpenGL::CeglCreatePixmapSurface::CeglCreatePixmapSurface(EGLSurface return_value,
                                                               EGLDisplay dpy,
                                                               EGLConfig config,
                                                               EGLNativePixmapType pixmap,
                                                               const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _pixmap(pixmap),
      _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreatePixmapSurface::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _pixmap, _attrib_list);
}

void gits::OpenGL::CeglCreatePixmapSurface::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  //TODO: add support for pixmaps
  throw ENotImplemented(EXCEPTION_MESSAGE);
  //drv.egl.eglCreatePixmapSurface(*_dpy, *_config, *_pixmap, *_attrib_list);
}

/* ***************************** EGL_DESTROY_SURFACE *************************** */

gits::OpenGL::CeglDestroySurface::CeglDestroySurface() {}

gits::OpenGL::CeglDestroySurface::CeglDestroySurface(EGLBoolean return_value,
                                                     EGLDisplay dpy,
                                                     EGLSurface surface)
    : _return_value(return_value), _dpy(dpy), _surface(surface) {
  CStateDynamicNative::Get().UnMapEglCtxToDispSurfBySurface((void*)surface);
}

gits::CArgument& gits::OpenGL::CeglDestroySurface::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface);
}

void gits::OpenGL::CeglDestroySurface::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  //egl surfaces are 1 to 1 with native objects (windows/pixmaps)
  //this is implied by egl using SetPixelFormat on egl window creation
  drv.egl.eglDestroySurface(*_dpy, *_surface);
  CStateDynamicNative::Get().UnMapEglCtxToDispSurfBySurface((void*)*_surface);
}

/* ***************************** EGL_QUERY_SURFACE *************************** */

gits::OpenGL::CeglQuerySurface::CeglQuerySurface() {}

gits::OpenGL::CeglQuerySurface::CeglQuerySurface(
    EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value)
    : _return_value(return_value),
      _dpy(dpy),
      _surface(surface),
      _attribute(attribute),
      _value(value ? *value : -1) {}

gits::CArgument& gits::OpenGL::CeglQuerySurface::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _attribute, _value);
}

void gits::OpenGL::CeglQuerySurface::Write(CCodeOStream& stream) const {
  stream << "{" << std::endl;
  stream << "EGLint value;" << std::endl;
  stream << "eglQuerySurface(" << _dpy << std::dec << ", " << _surface << " , " << _attribute
         << ", &value);" << std::endl;
  stream << "}" << std::endl;
}

void gits::OpenGL::CeglQuerySurface::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLint value;
  drv.egl.eglQuerySurface(*_dpy, *_surface, *_attribute, &value);
}

/* ***************************** EGL_BIND_API *************************** */

gits::OpenGL::CeglBindAPI::CeglBindAPI() {}

gits::OpenGL::CeglBindAPI::CeglBindAPI(EGLBoolean return_value, EGLenum api)
    : _return_value(return_value), _api(api) {}

gits::CArgument& gits::OpenGL::CeglBindAPI::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _api);
}

void gits::OpenGL::CeglBindAPI::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglBindAPI(*_api);
}

void gits::OpenGL::CeglBindAPI::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream.Indent() << "eglBindAPI(" << std::hex << (uintptr_t)*_api << ");" << std::endl;
}

/* ***************************** EGL_QUERY_API *************************** */

gits::OpenGL::CeglQueryAPI::CeglQueryAPI() {}

gits::OpenGL::CeglQueryAPI::CeglQueryAPI(EGLenum return_value) : _return_value(return_value) {
  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
}

gits::CArgument& gits::OpenGL::CeglQueryAPI::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglQueryAPI::Run() {
  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() == PtblNativeAPI::Type::EGL) {
    initialize_egl();
  } else {
    isEGLInitialized = true;
  }

  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglQueryAPI();
}

/* ***************************** EGL_WAIT_CLIENT *************************** */

gits::OpenGL::CeglWaitClient::CeglWaitClient() {}

gits::OpenGL::CeglWaitClient::CeglWaitClient(EGLBoolean return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CeglWaitClient::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglWaitClient::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglWaitClient();
}

/* ***************************** EGL_RELEASE_THREAD *************************** */

gits::OpenGL::CeglReleaseThread::CeglReleaseThread() {}

gits::OpenGL::CeglReleaseThread::CeglReleaseThread(EGLBoolean return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CeglReleaseThread::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglReleaseThread::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglReleaseThread();
}

/* ***************************** EGL_CREATE_PBUFFER_FROM_CLIENT_BUFFER *************************** */

gits::OpenGL::CeglCreatePbufferFromClientBuffer::CeglCreatePbufferFromClientBuffer() {}

gits::OpenGL::CeglCreatePbufferFromClientBuffer::CeglCreatePbufferFromClientBuffer(
    EGLSurface return_value,
    EGLDisplay dpy,
    EGLenum buftype,
    EGLClientBuffer buffer,
    EGLConfig config,
    const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _buftype(buftype),
      _buffer(buffer),
      _config(config),
      _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreatePbufferFromClientBuffer::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _buftype, _buffer, _config, _attrib_list);
}

void gits::OpenGL::CeglCreatePbufferFromClientBuffer::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  //pbuffers from client buffers only supported in OpenVG as of EGL1.4
  throw ENotImplemented(EXCEPTION_MESSAGE);
  //drv.egl.eglCreatePbufferFromClientBuffer(*_dpy, *_buftype, *_buffer, *_config, *_attrib_list);
}

/* ***************************** EGL_SURFACE_ATTRIB *************************** */

gits::OpenGL::CeglSurfaceAttrib::CeglSurfaceAttrib() {}

gits::OpenGL::CeglSurfaceAttrib::CeglSurfaceAttrib(
    EGLBoolean return_value, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
    : _return_value(return_value),
      _dpy(dpy),
      _surface(surface),
      _attribute(attribute),
      _value(value) {}

gits::CArgument& gits::OpenGL::CeglSurfaceAttrib::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _attribute, _value);
}

void gits::OpenGL::CeglSurfaceAttrib::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglSurfaceAttrib(*_dpy, *_surface, *_attribute, *_value);
}

/* ***************************** EGL_BIND_TEX_IMAGE *************************** */

gits::OpenGL::CeglBindTexImage::CeglBindTexImage() {}

gits::OpenGL::CeglBindTexImage::CeglBindTexImage(EGLBoolean return_value,
                                                 EGLDisplay dpy,
                                                 EGLSurface surface,
                                                 EGLint buffer)
    : _return_value(return_value), _dpy(dpy), _surface(surface), _buffer(buffer) {}

gits::CArgument& gits::OpenGL::CeglBindTexImage::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _buffer);
}

void gits::OpenGL::CeglBindTexImage::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglBindTexImage(*_dpy, *_surface, *_buffer);
}

/* ***************************** EGL_RELEASE_TEX_IMAGE *************************** */

gits::OpenGL::CeglReleaseTexImage::CeglReleaseTexImage() {}

gits::OpenGL::CeglReleaseTexImage::CeglReleaseTexImage(EGLBoolean return_value,
                                                       EGLDisplay dpy,
                                                       EGLSurface surface,
                                                       EGLint buffer)
    : _return_value(return_value), _dpy(dpy), _surface(surface), _buffer(buffer) {}

gits::CArgument& gits::OpenGL::CeglReleaseTexImage::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _buffer);
}

void gits::OpenGL::CeglReleaseTexImage::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglReleaseTexImage(*_dpy, *_surface, *_buffer);
}

/* ***************************** EGL_SWAP_INTERVAL *************************** */

gits::OpenGL::CeglSwapInterval::CeglSwapInterval() {}

gits::OpenGL::CeglSwapInterval::CeglSwapInterval(EGLBoolean return_value,
                                                 EGLDisplay dpy,
                                                 EGLint interval)
    : _return_value(return_value), _dpy(dpy), _interval(interval) {}

gits::CArgument& gits::OpenGL::CeglSwapInterval::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _interval);
}

void gits::OpenGL::CeglSwapInterval::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglSwapInterval(*_dpy, *_interval);
}

/* ***************************** EGL_CREATE_CONTEXT *************************** */

gits::OpenGL::CeglCreateContext::CeglCreateContext() {}

gits::OpenGL::CeglCreateContext::CeglCreateContext(EGLContext return_value,
                                                   EGLDisplay dpy,
                                                   EGLConfig config,
                                                   EGLContext share_context,
                                                   const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _share_context(share_context),
      _attrib_list(attrib_list, EGL_NONE, 2),
      _config_attribs(get_config_attribs(dpy, config)) {
  SD().AddContext(return_value, share_context);
}

gits::CArgument& gits::OpenGL::CeglCreateContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _share_context, _attrib_list,
                       _config_attribs);
}

void gits::OpenGL::CeglCreateContext::Run() {
  CHECK_FOR_EGL_INITIALIZATION(Name())
  //eglcontext created assume, we need GLES
  switch (ptbl_eglQueryAPI()) {
  case EGL_OPENGL_API:
    //wgl/glXCreateContext will initialize GL_API too
    ptblInitialize(CGlDriver::API_GL);
    break;
  case EGL_OPENGL_ES_API: {
    //as of EGL1.3 only version can be passed in context attribs for ES
    //look in attrib list for api version to initialize
    if (_attrib_list.Vector().size() >= 2 &&
        _attrib_list.Vector()[0] == EGL_CONTEXT_CLIENT_VERSION && _attrib_list.Vector()[1] >= 2) {
      ptblInitialize(CGlDriver::API_GLES2);
    } else {
      ptblInitialize(CGlDriver::API_GLES1);
    }
  } break;
  case EGL_OPENVG_API:
    throw ENotImplemented((std::string)EXCEPTION_MESSAGE + " OpenVG is not supported");
    break;
  default:
    break;
  }

  EGLConfig config = nullptr;
  if (*_config != nullptr) {
    const auto& attr = _config_attribs.Vector();
    config = ptblFindConfigEGL(*_dpy, attr);
  }

  EGLint* ctx_attribs_ptr = *_attrib_list;
  EGLContext ctx = ptbl_eglCreateContext(*_dpy, config, *_share_context, ctx_attribs_ptr);
  _return_value.AddMapping(ctx);
  SD().AddContext(ctx, *_share_context);
}

void gits::OpenGL::CeglCreateContext::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  CScopeGenerator generateScopedBrackets(stream);

  _attrib_list.VariableNameRegister(stream, false);
  _attrib_list.Declare(stream);

  _config_attribs.VariableNameRegister(stream, false);
  _config_attribs.Declare(stream);

  stream.Indent() << std::endl;
  stream.Indent() << "std::vector<int> config_attribs (" << _config_attribs << ", "
                  << _config_attribs << " + " << _config_attribs.Vector().size() << ");"
                  << std::endl;
  stream.Indent() << "EGLConfig config = find_config(" << _dpy << ", config_attribs);" << std::endl;
  stream.Indent() << _return_value << " =  eglCreateContext_wrap(" << _dpy << ", config, "
                  << _share_context << ", " << _attrib_list << ");" << std::endl;
  CALL_ONCE[&] {
    stream.Indent() << "drv.gl.Initialize(find_attrib(" << _attrib_list
                    << ", 12440) >= 2 ? CGlDriver::API_GLES2 : CGlDriver::API_GLES1);\n";
  };
}

/* ***************************** EGL_DESTROY_CONTEXT *************************** */

gits::OpenGL::CeglDestroyContext::CeglDestroyContext() {}

gits::OpenGL::CeglDestroyContext::CeglDestroyContext(EGLBoolean return_value,
                                                     EGLDisplay dpy,
                                                     EGLContext ctx)
    : _return_value(return_value), _dpy(dpy), _ctx(ctx) {
  CStateDynamicNative::Get().UnMapEglCtxToDispSurfByCtx((void*)ctx);
  SD().RemoveContext((void*)ctx);
}

gits::CArgument& gits::OpenGL::CeglDestroyContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _ctx);
}

void gits::OpenGL::CeglDestroyContext::Run() {
  CHECK_FOR_EGL_INITIALIZATION(Name())
  ptbl_eglDestroyContext(*_dpy, *_ctx);
  SD().RemoveContext((void*)*_ctx);
  CStateDynamicNative::Get().UnMapEglCtxToDispSurfByCtx((void*)*_ctx);
}

/* ***************************** EGL_MAKE_CURRENT *************************** */

gits::OpenGL::CeglMakeCurrent::CeglMakeCurrent() {}

gits::OpenGL::CeglMakeCurrent::CeglMakeCurrent(
    EGLBoolean return_value, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
    : _return_value(return_value), _dpy(dpy), _draw(draw), _read(read), _ctx(ctx) {
  CStateDynamicNative::Get().MapEglCtxToSurf((void*)ctx, (void*)draw, (void*)read);
  CStateDynamicNative::Get().MapCtxToDisplay((void*)ctx, (void*)dpy);
  SD().SetCurrentContext((void*)ctx);
}

gits::CArgument& gits::OpenGL::CeglMakeCurrent::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _draw, _read, _ctx);
}

void gits::OpenGL::CeglMakeCurrent::Run() {
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLBoolean ret = ptbl_eglMakeCurrent(*_dpy, *_draw, *_read, *_ctx);

  if (ret != 0) {
    SD().SetCurrentContext((void*)*_ctx);
    CStateDynamicNative::Get().MapEglCtxToSurf((void*)*_ctx, (void*)*_draw, (void*)*_read);
    CStateDynamicNative::Get().MapCtxToDisplay((void*)*_ctx, (void*)*_dpy);
  }

  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
}

/* ***************************** EGL_GET_CURRENT_CONTEXT *************************** */

gits::OpenGL::CeglGetCurrentContext::CeglGetCurrentContext() {}

gits::OpenGL::CeglGetCurrentContext::CeglGetCurrentContext(EGLContext return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CeglGetCurrentContext::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglGetCurrentContext::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglGetCurrentContext();
}

/* ***************************** EGL_GET_CURRENT_SURFACE *************************** */

gits::OpenGL::CeglGetCurrentSurface::CeglGetCurrentSurface() {}

gits::OpenGL::CeglGetCurrentSurface::CeglGetCurrentSurface(EGLSurface return_value, EGLint readdraw)
    : _return_value(return_value), _readdraw(readdraw) {}

gits::CArgument& gits::OpenGL::CeglGetCurrentSurface::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _readdraw);
}

void gits::OpenGL::CeglGetCurrentSurface::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglGetCurrentSurface(*_readdraw);
}

void gits::OpenGL::CeglGetCurrentSurface::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream << Name() << "(" << _readdraw << ");" << std::endl;
}

/* ***************************** EGL_GET_CURRENT_DISPLAY *************************** */

gits::OpenGL::CeglGetCurrentDisplay::CeglGetCurrentDisplay() {}

gits::OpenGL::CeglGetCurrentDisplay::CeglGetCurrentDisplay(EGLDisplay return_value)
    : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CeglGetCurrentDisplay::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglGetCurrentDisplay::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglGetCurrentDisplay();
}

/* ***************************** EGL_QUERY_CONTEXT *************************** */

gits::OpenGL::CeglQueryContext::CeglQueryContext() {}

gits::OpenGL::CeglQueryContext::CeglQueryContext(
    EGLBoolean return_value, EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value)
    : _return_value(return_value),
      _dpy(dpy),
      _ctx(ctx),
      _attribute(attribute),
      _value(value ? *value : -1) {}

gits::CArgument& gits::OpenGL::CeglQueryContext::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _ctx, _attribute, _value);
}

void gits::OpenGL::CeglQueryContext::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLint value;
  drv.egl.eglQueryContext(*_dpy, *_ctx, *_attribute, &value);
}

/* ***************************** EGL_WAIT_GL *************************** */

gits::OpenGL::CeglWaitGL::CeglWaitGL() {}

gits::OpenGL::CeglWaitGL::CeglWaitGL(EGLBoolean return_value) : _return_value(return_value) {}

gits::CArgument& gits::OpenGL::CeglWaitGL::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglWaitGL::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglWaitGL();
}

/* ***************************** EGL_WAIT_NATIVE *************************** */

gits::OpenGL::CeglWaitNative::CeglWaitNative() {}

gits::OpenGL::CeglWaitNative::CeglWaitNative(EGLBoolean return_value, EGLint engine)
    : _return_value(return_value), _engine(engine) {}

gits::CArgument& gits::OpenGL::CeglWaitNative::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _engine);
}

void gits::OpenGL::CeglWaitNative::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglWaitNative(*_engine);
}

/* ***************************** EGL_SWAP_BUFFERS *************************** */

gits::OpenGL::CeglSwapBuffers::CeglSwapBuffers() {}

gits::OpenGL::CeglSwapBuffers::CeglSwapBuffers(EGLBoolean return_value,
                                               EGLDisplay dpy,
                                               EGLSurface surface)
    : _return_value(return_value), _dpy(dpy), _surface(surface) {}

gits::CArgument& gits::OpenGL::CeglSwapBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface);
}

void gits::OpenGL::CeglSwapBuffers::Run() {
  CHECK_FOR_EGL_INITIALIZATION(Name())
  // Static counter to not break compatibility with egl streams,
  TODO("Reconsider keeping exact recorder side frame numbers, do we need this???")
  PreSwap();
  ptbl_eglSwapBuffers(*_dpy, *_surface);
}

void gits::OpenGL::CeglSwapBuffers::Write(CCodeOStream& stream) const {
  stream.select(CCodeOStream::GITS_FRAMES_CPP);
  stream.Indent() << "eglSwapBuffers_wrap(" << _dpy << ", " << _surface << ");\n";
}

/* ***************************** EGL_COPY_BUFFERS *************************** */

gits::OpenGL::CeglCopyBuffers::CeglCopyBuffers() {}

gits::OpenGL::CeglCopyBuffers::CeglCopyBuffers(EGLBoolean return_value,
                                               EGLDisplay dpy,
                                               EGLSurface surface,
                                               EGLNativePixmapType target)
    : _return_value(return_value), _dpy(dpy), _surface(surface), _target(target) {}

gits::CArgument& gits::OpenGL::CeglCopyBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _target);
}

void gits::OpenGL::CeglCopyBuffers::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglCopyBuffers(*_dpy, *_surface, *_target);
}

/* ***************************** EGL_GET_PROC_ADDRESS *************************** */

gits::OpenGL::CeglGetProcAddress::CeglGetProcAddress() {}

gits::OpenGL::CeglGetProcAddress::CeglGetProcAddress(GLsizeiptr return_value, const char* procname)
    : _return_value(return_value), _procname(procname, '\0', 1) {}

gits::CArgument& gits::OpenGL::CeglGetProcAddress::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _procname);
}

void gits::OpenGL::CeglGetProcAddress::Write(CCodeOStream& stream) const {}

void gits::OpenGL::CeglGetProcAddress::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  drv.egl.eglGetProcAddress(*_procname);
}

/* ***************************** ID_EGL_CLIENT_WAIT_SYNC_KHR *************************** */

gits::OpenGL::CeglClientWaitSyncKHR::CeglClientWaitSyncKHR() {}

gits::OpenGL::CeglClientWaitSyncKHR::CeglClientWaitSyncKHR(EGLDisplay dpy,
                                                           EGLSyncKHR sync,
                                                           EGLint flags,
                                                           EGLTimeKHR timeout)
    : _dpy(dpy), _sync(sync), _flags(flags), _timeout(timeout) {}

gits::CArgument& gits::OpenGL::CeglClientWaitSyncKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _sync, _flags, _timeout);
}

void gits::OpenGL::CeglClientWaitSyncKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglClientWaitSyncKHR(*_dpy, *_sync, *_flags, *_timeout);
}

/* ***************************** ID_EGL_CLIENT_WAIT_SYNC_NV *************************** */

gits::OpenGL::CeglClientWaitSyncNV::CeglClientWaitSyncNV() {}

gits::OpenGL::CeglClientWaitSyncNV::CeglClientWaitSyncNV(EGLSyncNV sync,
                                                         EGLint flags,
                                                         EGLTimeNV timeout)
    : _sync(sync), _flags(flags), _timeout(timeout) {}

gits::CArgument& gits::OpenGL::CeglClientWaitSyncNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _flags, _timeout);
}

void gits::OpenGL::CeglClientWaitSyncNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglClientWaitSyncNV(*_sync, *_flags, *_timeout);
}

/* ***************************** ID_EGL_CREATE_DRMIMAGE_MESA *************************** */

gits::OpenGL::CeglCreateDRMImageMESA::CeglCreateDRMImageMESA() {}

gits::OpenGL::CeglCreateDRMImageMESA::CeglCreateDRMImageMESA(EGLDisplay dpy,
                                                             const EGLint* attrib_list)
    : _dpy(dpy), _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreateDRMImageMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _attrib_list);
}

void gits::OpenGL::CeglCreateDRMImageMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglCreateDRMImageMESA(*_dpy, *_attrib_list);
}

/* ***************************** ID_EGL_CREATE_FENCE_SYNC_NV *************************** */

gits::OpenGL::CeglCreateFenceSyncNV::CeglCreateFenceSyncNV() {}

gits::OpenGL::CeglCreateFenceSyncNV::CeglCreateFenceSyncNV(EGLDisplay dpy,
                                                           EGLenum condition,
                                                           const EGLint* attrib_list)
    : _dpy(dpy), _condition(condition), _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreateFenceSyncNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _condition, _attrib_list);
}

void gits::OpenGL::CeglCreateFenceSyncNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglCreateFenceSyncNV(*_dpy, *_condition, *_attrib_list);
}

/* ***************************** ID_EGL_CREATE_IMAGE_KHR *************************** */

gits::OpenGL::CeglCreateImageKHR::CeglCreateImageKHR() {}

gits::OpenGL::CeglCreateImageKHR::CeglCreateImageKHR(EGLImageKHR return_value,
                                                     EGLDisplay dpy,
                                                     EGLContext ctx,
                                                     EGLenum target,
                                                     EGLClientBuffer buffer,
                                                     const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _ctx(ctx),
      _target(target),
      _buffer(buffer),
      _attrib_list(attrib_list, EGL_NONE, 2) {}

unsigned gits::OpenGL::CeglCreateImageKHR::ArgumentCount() const {
  return 11;
}

gits::CArgument& gits::OpenGL::CeglCreateImageKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _ctx, _target, _buffer, _attrib_list, _anb_resource,
                       _anb_width, _anb_height, _anb_format, _anb_usage, _anb_stride);
}

void gits::OpenGL::CeglCreateImageKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    Log(ERR) << "Stream may not be portable - egl images can't be translated to other native APIs. "
                "eglCreateImageKHR skipped.";
    return;
  }

  _buffer.AddMapping(_buffer.Original());
  EGLClientBuffer buffer = nullptr;

  GLuint bufferOrig = (GLuint)(uintptr_t)(_buffer.Original());

  if (*_target == EGL_GL_TEXTURE_2D_KHR || *_target == EGL_GL_TEXTURE_3D_KHR ||
      (*_target >= EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR &&
       *_target <= EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR)) {
    if (!CGLTexture::CheckMapping(bufferOrig)) {
      //After implementing ES texture restoration this texture will need to be restored in this token
      Log(WARN) << "Texture used to create eglImage not restored - eglCreateImageKHR will fail - "
                   "shouldn't influence rendered image.";
    } else {
      buffer = (EGLClientBuffer)(uintptr_t)CGLTexture::GetMapping(bufferOrig);
    }
  } else if (_target.Original() == EGL_GL_RENDERBUFFER_KHR) {
    if (!CGLRenderbuffer::CheckMapping(bufferOrig)) {
      //After implementing ES texture restoration this texture will need to be restored in this token
      Log(WARN) << "Renderbuffer used to create eglImage not restored - eglCreateImageKHR will "
                   "fail - shouldn't influence rendered image.";
    } else {
      buffer = (EGLClientBuffer)(uintptr_t)CGLRenderbuffer::GetMapping(bufferOrig);
    }
  } else {
    Log(ERR) << "Not supported eglCreateImageKHR target: " << *_target;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  EGLImageKHR image = drv.egl.eglCreateImageKHR(*_dpy, *_ctx, *_target, buffer, *_attrib_list);
  _return_value.AddMapping(image);
}

void gits::OpenGL::CeglCreateImageKHR::Write(CCodeOStream& stream) const {

  CScopeGenerator generateScopedBrackets(stream);
  stream.Indent() << "EGLDisplay dpy = " << _dpy << ";" << std::endl;
  stream.Indent() << "EGLContext ctx = " << _ctx << ";" << std::endl;
  _attrib_list.VariableNameRegister(stream, false);
  _attrib_list.Declare(stream);
  stream.Indent() << _return_value << " = " << Name() << "_wrap(dpy, ctx, " << _target
                  << ", (EGLClientBuffer)" << _buffer.Original() << ", " << _attrib_list << ", "
                  << _anb_width << ", " << _anb_height << ", " << _anb_format << ", " << _anb_usage
                  << ", " << _anb_stride << ", " << _anb_resource << ");" << std::endl;
}

/* ***************************** ID_EGL_CREATE_PIXMAP_SURFACE_HI *************************** */

gits::OpenGL::CeglCreatePixmapSurfaceHI::CeglCreatePixmapSurfaceHI() {}

gits::OpenGL::CeglCreatePixmapSurfaceHI::CeglCreatePixmapSurfaceHI(EGLDisplay dpy,
                                                                   EGLConfig config,
                                                                   void* pixmap)
    : _dpy(dpy), _config(config), _pixmap(pixmap) {}

gits::CArgument& gits::OpenGL::CeglCreatePixmapSurfaceHI::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _pixmap);
}

void gits::OpenGL::CeglCreatePixmapSurfaceHI::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglCreatePixmapSurfaceHI(*_dpy, *_config, *_pixmap);
}

/* ***************************** ID_EGL_CREATE_STREAM_FROM_FILE_DESCRIPTOR_KHR *************************** */

gits::OpenGL::CeglCreateStreamFromFileDescriptorKHR::CeglCreateStreamFromFileDescriptorKHR() {}

gits::OpenGL::CeglCreateStreamFromFileDescriptorKHR::CeglCreateStreamFromFileDescriptorKHR(
    EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor)
    : _dpy(dpy), _file_descriptor(file_descriptor) {}

gits::CArgument& gits::OpenGL::CeglCreateStreamFromFileDescriptorKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _file_descriptor);
}

void gits::OpenGL::CeglCreateStreamFromFileDescriptorKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglCreateStreamFromFileDescriptorKHR(*_dpy, *_file_descriptor);
}

/* ***************************** ID_EGL_CREATE_STREAM_KHR *************************** */

gits::OpenGL::CeglCreateStreamKHR::CeglCreateStreamKHR() {}

gits::OpenGL::CeglCreateStreamKHR::CeglCreateStreamKHR(EGLDisplay dpy, const EGLint* attrib_list)
    : _dpy(dpy), _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreateStreamKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _attrib_list);
}

void gits::OpenGL::CeglCreateStreamKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglCreateStreamKHR(*_dpy, *_attrib_list);
}

/* ***************************** ID_EGL_CREATE_STREAM_PRODUCER_SURFACE_KHR *************************** */

gits::OpenGL::CeglCreateStreamProducerSurfaceKHR::CeglCreateStreamProducerSurfaceKHR() {}

gits::OpenGL::CeglCreateStreamProducerSurfaceKHR::CeglCreateStreamProducerSurfaceKHR(
    EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint* attrib_list)
    : _dpy(dpy), _config(config), _stream(stream), _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreateStreamProducerSurfaceKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _stream, _attrib_list);
}

void gits::OpenGL::CeglCreateStreamProducerSurfaceKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglCreateStreamProducerSurfaceKHR(*_dpy, *_config, *_stream, *_attrib_list);
}

/* ***************************** ID_EGL_CREATE_SYNC_KHR *************************** */

gits::OpenGL::CeglCreateSyncKHR::CeglCreateSyncKHR() {}

gits::OpenGL::CeglCreateSyncKHR::CeglCreateSyncKHR(EGLSyncKHR return_value,
                                                   EGLDisplay dpy,
                                                   EGLenum type,
                                                   const EGLint* attrib_list)
    : _return_value(return_value), _dpy(dpy), _type(type), _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglCreateSyncKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _type, _attrib_list, _return_value);
}

void gits::OpenGL::CeglCreateSyncKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  EGLSyncKHR sync = drv.egl.eglCreateSyncKHR(*_dpy, *_type, *_attrib_list);
  _return_value.AddMapping(sync);
}

/* ***************************** ID_EGL_DESTROY_IMAGE_KHR *************************** */

gits::OpenGL::CeglDestroyImageKHR::CeglDestroyImageKHR() {}

gits::OpenGL::CeglDestroyImageKHR::CeglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
    : _dpy(dpy), _image(image) {}

gits::CArgument& gits::OpenGL::CeglDestroyImageKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _image);
}

void gits::OpenGL::CeglDestroyImageKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    Log(ERR) << "Stream may not be portable - egl images can't be translated to other native APIs. "
                "eglDestroyImageKHR skipped.";
    return;
  }
  drv.egl.eglDestroyImageKHR(*_dpy, *_image);
  _image.RemoveMapping();
}

/* ***************************** ID_EGL_DESTROY_STREAM_KHR *************************** */

gits::OpenGL::CeglDestroyStreamKHR::CeglDestroyStreamKHR() {}

gits::OpenGL::CeglDestroyStreamKHR::CeglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream)
    : _dpy(dpy), _stream(stream) {}

gits::CArgument& gits::OpenGL::CeglDestroyStreamKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream);
}

void gits::OpenGL::CeglDestroyStreamKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglDestroyStreamKHR(*_dpy, *_stream);
}

/* ***************************** ID_EGL_DESTROY_SYNC_KHR *************************** */

gits::OpenGL::CeglDestroySyncKHR::CeglDestroySyncKHR() {}

gits::OpenGL::CeglDestroySyncKHR::CeglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
    : _dpy(dpy), _sync(sync) {}

gits::CArgument& gits::OpenGL::CeglDestroySyncKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _sync);
}

void gits::OpenGL::CeglDestroySyncKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglDestroySyncKHR(*_dpy, *_sync);
  _sync.RemoveMapping();
}

/* ***************************** ID_EGL_DESTROY_SYNC_NV *************************** */

gits::OpenGL::CeglDestroySyncNV::CeglDestroySyncNV() {}

gits::OpenGL::CeglDestroySyncNV::CeglDestroySyncNV(EGLSyncNV sync) : _sync(sync) {}

gits::CArgument& gits::OpenGL::CeglDestroySyncNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _sync);
}

void gits::OpenGL::CeglDestroySyncNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglDestroySyncNV(*_sync);
}

/* ***************************** ID_EGL_EXPORT_DRMIMAGE_MESA *************************** */

gits::OpenGL::CeglExportDRMImageMESA::CeglExportDRMImageMESA() {}

gits::OpenGL::CeglExportDRMImageMESA::CeglExportDRMImageMESA(
    EGLDisplay dpy, EGLImageKHR image, EGLint* name, EGLint* handle, EGLint* stride)
    : _dpy(dpy), _image(image), _name(name), _handle(handle), _stride(stride) {}

gits::CArgument& gits::OpenGL::CeglExportDRMImageMESA::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _image, _name, _handle, _stride);
}

void gits::OpenGL::CeglExportDRMImageMESA::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglExportDRMImageMESA(*_dpy, *_image, *_name, *_handle, *_stride);
}

/* ***************************** ID_EGL_FENCE_NV *************************** */

gits::OpenGL::CeglFenceNV::CeglFenceNV() {}

gits::OpenGL::CeglFenceNV::CeglFenceNV(EGLSyncNV sync) : _sync(sync) {}

gits::CArgument& gits::OpenGL::CeglFenceNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _sync);
}

void gits::OpenGL::CeglFenceNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglFenceNV(*_sync);
}

/* ***************************** ID_EGL_GET_STREAM_FILE_DESCRIPTOR_KHR *************************** */

gits::OpenGL::CeglGetStreamFileDescriptorKHR::CeglGetStreamFileDescriptorKHR() {}

gits::OpenGL::CeglGetStreamFileDescriptorKHR::CeglGetStreamFileDescriptorKHR(EGLDisplay dpy,
                                                                             EGLStreamKHR stream)
    : _dpy(dpy), _stream(stream) {}

gits::CArgument& gits::OpenGL::CeglGetStreamFileDescriptorKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream);
}

void gits::OpenGL::CeglGetStreamFileDescriptorKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglGetStreamFileDescriptorKHR(*_dpy, *_stream);
}

/* ***************************** ID_EGL_GET_SYNC_ATTRIB_KHR *************************** */

gits::OpenGL::CeglGetSyncAttribKHR::CeglGetSyncAttribKHR() {}

gits::OpenGL::CeglGetSyncAttribKHR::CeglGetSyncAttribKHR(EGLDisplay dpy,
                                                         EGLSyncKHR sync,
                                                         EGLint attribute,
                                                         EGLint* value)
    : _dpy(dpy), _sync(sync), _attribute(attribute), _value(value ? *value : -1) {}

gits::CArgument& gits::OpenGL::CeglGetSyncAttribKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _sync, _attribute, _value);
}

void gits::OpenGL::CeglGetSyncAttribKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  CHECK_FOR_EGL_INITIALIZATION(Name())
  EGLint value;
  drv.egl.eglGetSyncAttribKHR(*_dpy, *_sync, *_attribute, &value);
}

/* ***************************** ID_EGL_GET_SYNC_ATTRIB_NV *************************** */

gits::OpenGL::CeglGetSyncAttribNV::CeglGetSyncAttribNV() {}

gits::OpenGL::CeglGetSyncAttribNV::CeglGetSyncAttribNV(EGLSyncNV sync,
                                                       EGLint attribute,
                                                       EGLint* value)
    : _sync(sync), _attribute(attribute), _value(value) {}

gits::CArgument& gits::OpenGL::CeglGetSyncAttribNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _sync, _attribute, _value);
}

void gits::OpenGL::CeglGetSyncAttribNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglGetSyncAttribNV(*_sync, *_attribute, *_value);
}

/* ***************************** ID_EGL_GET_SYSTEM_TIME_FREQUENCY_NV *************************** */

gits::OpenGL::CeglGetSystemTimeFrequencyNV::CeglGetSystemTimeFrequencyNV() {}

gits::CArgument& gits::OpenGL::CeglGetSystemTimeFrequencyNV::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglGetSystemTimeFrequencyNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglGetSystemTimeFrequencyNV();
}

/* ***************************** ID_EGL_GET_SYSTEM_TIME_NV *************************** */

gits::OpenGL::CeglGetSystemTimeNV::CeglGetSystemTimeNV() {}

gits::CArgument& gits::OpenGL::CeglGetSystemTimeNV::Argument(unsigned idx) {
  report_cargument_error(__FUNCTION__, idx);
}

void gits::OpenGL::CeglGetSystemTimeNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglGetSystemTimeNV();
}

/* ***************************** ID_EGL_LOCK_SURFACE_KHR *************************** */

gits::OpenGL::CeglLockSurfaceKHR::CeglLockSurfaceKHR() {}

gits::OpenGL::CeglLockSurfaceKHR::CeglLockSurfaceKHR(EGLDisplay display,
                                                     EGLSurface surface,
                                                     const EGLint* attrib_list)
    : _display(display), _surface(surface), _attrib_list(attrib_list, EGL_NONE, 2) {}

gits::CArgument& gits::OpenGL::CeglLockSurfaceKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _display, _surface, _attrib_list);
}

void gits::OpenGL::CeglLockSurfaceKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglLockSurfaceKHR(*_display, *_surface, *_attrib_list);
}

/* ***************************** ID_EGL_POST_SUB_BUFFER_NV *************************** */

gits::OpenGL::CeglPostSubBufferNV::CeglPostSubBufferNV() {}

gits::OpenGL::CeglPostSubBufferNV::CeglPostSubBufferNV(
    EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height)
    : _dpy(dpy), _surface(surface), _x(x), _y(y), _width(width), _height(height) {}

gits::CArgument& gits::OpenGL::CeglPostSubBufferNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _x, _y, _width, _height);
}

void gits::OpenGL::CeglPostSubBufferNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglPostSubBufferNV(*_dpy, *_surface, *_x, *_y, *_width, *_height);
}

/* ***************************** ID_EGL_QUERY_STREAM_KHR *************************** */

gits::OpenGL::CeglQueryStreamKHR::CeglQueryStreamKHR() {}

gits::OpenGL::CeglQueryStreamKHR::CeglQueryStreamKHR(EGLDisplay dpy,
                                                     EGLStreamKHR stream,
                                                     EGLenum attribute,
                                                     EGLint* value)
    : _dpy(dpy), _stream(stream), _attribute(attribute), _value(value) {}

gits::CArgument& gits::OpenGL::CeglQueryStreamKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream, _attribute, _value);
}

void gits::OpenGL::CeglQueryStreamKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglQueryStreamKHR(*_dpy, *_stream, *_attribute, *_value);
}

/* ***************************** ID_EGL_QUERY_STREAM_TIME_KHR *************************** */

gits::OpenGL::CeglQueryStreamTimeKHR::CeglQueryStreamTimeKHR() {}

gits::OpenGL::CeglQueryStreamTimeKHR::CeglQueryStreamTimeKHR(EGLDisplay dpy,
                                                             EGLStreamKHR stream,
                                                             EGLenum attribute,
                                                             EGLTimeKHR* value)
    : _dpy(dpy), _stream(stream), _attribute(attribute), _value(value) {}

gits::CArgument& gits::OpenGL::CeglQueryStreamTimeKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream, _attribute, _value);
}

void gits::OpenGL::CeglQueryStreamTimeKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglQueryStreamTimeKHR(*_dpy, *_stream, *_attribute, *_value);
}

/* ***************************** ID_EGL_QUERY_STREAMU64KHR *************************** */

gits::OpenGL::CeglQueryStreamu64KHR::CeglQueryStreamu64KHR() {}

gits::OpenGL::CeglQueryStreamu64KHR::CeglQueryStreamu64KHR(EGLDisplay dpy,
                                                           EGLStreamKHR stream,
                                                           EGLenum attribute,
                                                           EGLuint64KHR* value)
    : _dpy(dpy), _stream(stream), _attribute(attribute), _value(value) {}

gits::CArgument& gits::OpenGL::CeglQueryStreamu64KHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream, _attribute, _value);
}

void gits::OpenGL::CeglQueryStreamu64KHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglQueryStreamu64KHR(*_dpy, *_stream, *_attribute, *_value);
}

/* ***************************** ID_EGL_QUERY_SURFACE_POINTER_ANGLE *************************** */

gits::OpenGL::CeglQuerySurfacePointerANGLE::CeglQuerySurfacePointerANGLE() {}

gits::OpenGL::CeglQuerySurfacePointerANGLE::CeglQuerySurfacePointerANGLE(EGLDisplay dpy,
                                                                         EGLSurface surface,
                                                                         EGLint attribute,
                                                                         void** value)
    : _dpy(dpy), _surface(surface), _attribute(attribute), _value(value) {}

gits::CArgument& gits::OpenGL::CeglQuerySurfacePointerANGLE::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _surface, _attribute, _value);
}

void gits::OpenGL::CeglQuerySurfacePointerANGLE::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglQuerySurfacePointerANGLE(*_dpy, *_surface, *_attribute, *_value);
}

/* ***************************** ID_EGL_SIGNAL_SYNC_KHR *************************** */

gits::OpenGL::CeglSignalSyncKHR::CeglSignalSyncKHR() {}

gits::OpenGL::CeglSignalSyncKHR::CeglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode)
    : _dpy(dpy), _sync(sync), _mode(mode) {}

gits::CArgument& gits::OpenGL::CeglSignalSyncKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _sync, _mode);
}

void gits::OpenGL::CeglSignalSyncKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglSignalSyncKHR(*_dpy, *_sync, *_mode);
}

/* ***************************** ID_EGL_SIGNAL_SYNC_NV *************************** */

gits::OpenGL::CeglSignalSyncNV::CeglSignalSyncNV() {}

gits::OpenGL::CeglSignalSyncNV::CeglSignalSyncNV(EGLSyncNV sync, EGLenum mode)
    : _sync(sync), _mode(mode) {}

gits::CArgument& gits::OpenGL::CeglSignalSyncNV::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _sync, _mode);
}

void gits::OpenGL::CeglSignalSyncNV::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglSignalSyncNV(*_sync, *_mode);
}

/* ***************************** ID_EGL_STREAM_ATTRIB_KHR *************************** */

gits::OpenGL::CeglStreamAttribKHR::CeglStreamAttribKHR() {}

gits::OpenGL::CeglStreamAttribKHR::CeglStreamAttribKHR(EGLDisplay dpy,
                                                       EGLStreamKHR stream,
                                                       EGLenum attribute,
                                                       EGLint value)
    : _dpy(dpy), _stream(stream), _attribute(attribute), _value(value) {}

gits::CArgument& gits::OpenGL::CeglStreamAttribKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream, _attribute, _value);
}

void gits::OpenGL::CeglStreamAttribKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglStreamAttribKHR(*_dpy, *_stream, *_attribute, *_value);
}

/* ***************************** ID_EGL_STREAM_CONSUMER_ACQUIRE_KHR *************************** */

gits::OpenGL::CeglStreamConsumerAcquireKHR::CeglStreamConsumerAcquireKHR() {}

gits::OpenGL::CeglStreamConsumerAcquireKHR::CeglStreamConsumerAcquireKHR(EGLDisplay dpy,
                                                                         EGLStreamKHR stream)
    : _dpy(dpy), _stream(stream) {}

gits::CArgument& gits::OpenGL::CeglStreamConsumerAcquireKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream);
}

void gits::OpenGL::CeglStreamConsumerAcquireKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglStreamConsumerAcquireKHR(*_dpy, *_stream);
}

/* ***************************** ID_EGL_STREAM_CONSUMER_GLTEXTURE_EXTERNAL_KHR *************************** */

gits::OpenGL::CeglStreamConsumerGLTextureExternalKHR::CeglStreamConsumerGLTextureExternalKHR() {}

gits::OpenGL::CeglStreamConsumerGLTextureExternalKHR::CeglStreamConsumerGLTextureExternalKHR(
    EGLDisplay dpy, EGLStreamKHR stream)
    : _dpy(dpy), _stream(stream) {}

gits::CArgument& gits::OpenGL::CeglStreamConsumerGLTextureExternalKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream);
}

void gits::OpenGL::CeglStreamConsumerGLTextureExternalKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglStreamConsumerGLTextureExternalKHR(*_dpy, *_stream);
}

/* ***************************** ID_EGL_STREAM_CONSUMER_RELEASE_KHR *************************** */

gits::OpenGL::CeglStreamConsumerReleaseKHR::CeglStreamConsumerReleaseKHR() {}

gits::OpenGL::CeglStreamConsumerReleaseKHR::CeglStreamConsumerReleaseKHR(EGLDisplay dpy,
                                                                         EGLStreamKHR stream)
    : _dpy(dpy), _stream(stream) {}

gits::CArgument& gits::OpenGL::CeglStreamConsumerReleaseKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _stream);
}

void gits::OpenGL::CeglStreamConsumerReleaseKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglStreamConsumerReleaseKHR(*_dpy, *_stream);
}

/* ***************************** ID_EGL_UNLOCK_SURFACE_KHR *************************** */

gits::OpenGL::CeglUnlockSurfaceKHR::CeglUnlockSurfaceKHR() {}

gits::OpenGL::CeglUnlockSurfaceKHR::CeglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface)
    : _display(display), _surface(surface) {}

gits::CArgument& gits::OpenGL::CeglUnlockSurfaceKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _display, _surface);
}

void gits::OpenGL::CeglUnlockSurfaceKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglUnlockSurfaceKHR(*_display, *_surface);
}

/* ***************************** ID_EGL_WAIT_SYNC_KHR *************************** */

gits::OpenGL::CeglWaitSyncKHR::CeglWaitSyncKHR() {}

gits::OpenGL::CeglWaitSyncKHR::CeglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags)
    : _dpy(dpy), _sync(sync), _flags(flags) {}

gits::CArgument& gits::OpenGL::CeglWaitSyncKHR::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _sync, _flags);
}

void gits::OpenGL::CeglWaitSyncKHR::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglWaitSyncKHR(*_dpy, *_sync, *_flags);
}

/* ***************************** ID_EGL_SET_BLOB_CACHE_FUNCS_ANDROID *************************** */

gits::OpenGL::CeglSetBlobCacheFuncsANDROID::CeglSetBlobCacheFuncsANDROID() {}

gits::OpenGL::CeglSetBlobCacheFuncsANDROID::CeglSetBlobCacheFuncsANDROID(EGLDisplay dpy,
                                                                         EGLSetBlobFuncANDROID set,
                                                                         EGLGetBlobFuncANDROID get)
    : _dpy(dpy) {}

gits::CArgument& gits::OpenGL::CeglSetBlobCacheFuncsANDROID::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy);
}

void gits::OpenGL::CeglSetBlobCacheFuncsANDROID::Run() {
  if (PtblNtvApi() != PtblNtvStreamApi()) {
    return;
  }
  drv.egl.eglSetBlobCacheFuncsANDROID(
      *_dpy,
      [](const void* key, EGLsizeiANDROID keySize, const void* value,
         EGLsizeiANDROID valueSize) -> void { Log(WARN) << "Set blob cache function used."; },
      [](const void* key, EGLsizeiANDROID keySize, void* value,
         EGLsizeiANDROID valueSize) -> EGLsizeiANDROID {
        Log(WARN) << "Get blob cache function used, returning nothing";
        return 0;
      });
}

/* ***************************** ID_EGL_GET_PLATFORM_DISPLAY_EXT *************************** */

gits::OpenGL::CeglGetPlatformDisplayEXT::CeglGetPlatformDisplayEXT() {
  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
}

gits::OpenGL::CeglGetPlatformDisplayEXT::CeglGetPlatformDisplayEXT(EGLDisplay return_value,
                                                                   EGLenum platform,
                                                                   Display* native_display,
                                                                   const EGLint* attrib_list)
    : _return_value(return_value),
      _platform(platform),
      _native_display(native_display),
      _attrib_list(attrib_list, EGL_NONE, 2) {
  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
}

gits::CArgument& gits::OpenGL::CeglGetPlatformDisplayEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _platform, _native_display, _attrib_list);
}

void gits::OpenGL::CeglGetPlatformDisplayEXT::Run() {
  _native_display.AddMapping((Display*)GetNativeDisplay());

  PtblNtvStreamApi(PtblNativeAPI::Type::EGL);
  CGits::Instance().apis.UseApi3dIface(std::shared_ptr<ApisIface::Api3d>(new OpenGLApi()));
  if (PtblNtvApi() == PtblNativeAPI::Type::EGL) {
    initialize_egl();
  } else {
    isEGLInitialized = true;
  }

  EGLDisplay display =
      drv.egl.eglGetPlatformDisplayEXT(*_platform, GetNativeDisplay(), *_attrib_list);

  _return_value.AddMapping(display);
}

/* ***************************** ID_EGL_CREATE_PLATFORM_WINDOW_SURFACE_EXT *************************** */

gits::OpenGL::CeglCreatePlatformWindowSurfaceEXT::CeglCreatePlatformWindowSurfaceEXT() {}

gits::OpenGL::CeglCreatePlatformWindowSurfaceEXT::CeglCreatePlatformWindowSurfaceEXT(
    EGLSurface return_value,
    EGLDisplay dpy,
    EGLConfig config,
    EGLNativeWindowType native_window,
    const EGLint* attrib_list)
    : _return_value(return_value),
      _dpy(dpy),
      _config(config),
      _native_window(native_window),
      _attrib_list(attrib_list, EGL_NONE, 2),
      _config_attribs(get_config_attribs(dpy, config)),
      _surface_attribs(get_surface_attribs(dpy, return_value)) {}

gits::CArgument& gits::OpenGL::CeglCreatePlatformWindowSurfaceEXT::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dpy, _config, _native_window, _attrib_list,
                       _config_attribs, _surface_attribs);
}

void gits::OpenGL::CeglCreatePlatformWindowSurfaceEXT::Run() {
  float scale = Config::Get().player.scaleFactor;
  EGLint width = static_cast<EGLint>(scale * find_attrib(EGL_WIDTH, _surface_attribs.Vector()));
  EGLint height = static_cast<EGLint>(scale * find_attrib(EGL_HEIGHT, _surface_attribs.Vector()));

  if (Config::Get().player.forceWindowSize) {
    width = Config::Get().player.windowSize.first;
    height = Config::Get().player.windowSize.second;
  }

  const auto& attr = _config_attribs.Vector();
  EGLConfig config = ptblFindConfigEGL(*_dpy, attr);
  log_config_properties(*_dpy, config);

  Window_* wnd = new Window_(config, width, height, 0, 0, true);
  EGLSurface surface = drv.egl.eglCreatePlatformWindowSurfaceEXT(
      *_dpy, config, static_cast<void*>(wnd), *_attrib_list);

  CStateDynamicNative::Get().MapAddWindowPlayer(wnd->handle(), wnd);
  // TODO: Call MapDeleteWindowPlayer on eglDestroySurface. We'll likely need a map from EGLSurface to Window_.

  _return_value.AddMapping(surface);
}
