// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsFunctions.cpp
*
* @brief Automatically generated definitions of OpenGL library simple function call wrappers.
*
*/

#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "gitsFunctions.h"
#include "glFunctions.h"
#include "openglLibrary.h"
#include "stateDynamic.h"
#include "gits.h"
#include "exception.h"
#include "log.h"
#include "streams.h"
#include "wglFunctions.h"

/* ***************************** GITS CLIENT ARRAY UPDATE *************************** */

gits::CArgument& gits::OpenGL::CgitsClientArraysUpdate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _update);
}

/* ***************************** GITS CLIENT INDIRECT ARRAY UPDATE *************************** */
template <class T>
std::vector<T> gits::OpenGL::CgitsClientIndirectArraysUpdate::GetIndirectParams(
    const void* indirect, GLsizei drawcount, GLsizei stride) {
  // ES and Core contexts do not support client indirect arrays.
  // TODO: does it affect those entire calls or only client-side buffers? In the latter case, GPU-side buffers would be OK.
  bool isCoreCtx = SD().GetCurrentContextStateData().ProfileMask() & GL_CONTEXT_CORE_PROFILE_BIT;
  if (!curctx::IsOgl() || isCoreCtx) {
    return {};
  }

  GLint buff = 0;
  drv.gl.glGetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING, &buff);

  // Get pointer to indirect data from client side or mapped buffer.
  const void* ptr;
  std::shared_ptr<MapBuffer> buffMap;
  if (buff != 0) { // `indirect` is an offset into a buffer.
    // Check size, no point in updating a zero-sized buffer.
    GLint buffSize = 0;
    drv.gl.glGetBufferParameteriv(GL_DRAW_INDIRECT_BUFFER, GL_BUFFER_SIZE, &buffSize);
    if (buffSize == 0) {
      return {};
    }

    buffMap.reset(new MapBuffer(GL_DRAW_INDIRECT_BUFFER, buff));
    ptr = (GLvoid*)((uintptr_t)buffMap->Data() + (uintptr_t)indirect);
  } else { // `indirect` is a pointer to client memory.
    ptr = indirect;
  }

  // Dump indirect data from client side.
  if (buff == 0) {
    size_t size;
    if (stride == 0) {
      size = drawcount * sizeof(T);
    } else {
      size = drawcount * stride;
    }
    _indirectUpdatePtr.reset(new CDataUpdate);
    _indirectUpdatePtr->Diff((uint64_t)indirect, (uint64_t)indirect, size);
  }

  // Read indirect params to vector.
  std::vector<T> indirectDataVec;
  indirectDataVec.reserve(drawcount);
  if (stride == 0) {
    // It's just a normal array of structs, pointer counts as an iterator.
    const auto* structPtr = (const T*)ptr;
    indirectDataVec.assign(structPtr, structPtr + drawcount);
  } else {
    // The elements sit apart, we'll iterate them manually.
    const char* bytePtr;
    const T* structPtr;
    for (int i = 0; i < drawcount; i++) {
      bytePtr = (const char*)ptr + (stride * i);
      structPtr = (const T*)bytePtr;
      indirectDataVec.push_back(*structPtr);
    }
  }

  return indirectDataVec;
}

typedef struct {
  GLuint count;
  GLuint instanceCount;
  GLuint first;
  GLuint baseInstance;
} DrawArraysIndirectCommand;

gits::OpenGL::CgitsClientIndirectArraysUpdate::CgitsClientIndirectArraysUpdate(GLenum mode,
                                                                               const void* indirect,
                                                                               GLsizei drawcount,
                                                                               GLsizei stride) {
  auto indirectDataVec = GetIndirectParams<DrawArraysIndirectCommand>(indirect, drawcount, stride);

  // Update client arrays.
  for (auto& data : indirectDataVec) {
    auto cArrayUpdatePtr = std::make_shared<ClientArraysUpdate>(
        data.first, data.count, data.instanceCount, data.baseInstance);
    _arraysUpdateVec.push_back(cArrayUpdatePtr);
  }
}

typedef struct {
  GLuint count;
  GLuint instanceCount;
  GLuint firstIndex;
  GLint baseVertex;
  GLuint baseInstance;
} DrawElementsIndirectCommand;

gits::OpenGL::CgitsClientIndirectArraysUpdate::CgitsClientIndirectArraysUpdate(
    GLenum mode, GLenum type, const void* indirect, GLsizei drawcount, GLsizei stride) {
  auto indirectDataVec =
      GetIndirectParams<DrawElementsIndirectCommand>(indirect, drawcount, stride);

  // Update client arrays.
  for (auto& data : indirectDataVec) {
    auto cArrayUpdatePtr = std::make_shared<ClientArraysUpdate>(
        data.count, type, reinterpret_cast<void*>(data.firstIndex), data.instanceCount,
        data.baseInstance, data.baseVertex);
    _arraysUpdateVec.push_back(cArrayUpdatePtr);
  }
}

