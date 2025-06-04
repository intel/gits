// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglState.h
 *
 * @brief Declaration of OpenGL library state wrapper.
 *
 */

#pragma once

#include "state.h"
#include "platform.h"
#include "tools.h"
#include "exception.h"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <array>

namespace gits {
namespace OpenGL {
/**
     * @brief OpenGL library current context share group getter class
     *
     * gits::OpenGL::CSharedState class is responsible for handling OpenGL library
     * state per share group.
     */
class CSharedState : public gits::CComponentState {
public:
  CSharedState();
};

/**
     * @brief OpenGL library current context non sharable state getter class
     *
     * gits::OpenGL::CContextState class is responsible for handling OpenGL library
     * of non shared objects state per context.
     */
class CContextState : public gits::CComponentState {
public:
  CContextState();
};

/**
     * @brief Library state getter class
     *
     * gits::OpenGL::CState class is responsible for obtaining
     * current state component on each context (TODO)
     */
class CState : public gits::CState {
  void* _originalContext;

  void ScheduleCurrentContextEGL(CScheduler& scheduler, void* context) const;
  void ScheduleCurrentContext(CScheduler& scheduler, void* context) const;

public:
  CState();
  ~CState();
  CState(const CState& other) = delete;
  CState& operator=(const CState& other) = delete;

  void Get();
  void Schedule(CScheduler& scheduler) const;
  void Finish(CScheduler& scheduler) const {}
};

/**
     * @brief OpenGL library matrix getter class
     *
     * Class is responsible for getting actual state of OpenGL martices
     * state.
     */
class CVariableMatrices : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL capability variable getter class
     *
     * Class is responsible for getting actual state of OpenGL capabilities.
     * It will produce glEnable() and glDisable() OpenGL calls if necessary.
     */
class CVariableCapability : public gits::CComponentState::CVariable {
  const GLenum _capability;
  bool _value;

public:
  CVariableCapability(GLenum capability, bool defaultValue = false);
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL capability variable getter class
     *
     * Class is responsible for getting actual client state of OpenGL capabilities.
     * It will produce glEnableClientState() and glDisableClientState() OpenGL calls if necessary.
     */
class CVariableClientCapability : public gits::CComponentState::CVariable {
  const GLenum _capability;
  bool _value;

public:
  CVariableClientCapability(GLenum capability, bool defaultValue = false);
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL capability variable getter class
     *
     * Class is responsible for getting actual client state of OpenGL capabilities.
     * It will produce glEnableClientState() and glDisableClientState() OpenGL calls if necessary.
     */
class CVariableTextureUnitCapabilities : public gits::CComponentState::CVariable {
  bool _supported;

  struct TDataInfo {
    GLenum pname;
    GLint defValue;
  };

  GLenum _textureUnit;
  GLenum _originalUnit;

  static const unsigned _targetNum = 4;
  static const unsigned _texGenNum = 4;

  static const GLenum _targets[_targetNum];
  static const GLenum _texGenCoords[_texGenNum];
  static const GLenum _texGenTargets[_texGenNum];

  GLint _texGenCoordsEnabled[_texGenNum];

  struct TTexGenData {
    GLint mode;
    GLint objectPlane[4];
    GLint eyePlane[4];
  };
  TTexGenData _texGenInfo[_texGenNum];

