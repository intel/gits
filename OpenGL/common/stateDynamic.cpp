// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   stateDynamic.cpp
 *
 * @brief Definition of OpenGL common part library implementation.
 *
 */

#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "openglLibrary.h"
#include "key_value.h"
#include "config.h"
#include "windowContextState.h"

namespace gits {
namespace OpenGL {

//----------------------------SHARED STATE DYNAMIC-----------------------------

CSharedStateDynamic& CSharedStateDynamic::GetStatic() {
  static CSharedStateDynamic* ptr = new CSharedStateDynamic;
  return *ptr;
}

CSharedStateDynamic::CSharedStateDynamic()
    : coherentBufferMapping(false), coherentBufferFrameNumber(0), _owner(nullptr) {
  // In OGL, textures bound by default have the name zero.
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_1D));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_2D));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_3D));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_RECTANGLE));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_BUFFER));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_CUBE_MAP));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_1D_ARRAY));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_2D_ARRAY));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_CUBE_MAP_ARRAY));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_2D_MULTISAMPLE));
  Textures().Add(CTextureStateObj(0, GL_TEXTURE_2D_MULTISAMPLE_ARRAY));
  // This list might need to be expanded in the future.
}

CSharedStateDynamic::~CSharedStateDynamic() {}

//----------------------------CONTEXT STATE DYNAMIC-----------------------------

CContextStateDynamic& CContextStateDynamic::GetStatic() {
  static CContextStateDynamic* ptr = new CContextStateDynamic;
  return *ptr;
}

CContextStateDynamic::CContextStateDynamic()
    : pushUsed(false),
      glBeginState(false),
      restartIndexValue(4294967295u),
      _version(-1),
      _isEs(-1),
      _isNvidia(-1),
      _profileMask(-1),
      _schedulerStatePtr(nullptr) {}

CContextStateDynamic::~CContextStateDynamic() {}

int CContextStateDynamic::Version() {
  // Only ask for version once. Ideally this should be moved to
  // per context state dynamic.
  if (_version != -1) {
    return _version;
  }

  const char* ver = (const char*)drv.gl.glGetString(GL_VERSION);
  if (ver == nullptr) {
    Log(WARN) << "glGetString(GL_VERSION) returned 0";
    _version = 0;
    return _version;
  }

  Log(INFO) << "Queried context version: " << ver;

  std::string version;
  std::stringstream str(ver);

  bool isES = false;
  if (strstr(ver, "OpenGL ES") != nullptr) {
    isES = true;
    str >> version; // consume 'OpenGL'
    str >> version; // consume 'ES-XX'
  }
  str >> version; // consume rest

  // drop the dot
  std::replace(version.begin(), version.end(), '.', ' ');

  // get major / minor version
  int minor = 0, major = 0;
  str.clear();
  str.str(version);
  str >> major >> minor;

  // Check for valid OpenGL and OpenGL ES version ranges
  if ((isES && (major < 1 || major > 3 || minor < 0 || minor > 2)) ||
      (!isES && (major < 1 || major > 4 || minor < 0 || minor > 6))) {
    throw std::out_of_range("Invalid OpenGL or OpenGL ES version numbers");
  }

  // Perform safe arithmetic operations
  if (major > (INT_MAX / 100) || minor > (INT_MAX / 10)) {
    throw std::overflow_error("Version number calculation overflow");
  }

  _version = 100 * major + 10 * minor;
  return _version;
}

bool CContextStateDynamic::IsEs() {
  // Only ask for version once. TODO: Ideally this should be moved to
  // per context state dynamic.
  if (_isEs != -1) {
    return _isEs;
  }

  const char* ver = (const char*)drv.gl.glGetString(GL_VERSION);
  if (ver == nullptr) {
    Log(WARN) << "glGetString(GL_VERSION) returned 0. Using legacy method.";
    return drv.gl.Api() != CGlDriver::API_GL;
  }

  std::string version;
  std::stringstream strstream(ver);

  version = strstream.str();

  size_t found = version.find("OpenGL ES");
  if (found == std::string::npos) {
    _isEs = 0;
    return _isEs;
  } else {
    _isEs = 1;
    return _isEs;
  }
}

