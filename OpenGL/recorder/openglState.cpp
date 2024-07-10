// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglState.cpp
 *
 * @brief Definition of OpenGL library state wrapper.
 *
 */

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "openglLibrary.h"
#include "stateDynamic.h"
#include "openglState.h"
#include "openglFunction.h"
#include "gits.h"
#include "scheduler.h"
#include "streams.h"
#include "log.h"
#include "config.h"
#include "tools.h"
#include "windowContextState.h"

#include "wglFunctions.h"
#include "eglFunctions.h"
#include "glxFunctions.h"
#include "openglTools.h"
#include "gitsFunctions.h"
#include "glFunctions.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstring>
#include <memory>
#include <iomanip>

/* ********************************** S T A T E ******************************** */

/**
 * @brief Constructor
 *
 * Contructor of gits::OpenGL::CContextState class. It creates a list of all OpenGL specific
 * non shared state variable getters.
 */

gits::OpenGL::CContextState::CContextState() {
  Register(new CVariableVAOInfo);

  if (curctx::IsOgl()) {
    Register(new CVariableFramebufferEXTInfo);
  }

  if (curctx::IsOgl() || curctx::IsEs2Plus()) {
    Register(new CVariableFramebufferInfo);
  }

  if (curctx::IsOgl() || curctx::IsEs3Plus()) {
    Register(new CVariableDrawBuffers); // It must be restored after FBO!!!
  }

  if (curctx::IsOgl()) {
    Register(new CVariableMappedTexture);
  }

  Register(new CVariableViewport);

  if (curctx::IsOgl()) {
    Register(new CVariableCapability(GL_PRIMITIVE_RESTART));
    Register(new CVariablePrimitiveRestartIndex);
  }

  GLint clipPlaneNum = 0;
  GLint lightNum = 0;
  GLint maxTexCoords = 0;

  // CVariableLight has to be scheduled before CVariableMatrices to avoid double
  // transformation
  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableLightModel);
    drv.gl.glGetIntegerv(GL_MAX_CLIP_PLANES, &clipPlaneNum);
    drv.gl.glGetIntegerv(GL_MAX_LIGHTS, &lightNum);
    drv.gl.glGetIntegerv(GL_MAX_TEXTURE_COORDS, &maxTexCoords);
    for (GLint idx = 0; idx < lightNum; idx++) {
      Register(new CVariableLight(GL_LIGHT0 + idx));
    }

    Register(new CVariableMatrices);
  }

  // register capability variables
  Register(new CVariableCapability(GL_ALPHA_TEST));
  Register(new CVariableCapability(GL_BLEND));

  if (curctx::IsOgl()) {
    for (GLint idx = 0; idx < clipPlaneNum; idx++) {
      Register(new CVariableCapability(GL_CLIP_PLANE0 + idx));
    }
  }

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableClientCapability(GL_COLOR_ARRAY));
    Register(new CVariableCapability(GL_COLOR_LOGIC_OP));
    Register(new CVariableCapability(GL_COLOR_MATERIAL));
  }

  Register(new CVariableCapability(GL_CULL_FACE));
  Register(new CVariableCapability(GL_DEPTH_TEST));
  Register(new CVariableCapability(GL_DITHER, true));

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableCapability(GL_FOG));
    Register(new CVariableCapability(GL_INDEX_LOGIC_OP));
  }

  if (curctx::IsOgl()) {
    Register(new CVariableCapability(GL_FRAGMENT_PROGRAM_ARB));
    Register(new CVariableCapability(GL_VERTEX_PROGRAM_ARB));
  }

  Register(new CVariableCapability(GL_SAMPLE_ALPHA_TO_COVERAGE));
  Register(new CVariableCapability(GL_STENCIL_TEST_TWO_SIDE_EXT));

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    for (GLint idx = 0; idx < lightNum; idx++) {
      Register(new CVariableCapability(GL_LIGHT0 + idx));
    }

    Register(new CVariableCapability(GL_LIGHTING));
  }

  Register(new CVariableCapability(GL_LINE_STIPPLE));

  if (curctx::IsOgl()) {
    Register(new CVariableCapability(GL_MAP1_COLOR_4));
    Register(new CVariableCapability(GL_MAP1_INDEX));
    Register(new CVariableCapability(GL_MAP1_NORMAL));
    Register(new CVariableCapability(GL_MAP1_TEXTURE_COORD_1));
    Register(new CVariableCapability(GL_MAP1_TEXTURE_COORD_2));
    Register(new CVariableCapability(GL_MAP1_TEXTURE_COORD_3));
    Register(new CVariableCapability(GL_MAP1_TEXTURE_COORD_4));
    Register(new CVariableCapability(GL_MAP2_COLOR_4));
    Register(new CVariableCapability(GL_MAP2_INDEX));
    Register(new CVariableCapability(GL_MAP2_NORMAL));
    Register(new CVariableCapability(GL_MAP2_TEXTURE_COORD_1));
    Register(new CVariableCapability(GL_MAP2_TEXTURE_COORD_2));
    Register(new CVariableCapability(GL_MAP2_TEXTURE_COORD_3));
    Register(new CVariableCapability(GL_MAP2_TEXTURE_COORD_4));
    Register(new CVariableCapability(GL_MAP2_VERTEX_3));
    Register(new CVariableCapability(GL_MAP2_VERTEX_4));
    Register(new CVariableCapability(GL_FRAMEBUFFER_SRGB_EXT));
    Register(new CVariableCapability(GL_PROGRAM_POINT_SIZE));
    Register(new CVariableCapability(GL_POINT_SPRITE));
    Register(new CVariableCapability(GL_FRAMEBUFFER_SRGB_EXT));
    Register(new CVariableCapability(GL_TEXTURE_RECTANGLE));
    Register(new CVariableCapability(GL_TEXTURE_3D));
    Register(new CVariableCapability(GL_TEXTURE_1D));
  }
  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableClientCapability(GL_NORMAL_ARRAY));
    Register(new CVariableCapability(GL_NORMALIZE));
    Register(new CVariableCapability(GL_POINT_SMOOTH));
    Register(new CVariableCapability(GL_TEXTURE_2D));
  }

  Register(new CVariableCapability(GL_POLYGON_OFFSET_FILL));
  Register(new CVariableCapability(GL_POLYGON_OFFSET_LINE));
  Register(new CVariableCapability(GL_POLYGON_OFFSET_POINT));
  Register(new CVariableCapability(GL_POLYGON_STIPPLE));
  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableCapability(GL_RESCALE_NORMAL));
  }

  Register(new CVariableCapability(GL_SCISSOR_TEST));
  Register(new CVariableCapability(GL_STENCIL_TEST));

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableClientCapability(GL_VERTEX_ARRAY));
  }

  // register capability parameters getters
  if (curctx::IsOgl() || curctx::IsEs1()) {
    Register(new CVariableAlphaFunc);
  }
  Register(new CVariableBlendFunc);
  if (curctx::IsOgl() && !SD().IsCurrentContextCore()) {
    Register(new CVariableClearAccum);
  }
  Register(new CVariableClearColor);
  Register(new CVariableClearDepth);
  Register(new CVariableClearStencil);

  if (curctx::IsOgl()) {
    for (GLint idx = 0; idx < clipPlaneNum; idx++) {
      Register(new CVariableClipPlane(GL_CLIP_PLANE0 + idx));
    }
  }
  if (curctx::IsEs1()) {
    for (GLint idx = 0; idx < clipPlaneNum; idx++) {
      Register(new CVariableClipPlanef(GL_CLIP_PLANE0 + idx));
    }
  }

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableColor);
  }

  Register(new CVariableColorMask);

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableColorMaterial);
    Register(new CVariableMaterial());
  }

  Register(new CVariableCullFace);
  Register(new CVariableDepthMask);
  Register(new CVariableDepthFunc);
  Register(new CVariableLineWidth);
  if (curctx::IsOgl()) {
    Register(new CVariableDepthRange);
  } else {
    TODO("Provide appropriate GLES handler for function glDepthRangefOES")
    Register(new CVariableDepthRangef);
  }

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableFog);
  }
  Register(new CVariableFrontFace);
  Register(new CVariableHint);
  Register(new CVariableBlendEquation);
  Register(new CVariableBlendFunci);
  Register(new CVariableBlendEquationi);
  Register(new CVariableDrawBuffersIndexedEXTInfo);

  if (curctx::IsOgl() && !SD().IsCurrentContextCore()) {
    Register(new CVariableLineStipple);
  }

  Register(new CVariableLogicOp);

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariableNormal);
  }

  Register(new CVariablePointSize);
  Register(new CVariablePolygonOffset);

  // WA for new Specviewperf workloads
  // Scheduling glPolygonStipple in version >= 450 causes crashes
  if (curctx::IsOgl() && curctx::Version() < 450) {
    Register(new CVariablePolygonStipple);
  }
  Register(new CVariableScissor);
  Register(new CVariableShadeModel);
  Register(new CVariableStencilFunc);
  Register(new CVariableStencilOp);
  Register(new CVariableStencilMask);
  if (curctx::IsOgl() && !SD().IsCurrentContextCore()) {
    Register(new CVariableTexCoord);
  }

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    for (GLint i = 0; i < maxTexCoords; ++i) {
      Register(new CVariableTexEnv(GL_TEXTURE0 + i));
    }
    for (GLint i = 0; i < maxTexCoords; ++i) {
      Register(new CVariableTextureUnitCapabilities(GL_TEXTURE0 + i));
    }
  }

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore() && !curctx::IsOgl11())) {
    Register(new CVariableColorPointer);
    Register(new CVariableSecondaryColorPointer);
    Register(new CVariableVertexPointer);
    Register(new CVariableNormalPointer);
    Register(new CVariableTexCoordInfo);
  }

  if (curctx::IsEs1() || (curctx::IsOgl() && !SD().IsCurrentContextCore())) {
    Register(new CVariablePointParameter);
  }

  Register(new CVariablePatchParameter);

  Register(new CVariablePixelStore);

  if (curctx::IsOgl()) {
    Register(new CVariablePolygonMode);
    Register(new CVariableMap1);
    Register(new CVariableMap2);
  }

  Register(new CVariableActiveTexture);

  Register(new CVariableVertexAttribInfo);

  Register(new CVariableGLSLPipelinesInfo);

  // Bindings restoration
  Register(new CVariableSamplerBinding);

  Register(new CVariableBufferBindings);

  if (curctx::IsOgl()) {
    Register(new CVariableProgramBinding);
  }

  if (curctx::IsOgl() && !SD().IsCurrentContextCore() && !curctx::IsOgl11()) {
    Register(new CVariableRenderbufferEXTBinding);
  }

  if ((curctx::IsOgl() && !curctx::IsOgl11()) || curctx::IsEs2Plus()) {
    Register(new CVariableRenderbufferBinding);
  }

  Register(new CVariableGLSLBindings);
  Register(new CVariableBindImageTextureInfo);
  if (!curctx::IsOgl11()) {
    GLint maxTexUnits;
    if (curctx::IsEs1()) {
      drv.gl.glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxTexUnits);
    } else {
      drv.gl.glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
    }
    for (GLint i = 0; i < maxTexUnits; ++i) {
      Register(new CVariableTextureBinding(GL_TEXTURE0 + i));
    }
  }

  // Do not add here anything excepting bindings restoration. Bindings must be
  // restored as last. Rest of CVariables should be added above Bindings
  // restoration
}

/**
 * @brief Constructor
 *
 * Contructor of gits::OpenGL::CSharedState class. It creates a list of all OpenGL specific
 * shared state variable getters.
 */

gits::OpenGL::CSharedState::CSharedState() {
  if (curctx::IsOgl() && !curctx::IsOgl11()) {
    Register(new CVariableProgramInfo);
  }

  Register(new CVariableSamplerInfo);

  if (curctx::IsOgl() && !SD().IsCurrentContextCore() && !curctx::IsOgl11()) {
    Register(new CVariableRenderbufferEXTInfo);
  }

  Register(new CVariableDefaultFramebuffer);

  if ((curctx::IsOgl() && !curctx::IsOgl11()) || curctx::IsEs2Plus()) {
    Register(new CVariableGLSLInfo);
    Register(new CVariableRenderbufferInfo);
  }

  if (curctx::IsOgl() || ESBufferState() != TBuffersState::CAPTURE_ALWAYS) {
    Register(new CVariableBufferInfo);
  }
  Register(new CVariableTextureInfo);
}

/**
 * @brief Constructor
 *
 * Contructor of gits::OpenGL::CState class.
 */

gits::OpenGL::CState::CState() : _originalContext(nullptr) {
  _stateDataObsoleted.sharedState = new CSharedState;
  _stateDataObsoleted.contextState = new CContextState;
}

gits::OpenGL::CState::~CState() {
  delete _stateDataObsoleted.sharedState;
  delete _stateDataObsoleted.contextState;
}

void gits::OpenGL::CState::ScheduleCurrentContextEGL(CScheduler& scheduler, void* context) const {
  EGLSurface eglDrawDrawable = CStateDynamicNative::Get().GetEglDrawSurfaceFromContext(context);
  EGLSurface eglReadDrawable = CStateDynamicNative::Get().GetEglReadSurfaceFromContext(context);
  if (eglDrawDrawable == nullptr || eglReadDrawable == nullptr) {
    Log(ERR) << "Can't schedule eglMakeCurrent to get state of context:" << context
             << " unknown drawable";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  scheduler.Register(new CeglMakeCurrent(true, drv.egl.eglGetCurrentDisplay(),
                                         (EGLSurface)eglReadDrawable, (EGLSurface)eglDrawDrawable,
                                         (EGLContext)context));
}

void gits::OpenGL::CState::ScheduleCurrentContext(CScheduler& scheduler, void* context) const {
#ifdef GITS_PLATFORM_WINDOWS
  // use EGL when libEGL.dll is loaded
  if (drv.gl.Api() == CGlDriver::API_GLES1 || drv.gl.Api() == CGlDriver::API_GLES2) {
    ScheduleCurrentContextEGL(scheduler, context);
  }
  // use WGL when OpenGL32.dll is loaded
  else if (drv.gl.Api() == CGlDriver::API_GL || curctx::IsOgl()) {
    HDC hdc = (HDC)CStateDynamicNative::Get().GetHdcFromHglrc(context);
    if (hdc == 0) {
      Log(ERR) << "Can't schedule wglMakeCurrent to get state of context:" << context
               << " unknown hdc";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    scheduler.Register(new CwglMakeCurrent(true, hdc, (HGLRC)context));
  }
  // this code should never be reached
  else {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
#elif defined GITS_PLATFORM_X11
  if (curctx::IsOgl()) {
    GLXDrawable glXDrawDrawable = CStateDynamicNative::Get().GetglXDrawSurfaceFromContext(context);
    GLXDrawable glXReadDrawable = CStateDynamicNative::Get().GetglXReadSurfaceFromContext(context);
    if (glXDrawDrawable == 0 || glXReadDrawable == 0) {
      Log(ERR) << "Can't schedule glXMakeContextCurrent to get state of context:" << context
               << " unknown drawable";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    scheduler.Register(
        new CglXMakeContextCurrent(0, drv.glx.glXGetCurrentDisplay(), (GLXDrawable)glXReadDrawable,
                                   (GLXDrawable)glXDrawDrawable, (GLXContext)context));
  } else {
    ScheduleCurrentContextEGL(scheduler, context);
  }
#endif
}

// Get state per context
void gits::OpenGL::CState::Get() {

  _originalContext = GetCurrentContextAPI();
  for (auto context : SD().GetContextVector()) {
    if (SetCurrentContext(context)) {
      _contextStateDataMap.insert(std::make_pair(context, CStateData()));

      if (SD().IsCurrentContextSharedDataOwner()) {
        _contextStateDataMap[context].sharedState.reset(new CSharedState);
        _contextStateDataMap[context].sharedState->Get();
      }

      _contextStateDataMap[context].contextState.reset(new CContextState);
      _contextStateDataMap[context].contextState->Get();
    }
  }
  // Set original context
  SetCurrentContext(_originalContext);
}

void gits::OpenGL::CState::Schedule(CScheduler& scheduler) const {
  // Schedule per shared group state
  for (const auto& elem : ContextStateDataMap()) {
    // Current context in scheduler is required because state dynamic is used
    // and state selection is done via current context
    SetCurrentContext(elem.first);
    ScheduleCurrentContext(scheduler, elem.first);

    // Check if shared state exists for scheduled context
    if (elem.second.sharedState.use_count() != 0) {
      elem.second.sharedState->Schedule(scheduler, CSharedState());
    }
  }

  // Schedule per context state
  for (const auto& elem : ContextStateDataMap()) {
    // Current context in scheduler is required because state dynamic is used
    // and state selection is done via current context
    SetCurrentContext(elem.first);
    ScheduleCurrentContext(scheduler, elem.first);

    elem.second.contextState->Schedule(scheduler, CContextState());
  }

  // Set original context
  ScheduleCurrentContext(scheduler, _originalContext);
  SetCurrentContext(_originalContext);
}

/* ***************************** C A P A B I L I T Y *************************** */

gits::OpenGL::CVariableCapability::CVariableCapability(GLenum capability,
                                                       bool defaultValue /* false */)
    : _capability(capability), _value(defaultValue) {}

void gits::OpenGL::CVariableCapability::Get() {
  _value = drv.gl.glIsEnabled(_capability);
}

void gits::OpenGL::CVariableCapability::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  const CVariableCapability& variable = static_cast<const CVariableCapability&>(lastValue);

  if (_value != variable._value) {
    if (_value) {
      scheduler.Register(new OpenGL::CglEnable(_capability));
    } else {
      scheduler.Register(new OpenGL::CglDisable(_capability));
    }
  }
}

/* ***************************** C L I E N T      C A P A B I L I T Y *************************** */
// could go to template CVariableCapability<typename Enabler, typename Disabler>
gits::OpenGL::CVariableClientCapability::CVariableClientCapability(GLenum capability,
                                                                   bool defaultValue /* false */)
    : _capability(capability), _value(defaultValue) {}

void gits::OpenGL::CVariableClientCapability::Get() {
  _value = drv.gl.glIsEnabled(_capability);
}

void gits::OpenGL::CVariableClientCapability::Schedule(CScheduler& scheduler,
                                                       const CVariable& lastValue) const {
  const CVariableClientCapability& variable =
      static_cast<const CVariableClientCapability&>(lastValue);

  if (_value != variable._value) {
    if (_value) {
      scheduler.Register(new OpenGL::CglEnableClientState(_capability));
    } else {
      scheduler.Register(new OpenGL::CglDisableClientState(_capability));
    }
  }
}

/* ***************************** V E R T E X     A T T R I B     I N F O ***************************/
bool gits::OpenGL::CVariableVertexAttribInfo::_supported = false;

GLint gits::OpenGL::CVariableVertexAttribInfo::_maxVertexAttribs = 0;

gits::OpenGL::CVariableVertexAttribInfo::TVertexAttribData::TVertexAttribData()
    : _enabled(GL_FALSE),
      _size(4),
      _type(GL_FLOAT),
      _stride(0),
      _normalized(GL_FALSE),
      _bufferBinding(0),
      _divisor(0),
      _binding(0),
      _relativeOffset(0),
      _bindingOffset(0),
      _bindingStride(0),
      _bindingDivisor(0),
      _pointer(nullptr) {}

gits::OpenGL::CVariableVertexAttribInfo::CVariableVertexAttribInfo() : _vertexArrayObject(0) {
  if (curctx::IsEs2Plus() || (curctx::IsOgl() && curctx::Version() >= 200)) {
    _supported = true;
  }

  if (_maxVertexAttribs == 0) {
    drv.gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_maxVertexAttribs);
  }
  _vertexAttribs.resize(_maxVertexAttribs);
}

void gits::OpenGL::CVariableVertexAttribInfo::Get() {
  if (!_supported) {
    return;
  }

  if (curctx::IsEs3Plus() || (curctx::IsOgl() && curctx::Version() >= 300)) {
    drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&_vertexArrayObject);
  }

  for (GLint i = 0; i < _maxVertexAttribs; ++i) {
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &_vertexAttribs[i]._enabled);
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &_vertexAttribs[i]._size);
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &_vertexAttribs[i]._type);
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
                               &_vertexAttribs[i]._normalized);
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &_vertexAttribs[i]._stride);
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &_vertexAttribs[i]._divisor);
    drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
                               &_vertexAttribs[i]._bufferBinding);
    if ((curctx::IsEs31Plus() || (curctx::IsOgl() && curctx::Version() >= 430)) &&
        _vertexArrayObject) {
      drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_BINDING, &_vertexAttribs[i]._binding);
      drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_RELATIVE_OFFSET,
                                 &_vertexAttribs[i]._relativeOffset);
      drv.gl.glGetIntegeri_v(GL_VERTEX_BINDING_OFFSET, i, &_vertexAttribs[i]._bindingOffset);
      drv.gl.glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, i, &_vertexAttribs[i]._bindingStride);
      drv.gl.glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, i, &_vertexAttribs[i]._bindingDivisor);
    }
    drv.gl.glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER,
                                     &_vertexAttribs[i]._pointer);
  }
}

void gits::OpenGL::CVariableVertexAttribInfo::Schedule(CScheduler& scheduler,
                                                       const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  int array_binding;
  drv.gl.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_binding);

  const CVariableVertexAttribInfo& variable =
      static_cast<const CVariableVertexAttribInfo&>(lastValue);

  for (GLint i = 0; i < _maxVertexAttribs; ++i) {
    if (_vertexAttribs[i]._size != variable._vertexAttribs[i]._size ||
        _vertexAttribs[i]._type != variable._vertexAttribs[i]._type ||
        _vertexAttribs[i]._normalized != variable._vertexAttribs[i]._normalized ||
        _vertexAttribs[i]._stride != variable._vertexAttribs[i]._stride ||
        _vertexAttribs[i]._bufferBinding != variable._vertexAttribs[i]._bufferBinding ||
        _vertexAttribs[i]._pointer != variable._vertexAttribs[i]._pointer ||
        _vertexAttribs[i]._divisor != variable._vertexAttribs[i]._divisor ||
        _vertexAttribs[i]._binding != variable._vertexAttribs[i]._binding ||
        _vertexAttribs[i]._relativeOffset != variable._vertexAttribs[i]._relativeOffset ||
        _vertexAttribs[i]._bindingStride != variable._vertexAttribs[i]._bindingStride ||
        _vertexAttribs[i]._bindingDivisor != variable._vertexAttribs[i]._bindingDivisor) {
      auto isEsEnabled = curctx::IsEs31Plus();
      if ((isEsEnabled || (curctx::IsOgl() && curctx::Version() >= 430)) && _vertexArrayObject) {
        if (_vertexAttribs[i]._bufferBinding) {
          scheduler.Register(new CglVertexAttribFormat(
              i, _vertexAttribs[i]._size, _vertexAttribs[i]._type,
              _vertexAttribs[i]._normalized == GL_TRUE, _vertexAttribs[i]._relativeOffset));
          scheduler.Register(new CglVertexAttribBinding(i, _vertexAttribs[i]._binding));
          scheduler.Register(new CglBindVertexBuffer(
              _vertexAttribs[i]._binding, _vertexAttribs[i]._bufferBinding,
              (GLintptr)_vertexAttribs[i]._bindingOffset, _vertexAttribs[i]._bindingStride));
          if (_vertexAttribs[i]._bindingDivisor != variable._vertexAttribs[i]._bindingDivisor) {
            scheduler.Register(new CglVertexBindingDivisor(_vertexAttribs[i]._binding,
                                                           _vertexAttribs[i]._bindingDivisor));
          }
        }
      } else {
        scheduler.Register(new CglBindBuffer(GL_ARRAY_BUFFER, _vertexAttribs[i]._bufferBinding));
        scheduler.Register(new CglVertexAttribPointer(
            i, _vertexAttribs[i]._size, _vertexAttribs[i]._type,
            _vertexAttribs[i]._normalized == GL_TRUE, _vertexAttribs[i]._stride,
            _vertexAttribs[i]._pointer, _vertexAttribs[i]._bufferBinding));
        if (_vertexAttribs[i]._divisor != variable._vertexAttribs[i]._divisor) {
          scheduler.Register(new CglVertexAttribDivisorARB(i, _vertexAttribs[i]._divisor));
        }
      }
    }
    if (_vertexAttribs[i]._enabled != variable._vertexAttribs[i]._enabled) {
      if (_vertexAttribs[i]._enabled) {
        scheduler.Register(new CglEnableVertexAttribArray(i));
      } else {
        scheduler.Register(new CglDisableVertexAttribArray(i));
      }
    }
  }
  scheduler.Register(new CglBindBuffer(GL_ARRAY_BUFFER, array_binding));
}

/* ***************************** T E X T U R E    U N I T   C A P A B I L I T I E S *************************** */

const GLenum gits::OpenGL::CVariableTextureUnitCapabilities::_targets[] = {
    GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP};

const GLenum gits::OpenGL::CVariableTextureUnitCapabilities::_texGenCoords[] = {
    GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_T,
    GL_TEXTURE_GEN_R,
    GL_TEXTURE_GEN_Q,
};

const GLenum gits::OpenGL::CVariableTextureUnitCapabilities::_texGenTargets[] = {
    GL_S,
    GL_T,
    GL_R,
    GL_Q,
};

gits::OpenGL::CVariableTextureUnitCapabilities::CVariableTextureUnitCapabilities(GLenum textureUnit)
    : _textureUnit(textureUnit), _textureCoordArray(false) {
  _supported =
      curctx::IsEs1() ||
      (curctx::IsOgl() && (curctx::Version() >= 130 || drv.gl.HasExtension("GL_ARB_multitexture")));

  std::fill_n(_texGenCoordsEnabled, 4, GL_FALSE);
  std::fill_n(_texturing, _targetNum, static_cast<GLboolean>(0));

  for (unsigned i = 0; i < _texGenNum; ++i) {
    _texGenInfo[i].mode = GL_EYE_LINEAR;
    std::fill_n(_texGenInfo[i].eyePlane, 4, 0);
    std::fill_n(_texGenInfo[i].objectPlane, 4, 0);
    if (i == 0) { // s plane
      _texGenInfo[i].eyePlane[0] = _texGenInfo[i].objectPlane[0] = 1;
    }
    if (i == 1) { // t plane
      _texGenInfo[i].eyePlane[1] = _texGenInfo[i].objectPlane[1] = 1;
    }
  }
}

void gits::OpenGL::CVariableTextureUnitCapabilities::Get() {
  if (!_supported) {
    return;
  }

  GLint currentTexture = GL_TEXTURE0;
  drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &currentTexture);
  _originalUnit = currentTexture;

  if (_textureUnit != _originalUnit) {
    drv.gl.glActiveTexture(_textureUnit);
    drv.gl.glClientActiveTexture(_textureUnit);
  }

  for (unsigned i = 0; i < _targetNum; ++i) {
    _texturing[i] = drv.gl.glIsEnabled(_targets[i]);
  }

  _textureCoordArray = drv.gl.glIsEnabled(GL_TEXTURE_COORD_ARRAY);

  if (curctx::IsOgl()) {
    for (unsigned i = 0; i < _texGenNum; ++i) {
      _texGenCoordsEnabled[i] = drv.gl.glIsEnabled(_texGenCoords[i]);
      drv.gl.glGetTexGeniv(_texGenTargets[i], GL_TEXTURE_GEN_MODE, &_texGenInfo[i].mode);
      drv.gl.glGetTexGeniv(_texGenTargets[i], GL_EYE_PLANE, _texGenInfo[i].eyePlane);
      drv.gl.glGetTexGeniv(_texGenTargets[i], GL_OBJECT_PLANE, _texGenInfo[i].objectPlane);
    }
  }

  if (_textureUnit != _originalUnit) {
    drv.gl.glActiveTexture(_originalUnit);
    drv.gl.glClientActiveTexture(_originalUnit);
  }
}

void gits::OpenGL::CVariableTextureUnitCapabilities::Schedule(CScheduler& scheduler,
                                                              const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  const CVariableTextureUnitCapabilities& variable =
      static_cast<const CVariableTextureUnitCapabilities&>(lastValue);

  scheduler.Register(new OpenGL::CglActiveTexture(_textureUnit));

  for (unsigned i = 0; i < _targetNum; ++i) {
    if (_texturing[i] != variable._texturing[i]) {
      if (_texturing[i] == GL_FALSE) {
        scheduler.Register(new CglDisable(_targets[i]));
      } else {
        scheduler.Register(new CglEnable(_targets[i]));
      }
    }
  }

  for (unsigned i = 0; i < _texGenNum; ++i) {
    if (_texGenCoordsEnabled[i] != variable._texGenCoordsEnabled[i]) {
      if (_texGenCoordsEnabled[i] == GL_FALSE) {
        scheduler.Register(new CglDisable(_texGenCoords[i]));
      } else {
        scheduler.Register(new CglEnable(_texGenCoords[i]));
      }
    }
  }

  // this state depends on client active texture
  if (_textureCoordArray != variable._textureCoordArray) {
    scheduler.Register(new OpenGL::CglClientActiveTexture(_textureUnit));
    if (_textureCoordArray == GL_FALSE) {
      scheduler.Register(new CglDisableClientState(GL_TEXTURE_COORD_ARRAY));
    } else {
      scheduler.Register(new CglEnableClientState(GL_TEXTURE_COORD_ARRAY));
    }
    scheduler.Register(new OpenGL::CglClientActiveTexture(_originalUnit));
  }
  if (curctx::IsOgl()) {
    for (unsigned i = 0; i < _texGenNum; ++i) {
      if (_texGenInfo[i].mode != variable._texGenInfo[i].mode) {
        scheduler.Register(
            new CglTexGeni(_texGenTargets[i], GL_TEXTURE_GEN_MODE, _texGenInfo[i].mode));
      }
      if (!std::equal(_texGenInfo[i].eyePlane, _texGenInfo[i].eyePlane + 4,
                      variable._texGenInfo[i].eyePlane)) {
        scheduler.Register(
            new CglTexGeniv(_texGenTargets[i], GL_EYE_PLANE, _texGenInfo[i].eyePlane));
      }
      if (!std::equal(_texGenInfo[i].objectPlane, _texGenInfo[i].objectPlane + 4,
                      variable._texGenInfo[i].objectPlane)) {
        scheduler.Register(
            new CglTexGeniv(_texGenTargets[i], GL_OBJECT_PLANE, _texGenInfo[i].objectPlane));
      }
    }
  }
  scheduler.Register(new OpenGL::CglActiveTexture(_originalUnit));
}

/* ****************************** A C T I V E   T E X T U R E ****************** */
void gits::OpenGL::CVariableActiveTexture::Get() {}

void gits::OpenGL::CVariableActiveTexture::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  GLint& activeTexture =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture;
  scheduler.Register(new OpenGL::CglActiveTexture(activeTexture));
}

/* ***************************** A L P H A   F U N C *************************** */

void gits::OpenGL::CVariableAlphaFunc::Get() {
  CGeneralStateData::Restore::CAlphaFunc& alphaFunc =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.alphaFunc;
  drv.gl.glGetIntegerv(GL_ALPHA_TEST_FUNC, &alphaFunc.func);
  drv.gl.glGetFloatv(GL_ALPHA_TEST_REF, &alphaFunc.ref);
}

void gits::OpenGL::CVariableAlphaFunc::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  CGeneralStateData::Restore::CAlphaFunc& alphaFunc =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.alphaFunc;
  scheduler.Register(new OpenGL::CglAlphaFunc(alphaFunc.func, alphaFunc.ref));
}

/* ***************************** B L E N D   F U N C *************************** */

void gits::OpenGL::CVariableBlendFunc::Get() {}

void gits::OpenGL::CVariableBlendFunc::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  CGeneralStateData::Track::CBlendFunc& blendFunc =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.blendFunc;
  CGeneralStateData::Track::CBlendFuncSeparate& blendFuncSeparate =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.blendFuncSeparate;
  if (blendFunc.used == true) {
    scheduler.Register(new OpenGL::CglBlendFunc(blendFunc.src, blendFunc.dst));
  } else if (blendFuncSeparate.used == true) {
    scheduler.Register(
        new OpenGL::CglBlendFuncSeparate(blendFuncSeparate.srcRGB, blendFuncSeparate.dstRGB,
                                         blendFuncSeparate.srcAlpha, blendFuncSeparate.dstAlpha));
  }
}

/* ****************************** C L E A R   A C C U M ************************ */

void gits::OpenGL::CVariableClearAccum::Get() {
  GLfloat color[4];
  CGeneralStateData::Restore::CClearAccum& clearAccum =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearAccum;
  drv.gl.glGetFloatv(GL_ACCUM_CLEAR_VALUE, color);
  clearAccum.red = color[0];
  clearAccum.green = color[1];
  clearAccum.blue = color[2];
  clearAccum.alpha = color[3];
}

void gits::OpenGL::CVariableClearAccum::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  CGeneralStateData::Restore::CClearAccum& clearAccum =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearAccum;
  scheduler.Register(new OpenGL::CglClearAccum(clearAccum.red, clearAccum.green, clearAccum.blue,
                                               clearAccum.alpha));
}

/* ****************************** C L E A R   C O L O R ************************ */

void gits::OpenGL::CVariableClearColor::Get() {
  CGeneralStateData::Restore::CClearColor& clearColor =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearColor;
  GLclampf color[4];
  drv.gl.glGetFloatv(GL_COLOR_CLEAR_VALUE, color);
  clearColor.red = color[0];
  clearColor.green = color[1];
  clearColor.blue = color[2];
  clearColor.alpha = color[3];
}

void gits::OpenGL::CVariableClearColor::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  CGeneralStateData::Restore::CClearColor& clearColor =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearColor;
  scheduler.Register(new OpenGL::CglClearColor(clearColor.red, clearColor.green, clearColor.blue,
                                               clearColor.alpha));
}

/* ****************************** M A T R I C E S ************************ */

void gits::OpenGL::CVariableMatrices::Get() {
  GLint mode;
  CGeneralStateData::Restore::CMatrices& matrices =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.matrices;
  drv.gl.glGetIntegerv(GL_MATRIX_MODE, &mode);
  matrices.mode = mode;

  drv.gl.glGetFloatv(GL_MODELVIEW_MATRIX, matrices.matrixArray[0]);
  drv.gl.glGetFloatv(GL_PROJECTION_MATRIX, matrices.matrixArray[1]);
  drv.gl.glGetFloatv(GL_TEXTURE_MATRIX, matrices.matrixArray[2]);
  if (curctx::IsOgl()) {
    drv.gl.glGetFloatv(GL_COLOR_MATRIX, matrices.matrixArray[3]);
  }
}