gits::OpenGL::CgitsClientIndirectArraysUpdate::CgitsClientIndirectArraysUpdate(GLenum mode,
                                                                               GLenum type,
                                                                               const void* indirect,
                                                                               GLintptr drawcount,
                                                                               GLsizei maxdrawcount,
                                                                               GLsizei stride) {
  // In MultiDrawElementsIndirectCount, drawcount is an offset into buffer
  // object bound to GL_PARAMETER_BUFFER, at which the actual draw count is
  // stored as a single GLsizei. If the stored draw count exceeds maxdrawcount,
  // maxdrawcount is used instead.
  GLsizei actualDrawCount;
// TODO: properly define this enum in enums file.
#define GL_PARAMETER_BUFFER 0x80EE
  drv.gl.glGetBufferSubData(GL_PARAMETER_BUFFER, drawcount, sizeof(GLsizei), &actualDrawCount);
  if (actualDrawCount > maxdrawcount) {
    actualDrawCount = maxdrawcount;
  }

  auto indirectDataVec =
      GetIndirectParams<DrawElementsIndirectCommand>(indirect, actualDrawCount, stride);

  // Update client arrays.
  for (auto& data : indirectDataVec) {
    auto cArrayUpdatePtr = std::make_shared<ClientArraysUpdate>(
        data.count, type, reinterpret_cast<void*>(data.firstIndex), data.instanceCount,
        data.baseInstance, data.baseVertex);
    _arraysUpdateVec.push_back(cArrayUpdatePtr);
  }
}

gits::CArgument& gits::OpenGL::CgitsClientIndirectArraysUpdate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx);
}

void gits::OpenGL::CgitsClientIndirectArraysUpdate::Write(CBinOStream& stream) const {
  //Write indirect client data
  CGLboolean hasIndirect(_indirectUpdatePtr.use_count() > 0);
  stream << hasIndirect;
  if (*hasIndirect > 0) {
    stream << _indirectUpdatePtr;
  }

  //Write attribs client data
  CGLuint count((GLuint)_arraysUpdateVec.size());
  stream << count;
  for (auto& update : _arraysUpdateVec) {
    stream << *update;
  }
}

void gits::OpenGL::CgitsClientIndirectArraysUpdate::Read(CBinIStream& stream) {
  //Read indirect client data
  CGLboolean hasIndirect;
  stream >> hasIndirect;
  if (*hasIndirect > 0) {
    _indirectUpdatePtr.reset(new CDataUpdate);
    stream >> *_indirectUpdatePtr;
  }

  //Read attribs client data
  CGLuint count;
  stream >> count;
  for (unsigned i = 0; i < *count; i++) {
    _arraysUpdateVec.push_back(std::shared_ptr<ClientArraysUpdate>(new ClientArraysUpdate()));
    stream >> *(_arraysUpdateVec.back());
  }
}

void gits::OpenGL::CgitsClientIndirectArraysUpdate::Write(CCodeOStream& stream) const {
  //Write indirect client data
  if (_indirectUpdatePtr.use_count() > 0) {
    stream << *_indirectUpdatePtr;
  }

  //Write attribs client data
  for (auto& update : _arraysUpdateVec) {
    stream << *update;
  }
}

void gits::OpenGL::CgitsClientIndirectArraysUpdate::Run() {
  //Apply indirect client data
  if (_indirectUpdatePtr.use_count() > 0) {
    _indirectUpdatePtr->Apply();
  }

  //Apply attribs client data
  for (auto& update : _arraysUpdateVec) {
    update->Apply();
  }
}

/* ***************************** GITS LINK PROGRAM ATTRIBS SETTING ****************** */

gits::CArgument& gits::OpenGL::CgitsLinkProgramAttribsSetting::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _program, _attrib_locations, _frag_data_locations,
                       _frag_data_locationsEXT);
}

gits::OpenGL::CgitsLinkProgramAttribsSetting::CgitsLinkProgramAttribsSetting(GLuint program)
    : _program(program) {
  // This needs to be called _after_ glLinkProgram
  GLint status = 0;
  drv.gl.glGetProgramiv(program, GL_LINK_STATUS, &status);

  CGLSLProgramStateObj* progStateObjPtr =
      SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
  if (progStateObjPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  // We managed to link correctly, aquire attributes locations
  if (status != 0) {
    auto& attribs = progStateObjPtr->Attributes();
    attribs.clear();

    GLint active_attribs = 0;
    drv.gl.glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active_attribs);

    GLint max_length = 0;
    drv.gl.glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);

    if (max_length != 0) {
      for (int i = 0; i < active_attribs; ++i) {
        GLint size_unused;
        GLenum type_unused;
        std::vector<char> name(max_length);
        drv.gl.glGetActiveAttrib(program, i, max_length, nullptr, &size_unused, &type_unused,
                                 (char*)&name.at(0));

        GLint location = drv.gl.glGetAttribLocation(program, (GLchar*)&name[0]);

        _attrib_locations.Map()[name.data()] = location;
        attribs.insert(location);
      }
    }

    auto& fragDataLocations = progStateObjPtr->Data().track.fragDataLocationBindings;
    for (auto& location : fragDataLocations) {
      _frag_data_locations.Map()[location.second] = location.first;
    }
  }
}