bool CContextStateDynamic::IsNvidia() {
  if (_isNvidia != -1) {
    return _isNvidia;
  }

  const char* ver = (const char*)drv.gl.glGetString(GL_VERSION);

  if (ver == nullptr) {
    // defaulting to non-nvidia
    _isNvidia = 0;
  } else {
    _isNvidia = strstr(ver, "NVIDIA") != nullptr ? 1 : 0;
  }

  return _isNvidia;
}

GLint CContextStateDynamic::ProfileMask() {
  if (_profileMask != -1) {
    return _profileMask;
  }

  drv.gl.glGetError();
  drv.gl.glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &_profileMask);
  if (drv.gl.glGetError() != GL_NO_ERROR) {
    CALL_ONCE[&] {
      Log(WARN) << "GL_CONTEXT_PROFILE_MASK. Error querying profile mask";
    };
    _profileMask = 0;
  }
  return _profileMask;
}

//------------------------------- STATE DYNAMIC ---------------------------------
CStateDynamic::CStateDynamic()
    : _ctxToSharedGroupIdMap(),
      _currentContextUpdateNeeded(true),
      _currentThread(), // No thread in particular.
      _contextStateData(nullptr),
      _currentContext(nullptr),
      _contextToSharedGroup(_ctxToSharedGroupIdMap.end()) {}

CStateDynamic::~CStateDynamic() {}

CStateDynamic& CStateDynamic::Get() {
  static CStateDynamic* ptr = new CStateDynamic;
  return *ptr;
}

void CStateDynamic::WriteClientSizes() {
  if (!Configurator::Get().common.recorder.enabled) {
    return;
  }

  std::map<uint64_t, uint64_t> mapAreasSizes;
  for (auto& area : _memTracker) {
    mapAreasSizes[area.first] = area.second.size();
  }
#ifndef BUILD_FOR_CCODE
  write_map(Configurator::Get().common.recorder.dumpPath / "gitsClientSizes.dat", mapAreasSizes);
#endif
}

std::function<void()> CStateDynamic::CreateCArraysRestorePoint() {
  auto& ref = _memTracker;
  return [=]() mutable { // mutable to be able to use [] on ref
    for (auto& e : _memTracker) {
      std::copy(ref[e.first].begin(), ref[e.first].end(), e.second.begin());
    }
  };
}

CStateDynamic::CContextStateData* CStateDynamic::GetContextStateData(void* context) {
  if (context == nullptr) {
    Log(WARN) << "Trying to get state dynamic data from null context";
    return nullptr;
  }

  CContextMap::iterator contextData = _contextMap.find(context);
  if (contextData == _contextMap.end()) {
    CALL_ONCE[] {
      Log(WARN) << "Trying to get state dynamic data from unknown context";
    };
    return nullptr;
  }

  return &(contextData->second);
}

void CStateDynamic::AddContext(void* context, void* shared) {
  if (context == nullptr) {
    return;
  }

  // CREATE STATE DYNAMIC FOR CONTEXT/SHARED GROUP
  if (shared == nullptr || context == shared) {
    // add new shared state and non-shared state
    _contextMap.insert(std::pair<void*, CContextStateData>(context, CContextStateData()));
    _contextMap[context].contextStateDynamic.reset(new CContextStateDynamic);
    _contextMap[context].sharedStateDynamic.reset(new CSharedStateDynamic);
    _contextMap[context].sharedStateDynamic->_owner = context;
  } else {
    // Find context to share with
    CContextStateData* contextDataToShare = GetContextStateData(shared);
    if (contextDataToShare == nullptr) {
      throw ENotFound(EXCEPTION_MESSAGE);
    }

    // add new non shared state and point to proper shared state
    _contextMap.insert(std::pair<void*, CContextStateData>(context, CContextStateData()));
    _contextMap[context].contextStateDynamic.reset(new CContextStateDynamic);
    _contextMap[context].sharedStateDynamic = contextDataToShare->sharedStateDynamic;
  }

  _contextMap[context].coreContext = -1; // mark core context as not checked

  static int contextOrdinal = 1;
  _contextMap[context].contextOrdinal = contextOrdinal++;

  // ADD CONTEXT TO SHARED GROUP ID MAP
  static unsigned sharedGroupId = 0;
  sharedGroupId++;
  if (shared == nullptr) {
    _ctxToSharedGroupIdMap.insert(std::pair<void*, unsigned>(context, sharedGroupId));
  } else {
    _ctxToSharedGroupIdMap.insert(
        std::pair<void*, unsigned>(context, _ctxToSharedGroupIdMap[shared]));
  }
}