void gits::OpenGL::CVariableMatrices::Schedule(CScheduler& scheduler,
                                               const CVariable& lastValue) const {
  CGeneralStateData::Restore::CMatrices& matrices =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.matrices;
  // Matrices initialized as per default state
  CGeneralStateData::Restore::CMatrices defaultMatrices;

  bool dirty = false;
  GLint recMode;
  drv.gl.glGetIntegerv(GL_MATRIX_MODE, &recMode);

  // change a == b to |a - b| < e
  if (!std::equal(matrices.matrixArray[0], matrices.matrixArray[0] + 16,
                  defaultMatrices.matrixArray[0])) {
    scheduler.Register(new CglMatrixMode(GL_MODELVIEW));
    scheduler.Register(new CglLoadMatrixf(matrices.matrixArray[0]));
  }
  if (!std::equal(matrices.matrixArray[1], matrices.matrixArray[1] + 16,
                  defaultMatrices.matrixArray[1])) {
    scheduler.Register(new CglMatrixMode(GL_PROJECTION));
    scheduler.Register(new CglLoadMatrixf(matrices.matrixArray[1]));
    dirty = true;
  }
  if (!std::equal(matrices.matrixArray[2], matrices.matrixArray[2] + 16,
                  defaultMatrices.matrixArray[2])) {
    scheduler.Register(new CglMatrixMode(GL_TEXTURE));
    scheduler.Register(new CglLoadMatrixf(matrices.matrixArray[2]));
    dirty = true;
  }
  if (curctx::IsOgl()) {
    if (!std::equal(matrices.matrixArray[3], matrices.matrixArray[3] + 16,
                    defaultMatrices.matrixArray[3])) {
      // this is to check for support of GL_COLOR
      while (drv.gl.glGetError() != GL_NO_ERROR) {
        ;
      }
      drv.gl.glMatrixMode(GL_COLOR);
      if (drv.gl.glGetError() == GL_NO_ERROR) {
        scheduler.Register(new CglMatrixMode(GL_COLOR));
        scheduler.Register(new CglLoadMatrixf(matrices.matrixArray[3]));
      }
      dirty = true;
    }
  }

  if (matrices.mode != defaultMatrices.mode || dirty) {
    scheduler.Register(new CglMatrixMode(matrices.mode));
  }

  drv.gl.glMatrixMode(recMode);
}

/* ****************************** C L E A R   D E P T H ************************ */

void gits::OpenGL::CVariableClearDepth::Get() {
  GLclampf& depth =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearDepth;
  drv.gl.glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depth);
}

void gits::OpenGL::CVariableClearDepth::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  GLclampf& depth =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearDepth;
  if (curctx::IsOgl()) {
    scheduler.Register(new CglClearDepth(depth));
  } else {
    scheduler.Register(new CglClearDepthf(depth));
  }
}

/* ****************************** C L E A R   S T E N C I L ************************ */

void gits::OpenGL::CVariableClearStencil::Get() {
  GLint& clearStencil =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearStencil;
  drv.gl.glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &clearStencil);
}

void gits::OpenGL::CVariableClearStencil::Schedule(CScheduler& scheduler,
                                                   const CVariable& lastValue) const {
  GLint& clearStencil =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.clearStencil;
  scheduler.Register(new OpenGL::CglClearStencil(clearStencil));
}

/* ***************************** C L I P   P L A N E *************************** */

gits::OpenGL::CVariableClipPlane::CVariableClipPlane(GLenum plane) : _plane(plane) {
  for (auto& component : _equation) {
    component = 0;
  }
}

void gits::OpenGL::CVariableClipPlane::Get() {
  drv.gl.glGetClipPlane(_plane, _equation);
}

void gits::OpenGL::CVariableClipPlane::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  const CVariableClipPlane& variable = static_cast<const CVariableClipPlane&>(lastValue);

  for (unsigned short i = 0; i < COORD_NUM; i++) {
    if (_equation[i] != variable._equation[i]) {
      scheduler.Register(new OpenGL::CglClipPlane(_plane, _equation));
      break;
    }
  }
}

/* ***************************** C L I P   P L A N E *************************** */

gits::OpenGL::CVariableClipPlanef::CVariableClipPlanef(GLenum plane) : _plane(plane) {
  for (auto& component : _equation) {
    component = 0;
  }
}

void gits::OpenGL::CVariableClipPlanef::Get() {
  drv.gl.glGetClipPlanef(_plane, _equation);
}

void gits::OpenGL::CVariableClipPlanef::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  const CVariableClipPlanef& variable = static_cast<const CVariableClipPlanef&>(lastValue);

  for (unsigned short i = 0; i < COORD_NUM; i++) {
    if (_equation[i] != variable._equation[i]) {
      scheduler.Register(new OpenGL::CglClipPlanef(_plane, _equation));
      break;
    }
  }
}

/* ****************************** C O L O R ************************************ */

void gits::OpenGL::CVariableColor::Get() {
  CGeneralStateData::Restore::CColor& color =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.color;
  GLfloat colorParams[] = {1, 1, 1, 1};
  drv.gl.glGetFloatv(GL_CURRENT_COLOR, colorParams);
  color.red = colorParams[0];
  color.green = colorParams[1];
  color.blue = colorParams[2];
  color.alpha = colorParams[3];
}

void gits::OpenGL::CVariableColor::Schedule(CScheduler& scheduler,
                                            const CVariable& lastValue) const {
  CGeneralStateData::Restore::CColor& color =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.color;

  if (color.red != 1 || color.green != 1 || color.blue != 1 || color.alpha != 1) {
    scheduler.Register(new OpenGL::CglColor4f(color.red, color.green, color.blue, color.alpha));
  }
}

/* ****************************** C O L O R   M A S K ************************** */

void gits::OpenGL::CVariableColorMask::Get() {
  CGeneralStateData::Restore::CColorMask& colorMask =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.colorMask;
  GLboolean colorMaskParams[4];
  drv.gl.glGetBooleanv(GL_COLOR_WRITEMASK, colorMaskParams);
  colorMask.red = colorMaskParams[0];
  colorMask.green = colorMaskParams[1];
  colorMask.blue = colorMaskParams[2];
  colorMask.alpha = colorMaskParams[3];
}

void gits::OpenGL::CVariableColorMask::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  CGeneralStateData::Restore::CColorMask& colorMask =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.colorMask;
  scheduler.Register(
      new OpenGL::CglColorMask(colorMask.red, colorMask.green, colorMask.blue, colorMask.alpha));
}

/* ****************************** C O L O R   M A T E R I A L ****************** */

gits::OpenGL::CVariableColorMaterial::CVariableColorMaterial()
    : _face(GL_FRONT_AND_BACK), _mode(GL_AMBIENT_AND_DIFFUSE) {}

void gits::OpenGL::CVariableColorMaterial::Get() {
  drv.gl.glGetIntegerv(GL_COLOR_MATERIAL_FACE, &_face);
  drv.gl.glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, &_mode);
}

void gits::OpenGL::CVariableColorMaterial::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  const CVariableColorMaterial& variable = static_cast<const CVariableColorMaterial&>(lastValue);

  if (_face != variable._face || _mode != variable._mode) {
    scheduler.Register(new OpenGL::CglColorMaterial(_face, _mode));
  }
}

/* ****************************** C O L O R   P O I N T E R ******************** */

gits::OpenGL::CVariableColorPointer::CVariableColorPointer()
    : _size(4), _type(GL_FLOAT), _stride(0), _pointer(nullptr), _boundBuffer(0) {}

void gits::OpenGL::CVariableColorPointer::Get() {
  drv.gl.glGetIntegerv(GL_COLOR_ARRAY_BUFFER_BINDING, &_boundBuffer);
  drv.gl.glGetIntegerv(GL_COLOR_ARRAY_SIZE, &_size);
  drv.gl.glGetIntegerv(GL_COLOR_ARRAY_TYPE, &_type);
  drv.gl.glGetIntegerv(GL_COLOR_ARRAY_STRIDE, &_stride);
  drv.gl.glGetPointerv(GL_COLOR_ARRAY_POINTER, &_pointer);
}

void gits::OpenGL::CVariableColorPointer::Schedule(CScheduler& scheduler,
                                                   const CVariable& lastValue) const {
  const CVariableColorPointer& variable = static_cast<const CVariableColorPointer&>(lastValue);

  if (_size != variable._size || _type != variable._type || _stride != variable._stride ||
      _pointer != variable._pointer) {
    if (_boundBuffer != variable._boundBuffer) {
      scheduler.Register(new OpenGL::CglBindBuffer(GL_ARRAY_BUFFER, _boundBuffer));
    }
    scheduler.Register(new OpenGL::CglColorPointer(_size, _type, _stride, _pointer, _boundBuffer));
  }
}

/* ****************************** C O L O R   P O I N T E R ******************** */

gits::OpenGL::CVariableSecondaryColorPointer::CVariableSecondaryColorPointer()
    : _size(3), _type(GL_FLOAT), _stride(0), _pointer(nullptr), _boundBuffer(0) {}

void gits::OpenGL::CVariableSecondaryColorPointer::Get() {
  drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING, &_boundBuffer);
  drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_SIZE, &_size);
  drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_TYPE, &_type);
  drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_STRIDE, &_stride);
  drv.gl.glGetPointerv(GL_SECONDARY_COLOR_ARRAY_POINTER, &_pointer);
}

void gits::OpenGL::CVariableSecondaryColorPointer::Schedule(CScheduler& scheduler,
                                                            const CVariable& lastValue) const {
  const CVariableSecondaryColorPointer& variable =
      static_cast<const CVariableSecondaryColorPointer&>(lastValue);

  if (_size != variable._size || _type != variable._type || _stride != variable._stride ||
      _pointer != variable._pointer) {
    if (_boundBuffer != variable._boundBuffer) {
      scheduler.Register(new OpenGL::CglBindBuffer(GL_ARRAY_BUFFER, _boundBuffer));
    }
    scheduler.Register(
        new OpenGL::CglSecondaryColorPointer(_size, _type, _stride, _pointer, _boundBuffer));
  }
}

/* ****************************** N O R M A L   P O I N T E R ******************** */

gits::OpenGL::CVariableNormalPointer::CVariableNormalPointer()
    : _type(GL_FLOAT), _stride(0), _pointer(nullptr), _boundBuffer(0) {}

void gits::OpenGL::CVariableNormalPointer::Get() {
  drv.gl.glGetIntegerv(GL_NORMAL_ARRAY_BUFFER_BINDING, &_boundBuffer);
  drv.gl.glGetIntegerv(GL_NORMAL_ARRAY_TYPE, &_type);
  drv.gl.glGetIntegerv(GL_NORMAL_ARRAY_STRIDE, &_stride);
  drv.gl.glGetPointerv(GL_NORMAL_ARRAY_POINTER, &_pointer);
}

void gits::OpenGL::CVariableNormalPointer::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  const CVariableNormalPointer& variable = static_cast<const CVariableNormalPointer&>(lastValue);

  if (_type != variable._type || _stride != variable._stride || _pointer != variable._pointer ||
      _boundBuffer != variable._boundBuffer) {
    if (_boundBuffer != variable._boundBuffer) {
      scheduler.Register(new OpenGL::CglBindBuffer(GL_ARRAY_BUFFER, _boundBuffer));
    }
    scheduler.Register(new OpenGL::CglNormalPointer(_type, _stride, _pointer, _boundBuffer));
  }
}

/* ****************************** V E R T E X   P O I N T E R ******************** */

gits::OpenGL::CVariableVertexPointer::CVariableVertexPointer()
    : _size(4), _type(GL_FLOAT), _stride(0), _pointer(nullptr), _boundBuffer(0) {}

void gits::OpenGL::CVariableVertexPointer::Get() {
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BUFFER_BINDING, &_boundBuffer);
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &_size);
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_TYPE, &_type);
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &_stride);
  drv.gl.glGetPointerv(GL_VERTEX_ARRAY_POINTER, &_pointer);
}

void gits::OpenGL::CVariableVertexPointer::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  const CVariableVertexPointer& variable = static_cast<const CVariableVertexPointer&>(lastValue);

  if (_size != variable._size || _type != variable._type || _stride != variable._stride ||
      _pointer != variable._pointer || _boundBuffer != variable._boundBuffer) {
    if (_boundBuffer != variable._boundBuffer) {
      scheduler.Register(new OpenGL::CglBindBuffer(GL_ARRAY_BUFFER, _boundBuffer));
    }
    scheduler.Register(new OpenGL::CglVertexPointer(_size, _type, _stride, _pointer, _boundBuffer));
  }
}

/* ****************************** M A T E R I A L **************************** */
const GLenum gits::OpenGL::CVariableMaterial::_faces[_facesNum] = {GL_FRONT, GL_BACK};

gits::OpenGL::CVariableMaterial::TMaterialInfo::TMaterialInfo() {
  std::fill_n(ambient, 3, 0.2f);
  ambient[3] = 1.0f;

  std::fill_n(diffuse, 3, 0.8f);
  diffuse[3] = 1.0f;

  std::fill_n(specular, 3, 0.0f);
  specular[3] = 1.0f;

  std::fill_n(emission, 3, 0.0f);
  emission[3] = 1.0f;

  shininess = 0.0f;

  // guess
  std::fill_n(indexes, 3, 0.0f);
}

gits::OpenGL::CVariableMaterial::CVariableMaterial() {}

void gits::OpenGL::CVariableMaterial::Get() {
  for (unsigned i = 0; i < _facesNum; ++i) {
    drv.gl.glGetMaterialfv(_faces[i], GL_AMBIENT, _data[i].ambient);
    drv.gl.glGetMaterialfv(_faces[i], GL_DIFFUSE, _data[i].diffuse);
    drv.gl.glGetMaterialfv(_faces[i], GL_SPECULAR, _data[i].specular);
    drv.gl.glGetMaterialfv(_faces[i], GL_EMISSION, _data[i].emission);
    drv.gl.glGetMaterialfv(_faces[i], GL_SHININESS, &_data[i].shininess);
    drv.gl.glGetMaterialfv(_faces[i], GL_COLOR_INDEXES, _data[i].indexes);
  }
}

void gits::OpenGL::CVariableMaterial::Schedule(CScheduler& scheduler,
                                               const CVariable& lastValue) const {
  const CVariableMaterial& variable = static_cast<const CVariableMaterial&>(lastValue);

  for (unsigned i = 0; i < _facesNum; ++i) {
    if (!std::equal(_data[i].ambient, _data[i].ambient + 4, variable._data[i].ambient)) {
      scheduler.Register(new CglMaterialfv(_faces[i], GL_AMBIENT, _data[i].ambient));
    }
    if (!std::equal(_data[i].diffuse, _data[i].diffuse + 4, variable._data[i].diffuse)) {
      scheduler.Register(new CglMaterialfv(_faces[i], GL_DIFFUSE, _data[i].diffuse));
    }
    if (!std::equal(_data[i].specular, _data[i].specular + 4, variable._data[i].specular)) {
      scheduler.Register(new CglMaterialfv(_faces[i], GL_SPECULAR, _data[i].specular));
    }
    if (!std::equal(_data[i].emission, _data[i].emission + 4, variable._data[i].emission)) {
      scheduler.Register(new CglMaterialfv(_faces[i], GL_EMISSION, _data[i].emission));
    }
    if (_data[i].shininess != variable._data[i].shininess) {
      scheduler.Register(new CglMaterialfv(_faces[i], GL_SHININESS, &_data[i].shininess));
    }
    if (!std::equal(_data[i].indexes, _data[i].indexes + size(_data[i].indexes),
                    variable._data[i].indexes)) {
      scheduler.Register(new CglMaterialfv(_faces[i], GL_COLOR_INDEXES, _data[i].indexes));
    }
  }
}

/* ****************************** C U L L   F A C E **************************** */

gits::OpenGL::CVariableCullFace::CVariableCullFace() : _mode(GL_BACK) {}

void gits::OpenGL::CVariableCullFace::Get() {
  drv.gl.glGetIntegerv(GL_CULL_FACE_MODE, &_mode);
}

void gits::OpenGL::CVariableCullFace::Schedule(CScheduler& scheduler,
                                               const CVariable& lastValue) const {
  const CVariableCullFace& variable = static_cast<const CVariableCullFace&>(lastValue);

  if (_mode != variable._mode) {
    scheduler.Register(new OpenGL::CglCullFace(_mode));
  }
}

/* ***************************** D E P T H   F U N C *************************** */

void gits::OpenGL::CVariableDepthFunc::Get() {
  GLint& depthFunc =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.depthFunc;
  drv.gl.glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
}

void gits::OpenGL::CVariableDepthFunc::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  GLint& depthFunc =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.depthFunc;

  if (depthFunc != GL_LESS) {
    scheduler.Register(new OpenGL::CglDepthFunc(depthFunc));
  }
}

/* ***************************** D E P T H   M A S K *************************** */

void gits::OpenGL::CVariableDepthMask::Get() {
  GLboolean& depthMask =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.depthMask;
  drv.gl.glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
}

void gits::OpenGL::CVariableDepthMask::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  GLboolean& depthMask =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.depthMask;

  if (depthMask != GL_TRUE) {
    scheduler.Register(new OpenGL::CglDepthMask(depthMask));
  }
}

/* **************************** D E P T H   R A N G E ************************** */

gits::OpenGL::CVariableDepthRange::CVariableDepthRange() {
  _value[0] = 0;
  _value[1] = 1;
}

void gits::OpenGL::CVariableDepthRange::Get() {
  drv.gl.glGetDoublev(GL_DEPTH_RANGE, _value);
}

void gits::OpenGL::CVariableDepthRange::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  const CVariableDepthRange& variable = static_cast<const CVariableDepthRange&>(lastValue);

  if (_value[0] != variable._value[0] || _value[1] != variable._value[1]) {
    scheduler.Register(new OpenGL::CglDepthRange(_value[0], _value[1]));
  }
}

/* **************************** D E P T H   R A N G E F ************************** */

gits::OpenGL::CVariableDepthRangef::CVariableDepthRangef() {
  _value[0] = 0;
  _value[1] = 1;
}

void gits::OpenGL::CVariableDepthRangef::Get() {
  drv.gl.glGetFloatv(GL_DEPTH_RANGE, _value);
}

void gits::OpenGL::CVariableDepthRangef::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  const CVariableDepthRangef& variable = static_cast<const CVariableDepthRangef&>(lastValue);

  if (_value[0] != variable._value[0] || _value[1] != variable._value[1]) {
    scheduler.Register(new OpenGL::CglDepthRangef(_value[0], _value[1]));
  }
}

/* **************************** D R A W   B U F F E R S ************************** */

gits::OpenGL::CVariableDrawBuffers::CVariableDrawBuffers() {}

void gits::OpenGL::CVariableDrawBuffers::Get() {
  GLint maxDrawBuffers;
  drv.gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
  std::vector<GLenum>& drawBuffers =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.drawBuffers;
  drawBuffers.clear();
  GLint fbo;
  drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
  for (int i = 0; i < maxDrawBuffers; ++i) {
    GLint drawBuffer;
    drv.gl.glGetIntegerv(GL_DRAW_BUFFER0 + i, &drawBuffer);
    if (fbo == 0 && drawBuffer == GL_BACK) {
      drawBuffers.push_back(GL_BACK_LEFT);
    } else {
      drawBuffers.push_back(drawBuffer);
    }
  }
}

void gits::OpenGL::CVariableDrawBuffers::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  std::vector<GLenum>& drawBuffers =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.drawBuffers;
  scheduler.Register(new CglDrawBuffers((GLsizei)drawBuffers.size(), &drawBuffers[0]));
}

/* ********************************** F O G ************************************ */

const gits::OpenGL::CVariableFog::TDataInfo gits::OpenGL::CVariableFog::_dataInfo[] = {
    {GL_FOG_MODE, 1},  {GL_FOG_DENSITY, 1}, {GL_FOG_START, 1},     {GL_FOG_END, 1},
    {GL_FOG_INDEX, 1}, {GL_FOG_COLOR, 4},   {GL_FOG_COORD_SRC, 1},
};

const unsigned gits::OpenGL::CVariableFog::_dataNum = sizeof(_dataInfo) / sizeof(TDataInfo);

gits::OpenGL::CVariableFog::CVariableFog() : _data(new GLfloat*[_dataNum]) {
  for (unsigned i = 0; i < _dataNum; i++) {
    _data[i] = new GLfloat[_dataInfo[i].num];
  }

  // GL_FOG_MODE
  _data[0][0] = GL_EXP;

  // GL_FOG_DENSITY
  _data[1][0] = 1;

  // GL_FOG_START
  _data[2][0] = 0;

  // GL_FOG_END
  _data[3][0] = 1;

  // GL_FOG_INDEX
  _data[4][0] = 0;

  // GL_FOG_COLOR
  _data[5][0] = 0;
  _data[5][1] = 0;
  _data[5][2] = 0;
  _data[5][3] = 0;

  // GL_FOG_COORD_SRC
  _data[6][0] = GL_FRAGMENT_DEPTH;
}

gits::OpenGL::CVariableFog::~CVariableFog() {
  for (unsigned i = 0; i < _dataNum; i++) {
    delete[] _data[i];
  }
  delete[] _data;
}

void gits::OpenGL::CVariableFog::Get() {
  for (unsigned i = 0; i < _dataNum; i++) {
    drv.gl.glGetFloatv(_dataInfo[i].name, _data[i]);
  }
}

void gits::OpenGL::CVariableFog::Schedule(CScheduler& scheduler, const CVariable& lastValue) const {
  const CVariableFog& variable = static_cast<const CVariableFog&>(lastValue);

  for (unsigned i = 0; i < _dataNum; i++) {
    if (memcmp(_data[i], variable._data[i], _dataInfo[i].num * sizeof(GLfloat))) {
      scheduler.Register(new OpenGL::CglFogfv(_dataInfo[i].name, _data[i]));
    }
  }
}

/* ***************************** F R O N T   F A C E *************************** */

void gits::OpenGL::CVariableFrontFace::Get() {
  GLint& frontFace =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.frontFace;
  drv.gl.glGetIntegerv(GL_FRONT_FACE, &frontFace);
}

void gits::OpenGL::CVariableFrontFace::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  GLint& frontFace =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.frontFace;

  if (frontFace != GL_CCW) {
    scheduler.Register(new OpenGL::CglFrontFace(frontFace));
  }
}

/* ********************************** H I N T ************************************ */
const gits::OpenGL::CVariableHint::TDataInfo gits::OpenGL::CVariableHint::_dataInfo[] = {
    {GL_FOG_HINT, GL_DONT_CARE},
    {GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE},
    {GL_LINE_SMOOTH_HINT, GL_DONT_CARE},
    {GL_PERSPECTIVE_CORRECTION_HINT, GL_DONT_CARE},
    {GL_POINT_SMOOTH_HINT, GL_DONT_CARE},
    {GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE},
    {GL_TEXTURE_COMPRESSION_HINT, GL_DONT_CARE},
    {GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE},
};

gits::OpenGL::CVariableHint::CVariableHint() {
  _data = new GLint[HINT_ARRAY_SIZE];
  for (unsigned i = 0; i < HINT_ARRAY_SIZE; ++i) {
    _data[i] = _dataInfo[i].mode;
  }
}

gits::OpenGL::CVariableHint::~CVariableHint() {
  delete[] _data;
}

void gits::OpenGL::CVariableHint::Get() {
  for (unsigned i = 0; i < HINT_ARRAY_SIZE; i++) {
    drv.gl.glGetIntegerv(_dataInfo[i].name, &_data[i]);
  }
}

void gits::OpenGL::CVariableHint::Schedule(CScheduler& scheduler,
                                           const CVariable& lastValue) const {
  const CVariableHint& variable = static_cast<const CVariableHint&>(lastValue);

  for (unsigned i = 0; i < HINT_ARRAY_SIZE; i++) {
    if (_data[i] != variable._data[i]) {
      scheduler.Register(new OpenGL::CglHint(_dataInfo[i].name, _data[i]));
    }
  }
}

/* ********************************** L I G H T     M O D E L ******************************** */

gits::OpenGL::CVariableLightModel::CVariableLightModel()
    : _colorControl(GL_SINGLE_COLOR), _localViewer(0), _twoSide(0) {
  std::fill_n(_ambient, 3, 0.2f);
  _ambient[3] = 1;
}

void gits::OpenGL::CVariableLightModel::Get() {
  drv.gl.glGetFloatv(GL_LIGHT_MODEL_AMBIENT, _ambient);
  drv.gl.glGetFloatv(GL_LIGHT_MODEL_COLOR_CONTROL, &_colorControl);
  drv.gl.glGetFloatv(GL_LIGHT_MODEL_LOCAL_VIEWER, &_localViewer);
  drv.gl.glGetFloatv(GL_LIGHT_MODEL_TWO_SIDE, &_twoSide);
}

float float_abs(float val) {
  return (val < 0) ? val * (-1) : val;
}

static bool floats_close(float lhs, float rhs) {
  return float_abs(lhs - rhs) < 0.01;
}

void gits::OpenGL::CVariableLightModel::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  const CVariableLightModel& variable = static_cast<const CVariableLightModel&>(lastValue);

  if (_colorControl != variable._colorControl) {
    scheduler.Register(new OpenGL::CglLightModelf(GL_LIGHT_MODEL_COLOR_CONTROL, _colorControl));
  }
  if (_localViewer != variable._localViewer) {
    scheduler.Register(new OpenGL::CglLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, _localViewer));
  }
  if (_twoSide != variable._twoSide) {
    scheduler.Register(new OpenGL::CglLightModelf(GL_LIGHT_MODEL_TWO_SIDE, _twoSide));
  }
  if (!std::equal(_ambient, _ambient + 4, variable._ambient, floats_close)) {
    scheduler.Register(new OpenGL::CglLightModelfv(GL_LIGHT_MODEL_AMBIENT, _ambient));
  }
}

/* ********************************** L I G H T ******************************** */

const gits::OpenGL::CVariableLight::TDataInfo gits::OpenGL::CVariableLight::_dataInfo[] = {
    {GL_AMBIENT, 4},
    {GL_DIFFUSE, 4},
    {GL_SPECULAR, 4},
    {GL_POSITION, 4},
    {GL_SPOT_DIRECTION, 3},
    {GL_SPOT_EXPONENT, 1},
    {GL_SPOT_CUTOFF, 1},
    {GL_CONSTANT_ATTENUATION, 1},
    {GL_LINEAR_ATTENUATION, 1},
    {GL_QUADRATIC_ATTENUATION, 1}};

const unsigned gits::OpenGL::CVariableLight::_dataNum = sizeof(_dataInfo) / sizeof(TDataInfo);

gits::OpenGL::CVariableLight::CVariableLight(GLenum light)
    : _light(light), _data(new GLfloat*[_dataNum]) {
  for (unsigned i = 0; i < _dataNum; i++) {
    _data[i] = new GLfloat[_dataInfo[i].num];
  }

  // GL_AMBIENT
  _data[0][0] = 0;
  _data[0][1] = 0;
  _data[0][2] = 0;
  _data[0][3] = 1;

  // GL_DIFFUSE
  _data[1][0] = _light == GL_LIGHT0 ? 1.0f : 0.0f;
  _data[1][1] = _light == GL_LIGHT0 ? 1.0f : 0.0f;
  _data[1][2] = _light == GL_LIGHT0 ? 1.0f : 0.0f;
  _data[1][3] = 1.0f;

  // GL_SPECULAR
  _data[2][0] = _light == GL_LIGHT0 ? 1.0f : 0.0f;
  _data[2][1] = _light == GL_LIGHT0 ? 1.0f : 0.0f;
  _data[2][2] = _light == GL_LIGHT0 ? 1.0f : 0.0f;
  _data[2][3] = 1.0f;

  // GL_POSITION
  _data[3][0] = 0;
  _data[3][1] = 0;
  _data[3][2] = 1;
  _data[3][3] = 0;

  // GL_SPOT_DIRECTION
  _data[4][0] = 0;
  _data[4][1] = 0;
  _data[4][2] = -1;

  // GL_SPOT_EXPONENT
  _data[5][0] = 0;

  // GL_SPOT_CUTOFF
  _data[6][0] = 180;

  // GL_CONSTANT_ATTENUATION
  _data[7][0] = 1;

  // GL_LINEAR_ATTENUATION
  _data[8][0] = 0;

  // GL_QUADRATIC_ATTENUATION
  _data[9][0] = 0;
}

gits::OpenGL::CVariableLight::~CVariableLight() {
  for (unsigned i = 0; i < _dataNum; i++) {
    delete[] _data[i];
  }
  delete[] _data;
}

void gits::OpenGL::CVariableLight::Get() {
  for (unsigned i = 0; i < _dataNum; i++) {
    drv.gl.glGetLightfv(_light, _dataInfo[i].name, _data[i]);
  }
}

void gits::OpenGL::CVariableLight::Schedule(CScheduler& scheduler,
                                            const CVariable& lastValue) const {
  const CVariableLight& variable = static_cast<const CVariableLight&>(lastValue);

  for (unsigned i = 0; i < _dataNum; i++) {
    if (memcmp(_data[i], variable._data[i], _dataInfo[i].num * sizeof(GLfloat)) != 0) {
      scheduler.Register(new OpenGL::CglLightfv(_light, _dataInfo[i].name, _data[i]));
    }
  }
}

/* ****************************** L I N E   S T I P P L E ********************** */

void gits::OpenGL::CVariableLineStipple::Get() {}

void gits::OpenGL::CVariableLineStipple::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  CGeneralStateData::Track::CLineStipple& lineStipple =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.lineStipple;
  if (lineStipple.used == true) {
    scheduler.Register(new OpenGL::CglLineStipple(lineStipple.factor, lineStipple.pattern));
  }
}

/* ****************************** L I N E   WIDTH ********************** */

void gits::OpenGL::CVariableLineWidth::Get() {
  GLfloat& lineWidth =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.lineWidth;
  drv.gl.glGetFloatv(GL_LINE_WIDTH, &lineWidth);
}

void gits::OpenGL::CVariableLineWidth::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  GLfloat& lineWidth =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.lineWidth;
  if (lineWidth != 1.0f) {
    scheduler.Register(new OpenGL::CglLineWidth(lineWidth));
  }
}

/* ****************************** L O G I C   O P E R A T I O N **************** */

void gits::OpenGL::CVariableLogicOp::Get() {
  GLint& logicOp = SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.logicOp;
  drv.gl.glGetIntegerv(GL_LOGIC_OP_MODE, &logicOp);
}

void gits::OpenGL::CVariableLogicOp::Schedule(CScheduler& scheduler,
                                              const CVariable& lastValue) const {
  GLint& logicOp = SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.logicOp;

  if (logicOp != GL_COPY) {
    scheduler.Register(new OpenGL::CglLogicOp(logicOp));
  }
}

/* ****************************** M A T R I X   M O D E ************************ */

gits::OpenGL::CVariableMatrixMode::CVariableMatrixMode() : _mode(GL_MODELVIEW) {}

void gits::OpenGL::CVariableMatrixMode::Get() {
  drv.gl.glGetIntegerv(GL_MATRIX_MODE, &_mode);
}

void gits::OpenGL::CVariableMatrixMode::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  const CVariableMatrixMode& variable = static_cast<const CVariableMatrixMode&>(lastValue);

  if (_mode != variable._mode) {
    scheduler.Register(new OpenGL::CglMatrixMode(_mode));
  }
}

/* ****************************** N O R M A L ********************************** */

gits::OpenGL::CVariableNormal::CVariableNormal() : _nx(0), _ny(0), _nz(1) {}

void gits::OpenGL::CVariableNormal::Get() {
  GLfloat ndata[3];
  drv.gl.glGetFloatv(GL_CURRENT_NORMAL, ndata);
  _nx = ndata[0];
  _ny = ndata[1];
  _nz = ndata[2];
}

void gits::OpenGL::CVariableNormal::Schedule(CScheduler& scheduler,
                                             const CVariable& lastValue) const {
  const CVariableNormal& variable = static_cast<const CVariableNormal&>(lastValue);

  if (_nx != variable._nx || _ny != variable._ny || _nz != variable._nz) {
    scheduler.Register(new OpenGL::CglNormal3f(_nx, _ny, _nz));
  }
}

/* ****************************** P I X E L   S T O R E ********************************** */

const gits::OpenGL::CVariablePixelStore::TDataInfo gits::OpenGL::CVariablePixelStore::_dataInfo[] =
    {{GL_PACK_SWAP_BYTES, 0},   {GL_PACK_LSB_FIRST, 0},     {GL_PACK_ROW_LENGTH, 0},
     {GL_PACK_IMAGE_HEIGHT, 0}, {GL_PACK_SKIP_ROWS, 0},     {GL_PACK_SKIP_PIXELS, 0},
     {GL_PACK_SKIP_IMAGES, 0},  {GL_PACK_ALIGNMENT, 4},     {GL_UNPACK_SWAP_BYTES, 0},
     {GL_UNPACK_LSB_FIRST, 0},  {GL_UNPACK_ROW_LENGTH, 0},  {GL_UNPACK_IMAGE_HEIGHT, 0},
     {GL_UNPACK_SKIP_ROWS, 0},  {GL_UNPACK_SKIP_PIXELS, 0}, {GL_UNPACK_SKIP_IMAGES, 0},
     {GL_UNPACK_ALIGNMENT, 4}};

gits::OpenGL::CVariablePixelStore::CVariablePixelStore() {
  for (unsigned i = 0; i < STORAGE_PARAMS_NUM; ++i) {
    _values[i] = _dataInfo[i].value;
  }
}

void gits::OpenGL::CVariablePixelStore::Get() {
  for (unsigned i = 0; i < STORAGE_PARAMS_NUM; ++i) {
    drv.gl.glGetIntegerv(_dataInfo[i].name, &_values[i]);
  }
}

void gits::OpenGL::CVariablePixelStore::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  const CVariablePixelStore& variable = static_cast<const CVariablePixelStore&>(lastValue);

  for (unsigned i = 0; i < STORAGE_PARAMS_NUM; ++i) {
    // GL_UNPACK_ALIGNMENT GL_UNPACK_ROW_LENGTH are modified by texture restore,
    // restore tham always
    if (variable._values[i] != _values[i] || _dataInfo[i].name == GL_UNPACK_ALIGNMENT ||
        _dataInfo[i].name == GL_UNPACK_ROW_LENGTH) {
      scheduler.Register(new OpenGL::CglPixelStorei(_dataInfo[i].name, _values[i]));
    }
  }
}

