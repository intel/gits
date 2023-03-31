// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   mapping.h
 *
 * @brief The purpose of this file is to provide mapping functionality that maps recording
 *        time IDs to playback time IDs. shader id is one such example.
 *
 */

#pragma once
#include "openglTypes.h"
#include "openglLibrary.h"
#ifdef BUILD_FOR_CCODE
#include "openglArguments.h" // "openglArguments.h" file for CCode is different than the one used for regular GITS builds
#endif

#include <unordered_map>

#ifdef check
#undef check
#endif
DISABLE_WARNINGS
#include <boost/icl/interval_map.hpp>
ENABLE_WARNINGS

namespace icl = boost::icl;

namespace gits {
namespace OpenGL {

//This enumerator is for all distinct Object types which require Recording->Playback mapping of IDs.
//The EXT version of an Object may require a separate mapping instance in which case a distinct enumerator should also be
//added for it.
typedef enum MapObjects {
  OGL_PROGRAM_SHADER_MAP = 0,
  OGL_VERTEXARRAY_MAP,
  OGL_VERTEXARRAY_EXT_MAP,
  OGL_FRAME_BUFFER_MAP,
  OGL_FRAME_BUFFER_EXT_MAP,
  OGL_RENDER_BUFFER_MAP,
  OGL_RENDER_BUFFER_EXT_MAP,
  OGL_QUERY_MAP,
  OGL_TEXTURE_MAP,
  OGL_BUFFER_MAP,
  OGL_UNIFORM_LOCATION_MAP,
  OGL_UNIFORM_SUBROUTINE_LOCATION_MAP,
  OGL_SUBROUTINE_INDEX_MAP,
  OGL_UNIFORM_INDEX_MAP,
  OGL_UNIFORM_BLOCK_INDEX_MAP,
  OGL_SAMPLER_MAP,
  OGL_SYNC_MAP,
  OGL_VERTEXARRAY_APPLE_MAP,
  OGL_PIPELINE_MAP,
  OGL_TRANSFORM_FEEDBACK_MAP,
  OGL_FENCE_MAP,
  OGL_TEXTURE_HANDLE_MAP,

  WGL_HDC_MAP,
  WGL_HGLRC_MAP,
  WGL_HANDLE_MAP,
  WGL_HWND_MAP,
  WGL_HPBUFFERARB_MAP,
  WGL_HPBUFFEREXT_MAP,

  EGL_CONTEXT_MAP,
  EGL_DISPLAY_MAP,
  EGL_SURFACE_MAP,
  EGL_CLIENTBUFFER_MAP,
  EGL_NATIVEDISPLAYTYPE_MAP,
  EGL_NATIVEPIXMAPTYPE_MAP,
  EGL_NATIVEWINDOWTYPE_MAP,

  EGL_IMAGE_KHR_MAP,
  EGL_SYNC_KHR_MAP,

  CGL_RENDER_INFO_MAP,
  CGL_PIXELFORMAT_MAP,
  CGL_CONTEXT_MAP,
  CGL_PBUFFER_MAP,
  CGL_APPLEWINDOW_MAP,

  X_DISPLAY_MAP,
  X_VISUALINFO_MAP,
  GLX_FBCONFIG_MAP,
  GLX_CONTEXT_MAP,
  GLX_DRAWABLE_MAP,
} MapObjects;

namespace {

template <class T>
struct ptr_to_void {
  typedef T type;
};
template <class T>
struct ptr_to_void<T*> {
  typedef void* type;
};

template <class T>
T gl_name_cast(int name) {
  return name;
}
template <>
void* gl_name_cast(int name) {
  return reinterpret_cast<void*>(static_cast<uintptr_t>(name));
}
} // namespace

template <int Native, int Shared, int Legacy, int Nezero>
struct MappedObjectTraits {
  enum { native = Native }; // is this native api object or GL object
  enum { shared = Shared }; // is the object shared in multiple contexts
  enum {
    legacy = Legacy
  };                        // does the object support bind w/o gen in some ctxs
                            // 0 - requires mapping everywhere
                            // 1 - doesn't require mapping in compat desktop
                            // 2 - doesn't require mapping in ES context
  enum { nezero = Nezero }; // does the object name 0 map to itself
};

bool isMappingRequired(MapObjects mapObject, int legacy);

// Structure for mapping id of mapped argument to its type.
template <int>
struct IdMappedArgMap {};

struct CMappedArgumentName {
  static const char* NAME;
};

template <MapObjects MapObjectId, typename Type, class Traits>
class CMappedArgument : public CArgument, private CMappedArgumentName {
  Type key_;

public:
  using CMappedArgumentName::NAME;
  typedef CArgumentMappedSizedArray<Type, CMappedArgument, gits::NO_ACTION> CSArray;
  typedef CArgumentMappedSizedArray<Type, CMappedArgument, gits::ADD_MAPPING> CSMapArray;
  typedef CArgumentMappedSizedArray<Type, CMappedArgument, gits::REMOVE_MAPPING> CSUnmapArray;