void CStateDynamic::RemoveContext(void* context) {
  if (context == nullptr) {
    return;
  }

  if (GetCurrentContext() == context) {
    auto itr = _currentThreadToCtxMap.find(CGits::Instance().CurrentThreadId());
    if (itr != _currentThreadToCtxMap.end()) {
      _currentThreadToCtxMap.erase(itr);
    }
  }
  _contextMap.erase(context);
  _ctxToSharedGroupIdMap.erase(context);
}

void CStateDynamic::ShareContext(void* context1, void* context2) {
  if (context1 == nullptr || context2 == nullptr) {
    return;
  }

  // SET STATE DYNAMIC DATA SHARING
  CContextStateData* context1Data = GetContextStateData(context1);
  CContextStateData* context2Data = GetContextStateData(context2);

  if (context1Data == nullptr || context2Data == nullptr) {
    throw ENotFound(EXCEPTION_MESSAGE);
  }

  // Make context2 to use shared data from context1. context2 data is deleted.
  // Assumption made that context 2 has no data.
  context2Data->sharedStateDynamic = context1Data->sharedStateDynamic;

  // UPDATE CONTEXT TO SHARED GROUP ID MAP
  // Context 2 switches to context 1 shared group id
  _ctxToSharedGroupIdMap[context2] = _ctxToSharedGroupIdMap[context1];
}

void CStateDynamic::SetCurrentContext(void* context) {
  _currentThreadToCtxMap[CGits::Instance().CurrentThreadId()] = context;
  _currentContextUpdateNeeded = true;
}

void* CStateDynamic::GetCurrentContext() const {
  auto itr = _currentThreadToCtxMap.find(CGits::Instance().CurrentThreadId());
  if (itr == _currentThreadToCtxMap.end()) {
    return nullptr;
  }
  return itr->second;
  // return _currentThreadToCtxMap.at(CGits::Instance().CurrentThreadId());
}

void* CStateDynamic::GetContextFromThread(int thread) {
  if (_currentThreadToCtxMap.find(thread) == _currentThreadToCtxMap.end()) {
    return nullptr;
  } else {
    return _currentThreadToCtxMap.at(thread);
  }
}

std::vector<void*> CStateDynamic::GetContextVector() const {
  std::vector<void*> contexts;

  for (auto& elem : _contextMap) {
    contexts.push_back(elem.first);
  }

  return contexts;
}

CSharedStateDynamic& CStateDynamic::GetCurrentSharedStateData() {
  // RECORDER
  UpdateContextData();
  if (_contextStateData == nullptr) {
    return CSharedStateDynamic::GetStatic(); // Returns trash state dynamic.
  } else {
    if (!CGits::Instance().IsStateRestoration()) {
      _contextStateData->sharedStateDynamic.get()->_owner = _currentContext;
    }
    return (*_contextStateData->sharedStateDynamic);
  }
}

CContextStateDynamic& CStateDynamic::GetCurrentContextStateData() {
  // RECORDER
  UpdateContextData();
  if (_contextStateData == nullptr) {
    return CContextStateDynamic::GetStatic(); // Returns trash state dynamic.
  } else {
    return (*_contextStateData->contextStateDynamic);
  }
}

// Shared data owner is each context which is not sharing and only one context
// in each shared group
bool CStateDynamic::IsCurrentContextSharedDataOwner() {
  UpdateContextData();
  if (_contextStateData == nullptr) {
    throw ENotFound(EXCEPTION_MESSAGE);
  }

  return (_contextStateData->sharedStateDynamic.get()->_owner == _currentContext);
}

