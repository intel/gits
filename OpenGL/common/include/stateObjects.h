// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   stateObjects.h
 *
 * @brief Declaration of OpenGL library implementation.
 *
 */
#pragma once

#include "platform.h"
#include "version.h"
#include "tools.h"
#include "gits.h"

#include "openglCommon.h"
#include "openglTools.h"
#include "openglEnums.h"
#include "intervalSet.h"

#include <unordered_map>
#include <list>
#include <unordered_set>
#include <vector>

//
//  Following macro and associated template function
//  are used to workaround issue found in STL that ships with g++4.3.3 -
//  set iterators are made 'const_iterators' under the hood (probably
//  to prevent changes to objects that would affect their ordering,
//  this also prevents changes that do not affect ordering of set elements
//  As a workaround i provide GCC433WA_0 macro that takes a set iterator
//  dereferences it, casts away const from resulting lvalue and returns
//  its address (there may be potential issues here, as type of expression
//  change to 'pointer to T' but this looks like the easiest solution
//  On Windows this is a noop
//
//  Apparently, it was clarified in next C++ standard that above behavior
//  is intended one, so we are in fact illegal, the workaround will stay
//  untill all clieant are refactored to use std::map instead of std::set
//
#ifndef GCC433WA_0
template <class T>
T& castaway_const(const T& x) {
  return const_cast<T&>(x);
}
#define GCC433WA_0(iter) (&castaway_const(*iter))
#endif

namespace gits {
/**
   * @brief OpenGL library specific GITS namespace
   */

namespace OpenGL {
struct CTextureStateData {
  CTextureStateData();
  explicit CTextureStateData(GLuint name, GLenum target = 0)
      : track(name, target), restore(target) {}

  struct TTextureGenericData {
    GLint magFilter;
    GLint minFilter;
    GLint wrapS;
    GLint wrapT;
    GLint wrapR;
    GLint borderColor[4];
    GLint minLod;
    GLint maxLod;
    GLint baseLevel;
    GLint maxLevel;
    GLint textureMode;
    GLint compareMode;
    GLint compareFunc;
    GLint generateMipmap;
    GLfloat maxAnisotropy;
    GLint swizzleR;
    GLint swizzleG;
    GLint swizzleB;
    GLint swizzleA;
    GLint memoryLayout;
    GLint srgbDecode;
    GLint immutable;
    std::vector<int> textureCropRectOES;

    TTextureGenericData()
        : magFilter(GL_LINEAR),
          minFilter(GL_NEAREST_MIPMAP_LINEAR),
          wrapS(GL_REPEAT),
          wrapT(GL_REPEAT),
          wrapR(GL_REPEAT),
          minLod(-1000),
          maxLod(1000),
          baseLevel(0),
          maxLevel(1000),
          textureMode(GL_LUMINANCE),
          compareMode(GL_NONE),
          compareFunc(GL_LEQUAL),
          generateMipmap(GL_FALSE),
          maxAnisotropy(1),
          swizzleR(GL_RED),
          swizzleG(GL_GREEN),
          swizzleB(GL_BLUE),
          swizzleA(GL_ALPHA),
          memoryLayout(0),
          srgbDecode(GL_DECODE_EXT),
          immutable(GL_FALSE),
          textureCropRectOES(4, 0) {
      std::fill_n(borderColor, 4, 0);
    }
  };

  struct TMipmapTextureData {
    GLint width;
    GLint height;
    GLint depth;
    GLint internalFormat;
    GLint border;
    GLint alphaSize;
    GLint luminanceSize;
    GLint compressed;
    GLint compressedImageSize;
    GLint samples;
    GLint fixedsamplelocations;
    GLint format;
    GLenum type;

    TMipmapTextureData()
        : width(0),
          height(0),
          depth(0),
          internalFormat(0),
          border(0),
          alphaSize(0),
          luminanceSize(0),
          compressed(0),
          compressedImageSize(0),
          samples(0),
          fixedsamplelocations(0),
          format(0),
          type(0) {}
  };

  struct CRestoredTextureData {
    TTextureGenericData genericData;
    CRestoredTextureData() {}
    virtual ~CRestoredTextureData() {
    } // To make CTextureData polymorphic type so that dynamic_cast can be applied
  };

  struct CTextureNDData : public CRestoredTextureData {
    std::vector<hash_t> pixels;
  };

  struct CTextureCubeData : public CRestoredTextureData {
    std::vector<hash_t> pixels[6];
  };

  struct CTextureBufferData : public CRestoredTextureData {
    GLint buffer;
    GLint offset;
    GLint size;
    GLint internalformat;
    CTextureBufferData() : buffer(0), offset(0), size(0), internalformat(0) {}
  };

  struct Tracked {
    GLuint name;
    GLenum target;
    void* eglImage;
    GLenum texbuffer_internalformat;
    GLuint texbuffer_buffer;
    std::vector<TMipmapTextureData> mipmapData;
    Tracked(GLuint tname, GLenum ttarget = 0)
        : name(tname),
          target(ttarget),
          eglImage(0),
          texbuffer_internalformat(0),
          texbuffer_buffer(0) {}
  } track;

  struct Restored {
    std::shared_ptr<CRestoredTextureData> ptr;
    bool isESTexReadable;
    Restored(GLenum target) : isESTexReadable(false) {
      GetNewObjectForTarget(target);
    }

    void GetNewObjectForTarget(GLenum target);
  } restore;

