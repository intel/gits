// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   stateDynamic.h
 *
 * @brief Declaration of OpenGL library implementation.
 *
 */
#pragma once

#include "version.h"
#include "tools.h"
#include "gits.h"
#include "stateObjects.h"
#include "openglCommon.h"
#include "openglEnums.h"
#include "openglDrivers.h"
#include "message_pump.h"

#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Window_;

namespace gits {
/**
   * @brief OpenGL library specific GITS namespace
   */

namespace OpenGL {

/**
    *
    * CObjectsList - generic container class
    *
    */
template <typename Object>
class CObjectsList {
public:
  typedef std::set<Object> CListType;

  CObjectsList() {}
  ~CObjectsList() {}

  void Generate(const GLuint number, const GLuint* names) {
    for (unsigned i = 0; i < number; i++) {
      _list.insert(Object(*names));
      names++;
    }
  }

  void Add(Object object) {
    _list.insert(object);
  }

  void Remove(const GLuint number, const GLuint* names) {
    for (unsigned i = 0; i < number; i++) {
      if (_list.find(Object(*names)) != _list.end()) {
        _list.erase(Object(*names));
      }
      names++;
    }
  }

  Object* Get(GLuint name) {
    typename CListType::iterator iter = _list.find(Object(name));
    if (iter != _list.end()) {
      return GCC433WA_0(&*iter);
    } else {
      return nullptr;
    }
  }

  Object* Get(Object object) {
    typename CListType::iterator iter = _list.find(object);
    if (iter != _list.end()) {
      return GCC433WA_0(&*iter);
    } else {
      return nullptr;
    }
  }

  CListType& List() {
    return _list;
  }

  template <typename Function>
  void EraseIfNot(
      Function&
          fnc) { //Used by recordDrawCall option to remove objects not tied with recorded drawcall
    for (auto iter = _list.begin(); iter != _list.end();) {
      if (iter->Name() == 0 || iter->LastBindEnum() == 0 ||
          !fnc(iter->LastBindEnum(), iter->Name())) {
        _list.erase(iter++);
      } else {
        iter++;
      }
    }
  }

private:
  CListType _list;
};

class CSchedulerState;

/**
    *
    * CSharedStateDynamic - shared group state object - holds state of shared objects
    *
    */
class CSharedStateDynamic {
public:
  CSharedStateDynamic();
  ~CSharedStateDynamic();

  friend class CStateDynamic;

  bool coherentBufferMapping;
  std::unordered_set<GLint> coherentBufferUpdatedSet;
  GLuint coherentBufferFrameNumber;

  // Generic container type definitions
  typedef CObjectsList<CRenderbufferStateObj> CRenderbuffers;
  typedef CObjectsList<CRenderbufferStateObj> CRenderbuffersEXT;
  typedef CObjectsList<CTextureStateObj> CTextures;
  typedef CObjectsList<CBufferStateObj> CBuffers;
  typedef CObjectsList<CSamplerStateObj> CSamplers;
  typedef CObjectsList<CARBProgramStateObj> CARBPrograms;
  typedef CObjectsList<CARBProgramEnvParamsStateObj> CARBProgEnvParams;
  typedef CObjectsList<CGLSLShaderStateObj> CGLSLShaders;
  typedef CObjectsList<CGLSLProgramStateObj> CGLSLPrograms;

  // Non generic container type definitions
  typedef std::unordered_map<GLuint, std::vector<char>> CBufferDataMap;

  // Generic container accessors
  CRenderbuffers& Renderbuffers() {
    return _renderbuffers;
  }
  CRenderbuffers& RenderbuffersEXT() {
    return _renderbuffersEXT;
  }
  CTextures& Textures() {
    return _textures;
  }
  CIndexedBindingRangeStateObj& IndexedBoundBuffers() {
    return _indexedBindingRangeInfo;
  }
  CBuffers& Buffers() {
    return _buffers;
  }
  CSamplers& Samplers() {
    return _samplers;
  }
  CARBPrograms& ARBPrograms() {
    return _arbPrograms;
  }
  CARBProgEnvParams& ARBProgEnvParams() {
    return _arbProgEnvParams;
  }
  CGLSLShaders& GLSLShaders() {
    return _glslShaders;
  }
  CGLSLPrograms& GLSLPrograms() {
    return _glslPrograms;
  }