gits::OpenGL::CgitsLinkProgramAttribsSetting::CgitsLinkProgramAttribsSetting(
    GLuint program, const CAttribsMap::map_t& attribsMap)
    : _program(program), _attrib_locations(attribsMap) {
  CGLSLProgramStateObj* progStateObjPtr =
      SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
  if (progStateObjPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto& attribs = progStateObjPtr->Attributes();
  attribs.clear();
  for (auto& attribElem : attribsMap) {
    attribs.insert(attribElem.second);
  }

  auto& fragDataLocations = progStateObjPtr->Data().track.fragDataLocationBindings;
  for (auto& location : fragDataLocations) {
    _frag_data_locations.Map()[location.second] = location.first;
  }
}

void gits::OpenGL::CgitsLinkProgramAttribsSetting::Run() {
  if (!Config::Get().player.linkUseProgBinary) {
    GLint program = *_program;

    // Vertex attributes
    for (const auto& location : _attrib_locations.Map()) {
      if (strncmp(location.first.c_str(), "gl_", 3) != 0) {
        drv.gl.glBindAttribLocation(program, location.second, (GLchar*)location.first.c_str());
      }
    }

    // Fragment outputs
    for (const auto& location : _frag_data_locations.Map()) {
      drv.gl.glBindFragDataLocation(program, location.second, (GLchar*)location.first.c_str());
    }
  }
}

void gits::OpenGL::CgitsLinkProgramAttribsSetting::Write(CCodeOStream& stream) const {
  stream.select(stream.selectCCodeFile());
  for (const auto& location : _attrib_locations.Map()) {
    if (strncmp(location.first.c_str(), "gl_", 3) != 0) {
      gits::OpenGL::CglBindAttribLocation(_program.Original(), location.second,
                                          (GLchar*)location.first.c_str())
          .Write(stream);
    }
  }
  for (const auto& location : _frag_data_locations.Map()) {
    gits::OpenGL::CglBindFragDataLocation(_program.Original(), location.second,
                                          (GLchar*)location.first.c_str())
        .Write(stream);
  }
}

/* ***************************** GITS LINK PROGRAM BUFFERS SETTING ****************** */

namespace {
void StoreProgramBlockBindingsMapValues(GLuint program,
                                        GLenum programInterface,
                                        std::unordered_map<std::string, GLint>& blockBindingsMap) {
  GLint active_blocks = 0;
  gits::OpenGL::drv.gl.glGetProgramInterfaceiv(program, programInterface, GL_ACTIVE_RESOURCES,
                                               &active_blocks);

  if (active_blocks > 0) {
    GLint max_name_length = 0;
    gits::OpenGL::drv.gl.glGetProgramInterfaceiv(program, programInterface, GL_MAX_NAME_LENGTH,
                                                 &max_name_length);

    for (int i = 0; i < active_blocks; ++i) {
      std::vector<GLchar> name(max_name_length + 100);
      gits::OpenGL::drv.gl.glGetProgramResourceName(program, programInterface, i,
                                                    max_name_length + 100, nullptr, name.data());

      GLenum property = GL_BUFFER_BINDING;
      GLint binding = 0;
      gits::OpenGL::drv.gl.glGetProgramResourceiv(program, programInterface, i, 1, &property, 1,
                                                  nullptr, &binding);

      blockBindingsMap[std::string(name.data())] = binding;
    }
  }
}

void SetProgramBlockBindings(GLuint program,
                             GLenum programInterface,
                             std::unordered_map<std::string, GLint>& blockBindingsMap,
                             void_t(STDCALL* setter)(GLuint, GLuint, GLuint)) {
  GLint active_blocks = 0;
  gits::OpenGL::drv.gl.glGetProgramInterfaceiv(program, programInterface, GL_ACTIVE_RESOURCES,
                                               &active_blocks);

  if (active_blocks > 0) {
    GLint max_name_length = 0;
    gits::OpenGL::drv.gl.glGetProgramInterfaceiv(program, programInterface, GL_MAX_NAME_LENGTH,
                                                 &max_name_length);

    for (int i = 0; i < active_blocks; ++i) {
      std::vector<GLchar> name(max_name_length + 100);
      gits::OpenGL::drv.gl.glGetProgramResourceName(program, programInterface, i,
                                                    max_name_length + 100, nullptr, name.data());
      std::string name_string(name.data());

      if (blockBindingsMap.end() != blockBindingsMap.find(name_string)) {
        setter(program, i, blockBindingsMap[name_string]);
      }
    }
  }
}
} // namespace

gits::CArgument& gits::OpenGL::CgitsLinkProgramBuffersSetting::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _program, _uniform_blocks_bindings,
                       _storage_blocks_bindings);
}