  void SetTarget(GLenum target) {
    if (target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X || target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
        target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z || target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
        target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y || target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z) {
      target = GL_TEXTURE_CUBE_MAP;
    }
    if (track.target != target) {
      track.target = target;
      restore.GetNewObjectForTarget(target);
    }
  }
};
/**
    *
    * CTextureStateObj - texture state tracking class
    *
    */
class CTextureStateObj {
  CTextureStateData _data;
  CTextureStateObj() {} // Default constructor shouldn't be used
public:
  // default parameter is used for texture deletion (we don't know its target in
  // that context)
  CTextureStateObj(GLuint texture, GLenum target = 0);
  GLint Name() const {
    return _data.track.name;
  }
  GLenum Target() const {
    return _data.track.target;
  } // target to which texture is bound
  void SetTarget(GLenum target) {
    _data.SetTarget(target);
  }
  void SetTexLevelParams(GLint level,
                         GLint width,
                         GLint height,
                         GLint depth,
                         GLint internalformat,
                         GLint border,
                         GLint compressed,
                         GLint compressedimagesize,
                         GLint samples,
                         GLint fixedsamplelocations,
                         GLint format,
                         GLenum type);
  GLenum LastBindEnum() const {
    return GetBindingEnum(_data.track.target);
  }
  CTextureStateData& Data() {
    return _data;
  }
  bool operator<(const CTextureStateObj& cmp) const {
    // there is a number of textures with id 0 equal to number of different
    // targets
    if (_data.track.name == 0 && cmp._data.track.name == 0) {
      return _data.track.target < cmp._data.track.target;
    }
    return _data.track.name < cmp._data.track.name;
  }
};

/**
    *
    * CARBProgramStateObj - program state tracking class
    *
    */
class CARBProgramStateObj {
public:
  typedef std::set<GLuint> CParameterList;

private:
  GLuint _name;
  GLenum _target;
  CParameterList _localParameters;

public:
  CARBProgramStateObj(GLuint program, GLenum target = 0);
  GLuint Name() const {
    return _name;
  }
  GLenum Target() const {
    return _target;
  }
  GLenum LastBindEnum() const {
    return GL_PROGRAM_BINDING_ARB;
  }

  void LocalParameterUsed(unsigned prm) {
    _localParameters.insert(prm);
  }
  CParameterList& LocalParametersUsed() {
    return _localParameters;
  }
  const CParameterList& LocalParametersUsed() const {
    return _localParameters;
  }

  bool operator<(const CARBProgramStateObj& cmp) const {
    return _name < cmp._name;
  }
};

/**
    *
    * CARBProgramEnvParamsStateObj - program state tracking class
    *
    */
class CARBProgramEnvParamsStateObj {
  GLint _index;
  GLenum _target;

public:
  CARBProgramEnvParamsStateObj(GLenum target, GLint index) : _index(index), _target(target) {}
  GLuint Name() const {
    return _index;
  }
  GLenum Target() const {
    return _target;
  }
  GLenum LastBindEnum() const {
    return 0;
  } // Fake

  bool operator<(const CARBProgramEnvParamsStateObj& cmp) const {
    if (_target == cmp._target) {
      return _index < cmp._index;
    } else {
      return _target < cmp._target;
    }
  }
};

/**
    *
    * CGLSLShaderStateObj - program state tracking class
    *
    */

struct CGLSLShaderStateData {
  struct Tracked {
    GLuint name;
    GLenum type;
    bool compiled;
    std::string source;
    std::string shader_name;
    int uniqueSource; // unique number is assigned during source load
    bool markedToDelete;
    Tracked(GLuint tname, GLenum ttype)
        : name(tname), type(ttype), compiled(false), uniqueSource(0), markedToDelete(false) {}
  } track;

  struct Restored {
    GLuint uniqueName;
    // uniqueName by default is same as name. Different unique name is generated
    // during state restoration
    // for objects which needs to be restored with different name (for example
    // linked and modified subsequently)
    Restored(GLuint runiqueName) : uniqueName(runiqueName) {}
  } restore;

  CGLSLShaderStateData() : track(0, 0), restore(0) {}
  CGLSLShaderStateData(GLuint name, GLenum type) : track(name, type), restore(name) {}
  bool operator<(const CGLSLShaderStateData& cmp) const {
    return track.name < cmp.track.name;
  }
};

class CGLSLShaderStateObj {
private:
  // Shared ptr here allows to keep Data object as long as shader is deleted,
  // detached from all programs and there is no program linked with this shader
  std::shared_ptr<CGLSLShaderStateData> _data;

  CGLSLShaderStateObj() {} // Default constructor shouldn't be used

public:
  CGLSLShaderStateObj(GLuint name, GLenum target = 0)
      : _data(new CGLSLShaderStateData(name, target)) {
    GLSLUnique::UseName(name);
  }

  // Track data methods
  GLuint Name() const {
    return _data->track.name;
  }
  GLenum Target() const {
    return _data->track.type;
  }
  GLenum LastBindEnum() const {
    return 0;
  }
  std::string GetShaderName() const {
    return _data->track.shader_name;
  }
  bool IsAttached() const {
    return !_data.unique();
  }
  void MarkToDelete() {
    _data->track.markedToDelete = true;
  }
  bool MarkedToDelete() const {
    return _data->track.markedToDelete;
  }
  void Compile() {
    _data->track.compiled = true;
  }
  void Source(std::string& source);
  void SetShaderName(std::string& shader_name) {
    _data->track.shader_name = shader_name;
  }

  // General data methods
  CGLSLShaderStateData& Data() {
    return *_data;
  }
  const CGLSLShaderStateData& Data() const {
    return *_data;
  }
  const std::shared_ptr<CGLSLShaderStateData>& DataShared() const {
    return _data;
  } // Used to share data with Program objects

  bool operator<(const CGLSLShaderStateObj& cmp) const {
    return _data->track.name < cmp._data->track.name;
  }
};

/**
    *
    * CGLSLProgramStateObj - program state tracking class
    *
    */
struct CGLSLProgramStateData {
  struct Tracked {
    typedef std::unordered_map<GLuint, std::shared_ptr<CGLSLShaderStateData>> CAttachedShaders;
    typedef std::unordered_map<GLuint, CGLSLShaderStateData> CLinkedShaders;
    typedef std::vector<std::string> CTransformVaryings;
    GLuint name;
    bool linked;
    bool isSepShaderObj;
    GLenum _tfBufferMode;
    bool markedToDelete;
    // Contains pointers to shader data objects.
    CAttachedShaders attachedShaders;
    // Contains copies of attached shaders data from linkage time.
    CLinkedShaders linkedShaders;
    // Indices of program active attributes.
    std::unordered_set<GLint> attribs;
    std::unordered_map<GLuint, std::string> fragDataLocationBindings;
    CTransformVaryings transformVaryings;