/* ****************************** P O I N T   S I Z E ************************** */

void gits::OpenGL::CVariablePointSize::Get() {
  GLfloat& pointSize =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.pointSize;
  drv.gl.glGetFloatv(GL_POINT_SIZE, &pointSize);
}

void gits::OpenGL::CVariablePointSize::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  GLfloat& pointSize =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.pointSize;

  if (pointSize != 1.0f) {
    scheduler.Register(new OpenGL::CglPointSize(pointSize));
  }
}

/* ************************* P O L Y G O N   O F F S E T *********************** */

void gits::OpenGL::CVariablePolygonOffset::Get() {
  CGeneralStateData::Restore::CPolygonOffset& polygonOffset =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.polygonOffset;
  drv.gl.glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &polygonOffset.factor);
  drv.gl.glGetFloatv(GL_POLYGON_OFFSET_UNITS, &polygonOffset.units);
}

void gits::OpenGL::CVariablePolygonOffset::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  CGeneralStateData::Restore::CPolygonOffset& polygonOffset =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.polygonOffset;

  if (polygonOffset.factor != 0 || polygonOffset.units != 0) {
    scheduler.Register(new OpenGL::CglPolygonOffset(polygonOffset.factor, polygonOffset.units));
  }
}

/* ************************* P O L Y G O N   S T I P P L E *********************** */

gits::OpenGL::CVariablePolygonStipple::CVariablePolygonStipple() {
  for (auto& byte : _mask) {
    byte = 0xFF;
  }
}

void gits::OpenGL::CVariablePolygonStipple::Get() {
  drv.gl.glGetPolygonStipple(_mask);
}

void gits::OpenGL::CVariablePolygonStipple::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  const CVariablePolygonStipple& variable = static_cast<const CVariablePolygonStipple&>(lastValue);

  if (memcmp(_mask, variable._mask, sizeof(_mask)) != 0) {
    scheduler.Register(new OpenGL::CglPolygonStipple(_mask));
  }
}

/* ****************************** S C I S S O R ******************************** */

gits::OpenGL::CVariableScissor::CVariableScissor() : _x(0), _y(0) {
  // this case is similar to viewport setting, we act in the same way
  _x = -1;
  _y = -1;
  _width = -1;
  _height = -1;
}

void gits::OpenGL::CVariableScissor::Get() {
  GLint scissorParams[SCISSOR_PARAMS_ARRAY_SIZE];
  drv.gl.glGetIntegerv(GL_SCISSOR_BOX, scissorParams);

  _x = scissorParams[0];
  _y = scissorParams[1];
  _width = scissorParams[2];
  _height = scissorParams[3];
}

void gits::OpenGL::CVariableScissor::Schedule(CScheduler& scheduler,
                                              const CVariable& lastValue) const {
  const CVariableScissor& variable = static_cast<const CVariableScissor&>(lastValue);

  if (_x != variable._x || _y != variable._y || _width != variable._width ||
      _height != variable._height) {
    scheduler.Register(new OpenGL::CglScissor(_x, _y, _width, _height));
  }
}

/* ****************************** S H A D E   M O D E L ********************** */

void gits::OpenGL::CVariableShadeModel::Get() {
  GLint& shaderModel =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.shaderModel;
  drv.gl.glGetIntegerv(GL_SHADE_MODEL, &shaderModel);
}

void gits::OpenGL::CVariableShadeModel::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  GLint& shaderModel =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.shaderModel;

  if (shaderModel != GL_SMOOTH) {
    scheduler.Register(new OpenGL::CglShadeModel(shaderModel));
  }
}

/* ****************************** S T E N C I L   F U N C ********************** */

gits::OpenGL::CVariableStencilFunc::CVariableStencilFunc()
    : _func(GL_ALWAYS), _ref(0), _mask(0), _funcBack(GL_ALWAYS), _refBack(0), _maskBack(0) {
  int maskBitsNum;

  drv.gl.glGetIntegerv(GL_STENCIL_BITS, &maskBitsNum);

  for (int i = 0; i < maskBitsNum; i++) {
    _mask |= (1 << i);
  }
  _maskBack = _mask;
}

void gits::OpenGL::CVariableStencilFunc::Get() {
  drv.gl.glGetIntegerv(GL_STENCIL_FUNC, &_func);
  drv.gl.glGetIntegerv(GL_STENCIL_REF, &_ref);
  drv.gl.glGetIntegerv(GL_STENCIL_VALUE_MASK, &_mask);

  if (SD().GetCurrentContextStateData().Version() >= 200) {
    drv.gl.glGetIntegerv(GL_STENCIL_BACK_FUNC, &_funcBack);
    drv.gl.glGetIntegerv(GL_STENCIL_BACK_REF, &_refBack);
    drv.gl.glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &_maskBack);
  }
}

void gits::OpenGL::CVariableStencilFunc::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  const CVariableStencilFunc& variable = static_cast<const CVariableStencilFunc&>(lastValue);

  if (SD().GetCurrentContextStateData().Version() >= 200) {
    bool frontModified = false;
    bool backModified = false;

    // there are 2 separate stencil buffers: FRONT and BACK
    if (_func != variable._func || _ref != variable._ref || _mask != variable._mask) {
      frontModified = true;
    }

    if (_funcBack != variable._funcBack || _refBack != variable._refBack ||
        _maskBack != variable._maskBack) {
      backModified = true;
    }

    if (frontModified && backModified && _func == _funcBack && _ref == _refBack &&
        _mask == _maskBack) {
      scheduler.Register(new OpenGL::CglStencilFunc(_func, _ref, _mask));
    } else {
      if (frontModified) {
        scheduler.Register(new OpenGL::CglStencilFuncSeparate(GL_FRONT, _func, _ref, _mask));
      }

      if (backModified) {
        scheduler.Register(
            new OpenGL::CglStencilFuncSeparate(GL_BACK, _funcBack, _refBack, _maskBack));
      }
    }
  } else {
    // there is only one stencil buffer
    if (_func != variable._func || _ref != variable._ref || _mask != variable._mask) {
      scheduler.Register(new OpenGL::CglStencilFunc(_func, _ref, _mask));
    }
  }
}

/* ****************************** S T E N C I L   O P ********************** */

gits::OpenGL::CVariableStencilOp::CVariableStencilOp()
    : _sfail(GL_KEEP),
      _dpfail(GL_KEEP),
      _dppass(GL_KEEP),
      _sfailBack(GL_KEEP),
      _dpfailBack(GL_KEEP),
      _dppassBack(GL_KEEP) {}

void gits::OpenGL::CVariableStencilOp::Get() {
  drv.gl.glGetIntegerv(GL_STENCIL_FAIL, &_sfail);
  drv.gl.glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &_dpfail);
  drv.gl.glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &_dppass);

  // in both GL2.0 and GLES2.0
  if (SD().GetCurrentContextStateData().Version() >= 200) {
    drv.gl.glGetIntegerv(GL_STENCIL_BACK_FAIL, &_sfailBack);
    drv.gl.glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &_dpfailBack);
    drv.gl.glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &_dppassBack);
  }
}

void gits::OpenGL::CVariableStencilOp::Schedule(CScheduler& scheduler,
                                                const CVariable& lastValue) const {
  const CVariableStencilOp& variable = static_cast<const CVariableStencilOp&>(lastValue);

  if (SD().GetCurrentContextStateData().Version() >= 200) {
    bool frontModified = false;
    bool backModified = false;

    // there are 2 separate stencil buffers: FRONT and BACK
    if (_sfail != variable._sfail || _dpfail != variable._dpfail || _dppass != variable._dppass) {
      frontModified = true;
    }

    if (_sfailBack != variable._sfailBack || _dpfailBack != variable._dpfailBack ||
        _dppassBack != variable._dppassBack) {
      backModified = true;
    }

    if (frontModified && backModified && _sfail == _sfailBack && _dpfail == _dpfailBack &&
        _dppass == _dppassBack) {
      scheduler.Register(new OpenGL::CglStencilOp(_sfail, _dpfail, _dppass));
    } else {
      if (frontModified) {
        scheduler.Register(new OpenGL::CglStencilOpSeparate(GL_FRONT, _sfail, _dpfail, _dppass));
      }

      if (backModified) {
        scheduler.Register(
            new OpenGL::CglStencilOpSeparate(GL_BACK, _sfailBack, _dpfailBack, _dppassBack));
      }
    }
  } else {
    // there is only one stencil buffer
    if (_sfail != variable._sfail || _dpfail != variable._dpfail || _dppass != variable._dppass) {
      scheduler.Register(new OpenGL::CglStencilOp(_sfail, _dpfail, _dppass));
    }
  }
}

/* ****************************** S T E N C I L   M A S K ********************** */

gits::OpenGL::CVariableStencilMask::CVariableStencilMask() : _mask(0) {
  int maskBitsNum;

  drv.gl.glGetIntegerv(GL_STENCIL_BITS, &maskBitsNum);

  for (int i = 0; i < maskBitsNum; i++) {
    _mask |= (1 << i);
  }
}

void gits::OpenGL::CVariableStencilMask::Get() {
  drv.gl.glGetIntegerv(GL_STENCIL_WRITEMASK, &_mask);
}

void gits::OpenGL::CVariableStencilMask::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  const CVariableStencilMask& variable = static_cast<const CVariableStencilMask&>(lastValue);

  if (_mask != variable._mask) {
    scheduler.Register(new OpenGL::CglStencilMask(_mask));
  }
}

/* ****************************** T E X T U R E   C O O R D I N A T E S ******** */

gits::OpenGL::CVariableTexCoord::CVariableTexCoord()
    : _originalUnit(GL_TEXTURE0), _maxTexCoords(0) {}

void gits::OpenGL::CVariableTexCoord::Get() {
  GLint tmp = GL_TEXTURE0;
  drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &tmp);
  _originalUnit = tmp;

  drv.gl.glGetIntegerv(GL_MAX_TEXTURE_COORDS, &_maxTexCoords);
  SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_s.clear();
  SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_t.clear();
  SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_r.clear();
  SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_q.clear();

  GLenum _textureUnit = GL_TEXTURE0;
  for (int i = 0; i < _maxTexCoords; i++) {
    _textureUnit = GL_TEXTURE0 + i;

    drv.gl.glActiveTexture(_textureUnit);

    GLfloat texCoordParams[TEXCOORD_PARAMS_ARRAY_SIZE];

    drv.gl.glGetFloatv(GL_CURRENT_TEXTURE_COORDS, texCoordParams);
    SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_s.push_back(
        texCoordParams[0]);
    SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_t.push_back(
        texCoordParams[1]);
    SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_r.push_back(
        texCoordParams[2]);
    SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_q.push_back(
        texCoordParams[3]);
  }
  // rollback to old texture unit
  drv.gl.glActiveTexture(_originalUnit);
}

void gits::OpenGL::CVariableTexCoord::Schedule(CScheduler& scheduler,
                                               const CVariable& lastValue) const {
  GLenum _textureUnit = GL_TEXTURE0;
  for (int i = 0; i < _maxTexCoords; i++) {
    _textureUnit = GL_TEXTURE0 + i;
    scheduler.Register(new OpenGL::CglActiveTexture(_textureUnit));
    GLfloat s =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_s[i];
    GLfloat t =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_t[i];
    GLfloat r =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_r[i];
    GLfloat q =
        SD().GetCurrentContextStateData().GeneralStateObjects().Data().restored.texCoord_q[i];

    scheduler.Register(new OpenGL::CglTexCoord4f(s, t, r, q));
  }
  // rollback to old texture unit
  scheduler.Register(new OpenGL::CglActiveTexture(_originalUnit));
}

/* ****************************** T E X   C O O R D   I N F O ************ */

gits::OpenGL::CVariableTexCoordInfo::TTexCoordData::TTexCoordData()
    : _size(4), _type(GL_FLOAT), _stride(0), _boundBuffer(0), _pointer(nullptr) {}

gits::OpenGL::CVariableTexCoordInfo::CVariableTexCoordInfo() {
  GLint maxTexCoords;
  GLenum query = curctx::IsEs1() ? GL_MAX_TEXTURE_UNITS : GL_MAX_TEXTURE_COORDS;
  drv.gl.glGetIntegerv(query, &maxTexCoords);
  _texCoords.resize(maxTexCoords);
}

void gits::OpenGL::CVariableTexCoordInfo::Get() {
  GLint clientActiveTexture;
  drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTexture);
  for (unsigned i = 0; i < _texCoords.size(); ++i) {
    drv.gl.glClientActiveTexture(GL_TEXTURE0 + i);
    drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &_texCoords[i]._size);
    drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE, &_texCoords[i]._type);
    drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE, &_texCoords[i]._stride);
    drv.gl.glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &_texCoords[i]._pointer);
    drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING, &_texCoords[i]._boundBuffer);
  }
  drv.gl.glClientActiveTexture(clientActiveTexture);
}

void gits::OpenGL::CVariableTexCoordInfo::Schedule(CScheduler& scheduler,
                                                   const CVariable& lastValue) const {
  const CVariableTexCoordInfo& variable = static_cast<const CVariableTexCoordInfo&>(lastValue);

  GLint clientActiveTexture = -1;
  drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTexture);
  GLint lastScheduledCAT = clientActiveTexture;
  for (unsigned i = 0; i < _texCoords.size(); ++i) {
    if (_texCoords[i] != variable._texCoords[i]) {
      // we need to alter GL state to retrieve correct buffer binding in
      // CglTexCoordPointer
      drv.gl.glClientActiveTexture(GL_TEXTURE0 + i);

      scheduler.Register(new OpenGL::CglBindBuffer(GL_ARRAY_BUFFER, _texCoords[i]._boundBuffer));
      lastScheduledCAT = GL_TEXTURE0 + i;
      scheduler.Register(new OpenGL::CglClientActiveTexture(lastScheduledCAT));
      scheduler.Register(new OpenGL::CglTexCoordPointer(
          _texCoords[i]._size, _texCoords[i]._type, _texCoords[i]._stride, _texCoords[i]._pointer,
          _texCoords[i]._boundBuffer));
    }
  }

  if (lastScheduledCAT != clientActiveTexture) {
    scheduler.Register(new OpenGL::CglClientActiveTexture(clientActiveTexture));
  }

  drv.gl.glClientActiveTexture(clientActiveTexture);
}

/* ****************************** B U F F E R    I N F O  ******************************** */
bool gits::OpenGL::CVariableBufferInfo::_supported = false;

gits::OpenGL::CVariableBufferInfo::CVariableBufferInfo() {
  _supported = (SD().GetCurrentContextStateData().Version() >= 150) ||
               drv.gl.HasExtension("GL_ARB_vertex_buffer_object") || !curctx::IsOgl();

  GLint param = 0;
  drv.gl.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &param);
}

void gits::OpenGL::CVariableBufferInfo::Get() {
  if (!_supported) {
    return;
  }

  BufferStateStash bufferStateStash;

  // get data of every known buffer
  auto iter = SD().GetCurrentSharedStateData().Buffers().List().begin();
  auto end = SD().GetCurrentSharedStateData().Buffers().List().end();

  for (; iter != end; ++iter) {
    if (iter->Name() == 0 || iter->Target() == 0) {
      continue;
    }
    // bind this buffer so we can obtain its data
    drv.gl.glBindBuffer(iter->Target(), iter->Name());

    if (iter->Size() == 0) {
      continue;
    }

    // Buffer data on ES is not restored if special option is not turned on
    if (!curctx::IsOgl() && ESBufferState() == TBuffersState::CAPTURE_ALWAYS) {
      continue;
    }

    std::vector<GLubyte>& buffer = GCC433WA_0(iter)->Buffer();
    buffer.resize(iter->Size());

    if (!iter->Data().restore.mapped) {
      if (!curctx::IsOgl()) {
        // We already checked that reading from mapped buffer is ok here.
        void* ptr = drv.gl.glMapBufferOES(iter->Target(), GL_WRITE_ONLY);
        memcpy(&buffer[0], ptr, iter->Size());
        drv.gl.glUnmapBufferOES(iter->Target());
      } else {
        drv.gl.glGetBufferSubData(iter->Target(), 0, iter->Size(), &buffer[0]);
      }
    } else {
      GLvoid* data_pointer;
      drv.gl.glGetBufferPointerv(iter->Target(), GL_BUFFER_MAP_POINTER, &data_pointer);
      memcpy(static_cast<void*>(&buffer[0]), data_pointer, iter->Size());
    }
  }
  bufferStateStash.Restore();
}

void gits::OpenGL::CVariableBufferInfo::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  auto list = SD().GetCurrentSharedStateData().Buffers().List();
  auto& indexedTargets = SD().GetCurrentSharedStateData().IndexedBoundBuffers().TargetsInfo();

  if (!_supported) {
    return;
  }

  // Schedule generic bound buffers
  for (const auto& currBuffer : list) {
    if (currBuffer.Name() == 0) {
      continue;
    }
    // Buffer data on ES is not restored if special option is not turned on
    if (!curctx::IsOgl() && ESBufferState() == TBuffersState::CAPTURE_ALWAYS) {
      continue;
    }

    // this buffer is new, or some changes in it occurred, we handle
    // both cases simultaneously for the sake of simplicity
    GLuint bufferName = currBuffer.Name();
    if (ESBufferState() == TBuffersState::RESTORE || curctx::IsOgl() ||
        IsGlGetTexAndCompressedTexImagePresentOnGLES()) {
      scheduler.Register(new OpenGL::CglGenBuffers(1, &bufferName));
    }
    if (currBuffer.Target() == 0) {
      continue;
    }
    scheduler.Register(new OpenGL::CglBindBuffer(currBuffer.Target(), currBuffer.Name()));
    std::vector<GLubyte>& buffer = GCC433WA_0(&currBuffer)->Buffer();
    // Do not restore content of not initialized buffers
    if (buffer.size() == 0) {
      continue;
    }
    if (ESBufferState() == TBuffersState::MIXED && !curctx::IsOgl() &&
        !IsGlGetTexAndCompressedTexImagePresentOnGLES()) {
      GCC433WA_0(&currBuffer)->TrackBufferData(0, currBuffer.Size(), &buffer[0]);
      scheduler.Register(
          new OpenGL::CglBufferSubData(currBuffer.Target(), 0, currBuffer.Size(), &buffer[0]));
    } else {
      GCC433WA_0(&currBuffer)->TrackBufferData(0, currBuffer.Size(), &buffer[0]);
      if (currBuffer.Immutable() == false) {
        scheduler.Register(new OpenGL::CglBufferData(currBuffer.Target(), currBuffer.Size(),
                                                     &buffer[0], currBuffer.Usage()));
      } else {
        scheduler.Register(new OpenGL::CglBufferStorage(currBuffer.Target(), currBuffer.Size(),
                                                        &buffer[0], currBuffer.Flags()));
      }
    }
    // Schedule mapped buffers
    if (currBuffer.Data().restore.mapped) {
      if ((currBuffer.Data().restore.mapAccess & GL_MAP_PERSISTENT_BIT) ||
          (currBuffer.Data().restore.mapAccess & GL_MAP_COHERENT_BIT)) {
        scheduler.Register(new OpenGL::CglMapBufferRange(
            (void*)true, currBuffer.Target(), currBuffer.Data().restore.mapOffset,
            currBuffer.Data().restore.mapLength, currBuffer.Data().restore.mapAccess));
      } else {
        scheduler.Register(
            new OpenGL::CglMapBufferARB((void*)true, currBuffer.Target(), GL_READ_WRITE));
      }
    }
  }

  // Schedule indexed bound buffers
  for (const auto& targetBoundBuffer : indexedTargets) {
    for (const auto& indexBoundBufer : targetBoundBuffer.second) {
      if (indexBoundBufer.second.size == 0) {
        if (indexBoundBufer.second.offset == 0) {
          scheduler.Register(new OpenGL::CglBindBufferBase(
              targetBoundBuffer.first, indexBoundBufer.first, indexBoundBufer.second.buffer));
        } else {
          scheduler.Register(new OpenGL::CglBindBufferOffsetEXT(
              targetBoundBuffer.first, indexBoundBufer.first, indexBoundBufer.second.buffer,
              indexBoundBufer.second.offset));
        }
      } else {
        scheduler.Register(new OpenGL::CglBindBufferRange(
            targetBoundBuffer.first, indexBoundBufer.first, indexBoundBufer.second.buffer,
            indexBoundBufer.second.offset, indexBoundBufer.second.size));
      }
    }
  }
}

/* ****************************** B U F F E R    B I N D I N G S  ******************************** */
gits::OpenGL::CVariableBufferBindings::CVariableBufferBindings() {}

void gits::OpenGL::CVariableBufferBindings::Get() {}

void gits::OpenGL::CVariableBufferBindings::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  auto& boundBuffers = SD().GetCurrentContextStateData().Bindings().BoundBuffers();
  auto& vertexArrays = SD().GetCurrentContextStateData().VertexArrays().List();

  // Check if VAO is used
  if (vertexArrays.begin() != vertexArrays.end()) {
    // When VAO is used, element array buffers and array buffers are restored
    // during VAO restoration, so don't override it here
    for (auto& elem : boundBuffers) {
      if (elem.first != GL_ELEMENT_ARRAY_BUFFER && elem.first != GL_ARRAY_BUFFER) {
        scheduler.Register(new OpenGL::CglBindBuffer(elem.first, elem.second));
      }
    }
  } else {
    for (auto& elem : boundBuffers) {
      scheduler.Register(new OpenGL::CglBindBuffer(elem.first, elem.second));
    }
  }
}

/* ****************************** G L S L   P R O G R A M    B I N D I N G S  ******************************** */
gits::OpenGL::CVariableGLSLBindings::CVariableGLSLBindings() {}

void gits::OpenGL::CVariableGLSLBindings::Get() {}

void gits::OpenGL::CVariableGLSLBindings::Schedule(CScheduler& scheduler,
                                                   const CVariable& lastValue) const {
  scheduler.Register(new CglUseProgram(SD().GetCurrentContextStateData().Bindings().GLSLProgram()));
  if (SD().GetCurrentContextStateData().GLSLPipelines().List().size() > 0) {
    scheduler.Register(
        new CglBindProgramPipeline(SD().GetCurrentContextStateData().Bindings().GLSLPipeline()));
  }
}

/* ****************************** T E X   E N V ******************************** */

const gits::OpenGL::CVariableTexEnv::TDataInfo gits::OpenGL::CVariableTexEnv::_scalarDataInfo[] =
    { // target, pname, default
        {GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE},
        {GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE},
        {GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE},
        {GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE},
        {GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS},
        {GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT},
        {GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE},
        {GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS},
        {GL_TEXTURE_ENV, GL_SRC2_ALPHA, GL_CONSTANT},
        {GL_TEXTURE_ENV, GL_RGB_SCALE, 1},
        {GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1},
        {GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR},
        {GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR},
        {GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA},
        {GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA},
        {GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA},
        {GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA},
        {GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0},
        {GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE}};

const unsigned gits::OpenGL::CVariableTexEnv::_dataNum =
    sizeof(gits::OpenGL::CVariableTexEnv::_scalarDataInfo) /
    sizeof(gits::OpenGL::CVariableTexEnv::TDataInfo);

/*
//special case - only vector data to be read from glGetTexEnv
const gits::OpenGL::CVariableTexEnv::TDataInfo
gits::OpenGL::CVariableTexEnv::_vectorDataInfo[] =
{
    { GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, {0, 0, 0, 0} }
}
*/

gits::OpenGL::CVariableTexEnv::CVariableTexEnv(GLenum textureUnit)
    : _data(new GLint[_dataNum]), _textureUnit(textureUnit), _originalUnit(GL_TEXTURE0) {
  // fill with defaults
  for (unsigned i = 0; i < _dataNum; ++i) {
    _data[i] = _scalarDataInfo[i].defValue;
  }
  // fill special case: { GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, {0, 0, 0, 0} }
  std::fill_n(_texEnvColor, 4, 0);

  _supported = drv.gl.glActiveTextureARB;
}

gits::OpenGL::CVariableTexEnv::~CVariableTexEnv() {
  delete[] _data;
}

void gits::OpenGL::CVariableTexEnv::Get() {
  if (!_supported) {
    return;
  }
  GLint tmp = GL_TEXTURE0;
  drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &tmp);
  _originalUnit = tmp;

  // select aproperiate texturing unit
  if (_textureUnit != _originalUnit) {
    drv.gl.glActiveTexture(_textureUnit);
  }

  for (unsigned i = 0; i < _dataNum; i++) {
    drv.gl.glGetTexEnviv(_scalarDataInfo[i].target, _scalarDataInfo[i].pname, &_data[i]);
  }
  drv.gl.glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, _texEnvColor);
  // rollback to old texture unit
  if (_textureUnit != _originalUnit) {
    drv.gl.glActiveTexture(_originalUnit);
  }
}

void gits::OpenGL::CVariableTexEnv::Schedule(CScheduler& scheduler,
                                             const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }
  const CVariableTexEnv& variable = static_cast<const CVariableTexEnv&>(lastValue);

  const bool texEnvColorEqual = std::equal(_texEnvColor, _texEnvColor + 4, variable._texEnvColor);
  const bool dataEqual = std::equal(_data, _data + _dataNum, variable._data);

  // this is done to prevent pointless texture unit changes
  if (!texEnvColorEqual || !dataEqual) {
    scheduler.Register(new OpenGL::CglActiveTexture(_textureUnit));
    if (!dataEqual) {
      for (unsigned i = 0; i < _dataNum; i++) {
        if (_data[i] != variable._data[i]) {
          scheduler.Register(new OpenGL::CglTexEnvi(_scalarDataInfo[i].target,
                                                    _scalarDataInfo[i].pname, _data[i]));
        }
      }
    }
    if (!texEnvColorEqual) {
      scheduler.Register(
          new OpenGL::CglTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, _texEnvColor));
    }
    scheduler.Register(new OpenGL::CglActiveTexture(_originalUnit));
  }
}

/* ****************************** V I E W P O R T ****************************** */

gits::OpenGL::CVariableViewport::CVariableViewport() : _x(0), _y(0) {
  // Default state of viewport depends on drawable for which it was created
  // by providing these dummy values, we effectively force viewport state
  // to be scheduled when compared against default state
  _x = -1;
  _y = -1;
  _width = -1;
  _height = -1;
}

void gits::OpenGL::CVariableViewport::Get() {
  GLint viewportParams[VIEWPORT_PARAMS_ARRAY_SIZE];
  drv.gl.glGetIntegerv(GL_VIEWPORT, viewportParams);

  _x = viewportParams[0];
  _y = viewportParams[1];
  _width = viewportParams[2];
  _height = viewportParams[3];
}

void gits::OpenGL::CVariableViewport::Schedule(CScheduler& scheduler,
                                               const CVariable& lastValue) const {
  const CVariableViewport& variable = static_cast<const CVariableViewport&>(lastValue);

  if (_x != variable._x || _y != variable._y || _width != variable._width ||
      _height != variable._height) {
#ifdef GITS_PLATFORM_WINDOWS
    HWND hwnd = WindowFromDC(drv.wgl.wglGetCurrentDC());
    int windowThread = CStateDynamicNative::Get().GetWindowThread(hwnd);
    int currentThread = CGits::Instance().CurrentThreadId();
    scheduler.Register(new CTokenMakeCurrentThreadNoCtxSwitch(windowThread));
#endif
    scheduler.Register(new OpenGL::CgitsViewportSettings(_x, _y, _width, _height));
#ifdef GITS_PLATFORM_WINDOWS
    scheduler.Register(new CTokenMakeCurrentThreadNoCtxSwitch(currentThread));
#endif
    scheduler.Register(new OpenGL::CglViewport(_x, _y, _width, _height));
  }
}

/* ****************************** P R I M I T I V E   R E S T A R T ************************** */

gits::OpenGL::CVariablePrimitiveRestartIndex::CVariablePrimitiveRestartIndex() : _index(0) {}

void gits::OpenGL::CVariablePrimitiveRestartIndex::Get() {
  drv.gl.glGetIntegerv(GL_PRIMITIVE_RESTART_INDEX, reinterpret_cast<GLint*>(&_index));
}

void gits::OpenGL::CVariablePrimitiveRestartIndex::Schedule(CScheduler& scheduler,
                                                            const CVariable& lastValue) const {
  scheduler.Register(new OpenGL::CglPrimitiveRestartIndex(_index));
}

/* ****************************** T E X T U R E     I N F O ************************** */

gits::OpenGL::CVariableTextureInfo::CVariableTextureInfo() {}

gits::OpenGL::CVariableTextureInfo::CTexture1D::CTexture1D(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTexture1D::~CTexture1D() {}

void gits::OpenGL::CVariableTextureInfo::CTexture1D::GetTextureLevelsDataGL() {
  // obtain pixels
  Texture1DData().pixels.resize(MipmapData().size());

  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    std::vector<char> pixels;
    if (currMip.compressed == GL_TRUE) {
      pixels.resize(currMip.compressedImageSize);
      drv.gl.glGetCompressedTexImage(Target(), i, &pixels[0]);
    } else {
      pixels.resize(TexelSize(currMip.format, currMip.type) * currMip.width * currMip.height *
                    currMip.depth);
      drv.gl.glGetTexImage(Target(), i, currMip.format, currMip.type, &pixels[0]);
    }

    Texture1DData().pixels.at(i) =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &pixels[0], pixels.size());
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture1D::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  // set data, for the rest of mip-maps to be set
  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData()[i];

    if (currMip.compressed == GL_TRUE) {
      scheduler.Register(new CglCompressedTexImage1D(
          Target(), i, currMip.internalFormat, currMip.width, currMip.border,
          currMip.compressedImageSize, Texture1DData().pixels.at(i)));
    } else {
      scheduler.Register(new CglTexImage1D(Target(), i, currMip.internalFormat, currMip.width,
                                           currMip.border, currMip.format, currMip.type,
                                           Texture1DData().pixels.at(i)));
    }
  }
}

gits::OpenGL::CVariableTextureInfo::CTexture2D::CTexture2D(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTexture2D::~CTexture2D() {}

void gits::OpenGL::CVariableTextureInfo::CTexture2D::GetTextureLevelsDataGL() {
  // Obtain pixels.
  Texture2DData().pixels.resize(MipmapData().size());

  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    std::vector<char> data;
    if (currMip.compressed == GL_TRUE && currMip.compressedImageSize > 0) {
      data.resize(currMip.compressedImageSize);
      drv.gl.glGetCompressedTexImage(Target(), i, &data[0]);
    } else {
      auto format = currMip.format;
#ifdef GITS_PLATFORM_WINDOWS
      bool restoreIndexedTexturesWA = Config::Get().opengl.recorder.restoreIndexedTexturesWA;
      if (currMip.internalFormat == GL_RGBA && currMip.format == GL_COLOR_INDEX &&
          restoreIndexedTexturesWA) {
        format = currMip.internalFormat;
      }
#endif

      int size = TexelSize(format, currMip.type) * currMip.width * currMip.height * currMip.depth;
      data.resize((size > 0) ? size : 1);
      drv.gl.glGetTexImage(Target(), i, format, currMip.type, &data[0]);
    }

    Texture2DData().pixels.at(i) =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &data[0], data.size());
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture2D::GetTextureLevelsDataGLES() {
  // Compressed textures data are tracked on OGLES
  if (MipmapData().at(0).compressed == GL_TRUE) {
    return;
  }

  CTextureStateObj* texStateData =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(TextureId(), Target()));

  // Check if texture tied with eglImage
  // Content of EGLImage is not restored in current implementation.We are
  // assuming that it was not changed since last call to eglCreateImageKHR
  if (texStateData == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  // Prepare generic FBO APIs
  auto genFramebuffers = drv.gl.glGenFramebuffers;
  auto deleteFramebuffers = drv.gl.glDeleteFramebuffers;
  auto bindFramebuffer = drv.gl.glBindFramebuffer;
  auto framebufferTexture2D = drv.gl.glFramebufferTexture2D;
  auto checkFramebufferStatus = drv.gl.glCheckFramebufferStatus;
  if (drv.gl.HasExtension("GL_OES_framebuffer_object")) {
    genFramebuffers = drv.gl.glGenFramebuffersOES;
    deleteFramebuffers = drv.gl.glDeleteFramebuffersOES;
    bindFramebuffer = drv.gl.glBindFramebufferOES;
    framebufferTexture2D = drv.gl.glFramebufferTexture2DOES;
    checkFramebufferStatus = drv.gl.glCheckFramebufferStatusOES;
  }

  GLuint tmpFbo = 0;
  FboBindStateStash fboBindStateStash;
  genFramebuffers(1, &tmpFbo);
  bindFramebuffer(GL_FRAMEBUFFER, tmpFbo);

  // Get levels data
  Texture2DData().pixels.resize(MipmapData().size());
  int level = 0;
  bool& readable = texStateData->Data().restore.isESTexReadable;
  for (auto& mipmap : MipmapData()) {
    framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Target(), TextureId(), level);
    GLenum fboStatus = checkFramebufferStatus(GL_FRAMEBUFFER);
    bool fboComplete = fboStatus == GL_FRAMEBUFFER_COMPLETE;

    if (level == 0) {
      readable = fboComplete && AreESPixelsReadable(mipmap.format, mipmap.type);
    }

    if (readable) {
      std::vector<char> data;
      unsigned storeDataSize = TexelSize(mipmap.format, mipmap.type) * mipmap.width * mipmap.height;

      data.resize(storeDataSize);
      drv.gl.glReadPixels(0, 0, mipmap.width, mipmap.height, mipmap.format, mipmap.type, &data[0]);
      Texture2DData().pixels.at(level) =
          CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &data[0], data.size());
    } else {
      Log(WARN) << "Texture: " << TextureId() << " level: " << level
                << " content will not be restored.";
    }

    level++;
  }

  deleteFramebuffers(1, &tmpFbo);
  fboBindStateStash.Restore();
}

