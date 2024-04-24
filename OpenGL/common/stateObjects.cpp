// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   stateObjects.cpp
 *
 * @brief Definition of OpenGL common part library implementation.
 *
 */

#include "stateObjects.h"
#include "openglDrivers.h"
#include "stateDynamic.h"
#include "config.h"
#include "tools.h"

namespace gits {
namespace OpenGL {

//----------------------CTEXTURESTATEOBJ----------------------
void CTextureStateData::Restored::GetNewObjectForTarget(GLenum target) {
  switch (target) {
  case 0:
    // Do nothing.
    break;
  case GL_TEXTURE_1D:
  case GL_TEXTURE_1D_ARRAY:
  case GL_TEXTURE_2D:
  case GL_TEXTURE_2D_MULTISAMPLE:
  case GL_TEXTURE_2D_ARRAY:
  case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
  case GL_TEXTURE_EXTERNAL_OES:
  case GL_TEXTURE_CUBE_MAP_ARRAY_ARB:
  case GL_TEXTURE_3D:
  case GL_TEXTURE_RECTANGLE_ARB:
    ptr.reset(new CTextureNDData());
    break;
  case GL_TEXTURE_CUBE_MAP:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    ptr.reset(new CTextureCubeData());
    break;
  case GL_TEXTURE_BUFFER:
    ptr.reset(new CTextureBufferData());
    break;
  default:
    Log(ERR) << "Not supported texture target: " << target;
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

CTextureStateObj::CTextureStateObj(GLuint texture, GLenum target) : _data(texture, target) {}

void CTextureStateObj::SetTexLevelParams(GLint level,
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
                                         GLenum type) {
  if (_data.track.mipmapData.size() > 0) {
    //Here we skip levels greater then max possible texture level (WA for strange app behaviour for ie Elactopia Benchmark)
    if (_data.track.mipmapData[0].width > 0) { //Do it only if level zero loaded
      auto longest = std::max(_data.track.mipmapData[0].width, _data.track.mipmapData[0].height);
      longest = std::max(longest, _data.track.mipmapData[0].depth);
      if (log2i(longest) < level) {
        Log(WARN) << "Skipping tracking of level: " << level << " of texture: " << Name()
                  << " due to be greater then max possible texture level for this texture.";
        return;
      }
    }
  }

  while (_data.track.mipmapData.size() <= (unsigned)level) {
    _data.track.mipmapData.push_back(CTextureStateData::TMipmapTextureData());
  }

  _data.track.mipmapData[level].width = width;
  _data.track.mipmapData[level].height = height;
  _data.track.mipmapData[level].depth = depth;
  _data.track.mipmapData[level].internalFormat = internalformat;
  _data.track.mipmapData[level].border = border;
  _data.track.mipmapData[level].samples = samples;
  _data.track.mipmapData[level].fixedsamplelocations = fixedsamplelocations;
  _data.track.mipmapData[level].format = format;
  _data.track.mipmapData[level].type = type;

  _data.track.mipmapData[level].compressed = compressed;
  _data.track.mipmapData[level].compressedImageSize = compressedimagesize;

  //compressed==true means that it was set in CompressedTexImage like API.
  //It is possible to load compressed texture using TexImage/TexStorage call.
  //We need to check it here and update compressed value if needed
  if (!compressed && (curctx::IsOgl() || IsGlGetTexAndCompressedTexImagePresentOnGLES())) {
    //OpenGL
    GLint boundName = 0;

    if (Config::Get().recorder.openGL.utilities.trackTextureBindingWA) {
      auto unit =
          SD().GetCurrentContextStateData().GeneralStateObjects().Data().tracked.activeTexture;
      boundName = SD().GetCurrentContextStateData()
                      .GeneralStateObjects()
                      .Data()
                      .tracked.boundTextures[unit][_data.track.target];
    } else {
      drv.gl.glGetIntegerv(GetBindingEnum(_data.track.target), &boundName);
    }

    if ((unsigned)boundName != _data.track.name) {
      drv.gl.glBindTexture(_data.track.target, _data.track.name);
    }

    //We assume that all cube maps has same size and all parts of it are initialized
    GLenum paramTarget = _data.track.target;
    if (paramTarget == GL_TEXTURE_CUBE_MAP) {
      paramTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }
    drv.gl.glGetTexLevelParameteriv(paramTarget, level, GL_TEXTURE_COMPRESSED,
                                    &_data.track.mipmapData[level].compressed);
    drv.gl.glGetTexLevelParameteriv(paramTarget, level, GL_TEXTURE_INTERNAL_FORMAT,
                                    &_data.track.mipmapData[level].internalFormat);
    if (_data.track.mipmapData[level].compressed != 0) {
      drv.gl.glGetTexLevelParameteriv(paramTarget, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
                                      &_data.track.mipmapData[level].compressedImageSize);
    }

    if ((unsigned)boundName != _data.track.name) {
      drv.gl.glBindTexture(_data.track.target, boundName);
    }
  } else if (!compressed) {
    //OpenGL ES
    _data.track.mipmapData[level].compressed =
        IsTextureCompressed(_data.track.mipmapData[level].internalFormat);
    //compressed image data size has to be set in glCompressedTexSubImage calls for ES.
  }

  if (!_data.track.mipmapData[level].compressed &&
      (_data.track.mipmapData[level].format == 0 || _data.track.mipmapData[level].type == 0)) {
    if (getDataFromInternalFormat(_data.track.mipmapData[level].internalFormat,
                                  _data.track.mipmapData[level].format,
                                  _data.track.mipmapData[level].type) != true) {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
  }
}

//----------------------CARBPROGRAMSTATEOBJ----------------------
CARBProgramStateObj::CARBProgramStateObj(GLuint program, GLenum target /* 0 */)
    : _name(program), _target(target) {}

//----------------------CGLSLSHADERSTATEOBJ--------------------
void CGLSLShaderStateObj::Source(std::string& source) {
  _data->track.compiled = false;
  _data->track.source = source;
  static int uniqueSource = 1;
  _data->track.uniqueSource = uniqueSource++;
}

//----------------------CGLSLPROGRAMSTATEOBJ--------------------
void CGLSLProgramStateObj::DetachShader(GLuint shadername) {
  _data->track.attachedShaders.erase(shadername);

  //Delete shader if marked to delete and not attached to other program
  CGLSLShaderStateObj* shaderStateObjPtr =
      SD().GetCurrentSharedStateData().GLSLShaders().Get(shadername);
  if (shaderStateObjPtr != nullptr && !shaderStateObjPtr->IsAttached() &&
      shaderStateObjPtr->MarkedToDelete()) {
    shaderStateObjPtr = nullptr;
    SD().GetCurrentSharedStateData().GLSLShaders().Remove(1, &shadername);
  }
}

void CGLSLProgramStateObj::Link() {
  _data->track.linked = true;

  // Attributes are gathered in CglLinkProgram ctor.
  _data->track.attribs.clear();

  // Storing attached shaders data - it allows to restore shaders as they had been just before linkage even if they were modified later.
  _data->track.linkedShaders.clear();
  std::for_each(_data->track.attachedShaders.begin(), _data->track.attachedShaders.end(),
                [&](std::pair<GLuint, std::shared_ptr<CGLSLShaderStateData>> iter) {
                  _data->track.linkedShaders[iter.first] = *(iter.second);
                });
}

//----------------------CGLSLPIPELINESTATEOBJ--------------------
void CGLSLPipelineStateObj::UseProgramStage(GLbitfield stages, GLuint program) {
  if (program == 0) {
    //Remove pipeline stages of program 0
    if (stages & GL_COMPUTE_SHADER_BIT) {
      _data.track.stages.erase(GL_COMPUTE_SHADER_BIT);
    }
    if (stages & GL_VERTEX_SHADER_BIT) {
      _data.track.stages.erase(GL_VERTEX_SHADER_BIT);
    }
    if (stages & GL_TESS_CONTROL_SHADER_BIT) {
      _data.track.stages.erase(GL_TESS_CONTROL_SHADER_BIT);
    }
    if (stages & GL_TESS_EVALUATION_SHADER_BIT) {
      _data.track.stages.erase(GL_TESS_EVALUATION_SHADER_BIT);
    }
    if (stages & GL_GEOMETRY_SHADER_BIT) {
      _data.track.stages.erase(GL_GEOMETRY_SHADER_BIT);
    }
    if (stages & GL_FRAGMENT_SHADER_BIT) {
      _data.track.stages.erase(GL_FRAGMENT_SHADER_BIT);
    }
  } else {
    //Update pipeline stages programs
    CGLSLProgramStateObj* programStateObj =
        SD().GetCurrentSharedStateData().GLSLPrograms().Get(program);
    if (programStateObj == nullptr) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (stages & GL_COMPUTE_SHADER_BIT) {
      _data.track.stages[GL_COMPUTE_SHADER_BIT] = programStateObj->DataShared();
    }
    if (stages & GL_VERTEX_SHADER_BIT) {
      _data.track.stages[GL_VERTEX_SHADER_BIT] = programStateObj->DataShared();
    }
    if (stages & GL_TESS_CONTROL_SHADER_BIT) {
      _data.track.stages[GL_TESS_CONTROL_SHADER_BIT] = programStateObj->DataShared();
    }
    if (stages & GL_TESS_EVALUATION_SHADER_BIT) {
      _data.track.stages[GL_TESS_EVALUATION_SHADER_BIT] = programStateObj->DataShared();
    }
    if (stages & GL_GEOMETRY_SHADER_BIT) {
      _data.track.stages[GL_GEOMETRY_SHADER_BIT] = programStateObj->DataShared();
    }
    if (stages & GL_FRAGMENT_SHADER_BIT) {
      _data.track.stages[GL_FRAGMENT_SHADER_BIT] = programStateObj->DataShared();
    }
  }
}

//----------------------CBUFFERSTATEOBJ----------------------
CBufferStateObj::CBufferStateObj(GLuint buffer, GLenum target) : _data(buffer, target) {
  _data.restore.mapped = false;
  _data.restore.mapLength = -1;
  _data.restore.mapAccess = 0;
  _data.restore.mapOffset = -1;
  _data.restore.named = false;
}

void CBufferStateObj::SetBufferMapPlay(GLbitfield access, bool named, GLint length, GLint offset) {
  _data.restore.mapAccess = access;
  _data.restore.mapLength = length;
  _data.restore.mapOffset = offset;
  _data.restore.mapFlushRangeOffset = 0;
  _data.restore.mapFlushRangeLength = 0;
  _data.restore.mapped = true;
  _data.restore.named = named;

  if (!_data.track.coherentMapping &&
      ((access & GL_MAP_COHERENT_BIT) ||
       ((access & GL_MAP_PERSISTENT_BIT) &&
        Config::Get().recorder.extras.utilities.coherentMapBehaviorWA))) {
    _data.track.coherentMapping = true;
  }
}

void CBufferStateObj::InitBufferMapPlay(GLbitfield access, bool named, GLint length, GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER;
  SetBufferMapPlay(access, named, length, offset);
}

void CBufferStateObj::InitBufferMapPlayOES(GLbitfield access,
                                           bool named,
                                           GLint length,
                                           GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER_OES;
  SetBufferMapPlay(access, named, length, offset);
}

void CBufferStateObj::InitBufferMapPlayARB(GLbitfield access,
                                           bool named,
                                           GLint length,
                                           GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER_ARB;
  SetBufferMapPlay(access, named, length, offset);
}

void CBufferStateObj::InitBufferMapPlayEXT(GLbitfield access,
                                           bool named,
                                           GLint length,
                                           GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER_EXT;
  SetBufferMapPlay(access, named, length, offset);
}

void CBufferStateObj::SetBufferMapRec(GLbitfield access, bool named, GLint length, GLint offset) {
  GLbitfield access_interceptor = access;
  if ((access & GL_MAP_COHERENT_BIT) ||
      ((access & GL_MAP_PERSISTENT_BIT) &&
       Config::Get().recorder.extras.utilities.coherentMapBehaviorWA)) {
    access_interceptor |= GL_MAP_READ_BIT;
    access_interceptor &= ~GL_MAP_UNSYNCHRONIZED_BIT;
  }
  _data.restore.mapAccess = access_interceptor;
  _data.restore.mapLength = length;
  _data.restore.mapOffset = offset;
  _data.restore.mapFlushRangeOffset = 0;
  _data.restore.mapFlushRangeLength = 0;
  _data.restore.mapped = true;
  _data.restore.named = named;

  if (!_data.track.coherentMapping &&
      ((access & GL_MAP_COHERENT_BIT) ||
       ((access & GL_MAP_PERSISTENT_BIT) &&
        Config::Get().recorder.extras.utilities.coherentMapBehaviorWA))) {
    _data.track.coherentMapping = true;
  }

  if (access & GL_MAP_INVALIDATE_BUFFER_BIT) {
    _data.track.initializedData = false;
    _data.track.initializedDataMap.clear();
  }
}

void CBufferStateObj::InitBufferMapRec(GLbitfield access, bool named, GLint length, GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER;
  SetBufferMapRec(access, named, length, offset);
}

void CBufferStateObj::InitBufferMapRecOES(GLbitfield access,
                                          bool named,
                                          GLint length,
                                          GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER_OES;
  SetBufferMapRec(access, named, length, offset);
}

void CBufferStateObj::InitBufferMapRecARB(GLbitfield access,
                                          bool named,
                                          GLint length,
                                          GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER_ARB;
  SetBufferMapRec(access, named, length, offset);
}

void CBufferStateObj::InitBufferMapRecEXT(GLbitfield access,
                                          bool named,
                                          GLint length,
                                          GLint offset) {
  _data.restore.type = CBufferStateData::Restored::MAP_BUFFER_EXT;
  SetBufferMapRec(access, named, length, offset);
}

void CBufferStateObj::FlushMappedBufferRange(GLintptr offset, GLsizeiptr length) {
  auto& origLength = _data.restore.mapFlushRangeLength;
  auto& origOffset = _data.restore.mapFlushRangeOffset;
  if (origOffset == 0 && origLength == 0) {
    origLength = length;
    origOffset = offset;
  } else {
    if (origOffset > offset) {
      origOffset = offset;
    }
    if ((origOffset + origLength) < (offset + length)) {
      origLength = offset + length - origOffset;
    }
  }
}

void CBufferStateObj::TrackBufferData(GLintptr buffoffset, GLsizeiptr size, const GLvoid* dataptr) {
  if (size == 0) {
    return;
  }
  if (_data.restore.buffer.size() < (unsigned)(buffoffset + size)) {
    _data.restore.buffer.resize(buffoffset + size);
  }
  if (dataptr == nullptr) {
    _data.track.initializedData = false;
    _data.track.initializedDataMap.clear();
  } else {
    //Initialized data update
    if (!_data.track.initializedData) {
      if (buffoffset == 0 && (unsigned)size == _data.restore.buffer.size() &&
          _data.restore.mapFlushRangeLength == 0) {
        //Mark entire buffer as initialized
        _data.track.initializedData = true;
      } else {
        //Get initialized interval
        uint64_t intervalFirst = buffoffset;
        uint64_t intervalLast = buffoffset + size;

        //Update intervals
        boost::icl::interval<uint64_t>::type interv(intervalFirst, intervalLast);
        _data.track.initializedDataMap.insert(interv);
      }

      //If initialized range covers entire buffer mark it as initialized
      auto iter = _data.track.initializedDataMap.find(0);
      if (iter != _data.track.initializedDataMap.end() &&
          iter->upper() == (_data.restore.buffer.size() - 1)) {
        _data.track.initializedData = true;
      }
    }

    char* srcPtr = (char*)dataptr;
    char* dstPtr = ((char*)&_data.restore.buffer[0]) + buffoffset;
    memcpy(dstPtr, srcPtr, size);
  }
}

void CBufferStateObj::CalculateMapChange(GLintptr& mapoffset,
                                         GLintptr& buffoffset,
                                         GLsizeiptr& size,
                                         const GLvoid* ptr) {
  if (!_data.restore.mapped) {
    return;
  }

  if (_data.restore.buffer.empty()) {
    return;
  }

  GLsizeiptr mapFlushSize = 0;
  if (_data.restore.mapFlushRangeLength > 0) {
    mapFlushSize = _data.restore.mapFlushRangeLength; //Use Flushed mapped buffer range size
  } else if (_data.restore.mapLength == -1) {
    mapFlushSize = _data.track.size; //Use Entire buffer size
  } else {
    mapFlushSize = _data.restore.mapLength; //Use Mapped buffer range size
  }

  GLintptr mapOffset = 0;
  if (_data.restore.mapOffset == -1) {
    mapOffset = 0; //Use Entire buffer offset
  } else {
    mapOffset = _data.restore.mapOffset; //Use Mapped buffer range offset
  }

  GLintptr mapFlushOffset = 0;
  if (_data.restore.mapFlushRangeLength > 0) {
    mapFlushOffset = _data.restore.mapFlushRangeOffset; //Use Flushed mapped buffer range offset
  }

  if (_data.restore.buffer.size() < (unsigned)(mapOffset + mapFlushSize)) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  const uint8_t* minPrePtr = &_data.restore.buffer.front() + mapOffset + mapFlushOffset;
  const uint8_t* maxPrePtr = minPrePtr + mapFlushSize;

  const uint8_t* minMapPtr = (const uint8_t*)ptr + mapFlushOffset;
  const uint8_t* maxMapPtr = minMapPtr + mapFlushSize;

  bool initialized = false;
  //Data initialization check
  if (_data.track.initializedData) {
    initialized = true;
  } else {
    //If entire buffer is not initialized check if mapped range is
    auto& initDataMap = _data.track.initializedDataMap;
    auto rangeLower = mapOffset + mapFlushOffset;
    auto rangeUpper = rangeLower + mapFlushSize;
    auto initializedRange = initDataMap.find(rangeLower);
    if (initializedRange != initDataMap.end() &&
        initializedRange->upper() <= (uint64_t)rangeUpper) {
      initialized = true;
    }
  }

  //Narrow diff update only if data is not invalidated. In case of invalid data we do not have fixed reference for comparison
  if (!(_data.restore.mapAccess & GL_MAP_INVALIDATE_RANGE_BIT) && initialized) {
    while (minPrePtr < maxPrePtr && *minPrePtr == *minMapPtr) {
      minMapPtr++;
      minPrePtr++;
    }
    while (minPrePtr < maxPrePtr) {
      maxMapPtr--;
      maxPrePtr--;
      if (*maxPrePtr != *maxMapPtr) {
        maxMapPtr++;
        maxPrePtr++;
        break;
      }
    }
  }
  size = maxPrePtr - minPrePtr;

  //Buffer offset - offset to changed data in reference to entire buffer data
  buffoffset = minPrePtr - &_data.restore.buffer.front();

  //Map Offset  - offset to changed data in reference to mapped range which may has additional offset in reference to entire buffer data
  mapoffset = minPrePtr - mapOffset - &_data.restore.buffer.front();
}

CBindingStateData::Tracked::Tracked() : glslProgram(0), glslPipeline(0) {
  boundBuffers[GL_ARRAY_BUFFER] = 0;
  boundBuffers[GL_ELEMENT_ARRAY_BUFFER] = 0;
  boundBuffers[GL_PIXEL_PACK_BUFFER] = 0;
  boundBuffers[GL_PIXEL_UNPACK_BUFFER] = 0;
  boundBuffers[GL_UNIFORM_BUFFER] = 0;
  boundBuffers[GL_TEXTURE_BUFFER] = 0;
}

//----------------------CCLIENTARRAYSSTATEOBJECT----------------------

void CClientArraysStateObj::RestoreClientAttribs() {
  bool status = false;

  status = drv.gl.glIsEnabled(GL_VERTEX_ARRAY);
  VertexArray().Enabled(status);

  status = drv.gl.glIsEnabled(GL_NORMAL_ARRAY);
  NormalArray().Enabled(status);

  status = drv.gl.glIsEnabled(GL_COLOR_ARRAY);
  ColorArray().Enabled(status);

  status = drv.gl.glIsEnabled(GL_SECONDARY_COLOR_ARRAY);
  SecondaryColorArray().Enabled(status);

  if (!_texcoordArrays.empty()) {
    int activeTextureIndex = -1;

    drv.gl.glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &activeTextureIndex);

    for (auto& elem : _texcoordArrays) {
      drv.gl.glClientActiveTexture(elem.first);
      status = drv.gl.glIsEnabled(GL_TEXTURE_COORD_ARRAY);
      elem.second.Enabled(status);
    }

    drv.gl.glClientActiveTexture(activeTextureIndex);
  }

  if (!curctx::IsEs1() && !_vertexAttribArrays.empty()) {
    bool checkRequired = false;
    GLint vertexArrayObject = 0;

    // First check for vertex array object
    if (curctx::IsOgl()) {
      drv.gl.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArrayObject);
    }

    if (vertexArrayObject == 0) {
      checkRequired = true;
    }

    for (auto& elem : _vertexAttribArrays) {
      GLuint attribIdx = 0;

      attribIdx = elem.first;

      if (checkRequired) {
        int vertexAttribArrayStatus = -1;

        drv.gl.glGetVertexAttribiv(attribIdx, GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                                   &vertexAttribArrayStatus);
        status = (vertexAttribArrayStatus != 0);
      } else {
        status = false; // false will ensure that its atleast not used in this invalid state.
      }

      elem.second.Enabled(status);
    }
  }
}

//----------------------CMAPPEDTEXTURESSTATEOBJECT----------------------
void CMappedTexturesStateObj::MapTexture(
    GLint name, GLint level, void* mapping, int size, int access, int* stride, int* layout) {
  MappedLevel mapped_level;
  mapped_level.texture = name;
  mapped_level.level = level;
  mapped_level.pointer = mapping;
  mapped_level.size = size;
  mapped_level.hash = 0;
  mapped_level.access = access;
  mapped_level.stride = *stride;
  mapped_level.layout = *layout;

  mapped_.insert(std::make_pair(name, mapped_level));
}

void CMappedTexturesStateObj::UnmapTexture(GLint name, GLint level) {
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  for (map_t::iterator it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level) {
      mapped_.erase(it);
      break;
    }
  }
}

void CMappedTexturesStateObj::InitializeTexture(GLint name, GLint level) {
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  for (map_t::iterator it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level) {
      it->second.isInitialized = true;
    }
  }
}

void CMappedTexturesStateObj::RemoveTexture(GLint name, GLint level) {
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  map_t::iterator it;
  for (it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level) {
      break;
    }
  }
  mapped_.erase(it);
}