  GLboolean _texturing[_targetNum]; // kind of texturing enabled for this unit
  GLboolean _textureCoordArray;

public:
  CVariableTextureUnitCapabilities(GLenum textureUnit);
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glActiveTexture variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glActiveTexture
     * variables. glActiveTexture() function will be produced if necessary.
     */
class CVariableActiveTexture : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glAlphaFunc variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glAlphaFunc
     * variables. glAlphaFunc() function will be produced if necessary.
     */
class CVariableAlphaFunc : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glBlendFunc variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glBlendFunc
     * variables. glBlendFunc() function will be produced if necessary.
     */
class CVariableBlendFunc : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glClearAccum variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glClearAccum
     * variables. glClearAccum() function will be produced if necessary.
     */
class CVariableClearAccum : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glClearColor variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glClearColor
     * variables. glClearColor() function will be produced if necessary.
     */
class CVariableClearColor : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glClearDepth variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glClearDepth
     * variables. glClearDepth() function will be produced if necessary.
     */
class CVariableClearDepth : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glClearStencil variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glClearStencil
     * variables. glClearStencil() function will be produced if necessary.
     */
class CVariableClearStencil : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glClipPlane variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glClipPlane
     * variables. glClipPlane() function will be produced if necessary.
     */
class CVariableClipPlane : public gits::CComponentState::CVariable {
  static const unsigned short COORD_NUM = 4;
  const GLenum _plane;
  GLdouble _equation[COORD_NUM];

public:
  CVariableClipPlane(GLenum plane);
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glClipPlanef variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glClipPlanef
     * variables. glClipPlanef() function will be produced if necessary.
     */
class CVariableClipPlanef : public gits::CComponentState::CVariable {
  static const unsigned short COORD_NUM = 4;
  const GLenum _plane;
  GLfloat _equation[COORD_NUM];

public:
  CVariableClipPlanef(GLenum plane);
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glColor variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glColor
     * variables. glColor() function will be produced if necessary.
     */
class CVariableColor : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glColorMask variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glColorMask
     * variables. glColorMask() function will be produced if necessary.
     */
class CVariableColorMask : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glColorMaterial variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glColorMaterial
     * variables. glColorMaterial() function will be produced if necessary.
     */
class CVariableColorMaterial : public gits::CComponentState::CVariable {
  GLint _face;
  GLint _mode;

public:
  CVariableColorMaterial();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glColorPointer variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glColorPointer
     * variables. glColorPointer() function will be produced if necessary.
     */
class CVariableColorPointer : public gits::CComponentState::CVariable {
  GLint _size;
  GLint _type;
  GLsizei _stride;
  GLvoid* _pointer;
  GLint _boundBuffer;

public:
  CVariableColorPointer();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glSecondaryColorPointer variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glSecondaryColorPointer
     * variables. glSecondaryColorPointer() function will be produced if necessary.
     */
class CVariableSecondaryColorPointer : public gits::CComponentState::CVariable {
  GLint _size;
  GLint _type;
  GLsizei _stride;
  GLvoid* _pointer;
  GLint _boundBuffer;

public:
  CVariableSecondaryColorPointer();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glNormalPointer variable getter class
    *
    * Class is responsible for getting actual state of OpenGL glNormalPointer
    * variables. glNormalPointer() function will be produced if necessary.
    */
class CVariableNormalPointer : public gits::CComponentState::CVariable {
  GLint _type;
  GLsizei _stride;
  GLvoid* _pointer;
  GLint _boundBuffer;

public:
  CVariableNormalPointer();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glVertexPointer variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glVertexPointer
     * variables. glVertexPointer() function will be produced if necessary.
     */
class CVariableVertexPointer : public gits::CComponentState::CVariable {
  GLint _size;
  GLint _type;
  GLsizei _stride;
  GLvoid* _pointer;
  GLint _boundBuffer;

public:
  CVariableVertexPointer();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glMaterial variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glMaterial
     * variables. glMaterial() function will be produced if necessary.
     */
class CVariableMaterial : public gits::CComponentState::CVariable {
  static const unsigned _facesNum = 2;
  static const GLenum _faces[_facesNum];

  struct TMaterialInfo {
    TMaterialInfo();

    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat emission[4];
    GLfloat shininess;
    GLfloat indexes[3];
  };
  TMaterialInfo _data[_facesNum];

public:
  CVariableMaterial();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glCullFace variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glCullFace
     * variables. glCullFace() function will be produced if necessary.
     */
class CVariableCullFace : public gits::CComponentState::CVariable {
  GLint _mode;

public:
  CVariableCullFace();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glDepthFunc variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glDepthFunc
     * variables. glDepthFunc() function will be produced if necessary.
     */
class CVariableDepthFunc : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glDepthMask variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glDepthMask
     * variables. glDepthMask() function will be produced if necessary.
     */
class CVariableDepthMask : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glDepthRange variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glDepthRange
     * variables. glDepthRange() function will be produced if necessary.
     */
class CVariableDepthRange : public gits::CComponentState::CVariable {
  static const unsigned short VALUE_NUM = 2;
  GLdouble _value[VALUE_NUM];

public:
  CVariableDepthRange();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glDepthRangef variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glDepthRangef
     * variables. glDepthRangef() function will be produced if necessary.
     */
class CVariableDepthRangef : public gits::CComponentState::CVariable {
  static const unsigned short VALUE_NUM = 2;
  GLfloat _value[VALUE_NUM];

public:
  CVariableDepthRangef();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glDrawBuffers variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glDrawBuffers
     * variables. glDrawBuffers() function will be produced if necessary.
     */
class CVariableDrawBuffers : public gits::CComponentState::CVariable {
public:
  CVariableDrawBuffers();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glFog variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glFog
     * variables. glFog() function will be produced if necessary.
     */
class CVariableFog : public gits::CComponentState::CVariable {
  struct TDataInfo {
    GLenum name;
    unsigned num;
  };
  static const TDataInfo _dataInfo[];
  static const unsigned _dataNum;

  GLfloat** _data;

public:
  CVariableFog();
  CVariableFog(const CVariableFog& other) = delete;
  CVariableFog& operator=(const CVariableFog& other) = delete;
  ~CVariableFog();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glFrontFace variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glFrontFace
     * variables. glFrontFace() function will be produced if necessary.
     */
class CVariableFrontFace : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glBlendEquation variable getter class
    *
    * Class is responsible for getting actual state of OpenGL glBlendEquation
    * variables. glBlendEquation() function will be produced if necessary.
    */
class CVariableBlendEquation : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glFog variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glFog
     * variables. glFog() function will be produced if necessary.
     */
class CVariableHint : public gits::CComponentState::CVariable {
  static const unsigned HINT_ARRAY_SIZE = 7;
  struct TDataInfo {
    GLenum name;
    GLenum mode;
  };
  static const TDataInfo _dataInfo[];