void gits::OpenGL::CVariableTextureInfo::CTexture2D::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  if (GenericData().immutable != GL_FALSE) {
    scheduler.Register(new CglTexStorage2D(Target(), (GLsizei)MipmapData().size(),
                                           MipmapData().at(0).internalFormat,
                                           MipmapData().at(0).width, MipmapData().at(0).height));
  }

  // set data, for the rest of mip-maps to be set
  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);
    if (currMip.compressed == GL_TRUE) {
      if (GenericData().immutable != GL_FALSE) {
        scheduler.Register(new CglCompressedTexSubImage2D(
            Target(), i, 0, 0, currMip.width, currMip.height, currMip.internalFormat,
            currMip.compressedImageSize, Texture2DData().pixels.at(i)));
      } else {
        scheduler.Register(new CglCompressedTexImage2D(
            Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.border,
            currMip.compressedImageSize, Texture2DData().pixels.at(i)));
      }
    } else {
#ifdef GITS_PLATFORM_WINDOWS
      bool restoreIndexedTexturesWA = Config::Get().opengl.recorder.restoreIndexedTexturesWA;
#endif
      if (GenericData().immutable != GL_FALSE) {
        scheduler.Register(new CglTexSubImage2D(Target(), i, 0, 0, currMip.width, currMip.height,
                                                currMip.format, currMip.type,
                                                Texture2DData().pixels.at(i)));
#ifdef GITS_PLATFORM_WINDOWS
      } else if (currMip.internalFormat == GL_RGBA && currMip.format == GL_COLOR_INDEX &&
                 restoreIndexedTexturesWA) {
        scheduler.Register(new CglTexImage2D(Target(), i, currMip.internalFormat, currMip.width,
                                             currMip.height, currMip.border, currMip.internalFormat,
                                             currMip.type, Texture2DData().pixels.at(i)));
#endif
      } else {
        scheduler.Register(new CglTexImage2D(Target(), i, currMip.internalFormat, currMip.width,
                                             currMip.height, currMip.border, currMip.format,
                                             currMip.type, Texture2DData().pixels.at(i)));
      }
    }
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture2D::ScheduleSameTargetTextureGLES(
    CScheduler& scheduler) const {
  CTextureStateObj* texStateData =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(TextureId(), Target()));

  // Check if texture tied with eglImage
  if (texStateData == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  // Set data, for the rest of mip-maps to be set
  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    if (currMip.compressed == GL_TRUE && Config::Get().opengl.recorder.texturesState ==
                                             TTexturesState::RESTORE) { // Compressed texture
      scheduler.Register(new CglCompressedTexImage2D(
          Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.border,
          currMip.compressedImageSize, Texture2DData().pixels.at(i)));
    } // Non compressed readable texture
    else if (texStateData->Data().restore.isESTexReadable) {
      if (Config::Get().opengl.recorder.texturesState == TTexturesState::MIXED) {
        scheduler.Register(new CglTexSubImage2D(Target(), i, 0, 0, currMip.width, currMip.height,
                                                currMip.format, currMip.type,
                                                Texture2DData().pixels.at(i)));
      } else if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
        scheduler.Register(new CglTexImage2D(Target(), i, currMip.internalFormat, currMip.width,
                                             currMip.height, currMip.border, currMip.format,
                                             currMip.type, Texture2DData().pixels.at(i)));
      }
    }
    // Non compressed, non readable texture and restore Textures mode
    // in restore Textures mode unreadable textures are initialized with
    // default content. It may cause corruptions.
    else if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
      scheduler.Register(new CglTexImage2D(Target(), i, currMip.internalFormat, currMip.width,
                                           currMip.height, currMip.border, currMip.format,
                                           currMip.type, nullptr));
      Log(WARN) << "Texture " << TextureId()
                << " not restored. Visual corruptions possible. In case of corruptions try another "
                   "textures recording mode.";
    }
  }
}

gits::OpenGL::CVariableTextureInfo::CTextureExternal::CTextureExternal(GLint target,
                                                                       GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTextureExternal::~CTextureExternal() {}

void gits::OpenGL::CVariableTextureInfo::CTextureExternal::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  CTextureStateObj* texStateData =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(TextureId(), Target()));
  if (texStateData == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  } else {
    scheduler.Register(
        new CglEGLImageTargetTexture2DOES(Target(), texStateData->Data().track.eglImage));
  }
}

gits::OpenGL::CVariableTextureInfo::CTexture2DMultisample::CTexture2DMultisample(GLint target,
                                                                                 GLint textureId)
    : CTexture(target, textureId) {}

void gits::OpenGL::CVariableTextureInfo::CTexture2DMultisample::GetTextureLevelsDataGL() {}

void gits::OpenGL::CVariableTextureInfo::CTexture2DMultisample::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  const TMipmapTextureData& currMip = MipmapData().at(0);

  scheduler.Register(new CglTexImage2DMultisample(
      Target(), currMip.samples, currMip.internalFormat, currMip.width, currMip.height,
      currMip.fixedsamplelocations ? GL_TRUE : GL_FALSE));
}

gits::OpenGL::CVariableTextureInfo::CTexture2DMultisampleArray::CTexture2DMultisampleArray(
    GLint target, GLint textureId)
    : CTexture(target, textureId) {}

void gits::OpenGL::CVariableTextureInfo::CTexture2DMultisampleArray::GetTextureLevelsDataGL() {}

void gits::OpenGL::CVariableTextureInfo::CTexture2DMultisampleArray::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  const TMipmapTextureData& currMip = MipmapData().at(0);

  scheduler.Register(new CglTexImage3DMultisample(
      Target(), currMip.samples, currMip.internalFormat, currMip.width, currMip.height,
      currMip.depth, currMip.fixedsamplelocations ? GL_TRUE : GL_FALSE));
}

gits::OpenGL::CVariableTextureInfo::CTexture1DArray::CTexture1DArray(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTexture1DArray::~CTexture1DArray() {}

void gits::OpenGL::CVariableTextureInfo::CTexture1DArray::GetTextureLevelsDataGL() {
  // obtain pixels
  Texture1DArrayData().pixels.resize(MipmapData().size());

  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    std::vector<char> pixels;
    if (currMip.compressed == GL_TRUE) {
      pixels.resize(currMip.compressedImageSize);
      drv.gl.glGetCompressedTexImage(Target(), i, &pixels[0]);
    } else {
      pixels.resize(TexelSize(currMip.format, currMip.type) * currMip.width * currMip.height *
                    currMip.depth);
      drv.gl.glGetTexImage(Target(), i, currMip.format, currMip.type, &pixels[0]);
    }

    Texture1DArrayData().pixels.at(i) =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &pixels[0], pixels.size());
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture1DArray::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  if (GenericData().immutable != GL_FALSE) {
    scheduler.Register(new CglTexStorage2D(Target(), (GLsizei)MipmapData().size(),
                                           MipmapData().at(0).internalFormat,
                                           MipmapData().at(0).width, MipmapData().at(0).height));
  }

  // set data, for the rest of mip-maps to be set
  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);
    if (currMip.compressed == GL_TRUE) {
      if (GenericData().immutable != GL_FALSE) {
        scheduler.Register(new CglCompressedTexSubImage2D(
            Target(), i, 0, 0, currMip.width, currMip.height, currMip.internalFormat,
            currMip.compressedImageSize, Texture1DArrayData().pixels.at(i)));
      } else {
        scheduler.Register(new CglCompressedTexImage2D(
            Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.border,
            currMip.compressedImageSize, Texture1DArrayData().pixels.at(i)));
      }
    } else {
      if (GenericData().immutable != GL_FALSE) {
        scheduler.Register(new CglTexSubImage2D(Target(), i, 0, 0, currMip.width, currMip.height,
                                                currMip.format, currMip.type,
                                                Texture1DArrayData().pixels.at(i)));
      } else {
        scheduler.Register(new CglTexImage2D(Target(), i, currMip.internalFormat, currMip.width,
                                             currMip.height, currMip.border, currMip.format,
                                             currMip.type, Texture1DArrayData().pixels.at(i)));
      }
    }
  }
}

gits::OpenGL::CVariableTextureInfo::CTexture2DArray::CTexture2DArray(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTexture2DArray::~CTexture2DArray() {}

void gits::OpenGL::CVariableTextureInfo::CTexture2DArray::GetTextureLevelsDataGL() {
  // obtain pixels
  Texture2DArrayData().pixels.resize(MipmapData().size());

  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    std::vector<char> pixels;
    if (currMip.compressed == GL_TRUE) {
      pixels.resize(currMip.compressedImageSize);
      drv.gl.glGetCompressedTexImage(Target(), i, &pixels[0]);
    } else {
      pixels.resize(TexelSize(currMip.format, currMip.type) * currMip.width * currMip.height *
                    currMip.depth);
      drv.gl.glGetTexImage(Target(), i, currMip.format, currMip.type, &pixels[0]);
    }

    Texture2DArrayData().pixels.at(i) =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &pixels[0], pixels.size());
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture2DArray::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  // set data, for the rest of mip-maps to be set
  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    if (currMip.compressed == GL_TRUE) {
      scheduler.Register(new CglCompressedTexImage3D(
          Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.depth,
          currMip.border, currMip.compressedImageSize, Texture2DArrayData().pixels.at(i)));
    } else {
      scheduler.Register(new CglTexImage3D(
          Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.depth,
          currMip.border, currMip.format, currMip.type, Texture2DArrayData().pixels.at(i)));
    }
  }
}

gits::OpenGL::CVariableTextureInfo::CTexture3D::CTexture3D(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTexture3D::~CTexture3D() {}

void gits::OpenGL::CVariableTextureInfo::CTexture3D::GetTextureLevelsDataGL() {
  // obtain pixels
  Texture3DData().pixels.resize(MipmapData().size());

  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    std::vector<char> pixels;
    if (currMip.compressed == GL_TRUE) {
      pixels.resize(currMip.compressedImageSize);
      drv.gl.glGetCompressedTexImage(Target(), i, &pixels[0]);
    } else {
      pixels.resize(std::max(TexelSize(currMip.format, currMip.type) * currMip.width *
                                 currMip.height * currMip.depth,
                             1u));
      drv.gl.glGetTexImage(Target(), i, currMip.format, currMip.type, &pixels[0]);
    }

    Texture3DData().pixels.at(i) =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &pixels[0], pixels.size());
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture3D::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  // set data, for the rest of mip-maps to be set
  for (unsigned i = 0; i < MipmapData().size(); ++i) {
    const TMipmapTextureData& currMip = MipmapData().at(i);

    if (currMip.compressed == GL_TRUE) {
      scheduler.Register(new CglCompressedTexImage3D(
          Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.depth,
          currMip.border, currMip.compressedImageSize, Texture3DData().pixels.at(i)));
    } else {
      scheduler.Register(new CglTexImage3D(
          Target(), i, currMip.internalFormat, currMip.width, currMip.height, currMip.depth,
          currMip.border, currMip.format, currMip.type, Texture3DData().pixels.at(i)));
    }
  }
}

gits::OpenGL::CVariableTextureInfo::CTextureCube::CTextureCube(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

gits::OpenGL::CVariableTextureInfo::CTextureCube::~CTextureCube() {}

void gits::OpenGL::CVariableTextureInfo::CTextureCube::GetTextureLevelsDataGL() {
  // obtain data
  const unsigned cubeFaces = 6;

  for (unsigned j = 0; j < cubeFaces; ++j) {
    // obtain pixels
    TextureCubeData().pixels[j].resize(MipmapData().size());

    for (unsigned i = 0; i < MipmapData().size(); ++i) {
      const TMipmapTextureData& currMip = MipmapData().at(i);

      // Do not proceed with faulty textures
      if (currMip.width != 0) {
        std::vector<char> pixels;
        if (currMip.compressed == GL_TRUE) {
          pixels.resize(currMip.compressedImageSize);
          drv.gl.glGetCompressedTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, &pixels[0]);
        } else {
          pixels.resize(TexelSize(currMip.format, currMip.type) * currMip.width * currMip.height *
                        currMip.depth);
          drv.gl.glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, currMip.format, currMip.type,
                               &pixels[0]);
        }

        TextureCubeData().pixels[j].at(i) =
            CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &pixels[0], pixels.size());
      }
    }
  }
}

void gits::OpenGL::CVariableTextureInfo::CTextureCube::GetTextureLevelsDataGLES() {
  // Compressed textures data are tracked on OGLES
  if (MipmapData().at(0).compressed == GL_TRUE) {
    return;
  }

  CTextureStateObj* texStateData =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(TextureId(), Target()));

  // Prepare generic FBO APIs
  auto genFramebuffers = drv.gl.glGenFramebuffers;
  auto deleteFramebuffers = drv.gl.glDeleteFramebuffers;
  auto bindFramebuffer = drv.gl.glBindFramebuffer;
  auto framebufferTexture2D = drv.gl.glFramebufferTexture2D;
  auto checkFramebufferStatus = drv.gl.glCheckFramebufferStatus;
  if (drv.gl.HasExtension("GL_OES_framebuffer_object")) {
    genFramebuffers = drv.gl.glGenFramebuffersOES;
    deleteFramebuffers = drv.gl.glDeleteFramebuffersOES;
    bindFramebuffer = drv.gl.glBindFramebufferOES;
    framebufferTexture2D = drv.gl.glFramebufferTexture2DOES;
    checkFramebufferStatus = drv.gl.glCheckFramebufferStatusOES;
  }

  // Prepare temporary FBO
  GLuint tmpFbo = 0;

  genFramebuffers(1, &tmpFbo);
  bindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
  bool& readable = texStateData->Data().restore.isESTexReadable;
  for (auto& hashes : TextureCubeData().pixels) {
    // obtain pixels
    hashes.resize(MipmapData().size());

    for (unsigned i = 0; i < MipmapData().size(); ++i) {
      const TMipmapTextureData& mipmap = MipmapData().at(i);

      // Do not proceed with faulty textures
      if (mipmap.width != 0) {
        framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Target(), TextureId(), i);
        GLenum fboStatus = checkFramebufferStatus(GL_FRAMEBUFFER);
        bool fboComplete = fboStatus == GL_FRAMEBUFFER_COMPLETE;

        if (i == 0) {
          readable = fboComplete && AreESPixelsReadable(mipmap.format, mipmap.type);
        }

        if (readable) {
          std::vector<char> data;
          unsigned storeDataSize =
              TexelSize(mipmap.format, mipmap.type) * mipmap.width * mipmap.height;
          data.resize(storeDataSize);

          drv.gl.glReadPixels(0, 0, mipmap.width, mipmap.height, mipmap.format, mipmap.type,
                              &data[0]);
          hashes.at(i) =
              CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &data[0], data.size());
        } else {
          Log(WARN) << "Texture: " << TextureId() << " content will not be restored.";
        }
      }
    }
  }
  deleteFramebuffers(1, &tmpFbo);
}

void gits::OpenGL::CVariableTextureInfo::CTextureCube::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  const unsigned cubeFaces = 6;
  if (GenericData().immutable != GL_FALSE) {
    scheduler.Register(new CglTexStorage2D(GL_TEXTURE_CUBE_MAP, (GLsizei)MipmapData().size(),
                                           MipmapData().at(0).internalFormat,
                                           MipmapData().at(0).width, MipmapData().at(0).height));
  }

  // set data, for the rest of mipmaps to be set
  for (unsigned j = 0; j < cubeFaces; ++j) {
    for (unsigned i = 0; i < MipmapData().size(); ++i) {
      const TMipmapTextureData& currMip = MipmapData().at(i);

      // Do not proceed with faulty textures
      if (currMip.width != 0) {
        if (currMip.compressed == GL_TRUE) {
          if (GenericData().immutable != GL_FALSE) {
            scheduler.Register(new CglCompressedTexSubImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, 0, 0, currMip.width, currMip.height,
                currMip.internalFormat, currMip.compressedImageSize,
                TextureCubeData().pixels[j].at(i)));
          } else {
            scheduler.Register(new CglCompressedTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, currMip.internalFormat, currMip.width,
                currMip.height, currMip.border, currMip.compressedImageSize,
                TextureCubeData().pixels[j].at(i)));
          }
        } else {
          if (GenericData().immutable != GL_FALSE) {
            scheduler.Register(new CglTexSubImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, 0, 0, currMip.width, currMip.height,
                currMip.format, currMip.type, TextureCubeData().pixels[j].at(i)));
          } else {
            scheduler.Register(new CglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i,
                                                 currMip.internalFormat, currMip.width,
                                                 currMip.height, currMip.border, currMip.format,
                                                 currMip.type, TextureCubeData().pixels[j].at(i)));
          }
        }
      }
    }
  }
}

void gits::OpenGL::CVariableTextureInfo::CTextureCube::ScheduleSameTargetTextureGLES(
    CScheduler& scheduler) const {
  CTextureStateObj* texStateData =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(TextureId(), Target()));

  const unsigned cubeFaces = 6;
  // set data, for the rest of mipmaps to be set
  for (unsigned j = 0; j < cubeFaces; ++j) {
    for (unsigned i = 0; i < MipmapData().size(); ++i) {
      const TMipmapTextureData& currMip = MipmapData().at(i);

      // Do not proceed with faulty textures
      if (currMip.width != 0) {
        // Compressed texture and restore Textures mode
        if (currMip.compressed == GL_TRUE &&
            Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          scheduler.Register(new CglCompressedTexImage2D(
              GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, currMip.internalFormat, currMip.width,
              currMip.height, currMip.border, currMip.compressedImageSize,
              TextureCubeData().pixels[j].at(i)));
        } // Non compressed readable texture
        else if (texStateData->Data().restore.isESTexReadable) {
          if (Config::Get().opengl.recorder.texturesState == TTexturesState::MIXED) {
            scheduler.Register(new CglTexSubImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, 0, 0, currMip.width, currMip.height,
                currMip.format, currMip.type, TextureCubeData().pixels[j].at(i)));
          } else if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
            scheduler.Register(new CglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i,
                                                 currMip.internalFormat, currMip.width,
                                                 currMip.height, currMip.border, currMip.format,
                                                 currMip.type, TextureCubeData().pixels[j].at(i)));
          }
        }
        // Non compressed, non readable texture and restore Textures mode
        // in restore Textures mode unreadable textures are initialized with
        // default content. It may cause corruptions.
        else if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE) {
          scheduler.Register(new CglTexImage2D(
              GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, currMip.internalFormat, currMip.width,
              currMip.height, currMip.border, currMip.format, currMip.type, nullptr));
          Log(WARN) << "Texture " << TextureId()
                    << " not restored. Visual corruptions possible. In case of corruptions try "
                       "another textures recording mode.";
        }
      }
    }
  }
}

gits::OpenGL::CVariableTextureInfo::CTextureBuffer::CTextureBuffer(GLint target, GLint textureId)
    : CTexture(target, textureId) {}

void gits::OpenGL::CVariableTextureInfo::CTextureBuffer::GetTextureLevelsDataGL() {
  CTextureStateObj* texStateData =
      SD().GetCurrentSharedStateData().Textures().Get(CTextureStateObj(TextureId(), Target()));

  TextureBufferData().internalformat = texStateData->Data().track.texbuffer_internalformat;
  TextureBufferData().buffer = texStateData->Data().track.texbuffer_buffer;

  // TODO - The lines below can be uncommented when implementation for
  // glTexBufferRange and glTextureBufferRangeEXT is added. This functions are
  // part of ARB_texture_buffer_range extension.

  // drv.gl.glGetTexLevelParameteriv( GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_OFFSET, &_offset );
  // drv.gl.glGetTexLevelParameteriv( GL_TEXTURE_BUFFER, 0, GL_TEXTURE_BUFFER_SIZE, &_size );
}

void gits::OpenGL::CVariableTextureInfo::CTextureBuffer::ScheduleSameTargetTextureGL(
    CScheduler& scheduler) const {
  if (TextureBufferData().offset == 0 && TextureBufferData().internalformat != 0) {
    scheduler.Register(new CglTexBuffer(GL_TEXTURE_BUFFER, TextureBufferData().internalformat,
                                        TextureBufferData().buffer));
  }
  // TODO - The lines below can be uncommented when implementation for
  // glTexBufferRange and glTextureBufferRangeEXT is added. This functions are
  // part of ARB_texture_buffer_range extension.
  /*else
    scheduler.Register( new CglTexBufferRange(GL_TEXTURE_BUFFER,
    _internalformat, _buffer, _offset, _size) );*/
}

unsigned gits::OpenGL::CVariableTextureInfo::CTexture::MipmapCount(GLenum target) {
  int tex_level = 0;
  if (target == GL_TEXTURE_RECTANGLE) {
    tex_level = 1;
  } else {
    // Only check max texture size once.
    // We should probably move this, together with other checks to library
    // to avoid local static state.
    static int max_tex_level = 0;
    if (max_tex_level == 0) {
      GLint max_tex_size = 0;
      drv.gl.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
      max_tex_level = log2i(max_tex_size);
    }
    tex_level = max_tex_level;
  }

  int max_defined = 0;
  // Each defined level has at least non-zero width (applies to all texture types).
  for (int i = 0; i < tex_level; ++i) {
    GLint width = 0;
    drv.gl.glGetTexLevelParameteriv(target, i, GL_TEXTURE_WIDTH, &width);
    if (width != 0) {
      max_defined = i + 1;
    }
  }

  return max_defined;
}

gits::OpenGL::CVariableTextureInfo::CTexture::CTexture(GLint target, GLint id)
    : _target(target), _id(id) {}

GLint gits::OpenGL::CVariableTextureInfo::CTexture::Target() const {
  return _target;
}

GLuint gits::OpenGL::CVariableTextureInfo::CTexture::TextureId() const {
  return _id;
}

void gits::OpenGL::CVariableTextureInfo::CTexture::GetTextureGenericData(GLenum target) {
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_MAG_FILTER, &GenericData().magFilter);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_MIN_FILTER, &GenericData().minFilter);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_WRAP_S, &GenericData().wrapS);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_WRAP_T, &GenericData().wrapT);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_WRAP_R, &GenericData().wrapR);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_BORDER_COLOR, GenericData().borderColor);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_MIN_LOD, &GenericData().minLod);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_MAX_LOD, &GenericData().maxLod);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_BASE_LEVEL, &GenericData().baseLevel);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &GenericData().maxLevel);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_IMMUTABLE_FORMAT, &GenericData().immutable);
  drv.gl.glGetTexParameteriv(target, GL_DEPTH_TEXTURE_MODE, &GenericData().textureMode);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_COMPARE_MODE, &GenericData().compareMode);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_COMPARE_FUNC, &GenericData().compareFunc);
  drv.gl.glGetTexParameteriv(target, GL_GENERATE_MIPMAP, &GenericData().generateMipmap);
  drv.gl.glGetTexParameterfv(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, &GenericData().maxAnisotropy);
  drv.gl.glGetTexParameteriv(target, GL_TEXTURE_SRGB_DECODE_EXT, &GenericData().srgbDecode);

  if (drv.gl.HasExtension("GL_EXT_texture_swizzle") ||
      (curctx::IsOgl() && curctx::Version() >= 330) || curctx::IsEs3Plus()) {
    drv.gl.glGetTexParameteriv(target, GL_TEXTURE_SWIZZLE_R_EXT, &GenericData().swizzleR);
    drv.gl.glGetTexParameteriv(target, GL_TEXTURE_SWIZZLE_G_EXT, &GenericData().swizzleG);
    drv.gl.glGetTexParameteriv(target, GL_TEXTURE_SWIZZLE_B_EXT, &GenericData().swizzleB);
    drv.gl.glGetTexParameteriv(target, GL_TEXTURE_SWIZZLE_A_EXT, &GenericData().swizzleA);
  }

  if (curctx::IsEs1()) {
    drv.gl.glGetTexParameteriv(target, GL_TEXTURE_CROP_RECT_OES,
                               &GenericData().textureCropRectOES[0]);
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture::ScheduleTextureGenericData(
    GLenum target, CScheduler& scheduler, const CTextureData& defaultTexture) const {
  if (GenericData().magFilter != defaultTexture.genericData.magFilter) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_MAG_FILTER, &GenericData().magFilter));
  }
  if (GenericData().minFilter != defaultTexture.genericData.minFilter) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_MIN_FILTER, &GenericData().minFilter));
  }
  if (GenericData().wrapS != defaultTexture.genericData.wrapS) {
    scheduler.Register(new CglTexParameteriv(target, GL_TEXTURE_WRAP_S, &GenericData().wrapS));
  }
  if (GenericData().wrapT != defaultTexture.genericData.wrapT) {
    scheduler.Register(new CglTexParameteriv(target, GL_TEXTURE_WRAP_T, &GenericData().wrapT));
  }
  if (GenericData().wrapR != defaultTexture.genericData.wrapR) {
    scheduler.Register(new CglTexParameteriv(target, GL_TEXTURE_WRAP_R, &GenericData().wrapR));
  }
  if (!std::equal(GenericData().borderColor, GenericData().borderColor + 4,
                  defaultTexture.genericData.borderColor)) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_BORDER_COLOR, GenericData().borderColor));
  }
  if (GenericData().minLod != defaultTexture.genericData.minLod) {
    scheduler.Register(new CglTexParameteriv(target, GL_TEXTURE_MIN_LOD, &GenericData().minLod));
  }
  if (GenericData().maxLod != defaultTexture.genericData.maxLod) {
    scheduler.Register(new CglTexParameteriv(target, GL_TEXTURE_MAX_LOD, &GenericData().maxLod));
  }
  if (GenericData().baseLevel != defaultTexture.genericData.baseLevel) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_BASE_LEVEL, &GenericData().baseLevel));
  }
  if (GenericData().maxLevel != defaultTexture.genericData.maxLevel) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &GenericData().maxLevel));
  }
  if (GenericData().textureMode != defaultTexture.genericData.textureMode) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_DEPTH_TEXTURE_MODE, &GenericData().textureMode));
  }
  if (GenericData().compareMode != defaultTexture.genericData.compareMode) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_COMPARE_MODE, &GenericData().compareMode));
  }
  if (GenericData().compareFunc != defaultTexture.genericData.compareFunc) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_COMPARE_FUNC, &GenericData().compareFunc));
  }
  if (GenericData().generateMipmap != defaultTexture.genericData.generateMipmap) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_GENERATE_MIPMAP, &GenericData().generateMipmap));
  }
  if (GenericData().swizzleR != defaultTexture.genericData.swizzleR) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_SWIZZLE_R_EXT, &GenericData().swizzleR));
  }
  if (GenericData().swizzleG != defaultTexture.genericData.swizzleG) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_SWIZZLE_G_EXT, &GenericData().swizzleG));
  }
  if (GenericData().swizzleB != defaultTexture.genericData.swizzleB) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_SWIZZLE_B_EXT, &GenericData().swizzleB));
  }
  if (GenericData().swizzleA != defaultTexture.genericData.swizzleA) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_SWIZZLE_A_EXT, &GenericData().swizzleA));
  }
  if (GenericData().memoryLayout != defaultTexture.genericData.memoryLayout) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_SWIZZLE_A_EXT, &GenericData().memoryLayout));
  }
  if (GenericData().maxAnisotropy != defaultTexture.genericData.maxAnisotropy) {
    scheduler.Register(
        new CglTexParameterfv(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, &GenericData().maxAnisotropy));
  }
  if (GenericData().srgbDecode != defaultTexture.genericData.srgbDecode) {
    scheduler.Register(
        new CglTexParameteriv(target, GL_TEXTURE_SRGB_DECODE_EXT, &GenericData().srgbDecode));
  }
  if (curctx::IsEs1()) {
    if (GenericData().textureCropRectOES != defaultTexture.genericData.textureCropRectOES) {
      scheduler.Register(new CglTexParameteriv(target, GL_TEXTURE_CROP_RECT_OES,
                                               &GenericData().textureCropRectOES[0]));
    }
  }
}

bool gits::OpenGL::CVariableTextureInfo::CTexture::AreESPixelsReadable(GLenum format, GLenum type) {
  // Depth and stencil formats are not readable via glReadPixels
  if (isDepthFormat(format) || isStencilFormat(format)) {
    return false;
  }

  // Check basic glReadPixels format and type
  if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
    return true;
  }

  // Check implementation dependent glReadPixels format and type
  GLint implColorFormat = 0;
  GLint implColorType = 0;
  drv.gl.glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &implColorFormat);
  drv.gl.glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &implColorType);
  if (format == (GLenum)implColorFormat && type == (GLenum)implColorType) {
    return true;
  } else {
    return false;
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture::Get() {
  drv.gl.glBindTexture(Target(), TextureId());
  GetTextureGenericData(Target());
  // Textures content restoration works for OGL and in case of GL_TEXTURE_2D and
  // GL_TEXTURE_1D also for OGLES
  if (curctx::IsOgl() ||
      (IsGlGetTexImagePresentOnGLES() && MipmapData().at(0).compressed == GL_FALSE) ||
      (IsGlGetCompressedTexImagePresentOnGLES() && MipmapData().at(0).compressed == GL_TRUE)) {
    GetTextureLevelsDataGL();
  } else if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE ||
             (Config::Get().opengl.recorder.texturesState == TTexturesState::MIXED &&
              Target() == GL_TEXTURE_2D)) {
    GetTextureLevelsDataGLES();
  }
}

void gits::OpenGL::CVariableTextureInfo::CTexture::Schedule(
    CScheduler& scheduler, const CTextureData& defaultTexture) const {
  GLuint texture = TextureId();
  if (texture &&
      (curctx::IsOgl() || Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE)) {
    // Textures with id 0 are present by default for each target.
    scheduler.Register(new CglGenTextures(1, &texture));
  }
  scheduler.Register(new CglBindTexture(Target(), TextureId()));
  ScheduleTextureGenericData(Target(), scheduler, defaultTexture);

  bool isMapped =
      SD().GetCurrentSharedStateData().GetMappedTextures().CheckIfTextureIsMapped(TextureId());
  if (isMapped) {
    scheduler.Register(
        new CglTexParameteri(Target(), GL_TEXTURE_MEMORY_LAYOUT_INTEL, GL_LAYOUT_LINEAR_INTEL));
  }
  if (curctx::IsOgl() ||
      (IsGlGetTexImagePresentOnGLES() && MipmapData().at(0).compressed == GL_FALSE) ||
      (IsGlGetCompressedTexImagePresentOnGLES() && MipmapData().at(0).compressed == GL_TRUE)) {
    ScheduleSameTargetTextureGL(scheduler);
  } else if (Config::Get().opengl.recorder.texturesState == TTexturesState::RESTORE ||
             (Config::Get().opengl.recorder.texturesState == TTexturesState::MIXED &&
              Target() == GL_TEXTURE_2D)) {
    ScheduleSameTargetTextureGLES(scheduler);
  }

  if (isMapped) {
    scheduler.Register(
        new CglTexParameteri(Target(), GL_TEXTURE_MEMORY_LAYOUT_INTEL, GL_LAYOUT_DEFAULT_INTEL));
  }
}

unsigned gits::OpenGL::CVariableTextureInfo::CTexture::TexelSize(GLenum format, GLenum type) {
  return texelSize(format, type);
}

bool gits::OpenGL::CVariableTextureInfo::IsNonZeroIdUnBound(GLuint id) const {
  // if texture was bound with failed result it won't be reported by
  // glIsTexture - in such case we need to just skip this texture
  // texture 0 is always present, even though it is not reported
  // as texture by glIsTexture
  return (id != 0 && !drv.gl.glIsTexture(id));
}

void gits::OpenGL::CVariableTextureInfo::Get() {
  GLint origBind[11];

  // get the original binding for each target
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_1D, &origBind[0]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &origBind[1]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_3D, &origBind[2]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &origBind[3]);
  drv.gl.glGetIntegerv(GL_TEXTURE_RECTANGLE_ARB, &origBind[4]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_1D_ARRAY, &origBind[5]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &origBind[6]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &origBind[7]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY, &origBind[8]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &origBind[9]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, &origBind[10]);

  GLint origPackAlignment = 0;
  drv.gl.glGetIntegerv(GL_PACK_ALIGNMENT, &origPackAlignment);
  drv.gl.glPixelStorei(GL_PACK_ALIGNMENT, 1);

  GLint origPackRowLength = 0;
  drv.gl.glGetIntegerv(GL_PACK_ROW_LENGTH, &origPackRowLength);
  drv.gl.glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  CSharedStateDynamic::CTextures::CListType::const_iterator it =
      SD().GetCurrentSharedStateData().Textures().List().begin();
  CSharedStateDynamic::CTextures::CListType::const_iterator itEnd =
      SD().GetCurrentSharedStateData().Textures().List().end();

  // Pixel pack buffer can't be bound while using glGetTexImage
  GLint origPbo = 0;
  if (curctx::IsOgl()) {
    drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &origPbo);
    if (origPbo != 0) {
      drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
  }

  for (; it != itEnd; ++it) {
    // if texture was bound with failed result it won't be reported by
    // glIsTexture - in such case we need to just skip this texture
    // texture 0 is always present, even though it is not reported
    // as texture by glIsTexture
    if (IsNonZeroIdUnBound(it->Name())) {
      continue;
    }

    if (GCC433WA_0(it)->Data().track.mipmapData.size() == 0 && it->Target() != GL_TEXTURE_BUFFER) {
      continue; // Skip uninitialized texture
    }

    CTexture* texture = nullptr;
    // create texture object
    switch (it->Target()) {
    case GL_TEXTURE_1D:
      texture = new CTexture1D(it->Target(), it->Name());
      break;
    case GL_TEXTURE_1D_ARRAY:
      texture = new CTexture1DArray(it->Target(), it->Name());
      break;
    case GL_TEXTURE_2D:
      texture = new CTexture2D(it->Target(), it->Name());
      break;
    case GL_TEXTURE_2D_MULTISAMPLE:
      texture = new CTexture2DMultisample(it->Target(), it->Name());
      break;
    case GL_TEXTURE_2D_ARRAY:
      texture = new CTexture2DArray(it->Target(), it->Name());
      break;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      texture = new CTexture2DMultisampleArray(it->Target(), it->Name());
      break;
    case GL_TEXTURE_3D:
    case GL_TEXTURE_CUBE_MAP_ARRAY:
      texture = new CTexture3D(it->Target(), it->Name());
      break;
    case GL_TEXTURE_CUBE_MAP:
      texture = new CTextureCube(it->Target(), it->Name());
      break;
    case GL_TEXTURE_RECTANGLE_ARB:
      // CTexture2D is generic enough to handle rectangle textures
      texture = new CTexture2D(it->Target(), it->Name());
      break;
    case GL_TEXTURE_EXTERNAL_OES:
      // CTexture2D is generic enough to handle rectangle textures
      texture = new CTextureExternal(it->Target(), it->Name());
      break;
    case GL_TEXTURE_BUFFER:
      texture = new CTextureBuffer(it->Target(), it->Name());
      break;
    default:
      // future texture types go here. Future types will also have to be added
      // in GetBindingEnum. textures 0 will come here anyways, skip them
      if (it->Name() == 0) {
        continue;
      }
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    // obtain data
    _textures.insert(texture);
    texture->Get();
  }

  if (origPbo != 0) {
    drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, origPbo);
  }

  // restore pack alignment
  drv.gl.glPixelStorei(GL_PACK_ALIGNMENT, origPackAlignment);
  drv.gl.glPixelStorei(GL_PACK_ROW_LENGTH, origPackRowLength);

  // restore original bindings
  if (origBind[0]) {
    drv.gl.glBindTexture(GL_TEXTURE_1D, origBind[0]);
  }
  if (origBind[1]) {
    drv.gl.glBindTexture(GL_TEXTURE_2D, origBind[1]);
  }
  if (origBind[2]) {
    drv.gl.glBindTexture(GL_TEXTURE_3D, origBind[2]);
  }
  if (origBind[3]) {
    drv.gl.glBindTexture(GL_TEXTURE_CUBE_MAP, origBind[3]);
  }
  if (origBind[4]) {
    drv.gl.glBindTexture(GL_TEXTURE_RECTANGLE_ARB, origBind[4]);
  }
  if (origBind[5]) {
    drv.gl.glBindTexture(GL_TEXTURE_1D_ARRAY, origBind[5]);
  }
  if (origBind[6]) {
    drv.gl.glBindTexture(GL_TEXTURE_2D_ARRAY, origBind[6]);
  }
  if (origBind[7]) {
    drv.gl.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, origBind[7]);
  }
  if (origBind[8]) {
    drv.gl.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, origBind[8]);
  }
  if (origBind[9]) {
    drv.gl.glBindTexture(GL_TEXTURE_BINDING_BUFFER, origBind[9]);
  }
  if (origBind[10]) {
    drv.gl.glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, origBind[10]);
  }
}