  static const int MapId = MapObjectId;
  CMappedArgument() {}
  CMappedArgument(Type arg) : key_(arg) {}

  static void AddMapping(Type key, Type value) {
    if (!identity<Traits::legacy>() || isMappingRequired(MapObjectId, Traits::legacy)) {
      get_map()[key] = value;
    }
  }

  void AddMapping(Type value) {
    AddMapping(key_, value);
  }

  static void AddMapping(const Type* keys, const Type* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      AddMapping(keys[i], values[i]);
    }
  }

  void RemoveMapping() {
    RemoveMapping(key_);
  }

  static void RemoveMapping(Type key) {
    get_map().erase(key);
  }

  static void RemoveMapping(const Type* keys, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      RemoveMapping(keys[i]);
    }
  }

  static Type GetMapping(Type key) {
    if (identity<Traits::legacy>() && !isMappingRequired(MapObjectId, Traits::legacy)) {
      return key;
    }

    if (identity<Traits::nezero>() && key == Type(0)) {
      return Type(0);
    }

    auto& the_map = get_map();
    auto iter = the_map.find(key);
    if (iter == the_map.end()) {
      Log(WARN) << "Couldn't map object name " << key << " of typeid " << MapObjectId;
      return (Type)gl_name_cast<typename ptr_to_void<Type>::type>(1000000);
    }

    return iter->second;
  }

  static void GetMapping(const Type* keys, Type* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      values[i] = GetMapping(keys[i]);
    }
  }

  static std::vector<Type> GetMapping(const Type* keys, size_t num) {
    std::vector<Type> v;
    v.reserve(num);
    for (size_t i = 0; i < num; ++i) {
      v.push_back(GetMapping(keys[i]));
    }
    return v;
  }

  static bool CheckMapping(Type key) {
    if ((identity<Traits::legacy>() && !isMappingRequired(MapObjectId, Traits::legacy)) ||
        (identity<Traits::nezero>() && key == Type(0))) {
      return true;
    }
    auto& the_map = get_map();
    return the_map.find(key) != the_map.end();
  }

  bool CheckMapping() {
    return CheckMapping(key_);
  }

  void Reset(Type value) {
    key_ = value;
  }
  Type Original() const {
    return key_;
  }
  Type Value() const {
    return GetMapping(key_);
  }
  Type operator*() const {
    return Value();
  }

  virtual const char* Name() const {
    return NAME;
  }

  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, key_);
  }

  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, key_);
  }

  virtual void Write(CCodeOStream& stream) const {
    stream << "name<" << IdEnumStr() << ">(";
    if (identity<boost::is_pointer<Type>::value>()) {
      stream << "0x";
    }
    stream << (typename ptr_to_void<Type>::type)key_ << ")";
  }

  static const char* TypeNameStr() {
    return IdMappedArgMap<MapId>::TypeNameStr();
  }
  static const char* WrapTypeNameStr() {
    return IdMappedArgMap<MapId>::WrapTypeNameStr();
  }
  static const char* IdEnumStr() {
    return IdMappedArgMap<MapId>::IdEnumStr();
  }

  void Assign(Type other) {
    AddMapping(other);
  }