// True means that the current context uses core profile; false means compatibility profile.
// These conditions are valid only for mapping feature. Impact of forward
// compatibility bit is also considered if any.
bool CStateDynamic::IsCurrentContextCore() {
  // Get current context data
  UpdateContextData();
  if (_contextStateData == nullptr) {
    throw ENotFound(EXCEPTION_MESSAGE);
  }

  // Check if core parameter checked for current context already
  if (_contextMap[_currentContext].coreContext >= 0) {
    return (_contextMap[_currentContext].coreContext > 0);
  }

  // Check core context
  _contextMap.at(_currentContext).coreContext = 0;
  auto version = SD().GetCurrentContextStateData().Version();
  if (version <= 300) {
    // OpenGL 3.0 and below versions behave same as comaptibility profile in
    // OpenGL 3.2 onwards i.e. buffer, texture and query id can be used even if
    // it is not generated using a call to glGen*, here mapping of ids is not
    // required.
    _contextMap.at(_currentContext).coreContext = 0;
  } else {
    if (version == 310) {
      // OpenGL 3.1 version behaves same as core profile in OpenGL 3.2 onwards
      // i.e. buffer, texture and query id must be generated using a call to
      // glGen*, here mapping of ids is required.
      _contextMap.at(_currentContext).coreContext = 1;
    } else {
      // OpenGL 3.2 onwards
      GLint profile = SD().GetCurrentContextStateData().ProfileMask();
      if (profile & GL_CONTEXT_CORE_PROFILE_BIT) {
        // OpenGL 3.2 onwards core profile, mapping of ids is required.
        _contextMap.at(_currentContext).coreContext = 1;
      }
    }
  }
  return (_contextMap[_currentContext].coreContext > 0);
}

unsigned CStateDynamic::GetCurrentContextSharedGroupId() {
  UpdateContextData();
  if (_currentContext == nullptr) {
    throw ENotFound(EXCEPTION_MESSAGE);
  }

  if (_contextToSharedGroup != _ctxToSharedGroupIdMap.end()) {
    return _contextToSharedGroup->second;
  } else {
    throw ENotFound(EXCEPTION_MESSAGE);
  }
}

int CStateDynamic::GetCurrentContextOrdinal() {
  auto ctx = GetContextStateData(GetCurrentContext());
  return ctx == nullptr ? 0 : ctx->contextOrdinal;
}

std::map<EGLConfig, std::map<EGLint, EGLint>>& CStateDynamic::GetEglConfigs() {
  return _eglConfigs;
}

namespace {
void GetCurrFboAttachments(std::set<GLint>& textures, std::set<GLint>& rbos) {
  GLint boundFBO = 0;
  GLint maxFBOAttach = 0;
  drv.gl.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxFBOAttach);
  drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundFBO);

  GLenum attachmentType;
  GLint attachmentName;
  for (int i = 0; i < maxFBOAttach; i++) {
    drv.gl.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                                 (GLint*)&attachmentType);
    drv.gl.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                                 &attachmentName);
    if (attachmentType == GL_RENDERBUFFER) {
      rbos.insert(attachmentName);
    } else if (attachmentType == GL_TEXTURE) {
      textures.insert(attachmentName);
    }
  }
  GLenum attatchments[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
  for (auto att : attatchments) {
    drv.gl.glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, att, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, (GLint*)&attachmentType);
    drv.gl.glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, att, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachmentName);
    if (attachmentType == GL_RENDERBUFFER) {
      rbos.insert(attachmentName);
    } else if (attachmentType == GL_TEXTURE) {
      textures.insert(attachmentName);
    }
  }
}
} // namespace