gits::OpenGL::CgitsLinkProgramBuffersSetting::CgitsLinkProgramBuffersSetting(GLuint program)
    : _program(program) {
  // This needs to be called AFTER glLinkProgram()
  GLint status = 0;
  drv.gl.glGetProgramiv(program, GL_LINK_STATUS, &status);

  CGLSLProgramStateObj* progStateObjPtr =
      SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
  if (progStateObjPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  // We managed to link correctly, aquire uniform and storage block bindings
  if (status != 0) {
    // Uniform block bindings
    StoreProgramBlockBindingsMapValues(program, GL_UNIFORM_BLOCK, _uniform_blocks_bindings.Map());
    // Storage block bindings
    StoreProgramBlockBindingsMapValues(program, GL_SHADER_STORAGE_BLOCK,
                                       _storage_blocks_bindings.Map());
  }
}

void gits::OpenGL::CgitsLinkProgramBuffersSetting::Run() {
  // This needs to be called AFTER glLinkProgram() !!!
  if (!Config::Get().player.linkUseProgBinary) {
    // Uniform block bindings
    SetProgramBlockBindings(*_program, GL_UNIFORM_BLOCK, _uniform_blocks_bindings.Map(),
                            drv.gl.glUniformBlockBinding);
    // Storage block bindings
    SetProgramBlockBindings(*_program, GL_SHADER_STORAGE_BLOCK, _storage_blocks_bindings.Map(),
                            drv.gl.glShaderStorageBlockBinding);
  }
}

void gits::OpenGL::CgitsLinkProgramBuffersSetting::Write(CCodeOStream& stream) const {
  stream << "// Restoration of uniform buffers and shader blocks bindings is not yet implemented "
            "in gits CCode.\n";
}

/* ********************************** CgitsRenderbufferStorage ********************* */

gits::CArgument& gits::OpenGL::CgitsRenderbufferStorage::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _target, _internalformat, _width, _height, _resource,
                       _renderbufferStorageType);
}

gits::OpenGL::CgitsRenderbufferStorage::CgitsRenderbufferStorage(
    GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    hash_t hash,
    RenderbufferStorageType renderbufferStorageType)
    : _target(target),
      _internalformat(internalformat),
      _width(width),
      _height(height),
      _resource(hash),
      _renderbufferStorageType(renderbufferStorageType) {}

void gits::OpenGL::CgitsRenderbufferStorage::Run() {
  if (*_renderbufferStorageType == RENDER_BUFFER) {
    renderBufferStorageRun();
  } else if (*_renderbufferStorageType == RENDER_BUFFER_EXT) {
    renderBufferStorageEXTRun();
  }
}

void gits::OpenGL::CgitsRenderbufferStorage::renderBufferStorageRun() {
  if (*_width == 0 || *_height == 0) {
    return;
  }

  if (!CGits::Instance().IsStateRestoration()) {
    return;
  }

  GLenum tmpFboAttachment = InternalFormatToFboMatch(*_internalformat);

  GLint targetRBO;
  drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING, &targetRBO);

  GLuint targetFBO;
  drv.gl.glGenFramebuffers(1, &targetFBO);
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
  drv.gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, tmpFboAttachment, GL_RENDERBUFFER, targetRBO);
  if (tmpFboAttachment != GL_COLOR_ATTACHMENT0 && curctx::IsOgl()) {
    drv.gl.glDrawBuffer(GL_NONE);
    drv.gl.glReadBuffer(GL_NONE);
  }
  GLenum fboStatus = drv.gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    Log(ERR)
        << "Renderbuffer restoration failed (targetFBO) - Framebuffer incomplete - FBO status: "
        << fboStatus;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  if (curctx::IsOgl() && _resource.GetResourceHash() != 0) {
    RestoreFramebuffer(targetFBO, *_internalformat, *_height, *_width, _resource);
  } else {
    drv.gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    gits::CGits::Instance().DrawCountUp();
  }

  drv.gl.glDeleteFramebuffers(1, &targetFBO);
}

void gits::OpenGL::CgitsRenderbufferStorage::renderBufferStorageEXTRun() {
  //_resource argument is non-zero initialized only for state restoration tokens from versions newer then GITS_FILE_COMPAT_RBO_RESTORE
  if (_resource.GetResourceHash() != 0) {
    //get current RBO and attach to FBO
    GLenum tmpFboAttachment = InternalFormatToFboMatch(*_internalformat);

    GLint targetRBO;
    drv.gl.glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &targetRBO);

    GLuint targetFBO;
    drv.gl.glGenFramebuffersEXT(1, &targetFBO);
    drv.gl.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, targetFBO);
    drv.gl.glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, tmpFboAttachment, GL_RENDERBUFFER,
                                        targetRBO);
    if (tmpFboAttachment != GL_COLOR_ATTACHMENT0) {
      drv.gl.glDrawBuffer(GL_NONE);
      drv.gl.glReadBuffer(GL_NONE);
    }
    GLenum fboStatus = drv.gl.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
      Log(ERR)
          << "Renderbuffer restoration failed (targetFBO) - Framebuffer incomplete - FBO status: "
          << fboStatus;
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    RestoreFramebufferEXT(targetFBO, *_internalformat, *_height, *_width, _resource);

    drv.gl.glDeleteFramebuffersEXT(1, &targetFBO);
  }
}

/* ********************************** CgitsUnmapBuffer ********************* */

gits::CArgument& gits::OpenGL::CgitsUnmapBuffer::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _target, _resource, _offset, _length, _buffer);
}