gits::OpenGL::CVariableTextureInfo::~CVariableTextureInfo() {
  // delete all texture objects
  for (auto& elem : _textures) {
    delete elem;
  }
}

void gits::OpenGL::CVariableTextureInfo::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  // store state at witch we obtain textures
  scheduler.Register(new CglPixelStorei(GL_UNPACK_ALIGNMENT, 1));
  scheduler.Register(new CglPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
  scheduler.Register(new CglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

  // Schedule glGenTexture for those textures which are generated but not yet
  // bound
  for (const auto& elem : SD().GetCurrentSharedStateData().Textures().List()) {
    GLuint textureId = elem.Name();

    if (IsNonZeroIdUnBound(textureId)) {
      scheduler.Register(new CglGenTextures(1, &textureId));
    }
  }

  // Schedule state of changed textures
  for (const auto& elem : _textures) {
    // This will create CTextureStateData with default values for each
    // attribute, used for comparison
    CTextureStateData defaultTexture(0, elem->Target());
    elem->Schedule(scheduler, *defaultTexture.restore.ptr);
  }
}

/* *********************** T E X T U R E    B I N D I N G ************************* */

gits::OpenGL::CVariableTextureBinding::CVariableTextureBinding(GLenum texture) : _texture(texture) {
  for (auto& binding : _bindingArray) {
    binding = 0;
  }
}

void gits::OpenGL::CVariableTextureBinding::Get() {
  GLint oldTexture;
  drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &oldTexture);
  _oldTexture = oldTexture;
  drv.gl.glActiveTexture(_texture);

  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_1D, &_bindingArray[0]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, &_bindingArray[1]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_3D, &_bindingArray[2]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &_bindingArray[3]);
  // drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_ARB, &_bindingArray[4]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &_bindingArray[5]);
  // drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_EXT, &_bindingArray[6]);
  // drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_NV, &_bindingArray[7]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &_bindingArray[8]);
  drv.gl.glGetIntegerv(GL_TEXTURE_BUFFER, &_bindingArray[9]);

  if (oldTexture != int(_texture)) {
    drv.gl.glActiveTexture(oldTexture);
  }
}

void gits::OpenGL::CVariableTextureBinding::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  const CVariableTextureBinding& variable = static_cast<const CVariableTextureBinding&>(lastValue);

  if (!std::equal(_bindingArray, _bindingArray + BINDING_ARRAY_SIZE, variable._bindingArray)) {
    scheduler.Register(new OpenGL::CglActiveTexture(_texture));

    if (_bindingArray[0] != variable._bindingArray[0]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_1D, _bindingArray[0]));
    }
    if (_bindingArray[1] != variable._bindingArray[1]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_2D, _bindingArray[1]));
    }
    if (_bindingArray[2] != variable._bindingArray[2]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_3D, _bindingArray[2]));
    }
    if (_bindingArray[3] != variable._bindingArray[3]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_CUBE_MAP, _bindingArray[3]));
    }
    //  if(_bindingArray[4] != variable._bindingArray[4])
    //    scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_CUBE_MAP_ARB,  _bindingArray[4]));
    if (_bindingArray[5] != variable._bindingArray[5]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_RECTANGLE_ARB, _bindingArray[5]));
    }
    // if(_bindingArray[6] != variable._bindingArray[6])
    //    scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_CUBE_MAP_EXT,  _bindingArray[6]));
    //  if(_bindingArray[7] != variable._bindingArray[7])
    //    scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_RECTANGLE_NV,  _bindingArray[7]));
    if (_bindingArray[8] != variable._bindingArray[8]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_2D_ARRAY_EXT, _bindingArray[8]));
    }
    if (_bindingArray[9] != variable._bindingArray[9]) {
      scheduler.Register(new OpenGL::CglBindTexture(GL_TEXTURE_BUFFER, _bindingArray[9]));
    }

    scheduler.Register(new OpenGL::CglActiveTexture(_oldTexture));
  }
}

///* ************************ B I N D   I M A G E   T E X T U R E ****************** */

bool gits::OpenGL::CVariableBindImageTextureInfo::_supported = false;

gits::OpenGL::CVariableBindImageTextureInfo::TImageTextureData::TImageTextureData()
    : name(0), level(0), layered(GL_FALSE), layer(0), access(GL_READ_ONLY), format(GL_R8) {}

gits::OpenGL::CVariableBindImageTextureInfo::CVariableBindImageTextureInfo() {
  if ((curctx::IsOgl() && curctx::Version() >= 420) || curctx::IsEs31Plus()) {
    _supported = true;

    GLint maxImageUnits;
    drv.gl.glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImageUnits);
    _imageTextures.resize(maxImageUnits);
  }
}

void gits::OpenGL::CVariableBindImageTextureInfo::Get() {
  if (!_supported) {
    return;
  }

  for (GLuint i = 0; i < _imageTextures.size(); i++) {
    drv.gl.glGetIntegeri_v(GL_IMAGE_BINDING_NAME, i, (GLint*)&_imageTextures[i].name);
    drv.gl.glGetIntegeri_v(GL_IMAGE_BINDING_LEVEL, i, &_imageTextures[i].level);
    drv.gl.glGetBooleani_v(GL_IMAGE_BINDING_LAYERED, i, &_imageTextures[i].layered);
    drv.gl.glGetIntegeri_v(GL_IMAGE_BINDING_LAYER, i, &_imageTextures[i].layer);
    drv.gl.glGetIntegeri_v(GL_IMAGE_BINDING_ACCESS, i, (GLint*)&_imageTextures[i].access);
    drv.gl.glGetIntegeri_v(GL_IMAGE_BINDING_FORMAT, i, (GLint*)&_imageTextures[i].format);
  }
}

void gits::OpenGL::CVariableBindImageTextureInfo::Schedule(CScheduler& scheduler,
                                                           const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  const CVariableBindImageTextureInfo& variable =
      static_cast<const CVariableBindImageTextureInfo&>(lastValue);
  for (GLuint i = 0; i < _imageTextures.size(); i++) {
    if (_imageTextures[i].name != variable._imageTextures[i].name ||
        _imageTextures[i].level != variable._imageTextures[i].level ||
        _imageTextures[i].layered != variable._imageTextures[i].layered ||
        _imageTextures[i].layer != variable._imageTextures[i].layer ||
        _imageTextures[i].access != variable._imageTextures[i].access ||
        _imageTextures[i].format != variable._imageTextures[i].format) {
      scheduler.Register(new CglBindImageTexture(
          i, _imageTextures[i].name, _imageTextures[i].level, _imageTextures[i].layered,
          _imageTextures[i].layer, _imageTextures[i].access, _imageTextures[i].format));
    }
  }
}

/* *************************** P O L Y G O N    M O D E ************************** */
gits::OpenGL::CVariablePolygonMode::CVariablePolygonMode() : _front(GL_FILL), _back(GL_FILL) {}

void gits::OpenGL::CVariablePolygonMode::Get() {
  GLint buffer[2];
  drv.gl.glGetIntegerv(GL_POLYGON_MODE, buffer);
  _front = buffer[0];
  _back = buffer[1];
}

void gits::OpenGL::CVariablePolygonMode::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  const CVariablePolygonMode& variable = static_cast<const CVariablePolygonMode&>(lastValue);

  if (variable._front != _front && variable._back != _back) {
    scheduler.Register(new OpenGL::CglPolygonMode(GL_FRONT_AND_BACK, _front));
  } else {
    if (variable._front != _front) {
      scheduler.Register(new OpenGL::CglPolygonMode(GL_FRONT, _front));
    }
    if (variable._back != _back) {
      scheduler.Register(new OpenGL::CglPolygonMode(GL_BACK, _back));
    }
  }
}

/* *************************** P R O G A M    I N F O ************************** */

gits::OpenGL::CVariableProgramInfo::CVariableProgramInfo() {
  _supported = drv.gl.HasExtension("GL_ARB_vertex_program");
}

gits::OpenGL::CVariableProgramInfo::~CVariableProgramInfo() {
  std::map<GLint, const TProgramData*>::iterator it;
  for (auto& elem : _programMap) {
    delete[] elem.second->_string;
    delete elem.second;
  }
}

void gits::OpenGL::CVariableProgramInfo::Get() {
  if (!_supported) {
    return;
  }

  GLint origBind[2];

  // get the original binding for each target
  drv.gl.glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &origBind[0]);
  drv.gl.glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &origBind[1]);

  // obtain program env parameters
  for (auto& env : SD().GetCurrentSharedStateData().ARBProgEnvParams().List()) {
    drv.gl.glGetProgramEnvParameterfvARB(env.Target(), env.Name(),
                                         _envParams[env.Target()][env.Name()].p);
  }

  // Obtain programs params
  for (auto& prog : SD().GetCurrentSharedStateData().ARBPrograms().List()) {
    TProgramData* data = new TProgramData;
    // bind the texture with the given ID to obtain it's params
    drv.gl.glBindProgramARB(prog.Target(), prog.Name());

    data->_id = prog.Name();
    data->_target = prog.Target();
    drv.gl.glGetProgramivARB(prog.Target(), GL_PROGRAM_LENGTH_ARB, &data->_strLen);

    data->_string = new GLbyte[data->_strLen + 1];
    drv.gl.glGetProgramStringARB(prog.Target(), GL_PROGRAM_STRING_ARB, data->_string);

    // obtain program local parameters
    for (auto localId : prog.LocalParametersUsed()) {
      drv.gl.glGetProgramLocalParameterfvARB(prog.Target(), localId, data->_params[localId].p);
    }

    _programMap[prog.Name()] = data;
  }

  // restore original bindings
  if (origBind[0]) {
    drv.gl.glBindProgramARB(GL_VERTEX_PROGRAM_ARB, origBind[0]);
  }
  if (origBind[1]) {
    drv.gl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, origBind[1]);
  }
}

void gits::OpenGL::CVariableProgramInfo::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  const CVariableProgramInfo& variable = static_cast<const CVariableProgramInfo&>(lastValue);

  CProgramMap lastMapCopy = variable._programMap;
  const TProgramData* currData;

  // todo: check for repeating parameters
  for (auto& envElem : _envParams) {
    GLenum target = envElem.first;
    for (auto& envParamElem : envElem.second) {
      GLuint index = envParamElem.first;
      const float* p = envParamElem.second.p;
      scheduler.Register(new CglProgramEnvParameter4fvARB(target, index, p));
    }
  }

  for (auto& progElem : _programMap) {
    currData = progElem.second;

    std::map<GLint, const TProgramData*>::iterator old = lastMapCopy.find(progElem.first);
    if (old != lastMapCopy.end()) {
      const TProgramData* oldData;
      oldData = old->second;
      std::list<CFunction*> functionList;
      if (currData->_target != oldData->_target) {
        // delete old binding and add new one - this has to be done if target is
        // changed
        functionList.push_back(new OpenGL::CglDeleteProgramsARB(1, &currData->_id));
      }
      if (currData->_strLen != oldData->_strLen ||
          (currData->_string &&
           memcmp(currData->_string, oldData->_string, (currData->_strLen * sizeof(GLbyte))))) {
        functionList.push_back(new OpenGL::CglProgramStringARB(
            currData->_target, GL_PROGRAM_FORMAT_ASCII_ARB, currData->_strLen, currData->_string));
      }

      // @todo: should any paramamters be acquired?
      if (!functionList.empty()) {
        scheduler.Register(new OpenGL::CglBindProgramARB(currData->_target, currData->_id));
        for (auto func : functionList) {
          scheduler.Register(func);
        }
      }

      // erase aready checked entry from old map - the remaining maps
      // will be checked seperatebly
      lastMapCopy.erase(progElem.first);
    } else if (currData->_strLen != 0) {
      // the previous state doesn't contain such a binding;
      // create it & set parameters
      scheduler.Register(new OpenGL::CglBindProgramARB(currData->_target, currData->_id));
      scheduler.Register(new OpenGL::CglProgramStringARB(
          currData->_target, GL_PROGRAM_FORMAT_ASCII_ARB, currData->_strLen, currData->_string));
      // schedule all parameters (todo: check for same values, and skip these
      // params)
      for (const auto& paramElem : currData->_params) {
        const float* p = paramElem.second.p;
        scheduler.Register(
            new CglProgramLocalParameter4fvARB(currData->_target, paramElem.first, p));
      }
    }
  }

  if (!lastMapCopy.empty()) {
    for (const auto& resiElem : lastMapCopy) {
      // remove textures that we present in the last state
      // but are no longer used
      scheduler.Register(new OpenGL::CglDeleteProgramsARB(1, &resiElem.second->_id));
    }
  }
}

/* *********************** P R O G R A M   B I N D I N G ************************* */

gits::OpenGL::CVariableProgramBinding::CVariableProgramBinding() {
  _supported = drv.gl.glBindProgramARB && drv.gl.glGetProgramivARB;

  for (auto& binding : _bindingArray) {
    binding = 0;
  }
}

void gits::OpenGL::CVariableProgramBinding::Get() {
  if (!_supported) {
    return;
  }

  drv.gl.glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &_bindingArray[0]);
  drv.gl.glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_BINDING_ARB, &_bindingArray[1]);
}

void gits::OpenGL::CVariableProgramBinding::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  const CVariableProgramBinding& variable = static_cast<const CVariableProgramBinding&>(lastValue);

  if (_bindingArray[0] != variable._bindingArray[0]) {
    scheduler.Register(new OpenGL::CglBindProgramARB(GL_VERTEX_PROGRAM_ARB, _bindingArray[0]));
  }
  if (_bindingArray[1] != variable._bindingArray[1]) {
    scheduler.Register(new OpenGL::CglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _bindingArray[1]));
  }
}

/**************************************** B L E N D   E Q U A T I O N **************************************/

void gits::OpenGL::CVariableBlendEquation::Get() {}

void gits::OpenGL::CVariableBlendEquation::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  CGeneralStateData::Track::CBlendEquation& blendEquation =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.blendEquation;
  CGeneralStateData::Track::CBlendEquationSeparate& blendEquationSeparate =
      SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.blendEquationSeparate;
  if (blendEquation.used == true) {
    scheduler.Register(new OpenGL::CglBlendEquation(blendEquation.mode));
  } else if (blendEquationSeparate.used == true) {
    scheduler.Register(new CglBlendEquationSeparate(blendEquationSeparate.modeRGB,
                                                    blendEquationSeparate.modeAlpha));
  }
}

/**************************************** G L S L   I N F O ************************************************/

gits::OpenGL::CVariableGLSLInfo::CVariableGLSLInfo() {}

void gits::OpenGL::CVariableGLSLInfo::Get() {
  static const int MAX_UNIFORM_NAME_LENGTH = 256;
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();

  GLint currentProgram;
  drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

  // Get programs data
  {
    CSharedStateDynamic::CGLSLPrograms::CListType::iterator iterProgram =
        stateDynamic.GLSLPrograms().List().begin();
    for (; iterProgram != stateDynamic.GLSLPrograms().List().end(); ++iterProgram) {

      // GET ATTRIBS
      GLint activeAttribs = 0, attribMaxLength = 0;
      drv.gl.glGetProgramiv(iterProgram->Name(), GL_ACTIVE_ATTRIBUTES, &activeAttribs);
      drv.gl.glGetProgramiv(iterProgram->Name(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attribMaxLength);
      for (int i = 0; i < activeAttribs; ++i) {
        // reserve enough space for uniform name (and 0 terminator)
        std::string name(attribMaxLength + 1, '\0');
        GLsizei actualLength = 0;
        GLint attribSize = 0;
        GLenum attribType = 0;

        drv.gl.glGetActiveAttrib(iterProgram->Name(), i, int(name.size()), &actualLength,
                                 &attribSize, &attribType, &name[0]);
        name.resize(actualLength);

        GCC433WA_0(iterProgram)->Data().restore.attribs[name] =
            drv.gl.glGetAttribLocation(iterProgram->Name(), name.c_str());
      }

      // GET UNIFORMS
      GLint activeUniforms = 0, uniformMaxLength = 0;
      drv.gl.glGetProgramiv(iterProgram->Name(), GL_ACTIVE_UNIFORMS, &activeUniforms);
      uniformMaxLength = MAX_UNIFORM_NAME_LENGTH;

      for (int i = 0; i < activeUniforms; ++i) {
        // reserve enough space for uniform name (and 0 terminator)
        std::string name(uniformMaxLength + 1, '\0');
        GLsizei actualLength = 0;
        GLint uniformSize = 0;
        GLenum uniformType = 0;

        drv.gl.glGetActiveUniform(iterProgram->Name(), i, int(name.size()), &actualLength,
                                  &uniformSize, &uniformType, &name[0]);
        name.resize(actualLength);

        // array
        if (uniformSize > 1) {
          // Removes array element indicator from uniform name
          std::size_t found;
          found = name.find("[");
          if (found != std::string::npos) {
            name.erase(found);
          }
        }

        if (uniformSize > 0) {
          GLint location = drv.gl.glGetUniformLocation(iterProgram->Name(), name.c_str());
          if (location >= 0) {
            ObtainUniform(iterProgram->Name(), GCC433WA_0(iterProgram)->Data(), name, location,
                          uniformSize, uniformType);
          }
        }
      }

      // GET SUBROUTINES
      // Check for version/extension should be enough, but some drivers lie
      // about that.
      if ((curctx::IsNvidia() && drv.gl.HasExtension("GL_ARB_shader_subroutine") &&
           curctx::Version() >= 400) ||
          (!curctx::IsNvidia() &&
           (drv.gl.HasExtension("GL_ARB_shader_subroutine") || curctx::Version() >= 400))) {
        CGLSLProgramStateData::Tracked::CLinkedShaders::iterator iterLinkedShader =
            GCC433WA_0(iterProgram)->Data().track.linkedShaders.begin();
        for (; iterLinkedShader != GCC433WA_0(iterProgram)->Data().track.linkedShaders.end();
             ++iterLinkedShader) {
          GLint shaderType = GCC433WA_0(iterProgram)
                                 ->Data()
                                 .track.linkedShaders[iterLinkedShader->first]
                                 .track.type;
          // clear set containing subroutine names for each shader type.
          GCC433WA_0(iterProgram)
              ->Data()
              .restore.uniformSubroutine[shaderType]
              .uniformSubroutineNames.clear();
          GCC433WA_0(iterProgram)
              ->Data()
              .restore.uniformSubroutine[shaderType]
              .subroutineNames.clear();
          drv.gl.glGetProgramStageiv(iterProgram->Name(), shaderType, GL_ACTIVE_SUBROUTINE_UNIFORMS,
                                     &activeUniforms);
          drv.gl.glGetProgramStageiv(iterProgram->Name(), shaderType,
                                     GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH, &uniformMaxLength);
          for (int i = 0; i < activeUniforms; ++i) {
            // reserve enough space for uniform subroutine name (and 0
            // terminator)
            std::string uniformSubName(uniformMaxLength + 1, '\0');
            GLsizei actualLength = 0;
            GLint uniformSize = 0;
            drv.gl.glGetActiveSubroutineUniformName(iterProgram->Name(), shaderType, i,
                                                    static_cast<GLsizei>(uniformSubName.size()),
                                                    &actualLength, &uniformSubName[0]);
            uniformSubName.resize(actualLength);
            drv.gl.glGetActiveSubroutineUniformiv(iterProgram->Name(), shaderType, i,
                                                  GL_UNIFORM_SIZE, &uniformSize);
            // array
            if (uniformSize > 1) {
              // Removes array element indicator from uniform name
              std::size_t found;
              found = uniformSubName.find("[");
              if (found != std::string::npos) {
                uniformSubName.erase(found);
              }
              for (int idx = 0; idx < uniformSize; ++idx) {
                std::ostringstream str;
                str << uniformSubName << "[" << idx << "]";
                GCC433WA_0(iterProgram)
                    ->Data()
                    .restore.uniformSubroutine[shaderType]
                    .uniformSubroutineNames.push_back(str.str());
              }
            } else {
              GCC433WA_0(iterProgram)
                  ->Data()
                  .restore.uniformSubroutine[shaderType]
                  .uniformSubroutineNames.push_back(uniformSubName);
            }
          }
          drv.gl.glGetProgramStageiv(iterProgram->Name(), shaderType, GL_ACTIVE_SUBROUTINES,
                                     &activeUniforms);
          drv.gl.glGetProgramStageiv(iterProgram->Name(), shaderType,
                                     GL_ACTIVE_SUBROUTINE_MAX_LENGTH, &uniformMaxLength);
          for (int i = 0; i < activeUniforms; ++i) {
            // reserve enough space for uniform subroutine name (and 0
            // terminator)
            std::string uniformSubName(uniformMaxLength + 1, '\0');
            GLsizei actualLength = 0;
            drv.gl.glGetActiveSubroutineName(iterProgram->Name(), shaderType, i,
                                             static_cast<GLsizei>(uniformSubName.size()),
                                             &actualLength, &uniformSubName[0]);
            uniformSubName.resize(actualLength);
            GCC433WA_0(iterProgram)
                ->Data()
                .restore.uniformSubroutine[shaderType]
                .subroutineNames.push_back(uniformSubName);
          }
        }
      }

      if (!curctx::IsEs() &&
          (drv.gl.HasExtension("GL_ARB_uniform_buffer_object") || curctx::Version() >= 310)) {
        // storing uniform names exists inside named uniform block to get
        // uniform indices during playback for mapping.
        {
          GLint activeUniformBlocks = 0;
          GLint uniformMaxNameLength = 0;

          drv.gl.glGetProgramiv(iterProgram->Name(), GL_ACTIVE_UNIFORM_BLOCKS,
                                &activeUniformBlocks);
          drv.gl.glGetProgramiv(iterProgram->Name(), GL_ACTIVE_UNIFORM_MAX_LENGTH,
                                &uniformMaxNameLength);

          for (int i = 0; i < activeUniformBlocks; i++) {
            GLint uniformNum = 0;

            drv.gl.glGetActiveUniformBlockiv(iterProgram->Name(), i,
                                             GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniformNum);
            std::vector<GLint> uniformIndices(uniformNum);
            drv.gl.glGetActiveUniformBlockiv(iterProgram->Name(), i,
                                             GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                             &uniformIndices[0]);

            for (int idx = 0; idx < uniformNum; idx++) {
              GLsizei actualLength = 0;
              std::string uniformName(uniformMaxNameLength + 1, '\0');

              drv.gl.glGetActiveUniformName(iterProgram->Name(), uniformIndices[idx],
                                            uniformMaxNameLength, &actualLength, &uniformName[0]);
              uniformName.resize(actualLength);
              GCC433WA_0(iterProgram)->Data().restore.uniformNames.push_back(uniformName);
            }
          }
        }
      }
    }
  }
  GLint pipeline;
  drv.gl.glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
  if ((pipeline == currentProgram) && (pipeline != 0)) {
    currentProgram = 0;
  }
  if (currentProgram != pipeline) {
    drv.gl.glUseProgram(currentProgram);
  }
}

void gits::OpenGL::CVariableGLSLInfo::Schedule(CScheduler& scheduler,
                                               const CVariable& lastValue) const {
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();

  // Schedule existing shaders creation
  for (auto& shader : stateDynamic.GLSLShaders().List()) {
    CreateShader(scheduler, shader.Data(), shader.Name());
  }

  // Save current program
  GLint currentProgram = 0;
  drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  GLint pipeline;
  drv.gl.glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
  if ((pipeline == currentProgram) && (pipeline != 0)) {
    currentProgram = 0;
  }
  bool programUsed = stateDynamic.GLSLPrograms().List().size() > 0 && (currentProgram != pipeline);

  // Schedule programs
  for (auto& program : stateDynamic.GLSLPrograms().List()) {
    CreateProgram(scheduler, program.Data());
  }

  // Schedule marked to delete shaders deletion
  for (const auto& shader : stateDynamic.GLSLShaders().List()) {
    if (shader.MarkedToDelete()) {
      scheduler.Register(new CglDeleteShader(shader.Name()));
    }
  }

  // Uniforms and subroutines restoration
  for (const auto& program : stateDynamic.GLSLPrograms().List()) {
    // Use created program to restore rest of data
    scheduler.Register(new CglUseProgram(program.Name()));

    // Restore uniforms
    {
      CGLUniformLocation::RecorderProgramOverride guard(program.Name());
      // setup necessary uniforms - register glGetUniformLocation,
      for (const auto& floatUniElem : program.Data().restore.floatUniforms) {
        // get current location of a uniform
        const GLint location =
            drv.gl.glGetUniformLocation(program.Name(), floatUniElem.first.c_str());
        // schedule location translation
        scheduler.Register(
            new CglGetUniformLocation(location, program.Name(), floatUniElem.first.c_str()));
        for (int arrayIndex = 0; arrayIndex < floatUniElem.second.arraySize; ++arrayIndex) {
          const std::vector<GLfloat>& data = floatUniElem.second.data[arrayIndex];
          switch (floatUniElem.second.type) {
          case GL_FLOAT:
            scheduler.Register(
                new CglUniform1fv(location + arrayIndex, int(data.size()), &data[0]));
            break;
          case GL_FLOAT_VEC2:
            scheduler.Register(
                new CglUniform2fv(location + arrayIndex, int(data.size()) / 2, &data[0]));
            break;
          case GL_FLOAT_VEC3:
            scheduler.Register(
                new CglUniform3fv(location + arrayIndex, int(data.size()) / 3, &data[0]));
            break;
          case GL_FLOAT_VEC4:
            scheduler.Register(
                new CglUniform4fv(location + arrayIndex, int(data.size()) / 4, &data[0]));
            break;
          case GL_FLOAT_MAT2:
            scheduler.Register(new CglUniformMatrix2fv(location + arrayIndex, int(data.size()) / 4,
                                                       false, &data[0]));
            break;
          case GL_FLOAT_MAT3:
            scheduler.Register(new CglUniformMatrix3fv(location + arrayIndex, int(data.size()) / 9,
                                                       false, &data[0]));
            break;
          case GL_FLOAT_MAT4:
            scheduler.Register(new CglUniformMatrix4fv(location + arrayIndex, int(data.size()) / 16,
                                                       false, &data[0]));
            break;
          case GL_FLOAT_MAT2x3:
            scheduler.Register(new CglUniformMatrix2x3fv(location + arrayIndex,
                                                         int(data.size()) / 6, false, &data[0]));
            break;
          case GL_FLOAT_MAT2x4:
            scheduler.Register(new CglUniformMatrix2x4fv(location + arrayIndex,
                                                         int(data.size()) / 8, false, &data[0]));
            break;
          case GL_FLOAT_MAT3x2:
            scheduler.Register(new CglUniformMatrix3x2fv(location + arrayIndex,
                                                         int(data.size()) / 6, false, &data[0]));
            break;
          case GL_FLOAT_MAT3x4:
            scheduler.Register(new CglUniformMatrix3x4fv(location + arrayIndex,
                                                         int(data.size()) / 12, false, &data[0]));
            break;
          case GL_FLOAT_MAT4x2:
            scheduler.Register(new CglUniformMatrix4x2fv(location + arrayIndex,
                                                         int(data.size()) / 8, false, &data[0]));
            break;
          case GL_FLOAT_MAT4x3:
            scheduler.Register(new CglUniformMatrix4x3fv(location + arrayIndex,
                                                         int(data.size()) / 12, false, &data[0]));
            break;
          default:
            // other types
            throw ENotImplemented(EXCEPTION_MESSAGE);
          }
        }
      }

      // setup necessary uniforms - register glGetUniformLocation,
      for (const auto& intUniElem : program.Data().restore.intUniforms) {
        // get current location of a uniform
        const GLint location =
            drv.gl.glGetUniformLocation(program.Name(), intUniElem.first.c_str());
        // schedule location translation
        scheduler.Register(
            new CglGetUniformLocation(location, program.Name(), intUniElem.first.c_str()));
        for (int arrayIndex = 0; arrayIndex < intUniElem.second.arraySize; ++arrayIndex) {
          const std::vector<GLint>& data = intUniElem.second.data[arrayIndex];
          switch (intUniElem.second.type) {
          case GL_SAMPLER_1D:
          case GL_SAMPLER_1D_ARRAY:
          case GL_SAMPLER_2D:
          case GL_SAMPLER_2D_ARRAY:
          case GL_SAMPLER_3D:
          case GL_SAMPLER_CUBE:
          case GL_SAMPLER_CUBE_SHADOW:
          case GL_SAMPLER_CUBE_MAP_ARRAY:
          case GL_SAMPLER_1D_SHADOW:
          case GL_SAMPLER_1D_ARRAY_SHADOW:
          case GL_SAMPLER_2D_SHADOW:
          case GL_SAMPLER_2D_ARRAY_SHADOW:
          case GL_SAMPLER_2D_RECT_ARB:
          case GL_SAMPLER_2D_RECT_SHADOW_ARB:
          case GL_SAMPLER_EXTERNAL_OES:
          case GL_INT_SAMPLER_1D:
          case GL_INT_SAMPLER_2D:
          case GL_INT_SAMPLER_3D:
          case GL_INT_SAMPLER_CUBE:
          case GL_INT_SAMPLER_1D_ARRAY:
          case GL_INT_SAMPLER_2D_ARRAY:
          case GL_UNSIGNED_INT_SAMPLER_1D:
          case GL_UNSIGNED_INT_SAMPLER_2D:
          case GL_UNSIGNED_INT_SAMPLER_3D:
          case GL_UNSIGNED_INT_SAMPLER_CUBE:
          case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
          case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
          case GL_SAMPLER_2D_MULTISAMPLE:
          case GL_BOOL:
          case GL_INT:
          case GL_SAMPLER_BUFFER:
          case GL_INT_SAMPLER_BUFFER:
          case GL_UNSIGNED_INT_SAMPLER_BUFFER:
          case GL_UNSIGNED_INT_ATOMIC_COUNTER:
          case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
          case GL_IMAGE_1D_EXT:
          case GL_IMAGE_2D_EXT:
          case GL_IMAGE_3D_EXT:
          case GL_IMAGE_2D_RECT_EXT:
          case GL_IMAGE_CUBE_EXT:
          case GL_IMAGE_BUFFER_EXT:
          case GL_IMAGE_1D_ARRAY_EXT:
          case GL_IMAGE_2D_ARRAY_EXT:
          case GL_IMAGE_CUBE_MAP_ARRAY_EXT:
          case GL_IMAGE_2D_MULTISAMPLE_EXT:
          case GL_IMAGE_2D_MULTISAMPLE_ARRAY_EXT:
          case GL_INT_IMAGE_1D_EXT:
          case GL_INT_IMAGE_2D_EXT:
          case GL_INT_IMAGE_3D_EXT:
          case GL_INT_IMAGE_2D_RECT_EXT:
          case GL_INT_IMAGE_CUBE_EXT:
          case GL_INT_IMAGE_BUFFER_EXT:
          case GL_INT_IMAGE_1D_ARRAY_EXT:
          case GL_INT_IMAGE_2D_ARRAY_EXT:
          case GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT:
          case GL_INT_IMAGE_2D_MULTISAMPLE_EXT:
          case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT:
          case GL_UNSIGNED_INT_IMAGE_1D_EXT:
          case GL_UNSIGNED_INT_IMAGE_2D_EXT:
          case GL_UNSIGNED_INT_IMAGE_3D_EXT:
          case GL_UNSIGNED_INT_IMAGE_2D_RECT_EXT:
          case GL_UNSIGNED_INT_IMAGE_CUBE_EXT:
          case GL_UNSIGNED_INT_IMAGE_BUFFER_EXT:
          case GL_UNSIGNED_INT_IMAGE_1D_ARRAY_EXT:
          case GL_UNSIGNED_INT_IMAGE_2D_ARRAY_EXT:
          case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT:
          case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_EXT:
          case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT:
            scheduler.Register(
                new CglUniform1iv(location + arrayIndex, int(data.size()), &data[0]));
            break;
          case GL_BOOL_VEC2:
          case GL_INT_VEC2:
            scheduler.Register(
                new CglUniform2iv(location + arrayIndex, int(data.size()) / 2, &data[0]));
            break;
          case GL_BOOL_VEC3:
          case GL_INT_VEC3:
            scheduler.Register(
                new CglUniform3iv(location + arrayIndex, int(data.size()) / 3, &data[0]));
            break;
          case GL_BOOL_VEC4:
          case GL_INT_VEC4:
            scheduler.Register(
                new CglUniform4iv(location + arrayIndex, int(data.size() / 4), &data[0]));
            break;
          default:
            throw ENotImplemented(EXCEPTION_MESSAGE);
          }
        }
      }

      for (const auto& uintUniElem : program.Data().restore.uintUniforms) {
        // get current location of a uniform
        const GLuint location =
            drv.gl.glGetUniformLocation(program.Name(), uintUniElem.first.c_str());
        // schedule location translation
        scheduler.Register(
            new CglGetUniformLocation(location, program.Name(), uintUniElem.first.c_str()));
        for (int arrayIndex = 0; arrayIndex < uintUniElem.second.arraySize; ++arrayIndex) {
          const std::vector<GLuint>& data = uintUniElem.second.data[arrayIndex];
          switch (uintUniElem.second.type) {
          case GL_UNSIGNED_INT:
            scheduler.Register(
                new CglUniform1uiv(location + arrayIndex, int(data.size()), &data[0]));
            break;
          case GL_UNSIGNED_INT_VEC2:
            scheduler.Register(
                new CglUniform2uiv(location + arrayIndex, int(data.size()) / 2, &data[0]));
            break;
          case GL_UNSIGNED_INT_VEC3:
            scheduler.Register(
                new CglUniform3uiv(location + arrayIndex, int(data.size()) / 3, &data[0]));
            break;
          case GL_UNSIGNED_INT_VEC4:
            scheduler.Register(
                new CglUniform4uiv(location + arrayIndex, int(data.size() / 4), &data[0]));
            break;
          default:
            throw ENotImplemented(EXCEPTION_MESSAGE);
          }
        }
      }
    }

    // Restore Subroutines
    // Check for version/extension should be enough, but some drivers lie about
    // that.
    if (drv.gl.HasExtension("GL_ARB_shader_subroutine") || curctx::Version() >= 400) {
      // setup necessary uniform subroutines - register
      // glGetSubroutineUniformLocation,
      for (const auto& uniformSubElem : program.Data().restore.uniformSubroutine) {
        std::vector<std::string>::const_iterator subIter =
            uniformSubElem.second.uniformSubroutineNames.begin();
        std::vector<GLuint> vecSubroutineIdx;
        vecSubroutineIdx.resize(uniformSubElem.second.uniformSubroutineNames.size());
        for (; subIter != uniformSubElem.second.uniformSubroutineNames.end(); ++subIter) {
          GLint locUniformSub = drv.gl.glGetSubroutineUniformLocation(
              program.Name(), uniformSubElem.first, subIter->c_str());
          GLuint idxSubroutine = 0;
          drv.gl.glGetUniformSubroutineuiv(uniformSubElem.first, locUniformSub, &idxSubroutine);
          vecSubroutineIdx[locUniformSub] = idxSubroutine;
        }
        subIter = uniformSubElem.second.subroutineNames.begin();
        for (; subIter != uniformSubElem.second.subroutineNames.end(); ++subIter) {
          GLuint index =
              drv.gl.glGetSubroutineIndex(program.Name(), uniformSubElem.first, subIter->c_str());
          // schedule index translation
          scheduler.Register(new CglGetSubroutineIndex(index, program.Name(), uniformSubElem.first,
                                                       subIter->c_str()));
        }
        if (!vecSubroutineIdx.empty()) {
          // schedule index translation
          scheduler.Register(new CglUniformSubroutinesuiv(
              uniformSubElem.first, static_cast<GLsizei>(vecSubroutineIdx.size()),
              &(vecSubroutineIdx[0])));
        }
      }
    }

    if (drv.gl.HasExtension("GL_ARB_uniform_buffer_object") || curctx::Version() >= 310 ||
        curctx::IsEs3Plus()) {
      // Schedule registering uniform block index getter API to populate uniform
      // block index map during playback.
      {
        GLint activeUniformBlocks = 0;
        GLint uniformBlockMaxLength = 0;
        GLuint uniformBlockBinding = 0;

        drv.gl.glGetProgramiv(program.Name(), GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);
        drv.gl.glGetProgramiv(program.Name(), GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH,
                              &uniformBlockMaxLength);

        for (int i = 0; i < activeUniformBlocks; i++) {
          if (uniformBlockMaxLength == 0) {
            uniformBlockMaxLength = 200;
          }
          // reserve enough space for uniform name (and 0 terminator)
          std::string name(uniformBlockMaxLength + 1, '\0');
          GLsizei actualLength = 0;

          drv.gl.glGetActiveUniformBlockName(program.Name(), i, int(name.size()), &actualLength,
                                             &name[0]);

          name.resize(actualLength);
          scheduler.Register(new CglGetUniformBlockIndex(i, program.Name(), name.c_str()));

          drv.gl.glGetActiveUniformBlockiv(program.Name(), i, GL_UNIFORM_BLOCK_BINDING,
                                           (GLint*)&uniformBlockBinding);
          scheduler.Register(new CglUniformBlockBinding(program.Name(), i, uniformBlockBinding));
        }
      }

      // Schedule registering uniform index getter API to populate uniform index
      // map during playback.
      {

        std::vector<GLuint> uniformIndices;
        std::vector<const GLchar*> uniformNames;

        uniformIndices.resize(program.Data().restore.uniformNames.size());
        uniformNames.resize(program.Data().restore.uniformNames.size());
        if (uniformNames.size() > 0 && uniformIndices.size() > 0) {
          for (unsigned int i = 0; i < uniformIndices.size(); i++) {
            uniformNames[i] = program.Data().restore.uniformNames[i].c_str();
          }
          drv.gl.glGetUniformIndices(program.Name(), (GLsizei)uniformIndices.size(),
                                     &uniformNames[0], &uniformIndices[0]);
          scheduler.Register(new CglGetUniformIndices(program.Name(),
                                                      (GLsizei)uniformIndices.size(),
                                                      &uniformNames[0], &uniformIndices[0]));
        }
      }
    }
    if ((drv.gl.HasExtension("GL_ARB_program_interface_query") && curctx::Version() >= 400) ||
        (curctx::Version() >= 430)) {
      GLint maxNameLength = 0;
      GLint maxShaderStorageBufferBindings = 0;
      drv.gl.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBufferBindings);

      drv.gl.glGetProgramInterfaceiv(program.Name(), GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH,
                                     &maxNameLength);
      for (int i = 0; i < maxShaderStorageBufferBindings; i++) {
        // reserve enough space for uniform name (and 0 terminator)
        std::string name(maxNameLength + 1, '\0');
        GLsizei actualLength = 0;

        drv.gl.glGetProgramResourceName(program.Name(), GL_SHADER_STORAGE_BLOCK, i,
                                        (GLsizei)name.size(), &actualLength, &name[0]);
        name.resize(actualLength);
        if (name.size() != 0) {
          GLuint return_value = drv.gl.glGetProgramResourceIndex(
              program.Name(), GL_SHADER_STORAGE_BLOCK, name.c_str());
          GLenum buffBindingEnum = GL_BUFFER_BINDING;
          GLint blockBindingOutput = 0;
          actualLength = 0;
          drv.gl.glGetProgramResourceiv(program.Name(), GL_SHADER_STORAGE_BLOCK, i, 1,
                                        &buffBindingEnum, 1, &actualLength, &blockBindingOutput);
          if (actualLength != 1) {
            Log(ERR) << "glGetProgramResourceiv in CVariableGLSLInfo return size " << actualLength
                     << " instead of 1.";
          }
          scheduler.Register(new CglGetProgramResourceIndex(return_value, program.Name(),
                                                            GL_SHADER_STORAGE_BLOCK, name.c_str()));
          scheduler.Register(new CglShaderStorageBlockBinding_V1(program.Name(), return_value,
                                                                 blockBindingOutput));
        }
      }
    }
  }

  // Schedule marked to delete programs deletion
  for (const auto& program : stateDynamic.GLSLPrograms().List()) {
    if (program.MarkedToDelete()) {
      scheduler.Register(new CglDeleteProgram(program.Name()));
    }
  }

  // Restore original program bindings
  if (programUsed) {
    drv.gl.glUseProgram(currentProgram);
    scheduler.Register(new CglUseProgram(currentProgram));
  }
}