void CStateDynamic::EraseNonCurrentData() {
  // Clean up non current contexts state
  void* currCtx = GetCurrentContext();
  for (auto iter = _contextMap.begin(); iter != _contextMap.end();) {
    if (currCtx != iter->first) {
      auto iterRemove = iter;
      iter++;
      RemoveContext(iterRemove->first);
    } else {
      iter++;
    }
  }

  // Clean up non bound state of current context
  for (auto& elem : _contextMap) {
    CContextStateData& stateData = elem.second;

    // Get State Data
    GLint maxTexUnits = 0;
    GLint activeTexUnit = 0;
    drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexUnit);
    drv.gl.glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);

    GLint boundVertProg = 0;
    GLint boundFragProg = 0;
    drv.gl.glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &boundVertProg);
    drv.gl.glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &boundFragProg);

    std::set<GLint> fboTexs;
    std::set<GLint> fboRbos;
    GetCurrFboAttachments(fboTexs, fboRbos);

    // Generic check binding function
    auto CheckBindingGeneric = [](GLenum bindEnum, GLint name) -> bool {
      GLint boundName = 0;
      drv.gl.glGetIntegerv(bindEnum, &boundName);
      return (boundName == name);
    };

    // Always fail check binding function - returning always fail, used to
    // remove all objects
    auto AlwaysFalse = [](GLenum bindEnum, GLint name) -> bool { return false; };

    // Specialized ARB programs check binding function
    auto CheckBindingARBProg = [=](GLenum bindEnum, GLint name) -> bool {
      return ((name == boundVertProg) || (name == boundFragProg));
    };

    // Specialized renderbuffers check binding function
    auto CheckBindingRenderbuffer = [&fboRbos](GLenum bindEnum, GLint name) -> bool {
      return (fboRbos.find(name) != fboRbos.end());
    };

    // Specialized textures and sampler check binding function
    auto CheckBindingForAllTexUnits = [&fboTexs, &maxTexUnits, &activeTexUnit](GLenum bindEnum,
                                                                               GLint name) -> bool {
      GLint boundName = 0;
      if (fboTexs.find(name) != fboTexs.end()) {
        return true;
      }

      for (int i = 0; i < maxTexUnits; i++) {
        drv.gl.glActiveTexture(GL_TEXTURE0 + i);
        drv.gl.glGetIntegerv(bindEnum, &boundName);
        if (boundName == name) {
          break;
        }
      }
      drv.gl.glActiveTexture(activeTexUnit);
      return (boundName == name);
    };

    // Specialized array buffers check binding function
    std::set<GLint> boundArrayBuffers;
    GLint maxVertexAttribs = 0;
    drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    for (GLint i = 0; i < maxVertexAttribs; i++) {
      GLint buff;
      drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buff);
      if (buff != 0) {
        boundArrayBuffers.insert(buff);
      }
    }
    // Specialized uniform buffers check binding function
    std::set<GLint> boundUniformBuffers;
    GLint maxUniformBufferBindings = 0;
    drv.gl.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
    for (GLint i = 0; i < maxUniformBufferBindings; i++) {
      GLint buff;
      drv.gl.glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, i, &buff);
      if (buff != 0) {
        boundUniformBuffers.insert(buff);
      }
    }
    // Specialized shader storage buffers check binding function
    std::set<GLint> boundShaderStorageBuffers;
    GLint maxShaderStorageBindings = 0;
    drv.gl.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBindings);
    for (GLint i = 0; i < maxShaderStorageBindings; i++) {
      GLint buff;
      drv.gl.glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, i, &buff);
      if (buff != 0) {
        boundShaderStorageBuffers.insert(buff);
      }
    }
    auto CheckBindingForBuffers = [&boundArrayBuffers, &boundUniformBuffers,
                                   &boundShaderStorageBuffers](GLenum bindEnum,
                                                               GLint name) -> bool {
      if (bindEnum == GL_ARRAY_BUFFER_BINDING &&
          boundArrayBuffers.find(name) != boundArrayBuffers.end()) {
        return true;
      }
      if (bindEnum == GL_UNIFORM_BUFFER_BINDING &&
          boundUniformBuffers.find(name) != boundUniformBuffers.end()) {
        return true;
      }
      if (bindEnum == GL_SHADER_STORAGE_BUFFER_BINDING &&
          boundShaderStorageBuffers.find(name) != boundShaderStorageBuffers.end()) {
        return true;
      }
      GLint boundName = 0;
      drv.gl.glGetIntegerv(bindEnum, &boundName);
      return (boundName == name);
    };

    // State data filtering
    stateData.contextStateDynamic->Framebuffers().EraseIfNot(CheckBindingGeneric);
    stateData.contextStateDynamic->FramebuffersEXT().EraseIfNot(CheckBindingGeneric);
    stateData.contextStateDynamic->VertexArrays().EraseIfNot(CheckBindingGeneric);
    stateData.sharedStateDynamic->ARBPrograms().EraseIfNot(CheckBindingARBProg);
    stateData.sharedStateDynamic->GLSLShaders().EraseIfNot(AlwaysFalse);
    stateData.sharedStateDynamic->GLSLPrograms().EraseIfNot(CheckBindingGeneric);
    stateData.sharedStateDynamic->Renderbuffers().EraseIfNot(CheckBindingRenderbuffer);
    stateData.sharedStateDynamic->RenderbuffersEXT().EraseIfNot(CheckBindingRenderbuffer);
    stateData.sharedStateDynamic->Buffers().EraseIfNot(CheckBindingForBuffers);
    stateData.sharedStateDynamic->Samplers().EraseIfNot(CheckBindingForAllTexUnits);
    stateData.sharedStateDynamic->Textures().EraseIfNot(CheckBindingForAllTexUnits);
  }
}