  // Non generic container accessors
  CMappedTexturesStateObj& GetMappedTextures() {
    return _mappedTextures;
  } // this function gives access to map containing mapped textures

private:
  // Generic container declarations
  CRenderbuffers _renderbuffers;
  CRenderbuffers _renderbuffersEXT;
  CTextures _textures;
  CIndexedBindingRangeStateObj _indexedBindingRangeInfo;
  CBuffers _buffers;
  CSamplers _samplers;
  CARBPrograms _arbPrograms;
  CARBProgEnvParams _arbProgEnvParams;
  CGLSLShaders _glslShaders;
  CGLSLPrograms _glslPrograms;

  // Non generic container declarations
  // this maps mapped textures from intel extentions to texture instances
  CMappedTexturesStateObj _mappedTextures;

  void* _owner; // Specifies owner context

  static CSharedStateDynamic&
  GetStatic(); // Returns global static instance of CSharedStateDynamic which is
               // not tracked per share group/context It is used in player and as
               // a trash for calls called on 0 on unknown context in recoreder
};

/**
    *
    * CContextStateDynamic - per context state object - holds state of non shared objects
    *
    */
class CContextStateDynamic {
public:
  CContextStateDynamic();
  ~CContextStateDynamic();

  friend class CStateDynamic;

  // Workaround for pushClientAttributes in Worms Ultimate Mayhem
  bool pushUsed;

  bool glBeginState;

  // Variable used for PrimitiveRestartIndex value
  // It is initialized with 4294967295 (value of -1 for uint)
  GLuint restartIndexValue;

  // Generic container type definitions
  typedef CObjectsList<CFramebufferStateObj> CFramebuffers;
  typedef CObjectsList<CFramebufferStateObj> CFramebuffersEXT;
  typedef CObjectsList<CVertexArraysStateObj> CVertexArrays;
  typedef CObjectsList<CGLSLPipelineStateObj> CGLSLPipelines;

  // Generic container accessors
  CFramebuffers& Framebuffers() {
    return _framebuffers;
  }
  CFramebuffers& FramebuffersEXT() {
    return _framebuffersEXT;
  }
  CVertexArrays& VertexArrays() {
    return _vertexArrays;
  }
  CGLSLPipelines& GLSLPipelines() {
    return _glslPipelines;
  }
  CGeneralStateObj& GeneralStateObjects() {
    return _generalStateObjects;
  }
  CBindingStateObj& Bindings() {
    return _bindings;
  }
  CClientArraysStateObj& ClientArrays() {
    return _clientArrays;
  }
  CBlendFunciObj& BlendFunci() {
    return _blendFunci;
  }
  CBlendEquationiObj& BlendEquationi() {
    return _blendEquationi;
  }

  CSchedulerState* GetSchedulerStatePtr() {
    return _schedulerStatePtr;
  }

  GLint ProfileMask();
  int Version();
  bool IsOgl() {
    return !IsEs();
  }
  bool IsEs();
  bool IsNvidia();

private:
  // Generic container declarations
  CBindingStateObj _bindings;
  CFramebuffers _framebuffers;
  CFramebuffers _framebuffersEXT;
  CVertexArrays _vertexArrays;
  CGLSLPipelines _glslPipelines;
  CGeneralStateObj _generalStateObjects;
  CClientArraysStateObj _clientArrays;
  CBlendFunciObj _blendFunci;
  CBlendEquationiObj _blendEquationi;
  int _version;
  int _isEs;
  int _isNvidia;
  GLint _profileMask;