    Tracked(GLuint tname)
        : name(tname),
          linked(false),
          isSepShaderObj(false),
          _tfBufferMode(0),
          markedToDelete(false) {}
  } track;

  struct Restored {
    template <typename T>
    struct TUniform {
      GLint arraySize;
      GLenum type;
      std::vector<std::vector<T>> data;
    };
    typedef std::unordered_map<std::string, TUniform<GLint>> CIntUniformMap;
    typedef std::unordered_map<std::string, TUniform<GLuint>> CUnsignedUniformMap;
    typedef std::unordered_map<std::string, TUniform<GLfloat>> CFloatUniformMap;
    struct SubroutineData {
      // list of all the subroutines in each shader
      std::vector<std::string> subroutineNames;
      // list of all the uniform subroutines in each shader
      std::vector<std::string> uniformSubroutineNames;
    };
    typedef std::unordered_map<GLenum, SubroutineData> CUniformSubroutineMap;

    std::unordered_map<std::string, int> attribs;
    CIntUniformMap intUniforms;
    CFloatUniformMap floatUniforms;
    CUnsignedUniformMap uintUniforms;

    // list of all active uniforms in a program.
    std::vector<std::string> uniformNames;
    CUniformSubroutineMap uniformSubroutine;
    Restored() {}
  } restore;

private:
  CGLSLProgramStateData() : track(0) {}

public:
  CGLSLProgramStateData(GLuint name) : track(name) {
    GLSLUnique::UseName(name);
  }
  bool operator<(const CGLSLProgramStateData& cmp) const {
    return track.name < cmp.track.name;
  }
};

class CGLSLProgramStateObj {
private:
  std::shared_ptr<CGLSLProgramStateData> _data;
  // Default constructor shouldn't be used
  CGLSLProgramStateObj() {}

public:
  CGLSLProgramStateObj(GLuint name) : _data(new CGLSLProgramStateData(name)) {}

  // Track data methods
  GLuint Name() const {
    return _data->track.name;
  }
  GLenum LastBindEnum() const {
    return GL_CURRENT_PROGRAM;
  }
  void AttachShader(const std::shared_ptr<CGLSLShaderStateData>& shaderdata) {
    _data->track.attachedShaders[shaderdata->track.name] = shaderdata;
  }
  std::vector<GLuint> AttachedShaders() {
    std::vector<GLuint> shadersNames;
    for (auto& shaderData : _data->track.attachedShaders) {
      shadersNames.push_back(shaderData.first);
    }
    return shadersNames;
  }
  void DetachShader(GLuint shadername);
  std::unordered_set<GLint>& Attributes() {
    return _data->track.attribs;
  }
  void Link();
  bool IsPartOfPipeline() const {
    return !_data.unique();
  }
  void MarkToDelete() {
    _data->track.markedToDelete = true;
  }
  bool MarkedToDelete() const {
    return _data->track.markedToDelete;
  }

  // General data methods
  CGLSLProgramStateData& Data() {
    return *_data;
  }
  const CGLSLProgramStateData& Data() const {
    return *_data;
  }
  const std::shared_ptr<CGLSLProgramStateData> DataShared() const {
    return _data;
  }

  bool operator<(const CGLSLProgramStateObj& cmp) const {
    return _data->track.name < cmp._data->track.name;
  }
};

/**
    *
    * CGLSLPipelineStateObj - pipeline state tracking class
    *
    */
struct CGLSLPipelineStateData {
  struct Tracked {
    GLuint name;
    std::map<GLenum, std::shared_ptr<CGLSLProgramStateData>> stages;
    Tracked(GLuint tname) : name(tname) {}
  } track;

  struct Restored {
  } restore;

  CGLSLPipelineStateData() : track(0) {}
  CGLSLPipelineStateData(GLuint name) : track(name) {}
  bool operator<(const CGLSLPipelineStateData& cmp) const {
    return track.name < cmp.track.name;
  }
};

class CGLSLPipelineStateObj {
private:
  CGLSLPipelineStateData _data;
  CGLSLPipelineStateObj() {} // Default constructor shouldn't be used
public:
  GLuint Name() const {
    return _data.track.name;
  }
  CGLSLPipelineStateObj(GLuint name) : _data(name) {}
  void UseProgramStage(GLbitfield stages, GLuint program);
  const CGLSLPipelineStateData& Data() const {
    return _data;
  }
  CGLSLPipelineStateData& Data() {
    return _data;
  }
  bool operator<(const CGLSLPipelineStateObj& cmp) const {
    return _data.track.name < cmp._data.track.name;
  }
};

/**
    *
    * CGLSLFramebufferStateObj - pipeline state tracking class
    *
    */
struct CFramebufferStateData {
  struct TAttachmentInfo {
    GLenum type;
    GLuint name;
    GLenum cubeFace;
    GLint layer3d;
    GLint level;
    TAttachmentInfo() : type(GL_NONE), name(0), cubeFace(0), layer3d(0), level(0) {}
  };
  struct Tracked {
    GLuint name;
    GLenum target;
    typedef std::unordered_map<GLenum, TAttachmentInfo> CAttachmentMap;
    CAttachmentMap attachments;
    // FIXME: Workaround for ES EGL driver issue where incorrect attachments
    // info is returned. After fixing the bug it can be removed. Contains color
    // buffers to be drawn into.
    std::vector<GLenum> drawBuffers;
    Tracked(GLuint tname, GLenum ttarget = 0) : name(tname), target(ttarget) {}
  } track;
  struct Restored {
    GLint readBuffer;
    Restored() : readBuffer(0) {}
  } restore;
  CFramebufferStateData(GLuint name, GLenum target = 0) : track(name, target), restore() {}
};
/**
    *
    * CFramebufferStateObj - framebuffer state tracking class
    *
    */
class CFramebufferStateObj {
  CFramebufferStateData _data;

public:
  typedef CFramebufferStateData::Tracked::CAttachmentMap CAttachmentMap;
  CFramebufferStateObj(GLuint name, GLenum target = GL_FRAMEBUFFER) : _data(name, target) {}
  GLuint Name() const {
    return _data.track.name;
  }
  GLenum Target() const {
    return _data.track.target;
  }
  void SetTarget(GLenum target) {
    _data.track.target = target;
  }
  GLenum LastBindEnum() const {
    return GetBindingEnum(_data.track.target);
  }