  GLint* _data;

public:
  CVariableHint();
  ~CVariableHint();
  CVariableHint(const CVariableHint& other) = delete;
  CVariableHint& operator=(const CVariableHint& other) = delete;
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glLight variable getter class
    *
    * Class is responsible for getting actual state of OpenGL glLight
    * variables. glLightModel() function will be produced if necessary.
    */
class CVariableLightModel : public gits::CComponentState::CVariable {
  float _ambient[4];
  float _colorControl;
  float _localViewer;
  float _twoSide;

public:
  CVariableLightModel();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glLight variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glLight
     * variables. glLight() function will be produced if necessary.
     */
class CVariableLight : public gits::CComponentState::CVariable {
  struct TDataInfo {
    GLenum name;
    unsigned num;
  };
  static const TDataInfo _dataInfo[];
  static const unsigned _dataNum;

  const GLenum _light;
  GLfloat** _data;

public:
  CVariableLight(GLenum light);
  CVariableLight(const CVariableLight& other) = delete;
  CVariableLight& operator=(const CVariableLight& other) = delete;
  ~CVariableLight();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glLineStipple variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glLineStipple
     * variables. glLineStipple() function will be produced if necessary.
     */
class CVariableLineStipple : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glLineWidth variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glLineWidth
     * variables. glLineWidth() function will be produced if necessary.
     */
class CVariableLineWidth : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glLogicOp variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glLogicOp
     * variables. glLogicOp() function will be produced if necessary.
     */
class CVariableLogicOp : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glMatrixMode variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glMatrixMode
     * variables. glMatrixMode() function will be produced if necessary.
     */
class CVariableMatrixMode : public gits::CComponentState::CVariable {
  GLint _mode;

public:
  CVariableMatrixMode();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glNormal variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glNormal
     * variables. glNormal() function will be produced if necessary.
     */
class CVariableNormal : public gits::CComponentState::CVariable {
  GLfloat _nx;
  GLfloat _ny;
  GLfloat _nz;

public:
  CVariableNormal();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glPixelStore variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glPixelStore
     * variables. glPixelStore() function will be produced if necessary.
     */
class CVariablePixelStore : public gits::CComponentState::CVariable {
  static const unsigned STORAGE_PARAMS_NUM = 16;
  struct TDataInfo {
    GLenum name;
    GLint value;
  };
  static const TDataInfo _dataInfo[STORAGE_PARAMS_NUM];

  GLint _values[STORAGE_PARAMS_NUM];

public:
  CVariablePixelStore();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glPolygonMode variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glPolygonMode
     * variables. glPolygonMode() function will be produced if necessary.
     */
class CVariablePolygonMode : public gits::CComponentState::CVariable {
  GLenum _front;
  GLenum _back;

public:
  CVariablePolygonMode();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glPointSize variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glPointSize
     * variables. glPointSize() function will be produced if necessary.
     */
class CVariablePointSize : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glPolygonOffset variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glPolygonOffset
     * variables. glPolygonOffset() function will be produced if necessary.
     */
class CVariablePolygonOffset : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glPolygonStipple variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glPolygonStipple
     * variables. glPolygonStipple() function will be produced if necessary.
     */
class CVariablePolygonStipple : public gits::CComponentState::CVariable {
  static const unsigned MASK_SIZE = 32 * 4;
  GLubyte _mask[MASK_SIZE];

public:
  CVariablePolygonStipple();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glScissor variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glScissor
     * variables. glScissor() function will be produced if necessary.
     */
class CVariableScissor : public gits::CComponentState::CVariable {
  GLint _x;
  GLint _y;
  GLsizei _width;
  GLsizei _height;

protected:
  static const unsigned SCISSOR_PARAMS_ARRAY_SIZE = 4;

public:
  CVariableScissor();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glShadeModel variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glShadeModel
     * variables. glShadeModel() function will be produced if necessary.
     */
class CVariableShadeModel : public gits::CComponentState::CVariable {
public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glStencilFunc variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glStencilFunc
     * variables. glStencilFunc() function will be produced if necessary.
     */
class CVariableStencilFunc : public gits::CComponentState::CVariable {
  GLint _func;
  GLint _ref;
  GLint _mask;
  GLint _funcBack;
  GLint _refBack;
  GLint _maskBack;

public:
  CVariableStencilFunc();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glStencilOp variable getter class
    *
    * Class is responsible for getting actual state of OpenGL glStencilOp
    * variables. glStencilOp() function will be produced if necessary.
    */
class CVariableStencilOp : public gits::CComponentState::CVariable {
  GLint _sfail;
  GLint _dpfail;
  GLint _dppass;
  GLint _sfailBack;
  GLint _dpfailBack;
  GLint _dppassBack;

public:
  CVariableStencilOp();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glStencilMask variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glStencilMask
     * variables. glStencilMask() function will be produced if necessary.
     */
class CVariableStencilMask : public gits::CComponentState::CVariable {
  GLint _mask;

public:
  CVariableStencilMask();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glTexCoord variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glTexCoord
     * variables. glTexCoord() function will be produced if necessary.
     */
class CVariableTexCoord : public gits::CComponentState::CVariable {
  GLenum _originalUnit;
  GLint _maxTexCoords;

protected:
  static const unsigned TEXCOORD_PARAMS_ARRAY_SIZE = 4;

public:
  CVariableTexCoord();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glTexCoordPointer variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glTexCoordPointer
     * variables. glTexCoordPointer() function will be produced if necessary.
     */
class CVariableTexCoordInfo : public gits::CComponentState::CVariable {
  struct TTexCoordData {
    TTexCoordData();
    GLint _size;
    GLint _type;
    GLint _stride;
    GLint _boundBuffer;
    GLvoid* _pointer;
    bool operator!=(const TTexCoordData& rhs) const {
      return _size != rhs._size || _type != rhs._type || _stride != rhs._stride ||
             _pointer != rhs._pointer || _boundBuffer != rhs._boundBuffer;
    }
  };
  std::vector<TTexCoordData> _texCoords;

public:
  CVariableTexCoordInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glTexEnv variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glTexEnv
     * variables. glTexEnv() function will be produced if necessary.
     */
class CVariableTexEnv : public gits::CComponentState::CVariable {
  bool _supported;