  static CContextStateDynamic&
  GetStatic(); // Returns global static instance of CSharedStateDynamic which is
               // not tracked per share group/context It is used in player and as
               // a trash for calls called on 0 on unknown context in recoreder

  friend class CSchedulerState;
  CSchedulerState* _schedulerStatePtr;
};

/**
    *
    * CStateDynamic holds all contexts state data
    *
    */
class CStateDynamic {
private:
  struct CContextStateData {
    int coreContext; // "coreContext<0" - core context not checked /
                     // "coreContext==0" - not a core context / "coreContext>0"
                     // - core context
    std::shared_ptr<CContextStateDynamic> contextStateDynamic;
    std::shared_ptr<CSharedStateDynamic> sharedStateDynamic;
    int contextOrdinal;
  };

  typedef std::unordered_map<void*, CContextStateData> CContextMap;
  CContextMap _contextMap;

  CContextStateData* GetContextStateData(void* context);

  typedef std::unordered_map<void*, unsigned> CCtxToSharedGroupIdMap;
  CCtxToSharedGroupIdMap _ctxToSharedGroupIdMap;

  typedef std::unordered_map<int, void*> CThreadToCtxMap;
  CThreadToCtxMap _currentThreadToCtxMap;

  bool _currentContextUpdateNeeded;
  std::thread::id _currentThread;
  CContextStateData* _contextStateData;
  void* _currentContext;
  CCtxToSharedGroupIdMap::iterator _contextToSharedGroup;
  std::map<EGLConfig, std::map<EGLint, EGLint>> _eglConfigs;

  CStateDynamic();

public:
  ~CStateDynamic();

  // Memory tracking
  typedef std::vector<char> TMemory;
  typedef std::unordered_map<uint64_t, TMemory> TMemoryAreas;
  TMemoryAreas _memTracker;
  void WriteClientSizes();
  std::function<void()> CreateCArraysRestorePoint();

  // GetTexture Data
  std::vector<GLchar> _getTextureData;

  static CStateDynamic& Get();

  void AddContext(void* context, void* shared);
  void RemoveContext(void* context);
  void ShareContext(void* context1, void* context2);
  void UpdateContextData();

  void SetCurrentContext(void* context);
  void* GetCurrentContext() const;

  void* GetContextFromThread(int thread);

  std::vector<void*> GetContextVector() const;
  CSharedStateDynamic& GetCurrentSharedStateData();
  CContextStateDynamic& GetCurrentContextStateData();

  bool IsCurrentContextSharedDataOwner();
  bool IsCurrentContextCore();

  unsigned GetCurrentContextSharedGroupId();
  int GetCurrentContextOrdinal();

  std::map<EGLConfig, std::map<EGLint, EGLint>>& GetEglConfigs();

  void EraseNonCurrentData(); // Used by recordDrawCall option to remove all state
                              // data not tied with currenttly recorded drawcall
};

inline CStateDynamic& SD() {
  return CStateDynamic::Get();
}

/**
    *
    * Scheduler State        - allows to track some temporary state data concerning currently scheduled objects in openglState.cpp (not an actual OpenGL state). This data may be used by certain tokens/arguments.
    *                          CSchedulerState objects should be declared as local function variables in state restore and accessed through the StateDynamic by subsequently scheduled tokens/arguments.
    *                          CSchedulerState registers itself in StateDynamic on construction and de-registers on destruction. Only one restore feedback allowed at the time.
    *
    **/
class CSchedulerState {
  std::map<GLenum, GLuint> _params;

public:
  CSchedulerState() {
    if (SD().GetCurrentContextStateData()._schedulerStatePtr != 0) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    SD().GetCurrentContextStateData()._schedulerStatePtr = this;
  }
  ~CSchedulerState() {
    SD().GetCurrentContextStateData()._schedulerStatePtr = 0;
  }
  void SaveGLParamui(GLenum param, GLuint val) {
    _params[param] = val;
  }
  GLuint GetGLParamui(GLenum param) {
    return (_params.find(param) == _params.end()) ? throw std::runtime_error(EXCEPTION_MESSAGE)
                                                  : _params[param];
  }
};

/**
    *
    * CStateDynamicNative - native wgl glx egl state
    *                   !!!Native code should be removed from stateDynamic and implemented in windowContextState!!!
    */
class CStateDynamicNative {
  // WGL
private:
  typedef std::unordered_map<void*, std::vector<int>> CWinMapRec;
  CWinMapRec _winMapRec;
  typedef std::unordered_map<win_handle_t, Window_*> CWinMapPlay;
  CWinMapPlay _winMapPlay;
  typedef std::unordered_map<void*, void*> CHwndToHdc;
  CHwndToHdc _hwndToHdc;
  typedef std::unordered_map<void*, void*> CHglrcToHdc;
  CHglrcToHdc _hglrcToHdc;