void gits::OpenGL::CVariableGLSLInfo::CreateShader(CScheduler& scheduler,
                                                   const CGLSLShaderStateData& shader,
                                                   GLuint shaderID) {
  scheduler.Register(new CglCreateShader(shaderID, shader.track.type));

  std::stringstream shaderSource(shader.track.source);
  std::vector<std::string> dataContainer;
  std::string line;
  while (std::getline(shaderSource, line)) {
    dataContainer.push_back(line);
  }

  std::vector<const char*> pointerContainer;
  for (auto& data : dataContainer) {
    pointerContainer.push_back(data.c_str());
  }
  if (int(pointerContainer.size()) > 0) {
    scheduler.Register(
        new CglShaderSource(shaderID, int(pointerContainer.size()), &pointerContainer[0], nullptr));
    if (shader.track.compiled) {
      scheduler.Register(new CglCompileShader(shaderID));
    }
  }
}

void gits::OpenGL::CVariableGLSLInfo::CreateProgram(CScheduler& scheduler,
                                                    const CGLSLProgramStateData& progData) {
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();

  drv.gl.glUseProgram(progData.track.name);
  scheduler.Register(new CglCreateProgram(progData.track.name));

  // helper container tracking temporary shaders to detach
  // and delete after linking
  std::set<const CGLSLShaderStateData*> temporaryShaders;

  // Schedule attach of all linked shaders and optionally create temporary
  // shaders
  for (auto iterLinkedShader = progData.track.linkedShaders.begin();
       iterLinkedShader != progData.track.linkedShaders.end(); iterLinkedShader++) {
    // if linked shader does not exist (not a marked to delete case) or has a
    // different source create temporary shader for linking
    if (stateDynamic.GLSLShaders().Get(iterLinkedShader->first) == nullptr ||
        stateDynamic.GLSLShaders().Get(iterLinkedShader->first)->Data().track.source !=
            iterLinkedShader->second.track.source) {
      // Generate new unique name for temporary shader creation creation
      GCC433WA_0(iterLinkedShader)->second.restore.uniqueName = GLSLUnique::GenUniqueName();
      CreateShader(scheduler, iterLinkedShader->second,
                   iterLinkedShader->second.restore.uniqueName);
      temporaryShaders.insert(&iterLinkedShader->second);
    }
    scheduler.Register(
        new CglAttachShader(progData.track.name, iterLinkedShader->second.restore.uniqueName));
  }
  // If program hasn't been linked yet, schedule attach of all attached shaders
  if (progData.track.linkedShaders.empty()) {
    for (auto iterAttachedShader = progData.track.attachedShaders.begin();
         iterAttachedShader != progData.track.attachedShaders.end(); iterAttachedShader++) {
      scheduler.Register(
          new CglAttachShader(progData.track.name, iterAttachedShader->second->restore.uniqueName));
    }
  }

  // Schedule transform varyings
  if (progData.track.transformVaryings.size() != 0) {
    std::vector<const char*> tfVaryings(progData.track.transformVaryings.size());
    for (std::vector<int>::size_type i = 0; i != tfVaryings.size(); i++) {
      tfVaryings[i] = progData.track.transformVaryings[i].c_str();
    }

    GLsizei varyingsNum = (GLsizei)progData.track.transformVaryings.size();
    scheduler.Register(new CglTransformFeedbackVaryings(
        progData.track.name, varyingsNum, &tfVaryings[0], progData.track._tfBufferMode));
  }

  // Set program parameters
  if (progData.track.isSepShaderObj) {
    scheduler.Register(
        new CglProgramParameteri(progData.track.name, GL_PROGRAM_SEPARABLE, GL_TRUE));
  }

  // Schedule program linking
  scheduler.Register(
      new CgitsLinkProgramAttribsSetting(progData.track.name, progData.restore.attribs));
  scheduler.Register(new CglLinkProgram(progData.track.name));
  if ((curctx::IsOgl() && curctx::Version() >= 430) || (curctx::IsEs31Plus()) ||
      (drv.gl.HasExtension("GL_ARB_program_interface_query") &&
       drv.gl.HasExtension("GL_ARB_shader_storage_buffer_object"))) {
    scheduler.Register(new CgitsLinkProgramBuffersSetting(progData.track.name));
  }

  // Schedule temporary shaders removal
  for (auto& tmpShElem : temporaryShaders) {
    scheduler.Register(new CglDetachShader(progData.track.name, tmpShElem->restore.uniqueName));
    scheduler.Register(new CglDeleteShader(tmpShElem->restore.uniqueName));
  }
}

void gits::OpenGL::CVariableGLSLInfo::ObtainUniform(GLuint programId,
                                                    CGLSLProgramStateData& program,
                                                    const std::string& name,
                                                    GLint location,
                                                    GLint arraySize,
                                                    GLenum uniformType) {
  std::pair<GLenum, GLuint> dimensionality = UniformDimensions(uniformType);

  if (dimensionality.first == GL_INT) {
    auto& iuniform = program.restore.intUniforms[name];
    iuniform.arraySize = arraySize;
    iuniform.type = uniformType;
    iuniform.data.resize(arraySize);
    for (int i = 0; i < arraySize; ++i) {
      iuniform.data[i].resize(dimensionality.second);
      drv.gl.glGetUniformiv(programId, location + i, iuniform.data[i].data());
    }
  } else if (dimensionality.first == GL_FLOAT) {
    auto& funiform = program.restore.floatUniforms[name];
    funiform.arraySize = arraySize;
    funiform.type = uniformType;
    funiform.data.resize(arraySize);
    for (int i = 0; i < arraySize; ++i) {
      funiform.data[i].resize(dimensionality.second);
      drv.gl.glGetUniformfv(programId, location + i, funiform.data[i].data());
    }
  } else if (dimensionality.first == GL_UNSIGNED_INT) {
    auto& uiuniform = program.restore.uintUniforms[name];
    uiuniform.arraySize = arraySize;
    uiuniform.type = uniformType;
    uiuniform.data.resize(arraySize);
    for (int i = 0; i < arraySize; ++i) {
      uiuniform.data[i].resize(dimensionality.second);
      drv.gl.glGetUniformuiv(programId, location + i, uiuniform.data[i].data());
    }
  } else {
    Log(ERR) << "Type " << GetGLEnumString(dimensionality.first) << " ("
             << gits::hex(dimensionality.first) << ") is not one of: ";
    Log(ERR) << " - GL_INT (" << gits::hex(GL_INT) << ")";
    Log(ERR) << " - GL_FLOAT (" << gits::hex(GL_FLOAT) << ")";
    Log(ERR) << " - GL_UNSIGNED_INT (" << gits::hex(GL_UNSIGNED_INT) << ")";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

std::pair<GLenum, GLuint> gits::OpenGL::CVariableGLSLInfo::UniformDimensions(GLenum type) {
  typedef std::map<GLenum, std::pair<GLenum, GLuint>> map_t;
  INIT_NEW_STATIC_OBJ(typeMap, map_t)

  if (typeMap.empty()) {
    typeMap[GL_FLOAT] = std::make_pair(GL_FLOAT, 1);
    typeMap[GL_FLOAT_VEC2] = std::make_pair(GL_FLOAT, 2);
    typeMap[GL_FLOAT_VEC3] = std::make_pair(GL_FLOAT, 3);
    typeMap[GL_FLOAT_VEC4] = std::make_pair(GL_FLOAT, 4);
    typeMap[GL_INT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_VEC2] = std::make_pair(GL_INT, 2);
    typeMap[GL_INT_VEC3] = std::make_pair(GL_INT, 3);
    typeMap[GL_INT_VEC4] = std::make_pair(GL_INT, 4);

    typeMap[GL_UNSIGNED_INT] = std::make_pair(GL_UNSIGNED_INT, 1);
    typeMap[GL_UNSIGNED_INT_VEC2] = std::make_pair(GL_UNSIGNED_INT, 2);
    typeMap[GL_UNSIGNED_INT_VEC3] = std::make_pair(GL_UNSIGNED_INT, 3);
    typeMap[GL_UNSIGNED_INT_VEC4] = std::make_pair(GL_UNSIGNED_INT, 4);

    typeMap[GL_BOOL] = std::make_pair(GL_INT, 1);
    typeMap[GL_BOOL_VEC2] = std::make_pair(GL_INT, 2);
    typeMap[GL_BOOL_VEC3] = std::make_pair(GL_INT, 3);
    typeMap[GL_BOOL_VEC4] = std::make_pair(GL_INT, 4);
    typeMap[GL_FLOAT_MAT2] = std::make_pair(GL_FLOAT, 4);
    typeMap[GL_FLOAT_MAT3] = std::make_pair(GL_FLOAT, 9);
    typeMap[GL_FLOAT_MAT4] = std::make_pair(GL_FLOAT, 16);
    typeMap[GL_FLOAT_MAT2x3] = std::make_pair(GL_FLOAT, 6);
    typeMap[GL_FLOAT_MAT2x4] = std::make_pair(GL_FLOAT, 8);
    typeMap[GL_FLOAT_MAT3x2] = std::make_pair(GL_FLOAT, 6);
    typeMap[GL_FLOAT_MAT3x4] = std::make_pair(GL_FLOAT, 12);
    typeMap[GL_FLOAT_MAT4x2] = std::make_pair(GL_FLOAT, 8);
    typeMap[GL_FLOAT_MAT4x3] = std::make_pair(GL_FLOAT, 12);

    typeMap[GL_SAMPLER_1D] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_1D_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_3D] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_CUBE] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_CUBE_SHADOW] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_CUBE_MAP_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_1D_SHADOW] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_1D_ARRAY_SHADOW] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D_SHADOW] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D_ARRAY_SHADOW] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D_RECT_ARB] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D_RECT_SHADOW_ARB] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_EXTERNAL_OES] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_1D] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_2D] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_3D] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_CUBE] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_1D_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_2D_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_1D] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_2D] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_3D] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_CUBE] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_1D_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_2D_ARRAY] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_2D_MULTISAMPLE] = std::make_pair(GL_INT, 1);
    typeMap[GL_SAMPLER_BUFFER] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_2D_RECT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_SAMPLER_BUFFER] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_SAMPLER_BUFFER] = std::make_pair(GL_INT, 1);

    typeMap[GL_UNSIGNED_INT_ATOMIC_COUNTER] = std::make_pair(GL_INT, 1);

    typeMap[GL_IMAGE_1D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_2D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_3D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_2D_RECT_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_CUBE_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_BUFFER_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_1D_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_2D_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_CUBE_MAP_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_2D_MULTISAMPLE_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_IMAGE_2D_MULTISAMPLE_ARRAY_EXT] = std::make_pair(GL_INT, 1);

    typeMap[GL_INT_IMAGE_1D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_2D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_3D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_2D_RECT_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_CUBE_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_BUFFER_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_1D_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_2D_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_2D_MULTISAMPLE_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT] = std::make_pair(GL_INT, 1);

    typeMap[GL_UNSIGNED_INT_IMAGE_1D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_2D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_3D_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_2D_RECT_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_CUBE_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_BUFFER_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_1D_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_2D_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_EXT] = std::make_pair(GL_INT, 1);
    typeMap[GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT] = std::make_pair(GL_INT, 1);
  }
  return typeMap[type];
}

/**************************************** G L S L   P I P E L I N E  I N F O ************************************************/
gits::OpenGL::CVariableGLSLPipelinesInfo::CVariableGLSLPipelinesInfo() {}

void gits::OpenGL::CVariableGLSLPipelinesInfo::Get() {}

void gits::OpenGL::CVariableGLSLPipelinesInfo::Schedule(CScheduler& scheduler,
                                                        const CVariable& lastValue) const {
  // Schedule pipelines
  for (const auto& pipeline : SD().GetCurrentContextStateData().GLSLPipelines().List()) {
    // Create pipeline
    // Copy name to non-const var
    GLuint pipelineName = pipeline.Data().track.name;
    scheduler.Register(new CglGenProgramPipelines(1, &pipelineName));
    scheduler.Register(new CglBindProgramPipeline(pipelineName));
    for (auto& stage : pipeline.Data().track.stages) {
      // Schedule stage
      const CGLSLProgramStateData& progData = *stage.second;
      scheduler.Register(new CglUseProgramStages(pipelineName, stage.first, progData.track.name));
    }
    scheduler.Register(new CglBindProgramPipeline(0));
  }
}

/**************************************** V A O   I N F O ************************************************/

gits::OpenGL::CVariableVAOInfo::CVariableVAOInfo() {}

void gits::OpenGL::CVariableVAOInfo::Get() {
  CContextStateDynamic& stateDynamic = SD().GetCurrentContextStateData();
  GLuint currentlyBoundVAO;
  // Check if VAO is used
  if (stateDynamic.VertexArrays().List().begin() != stateDynamic.VertexArrays().List().end()) {
    // Save currently bound VAO
    drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&currentlyBoundVAO);
    drv.gl.glBindVertexArray(0);

    // Get state from all VAOs by binding each one and getting state
    for (auto& vertexArrayState : stateDynamic.VertexArrays().List()) {
      auto* nonConstVertexState = GCC433WA_0(&vertexArrayState);
      drv.gl.glBindVertexArray(nonConstVertexState->Name());

      drv.gl.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,
                           &nonConstVertexState->Data().restore.elementArrayBufferBinding);

      // get state for each attrib array
      for (GLuint i : nonConstVertexState->Data().track.attribs) {
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                                   &nonConstVertexState->Data().restore.attribsMap[i].enabled);
        drv.gl.glGetVertexAttribiv(
            i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
            &nonConstVertexState->Data().restore.attribsMap[i].arrayBufferBinding);
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE,
                                   &nonConstVertexState->Data().restore.attribsMap[i].size);
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE,
                                   &nonConstVertexState->Data().restore.attribsMap[i].stride);
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE,
                                   &nonConstVertexState->Data().restore.attribsMap[i].type);
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
                                   &nonConstVertexState->Data().restore.attribsMap[i].normalized);
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_INTEGER,
                                   &nonConstVertexState->Data().restore.attribsMap[i].integer);
        drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR,
                                   &nonConstVertexState->Data().restore.attribsMap[i].divisor);
        if (curctx::IsEs31Plus() || (curctx::IsOgl() && curctx::Version() >= 430)) {
          GLint binding;
          drv.gl.glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_BINDING, &binding);
          nonConstVertexState->Data().restore.attribsMap[i].binding = binding;
          drv.gl.glGetVertexAttribiv(
              i, GL_VERTEX_ATTRIB_RELATIVE_OFFSET,
              &nonConstVertexState->Data().restore.attribsMap[i].relativeOffset);
          drv.gl.glGetIntegeri_v(
              GL_VERTEX_BINDING_OFFSET, binding,
              &nonConstVertexState->Data().restore.bindingPointsMap[binding].bindingOffset);
          drv.gl.glGetIntegeri_v(
              GL_VERTEX_BINDING_STRIDE, binding,
              &nonConstVertexState->Data().restore.bindingPointsMap[binding].bindingStride);
          drv.gl.glGetIntegeri_v(
              GL_VERTEX_BINDING_DIVISOR, binding,
              &nonConstVertexState->Data().restore.bindingPointsMap[binding].bindingDivisor);
          drv.gl.glGetVertexAttribiv(
              i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
              &nonConstVertexState->Data().restore.bindingPointsMap[binding].arrayBufferBinding);
        }
        drv.gl.glGetVertexAttribPointerv(
            i, GL_VERTEX_ATTRIB_ARRAY_POINTER,
            &nonConstVertexState->Data().restore.attribsMap[i].pointer);
      }

      if (!SD().IsCurrentContextCore()) {
        nonConstVertexState->Data().restore.colorState.enabled = drv.gl.glIsEnabled(GL_COLOR_ARRAY);
        drv.gl.glGetIntegerv(GL_COLOR_ARRAY_SIZE,
                             &nonConstVertexState->Data().restore.colorState.size);
        drv.gl.glGetIntegerv(GL_COLOR_ARRAY_TYPE,
                             &nonConstVertexState->Data().restore.colorState.type);
        drv.gl.glGetIntegerv(GL_COLOR_ARRAY_STRIDE,
                             &nonConstVertexState->Data().restore.colorState.stride);
        drv.gl.glGetPointerv(GL_COLOR_ARRAY_POINTER,
                             &nonConstVertexState->Data().restore.colorState.pointer);

        nonConstVertexState->Data().restore.secondaryColorState.enabled =
            drv.gl.glIsEnabled(GL_SECONDARY_COLOR_ARRAY);
        drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_SIZE,
                             &nonConstVertexState->Data().restore.secondaryColorState.size);
        drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_TYPE,
                             &nonConstVertexState->Data().restore.secondaryColorState.type);
        drv.gl.glGetIntegerv(GL_SECONDARY_COLOR_ARRAY_STRIDE,
                             &nonConstVertexState->Data().restore.secondaryColorState.stride);
        drv.gl.glGetPointerv(GL_SECONDARY_COLOR_ARRAY_POINTER,
                             &nonConstVertexState->Data().restore.secondaryColorState.pointer);

        nonConstVertexState->Data().restore.vertexPointerState.enabled =
            drv.gl.glIsEnabled(GL_VERTEX_ARRAY);
        drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BUFFER_BINDING,
                             &nonConstVertexState->Data().restore.vertexPointerState.boundBuffer);
        drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_SIZE,
                             &nonConstVertexState->Data().restore.vertexPointerState.size);
        drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_TYPE,
                             &nonConstVertexState->Data().restore.vertexPointerState.type);
        drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_STRIDE,
                             &nonConstVertexState->Data().restore.vertexPointerState.stride);
        drv.gl.glGetPointerv(GL_VERTEX_ARRAY_POINTER,
                             &nonConstVertexState->Data().restore.vertexPointerState.pointer);

        nonConstVertexState->Data().restore.normalPointerState.enabled =
            drv.gl.glIsEnabled(GL_NORMAL_ARRAY);
        drv.gl.glGetIntegerv(GL_NORMAL_ARRAY_BUFFER_BINDING,
                             &nonConstVertexState->Data().restore.normalPointerState.boundBuffer);
        drv.gl.glGetIntegerv(GL_NORMAL_ARRAY_TYPE,
                             &nonConstVertexState->Data().restore.normalPointerState.type);
        drv.gl.glGetIntegerv(GL_NORMAL_ARRAY_STRIDE,
                             &nonConstVertexState->Data().restore.normalPointerState.stride);
        drv.gl.glGetPointerv(GL_NORMAL_ARRAY_POINTER,
                             &nonConstVertexState->Data().restore.normalPointerState.pointer);

        GLint maxTexCoords;
        GLenum query = curctx::IsEs1() ? GL_MAX_TEXTURE_UNITS : GL_MAX_TEXTURE_COORDS;
        drv.gl.glGetIntegerv(query, &maxTexCoords);
        drv.gl.glGetIntegerv(query, &nonConstVertexState->Data().restore.maxTexCoords);

        GLint clientActiveTexture;
        drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTexture);
        drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE,
                             &nonConstVertexState->Data().restore.clientActiveTexture);

        for (int j = 0; j < maxTexCoords; ++j) {
          drv.gl.glClientActiveTexture(GL_TEXTURE0 + j);
          nonConstVertexState->Data().restore.texCoordMap[j].enabled =
              drv.gl.glIsEnabled(GL_TEXTURE_COORD_ARRAY);
          drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE,
                               &nonConstVertexState->Data().restore.texCoordMap[j].size);
          drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE,
                               &nonConstVertexState->Data().restore.texCoordMap[j].type);
          drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE,
                               &nonConstVertexState->Data().restore.texCoordMap[j].stride);
          drv.gl.glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER,
                               &nonConstVertexState->Data().restore.texCoordMap[j].pointer);
          drv.gl.glGetIntegerv(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING,
                               &nonConstVertexState->Data().restore.texCoordMap[j].boundBuffer);
        }
        drv.gl.glClientActiveTexture(clientActiveTexture);
      }

      drv.gl.glBindVertexArray(0);
    }

    drv.gl.glBindVertexArray(currentlyBoundVAO);
  }
}

void gits::OpenGL::CVariableVAOInfo::CreateVAO(CScheduler& scheduler,
                                               const CVertexArraysStateData& vaoToCreate) {
  // Create and bind new VAO
  GLuint tmpVaoName = vaoToCreate.track.name;
  scheduler.Register(new CglGenVertexArrays(1, &tmpVaoName));
  scheduler.Register(new CglBindVertexArray(vaoToCreate.track.name));

  scheduler.Register(
      new CglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vaoToCreate.restore.elementArrayBufferBinding));

  // Set attributes for created VAO
  auto isEsEnabled = curctx::IsEs31Plus();
  if (isEsEnabled || (curctx::IsOgl() && curctx::Version() >= 430)) {
    for (const auto& bindingPoint : vaoToCreate.restore.bindingPointsMap) {
      scheduler.Register(new CglBindVertexBuffer(
          bindingPoint.first, bindingPoint.second.arrayBufferBinding,
          (GLintptr)bindingPoint.second.bindingOffset, bindingPoint.second.bindingStride));

      if (bindingPoint.second.bindingDivisor > 0) {
        scheduler.Register(
            new CglVertexBindingDivisor(bindingPoint.first, bindingPoint.second.bindingDivisor));
      }
    }

    for (const auto& attribElem : vaoToCreate.restore.attribsMap) {
      if (attribElem.second.integer > 0) {
        scheduler.Register(new CglVertexAttribIFormat(attribElem.first, attribElem.second.size,
                                                      attribElem.second.type,
                                                      attribElem.second.relativeOffset));
      } else if ((attribElem.second.integer == 0) && (attribElem.second.type == GL_DOUBLE)) {
        scheduler.Register(new CglVertexAttribLFormat(attribElem.first, attribElem.second.size,
                                                      attribElem.second.type,
                                                      attribElem.second.relativeOffset));
      } else {
        scheduler.Register(new CglVertexAttribFormat(
            attribElem.first, attribElem.second.size, attribElem.second.type,
            GLboolean(attribElem.second.normalized), attribElem.second.relativeOffset));
      }

      scheduler.Register(new CglVertexAttribBinding(attribElem.first, attribElem.second.binding));
      scheduler.Register(new CglEnableVertexAttribArray(attribElem.first));
    }
  } else {
    for (const auto& attribElem : vaoToCreate.restore.attribsMap) {
      scheduler.Register(new CglBindBuffer(GL_ARRAY_BUFFER, attribElem.second.arrayBufferBinding));

      if ((attribElem.second.integer > 0) && (attribElem.second.type != GL_DOUBLE))
      // glVertexAttribIPointer
      {
        scheduler.Register(new CglVertexAttribIPointer(
            attribElem.first, attribElem.second.size, attribElem.second.type,
            attribElem.second.stride, attribElem.second.pointer,
            attribElem.second.arrayBufferBinding));
      }

      if ((attribElem.second.integer == 0) && (attribElem.second.type != GL_DOUBLE))
      // glVertexAttribPointer
      {
        scheduler.Register(new CglVertexAttribPointer(
            attribElem.first, attribElem.second.size, attribElem.second.type,
            GLboolean(attribElem.second.normalized), attribElem.second.stride,
            attribElem.second.pointer, attribElem.second.arrayBufferBinding));
      }

      //!!!!!!!!!!!!TODO uncoment code below after adding glVertexAttribLPointer
      //!to GITS

      //glVertexAttribLPointer
      //       if ( (newAttribsIter->second._vertexAttribArrayInteger > 0)
      //        && (newAttribsIter->second._vertexAttribArrayType == 5130) )
      //       {
      //        scheduler.Register(new CglVertexAttribLPointer(newAttribsIter->first,
      //                                                        newAttribsIter->second._vertexAttribArraySize,
      //                                                        newAttribsIter->second._vertexAttribArrayType,
      //                                                        newAttribsIter->second._vertexAttribArrayStride,
      //                                                        newAttribsIter->second._vertexAttribArrayPointer));
      //       }

      if (attribElem.second.divisor > 0) {
        // glVertexAttribDivisor
        scheduler.Register(new CglVertexAttribDivisor(attribElem.first, attribElem.second.divisor));
      }

      if (attribElem.second.enabled > 0) {
        // glEnableVertexAttribArray
        scheduler.Register(new CglEnableVertexAttribArray(attribElem.first));
      }
    }
  }

  if (!SD().IsCurrentContextCore()) {
    scheduler.Register(new OpenGL::CglEnableClientState(GL_COLOR_ARRAY));
    scheduler.Register(new OpenGL::CglColorPointer(
        vaoToCreate.restore.colorState.size, vaoToCreate.restore.colorState.type,
        vaoToCreate.restore.colorState.stride, vaoToCreate.restore.colorState.pointer, true));
    if (!vaoToCreate.restore.colorState.enabled) {
      scheduler.Register(new OpenGL::CglDisableClientState(GL_COLOR_ARRAY));
    }

    scheduler.Register(new OpenGL::CglEnableClientState(GL_SECONDARY_COLOR_ARRAY));
    scheduler.Register(new OpenGL::CglSecondaryColorPointer(
        vaoToCreate.restore.secondaryColorState.size, vaoToCreate.restore.secondaryColorState.type,
        vaoToCreate.restore.secondaryColorState.stride,
        vaoToCreate.restore.secondaryColorState.pointer, true));
    if (!vaoToCreate.restore.secondaryColorState.enabled) {
      scheduler.Register(new OpenGL::CglDisableClientState(GL_SECONDARY_COLOR_ARRAY));
    }

    scheduler.Register(new OpenGL::CglEnableClientState(GL_VERTEX_ARRAY));
    scheduler.Register(new OpenGL::CglBindBuffer(
        GL_ARRAY_BUFFER, vaoToCreate.restore.vertexPointerState.boundBuffer));
    scheduler.Register(new OpenGL::CglVertexPointer(
        vaoToCreate.restore.vertexPointerState.size, vaoToCreate.restore.vertexPointerState.type,
        vaoToCreate.restore.vertexPointerState.stride,
        vaoToCreate.restore.vertexPointerState.pointer, true));
    if (!vaoToCreate.restore.vertexPointerState.enabled) {
      scheduler.Register(new OpenGL::CglDisableClientState(GL_VERTEX_ARRAY));
    }

    scheduler.Register(new OpenGL::CglEnableClientState(GL_NORMAL_ARRAY));
    scheduler.Register(new OpenGL::CglBindBuffer(
        GL_ARRAY_BUFFER, vaoToCreate.restore.normalPointerState.boundBuffer));
    scheduler.Register(new OpenGL::CglNormalPointer(
        vaoToCreate.restore.normalPointerState.type, vaoToCreate.restore.normalPointerState.stride,
        vaoToCreate.restore.normalPointerState.pointer, true));
    if (!vaoToCreate.restore.normalPointerState.enabled) {
      scheduler.Register(new OpenGL::CglDisableClientState(GL_NORMAL_ARRAY));
    }

    int i = 0;
    GLint clientActiveTextureOld = -1;
    drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientActiveTextureOld);
    GLint lastScheduledCAT = clientActiveTextureOld;
    for (const auto& texCoordElem : vaoToCreate.restore.texCoordMap) {
      // we need to alter GL state to retrieve correct buffer binding in
      // CglTexCoordPointer
      drv.gl.glClientActiveTexture(GL_TEXTURE0 + i);

      scheduler.Register(
          new OpenGL::CglBindBuffer(GL_ARRAY_BUFFER, texCoordElem.second.boundBuffer));
      lastScheduledCAT = GL_TEXTURE0 + i;
      scheduler.Register(new OpenGL::CglClientActiveTexture(GL_TEXTURE0 + i));
      scheduler.Register(new OpenGL::CglEnableClientState(GL_TEXTURE_COORD_ARRAY));
      scheduler.Register(new OpenGL::CglTexCoordPointer(
          texCoordElem.second.size, texCoordElem.second.type, texCoordElem.second.stride,
          texCoordElem.second.pointer, true));
      if (!texCoordElem.second.enabled) {
        scheduler.Register(new OpenGL::CglDisableClientState(GL_TEXTURE_COORD_ARRAY));
      }
      i++;
    }

    if (lastScheduledCAT != vaoToCreate.restore.clientActiveTexture) {
      scheduler.Register(
          new OpenGL::CglClientActiveTexture(vaoToCreate.restore.clientActiveTexture));
      drv.gl.glClientActiveTexture(clientActiveTextureOld);
    }
  }
}