  struct TDataInfo {
    GLenum target;
    GLenum pname;
    GLint defValue;
  };
  static const TDataInfo _scalarDataInfo[];
  static const unsigned _dataNum;

  GLint _texEnvColor[4]; // only vector like data
  GLint* _data;          // space for scalar data
  GLenum _textureUnit;   // this is data for this multitexturing unit
  GLenum _originalUnit;  // this is unit actually active active
public:
  CVariableTexEnv(GLenum textureUnit);
  CVariableTexEnv(const CVariableTexEnv& other) = delete;
  CVariableTexEnv& operator=(const CVariableTexEnv& other) = delete;
  ~CVariableTexEnv();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL glViewport variable getter class
     *
     * Class is responsible for getting actual state of OpenGL glViewport
     * variables. glViewport() function will be produced if necessary.
     */
class CVariableViewport : public gits::CComponentState::CVariable {
  GLint _x;
  GLint _y;
  GLsizei _width;
  GLsizei _height;

protected:
  static const unsigned VIEWPORT_PARAMS_ARRAY_SIZE = 4;

public:
  CVariableViewport();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL CVariablePrimitiveRestartIndex variable getter class
     *
     * Class is responsible for getting actual state of OpenGL PrimitiveRestartIndex
     */
class CVariablePrimitiveRestartIndex : public gits::CComponentState::CVariable {
  GLuint _index;

public:
  CVariablePrimitiveRestartIndex();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL vertex attrib variable getter class
     *
     * Class is responsible for getting actual state of OpenGL vertex attribs
     */
class CVariableVertexAttribInfo : public gits::CComponentState::CVariable {
  static bool _supported;

  static GLint _maxVertexAttribs;
  struct TVertexAttribData {
    TVertexAttribData();
    // TODO: Give them proper types where needed, like GLintptr, GLuint, GLsizei, etc.
    GLint _enabled;
    GLint _size;
    GLint _type;
    GLint _stride;
    GLint _normalized;
    GLint _bufferBinding;
    GLint _divisor;
    GLint _binding;
    GLint _relativeOffset;
    GLint _bindingOffset;
    GLint _bindingStride;
    GLint _bindingDivisor;
    GLvoid* _pointer;
  };
  std::vector<TVertexAttribData> _vertexAttribs;

  GLuint _vertexArrayObject;

public:
  CVariableVertexAttribInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL buffer data getter class
     *
     * Class is responsible for getting buffers data of OpenGL .
     */
class CVariableBufferInfo : public gits::CComponentState::CVariable {
  static bool _supported;

public:
  CVariableBufferInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL buffer bindings getter class
     *
     * Class is responsible for getting buffers bindings of OpenGL .
     */
class CVariableBufferBindings : public gits::CComponentState::CVariable {
public:
  CVariableBufferBindings();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL GLSL program bindings getter class
     *
     * Class is responsible for getting program bindings of OpenGL .
     */
class CVariableGLSLBindings : public gits::CComponentState::CVariable {
public:
  CVariableGLSLBindings();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL default framebuffer state getter class
     *
     * Class is responsible for getting default framebuffer state of OpenGL .
     */
class CVariableDefaultFramebuffer : public gits::CComponentState::CVariable {
  bool _supported;
  GLuint _width;
  GLuint _height;
  hash_t _csDatahash;
  hash_t _colorDatahash;
  GLenum _dsInternalformat;
  GLenum _dsFormat;
  GLenum _dsType;
  GLenum _dsAttachment;
  GLenum _colorInternalformat;
  GLenum _colorFormat;
  GLenum _colorType;

  // Private functions used in recognition of proper internal format for
  // textures
  bool FormatSetup();
  bool SaveFramebufferData(GLenum internalFormat, GLenum format, GLenum type, GLenum attachment);

public:
  CVariableDefaultFramebuffer();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL renderbuffer state getter class
     *
     * Class is responsible for getting renderbuffers state of OpenGL .
     */
class CVariableRenderbufferEXTInfo : public gits::CComponentState::CVariable {
  static bool _supported;

  static void createRenderbuffer(CScheduler& scheduler, const CRenderbufferStateData&);

public:
  CVariableRenderbufferEXTInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL Renderbuffer EXT binding
    *
    * Class is responsible for getting actual renderbuffer ext binding
    */
class CVariableRenderbufferEXTBinding : public gits::CComponentState::CVariable {
  static bool _supported;

  GLint _boundRenderbufferEXT;

public:
  CVariableRenderbufferEXTBinding();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL renderbuffer state getter class
     *
     * Class is responsible for getting renderbuffers state of OpenGL .
     */
class CVariableRenderbufferInfo : public gits::CComponentState::CVariable {
  static bool _supported;