  typedef std::unordered_map<void*, int> CHwndToThread;
  CHwndToThread _hwndToThread;

public:
  CStateDynamicNative();
  ~CStateDynamicNative();
  static CStateDynamicNative& Get();

  // these functions give access to window info in recorder
  void MapAddWindowRecorder(void* win, std::vector<int>& winparams) {
    if (_winMapRec.find(win) == _winMapRec.end()) {
      _hwndToThread[win] = CGits::Instance().CurrentThreadId();
      _winMapRec[win] = winparams;
    }
  }
  void MapUpdateWindowRecorder(void* win, std::vector<int>& winparams) {
    _winMapRec[win] = winparams;
  }
  bool MapFindWindowRecorder(void* win, std::vector<int>& winparams);
  bool MapFindWindowRecorder(void* win);
  void MapDeleteWindowRecorder(void* win) {
    _winMapRec.erase(win);
  }
  void MapGetWinHandlesListRecorder(std::vector<void*>& winList);

  int GetWindowThread(void* win) {
    if (_winMapRec.find(win) != _winMapRec.end()) {
      return _hwndToThread[win];
    } else {
      return 0;
    }
  }

  // these functions give access to window object in player
  void MapAddWindowPlayer(win_handle_t winhandle, Window_* window) {
    _winMapPlay[winhandle] = window;
  }
  Window_* MapFindWindowPlayer(win_handle_t winhandle);
  void MapDeleteWindowPlayer(win_handle_t winhandle);
  void MapAddHDC(void* hwnd, void* hdc) {
    _hwndToHdc[hwnd] = hdc;
  }
  void MapDelHWND(void* hwnd) {
    _hwndToHdc.erase(hwnd);
  }
  void* MapGetHDCs(void* hwnd) {
    return _hwndToHdc[hwnd];
  }

  // these functions give access to window object in player and recorder
  void MapHglrcToHdc(void* context, void* hdc) {
    _hglrcToHdc[context] = hdc;
  }
  void ReMapHdc(void* hdcOld, void* hdcNew);
  void UnMapHglrcHdcByHglrc(void* context) {
    _hglrcToHdc.erase(context);
  }
  void UnMapHglrcHdcByHwnd(void* hwnd);
  void* GetHdcFromHglrc(void* context) {
    return (_hglrcToHdc.find(context) != _hglrcToHdc.end()) ? _hglrcToHdc[context] : 0;
  }
  void* GetHglrcFromHdc(void* hdc);
  void* GetHglrcFromHwnd(void* hwnd);