private:
  typedef std::unordered_map<Type, Type> name_map_t;
  typedef std::unordered_map<unsigned int, name_map_t> group_maps_t;
  typedef std::unordered_map<void*, name_map_t> context_maps_t;
  static name_map_t& get_map() {
    if (identity<Traits::native>()) {
      INIT_NEW_STATIC_OBJ(global_names_map, name_map_t)
      return global_names_map;
    } else {
      if (identity<Traits::shared>()) {
        INIT_NEW_STATIC_OBJ(shared_objects_map, group_maps_t)
        return shared_objects_map[SD().GetCurrentContextSharedGroupId()];
      } else {
        INIT_NEW_STATIC_OBJ(objects_map, context_maps_t)
        return objects_map[SD().GetCurrentContext()];
      }
    }
  }
};

typedef MappedObjectTraits<1, 0, 0, 1> NativeObjectTraits;         //egl, eglimage, etc
typedef MappedObjectTraits<1, 0, 0, 0> ZeroNativeObjectTraits;     //egl native display
typedef MappedObjectTraits<0, 1, 1, 1> SharedLegacyGLObjectTraits; //fboext, rboext
typedef MappedObjectTraits<0, 0, 1, 1> LegacyGLObjectTraits;       //query
typedef MappedObjectTraits<0, 1, 0, 1> SharedGLObjectTraits;       //program, shader, sync, sampler
typedef MappedObjectTraits<0, 0, 0, 1> GLObjectTraits;             //vao, pipe
typedef MappedObjectTraits<0, 0, 2, 1> GLObjectLegacyESTraits;

typedef MappedObjectTraits<0, 1, 2, 1> SharedGLObjectlegacyESTraits;

typedef MappedObjectTraits<0, 1, 3, 1> SharedLegacyESGLObjectTraits; //texture, buffer

#define DEFINE_MAPPED_ARGUMENT(id, type_, traits, name)                                            \
  typedef CMappedArgument<id, type_, traits> name;                                                 \
  template <>                                                                                      \
  struct IdMappedArgMap<id> {                                                                      \
    typedef name type;                                                                             \
    static const char* TypeNameStr() {                                                             \
      return #type_;                                                                               \
    }                                                                                              \
    static const char* WrapTypeNameStr() {                                                         \
      return #name;                                                                                \
    }                                                                                              \
    static const char* IdEnumStr() {                                                               \
      return #id;                                                                                  \
    }                                                                                              \
  };

//gl
DEFINE_MAPPED_ARGUMENT(OGL_FRAME_BUFFER_EXT_MAP,
                       GLuint,
                       SharedLegacyGLObjectTraits,
                       CGLFramebufferEXT)
DEFINE_MAPPED_ARGUMENT(OGL_RENDER_BUFFER_EXT_MAP,
                       GLuint,
                       SharedLegacyGLObjectTraits,
                       CGLRenderbufferEXT)
DEFINE_MAPPED_ARGUMENT(OGL_TEXTURE_MAP, GLuint, SharedLegacyESGLObjectTraits, CGLTexture)
DEFINE_MAPPED_ARGUMENT(OGL_BUFFER_MAP, GLuint, SharedLegacyESGLObjectTraits, CGLBuffer)
DEFINE_MAPPED_ARGUMENT(OGL_TEXTURE_HANDLE_MAP, GLuint64, GLObjectTraits, CGLTextureHandle)

DEFINE_MAPPED_ARGUMENT(OGL_QUERY_MAP, GLuint, LegacyGLObjectTraits, CGLQuery)
DEFINE_MAPPED_ARGUMENT(OGL_FENCE_MAP, GLuint, LegacyGLObjectTraits, CGLfence)

DEFINE_MAPPED_ARGUMENT(OGL_PROGRAM_SHADER_MAP, GLuint, SharedGLObjectTraits, CGLProgram)
DEFINE_MAPPED_ARGUMENT(OGL_SYNC_MAP, GLsync, SharedGLObjectTraits, CGLsync)
DEFINE_MAPPED_ARGUMENT(OGL_SAMPLER_MAP, GLuint, SharedGLObjectTraits, CGLSampler)