void CStateDynamic::UpdateContextData() {
  if ((std::this_thread::get_id() != _currentThread) || _currentContextUpdateNeeded) {
    _currentContext = GetCurrentContext();
    _contextStateData = GetContextStateData(_currentContext);
    _contextToSharedGroup = _ctxToSharedGroupIdMap.find(_currentContext);
    _currentThread = std::this_thread::get_id();
    _currentContextUpdateNeeded = false;
  }
}

//----------------------------NATIVE STATE DYNAMIC-----------------------------
/*!!!This code should be removed from stateDynamic and implemented in
 * windowContextState!!!*/
CStateDynamicNative& CStateDynamicNative::Get() {
  static CStateDynamicNative ptr = CStateDynamicNative();
  return ptr;
}

CStateDynamicNative::CStateDynamicNative() {}

CStateDynamicNative::~CStateDynamicNative() {
  for (auto& window : _winMapPlay) {
    delete window.second;
  }
}

//--------------------------NATIVE STATE DYNAMIC WGL-----------------------
bool CStateDynamicNative::MapFindWindowRecorder(void* win, std::vector<int>& winparams) {
  CWinMapRec::iterator iter = _winMapRec.find(win);
  if (iter != _winMapRec.end()) {
    winparams = iter->second;
    return true;
  } else {
    return false;
  }
}

bool CStateDynamicNative::MapFindWindowRecorder(void* win) {
  CWinMapRec::iterator iter = _winMapRec.find(win);
  if (iter != _winMapRec.end()) {
    return true;
  } else {
    return false;
  }
}

void CStateDynamicNative::UnMapHglrcHdcByHwnd(void* hwnd) {
#ifdef GITS_PLATFORM_WINDOWS
  CHglrcToHdc::iterator iter = _hglrcToHdc.begin();
  while (iter != _hglrcToHdc.end()) {
    if (WindowFromDC((HDC)iter->second) == hwnd) {
      CHglrcToHdc::iterator iterToDel = iter;
      ++iter;
      _hglrcToHdc.erase(iterToDel);
    } else {
      ++iter;
    }
  }
#endif
}

void* CStateDynamicNative::GetHglrcFromHdc(void* hdc) {
  for (const auto& elem : _hglrcToHdc) {
    if (elem.second == hdc) {
      return elem.first;
    }
  }
  return nullptr;
}

void* CStateDynamicNative::GetHglrcFromHwnd(void* hwnd) {
#ifdef GITS_PLATFORM_WINDOWS
  for (const auto& elem : _hglrcToHdc) {
    if (WindowFromDC((HDC)elem.second) == hwnd) {
      return elem.first;
    }
  }
#endif
  return nullptr;
}

void CStateDynamicNative::ReMapHdc(void* hdcOld, void* hdcNew) {
  for (auto& elem : _hglrcToHdc) {
    if (elem.second == hdcOld) {
      elem.second = hdcNew;
    }
  }
}

void CStateDynamicNative::MapGetWinHandlesListRecorder(std::vector<void*>& winList) {
  for (auto& elem : _winMapRec) {
    winList.push_back(elem.first);
  }
}

Window_* CStateDynamicNative::MapFindWindowPlayer(win_handle_t winhandle) {
  CWinMapPlay::iterator iter = _winMapPlay.find(winhandle);
  if (iter != _winMapPlay.end()) {
    return iter->second;
  } else {
    Log(ERR) << "Unknown window with handle: " << winhandle;
    throw ENotFound(EXCEPTION_MESSAGE);
  }
}

void CStateDynamicNative::MapDeleteWindowPlayer(win_handle_t winhandle) {
  delete _winMapPlay[winhandle];
  _winMapPlay.erase(winhandle);
}