  void AddAttachment(GLenum attachment) {
    _data.track.attachments[attachment];
  }
  void RemoveAttachment(GLenum attachment) {
    _data.track.attachments.erase(attachment);
  }
  CAttachmentMap& Attachments() {
    return _data.track.attachments;
  }
  const CAttachmentMap& Attachments() const {
    return _data.track.attachments;
  }

  CFramebufferStateData& Data() {
    return _data;
  }
  const CFramebufferStateData& Data() const {
    return _data;
  }

  bool operator<(const CFramebufferStateObj& cmp) const {
    return _data.track.name < cmp._data.track.name;
  }
};

/**
    *
    * CRenderbufferStateData - render buffer state tracking class
    *
    */

struct CRenderbufferStateData {
  struct Tracked {
    GLuint name;
    GLenum target;
    GLuint samples;
    void* eglImage;
    Tracked(GLuint tname, GLenum ttarget = 0)
        : name(tname), target(ttarget), samples(0), eglImage(0) {}
  } track;
  struct Restored {
    Restored() : width(0), height(0), internalformat(0), datahash(0) {}
    GLuint width;
    GLuint height;
    GLenum internalformat;
    hash_t datahash;
  } restore;
  CRenderbufferStateData(GLuint name, GLenum target = 0) : track(name, target) {}
};

class CRenderbufferStateObj {
  CRenderbufferStateData _data;

public:
  CRenderbufferStateObj(GLuint name, GLenum target = 0) : _data(name, target) {}
  GLuint Name() const {
    return _data.track.name;
  }
  GLenum Target() const {
    return _data.track.target;
  }
  GLenum LastBindEnum() const {
    return GetBindingEnum(_data.track.target);
  }
  CRenderbufferStateData& Data() {
    return _data;
  }
  const CRenderbufferStateData& Data() const {
    return _data;
  }
  void SetTarget(GLenum target) {
    _data.track.target = target;
  }
  bool operator<(const CRenderbufferStateObj& cmp) const {
    return _data.track.name < cmp._data.track.name;
  }
};

struct CBindingStateData {
  struct Restored {
    Restored() {}
  } restore;

  struct Tracked {
    std::unordered_map<GLenum, GLint> boundBuffers;
    GLuint glslProgram;
    GLuint glslPipeline;
    Tracked();
  } track;
};

/**
    *
    * CBindingStateObj - The class stores bindings of OGL objects
    *
    */
class CBindingStateObj {
private:
  CBindingStateData _data;

public:
  std::unordered_map<GLenum, GLint>& BoundBuffers() {
    return _data.track.boundBuffers;
  }
  GLint BoundBuffer(GLenum target) {
    if (_data.track.boundBuffers.find(target) != _data.track.boundBuffers.end()) {
      return _data.track.boundBuffers[target];
    }
    return 0;
  }
  GLuint GLSLProgram() {
    return _data.track.glslProgram;
  }
  void GLSLProgram(GLuint program) {
    _data.track.glslProgram = program;
  }
  GLuint GLSLPipeline() {
    return _data.track.glslPipeline;
  }
  void GLSLPipeline(GLuint pipeline) {
    _data.track.glslPipeline = pipeline;
  }
};

struct CIndexedBindingRangeStateData {
  struct TIndexInfo {
    GLuint buffer;
    GLintptr offset;
    GLsizeiptr size;
  };
  typedef std::map<GLuint, TIndexInfo> CIndexMap;
  typedef std::map<GLenum, CIndexMap> CTargetMap;
  struct Tracked {
    CTargetMap targetsInfo;
  } track;

  // struct Restored{}restore; //  All info is tracked
};

/**
    *
    * CIndexedBindingRangeStateObj - the class tracks range for buffers of indexed targets
    *
    */
class CIndexedBindingRangeStateObj {
  CIndexedBindingRangeStateData _data;

public:
  typedef CIndexedBindingRangeStateData::CTargetMap CTargetMap;
  CTargetMap& TargetsInfo() {
    return _data.track.targetsInfo;
  }
};

struct CBufferStateData {
  struct Tracked {
    GLuint name;
    GLenum target;
    GLint size;
    GLenum usage;
    bool initializedData;
    IntervalSet<uint64_t> initializedDataMap;
    bool immutable;
    GLbitfield flags;
    bool coherentMapping;
    Tracked(GLuint tname, GLenum ttarget = 0)
        : name(tname),
          target(ttarget),
          size(0),
          usage(GL_STATIC_DRAW),
          initializedData(true),
          immutable(false),
          flags(0),
          coherentMapping(false) {}
  } track;