DEFINE_MAPPED_ARGUMENT(OGL_VERTEXARRAY_MAP, GLuint, GLObjectTraits, CGLVertexArray)
DEFINE_MAPPED_ARGUMENT(OGL_VERTEXARRAY_APPLE_MAP, GLuint, GLObjectTraits, CGLVertexArrayAPPLE)
DEFINE_MAPPED_ARGUMENT(OGL_PIPELINE_MAP, GLuint, GLObjectTraits, CGLPipeline)
DEFINE_MAPPED_ARGUMENT(OGL_TRANSFORM_FEEDBACK_MAP, GLuint, GLObjectTraits, CGLTransformFeedback)
DEFINE_MAPPED_ARGUMENT(OGL_FRAME_BUFFER_MAP, GLuint, GLObjectLegacyESTraits, CGLFramebuffer)
DEFINE_MAPPED_ARGUMENT(OGL_RENDER_BUFFER_MAP, GLuint, SharedGLObjectlegacyESTraits, CGLRenderbuffer)

//wgl
DEFINE_MAPPED_ARGUMENT(WGL_HDC_MAP, HDC, NativeObjectTraits, CHDC)
DEFINE_MAPPED_ARGUMENT(WGL_HGLRC_MAP, HGLRC, NativeObjectTraits, CHGLRC)
DEFINE_MAPPED_ARGUMENT(WGL_HANDLE_MAP, HANDLE, NativeObjectTraits, CHANDLE)
DEFINE_MAPPED_ARGUMENT(WGL_HWND_MAP, HWND, NativeObjectTraits, CHWND)
DEFINE_MAPPED_ARGUMENT(WGL_HPBUFFERARB_MAP, HPBUFFERARB, NativeObjectTraits, CHPBUFFERARB)
DEFINE_MAPPED_ARGUMENT(WGL_HPBUFFEREXT_MAP, HPBUFFEREXT, NativeObjectTraits, CHPBUFFEREXT)

//egl
DEFINE_MAPPED_ARGUMENT(EGL_CONTEXT_MAP, EGLContext, NativeObjectTraits, CEGLContext)
DEFINE_MAPPED_ARGUMENT(EGL_DISPLAY_MAP, EGLDisplay, NativeObjectTraits, CEGLDisplay)
DEFINE_MAPPED_ARGUMENT(EGL_SURFACE_MAP, EGLSurface, NativeObjectTraits, CEGLSurface)
DEFINE_MAPPED_ARGUMENT(EGL_CLIENTBUFFER_MAP, EGLClientBuffer, NativeObjectTraits, CEGLClientBuffer)
DEFINE_MAPPED_ARGUMENT(EGL_NATIVEDISPLAYTYPE_MAP,
                       EGLNativeDisplayType,
                       ZeroNativeObjectTraits,
                       CEGLNativeDisplayType)
DEFINE_MAPPED_ARGUMENT(EGL_NATIVEPIXMAPTYPE_MAP,
                       EGLNativePixmapType,
                       NativeObjectTraits,
                       CEGLNativePixmapType)
DEFINE_MAPPED_ARGUMENT(EGL_NATIVEWINDOWTYPE_MAP,
                       EGLNativeWindowType,
                       NativeObjectTraits,
                       CEGLNativeWindowType)
DEFINE_MAPPED_ARGUMENT(EGL_IMAGE_KHR_MAP, EGLImageKHR, NativeObjectTraits, CEGLImageKHR)
DEFINE_MAPPED_ARGUMENT(EGL_SYNC_KHR_MAP, EGLSyncKHR, NativeObjectTraits, CEGLSyncKHR)

//glx
DEFINE_MAPPED_ARGUMENT(X_DISPLAY_MAP, Display*, NativeObjectTraits, CDisplayPtr)
DEFINE_MAPPED_ARGUMENT(X_VISUALINFO_MAP, XVisualInfo*, NativeObjectTraits, CXVisualInfoPtr)
DEFINE_MAPPED_ARGUMENT(GLX_FBCONFIG_MAP, GLXFBConfig, NativeObjectTraits, CGLXFBConfig)
DEFINE_MAPPED_ARGUMENT(GLX_CONTEXT_MAP, GLXContext, NativeObjectTraits, CGLXContext)
DEFINE_MAPPED_ARGUMENT(GLX_DRAWABLE_MAP, uint64_t, NativeObjectTraits, CGLXDrawable)

#undef DEFINE_MAPPED_ARGUMENT