  static void createRenderbuffer(CScheduler& scheduler, const CRenderbufferStateData&);

public:
  CVariableRenderbufferInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL Renderbuffer binding
    *
    * Class is responsible for getting actual renderbuffer binding
    */
class CVariableRenderbufferBinding : public gits::CComponentState::CVariable {
  static bool _supported;

  GLint _boundRenderbuffer;

public:
  CVariableRenderbufferBinding();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL frame buffer object data getter class
     *
     * Class is responsible for getting frame buffer objects data of OpenGL .
     */

class CVariableFramebufferInfoGITS : public gits::CComponentState::CVariable {
  static GLint _maxDrawBuffers;

  static void createFramebuffer(const CFramebufferStateData&);

  void getAttachmentInfo(GLenum targetType,
                         GLenum attachmentPoint,
                         GLenum& type,
                         GLuint& name,
                         GLenum& cubeFace,
                         GLint& layer3d,
                         GLint& level);

protected:
  static bool _supported;
  // pointer to scheduler used in the static worker functions (for lack of
  // closures)
  static CScheduler* _scheduler;
  static const CVariableFramebufferInfoGITS* _currinstance;

  // Common API calls
  virtual void glGetFramebufferAttachmentParameterivGITS(GLenum target,
                                                         GLenum attachment,
                                                         GLenum pname,
                                                         GLint* params) const = 0;
  virtual void glBindFramebufferGITS(GLenum target, GLuint framebuffer) const = 0;