  struct Restored {
    GLbitfield mapAccess;
    GLint mapLength;
    GLint mapOffset;
    GLintptr mapFlushRangeOffset;
    GLsizeiptr mapFlushRangeLength;
    bool mapped;
    enum buffer_type {
      MAP_BUFFER,
      MAP_BUFFER_OES,
      MAP_BUFFER_ARB,
      MAP_BUFFER_EXT
    };
    buffer_type type;
    std::vector<GLubyte> buffer;
    bool named;
  } restore;
  CBufferStateData(GLuint name, GLenum target = 0) : track(name, target) {}
};
/**
    *
    * CBufferStateObj - buffer state tracking class
    *
    */
class CBufferStateObj {
private:
  CBufferStateData _data;
  void SetBufferMapRec(GLenum access, bool named, GLint length, GLint offset);
  void SetBufferMapPlay(GLenum access, bool named, GLint length, GLint offset);

public:
  explicit CBufferStateObj(GLuint buffer, GLenum target = 0);

  GLenum Target() const {
    return _data.track.target;
  }
  void SetTarget(GLenum target) {
    _data.track.target = target;
  }
  GLuint Name() const {
    return _data.track.name;
  }
  GLenum LastBindEnum() const {
    return GetBindingEnum(_data.track.target);
  }
  GLuint Size() const {
    return _data.track.size;
  }
  void SizeSet(GLint size) {
    _data.track.size = size;
  }
  GLuint Usage() const {
    return _data.track.usage;
  }
  void UsageSet(GLint usage) {
    _data.track.immutable = false;
    _data.track.usage = usage;
  }
  GLbitfield Flags() const {
    return _data.track.flags;
  };
  void FlagsSet(GLbitfield flags) {
    _data.track.immutable = true;
    if ((flags & GL_MAP_COHERENT_BIT) ||
        ((flags & GL_MAP_PERSISTENT_BIT) &&
         Configurator::Get().opengl.recorder.coherentMapBehaviorWA)) {
      _data.track.flags = flags | GL_MAP_READ_BIT;
    } else {
      _data.track.flags = flags;
    }
  }
  bool Immutable() const {
    return _data.track.immutable;
  }
  std::vector<GLubyte>& Buffer() {
    return _data.restore.buffer;
  }
  void InitBufferMapPlay(GLenum access = GL_MAP_READ_BIT,
                         bool named = false,
                         GLint length = -1,
                         GLint offset = -1);
  void InitBufferMapPlayOES(GLenum access = GL_MAP_READ_BIT,
                            bool named = false,
                            GLint length = -1,
                            GLint offset = -1);
  void InitBufferMapPlayARB(GLenum access = GL_MAP_READ_BIT,
                            bool named = false,
                            GLint length = -1,
                            GLint offset = -1);
  void InitBufferMapPlayEXT(GLenum access = GL_MAP_READ_BIT,
                            bool named = false,
                            GLint length = -1,
                            GLint offset = -1);
  void InitBufferMapRec(GLenum access = GL_MAP_READ_BIT,
                        bool named = false,
                        GLint length = -1,
                        GLint offset = -1);
  void InitBufferMapRecOES(GLenum access = GL_MAP_READ_BIT,
                           bool named = false,
                           GLint length = -1,
                           GLint offset = -1);
  void InitBufferMapRecARB(GLenum access = GL_MAP_READ_BIT,
                           bool named = false,
                           GLint length = -1,
                           GLint offset = -1);
  void InitBufferMapRecEXT(GLenum access = GL_MAP_READ_BIT,
                           bool named = false,
                           GLint length = -1,
                           GLint offset = -1);
  void FlushMappedBufferRange(GLintptr offset, GLsizeiptr length);

  void TrackBufferData(GLintptr buffoffset, GLsizeiptr size, const GLvoid* dataptr);
  void CalculateMapChange(GLintptr& mapoffset,
                          GLintptr& buffoffset,
                          GLsizeiptr& size,
                          const GLvoid* ptr);
  void RemoveMapping() {
    _data.restore.mapped = false;
    _data.restore.mapAccess = 0;
    _data.restore.mapLength = -1;
    _data.restore.mapOffset = -1;
    _data.restore.mapFlushRangeOffset = 0;
    _data.restore.mapFlushRangeLength = 0;
  }
  const CBufferStateData& Data() const {
    return _data;
  }
  bool operator<(const CBufferStateObj& cmp) const {
    return _data.track.name < cmp._data.track.name;
  }
};

/**
    *
    * CVertexArrayStateObj - buffer state tracking class
    *
    */

struct CVertexArraysStateData {
  struct Tracked {
    typedef std::set<GLuint> CAttribsSet;

    GLint name;
    CAttribsSet attribs;
    Tracked(GLint tname) : name(tname) {}
  } track;
  struct Restored {
    struct TAttribState {
      GLint arrayBufferBinding;
      GLint enabled;
      GLint size;
      GLint stride;
      GLint type;
      GLint normalized;
      GLint integer;
      GLint divisor;
      GLint binding;
      GLint relativeOffset;
      GLvoid* pointer;
      TAttribState()
          : arrayBufferBinding(0),
            enabled(0),
            size(0),
            stride(0),
            type(0),
            normalized(0),
            integer(0),
            divisor(0),
            binding(0),
            relativeOffset(0),
            pointer(nullptr) {}
    };
    struct TBindingPointState {
      GLint arrayBufferBinding;
      GLint bindingOffset;
      GLint bindingStride;
      GLint bindingDivisor;
      TBindingPointState()
          : arrayBufferBinding(0), bindingOffset(0), bindingStride(0), bindingDivisor(0) {}
    };
    struct TColorState {
      GLboolean enabled;
      GLint size;
      GLint type;
      GLsizei stride;
      GLvoid* pointer;
      TColorState() : enabled(false), size(0), type(0), stride(0), pointer(nullptr) {}
    };
    struct TSecondaryColorState {
      GLboolean enabled;
      GLint size;
      GLint type;
      GLsizei stride;
      GLvoid* pointer;
      TSecondaryColorState() : enabled(false), size(0), type(0), stride(0), pointer(nullptr) {}
    };
    struct TVertexPointerState {
      GLboolean enabled;
      GLint size;
      GLint type;
      GLsizei stride;
      GLvoid* pointer;
      GLint boundBuffer;
      TVertexPointerState()
          : enabled(false), size(0), type(0), stride(0), pointer(nullptr), boundBuffer(0) {}
    };
    struct TNormalPointerState {
      GLboolean enabled;
      GLint type;
      GLsizei stride;
      GLvoid* pointer;
      GLint boundBuffer;
      TNormalPointerState()
          : enabled(false), type(0), stride(0), pointer(nullptr), boundBuffer(0) {}
    };
    struct TTexCoordState {
      GLboolean enabled;
      GLint size;
      GLint type;
      GLint stride;
      GLint boundBuffer;
      GLvoid* pointer;
      TTexCoordState()
          : enabled(false), size(0), type(0), stride(0), boundBuffer(0), pointer(nullptr) {}
    };