void gits::OpenGL::CVariableVAOInfo::Schedule(CScheduler& scheduler,
                                              const CVariable& lastValue) const {
  CContextStateDynamic& sharedStateDynamic = SD().GetCurrentContextStateData();
  if (sharedStateDynamic.VertexArrays().List().size() == 0) {
    return;
  }

  GLuint currentlyBoundVAO = 0;
  drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&currentlyBoundVAO);

  // Search for Vertex Array Objects (VAOs) to create
  for (auto& vao : sharedStateDynamic.VertexArrays().List()) {
    CreateVAO(scheduler, vao.Data());
  }

  // Bind original vao if used
  scheduler.Register(new CglBindVertexArray(currentlyBoundVAO));
}

/* ********************************** M A P 1 ******************************** */

const gits::OpenGL::CVariableMap1::TDataInfo gits::OpenGL::CVariableMap1::_dataInfo[] = {
    {GL_MAP1_VERTEX_3, 3},        {GL_MAP1_VERTEX_4, 4},        {GL_MAP1_INDEX, 1},
    {GL_MAP1_COLOR_4, 4},         {GL_MAP1_NORMAL, 3},          {GL_MAP1_TEXTURE_COORD_1, 1},
    {GL_MAP1_TEXTURE_COORD_2, 2}, {GL_MAP1_TEXTURE_COORD_3, 3}, {GL_MAP1_TEXTURE_COORD_4, 4}};

const unsigned gits::OpenGL::CVariableMap1::_dataNum =
    sizeof(_dataInfo) / sizeof(gits::OpenGL::CVariableMap1::TDataInfo);

gits::OpenGL::CVariableMap1::CVariableMap1() : _data(new TData[_dataNum]) {
  memset(_data, 0, _dataNum * sizeof(TData));
}

gits::OpenGL::CVariableMap1::~CVariableMap1() {
  for (unsigned i = 0; i < _dataNum; i++) {
    delete _data[i].points;
  }
  delete[] _data;
}

void gits::OpenGL::CVariableMap1::Get() {
  for (unsigned i = 0; i < _dataNum; i++) {
    if (drv.gl.glIsEnabled(_dataInfo[i].target)) {
      drv.gl.glGetMapdv(_dataInfo[i].target, GL_DOMAIN, &_data[i].u1);
      drv.gl.glGetMapiv(_dataInfo[i].target, GL_ORDER, &_data[i].order);
      if (!_data[i].points) {
        _data[i].points = new GLdouble[_data[i].order * _dataInfo[i].stride];
      }
      drv.gl.glGetMapdv(_dataInfo[i].target, GL_COEFF, _data[i].points);
    } else {
      if (_data[i].points) {
        delete[] _data[i].points;
        _data[i].points = nullptr;
      }
    }
  }
}

void gits::OpenGL::CVariableMap1::Schedule(CScheduler& scheduler,
                                           const CVariable& lastValue) const {
  const CVariableMap1& variable = static_cast<const CVariableMap1&>(lastValue);

  for (unsigned i = 0; i < _dataNum; i++) {
    if (_data[i].points) {
      if (variable._data[i].points) {
        // compare existing values
        if (!memcmp(_data[i].points, variable._data[i].points,
                    _data[i].order * _dataInfo[i].stride * sizeof(GLdouble)) &&
            _data[i].u1 == variable._data[i].u1 && _data[i].u2 == variable._data[i].u2 &&
            _data[i].order == variable._data[i].order) {
          continue;
        }
      }

      scheduler.Register(new CglMap1d(_dataInfo[i].target, _data[i].u1, _data[i].u2,
                                      _dataInfo[i].stride, _data[i].order, _data[i].points));
    }
  }
}

/* ********************************** M A P 2 ******************************** */

const gits::OpenGL::CVariableMap2::TDataInfo gits::OpenGL::CVariableMap2::_dataInfo[] = {
    {GL_MAP2_VERTEX_3, 3},        {GL_MAP2_VERTEX_4, 4},        {GL_MAP2_INDEX, 1},
    {GL_MAP2_COLOR_4, 4},         {GL_MAP2_NORMAL, 3},          {GL_MAP2_TEXTURE_COORD_1, 1},
    {GL_MAP2_TEXTURE_COORD_2, 2}, {GL_MAP2_TEXTURE_COORD_3, 3}, {GL_MAP2_TEXTURE_COORD_4, 4}};

const unsigned gits::OpenGL::CVariableMap2::_dataNum =
    sizeof(_dataInfo) / sizeof(gits::OpenGL::CVariableMap2::TDataInfo);

gits::OpenGL::CVariableMap2::CVariableMap2() : _data(new TData[_dataNum]) {
  memset(_data, 0, _dataNum * sizeof(TData));
}

gits::OpenGL::CVariableMap2::~CVariableMap2() {
  for (unsigned i = 0; i < _dataNum; i++) {
    delete _data[i].points;
  }
  delete[] _data;
}

void gits::OpenGL::CVariableMap2::Get() {
  for (unsigned i = 0; i < _dataNum; i++) {
    if (drv.gl.glIsEnabled(_dataInfo[i].target)) {
      drv.gl.glGetMapdv(_dataInfo[i].target, GL_DOMAIN, &_data[i].u1);
      drv.gl.glGetMapiv(_dataInfo[i].target, GL_ORDER, &_data[i].uorder);
      if (!_data[i].points) {
        _data[i].points = new GLdouble[_data[i].uorder * _data[i].vorder * _dataInfo[i].ustride];
      }
      drv.gl.glGetMapdv(_dataInfo[i].target, GL_COEFF, _data[i].points);
    } else {
      if (_data[i].points) {
        delete[] _data[i].points;
        _data[i].points = nullptr;
      }
    }
  }
}

void gits::OpenGL::CVariableMap2::Schedule(CScheduler& scheduler,
                                           const CVariable& lastValue) const {
  const CVariableMap2& variable = static_cast<const CVariableMap2&>(lastValue);

  for (unsigned i = 0; i < _dataNum; i++) {
    if (_data[i].points) {
      if (variable._data[i].points) {
        // compare existing values
        if (!memcmp(_data[i].points, variable._data[i].points,
                    _data[i].uorder * _data[i].vorder * _dataInfo[i].ustride * sizeof(GLdouble)) &&
            _data[i].u1 == variable._data[i].u1 && _data[i].u2 == variable._data[i].u2 &&
            _data[i].v1 == variable._data[i].v1 && _data[i].v2 == variable._data[i].v2 &&
            _data[i].uorder == variable._data[i].uorder &&
            _data[i].vorder == variable._data[i].vorder) {
          continue;
        }
      }

      scheduler.Register(new CglMap2d(_dataInfo[i].target, _data[i].u1, _data[i].u2,
                                      _dataInfo[i].ustride, _data[i].uorder, _data[i].v1,
                                      _data[i].v2, _dataInfo[i].ustride * _data[i].uorder,
                                      _data[i].vorder, _data[i].points));
    }
  }
}

namespace detail {
// template function coducts operations opLeft (on lonely left), opRight (on
// lonely right) or opBoth (in case element is in both sequences). Sequences are
// assumed to be sorted with pred.
template <typename Iter, typename OpLeft, typename OpBoth, typename OpRight, typename Pred>
void compareProcess(Iter lBegin,
                    Iter lEnd,
                    Iter rBegin,
                    Iter rEnd,
                    OpLeft opLeft,
                    OpBoth opBoth,
                    OpRight opRight,
                    Pred pred = std::less<typename Iter::value_type>()) {
  while (lBegin != lEnd && rBegin != rEnd) {
    const bool leftLess = pred(*lBegin, *rBegin);
    const bool rightLess = pred(*rBegin, *lBegin);
    const bool bothEqual = !(leftLess || rightLess);

    if (bothEqual) {
      opBoth(*lBegin, *rBegin);
      ++lBegin;
      ++rBegin;
    } else if (leftLess) {
      opLeft(*lBegin);
      ++lBegin;
    } else /*if( rightLess )*/ {
      opRight(*rBegin);
      ++rBegin;
    }
  }

  for (; lBegin != lEnd; ++lBegin) {
    opLeft(*lBegin);
  }

  for (; rBegin != rEnd; ++rBegin) {
    opRight(*rBegin);
  }
}

template <class T>
bool firstLess(const T& lhs, const T& rhs) {
  return lhs.first < rhs.first;
}
} // namespace detail

/****************************** DEFAULT FRAMEBUFER  I N F O **********************/

gits::OpenGL::CVariableDefaultFramebuffer::CVariableDefaultFramebuffer()
    : _supported(false),
      _width(0),
      _height(0),
      _csDatahash(0),
      _colorDatahash(0),
      _dsInternalformat(0),
      _dsFormat(0),
      _dsType(0),
      _dsAttachment(0),
      _colorInternalformat(0),
      _colorFormat(0),
      _colorType(0) {
  if (Config::Get().opengl.recorder.restoreDefaultFB &&
      (curctx::IsOgl() &&
       (curctx::Version() >= 300 || drv.gl.HasExtension("GL_ARB_framebuffer_object")))) {
    _supported = true;
  }
}

bool gits::OpenGL::CVariableDefaultFramebuffer::FormatSetup() {
  GLint depthsize, stencilsize;
  GLint viewportDims[4];

  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);

  drv.gl.glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                               GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthsize);
  drv.gl.glGetFramebufferAttachmentParameteriv(
      GL_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilsize);
  drv.gl.glGetIntegerv(GL_VIEWPORT, viewportDims);
  TODO("We need to change this from viewport into get_capture_viewport (need implementation for "
       "egl and glx first)")

  if (depthsize == 16 && stencilsize == 0) {
    _dsInternalformat = GL_DEPTH_COMPONENT16;
    _dsFormat = GL_DEPTH_COMPONENT;
    _dsType = GL_FLOAT;
    _dsAttachment = GL_DEPTH_ATTACHMENT;
  } else if (depthsize == 24 && stencilsize == 0) {
    _dsInternalformat = GL_DEPTH_COMPONENT24;
    _dsFormat = GL_DEPTH_COMPONENT;
    _dsType = GL_FLOAT;
    _dsAttachment = GL_DEPTH_ATTACHMENT;
  } else if (depthsize == 24 && stencilsize == 8) {
    _dsInternalformat = GL_DEPTH24_STENCIL8;
    _dsFormat = GL_DEPTH_STENCIL;
    _dsType = GL_UNSIGNED_INT_24_8;
    _dsAttachment = GL_DEPTH_STENCIL_ATTACHMENT;
    return false;
  } else {
    Log(WARN) << "Unknown DepthStencil format during restoration of default framebuffer.";
    Log(WARN) << "Depth value: " << depthsize << ", stencil value: " << stencilsize << ".";
    Log(WARN)
        << "Gits will skip restoration of default framebuffer, and continue to record normally.";
    return false;
  }

  _width = viewportDims[2];
  _height = viewportDims[3];

  _colorInternalformat = GL_RGBA;
  _colorFormat = GL_RGBA;
  _colorType = GL_UNSIGNED_BYTE;
  return true;
}

bool gits::OpenGL::CVariableDefaultFramebuffer::SaveFramebufferData(GLenum internalFormat,
                                                                    GLenum format,
                                                                    GLenum type,
                                                                    GLenum attachment) {
  // get Color data
  GLuint frameBuffer;
  drv.gl.glGenFramebuffers(1, &frameBuffer);
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

  GLuint origTexture;
  GLuint texture;
  drv.gl.glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&origTexture);
  drv.gl.glGenTextures(1, &texture);
  drv.gl.glBindTexture(GL_TEXTURE_2D, texture);
  drv.gl.glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _width, _height, 0, format, type, nullptr);

  drv.gl.glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);

  if (attachment != GL_COLOR_ATTACHMENT0) {
    drv.gl.glDrawBuffer(GL_NONE);
    drv.gl.glReadBuffer(GL_NONE);
  }

  GLenum fboStatus = drv.gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    Log(WARN) << "Default framebuffer restoration failed - framebuffer incomplete - FBO status: "
              << fboStatus;
    Log(WARN)
        << "Gits will skip restoration of default framebuffer, and continue to record normally.";
    drv.gl.glBindTexture(GL_TEXTURE_2D, origTexture);
    return false;
  }
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
  drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
  drv.gl.glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height, FboToBlitMask(attachment),
                           GL_NEAREST);

  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
  int dumpDataSize = texelSize(format, type) * _width * _height;
  std::vector<char> data(dumpDataSize);
  drv.gl.glReadPixels(0, 0, _width, _height, format, type, &data[0]);
  if (attachment == GL_COLOR_ATTACHMENT0) {
    _colorDatahash =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &data[0], dumpDataSize);
  } else {
    _csDatahash =
        CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, &data[0], dumpDataSize);
  }

  drv.gl.glDeleteTextures(1, &texture);
  drv.gl.glDeleteFramebuffers(1, &frameBuffer);
  drv.gl.glBindTexture(GL_TEXTURE_2D, origTexture);
  return true;
}

void gits::OpenGL::CVariableDefaultFramebuffer::Get() {
  if (!_supported) {
    return;
  }

  // Save current state (Read FBO, Draw FBO).
  GLint boundDrawFramebuffer = 0;
  GLint boundReadFramebuffer = 0;

  if (!drv.gl.HasExtension("GL_framebuffer_blit") &&
      !drv.gl.HasExtension("GL_EXT_framebuffer_blit") && !SD().IsCurrentContextCore()) {
    drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundDrawFramebuffer);
  } else {
    drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &boundDrawFramebuffer);
    drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundReadFramebuffer);
  }

  if (!(FormatSetup() &&
        SaveFramebufferData(_colorInternalformat, _colorFormat, _colorType, GL_COLOR_ATTACHMENT0) &&
        SaveFramebufferData(_dsInternalformat, _dsFormat, _dsType, _dsAttachment))) {
    _supported = false;
  }

  // Restore the state we saved (Read FBO, Draw FBO).
  if (!drv.gl.HasExtension("GL_framebuffer_blit") &&
      !drv.gl.HasExtension("GL_EXT_framebuffer_blit") && !SD().IsCurrentContextCore()) {
    drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, boundDrawFramebuffer);
  } else {
    drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, boundDrawFramebuffer);
    drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, boundReadFramebuffer);
  }
}

void gits::OpenGL::CVariableDefaultFramebuffer::Schedule(CScheduler& scheduler,
                                                         const CVariable& lastValue) const {
  // Schedule clears
  // Default framebuffer does not have to be bound there as we are in shared
  // state restoration before and FBOs weren't recreated yet
  scheduler.Register(new CglClearColor(0, 0, 0, 1));
  if (curctx::IsOgl()) {
    scheduler.Register(new CglClearDepth(1));
  } else {
    scheduler.Register(new CglClearDepthf(1));
  }
  scheduler.Register(new CglClearStencil(0));
  scheduler.Register(
      new CglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

  if (_supported) {
    scheduler.Register(new CRestoreDefaultGLFramebuffer(
        _dsInternalformat, _dsFormat, _dsType, _dsAttachment, _colorInternalformat, _colorFormat,
        _colorType, _width, _height, _csDatahash, _colorDatahash));
  }
}

/****************************** R E N D E R B U F F E R EXT  I N F O **********************/

bool gits::OpenGL::CVariableRenderbufferEXTInfo::_supported = false;

gits::OpenGL::CVariableRenderbufferEXTInfo::CVariableRenderbufferEXTInfo() {
  _supported = drv.gl.HasExtension("GL_EXT_framebuffer_object");
}

void gits::OpenGL::CVariableRenderbufferEXTInfo::Get() {
  if (!_supported) {
    return;
  }

  auto& renderbuffers = SD().GetCurrentSharedStateData().RenderbuffersEXT().List();
  auto iter = renderbuffers.begin();
  auto end = renderbuffers.end();

  if (iter == end) {
    return;
  }

  GLuint currentlyBoundRenderbufferEXT = 0;
  drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, (GLint*)&currentlyBoundRenderbufferEXT);

  for (; iter != end; ++iter) {
    GLuint name = iter->Name();
    GLenum target = iter->Target();
    GLint width = 0, height = 0, internalformat = 0;
    hash_t datahash = 0;

    if (iter->Name() == 0) {
      continue; // Parameters are not applicable for 0 object
    }

    drv.gl.glBindRenderbufferEXT(target, name);
    drv.gl.glGetRenderbufferParameterivEXT(target, GL_RENDERBUFFER_WIDTH_EXT, &width);
    drv.gl.glGetRenderbufferParameterivEXT(target, GL_RENDERBUFFER_HEIGHT_EXT, &height);
    drv.gl.glGetRenderbufferParameterivEXT(target, GL_RENDERBUFFER_INTERNAL_FORMAT_EXT,
                                           &internalformat);
    GLint currentSamples = 0;
    drv.gl.glGetRenderbufferParameterivEXT(target, GL_RENDERBUFFER_SAMPLES_EXT, &currentSamples);
    Log(WARN) << "For renderbuffer: " << iter->Name()
              << " there were: " << iter->Data().track.samples
              << " requested. Value returned by the driver is: " << currentSamples
              << ". Potential problems with MSAA buffers contents restoration!!!";

    // Check Render buffer content
    GLint depthSize = 0;
    GLint stencilSize = 0;
    GLint samples = iter->Data().track.samples;
    ;
    drv.gl.glGetRenderbufferParameterivEXT(target, GL_RENDERBUFFER_DEPTH_SIZE_EXT, &depthSize);
    drv.gl.glGetRenderbufferParameterivEXT(target, GL_RENDERBUFFER_STENCIL_SIZE_EXT, &stencilSize);
    // Get Render Buffer Content if renderbuffer is not multisampled
    if (samples == 0 && width != 0 && height != 0) {
      // save bound framebuffers
      std::map<GLenum, GLint> boundframebuffers;
      GLint boundfbo = 0;
      if (!drv.gl.HasExtension("GL_EXT_framebuffer_blit")) {
        drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &boundfbo);
        boundframebuffers[GL_FRAMEBUFFER_EXT] = boundfbo;
      } else {
        drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &boundfbo);
        boundframebuffers[GL_DRAW_FRAMEBUFFER_EXT] = boundfbo;

        drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &boundfbo);
        boundframebuffers[GL_READ_FRAMEBUFFER_EXT] = boundfbo;
      }

      GLenum tmpFboAttachmentType = InternalFormatToFboMatch(internalformat);
      GLenum dumpPixelFormat = 0;
      GLenum dumpPixelType = 0;
      unsigned dumpDataSize = 0;

      FboToImageMatch(tmpFboAttachmentType, dumpPixelFormat, dumpPixelType);
      dumpDataSize = texelSize(dumpPixelFormat, dumpPixelType) * width * height;

      // attach RBO to new FBO
      GLuint tmpFBO = 0;
      drv.gl.glGenFramebuffersEXT(1, &tmpFBO);
      drv.gl.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tmpFBO);
      drv.gl.glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, tmpFboAttachmentType, GL_RENDERBUFFER,
                                          name);
      if (tmpFboAttachmentType != GL_COLOR_ATTACHMENT0) {
        drv.gl.glDrawBuffer(GL_NONE);
        drv.gl.glReadBuffer(GL_NONE);
      }
      GLenum fboStatus = drv.gl.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
      if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        Log(ERR) << "Renderbuffer restoration failed - Framebuffer incomplete - FBO status: "
                 << fboStatus;
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }

      // Pixel pack buffer can't be bound while using glGetTexImage
      GLint origPbo = 0;
      if (curctx::IsOgl()) {
        drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &origPbo);
        if (origPbo != 0) {
          drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
      }

      // get RBO content
      std::vector<char> data(dumpDataSize); // GL_RGBA - 4 values
      drv.gl.glReadPixels(0, 0, width, height, dumpPixelFormat, dumpPixelType, &data[0]);
      datahash = CGits::Instance().ResourceManager2().put(RESOURCE_BUFFER, &data[0], dumpDataSize);

      if (origPbo != 0) {
        drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, origPbo);
      }

      // delete FBO
      drv.gl.glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, tmpFboAttachmentType, GL_RENDERBUFFER,
                                          0);
      drv.gl.glDeleteFramebuffersEXT(1, &tmpFBO);

      // bind previously bound framebuffer
      for (const auto& boundFboElem : boundframebuffers) {
        drv.gl.glBindFramebufferEXT(boundFboElem.first, boundFboElem.second);
      }
    } else {
      datahash = 0;
      Log(WARN) << "Can't restore renderbuffer content (multisampled renderbuffers content "
                   "restoration not supported).";
    }

    GCC433WA_0(iter)->Data().restore.width = width;
    GCC433WA_0(iter)->Data().restore.height = height;
    GCC433WA_0(iter)->Data().restore.internalformat = internalformat;
    GCC433WA_0(iter)->Data().track.samples = samples;
    GCC433WA_0(iter)->Data().restore.datahash = datahash;
  }

  drv.gl.glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, currentlyBoundRenderbufferEXT);
}

void gits::OpenGL::CVariableRenderbufferEXTInfo::createRenderbuffer(
    CScheduler& scheduler, const CRenderbufferStateData& value) {
  if (value.track.name != 0) {
    scheduler.Register(new CglBindRenderbufferEXT(value.track.target, value.track.name));

    if (value.restore.width == 0 || value.restore.height == 0) {
      return; // Skip renderbuffer initialization if not initialized
    }

    if (value.track.samples < 2) {
      scheduler.Register(new CglRenderbufferStorageEXT(value.track.target,
                                                       value.restore.internalformat,
                                                       value.restore.width, value.restore.height));
      scheduler.Register(new CgitsRenderbufferStorage(
          value.track.target, value.restore.internalformat, value.restore.width,
          value.restore.height, value.restore.datahash,
          CgitsRenderbufferStorage::RenderbufferStorageType::RENDER_BUFFER_EXT));
    } else {
      scheduler.Register(new CglRenderbufferStorageMultisampleEXT(
          value.track.target, value.track.samples, value.restore.internalformat,
          value.restore.width, value.restore.height));
    }
  }
}

void gits::OpenGL::CVariableRenderbufferEXTInfo::Schedule(CScheduler& scheduler,
                                                          const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  auto& renderbuffersEXT = SD().GetCurrentSharedStateData().RenderbuffersEXT().List();
  for (auto& rbo : renderbuffersEXT) {
    createRenderbuffer(scheduler, rbo.Data());
  }

  // binding through EXT api modifies the same state, and we preffer for this to
  // happen through core api
}

/****************************** R E N D E R B U F F E R EXT  B I N D I N G **********************/

bool gits::OpenGL::CVariableRenderbufferEXTBinding::_supported = false;

gits::OpenGL::CVariableRenderbufferEXTBinding::CVariableRenderbufferEXTBinding()
    : _boundRenderbufferEXT(0) {
  _supported = drv.gl.HasExtension("GL_EXT_framebuffer_object");
}

void gits::OpenGL::CVariableRenderbufferEXTBinding::Get() {
  if (!_supported) {
    return;
  }
  drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &_boundRenderbufferEXT);
}

void gits::OpenGL::CVariableRenderbufferEXTBinding::Schedule(CScheduler& scheduler,
                                                             const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  if (_boundRenderbufferEXT != 0) {
    scheduler.Register(new CglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _boundRenderbufferEXT));
  }
}

/****************************** R E N D E R B U F F E R  I N F O **********************/

bool gits::OpenGL::CVariableRenderbufferInfo::_supported = false;

gits::OpenGL::CVariableRenderbufferInfo::CVariableRenderbufferInfo() {
  if (curctx::IsEs2Plus() ||
      (curctx::IsOgl() &&
       (curctx::Version() >= 300 || drv.gl.HasExtension("GL_ARB_framebuffer_object")))) {
    _supported = true;
  }
}

void gits::OpenGL::CVariableRenderbufferInfo::Get() {
  if (!_supported) {
    return;
  }

  auto& renderbuffers = SD().GetCurrentSharedStateData().Renderbuffers().List();
  auto iter = renderbuffers.begin();
  auto end = renderbuffers.end();

  GLuint currentlyBoundRenderbuffer = 0;
  drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING, (GLint*)&currentlyBoundRenderbuffer);

  GLint origPbo = 0;
  if (curctx::IsOgl()) {
    drv.gl.glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &origPbo);
    if (origPbo != 0) {
      drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
  }

  // Prepare PixelStorei
  GLint origPackAlignment = 0;
  drv.gl.glGetIntegerv(GL_PACK_ALIGNMENT, &origPackAlignment);
  drv.gl.glPixelStorei(GL_PACK_ALIGNMENT, 1);

  GLint origPackRowLength = 0;
  drv.gl.glGetIntegerv(GL_PACK_ROW_LENGTH, &origPackRowLength);
  drv.gl.glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  for (; iter != end; ++iter) {
    GLuint name = iter->Name();
    GLenum target = iter->Target();
    GLint width = 0, height = 0, internalformat = 0;
    GLint samples = iter->Data().track.samples;
    ;
    hash_t datahash = 0;

    if (iter->Name() == 0 || iter->Target() == 0) {
      continue; // Parameters are not applicable for 0 object
    }

    drv.gl.glBindRenderbuffer(target, name);

    drv.gl.glGetRenderbufferParameteriv(target, GL_RENDERBUFFER_WIDTH, &width);
    drv.gl.glGetRenderbufferParameteriv(target, GL_RENDERBUFFER_HEIGHT, &height);
    drv.gl.glGetRenderbufferParameteriv(target, GL_RENDERBUFFER_INTERNAL_FORMAT, &internalformat);

    // Check Render buffer content
    GLint depthSize = 0;
    GLint stencilSize = 0;
    drv.gl.glGetRenderbufferParameteriv(target, GL_RENDERBUFFER_DEPTH_SIZE, &depthSize);
    drv.gl.glGetRenderbufferParameteriv(target, GL_RENDERBUFFER_STENCIL_SIZE, &stencilSize);
    GLint currentSamples = 0;
    drv.gl.glGetRenderbufferParameteriv(target, GL_RENDERBUFFER_SAMPLES, &currentSamples);
    if (currentSamples > 0 && iter->Data().track.samples != static_cast<GLuint>(currentSamples)) {
      Log(WARN) << "For renderbuffer: " << iter->Name()
                << " there were: " << iter->Data().track.samples
                << "samples requested. Value returned by the driver is: " << currentSamples
                << ". Potential problems with MSAA buffers contents restoration!!!";
    }

    // Get Render Buffer Content if renderbuffer contains color and is not tied
    // with eglImage
    if (width != 0 && height != 0 && curctx::IsOgl() && iter->Data().track.eglImage == nullptr &&
        InternalFormatToFboMatch(internalformat) != GL_STENCIL_ATTACHMENT) {
      if (samples > 0) {
        Log(WARN) << "MSAA renderbuffer will be resolved to single image! This may cause "
                     "corruptions. Consider disabling MSAA renderbuffers if possible.";
      }

      // save bound framebuffers
      std::map<GLenum, GLint> boundframebuffers;
      GLint boundfbo = 0;
      if (!drv.gl.HasExtension("GL_framebuffer_blit") &&
          !drv.gl.HasExtension("GL_EXT_framebuffer_blit") && !SD().IsCurrentContextCore()) {
        drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundfbo);
        boundframebuffers[GL_FRAMEBUFFER] = boundfbo;
      } else {
        drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &boundfbo);
        boundframebuffers[GL_DRAW_FRAMEBUFFER] = boundfbo;

        drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundfbo);
        boundframebuffers[GL_READ_FRAMEBUFFER] = boundfbo;
      }

      GLenum tmpFboAttachmentType = InternalFormatToFboMatch(internalformat);
      GLenum dumpPixelFormat = 0;
      GLenum dumpPixelType = 0;
      unsigned dumpDataSize = 0;

      FboToImageMatch(tmpFboAttachmentType, dumpPixelFormat, dumpPixelType);
      dumpDataSize = texelSize(dumpPixelFormat, dumpPixelType) * width * height;

      // attach RBO to new FBO
      GLuint tmpFBO = 0;
      drv.gl.glGenFramebuffers(1, &tmpFBO);
      drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, tmpFBO);
      drv.gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, tmpFboAttachmentType, GL_RENDERBUFFER, name);
      if (tmpFboAttachmentType != GL_COLOR_ATTACHMENT0) {
        drv.gl.glDrawBuffer(GL_NONE);
        drv.gl.glReadBuffer(GL_NONE);
      }
      GLenum fboStatus = drv.gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        Log(ERR) << "Renderbuffer restoration failed - Framebuffer incomplete - FBO status: "
                 << fboStatus;
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }

      // get RBO content
      std::vector<char> data(dumpDataSize);
      drv.gl.glReadPixels(0, 0, width, height, dumpPixelFormat, dumpPixelType, &data[0]);
      datahash = CGits::Instance().ResourceManager2().put(RESOURCE_BUFFER, &data[0], dumpDataSize);

      // delete FBO
      drv.gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, tmpFboAttachmentType, GL_RENDERBUFFER, 0);
      drv.gl.glDeleteFramebuffers(1, &tmpFBO);

      // bind previously bound framebuffer
      for (const auto& boundFboElem : boundframebuffers) {
        drv.gl.glBindFramebuffer(boundFboElem.first, boundFboElem.second);
      }
    } else {
      datahash = 0;
      Log(WARN) << "Can't restore renderbuffer content (multisampled renderbuffers content "
                   "restoration not supported).";
    }

    GCC433WA_0(iter)->Data().restore.width = width;
    GCC433WA_0(iter)->Data().restore.height = height;
    GCC433WA_0(iter)->Data().restore.internalformat = internalformat;
    GCC433WA_0(iter)->Data().track.samples = samples;
    GCC433WA_0(iter)->Data().restore.datahash = datahash;
  }

  if (origPbo != 0) {
    drv.gl.glBindBuffer(GL_PIXEL_PACK_BUFFER, origPbo);
  }
  drv.gl.glPixelStorei(GL_PACK_ALIGNMENT, origPackAlignment);
  drv.gl.glPixelStorei(GL_PACK_ROW_LENGTH, origPackRowLength);

  drv.gl.glBindRenderbuffer(GL_RENDERBUFFER, currentlyBoundRenderbuffer);
}

void gits::OpenGL::CVariableRenderbufferInfo::createRenderbuffer(
    CScheduler& scheduler, const CRenderbufferStateData& value) {
  GLuint rboname = value.track.name; // copy RBO name to non const var.
  if (rboname != 0) {
    scheduler.Register(new CglGenRenderbuffers(1, &rboname));
    if (value.track.target == 0) {
      return;
    }

    scheduler.Register(new CglBindRenderbuffer(value.track.target, value.track.name));

    if (value.track.eglImage != nullptr) {
      // Content of eglImage is not restored. We are assuming it was not changed
      // since last call to eglCreateImageKHR.
      scheduler.Register(
          new CglEGLImageTargetRenderbufferStorageOES(value.track.target, value.track.eglImage));
      return;
    }

    if (value.restore.width == 0 || value.restore.height == 0) {
      return; // Skip renderbuffer initialization if not initialized
    }

    if (value.track.samples < 2) {
      scheduler.Register(new CglRenderbufferStorage(value.track.target,
                                                    value.restore.internalformat,
                                                    value.restore.width, value.restore.height));
      scheduler.Register(new CgitsRenderbufferStorage(
          value.track.target, value.restore.internalformat, value.restore.width,
          value.restore.height, value.restore.datahash,
          CgitsRenderbufferStorage::RenderbufferStorageType::RENDER_BUFFER));
    } else {
      scheduler.Register(new CglRenderbufferStorageMultisample(
          value.track.target, value.track.samples, value.restore.internalformat,
          value.restore.width, value.restore.height));
    }
  }
}

void gits::OpenGL::CVariableRenderbufferInfo::Schedule(CScheduler& scheduler,
                                                       const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  // store state at witch we obtain textures
  scheduler.Register(new CglPixelStorei(GL_UNPACK_ALIGNMENT, 1));
  scheduler.Register(new CglPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
  scheduler.Register(new CglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

  auto& renderbuffers = SD().GetCurrentSharedStateData().Renderbuffers().List();
  for (auto& rbo : renderbuffers) {
    createRenderbuffer(scheduler, rbo.Data());
  }
}

/****************************** R E N D E R B U F F E R  B I N D I N G **********************/

bool gits::OpenGL::CVariableRenderbufferBinding::_supported = false;

gits::OpenGL::CVariableRenderbufferBinding::CVariableRenderbufferBinding() : _boundRenderbuffer(0) {
  if (curctx::IsEs2Plus() ||
      (curctx::IsOgl() &&
       (curctx::Version() >= 300 || drv.gl.HasExtension("GL_ARB_framebuffer_object")))) {
    _supported = true;
  }
}

void gits::OpenGL::CVariableRenderbufferBinding::Get() {
  if (!_supported) {
    return;
  }

  drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING, &_boundRenderbuffer);
}

void gits::OpenGL::CVariableRenderbufferBinding::Schedule(CScheduler& scheduler,
                                                          const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }

  if (_boundRenderbuffer != 0) {
    scheduler.Register(new CglBindRenderbuffer(GL_RENDERBUFFER, _boundRenderbuffer));
  }
}

/****************************** F R A M E B U F F E R   G I T S   I N F O ************************/
bool gits::OpenGL::CVariableFramebufferInfoGITS::_supported = false;
GLint gits::OpenGL::CVariableFramebufferInfoGITS::_maxDrawBuffers = 0;
gits::CScheduler* gits::OpenGL::CVariableFramebufferInfoGITS::_scheduler = nullptr;
const gits::OpenGL::CVariableFramebufferInfoGITS*
    gits::OpenGL::CVariableFramebufferInfoGITS::_currinstance = nullptr;

