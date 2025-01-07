// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsFunctions.h
*
* @brief Automatically generated declarations of OpenGL library simple function call wrappers.
*
*/

#pragma once

#include "gits.h"
#include "log.h"
#include "openglFunction.h"
#include "clientArrays.h"
#include "eglArguments.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "wglFunctions.h"

namespace gits {
namespace OpenGL {
/**
     * @brief OpenGL CgitsClientArraysUpdate() function call wrapper.
     *
     * OpenGL CgitsClientArraysUpdate() function call wrapper.
     */

class CgitsClientArraysUpdate : public CFunction {
  static const unsigned ARG_NUM = 1;
  ClientArraysUpdate _update;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  CgitsClientArraysUpdate() {}
  CgitsClientArraysUpdate(GLuint index) : _update(index) {}
  CgitsClientArraysUpdate(GLuint start, GLuint count, GLuint instances, GLuint baseinstance)
      : _update(start, count, instances, baseinstance) {}
  CgitsClientArraysUpdate(const GLsizei start[], const GLsizei count[], GLuint primcount)
      : _update(start, count, primcount) {} //for glMultiDrawArrays
  CgitsClientArraysUpdate(GLenum type,
                          const GLsizei* count,
                          const void* const* indices,
                          GLuint primcount)
      : _update(type, count, indices, primcount) {} //for glMultiDrawElements
  CgitsClientArraysUpdate(GLsizei count,
                          GLenum type,
                          const GLvoid* indices,
                          GLuint instances,
                          GLuint baseinstance,
                          GLuint basevertex)
      : _update(count, type, indices, instances, baseinstance, basevertex) {}
  virtual unsigned Id() const {
    return ID_GITS_CLIENT_ARRAYS_UPDATE;
  }
  virtual const char* Name() const {
    return "gitsClientArraysUpdate";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _update;
  }
  virtual void Run() {
    _update.Apply();
  }
};

/**
    * @brief OpenGL CgitsClientIndirectArraysUpdate() function call wrapper.
    *
    * OpenGL CgitsClientIndirectArraysUpdate() function call wrapper.
    */
class CgitsClientIndirectArraysUpdate : public CFunction {
  static const unsigned ARG_NUM = 0;
  std::vector<std::shared_ptr<ClientArraysUpdate>> _arraysUpdateVec;
  std::shared_ptr<CDataUpdate> _indirectUpdatePtr;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

  template <class T>
  std::vector<T> GetIndirectParams(const void* indirect, GLsizei drawcount, GLsizei stride);

public:
  CgitsClientIndirectArraysUpdate() {}
  // For gl[Multi]DrawArraysIndirect.
  CgitsClientIndirectArraysUpdate(GLenum mode,
                                  const void* indirect,
                                  GLsizei drawcount,
                                  GLsizei stride);
  // For gl[Multi]DrawElementsIndirect.
  CgitsClientIndirectArraysUpdate(
      GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride);
  // For glMultiDrawElementsIndirectCount.
  CgitsClientIndirectArraysUpdate(GLenum mode,
                                  GLenum type,
                                  const void* indirect,
                                  GLintptr drawcount,
                                  GLsizei maxdrawcount,
                                  GLsizei stride);