struct current_program_tag_t {};
extern current_program_tag_t current_program_tag;

class CGLUniformLocation : public gits::CArgument {
  GLint program_;
  GLint location_;

public:
  struct RecorderProgramOverride {
    RecorderProgramOverride(GLint program) {
      CGLUniformLocation::ProgramOverride() = program;
    }
    ~RecorderProgramOverride() {
      CGLUniformLocation::ProgramOverride() = 0;
    }
  };

  CGLUniformLocation();
  CGLUniformLocation(current_program_tag_t, GLint location);
  CGLUniformLocation(GLint program, GLint location);

  virtual const char* Name() const {
    return "<uniform location>";
  }
  virtual bool Array() const {
    return false;
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  GLint operator*() const;

  static void AddMapping(GLint program, GLint location, GLint loc_size, GLint actual_location);
  static void RemoveMappings(GLint program);

private:
  typedef icl::interval_map<GLint, GLint, icl::partial_enricher> locations_interval_map_t;
  typedef std::unordered_map<GLint, locations_interval_map_t> program_locations_map_t;
  static program_locations_map_t& getLocationsMap();
  static GLint& ProgramOverride() {
    static GLint program;
    return program;
  }
};

class CGLUniformSubroutineLocation : public gits::CArgument {
  GLint program_;
  GLenum type_;
  GLint location_;

public:
  CGLUniformSubroutineLocation();
  CGLUniformSubroutineLocation(current_program_tag_t, GLenum type, GLint location);
  CGLUniformSubroutineLocation(GLint program, GLenum type, GLint location);

  virtual const char* Name() const {
    return "<uniform location>";
  }
  virtual bool Array() const {
    return false;
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  GLint operator*() const;

  static void AddMapping(
      GLint program, GLenum type, GLint location, GLint loc_size, GLint actual_location);

private:
  typedef icl::interval_map<GLint, GLint, icl::partial_enricher> locations_interval_map_t;
  typedef std::unordered_map<GLint, locations_interval_map_t> shader_locations_map_t;
  typedef std::unordered_map<GLint, shader_locations_map_t> program_locations_map_t;
  static program_locations_map_t& getShadersMap();
};

//
// Subroutine indices are used only in an array'ed parameter. This does not mix well
// with CArgument's so instead static functions of this class are used in place
// where mappings are required.
//
class CGLSubroutineIndex {
public:
  static void AddMapping(GLint program, GLenum type, GLuint index, GLuint actual_index);
  static GLuint GetMapping(GLint program, GLenum type, GLuint index);

private:
  typedef std::unordered_map<GLuint, GLuint> indices_map_t;
  typedef std::unordered_map<GLint, indices_map_t> shader_indices_map_t;
  typedef std::unordered_map<GLint, shader_indices_map_t> program_indices_map_t;
  static program_indices_map_t& getShadersMap();
};

class CGLUniformBlockIndex : public gits::CArgument {
  GLint program_;
  GLint index_;

public:
  CGLUniformBlockIndex();
  CGLUniformBlockIndex(GLint program, GLint index);

  virtual const char* Name() const {
    return "<uniform block index>";
  }
  virtual bool Array() const {
    return false;
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  GLint operator*() const;

  static void AddMapping(GLint program, GLint index, GLint actual_index);

private:
  typedef std::unordered_map<GLint, GLint> indices_map_t;
  typedef std::unordered_map<GLint, indices_map_t> program_indices_map_t;
  static program_indices_map_t& getProgramsMap();
};
class CGLStorageBlockIndex : public gits::CArgument {
  GLint program_;
  GLuint index_;

public:
  CGLStorageBlockIndex();
  CGLStorageBlockIndex(GLint program, GLuint index);

  virtual const char* Name() const {
    return "<storage block index>";
  }
  virtual bool Array() const {
    return false;
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  GLuint operator*() const;

  static void AddMapping(GLint program, GLuint index, GLuint actual_index);

private:
  typedef std::unordered_map<GLuint, GLuint> indices_map_t;
  typedef std::unordered_map<GLint, indices_map_t> program_indices_map_t;
  static program_indices_map_t& getProgramsMap();
};
} // namespace OpenGL
} // namespace gits