gits::OpenGL::CgitsUnmapBuffer::CgitsUnmapBuffer(GLenum target, GLint buffer)
    : _target(target), _buffer(buffer) {
  bool named_buffer = true;
  if (buffer == -1) {
    named_buffer = false;
    buffer = SD().GetCurrentContextStateData().Bindings().BoundBuffer(target);
  }

  if (buffer == 0) {
    Log(WARN) << "CgitsUnmapBuffer: unmapping buffer zero. Ignoring call.";
    return;
    // Workaround for Outerra engine.
  } else if (SD().GetCurrentSharedStateData().Buffers().Get(buffer) == nullptr) {
    Log(WARN) << "CgitsUnmapBuffer: unmaping unknown buffer object. Ignoring call.";
    return;
    // Workaround for Outerra engine.
  }

  GLint size = 0;
  auto& mapping = SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().restore;
  GLint access = mapping.mapAccess;
  GLint length = mapping.mapLength;
  GLint offset = mapping.mapOffset;
  CBufferStateData::Restored::buffer_type type = mapping.type;

  auto func_map = _glMapBuffer_wrap;
  auto func_unmap = _glUnmapBuffer_wrap;
  auto func_get = _glGetBufferParameteriv_wrap;

  if (named_buffer == false) {
    if (type == CBufferStateData::Restored::MAP_BUFFER_ARB) {
      func_map = _glMapBufferARB_wrap;
      func_unmap = _glUnmapBufferARB_wrap;
    } else if (type == CBufferStateData::Restored::MAP_BUFFER_OES) {
      func_map = _glMapBufferOES_wrap;
      func_unmap = _glUnmapBufferOES_wrap;
    }
  } else {
    if (type == CBufferStateData::Restored::MAP_BUFFER_EXT) {
      func_map = _glMapNamedBufferEXT_wrap;
      func_unmap = _glUnmapNamedBufferEXT_wrap;
      func_get = _glGetNamedBufferParameterivEXT_wrap;
    }
  }

  GLvoid* pointer = nullptr;
  if (length == -1 && offset == -1) {
    if (!(access & GL_MAP_READ_BIT)) {
      // Unmapping after glMapBuffer
      auto access_type = GL_READ_ONLY;
      if ((curctx::IsEs1() || curctx::IsEs2Plus()) &&
          ESBufferState() != TBuffersState::BUFFERS_STATE_CAPTURE_ALWAYS) {
        // buffer is remapped with write permissions again, as we can read write-only mapping - read it, and save
        access_type = GL_WRITE_ONLY;
      } else {
        // buffer is remapped with read permissions now - read it, and save
        access_type = GL_READ_ONLY;
      }

      func_unmap(buffer, target);
      func_get(buffer, target, GL_BUFFER_SIZE, &size);
      pointer = func_map(buffer, target, access_type);
    }
  } else {
    if (!(access & GL_MAP_READ_BIT)) {
      // Unmapping after glMapBufferRange
      // here buffer is remapped with read permissions now - read it, and save
      func_unmap(buffer, target);
      if (named_buffer == false) {
        pointer = drv.gl.glMapBufferRange(target, offset, length, GL_MAP_READ_BIT);
      } else {
        pointer = drv.gl.glMapNamedBufferRangeEXT(buffer, offset, length, GL_MAP_READ_BIT);
      }
    }
  }
  if (pointer == nullptr) {
    auto func_get_buffer_pointer = _glGetBufferPointerv_wrap;
    if (named_buffer == false) {
      if (type == CBufferStateData::Restored::MAP_BUFFER_ARB) {
        func_get_buffer_pointer = _glGetBufferPointervARB_wrap;
      } else if (type == CBufferStateData::Restored::MAP_BUFFER_OES) {
        func_get_buffer_pointer = _glGetBufferPointervOES_wrap;
      }
    } else {
      if (type == CBufferStateData::Restored::MAP_BUFFER_EXT) {
        func_get_buffer_pointer = _glGetNamedBufferPointervEXT_wrap;
      }
    }

    func_get_buffer_pointer(buffer, target, GL_BUFFER_MAP_POINTER, &pointer);
  }

  // Populate _resource if data was changed.
  if (pointer != nullptr) {
    auto* bufferDataPtr = SD().GetCurrentSharedStateData().Buffers().Get(buffer);
    if (bufferDataPtr == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }

    // Optimized size of mapped buffer data dump
    if (Config::Get().recorder.openGL.utilities.optimizeBufferSize) {
      // Access to mapped memory is multiple times slower than access to cpu
      // memory, so it is better to read it only once. Though temporary copy of
      // mapped memory is created there.
      auto mapMemSrc = pointer;
      GLsizeiptr tmpMapMemCpySize = 0;
      if (bufferDataPtr->Data().restore.mapLength == -1) {
        tmpMapMemCpySize = bufferDataPtr->Data().track.size;
      } else {
        tmpMapMemCpySize = bufferDataPtr->Data().restore.mapLength;
      }
      void* tmpMapMemCpy = malloc(tmpMapMemCpySize);
      void* tmpMapMemCpyDst = tmpMapMemCpy;
      // If map range flushing is used, only flushed part of memory is being
      // copied to improve recorder performance.
      if (bufferDataPtr->Data().restore.mapFlushRangeLength > 0) {
        mapMemSrc = (char*)mapMemSrc + bufferDataPtr->Data().restore.mapFlushRangeOffset;
        tmpMapMemCpyDst = (char*)tmpMapMemCpy + bufferDataPtr->Data().restore.mapFlushRangeOffset;
        tmpMapMemCpySize = bufferDataPtr->Data().restore.mapFlushRangeLength;
      }
      memcpy(tmpMapMemCpyDst, mapMemSrc, tmpMapMemCpySize);

      GLintptr mapOffset = 0;
      GLintptr buffOffset = 0;
      GLsizeiptr mapSize = 0;
      bufferDataPtr->CalculateMapChange(mapOffset, buffOffset, mapSize, tmpMapMemCpy);
      bufferDataPtr->TrackBufferData(buffOffset, mapSize, (char*)tmpMapMemCpy + mapOffset);

      if (mapSize != 0) {
        _resource.reset(RESOURCE_BUFFER, ((GLubyte*)tmpMapMemCpy) + mapOffset, mapSize);
      }
      free(tmpMapMemCpy);

      *_offset = (int)mapOffset;
      *_length = (int)mapSize;
    }
    // Entire mapped buffer data dump
    else {
      if (bufferDataPtr->Data().restore.mapLength == -1) {
        *_length = bufferDataPtr->Data().track.size;
      } else {
        *_length = bufferDataPtr->Data().restore.mapLength;
      }

      *_offset = 0;
      if (*_length != 0) {
        _resource.reset(RESOURCE_BUFFER, (GLubyte*)pointer, *_length);
      }
    }
  }
}