//--------------------------NATIVE STATE DYNAMIC EGL-----------------------
void CStateDynamicNative::MapEglCtxToSurf(void* context, void* surfacedraw, void* surfaceread) {
  CDrawableEgl drawableEgl;
  drawableEgl.eglDrawSurface = surfacedraw;
  drawableEgl.eglReadSurface = surfaceread;
  _eglCtxToDrawable[context] = drawableEgl;
}

void CStateDynamicNative::UnMapEglCtxToDispSurfBySurface(void* surface) {
  CMapEglCtxToDrawable::iterator iter = _eglCtxToDrawable.begin();
  while (iter != _eglCtxToDrawable.end()) {
    if (iter->second.eglDrawSurface == surface || iter->second.eglReadSurface == surface) {
      CMapEglCtxToDrawable::iterator iterToDel = iter;
      ++iter;
      _eglCtxToDrawable.erase(iterToDel);
    } else {
      ++iter;
    }
  }
}

//---------------------TEXTURE STATE OBJECT-----------------------------------------------
CTextureStateObj& TextureStateObject(GLenum target, GLint texture) {
  if (texture == -1) {
    if (isTrackTextureBindingWAUsed()) {
      auto unit =
          SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture;
      texture = SD().GetCurrentContextStateData()
                    .GeneralStateObjects()
                    .Data()
                    .tracked.boundTextures[unit][target];
    } else {
      drv.gl.glGetIntegerv(GetBindingEnum(target), &texture);
    }
  }

  if (GL_TEXTURE_CUBE_MAP_POSITIVE_X <= target && GL_TEXTURE_CUBE_MAP_NEGATIVE_Z >= target) {
    target = GL_TEXTURE_CUBE_MAP;
  }

  CTextureStateObj* textureStateObj =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(texture, target));

  // Add texture-target if not already present (e.g. zero texture or in compatibility profile gen not called)
  if (textureStateObj == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  } else {
    return *textureStateObj;
  }
}

//---------------------GEN MIPMAP State Data----------------------------------------------
#ifndef BUILD_FOR_CCODE
void GenMipMapStateData(GLint texture, GLenum target, GLint levels) {
  GLint bound_texture;
  if (isTrackTextureBindingWAUsed()) {
    auto unit =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture;
    bound_texture = SD().GetCurrentContextStateData()
                        .GeneralStateObjects()
                        .Data()
                        .tracked.boundTextures[unit][target];
  } else {
    drv.gl.glGetIntegerv(GetBindingEnum(target), &bound_texture);
  }

  if (bound_texture != texture) {
    drv.gl.glBindTexture(target, texture);
  }

  // Get mipmap base level data reference
  auto& mipmapData = TextureStateObject(target, texture).Data().track.mipmapData;
  if (mipmapData.size() == 0) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  CTextureStateData::TMipmapTextureData baseLevel = mipmapData[0];

  // Evaluate number of levels.
  GLint longest; // longest dimension
  if (target == GL_TEXTURE_1D_ARRAY) {
    longest = baseLevel.width;
  } else {
    longest = std::max(baseLevel.width, baseLevel.height);
    if (target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP_ARRAY) {
      longest = std::max(longest, baseLevel.depth);
    }
  }
  GLint maxPossibleLevels = log2i(longest) + 1;
  if ((curctx::IsOgl() && curctx::Version() >= 420) || curctx::IsEs3Plus()) {
    // Immutable textures are available.
    GLint isImmutable = GL_FALSE;
    drv.gl.glGetTexParameteriv(target, GL_TEXTURE_IMMUTABLE_FORMAT, &isImmutable);
    if (isImmutable) {
      if (levels == -1) {
        levels = (GLint)mipmapData.size();
      }
    } else {
      levels = maxPossibleLevels;
    }
  } else {
    // Immutable textures are not available.
    levels = maxPossibleLevels;
  }

  // Evaluate mipmap sizes.
  for (int level = 1; level < levels; level++) {
    if (mipmapData.size() < (size_t)(level + 1)) {
      mipmapData.push_back(CTextureStateData::TMipmapTextureData());
    }
    mipmapData[level] = mipmapData[level - 1];
    mipmapData[level].width = std::max(mipmapData[level].width / 2, 1);
    if (target != GL_TEXTURE_1D_ARRAY) {
      mipmapData[level].height = std::max(mipmapData[level].height / 2, 1);
    }
    if (target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_CUBE_MAP_ARRAY) {
      mipmapData[level].depth = std::max(mipmapData[level].depth / 2, 1);
    }
    mipmapData[level].compressed = baseLevel.compressed;
    if (mipmapData[level].compressed) {
      if (target == GL_TEXTURE_CUBE_MAP) {
        drv.gl.glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level,
                                        GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
                                        &mipmapData[level].compressedImageSize);
      } else {
        drv.gl.glGetTexLevelParameteriv(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
                                        &mipmapData[level].compressedImageSize);
      }
    }
  }
  if (bound_texture != texture) {
    drv.gl.glBindTexture(target, bound_texture);
  }
}