void gits::OpenGL::CVariableFramebufferInfoGITS::getAttachmentInfo(GLenum targetType,
                                                                   GLenum attachmentPoint,
                                                                   GLenum& type,
                                                                   GLuint& name,
                                                                   GLenum& cubeFace,
                                                                   GLint& layer3d,
                                                                   GLint& level) {
  GLint tmp;
  glGetFramebufferAttachmentParameterivGITS(targetType, attachmentPoint,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &tmp);
  type = tmp;
  glGetFramebufferAttachmentParameterivGITS(targetType, attachmentPoint,
                                            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &tmp);
  name = tmp;
  glGetFramebufferAttachmentParameterivGITS(targetType, attachmentPoint,
                                            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, &tmp);
  cubeFace = tmp;
  glGetFramebufferAttachmentParameterivGITS(targetType, attachmentPoint,
                                            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER, &tmp);
  layer3d = tmp;
  glGetFramebufferAttachmentParameterivGITS(targetType, attachmentPoint,
                                            GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &tmp);
  level = tmp;
}

void gits::OpenGL::CVariableFramebufferInfoGITS::Get() {
  if (!_supported) {
    return;
  }

  auto& framebuffers = GetFramebufferIdList();

  auto iter = framebuffers.begin();
  auto end = framebuffers.end();
  GLint maxColorAttachments = 0;
  if (curctx::IsEs2Plus()) {
    maxColorAttachments = 1;
  } else {
    drv.gl.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
  }

  drv.gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &_maxDrawBuffers);

  for (; iter != end; ++iter) {
    // skip default framebuffer - we can't change its attachements anyways
    if (iter->Name() == 0 || iter->Target() == 0) {
      continue;
    }

    glBindFramebufferGITS(GL_FRAMEBUFFER, iter->Name());
    // get color attachments info
    for (auto& attachElem : GCC433WA_0(iter)->Attachments()) {
      getAttachmentInfo(GL_FRAMEBUFFER, attachElem.first, attachElem.second.type,
                        attachElem.second.name, attachElem.second.cubeFace,
                        attachElem.second.layer3d, attachElem.second.level);
    }
    if (GCC433WA_0(iter)->Attachments()[GL_DEPTH_ATTACHMENT_EXT].name != 0 &&
        GCC433WA_0(iter)->Attachments()[GL_STENCIL_ATTACHMENT_EXT].name != 0) {
      GCC433WA_0(iter)->Attachments()[GL_STENCIL_ATTACHMENT_EXT] =
          GCC433WA_0(iter)->Attachments()[GL_DEPTH_ATTACHMENT_EXT];
      Log(WARN) << "Depth stencil attachment driver WA";
    }

    if (!curctx::IsEs2Plus()) {
      // get read buffer
      drv.gl.glGetIntegerv(GL_READ_BUFFER, &(GCC433WA_0(iter)->Data().restore.readBuffer));
    }
  }
}

void gits::OpenGL::CVariableFramebufferInfoGITS::createFramebuffer(
    const CFramebufferStateData& value) {
  GLuint fboname = value.track.name; // Copy FBO name to non const var.
  _currinstance->ScheduleGenFramebuffers(1, &fboname);
  _currinstance->ScheduleBindFramebuffer(GL_FRAMEBUFFER, value.track.name);

  // Create attachments.
  for (const auto& attachment : value.track.attachments) {
    const CFramebufferStateData::TAttachmentInfo& attInfo = attachment.second;
    // Skip default framebuffer.
    if (attInfo.name == 0) {
      continue;
    }
    if (attInfo.type == GL_NONE) {
      continue;
    }
    if (attInfo.type == GL_RENDERBUFFER) {
      _currinstance->ScheduleFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment.first,
                                                     GL_RENDERBUFFER, attInfo.name);
    } else if (attInfo.type == GL_TEXTURE) {
      // Find texture and its target / check if texture still exists.
      CTextureStateObj* texState = SD().GetCurrentSharedStateData().Textures().Get(attInfo.name);
      GLenum target;
      if (texState != nullptr) {
        target = texState->Target();
      } else {
        continue;
      }
      switch (target) {
      case GL_TEXTURE_1D:
        _currinstance->ScheduleFramebufferTexture1D(GL_FRAMEBUFFER, attachment.first, target,
                                                    attInfo.name, attInfo.level);
        break;
      case GL_TEXTURE_1D_ARRAY:
        _currinstance->ScheduleFramebufferTextureLayer(
            GL_FRAMEBUFFER, attachment.first, attInfo.name, attInfo.level, attInfo.layer3d);
        break;
      case GL_TEXTURE_2D:
      case GL_TEXTURE_2D_MULTISAMPLE:
      case GL_TEXTURE_RECTANGLE_ARB:
        _currinstance->ScheduleFramebufferTexture2D(GL_FRAMEBUFFER, attachment.first, target,
                                                    attInfo.name, attInfo.level);
        break;
      case GL_TEXTURE_2D_ARRAY:
      case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        _currinstance->ScheduleFramebufferTextureLayer(
            GL_FRAMEBUFFER, attachment.first, attInfo.name, attInfo.level, attInfo.layer3d);
        break;
      case GL_TEXTURE_3D:
        _currinstance->ScheduleFramebufferTexture3D(GL_FRAMEBUFFER, attachment.first, target,
                                                    attInfo.name, attInfo.level, attInfo.layer3d);
        break;
      case GL_TEXTURE_CUBE_MAP:
        _currinstance->ScheduleFramebufferTexture2D(GL_FRAMEBUFFER, attachment.first,
                                                    attInfo.cubeFace, attInfo.name, attInfo.level);
        break;
      default:
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    } else {
      throw ENotSupported(EXCEPTION_MESSAGE);
    }
  }

  if (curctx::IsOgl() || curctx::IsEs3Plus()) {
    // Create draw buffers.
    std::vector<GLenum> drawBuffers(1, GL_COLOR_ATTACHMENT0);
    drawBuffers.resize(_maxDrawBuffers, GL_NONE);
    if (value.track.drawBuffers.size() != 0) {
      drawBuffers = value.track.drawBuffers;
    }
    _scheduler->Register(new CglDrawBuffers(_maxDrawBuffers, drawBuffers.data()));

    // Create read buffer.
    if (value.restore.readBuffer != GL_COLOR_ATTACHMENT0) {
      _scheduler->Register(new CglReadBuffer(value.restore.readBuffer));
    }
  }

  _currinstance->ScheduleCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (!curctx::IsOgl()) {
    _scheduler->Register(new CglClear(GL_DEPTH_BUFFER_BIT));
  }
}

void gits::OpenGL::CVariableFramebufferInfoGITS::Schedule(CScheduler& scheduler,
                                                          const CVariable& lastValue) const {
  // set context for 'processing functions'
  _scheduler = &scheduler;
  _currinstance = this;

  for (auto& fbo : GetFramebufferIdList()) {
    createFramebuffer(fbo.Data());
  }
}

/****************************** F R A M E B U F F E R   E X T   I N F O ************************/

gits::OpenGL::CVariableFramebufferEXTInfo::CVariableFramebufferEXTInfo() {
  _boundFramebuffer = 0;
  _supported = drv.gl.HasExtension("GL_EXT_framebuffer_object");
}

void gits::OpenGL::CVariableFramebufferEXTInfo::glGetFramebufferAttachmentParameterivGITS(
    GLenum target, GLenum attachment, GLenum pname, GLint* params) const {
  drv.gl.glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);
}

void gits::OpenGL::CVariableFramebufferEXTInfo::glBindFramebufferGITS(GLenum target,
                                                                      GLuint framebuffer) const {
  drv.gl.glBindFramebufferEXT(target, framebuffer);
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleGenFramebuffers(
    GLsizei n, GLuint framebuffers[]) const {
  _scheduler->Register(new CglGenFramebuffersEXT(n, framebuffers));
}
void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleBindFramebuffer(GLenum target,
                                                                        GLuint framebuffer) const {
  _scheduler->Register(new CglBindFramebufferEXT(target, framebuffer));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleFramebufferRenderbuffer(
    GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const {
  _scheduler->Register(
      new CglFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleFramebufferTexture1D(
    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const {
  _scheduler->Register(
      new CglFramebufferTexture1DEXT(target, attachment, textarget, texture, level));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleFramebufferTextureLayer(
    GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const {
  _scheduler->Register(
      new CglFramebufferTextureLayerEXT(target, attachment, texture, level, layer));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleFramebufferTexture2D(
    GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const {
  _scheduler->Register(new CglFramebufferTexture2DEXT(target, attachment, texture, level, layer));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleFramebufferTexture3D(GLenum target,
                                                                             GLenum attachment,
                                                                             GLenum textarget,
                                                                             GLuint texture,
                                                                             GLint level,
                                                                             GLint zoffset) const {
  _scheduler->Register(
      new CglFramebufferTexture3DEXT(target, attachment, textarget, texture, level, zoffset));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::ScheduleCheckFramebufferStatus(
    GLenum target) const {
  _scheduler->Register(new CglCheckFramebufferStatusEXT(GL_NONE, target));
}

void gits::OpenGL::CVariableFramebufferEXTInfo::Get() {
  if (!_supported) {
    return;
  }
  if (GetFramebufferIdList().empty()) {
    return;
  }

  // Save old binding
  drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &_boundFramebuffer);

  CVariableFramebufferInfoGITS::Get();
  // Restore binding
  drv.gl.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _boundFramebuffer);
}

void gits::OpenGL::CVariableFramebufferEXTInfo::Schedule(CScheduler& scheduler,
                                                         const CVariable& lastValue) const {
  if (GetFramebufferIdList().empty()) {
    return;
  }
  CVariableFramebufferInfoGITS::Schedule(scheduler, lastValue);

  scheduler.Register(new CglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _boundFramebuffer));
}

/****************************** F R A M E B U F F E R   I N F O ************************/

gits::OpenGL::CVariableFramebufferInfo::CVariableFramebufferInfo() {
  if (curctx::IsEs2Plus() ||
      (curctx::IsOgl() &&
       (curctx::Version() >= 300 || drv.gl.HasExtension("GL_ARB_framebuffer_object")))) {
    _supported = true;
  }
}

void gits::OpenGL::CVariableFramebufferInfo::glGetFramebufferAttachmentParameterivGITS(
    GLenum target, GLenum attachment, GLenum pname, GLint* params) const {
  drv.gl.glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

void gits::OpenGL::CVariableFramebufferInfo::glBindFramebufferGITS(GLenum target,
                                                                   GLuint framebuffer) const {
  drv.gl.glBindFramebuffer(target, framebuffer);
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleGenFramebuffers(GLsizei n,
                                                                     GLuint framebuffers[]) const {
  _scheduler->Register(new CglGenFramebuffers(n, framebuffers));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleBindFramebuffer(GLenum target,
                                                                     GLuint framebuffer) const {
  _scheduler->Register(new CglBindFramebuffer(target, framebuffer));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleFramebufferRenderbuffer(
    GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) const {
  _scheduler->Register(
      new CglFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleFramebufferTexture1D(
    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const {
  _scheduler->Register(new CglFramebufferTexture1D(target, attachment, textarget, texture, level));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleFramebufferTextureLayer(
    GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const {
  _scheduler->Register(new CglFramebufferTextureLayer(target, attachment, texture, level, layer));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleFramebufferTexture2D(
    GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const {
  _scheduler->Register(new CglFramebufferTexture2D(target, attachment, texture, level, layer));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleFramebufferTexture3D(GLenum target,
                                                                          GLenum attachment,
                                                                          GLenum textarget,
                                                                          GLuint texture,
                                                                          GLint level,
                                                                          GLint zoffset) const {
  _scheduler->Register(
      new CglFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset));
}

void gits::OpenGL::CVariableFramebufferInfo::ScheduleCheckFramebufferStatus(GLenum target) const {
  _scheduler->Register(new CglCheckFramebufferStatus(true, target));
}

void gits::OpenGL::CVariableFramebufferInfo::Get() {
  bool hasExtension = false;
  GLint framebuffer = -1;

  if (!_supported) {
    return;
  }
  if (GetFramebufferIdList().empty()) {
    return;
  }

  // save framebuffers bound before Get
  hasExtension = drv.gl.HasExtension("GL_EXT_framebuffer_blit") || SD().IsCurrentContextCore();
  if (!hasExtension) {
    drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
    _boundframebuffers[GL_FRAMEBUFFER] = framebuffer;
  } else {
    drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    _boundframebuffers[GL_DRAW_FRAMEBUFFER] = framebuffer;

    drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &framebuffer);
    _boundframebuffers[GL_READ_FRAMEBUFFER] = framebuffer;
  }

  CVariableFramebufferInfoGITS::Get();

  // bind previously bound framebuffer
  for (auto& boundFboElem : _boundframebuffers) {
    drv.gl.glBindFramebuffer(boundFboElem.first, boundFboElem.second);
  }
}

void gits::OpenGL::CVariableFramebufferInfo::Schedule(CScheduler& scheduler,
                                                      const CVariable& lastValue) const {
  if (GetFramebufferIdList().empty()) {
    return;
  }

  CVariableFramebufferInfoGITS::Schedule(scheduler, lastValue);

  // Schedule previously bound framebuffer binding.
  CBoundFramebuffers::const_iterator boundFBOIter;
  if (_boundframebuffers.size() == 2) {
    CBoundFramebuffers::const_iterator readFBOIter = _boundframebuffers.find(GL_READ_FRAMEBUFFER);
    CBoundFramebuffers::const_iterator drawFBOIter = _boundframebuffers.find(GL_DRAW_FRAMEBUFFER);
    if (readFBOIter == _boundframebuffers.end() || drawFBOIter == _boundframebuffers.end()) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " framebuffer not found");
    }

    if (readFBOIter->second == drawFBOIter->second) {
      // GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER targets bound to one
      // framebuffer.
      scheduler.Register(
          new CglBindFramebuffer(GL_FRAMEBUFFER, _boundframebuffers.begin()->second));
    } else {
      // GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER targets bound to different framebuffers.
      for (boundFBOIter = _boundframebuffers.begin(); boundFBOIter != _boundframebuffers.end();
           ++boundFBOIter) {
        scheduler.Register(new CglBindFramebuffer(boundFBOIter->first, boundFBOIter->second));
      }
    }
  } else if (_boundframebuffers.size() == 1) {
    // GL_DRAW_FRAMEBUFFER or GL_READ_FRAMEBUFFER target bound to one framebuffer.
    boundFBOIter = _boundframebuffers.begin();
    scheduler.Register(new CglBindFramebuffer(boundFBOIter->first, boundFBOIter->second));
  } else if (_boundframebuffers.size() > 2) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

/****************************** P O I N T   P A R A M E T E R **************************/

gits::OpenGL::CVariablePointParameter::CVariablePointParameter() {
  _scalarArgs[0] = 0.0f;          // GL_POINT_SIZE_MIN
  _scalarArgs[1] = 1.0f;          // GL_POINT_SIZE_MAX
  _scalarArgs[2] = 1.0f;          // GL_POINT_FADE_THRESHOLD_SIZE
  _scalarArgs[3] = GL_UPPER_LEFT; // GL_POINT_SPRITE_COORD_ORIGIN
  _distAtten[0] = 1.0f;           // GL_POINT_DISTANCE_ATTENUATION
  _distAtten[1] = 0.0f;
  _distAtten[2] = 0.0f;
}

void gits::OpenGL::CVariablePointParameter::Get() {
  drv.gl.glGetFloatv(GL_POINT_SIZE_MIN, &_scalarArgs[0]);
  drv.gl.glGetFloatv(GL_POINT_SIZE_MAX, &_scalarArgs[1]);
  drv.gl.glGetFloatv(GL_POINT_FADE_THRESHOLD_SIZE, &_scalarArgs[2]);
  drv.gl.glGetFloatv(GL_POINT_SPRITE_COORD_ORIGIN, &_scalarArgs[3]);

  drv.gl.glGetFloatv(GL_POINT_DISTANCE_ATTENUATION, _distAtten);
}

void gits::OpenGL::CVariablePointParameter::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  const CVariablePointParameter& variable = static_cast<const CVariablePointParameter&>(lastValue);
  if (!floats_close(_scalarArgs[0], variable._scalarArgs[0])) {
    scheduler.Register(new CglPointParameterfv(GL_POINT_SIZE_MIN, &_scalarArgs[0]));
  }
  if (!floats_close(_scalarArgs[1], variable._scalarArgs[1])) {
    scheduler.Register(new CglPointParameterfv(GL_POINT_SIZE_MAX, &_scalarArgs[1]));
  }
  if (!floats_close(_scalarArgs[2], variable._scalarArgs[2])) {
    scheduler.Register(new CglPointParameterfv(GL_POINT_FADE_THRESHOLD_SIZE, &_scalarArgs[2]));
  }
  if (_scalarArgs[3] != variable._scalarArgs[3]) {
    scheduler.Register(new CglPointParameterfv(GL_POINT_SPRITE_COORD_ORIGIN, &_scalarArgs[3]));
  }
  if (!std::equal(_distAtten, _distAtten + 3, variable._distAtten, floats_close)) {
    scheduler.Register(new CglPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, _distAtten));
  }
}

/****************************** P A T C H   P A R A M E T E R **************************/

bool gits::OpenGL::CVariablePatchParameter::_supported = false;

gits::OpenGL::CVariablePatchParameter::CVariablePatchParameter() : _vertices(3) {
  _defaultInnerLevel[0] = 1.0f;
  _defaultInnerLevel[1] = 1.0f;

  _defaultOuterLevel[0] = 1.0f;
  _defaultOuterLevel[1] = 1.0f;
  _defaultOuterLevel[2] = 1.0f;
  _defaultOuterLevel[3] = 1.0f;

  if (curctx::IsOgl() && curctx::Version() >= 400) {
    _supported = true;
  }
}

void gits::OpenGL::CVariablePatchParameter::Get() {
  if (!_supported) {
    return;
  }
  drv.gl.glGetFloatv(GL_PATCH_DEFAULT_INNER_LEVEL, &_defaultInnerLevel[0]);
  drv.gl.glGetFloatv(GL_PATCH_DEFAULT_OUTER_LEVEL, &_defaultOuterLevel[0]);
  drv.gl.glGetIntegerv(GL_PATCH_VERTICES, &_vertices);
}

void gits::OpenGL::CVariablePatchParameter::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }
  const CVariablePatchParameter& variable = static_cast<const CVariablePatchParameter&>(lastValue);
  if (_defaultInnerLevel[0] != variable._defaultInnerLevel[0] ||
      _defaultInnerLevel[1] != variable._defaultInnerLevel[1]) {
    scheduler.Register(new CglPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, _defaultInnerLevel));
  }

  if (_defaultOuterLevel[0] != variable._defaultOuterLevel[0] ||
      _defaultOuterLevel[1] != variable._defaultOuterLevel[1] ||
      _defaultOuterLevel[2] != variable._defaultOuterLevel[2] ||
      _defaultOuterLevel[3] != variable._defaultOuterLevel[3]) {
    scheduler.Register(new CglPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, _defaultOuterLevel));
  }

  if (_vertices != variable._vertices) {
    scheduler.Register(new CglPatchParameteri(GL_PATCH_VERTICES, _vertices));
  }
}

/****************************** S A M P L E R  O B J E C T S **************************/

gits::OpenGL::CVariableSamplerInfo::CVariableSamplerInfo() {}

void gits::OpenGL::CVariableSamplerInfo::Get() {
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();
  if (stateDynamic.Samplers().List().begin() == stateDynamic.Samplers().List().end()) {
    return;
  }

  // Get Sampler Objects state
  for (auto& samplerState : stateDynamic.Samplers().List()) {
    auto* nonConstSamplerState = GCC433WA_0(&samplerState);
    drv.gl.glGetSamplerParameterfv(nonConstSamplerState->Name(), GL_TEXTURE_BORDER_COLOR,
                                   &nonConstSamplerState->Data().restore._borderColor[0]);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_MIN_FILTER,
                                   &nonConstSamplerState->Data().restore._minificationFunction);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_MAG_FILTER,
                                   &nonConstSamplerState->Data().restore._magnificationFunction);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_WRAP_S,
                                   &nonConstSamplerState->Data().restore._texcoordSWrapMode);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_WRAP_T,
                                   &nonConstSamplerState->Data().restore._texcoordTWrapMode);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_WRAP_R,
                                   &nonConstSamplerState->Data().restore._texcoordRWrapMode);
    drv.gl.glGetSamplerParameterfv(nonConstSamplerState->Name(), GL_TEXTURE_MIN_LOD,
                                   &nonConstSamplerState->Data().restore._minimumLevelOfDetail);
    drv.gl.glGetSamplerParameterfv(nonConstSamplerState->Name(), GL_TEXTURE_MAX_LOD,
                                   &nonConstSamplerState->Data().restore._maximumLevelOfDetail);
    drv.gl.glGetSamplerParameterfv(nonConstSamplerState->Name(), GL_TEXTURE_LOD_BIAS,
                                   &nonConstSamplerState->Data().restore._textureLevelOfDetail);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_COMPARE_MODE,
                                   &nonConstSamplerState->Data().restore._textureCompareMode);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_COMPARE_FUNC,
                                   &nonConstSamplerState->Data().restore._textureCompareFunc);
    drv.gl.glGetSamplerParameterfv(
        nonConstSamplerState->Name(), GL_TEXTURE_MAX_ANISOTROPY_EXT,
        &nonConstSamplerState->Data().restore._textureMaxLevelOfAnisotropyEXT);
    drv.gl.glGetSamplerParameteriv(nonConstSamplerState->Name(), GL_TEXTURE_SRGB_DECODE_EXT,
                                   &nonConstSamplerState->Data().restore._textureSrgbDecode);
  }
}

void gits::OpenGL::CVariableSamplerInfo::Schedule(CScheduler& scheduler,
                                                  const CVariable& lastValue) const {
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();
  if (stateDynamic.Samplers().List().begin() == stateDynamic.Samplers().List().end()) {
    return;
  }

  // Search for Sampler Objects to create
  for (auto& sampler : stateDynamic.Samplers().List()) {
    CreateSampler(scheduler, sampler.Data());
  }
}

void gits::OpenGL::CVariableSamplerInfo::CreateSampler(CScheduler& scheduler,
                                                       const CSamplerStateData& samplerToCreate) {
  GLuint tmpSamplerName = samplerToCreate.track.name;
  scheduler.Register(new CglGenSamplers(1, &tmpSamplerName));

  scheduler.Register(new CglSamplerParameterfv(samplerToCreate.track.name, GL_TEXTURE_BORDER_COLOR,
                                               &samplerToCreate.restore._borderColor[0]));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_MIN_FILTER,
                                              samplerToCreate.restore._minificationFunction));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_MAG_FILTER,
                                              samplerToCreate.restore._magnificationFunction));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_WRAP_S,
                                              samplerToCreate.restore._texcoordSWrapMode));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_WRAP_T,
                                              samplerToCreate.restore._texcoordTWrapMode));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_WRAP_R,
                                              samplerToCreate.restore._texcoordRWrapMode));
  scheduler.Register(new CglSamplerParameterf(samplerToCreate.track.name, GL_TEXTURE_MIN_LOD,
                                              samplerToCreate.restore._minimumLevelOfDetail));
  scheduler.Register(new CglSamplerParameterf(samplerToCreate.track.name, GL_TEXTURE_MAX_LOD,
                                              samplerToCreate.restore._maximumLevelOfDetail));
  scheduler.Register(new CglSamplerParameterf(samplerToCreate.track.name, GL_TEXTURE_LOD_BIAS,
                                              samplerToCreate.restore._textureLevelOfDetail));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_COMPARE_MODE,
                                              samplerToCreate.restore._textureCompareMode));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name, GL_TEXTURE_COMPARE_FUNC,
                                              samplerToCreate.restore._textureCompareFunc));
  scheduler.Register(
      new CglSamplerParameterf(samplerToCreate.track.name, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                               samplerToCreate.restore._textureMaxLevelOfAnisotropyEXT));
  scheduler.Register(new CglSamplerParameteri(samplerToCreate.track.name,
                                              GL_TEXTURE_SRGB_DECODE_EXT,
                                              samplerToCreate.restore._textureSrgbDecode));
}

/****************************** S A M P L E R  O B J E C T S  B I N D I N G S **************************/

gits::OpenGL::CVariableSamplerBinding::CVariableSamplerBinding() {}

void gits::OpenGL::CVariableSamplerBinding::Get() {
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();
  if (stateDynamic.Samplers().List().begin() == stateDynamic.Samplers().List().end()) {
    return;
  }

  // Get current Sampler bindings
  GLint maxTexUnits;
  GLint activeTexUnit;
  GLint boundSampler;

  drv.gl.glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexUnit);
  drv.gl.glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
  for (int i = 0; i < maxTexUnits; i++) {
    drv.gl.glActiveTexture(GL_TEXTURE0 + i);
    drv.gl.glGetIntegerv(GL_SAMPLER_BINDING, &boundSampler);
    _boundSamplersMap[i] = boundSampler;
  }
  drv.gl.glActiveTexture(activeTexUnit);
}

void gits::OpenGL::CVariableSamplerBinding::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  CSharedStateDynamic& stateDynamic = SD().GetCurrentSharedStateData();
  if (stateDynamic.Samplers().List().begin() == stateDynamic.Samplers().List().end()) {
    return;
  }

  // Bind current Sampler objects
  for (const auto& elem : _boundSamplersMap) {
    if (elem.second != 0) {
      scheduler.Register(new CglBindSampler(elem.first, elem.second));
    }
  }
}

/****************************** MAPPED TEXTURES **************************/

gits::OpenGL::CVariableMappedTexture::CVariableMappedTexture() {}

void gits::OpenGL::CVariableMappedTexture::Get() {}

void gits::OpenGL::CVariableMappedTexture::Schedule(CScheduler& scheduler,
                                                    const CVariable& lastValue) const {
  // Registering new mapped textures
  Log(WARN)
      << "Current implementation of state restore for Mapped Textures Intel extension works in "
         "such way that "
         "it always uses glTexParameteri(Target(), GL_TEXTURE_MEMORY_LAYOUT_INTEL, "
         "GL_LAYOUT_LINEAR_INTEL during "
         "texture image specification. This is due to error in driver (glMapTexture2DINTEL does "
         "not returns proper layout)."
         "This may cause error or corruptions if texture was created with different parameter.";

  auto toAdd = SD().GetCurrentSharedStateData().GetMappedTextures().ReturnNewTextures();
  for (auto& addTex : toAdd) {
    scheduler.Register(new CglMapTexture2DINTEL(
        (void*)true, addTex.texture, addTex.level, addTex.access,
        reinterpret_cast<GLint*>(addTex.stride), reinterpret_cast<GLenum*>(addTex.layout)));
    SD().GetCurrentSharedStateData().GetMappedTextures().InitializeTexture(addTex.texture,
                                                                           addTex.level);
  }

  // Updating all textures
  auto toUpdate = SD().GetCurrentSharedStateData().GetMappedTextures().SaveAllTexturesToFile();
  for (auto& updTex : toUpdate) {
    scheduler.Register(
        new CUpdateMappedTexture(updTex.texture, updTex.level, updTex.hash, updTex.size));
  }
}

/**************************************** B L E N D   E Q U A T I O NI **************************************/

bool gits::OpenGL::CVariableBlendEquationi::_supported = false;

gits::OpenGL::CVariableBlendEquationi::CVariableBlendEquationi() {
  if (curctx::IsOgl() && curctx::Version() >= 400) {
    _supported = true;
  }
}

void gits::OpenGL::CVariableBlendEquationi::Get() {}

void gits::OpenGL::CVariableBlendEquationi::Schedule(CScheduler& scheduler,
                                                     const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }
  auto& blendEquations =
      SD().GetCurrentContextStateData().BlendEquationi().BlendEquationiNameMapInfo();
  for (const auto& equationi : blendEquations) {
    if (equationi.second.separate == true) {
      scheduler.Register(new CglBlendEquationSeparatei(equationi.first, equationi.second.modeRGB,
                                                       equationi.second.modeAlpha));
    } else {
      scheduler.Register(new CglBlendEquationi(equationi.first, equationi.second.mode));
    }
  }
}

/* ***************************** B L E N D   F U N CI *************************** */

bool gits::OpenGL::CVariableBlendFunci::_supported = false;

gits::OpenGL::CVariableBlendFunci::CVariableBlendFunci() {
  if (curctx::IsOgl() && curctx::Version() >= 400) {
    _supported = true;
  }
}

void gits::OpenGL::CVariableBlendFunci::Get() {}

void gits::OpenGL::CVariableBlendFunci::Schedule(CScheduler& scheduler,
                                                 const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }
  auto blendFuncs = SD().GetCurrentContextStateData().BlendFunci().BlendFunciNameMapInfo();
  for (const auto& funcsi : blendFuncs) {
    if (funcsi.second.separate == true) {
      scheduler.Register(new CglBlendFuncSeparatei(funcsi.first, funcsi.second.srcRGB,
                                                   funcsi.second.dstRGB, funcsi.second.srcAlpha,
                                                   funcsi.second.dstAlpha));
    } else {
      scheduler.Register(new CglBlendFunci(funcsi.first, funcsi.second.src, funcsi.second.dst));
    }
  }
}

/* ***************************** E X T    D R A W    B U F F E R S    I N D E X E D ***************************/

bool gits::OpenGL::CVariableDrawBuffersIndexedEXTInfo::_supported = false;

GLint gits::OpenGL::CVariableDrawBuffersIndexedEXTInfo::_maxDrawBuffers = 0;

gits::OpenGL::CVariableDrawBuffersIndexedEXTInfo::TDrawBuffersIndexedData::TDrawBuffersIndexedData()
    : _enabled(GL_FALSE),
      _blendSrcRgb(GL_ONE),
      _blendSrcAlpha(GL_ONE),
      _blendDstRgb(GL_ZERO),
      _blendDstAlpha(GL_ZERO),
      _blendEquationRgb(GL_FUNC_ADD),
      _blendEquationAlpha(GL_FUNC_ADD),
      _colorWriteMask(std::array<GLboolean, 4U>{{GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE}}) {}

gits::OpenGL::CVariableDrawBuffersIndexedEXTInfo::CVariableDrawBuffersIndexedEXTInfo() {
  if (drv.gl.HasExtension("GL_EXT_draw_buffers_indexed")) {
    _supported = true;
  }

  if (_maxDrawBuffers == 0) {
    drv.gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &_maxDrawBuffers);
  }
  _drawBuffers.resize(_maxDrawBuffers);
}

void gits::OpenGL::CVariableDrawBuffersIndexedEXTInfo::Get() {
  if (!_supported) {
    return;
  }
  for (GLint i = 0; i < _maxDrawBuffers; ++i) {
    _drawBuffers[i]._enabled = drv.gl.glIsEnablediEXT(GL_BLEND, i);
    drv.gl.glGetIntegeri_v(GL_BLEND_SRC_RGB, i, &_drawBuffers[i]._blendSrcRgb);
    drv.gl.glGetIntegeri_v(GL_BLEND_SRC_ALPHA, i, &_drawBuffers[i]._blendSrcAlpha);
    drv.gl.glGetIntegeri_v(GL_BLEND_DST_RGB, i, &_drawBuffers[i]._blendDstRgb);
    drv.gl.glGetIntegeri_v(GL_BLEND_DST_ALPHA, i, &_drawBuffers[i]._blendDstAlpha);
    drv.gl.glGetIntegeri_v(GL_BLEND_EQUATION_RGB, i, &_drawBuffers[i]._blendEquationRgb);
    drv.gl.glGetIntegeri_v(GL_BLEND_EQUATION_ALPHA, i, &_drawBuffers[i]._blendEquationAlpha);
    drv.gl.glGetBooleani_v(GL_COLOR_WRITEMASK, i, &_drawBuffers[i]._colorWriteMask[0]);
  }
}

void gits::OpenGL::CVariableDrawBuffersIndexedEXTInfo::Schedule(CScheduler& scheduler,
                                                                const CVariable& lastValue) const {
  if (!_supported) {
    return;
  }
  const CVariableDrawBuffersIndexedEXTInfo& variable =
      static_cast<const CVariableDrawBuffersIndexedEXTInfo&>(lastValue);

  for (GLint i = 0; i < _maxDrawBuffers; ++i) {
    if (_drawBuffers[i]._enabled != variable._drawBuffers[i]._enabled ||
        _drawBuffers[i]._blendSrcRgb != variable._drawBuffers[i]._blendSrcRgb ||
        _drawBuffers[i]._blendSrcAlpha != variable._drawBuffers[i]._blendSrcAlpha ||
        _drawBuffers[i]._blendDstRgb != variable._drawBuffers[i]._blendDstRgb ||
        _drawBuffers[i]._blendDstAlpha != variable._drawBuffers[i]._blendDstAlpha ||
        _drawBuffers[i]._blendEquationRgb != variable._drawBuffers[i]._blendEquationRgb ||
        _drawBuffers[i]._blendEquationAlpha != variable._drawBuffers[i]._blendEquationAlpha ||
        _drawBuffers[i]._colorWriteMask[0] != variable._drawBuffers[i]._colorWriteMask[0] ||
        _drawBuffers[i]._colorWriteMask[1] != variable._drawBuffers[i]._colorWriteMask[1] ||
        _drawBuffers[i]._colorWriteMask[2] != variable._drawBuffers[i]._colorWriteMask[2] ||
        _drawBuffers[i]._colorWriteMask[3] != variable._drawBuffers[i]._colorWriteMask[3]) {
      if (_drawBuffers[i]._enabled) {
        scheduler.Register(new CglEnableiEXT(GL_BLEND, i));
      } else {
        scheduler.Register(new CglDisableiEXT(GL_BLEND, i));
      }

      scheduler.Register(new CglBlendEquationSeparateiEXT(i, _drawBuffers[i]._blendEquationRgb,
                                                          _drawBuffers[i]._blendEquationAlpha));
      scheduler.Register(new CglBlendFuncSeparateiEXT(
          i, _drawBuffers[i]._blendSrcRgb, _drawBuffers[i]._blendDstRgb,
          _drawBuffers[i]._blendSrcAlpha, _drawBuffers[i]._blendDstAlpha));
      scheduler.Register(new CglColorMaskiEXT(
          i, _drawBuffers[i]._colorWriteMask[0], _drawBuffers[i]._colorWriteMask[1],
          _drawBuffers[i]._colorWriteMask[2], _drawBuffers[i]._colorWriteMask[3]));
    }
  }
}