void gits::OpenGL::CgitsUnmapBuffer::Run() {
  if (*_resource) {
    //we read some data - therefore buffer was originally mapped with write acces, so here we can just overwrite its data
    GLvoid* pointer;
    GLint recBuff = *_buffer;
    GLint playBuff;
    if (recBuff == -1) {
      playBuff = SD().GetCurrentContextStateData().Bindings().BoundBuffer(*_target);

      auto func_get_buffer_pointer = _glGetBufferPointerv_wrap;
      CBufferStateData::Restored::buffer_type type =
          SD().GetCurrentSharedStateData().Buffers().Get(playBuff)->Data().restore.type;
      if (type == CBufferStateData::Restored::MAP_BUFFER_ARB) {
        func_get_buffer_pointer = _glGetBufferPointervARB_wrap;
      } else if (type == CBufferStateData::Restored::MAP_BUFFER_OES) {
        func_get_buffer_pointer = _glGetBufferPointervOES_wrap;
      }

      func_get_buffer_pointer(*_buffer, *_target, GL_BUFFER_MAP_POINTER, &pointer);
    } else {
      playBuff = CGLBuffer::GetMapping(*_buffer);
      drv.gl.glGetNamedBufferPointervEXT(playBuff, GL_BUFFER_MAP_POINTER, &pointer);
    }

    if (playBuff == 0) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unmapping buffer zero");
    } else if (SD().GetCurrentSharedStateData().Buffers().Get(playBuff) == nullptr) {
      throw EOperationFailed((std::string)EXCEPTION_MESSAGE + " unknown buffer object");
    }
    if (!SD().GetCurrentSharedStateData().Buffers().Get(playBuff)->Data().restore.mapped) {
      Log(WARN) << "Unmapping unmapped buffer";
    } else {
      GLint size =
          SD().GetCurrentSharedStateData().Buffers().Get(playBuff)->Data().restore.mapLength;
      if (size == -1) {
        size = SD().GetCurrentSharedStateData().Buffers().Get(playBuff)->Size();
      }
      std::memcpy(((GLubyte*)pointer) + *_offset, *_resource, *_length);
    }
  }
}

void gits::OpenGL::CgitsUnmapBuffer::Write(CCodeOStream& stream) const {
  stream << "{\nGLvoid *pointer;" << std::endl;
  if (*_buffer == -1) {
    stream << "glGetBufferPointerv(" << _target << ", GL_BUFFER_MAP_POINTER, &pointer);"
           << std::endl;
  } else {
    stream << "glGetNamedBufferPointervEXT(" << _buffer << ", GL_BUFFER_MAP_POINTER, &pointer);"
           << std::endl;
  }

  stream << "std::memcpy((GLubyte*)pointer + " << _offset << "," << _resource << "," << _length
         << ");" << std::endl;
  stream << "}" << std::endl;
}

/* ********************************** CgitsPersistentBufferMapping ********************* */

gits::CArgument& gits::OpenGL::CgitsCoherentBufferMapping::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _data);
}

gits::OpenGL::CgitsCoherentBufferMapping::CgitsCoherentBufferMapping(
    CCoherentBufferUpdate::TCoherentBufferData::UpdateType updateType,
    CCoherentBufferUpdate::TCoherentBufferData::UpdateMode updateMode,
    bool oncePerFrame) {
  _data.Diff(updateType, updateMode, oncePerFrame);
}

void gits::OpenGL::CgitsCoherentBufferMapping::Run() {
  _data.Apply();
}

void gits::OpenGL::CgitsCoherentBufferMapping::Write(CCodeOStream& stream) const {
  _data.Write(stream);
}

/* ***************************** ID_UPDATE_MAPPED_TEXTURE *************************** */

gits::OpenGL::CUpdateMappedTexture::CUpdateMappedTexture() {}