std::vector<CMappedTexturesStateObj::MappedLevel> CMappedTexturesStateObj::ReturnNewTextures() {
  std::vector<MappedLevel> updated;
  for (auto& elem : mapped_) {
    if (elem.second.isInitialized == false) {
      updated.push_back(elem.second);
    }
  }

  return updated;
}

bool CMappedTexturesStateObj::CheckIfTextureIsMapped(int name) {
  if (mapped_.find(name) != mapped_.end()) {
    return true;
  }

  return false;
}

int CMappedTexturesStateObj::GetLayout(int name) {
  map_t::iterator it = mapped_.find(name);
  if (it == mapped_.end()) {
    return -1;
  }
  return it->second.layout;
}

std::vector<CMappedTexturesStateObj::MappedLevel> CMappedTexturesStateObj::SaveTextureToFile(
    GLint name, GLint level) {
  std::vector<MappedLevel> updated;
  hash_t hash;
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  for (map_t::iterator it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level) {
      hash = CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, it->second.pointer,
                                                      it->second.size);
      if (hash != it->second.hash) {
        it->second.hash = hash;
        updated.push_back(it->second);
      }
    }
  }

  return updated;
}

std::vector<CMappedTexturesStateObj::MappedLevel> CMappedTexturesStateObj::SaveAllTexturesToFile() {
  std::vector<MappedLevel> updated;
  for (auto& elem : mapped_) {
    hash_t hash = CGits::Instance().ResourceManager2().put(RESOURCE_TEXTURE, elem.second.pointer,
                                                           elem.second.size);
    if (hash != elem.second.hash) {
      elem.second.hash = hash;
      updated.push_back(elem.second);
    }
  }

  return updated;
}

void CMappedTexturesStateObj::UpdateTexture(GLint name, GLint level, const void* mapping) {
  // Get a range of MappedLevel items corresponding to the given name.
  std::pair<map_t::iterator, map_t::iterator> textureLevels = mapped_.equal_range(name);
  for (map_t::iterator it = textureLevels.first; it != textureLevels.second; ++it) {
    if (it->second.level == level && it->second.pointer != nullptr) {
      memcpy(it->second.pointer, mapping, it->second.size);
    }
  }
}

} // namespace OpenGL
} // namespace gits