  // EGL
private:
  struct CDrawableEgl {
    void* eglDrawSurface;
    void* eglReadSurface;
  };
  typedef std::map<void*, CDrawableEgl> CMapEglCtxToDrawable;
  typedef std::map<void*, void*> CMapCtxToDisplay;
  CMapEglCtxToDrawable _eglCtxToDrawable;
  CMapCtxToDisplay _eglCtxToDisplay;

public:
  void MapEglCtxToSurf(void* context, void* surfacedraw, void* surfaceread);
  void UnMapEglCtxToDispSurfByCtx(void* context) {
    _eglCtxToDrawable.erase(context);
  }
  void UnMapEglCtxToDispSurfBySurface(void* surface);
  void* GetEglDrawSurfaceFromContext(void* context) {
    return (_eglCtxToDrawable.find(context) != _eglCtxToDrawable.end())
               ? _eglCtxToDrawable[context].eglDrawSurface
               : 0;
  }
  void* GetEglReadSurfaceFromContext(void* context) {
    return (_eglCtxToDrawable.find(context) != _eglCtxToDrawable.end())
               ? _eglCtxToDrawable[context].eglReadSurface
               : 0;
  }
  void MapCtxToDisplay(void* ctx, void* display) {
    _eglCtxToDisplay[ctx] = display;
  }
  void* GetDisplayFromContext(void* ctx) {
    return (_eglCtxToDisplay.find(ctx) != _eglCtxToDisplay.end()) ? _eglCtxToDisplay[ctx] : 0;
  }

  // GLX
public:
  class CContextVisualInfo {
    unsigned mode_;
    short depth_;

  public:
    CContextVisualInfo() : mode_(0), depth_(0) {}
    CContextVisualInfo(unsigned mode, short depth) : mode_(mode), depth_(depth) {}
    unsigned mode() const {
      return mode_;
    }
    short depth() const {
      return depth_;
    }
  };

private:
  typedef std::map<GLXContext, CContextVisualInfo, std::less<void*>> CContextVisualInfoMap;
  CContextVisualInfoMap _ctxVisualInfoMap;

  struct CDrawableglX {
    long unsigned int glXDrawSurface;
    long unsigned int glXReadSurface;
  };
  typedef std::map<void*, CDrawableglX> CMapglXCtxToDrawable;
  CMapglXCtxToDrawable _glXCtxToDrawable;

public:
  const CContextVisualInfo& ContextVisualInfo(GLXContext ctx) const {
    const auto search = _ctxVisualInfoMap.find(ctx);
    if (search != _ctxVisualInfoMap.end()) {
      return search->second;
    } else {
      Log(ERR) << "Couldn't get info about an unknown GLXContext: " << ctx;
      throw ENotFound(EXCEPTION_MESSAGE);
    }
  }
  void ContextVisualInfo(GLXContext ctx, unsigned mode, short depth) {
    _ctxVisualInfoMap[ctx] = CContextVisualInfo(mode, depth);
  }

  void MapglXCtxToSurf(void* context, long unsigned int surfacedraw, long unsigned int surfaceread);
  void UnMapglXCtxToDispSurfByCtx(void* context) {
    _glXCtxToDrawable.erase(context);
  }
  void UnMapglXCtxToDispSurfBySurface(long unsigned int surface);
  long unsigned int GetglXDrawSurfaceFromContext(void* context) {
    return (_glXCtxToDrawable.find(context) != _glXCtxToDrawable.end())
               ? _glXCtxToDrawable[context].glXDrawSurface
               : 0;
  }
  long unsigned int GetglXReadSurfaceFromContext(void* context) {
    return (_glXCtxToDrawable.find(context) != _glXCtxToDrawable.end())
               ? _glXCtxToDrawable[context].glXReadSurface
               : 0;
  }
};

CTextureStateObj& TextureStateObject(GLenum target, GLint texture = -1);
#ifndef BUILD_FOR_CCODE
void GenMipMapStateData(GLint texture, GLenum target, GLint levels = -1);
void GenMipMapStateData(GLenum target, GLint levels = -1);
#endif
void SetTargetForTexture(GLenum target, GLuint texture);

namespace curctx {
bool IsOgl();
bool IsOgl11();
bool IsEs();
bool IsEs1();
bool IsEs2Plus();
bool IsEs3Plus();
bool IsEs31Plus();
int Version();
bool IsNvidia();
} // namespace curctx
} // namespace OpenGL
} // namespace gits