gits::OpenGL::CUpdateMappedTexture::CUpdateMappedTexture(unsigned texture,
                                                         unsigned level,
                                                         hash_t hash,
                                                         unsigned size)
    : _texture(texture), _level(level), _resource(hash) {}

gits::CArgument& gits::OpenGL::CUpdateMappedTexture::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _texture, _level, _resource);
}

void gits::OpenGL::CUpdateMappedTexture::Run() {
  SD().GetCurrentSharedStateData().GetMappedTextures().UpdateTexture(*_texture, *_level,
                                                                     _resource.Data());
}

void gits::OpenGL::CUpdateMappedTexture::Write(CCodeOStream& stream) const {
  TResourceHandle2 rHandle =
      gits::CGits::Instance().ResourceManager2().get_resource_handle(_resource.GetResourceHash());
  stream.select(stream.selectCCodeFile());
  stream.Indent() << "CMappedTextures::getInstance().UpdateTexture( " << _texture << ", " << *_level
                  << ", " << rHandle.offsetToStart << ", " << rHandle.offsetInsideChunk << ");"
                  << std::endl;
}

/* ***************************** ID_GITS_VIEWPORT_SETTINGS *************************** */
gits::OpenGL::CgitsViewportSettings::CgitsViewportSettings() {}

gits::OpenGL::CgitsViewportSettings::CgitsViewportSettings(GLint x,
                                                           GLint y,
                                                           GLsizei width,
                                                           GLsizei height)
    : _hwnd(0) {
#if defined GITS_PLATFORM_WINDOWS
  if (SD().GetCurrentContext() == 0) {
    return;
  }

  if (curctx::IsOgl()) {
    auto hwnd = WindowFromDC(drv.wgl.wglGetCurrentDC());
    UpdateWindowsRec(hwnd, _winparams, _hwnd_del_list);
    _hwnd.Reset(hwnd);
  }
#endif
}

void gits::OpenGL::CgitsViewportSettings::Run() {
#if defined GITS_PLATFORM_WINDOWS
  UpdateWindows(_hwnd, _winparams, _hwnd_del_list);
#endif
}

void gits::OpenGL::CgitsViewportSettings::Write(CCodeOStream& stream) const {
  stream.select(stream.selectCCodeFile()) << std::endl;
#if defined GITS_PLATFORM_WINDOWS
  stream.Indent() << "{\n";
  stream.ScopeBegin();

  _winparams.VariableNameRegister(stream, false);
  _winparams.Declare(stream);
  _hwnd_del_list.VariableNameRegister(stream, false);
  _hwnd_del_list.Declare(stream);
  stream.Indent() << "UpdateWindows_( " << _hwnd << ", " << _winparams << ", "
                  << _winparams.Vector().size() << ", " << _hwnd_del_list << ", "
                  << _hwnd_del_list.Vector().size() << ");\n";

  stream.ScopeEnd();
  stream.Indent() << "}\n";
#endif
}

gits::CArgument& gits::OpenGL::CgitsViewportSettings::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _hwnd, _winparams, _hwnd_del_list);
}

/* ***************************** RESTORE_DEFAULT_GL_FRAMEBUFFER *************************** */

gits::OpenGL::CRestoreDefaultGLFramebuffer::CRestoreDefaultGLFramebuffer() {}

gits::OpenGL::CRestoreDefaultGLFramebuffer::CRestoreDefaultGLFramebuffer(GLenum dsInternalformat,
                                                                         GLenum dsFormat,
                                                                         GLenum dsType,
                                                                         GLenum dsAttachment,
                                                                         GLenum colorInternalformat,
                                                                         GLenum colorFormat,
                                                                         GLenum colorType,
                                                                         GLsizei width,
                                                                         GLsizei height,
                                                                         hash_t dsDatahash,
                                                                         hash_t colorDatahash)
    : _dsInternalformat(dsInternalformat),
      _dsFormat(dsFormat),
      _dsType(dsType),
      _dsAttachment(dsAttachment),
      _colorInternalformat(colorInternalformat),
      _colorFormat(colorFormat),
      _colorType(colorType),
      _width(width),
      _height(height),
      _dsResource(dsDatahash),
      _colorResource(colorDatahash) {}

gits::CArgument& gits::OpenGL::CRestoreDefaultGLFramebuffer::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _dsInternalformat, _dsFormat, _dsType, _dsAttachment,
                       _colorInternalformat, _colorFormat, _colorType, _width, _height, _dsResource,
                       _colorResource);
}