  // Common API Schedule calls
  virtual void ScheduleGenFramebuffers(GLsizei n, GLuint framebuffers[]) const = 0;
  virtual void ScheduleBindFramebuffer(GLenum target, GLuint framebuffer) const = 0;
  virtual void ScheduleFramebufferRenderbuffer(GLenum target,
                                               GLenum attachment,
                                               GLenum renderbuffertarget,
                                               GLuint renderbuffer) const = 0;
  virtual void ScheduleFramebufferTexture1D(
      GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const = 0;
  virtual void ScheduleFramebufferTextureLayer(
      GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const = 0;
  virtual void ScheduleFramebufferTexture2D(
      GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const = 0;
  virtual void ScheduleFramebufferTexture3D(GLenum target,
                                            GLenum attachment,
                                            GLenum textarget,
                                            GLuint texture,
                                            GLint level,
                                            GLint zoffset) const = 0;
  virtual void ScheduleCheckFramebufferStatus(GLenum target) const = 0;

  // Helpers
  virtual const gits::OpenGL::CContextStateDynamic::CFramebuffers::CListType& GetFramebufferIdList()
      const = 0;
  virtual gits::OpenGL::CContextStateDynamic::CFramebuffers::CListType& GetFramebufferIdList() = 0;

public:
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL buffer data getter class for EXT version
     *
     * Class derived from CVariableFramebufferInfoGITS and specializes it for EXT version.
     */
class CVariableFramebufferEXTInfo : public gits::OpenGL::CVariableFramebufferInfoGITS {
  GLint _boundFramebuffer;

protected:
  // API calls
  virtual void glGetFramebufferAttachmentParameterivGITS(GLenum target,
                                                         GLenum attachment,
                                                         GLenum pname,
                                                         GLint* params) const;
  virtual void glBindFramebufferGITS(GLenum target, GLuint framebuffer) const;

  // API Schedule calls
  virtual void ScheduleGenFramebuffers(GLsizei n, GLuint framebuffers[]) const;
  virtual void ScheduleBindFramebuffer(GLenum target, GLuint framebuffer) const;
  virtual void ScheduleFramebufferRenderbuffer(GLenum target,
                                               GLenum attachment,
                                               GLenum renderbuffertarget,
                                               GLuint renderbuffer) const;
  virtual void ScheduleFramebufferTexture1D(
      GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const;
  virtual void ScheduleFramebufferTextureLayer(
      GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const;
  virtual void ScheduleFramebufferTexture2D(
      GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const;
  virtual void ScheduleFramebufferTexture3D(GLenum target,
                                            GLenum attachment,
                                            GLenum textarget,
                                            GLuint texture,
                                            GLint level,
                                            GLint zoffset) const;
  virtual void ScheduleCheckFramebufferStatus(GLenum target) const;

  // Helpers
  virtual const gits::OpenGL::CContextStateDynamic::CFramebuffersEXT::CListType&
  GetFramebufferIdList() const {
    return SD().GetCurrentContextStateData().FramebuffersEXT().List();
  }
  virtual gits::OpenGL::CContextStateDynamic::CFramebuffersEXT::CListType& GetFramebufferIdList() {
    return SD().GetCurrentContextStateData().FramebuffersEXT().List();
  }

public:
  CVariableFramebufferEXTInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL buffer data getter class for core version
     *
     * Class derived from CVariableFramebufferInfoGITS and specializes it for core version.
     */
class CVariableFramebufferInfo : public gits::OpenGL::CVariableFramebufferInfoGITS {
  typedef std::map<GLenum, GLuint> CBoundFramebuffers;
  CBoundFramebuffers _boundframebuffers;

protected:
  // API calls
  virtual void glGetFramebufferAttachmentParameterivGITS(GLenum target,
                                                         GLenum attachment,
                                                         GLenum pname,
                                                         GLint* params) const;
  virtual void glBindFramebufferGITS(GLenum target, GLuint framebuffer) const;

  // API Schedule calls
  virtual void ScheduleGenFramebuffers(GLsizei n, GLuint framebuffers[]) const;
  virtual void ScheduleBindFramebuffer(GLenum target, GLuint framebuffer) const;
  virtual void ScheduleFramebufferRenderbuffer(GLenum target,
                                               GLenum attachment,
                                               GLenum renderbuffertarget,
                                               GLuint renderbuffer) const;
  virtual void ScheduleFramebufferTexture1D(
      GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const;
  virtual void ScheduleFramebufferTextureLayer(
      GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const;
  virtual void ScheduleFramebufferTexture2D(
      GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) const;
  virtual void ScheduleFramebufferTexture3D(GLenum target,
                                            GLenum attachment,
                                            GLenum textarget,
                                            GLuint texture,
                                            GLint level,
                                            GLint zoffset) const;
  virtual void ScheduleCheckFramebufferStatus(GLenum target) const;

  // Helpers
  virtual const gits::OpenGL::CContextStateDynamic::CFramebuffers::CListType& GetFramebufferIdList()
      const {
    return SD().GetCurrentContextStateData().Framebuffers().List();
  }
  virtual gits::OpenGL::CContextStateDynamic::CFramebuffers::CListType& GetFramebufferIdList() {
    return SD().GetCurrentContextStateData().Framebuffers().List();
  }

public:
  CVariableFramebufferInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL texture parameters getter class
     *
     * Class is responsible for getting actual parameters of OpenGL
     * for a list of textures.
     */
class CVariableTextureInfo : public gits::CComponentState::CVariable {
  typedef CTextureStateData::TTextureGenericData TTextureGenericData;
  typedef CTextureStateData::TMipmapTextureData TMipmapTextureData;
  typedef CTextureStateData::CRestoredTextureData CTextureData;
  class CTexture {
    GLint _target;
    GLuint _id;

  protected:
    static unsigned MipmapCount(GLuint textureId);
    static unsigned TexelSize(GLenum format, GLenum type);

    void GetTextureGenericData(GLenum target);
    void ScheduleTextureGenericData(GLenum target,
                                    CScheduler& scheduler,
                                    const CTextureData& defaultTexture) const;
    // This member function gathers texture mipmap data.
    virtual void GetTextureLevelsDataGL() = 0;
    virtual void GetTextureLevelsDataGLES() = 0;
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const = 0;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const = 0;

    CTextureStateData& TextureStateData() const {
      auto& list = SD().GetCurrentSharedStateData().Textures().List();
      auto it = list.find(CTextureStateObj(_id, _target));

      if (it != list.end()) {
        return GCC433WA_0(it)->Data();
      } else {
        Log(ERR) << "Texture " << _id << " not found to be bound to target " << _target;
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    }

    // Checks if data is readable from current framebuffer using glReadPixels in
    // OpenGLES
    bool AreESPixelsReadable(GLenum format, GLenum type);

    CTextureStateData::CRestoredTextureData& TextureRestoredData() const {
      return *TextureStateData().restore.ptr;
    }
    CTextureStateData::TTextureGenericData& GenericData() const {
      return TextureRestoredData().genericData;
    }
    std::vector<CTextureStateData::TMipmapTextureData>& MipmapData() const {
      return TextureStateData().track.mipmapData;
    }

  public:
    CTexture(GLint target, GLint id);
    GLint Target() const;
    GLuint TextureId() const;

    void Get();
    void Schedule(CScheduler& scheduler, const CTextureData& defaultTexture) const;
    virtual ~CTexture() {}
  };

  class CTexture1D : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES() {
      Log(ERR) << "GL_TEXTURE_1D restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      Log(ERR) << "GL_TEXTURE_1D restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    CTextureStateData::CTextureNDData& Texture1DData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture1D(GLint target, GLint textureId);
    ~CTexture1D();
  };

  class CTexture1DArray : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void GetTextureLevelsDataGLES() {
      Log(ERR) << "GL_TEXTURE_1D_ARRAY restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      Log(ERR) << "GL_TEXTURE_1D_ARRAY restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    CTextureStateData::CTextureNDData& Texture1DArrayData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture1DArray(GLint target, GLint textureId);
    ~CTexture1DArray();
  };

  class CTexture2D : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES();
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const;
    CTextureStateData::CTextureNDData& Texture2DData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture2D(GLint target, GLint textureId);
    ~CTexture2D();
  };

  class CTextureExternal : public CTexture {
    virtual void GetTextureLevelsDataGL() {}
    virtual void GetTextureLevelsDataGLES() {}
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    CTextureStateData::CTextureNDData& TextureExternalData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      ScheduleSameTargetTextureGL(scheduler);
    }

  public:
    CTextureExternal(GLint target, GLint textureId);
    ~CTextureExternal();
  };

  class CTexture2DArray : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES() {
      Log(ERR) << "GL_TEXTURE_2D_ARRAY restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      Log(ERR) << "GL_TEXTURE_2D_ARRAY restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    CTextureStateData::CTextureNDData& Texture2DArrayData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture2DArray(GLint target, GLint textureId);
    ~CTexture2DArray();
  };

  class CTexture2DMultisample : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES() {
      GetTextureLevelsDataGL();
    }
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      ScheduleSameTargetTextureGL(scheduler);
    }
    CTextureStateData::CTextureNDData& Texture2DMultiSampleData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture2DMultisample(GLint target, GLint textureId);
  };

  class CTexture3D : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES() {
      Log(ERR) << "GL_TEXTURE_3D restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      Log(ERR) << "GL_TEXTURE_3D restoration for OGLES not implemented.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
    CTextureStateData::CTextureNDData& Texture3DData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture3D(GLint target, GLint textureId);
    ~CTexture3D();
  };

  class CTexture2DMultisampleArray : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES() {
      GetTextureLevelsDataGL();
    }
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      ScheduleSameTargetTextureGL(scheduler);
    }
    CTextureStateData::CTextureNDData& Texture2DMultisampleArrayData() const {
      return dynamic_cast<CTextureStateData::CTextureNDData&>(TextureRestoredData());
    }

  public:
    CTexture2DMultisampleArray(GLint target, GLint textureId);
  };

  class CTextureCube : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES();
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const;
    CTextureStateData::CTextureCubeData& TextureCubeData() const {
      return dynamic_cast<CTextureStateData::CTextureCubeData&>(TextureRestoredData());
    }

  public:
    CTextureCube(GLint target, GLint textureId);
    ~CTextureCube();
  };

  class CTextureBuffer : public CTexture {
    virtual void GetTextureLevelsDataGL();
    virtual void GetTextureLevelsDataGLES() {
      GetTextureLevelsDataGL();
    }
    virtual void ScheduleSameTargetTextureGL(CScheduler& scheduler) const;
    virtual void ScheduleSameTargetTextureGLES(CScheduler& scheduler) const {
      ScheduleSameTargetTextureGL(scheduler);
    }
    CTextureStateData::CTextureBufferData& TextureBufferData() const {
      return dynamic_cast<CTextureStateData::CTextureBufferData&>(TextureRestoredData());
    }

  public:
    CTextureBuffer(GLint target, GLint textureId);
  };

  typedef std::set<CTexture*> CTextureSet;
  CTextureSet _textures;

  bool IsNonZeroIdUnBound(GLuint id) const;

public:
  CVariableTextureInfo();
  CVariableTextureInfo(const CVariableTextureInfo& other) = delete;
  CVariableTextureInfo& operator=(const CVariableTextureInfo& other) = delete;
  ~CVariableTextureInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL currently bound textures getter class
     *
     * Class is responsible for getting a list of actual bound openGl textures.
     */
class CVariableTextureBinding : public gits::CComponentState::CVariable {
  static const unsigned BINDING_ARRAY_SIZE = 10;
  GLint _bindingArray[BINDING_ARRAY_SIZE];
  GLenum _texture;
  GLenum _oldTexture;

public:
  CVariableTextureBinding(GLenum texture);
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL textures currently bound to image units
    *
    * Class is responsible for getting a list of textures currently
    * bound to image units.
    */
class CVariableBindImageTextureInfo : public gits::CComponentState::CVariable {
  struct TImageTextureData {
    TImageTextureData();
    GLuint name;
    GLint level;
    GLboolean layered;
    GLint layer;
    GLenum access;
    GLenum format;
  };

  static bool _supported;
  std::vector<TImageTextureData> _imageTextures;

public:
  CVariableBindImageTextureInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL program parameters getter class
     *
     * Class is responsible for getting actual parameters of OpenGL
     * for a list of shader programs.
     */
class CVariableProgramInfo : public gits::CComponentState::CVariable {
  bool _supported;

  struct ProgramParameter {
    float p[4];
  };
  typedef std::map<GLuint, ProgramParameter> CProgramParamsMap;

  struct TProgramData {
    GLuint _id;
    GLenum _target;
    GLint _strLen;

    CProgramParamsMap _params;

    GLbyte* _string;
  };

  typedef std::map<GLint, const TProgramData*> CProgramMap;
  CProgramMap _programMap;
  typedef std::map<GLenum, CProgramParamsMap> CProgramEnvParamsMap;
  CProgramEnvParamsMap _envParams;

public:
  CVariableProgramInfo();
  CVariableProgramInfo(const CVariableProgramInfo& other) = delete;
  CVariableProgramInfo& operator=(const CVariableProgramInfo& other) = delete;
  ~CVariableProgramInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL currently bound shader programs getter class
     *
     * Class is responsible for getting a list of actual bound openGl shader programs.
     */
class CVariableProgramBinding : public gits::CComponentState::CVariable {
  bool _supported;

  static const unsigned BINDING_ARRAY_SIZE = 2;
  GLint _bindingArray[BINDING_ARRAY_SIZE];

public:
  CVariableProgramBinding();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

class CVariablePointParameter : public gits::CComponentState::CVariable {
  GLfloat _scalarArgs[4];
  GLfloat _distAtten[3];

public:
  CVariablePointParameter();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL current patch parameters getter class
    *
    * Class is responsible for getting values of currently set patch parameters
    */
class CVariablePatchParameter : public gits::CComponentState::CVariable {
  static bool _supported;
  GLfloat _defaultInnerLevel[2];
  GLfloat _defaultOuterLevel[4];
  GLint _vertices;

public:
  CVariablePatchParameter();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL currently bound GL2.0 shaders and programs getter class
    *
    * Class is responsible for getting a list of actual bound openGl shaders and programs.
    */
class CVariableGLSLInfo : public gits::CComponentState::CVariable {
  // loads Uniform variable to appropriate program container
  static void ObtainUniform(GLuint programId,
                            CGLSLProgramStateData& program,
                            const std::string& name,
                            GLint location,
                            GLint arraySize,
                            GLenum uniformType);

  // schedules all calls for shaders
  static void CreateShader(CScheduler& scheduler,
                           const CGLSLShaderStateData& shader,
                           GLuint shaderID);
  static void CreateProgram(CScheduler& scheduler, const CGLSLProgramStateData& progData);

  // return type and dimensionality of uniform variable (type is
  // GL_FLOAT/GL_INT) - for use with glGetUniform functions
  static std::pair<GLenum, GLuint> UniformDimensions(GLenum type);

public:
  CVariableGLSLInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL currently bound GL2.0 shaders and programs getter class
    *
    * Class is responsible for getting a list of actual bound openGl shaders and programs.
    */
class CVariableGLSLPipelinesInfo : public gits::CComponentState::CVariable {
public:
  CVariableGLSLPipelinesInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL Vertex of Arrays Objects getter class
    *
    * Class is responsible for getting a list of actual generated/bound VAOs and their attributes.
    */
class CVariableVAOInfo : public gits::CComponentState::CVariable {
  static void CreateVAO(CScheduler&, const CVertexArraysStateData&);

public:
  CVariableVAOInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glMap1 variable getter class
    *
    * Class is responsible for getting actual state of OpenGL glMap1
    * variables. glMap1d() function will be produced if necessary.
    */
class CVariableMap1 : public gits::CComponentState::CVariable {
  struct TDataInfo {
    GLenum target;
    GLint stride;
  };
  static const TDataInfo _dataInfo[];
  static const unsigned _dataNum;

  struct TData {
    GLdouble u1;
    GLdouble u2;
    GLint order;
    GLdouble* points;
  };
  TData* _data;

public:
  CVariableMap1();
  CVariableMap1(const CVariableMap1& other) = delete;
  CVariableMap1& operator=(const CVariableMap1& other) = delete;
  ~CVariableMap1();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glMap2 variable getter class
    *
    * Class is responsible for getting actual state of OpenGL glMap2
    * variables. glMap2d() function will be produced if necessary.
    */
class CVariableMap2 : public gits::CComponentState::CVariable {
  struct TDataInfo {
    GLenum target;
    GLint ustride;
  };
  static const TDataInfo _dataInfo[];
  static const unsigned _dataNum;

  struct TData {
    GLdouble u1;
    GLdouble u2;
    GLdouble v1;
    GLdouble v2;
    GLint uorder;
    GLint vorder;
    GLdouble* points;
  };
  TData* _data;

public:
  CVariableMap2();
  ~CVariableMap2();
  CVariableMap2(const CVariableMap2& other) = delete;
  CVariableMap2& operator=(const CVariableMap2& other) = delete;
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL ARB Sampler objects getter class
    *
    * Class is responsible for getting actual state of OpenGL ARB samplers
    */
class CVariableSamplerInfo : public gits::CComponentState::CVariable {
  static void CreateSampler(CScheduler&, const CSamplerStateData&);

public:
  CVariableSamplerInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL Sampler binding
    *
    * Class is responsible for getting actual Sampler binding
    */
class CVariableSamplerBinding : public gits::CComponentState::CVariable {
  typedef std::map<GLint, GLint> CSamplerBindingMap;
  CSamplerBindingMap _boundSamplersMap;

public:
  CVariableSamplerBinding();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

class CVariableMappedTexture : public gits::CComponentState::CVariable {
public:
  CVariableMappedTexture();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glBlendEquationi variable getter class
    *
    */

class CVariableBlendEquationi : public gits::CComponentState::CVariable {
  static bool _supported;

public:
  CVariableBlendEquationi();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
    * @brief OpenGL glBlendFunci variable getter class
    *
    */
class CVariableBlendFunci : public gits::CComponentState::CVariable {
  static bool _supported;

public:
  CVariableBlendFunci();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

/**
     * @brief OpenGL separate draw buffer blend and color mask variable getter class
     *
     * Class is responsible for getting actual state of OpenGL blend and color mask for each draw buffer separately
     */
class CVariableDrawBuffersIndexedEXTInfo : public gits::CComponentState::CVariable {
  static bool _supported;

  static GLint _maxDrawBuffers;
  struct TDrawBuffersIndexedData {
    TDrawBuffersIndexedData();
    GLint _enabled;
    GLint _blendSrcRgb;
    GLint _blendSrcAlpha;
    GLint _blendDstRgb;
    GLint _blendDstAlpha;
    GLint _blendEquationRgb;
    GLint _blendEquationAlpha;
    std::array<GLboolean, 4> _colorWriteMask;
  };
  std::vector<TDrawBuffersIndexedData> _drawBuffers;

public:
  CVariableDrawBuffersIndexedEXTInfo();
  virtual void Get();
  virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const;
};

} // namespace OpenGL
} // namespace gits