    typedef std::map<unsigned, TAttribState> CAttribsMap;
    typedef std::map<GLint, TBindingPointState> CBindingPointsMap;
    typedef std::map<unsigned, TTexCoordState> CTexCoordMap;

    CAttribsMap attribsMap;
    CBindingPointsMap bindingPointsMap;
    TColorState colorState;
    TSecondaryColorState secondaryColorState;
    TVertexPointerState vertexPointerState;
    TNormalPointerState normalPointerState;
    CTexCoordMap texCoordMap;
    GLint clientActiveTexture;
    GLint maxTexCoords;

    GLint elementArrayBufferBinding;
    Restored()
        : colorState(),
          secondaryColorState(),
          vertexPointerState(),
          normalPointerState(),
          clientActiveTexture(0),
          maxTexCoords(0),
          elementArrayBufferBinding(0) {}
  } restore;
  CVertexArraysStateData(GLint name) : track(name), restore() {}
};

class CVertexArraysStateObj {
  CVertexArraysStateData _data;

public:
  CVertexArraysStateObj(GLint name) : _data(name) {}
  GLuint Name() const {
    return _data.track.name;
  }
  GLenum LastBindEnum() const {
    return GL_VERTEX_ARRAY_BINDING;
  }
  CVertexArraysStateData& Data() {
    return _data;
  }
  const CVertexArraysStateData& Data() const {
    return _data;
  }
  bool operator<(const CVertexArraysStateObj& cmp) const {
    return _data.track.name < cmp._data.track.name;
  }
};

/**
    *
    * CSamplerStateObj - buffer state tracking class
    *
    */

struct CSamplerStateData {
  struct Tracked {
    GLuint name;
    Tracked(GLuint tname) : name(tname) {}
  } track;
  struct Restored {
    GLfloat _borderColor[4];
    GLint _minificationFunction;
    GLint _magnificationFunction;
    GLint _texcoordSWrapMode;
    GLint _texcoordTWrapMode;
    GLint _texcoordRWrapMode;
    GLfloat _minimumLevelOfDetail;
    GLfloat _maximumLevelOfDetail;
    GLfloat _textureLevelOfDetail;
    GLint _textureCompareMode;
    GLint _textureCompareFunc;
    GLfloat _textureMaxLevelOfAnisotropyEXT;
    GLint _textureSrgbDecode;
    Restored()
        : _minificationFunction(0),
          _magnificationFunction(0),
          _texcoordSWrapMode(0),
          _texcoordTWrapMode(0),
          _texcoordRWrapMode(0),
          _minimumLevelOfDetail(0),
          _maximumLevelOfDetail(0),
          _textureLevelOfDetail(0),
          _textureCompareMode(0),
          _textureCompareFunc(0),
          _textureMaxLevelOfAnisotropyEXT(0),
          _textureSrgbDecode(0) {}
  } restore;
  CSamplerStateData(GLuint name) : track(name), restore() {}
};

class CSamplerStateObj {
  CSamplerStateData _data;

public:
  CSamplerStateObj(GLuint name) : _data(name) {}
  GLuint Name() const {
    return _data.track.name;
  }
  GLenum LastBindEnum() const {
    return GL_SAMPLER_BINDING;
  }
  CSamplerStateData& Data() {
    return _data;
  }
  const CSamplerStateData& Data() const {
    return _data;
  }
  bool operator<(const CSamplerStateObj& cmp) const {
    return _data.track.name < cmp._data.track.name;
  }
};

/**
    *
    * CClientArrayStateObject - client array state tracking class
    *                           representing state of GL_xxx_ARRAY, along with set glxxxPointer
    */
struct CClientArrayStateData {
  struct Tracked {
    bool enabled;
    bool isBuff;
    GLint size;
    GLuint divisor;
    GLenum type;
    GLsizei stride;
    const GLvoid* ptr;
    Tracked() : enabled(false), isBuff(false), size(0), divisor(0), type(0), stride(0), ptr(0) {}
  } track;

  CClientArrayStateData() : track() {}
};

class CClientArrayStateObj {
  CClientArrayStateData _data;

public:
  CClientArrayStateObj() {}
  ~CClientArrayStateObj() {}