void gits::OpenGL::CRestoreDefaultGLFramebuffer::Run() {
  // Save current state (Read FBO, Draw FBO).
  GLint boundDrawFramebuffer = 0;
  GLint boundReadFramebuffer = 0;
  if (!drv.gl.HasExtension("GL_framebuffer_blit")) {
    drv.gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &boundDrawFramebuffer);
  } else {
    drv.gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &boundDrawFramebuffer);
    drv.gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &boundReadFramebuffer);
  }

  // Prepare data to be loaded into FBO 0.
  GLuint sourceFrameBuffer;
  drv.gl.glGenFramebuffers(1, &sourceFrameBuffer);
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, sourceFrameBuffer);
  GLuint ColorTexture;
  drv.gl.glGenTextures(1, &ColorTexture);
  drv.gl.glBindTexture(GL_TEXTURE_2D, ColorTexture);
  drv.gl.glTexImage2D(GL_TEXTURE_2D, 0, *_colorInternalformat, *_width, *_height, 0, *_colorFormat,
                      *_colorType, _colorResource.Data());
  GLuint DSTexture;
  drv.gl.glGenTextures(1, &DSTexture);
  drv.gl.glBindTexture(GL_TEXTURE_2D, DSTexture);
  drv.gl.glTexImage2D(GL_TEXTURE_2D, 0, *_dsInternalformat, *_width, *_height, 0, *_dsFormat,
                      *_dsType, _dsResource.Data());
  drv.gl.glFramebufferTexture2D(GL_FRAMEBUFFER, *_dsAttachment, GL_TEXTURE_2D, DSTexture, 0);
  drv.gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ColorTexture,
                                0);

  GLenum fboStatus = drv.gl.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
    Log(ERR) << "Renderbuffer restoration failed (sourceFrameBuffer) (GLColor) - Framebuffer "
                "incomplete - FBO status: "
             << fboStatus;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  // Blit operation.
  drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
  drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFrameBuffer);
  drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  drv.gl.glBlitFramebuffer(0, 0, *_width, *_height, 0, 0, *_width, *_height,
                           GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                           GL_NEAREST);
  CGits::Instance().DrawCountUp();

  // Clean OpenGL objects.
  drv.gl.glDeleteFramebuffers(1, &sourceFrameBuffer);
  drv.gl.glDeleteTextures(1, &DSTexture);
  drv.gl.glDeleteTextures(1, &ColorTexture);

  // Restore the state we saved (Read FBO, Draw FBO).
  if (!drv.gl.HasExtension("GL_framebuffer_blit")) {
    drv.gl.glBindFramebuffer(GL_FRAMEBUFFER, boundDrawFramebuffer);
  } else {
    drv.gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, boundDrawFramebuffer);
    drv.gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, boundReadFramebuffer);
  }
}

void gits::OpenGL::CRestoreDefaultGLFramebuffer::Write(CCodeOStream& stream) const {
  stream << "// Restoration of default framebuffer is not yet implemented in gits CCode.\n";
}

/* ********************************** CgitsFlushMappedBufferRange ********************* */

gits::CArgument& gits::OpenGL::CgitsFlushMappedBufferRange::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _target, _resource, _offset, _length);
}

gits::OpenGL::CgitsFlushMappedBufferRange::CgitsFlushMappedBufferRange(GLenum target,
                                                                       GLintptr offset,
                                                                       GLsizeiptr length)
    : _target(target), _offset(offset), _length(length) {

  GLint buffer = SD().GetCurrentContextStateData().Bindings().BoundBuffer(target);

  GLbitfield flags = SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().track.flags;

  auto& mapping = SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().restore;
  GLbitfield access = mapping.mapAccess;
  GLint length_bufferrange = mapping.mapLength;
  GLint offset_bufferrange = mapping.mapOffset;
  GLvoid* pointer = nullptr;
  if ((flags & GL_MAP_PERSISTENT_BIT) && (access & GL_MAP_PERSISTENT_BIT)) {
    if (!(access & GL_MAP_READ_BIT)) {
      //Unmapping after glMapBufferRange
      //here buffer is remapped with read permissions now - read it, and save
      drv.gl.glUnmapBuffer(target);
      pointer = drv.gl.glMapBufferRange(target, offset_bufferrange, length_bufferrange,
                                        access | GL_MAP_READ_BIT);
    }
    if (pointer == nullptr) {
      drv.gl.glGetBufferPointerv(target, GL_BUFFER_MAP_POINTER, &pointer);
    }
    if (pointer != nullptr && length > 0) {
      _resource.reset(RESOURCE_BUFFER, ((GLubyte*)pointer) + offset, length);
    }
  }
}

void gits::OpenGL::CgitsFlushMappedBufferRange::Run() {
  if (*_resource) {
    //we read some data - therefore buffer was originally mapped with write access, so here we can just overwrite its data
    GLint buffer;
    GLvoid* pointer;

    buffer = SD().GetCurrentContextStateData().Bindings().BoundBuffer(*_target);

    drv.gl.glGetBufferPointerv(*_target, GL_BUFFER_MAP_POINTER, &pointer);

    if (!SD().GetCurrentSharedStateData().Buffers().Get(buffer)->Data().restore.mapped) {
      Log(WARN) << "Flushing unmapped buffer";
    }

    std::memcpy(((GLubyte*)pointer) + *_offset, *_resource, *_length);
  }
}

void gits::OpenGL::CgitsFlushMappedBufferRange::Write(CCodeOStream& stream) const {
  if (_resource.GetResourceHash() != CResourceManager::EmptyHash) {
    stream << "{\nGLvoid *pointer;" << std::endl;
    stream << "glGetBufferPointerv(" << _target << ", GL_BUFFER_MAP_POINTER, &pointer);"
           << std::endl;

    stream << "std::memcpy((GLubyte*)pointer + " << _offset << "," << _resource << "," << _length
           << ");" << std::endl;
    stream << "}" << std::endl;
  }
}