  virtual unsigned Id() const {
    return ID_GITS_CLIENT_INDIRECT_ARRAYS_UPDATE;
  }
  virtual const char* Name() const {
    return "gitsClientArraysIndirectUpdate";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

/**
    * @brief OpenGL glLinkProgram() function call wrapper.
    *
    * OpenGL glLinkProgram() function call wrapper.
    */

class CgitsLinkProgramAttribsSetting : public CFunction {
  static const unsigned ARG_NUM = 4;
  CGLProgram _program;
  CAttribsMap _attrib_locations;
  CAttribsMap _frag_data_locations;
  CAttribsMap _frag_data_locationsEXT;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  CgitsLinkProgramAttribsSetting() {}
  CgitsLinkProgramAttribsSetting(GLuint program);
  CgitsLinkProgramAttribsSetting(GLuint program, const CAttribsMap::map_t& attribsMap);
  virtual unsigned Id() const {
    return ID_GITS_LINK_PROGRAM_ATTRIBS_SETTINGS;
  }
  virtual const char* Name() const {
    return "gitsLinkProgramAttribsSetting";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

class CgitsLinkProgramBuffersSetting : public CFunction {
  static const unsigned ARG_NUM = 3;
  CGLProgram _program;
  CAttribsMap _uniform_blocks_bindings;
  CAttribsMap _storage_blocks_bindings;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  CgitsLinkProgramBuffersSetting() {}
  CgitsLinkProgramBuffersSetting(GLuint program);
  virtual unsigned Id() const {
    return ID_GITS_LINK_PROGRAM_BUFFERS_SETTINGS;
  }
  virtual const char* Name() const {
    return "gitsLinkProgramBuffersSetting";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

/**
     * @brief OpenGL glRenderbufferStorage() function call wrapper.
     *
     * OpenGL glRenderbufferStorage() function call wrapper.
     */

class CgitsRenderbufferStorage : public CFunction {
  static const unsigned ARG_NUM = 6;
  CGLenum _target;
  CGLenum _internalformat;
  CGLsizei _width;
  CGLsizei _height;
  CBinaryResource _resource;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  enum RenderbufferStorageType {
    RENDER_BUFFER,
    RENDER_BUFFER_EXT
  };
  CGLuint _renderbufferStorageType;
  void renderBufferStorageRun();
  void renderBufferStorageEXTRun();
  CgitsRenderbufferStorage() {}
  CgitsRenderbufferStorage(GLenum target,
                           GLenum internalformat,
                           GLsizei width,
                           GLsizei height,
                           hash_t hash,
                           RenderbufferStorageType renderbufferStorageType);
  virtual unsigned Id() const {
    return ID_GITS_RENDERBUFFER_STORAGE;
  }
  virtual const char* Name() const {
    return "gitsRenderbufferStorage";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const {}
  virtual void Run();
};

/**
     * @brief OpenGL glUnmapBuffer() function call wrapper.
     *
     * OpenGL glUnmapBuffer() function call wrapper.
     */

class CgitsUnmapBuffer : public CFunction {
  static const unsigned ARG_NUM = 5;
  CGLenum _target;
  CBinaryResource _resource;
  CGLintptr _offset;
  CGLsizeiptr _length;
  CGLint _buffer;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  CgitsUnmapBuffer() {}
  CgitsUnmapBuffer(GLenum target, GLint buffer = -1);

  virtual unsigned Id() const {
    return ID_GITS_UNMAP_BUFFER;
  }
  virtual const char* Name() const {
    return "gitsUnmapBuffer";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

/**
    * @brief OpenGL CgitsPersistentBufferMapping() function call wrapper.
    *
    * OpenGL CgitsPersistentBufferMapping() function call wrapper.
    */

class CgitsCoherentBufferMapping : public CFunction {
  static const unsigned ARG_NUM = 1;
  CCoherentBufferUpdate _data;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  CgitsCoherentBufferMapping() {}
  CgitsCoherentBufferMapping(CCoherentBufferUpdate::TCoherentBufferData::UpdateType updateType,
                             CCoherentBufferUpdate::TCoherentBufferData::UpdateMode updateMode,
                             bool oncePerFrame);
  virtual unsigned Id() const {
    return ID_GITS_COHERENT_BUFFER_MAPPING;
  }
  virtual const char* Name() const {
    return "gitsPersistentBufferMapping";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

/**
    * @brief OpenGL CUpdateMappedTexture() function call wrapper.
    *
    * OpenGL CUpdateMappedTexture() function call wrapper.
    */
class CUpdateMappedTexture : public CFunction {
  CGLTexture _texture;
  CGLuint _level;
  CBinaryResource _resource;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CUpdateMappedTexture();
  CUpdateMappedTexture(GLuint texture, GLuint level, hash_t, GLuint size);
  virtual unsigned Id() const {
    return ID_UPDATE_MAPPED_TEXTURE;
  }
  virtual const char* Name() const {
    return "UpdateMappedTexture";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Run();
  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief OpenGL glViewport() function call wrapper.
    *
    * OpenGL glViewport() function call wrapper.
    */
class CgitsViewportSettings : public CFunction {
  CHWND _hwnd;
  Cint::CSArray _winparams;
  CHWND::CSArray _hwnd_del_list;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 3;
  }

public:
  CgitsViewportSettings();
  CgitsViewportSettings(GLint x, GLint y, GLsizei width, GLsizei height);
  virtual unsigned Id() const {
    return ID_GITS_VIEWPORT_SETTINGS;
  }
  virtual const char* Name() const {
    return "gitsViewportSettings";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

/**
    * @brief Function wrapper for restoration of default framebuffer.
    *
    * Wrapper for process of restoration of default framebuffer.
    * Returns: void
    */

class CRestoreDefaultGLFramebuffer : public CFunction {
  CGLenum _dsInternalformat;
  CGLenum _dsFormat;
  CGLenum _dsType;
  CGLenum _dsAttachment;
  CGLenum _colorInternalformat;
  CGLenum _colorFormat;
  CGLenum _colorType;
  CGLuint _width;
  CGLuint _height;
  CBinaryResource _dsResource;
  CBinaryResource _colorResource;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return 11;
  }

public:
  CRestoreDefaultGLFramebuffer();
  CRestoreDefaultGLFramebuffer(GLenum dsInternalformat,
                               GLenum dsFormat,
                               GLenum dsType,
                               GLenum dsAttachment,
                               GLenum colorInternalformat,
                               GLenum colorFormat,
                               GLenum colorType,
                               GLsizei width,
                               GLsizei height,
                               hash_t dsDatahash,
                               hash_t colorDatahash);
  virtual unsigned Id() const {
    return ID_RESTORE_DEFAULT_GL_FRAMEBUFFER;
  }
  virtual const char* Name() const {
    return "CRestoreDefaultGLFramebuffer";
  }
  virtual unsigned Version() const {
    return VERSION_2_0;
  }

  virtual void Run();
  virtual void Write(CCodeOStream& stream) const;
};

/**
    * @brief OpenGL glFlushMappedBufferRange() function call wrapper.
    *
    * OpenGL glFlushMappedBufferRange() function call wrapper.
    */

class CgitsFlushMappedBufferRange : public CFunction {
  static const unsigned ARG_NUM = 4;
  CGLenum _target;
  CBinaryResource _resource;
  CGLintptr _offset;
  CGLsizeiptr _length;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }

public:
  CgitsFlushMappedBufferRange() {}
  CgitsFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);

  virtual unsigned Id() const {
    return ID_GITS_GL_FLUSH_MAPPED_BUFFER_RANGE;
  }
  virtual const char* Name() const {
    return "FlushMappedBufferRange";
  }
  virtual unsigned Version() const {
    return VERSION_1_1;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Run();
};

} // namespace OpenGL
} // namespace gits