void GenMipMapStateData(GLenum target, GLint levels) {
  GLint texture;
  if (isTrackTextureBindingWAUsed()) {
    auto unit =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture;
    texture = SD().GetCurrentContextStateData()
                  .GeneralStateObjects()
                  .Data()
                  .tracked.boundTextures[unit][target];
  } else {
    drv.gl.glGetIntegerv(GetBindingEnum(target), &texture);
  }
  GenMipMapStateData(texture, target, levels);
}
#endif

//--------------------SET TARGET FOR TEXTURE--------------------------------
void SetTargetForTexture(GLenum target, GLuint texture) {
  CTextureStateObj* textureStateObj =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(texture, target));

  // Add texture-target if not already present (e.g. zero texture or in
  // compatibility profile gen not called)
  if (textureStateObj == nullptr) {
    SD().GetCurrentSharedStateData().Textures().Add(CTextureStateObj(texture, target));
  } else {
    // Only non-zero texture require explicit target setting (setting target for
    // 0 texture is incorrect as every target has individual 0 texture)
    if ((texture != 0) && (textureStateObj->Target() == 0)) {
      textureStateObj->SetTarget(target);
    }
  }
}

//--------------------------NATIVE STATE DYNAMIC GLX-----------------------

void CStateDynamicNative::MapglXCtxToSurf(void* context,
                                          long unsigned int surfacedraw,
                                          long unsigned int surfaceread) {
  CDrawableglX drawableglX;
  drawableglX.glXDrawSurface = surfacedraw;
  drawableglX.glXReadSurface = surfaceread;
  _glXCtxToDrawable[context] = drawableglX;
}

void CStateDynamicNative::UnMapglXCtxToDispSurfBySurface(long unsigned int surface) {
  CMapglXCtxToDrawable::iterator iter = _glXCtxToDrawable.begin();
  while (iter != _glXCtxToDrawable.end()) {
    if (iter->second.glXDrawSurface == surface || iter->second.glXReadSurface == surface) {
      CMapglXCtxToDrawable::iterator iterToDel = iter;
      ++iter;
      _glXCtxToDrawable.erase(iterToDel);
    } else {
      ++iter;
    }
  }
}

#ifndef BUILD_FOR_CCODE
bool curctx::IsOgl() {
  return SD().GetCurrentContextStateData().IsOgl();
}
bool curctx::IsOgl11() {
  return (SD().GetCurrentContextStateData().IsOgl() &&
          SD().GetCurrentContextStateData().Version() == 110);
}
bool curctx::IsEs() {
  return SD().GetCurrentContextStateData().IsEs();
}
bool curctx::IsEs1() {
  return (SD().GetCurrentContextStateData().IsEs() &&
          SD().GetCurrentContextStateData().Version() == 110);
}
bool curctx::IsEs2Plus() {
  return (SD().GetCurrentContextStateData().IsEs() &&
          SD().GetCurrentContextStateData().Version() >= 200);
}
bool curctx::IsEs3Plus() {
  return (SD().GetCurrentContextStateData().IsEs() &&
          SD().GetCurrentContextStateData().Version() >= 300);
}
bool curctx::IsEs31Plus() {
  return (SD().GetCurrentContextStateData().IsEs() &&
          SD().GetCurrentContextStateData().Version() >= 310);
}
int curctx::Version() {
  return SD().GetCurrentContextStateData().Version();
}
bool curctx::IsNvidia() {
  return SD().GetCurrentContextStateData().IsNvidia() == 1;
}
#endif
} // namespace OpenGL
} // namespace gits