  void Enabled(bool enabled) {
    _data.track.enabled = enabled;
  }
  bool Enabled() const {
    return _data.track.enabled && (_data.track.type != 0);
  } // type check is needed in case of buggy apps (like fishlabls galaxy on
  // fire) to ensure that pointer is initialized
  const GLvoid* Pointer() const {
    return _data.track.ptr;
  }
  bool IsBuffArray() const {
    return _data.track.isBuff;
  }
  void BuffArray(bool isbuff) {
    _data.track.isBuff = isbuff;
  }
  void Params(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {
    if (size == (GLint)GL_BGRA) {
      _data.track.size = 4;
    } else {
      _data.track.size = size;
    }
    _data.track.type = type;
    _data.track.stride = stride;
    _data.track.ptr = ptr;
  }
  void Params(bool isbuff, GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {
    Params(size, type, stride, ptr);
    _data.track.isBuff = isbuff;
  }
  CClientArrayStateData& Data() {
    return _data;
  }
  const CClientArrayStateData& Data() const {
    return _data;
  }
};

/**
    *
    * CClientArraysStateObject - client arrays state tracking class
    *                            aggregate holding state of arrays
    */
class CClientArraysStateObj {
public:
  typedef std::unordered_map<GLenum, CClientArrayStateObj> CClientArrayMap;

private:
  CClientArrayStateObj _vertexArray;
  CClientArrayStateObj _normalArray;
  CClientArrayStateObj _colorArray;
  CClientArrayStateObj _secondaryColorArray;
  CClientArrayStateObj _interleavedArray;
  CClientArrayMap _texcoordArrays;
  CClientArrayMap _vertexAttribArrays;

public:
  CClientArrayStateObj& VertexArray() {
    return _vertexArray;
  }
  CClientArrayStateObj& NormalArray() {
    return _normalArray;
  }
  CClientArrayStateObj& ColorArray() {
    return _colorArray;
  }
  CClientArrayStateObj& SecondaryColorArray() {
    return _secondaryColorArray;
  }
  CClientArrayStateObj& InterleavedArray() {
    return _interleavedArray;
  }

  CClientArrayStateObj& TexCoordArray(GLenum texture) {
    return _texcoordArrays[texture];
  }
  const CClientArrayMap& TexCoordArrayMap() const {
    return _texcoordArrays;
  }

  CClientArrayStateObj& VertexAttribArray(GLuint attribIdx) {
    return _vertexAttribArrays[attribIdx];
  }
  const CClientArrayMap& VertexAttribArrayMap() const {
    return _vertexAttribArrays;
  }
  void RestoreClientAttribs();
};

/**
    *
    * CMappedTexturesStateObject - mapped textures state tracking class
    *
    */
class CMappedTexturesStateObj {
public:
  struct MappedLevel {
    MappedLevel()
        : texture(0),
          level(0),
          size(0),
          pointer(0),
          isInitialized(false),
          isRemoved(false),
          access(0),
          stride(0),
          layout(0),
          hash(0) {}
    unsigned int texture;
    int level;
    unsigned int size;
    void* pointer;
    bool isInitialized;
    bool isRemoved;
    int access;
    int stride;
    int layout;
    hash_t hash;
  };

private:
  typedef std::multimap<GLint, MappedLevel> map_t;
  map_t mapped_;

public:
  CMappedTexturesStateObj() {}
  virtual ~CMappedTexturesStateObj() {}

  // Functions used by both recorder and player
  void MapTexture(
      GLint name, GLint level, void* mapping, int size, int access, int* stride, int* layout);
  void UnmapTexture(GLint name, GLint level);

  // Functions for StateRestore
  void InitializeTexture(GLint name, GLint level);
  void RemoveTexture(GLint name, GLint level);
  std::vector<MappedLevel> ReturnNewTextures();
  bool CheckIfTextureIsMapped(int name);
  int GetLayout(int name);

  // Functions used by Player
  void UpdateTexture(GLint name, GLint level, const void* mapping);

  // Functions used by recorder
  std::vector<MappedLevel> SaveTextureToFile(GLint name, GLint level);
  std::vector<MappedLevel> SaveAllTexturesToFile();
};

struct CBlendFunciData {
  struct TBlendFunciInfo {
    GLenum srcRGB;
    GLenum dstRGB;
    GLenum srcAlpha;
    GLenum dstAlpha;
    GLenum src;
    GLenum dst;
    bool separate;
  };
  typedef std::map<GLuint, TBlendFunciInfo> CNameBlendFunciMap;
  struct Tracked {
    CNameBlendFunciMap mapInfo;
  } track;
};

class CBlendFunciObj {
  CBlendFunciData _data;

public:
  typedef CBlendFunciData::CNameBlendFunciMap CNameBlendFunciMap;
  CNameBlendFunciMap& BlendFunciNameMapInfo() {
    return _data.track.mapInfo;
  }
};

struct CBlendEquationiData {
  struct TBlendEquationiInfo {
    GLenum modeRGB;
    GLenum modeAlpha;
    GLenum mode;
    bool separate;
  };
  typedef std::map<GLuint, TBlendEquationiInfo> CNameBlendEquationiMap;
  struct Tracked {
    CNameBlendEquationiMap mapInfo;
  } track;
};

class CBlendEquationiObj {
  CBlendEquationiData _data;

public:
  typedef CBlendEquationiData::CNameBlendEquationiMap CNameBlendEquationiMap;
  CNameBlendEquationiMap& BlendEquationiNameMapInfo() {
    return _data.track.mapInfo;
  }
};

struct CGeneralStateData {
  struct Restore {
    GLclampf clearDepth;
    GLint clearStencil;
    GLint cullFace;
    GLint depthFunc;
    GLboolean depthMask;
    GLint frontFace;
    GLint logicOp;
    GLfloat pointSize;
    GLint shaderModel;
    GLfloat lineWidth;
    std::vector<GLenum> drawBuffers;
    std::vector<GLfloat> texCoord_s;
    std::vector<GLfloat> texCoord_t;
    std::vector<GLfloat> texCoord_r;
    std::vector<GLfloat> texCoord_q;

    Restore()
        : clearDepth(1),
          clearStencil(0),
          cullFace(GL_BACK),
          depthFunc(GL_LESS),
          depthMask(GL_TRUE),
          frontFace(GL_CCW),
          logicOp(GL_COPY),
          pointSize(1.0f),
          shaderModel(GL_SMOOTH),
          lineWidth(1.0f) {}

    struct CAlphaFunc {
      GLint func;
      GLclampf ref;
      CAlphaFunc() : func(GL_ALWAYS), ref(0) {}
    } alphaFunc;

    struct CClearColor {
      GLclampf red;
      GLclampf green;
      GLclampf blue;
      GLclampf alpha;
      CClearColor() : red(0), green(0), blue(0), alpha(0) {}
    } clearColor;

    struct CColor {
      GLfloat red;
      GLfloat green;
      GLfloat blue;
      GLfloat alpha;
      CColor() : red(1), green(1), blue(1), alpha(1) {}
    } color;

    struct CColorMask {
      GLboolean red;
      GLboolean green;
      GLboolean blue;
      GLboolean alpha;
      CColorMask() : red(GL_TRUE), green(GL_TRUE), blue(GL_TRUE), alpha(GL_TRUE) {}
    } colorMask;

    struct CPolygonOffset {
      GLfloat factor;
      GLfloat units;
      CPolygonOffset() : factor(0), units(0) {}
    } polygonOffset;

    struct CMatrices {
      GLenum mode;
      GLfloat matrixArray[4][16];
      CMatrices() : mode(GL_MODELVIEW) {
        for (int i = 0; i < 4; ++i) {
          // a guess on default state (unit matrix,or zero ?)
          std::fill_n(matrixArray[i], 16, 0.f);
          matrixArray[i][0] = matrixArray[i][5] = matrixArray[i][10] = matrixArray[i][15] = 1.0f;
        }
      }
    } matrices;

    struct CClearAccum {
      GLfloat red;
      GLfloat green;
      GLfloat blue;
      GLfloat alpha;
      CClearAccum() : red(0), green(0), blue(0), alpha(0) {}
    } clearAccum;

  } restored;

  struct Track {
    GLint activeTexture;
    GLint clientActiveTexture;
    using TargetToTextureMap = std::map<GLenum, GLuint>;
    std::map<GLint, TargetToTextureMap> boundTextures;

    struct LineStipple {
      GLushort pattern;
      GLint factor;
      bool used;
      LineStipple() : pattern(0xFFFF), factor(1), used(0) {}
    };
    Track() : activeTexture(GL_TEXTURE0), clientActiveTexture(GL_TEXTURE0) {}

    struct CLineStipple {
      bool used;
      GLint factor;
      GLushort pattern;
      CLineStipple() : used(0), factor(1), pattern(0xFFFF) {}
    } lineStipple;

    struct CBlendFunc {
      bool used;
      GLenum src;
      GLenum dst;
      CBlendFunc() : used(0), src(GL_ONE), dst(GL_ZERO) {}
    } blendFunc;

    struct CBlendEquation {
      bool used;
      GLenum mode;
      CBlendEquation() : used(0), mode(GL_FUNC_ADD) {}
    } blendEquation;

    struct CBlendFuncSeparate {
      bool used;
      GLenum srcRGB;
      GLenum dstRGB;
      GLenum srcAlpha;
      GLenum dstAlpha;
      CBlendFuncSeparate()
          : used(0), srcRGB(GL_ONE), dstRGB(GL_ZERO), srcAlpha(GL_ZERO), dstAlpha(GL_ZERO) {}
    } blendFuncSeparate;

    struct CBlendEquationSeparate {
      bool used;
      GLenum modeRGB;
      GLenum modeAlpha;
      CBlendEquationSeparate() : used(0), modeRGB(GL_FUNC_ADD), modeAlpha(GL_FUNC_ADD) {}
    } blendEquationSeparate;

  } tracked;
};

/**
    *
    * CGeneralStateObj - simple OGL states tracking class
    *
    */
class CGeneralStateObj {
  CGeneralStateData _data;

public:
  CGeneralStateData& Data() {
    return _data;
  }
};
} // namespace OpenGL
} // namespace gits

#define GITS_MAKE_HASH(type)                                                                       \
  namespace std {                                                                                  \
  template <>                                                                                      \
  struct hash<type> {                                                                              \
    size_t operator()(const type& obj) const {                                                     \
      return hash<GLint>()(obj.Name());                                                            \
    }                                                                                              \
  };                                                                                               \
  }                                                                                                \
  namespace gits {                                                                                 \
  namespace OpenGL {                                                                               \
  inline bool operator==(const type& lhs, const type& rhs) {                                       \
    return lhs.Name() == rhs.Name();                                                               \
  }                                                                                                \
  }                                                                                                \
  }

GITS_MAKE_HASH(gits::OpenGL::CRenderbufferStateObj)
GITS_MAKE_HASH(gits::OpenGL::CVertexArraysStateObj)
GITS_MAKE_HASH(gits::OpenGL::CFramebufferStateObj)
GITS_MAKE_HASH(gits::OpenGL::CBufferStateObj)
GITS_MAKE_HASH(gits::OpenGL::CSamplerStateObj)
GITS_MAKE_HASH(gits::OpenGL::CARBProgramStateObj)
GITS_MAKE_HASH(gits::OpenGL::CGLSLShaderStateObj)
GITS_MAKE_HASH(gits::OpenGL::CGLSLProgramStateObj)
GITS_MAKE_HASH(gits::OpenGL::CGLSLPipelineStateObj)
#undef GITS_MAKE_HASH

// these two are special as they have composite hashes
namespace std {
template <>
struct hash<gits::OpenGL::CTextureStateObj> {
  size_t operator()(const gits::OpenGL::CTextureStateObj& obj) const {
    return hash<GLint>()(obj.Name()) ^ hash<GLenum>()(obj.Target());
  }
};
template <>
struct hash<gits::OpenGL::CARBProgramEnvParamsStateObj> {
  size_t operator()(const gits::OpenGL::CARBProgramEnvParamsStateObj& obj) const {
    return hash<GLint>()(obj.Name()) ^ hash<GLenum>()(obj.Target());
  }
};
} // namespace std

namespace gits {
namespace OpenGL {
inline bool operator==(const CTextureStateObj& lhs, const CTextureStateObj& rhs) {
  return lhs.Name() == rhs.Name() && lhs.Target() == rhs.Target();
}
inline bool operator==(const CARBProgramEnvParamsStateObj& lhs,
                       const CARBProgramEnvParamsStateObj& rhs) {
  return lhs.Name() == rhs.Name() && lhs.Target() == rhs.Target();
}
} // namespace OpenGL
} // namespace gits
