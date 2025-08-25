// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapping.h"
#include "openglLibrary.h"

namespace gits {
namespace OpenGL {
// Moved out of header file to appease gcc 'unused-variable' warning.
current_program_tag_t current_program_tag;

const char* CMappedArgumentName::NAME = "ObjName";

// Return values: true means that the current context uses core profile; false
// means compatibility profile.
// These conditions are valid only for mapping feature. Impact of forward
// compatibility bit is also considered if any. When the function returns true,
// the mapping of the object id will be mandatory as per OGL spec Input param
// mapObject is used to find whether mapping is required or not for the
// particular object This function supports objects (e.g. textures) till OGL 4.2
bool isMappingRequired(MapObjects mapObject, int legacy) {
  auto& csd = SD().GetCurrentContextStateData();
  int curContextGLVer = csd.Version();
  if (!csd.IsOgl()) {
    // return not legacy in ES.
    return !(legacy & 0x2);
  }

  // if not legacy in desktop - always requires mapping
  if (!(legacy & 0x1)) {
    return true;
  }

  if (curContextGLVer <= 310) {
    return false;
  } else {
    // OpenGL 3.2 onwards. Both Core and Compatibility profile supported
    return (csd.ProfileMask() & GL_CONTEXT_CORE_PROFILE_BIT);
  }
}

///////////////////////////////////////////// CGLUniformLocation /////////////////////////////////////////////////////

CGLUniformLocation::CGLUniformLocation() : program_(0), location_(0) {}
CGLUniformLocation::CGLUniformLocation(current_program_tag_t, GLint location)
    : program_(0), location_(location) {
  auto version = SD().GetCurrentContextStateData().Version();
  if (ProgramOverride() == 0 && !CGits::Instance().IsStateRestoration()) {
    drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
    if (program_ == 0 && curctx::IsOgl() && (version >= 400)) {
      GLint pipeline = 0;
      drv.gl.glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);
      drv.gl.glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &program_);
    }
  } else if (ProgramOverride() != 0 && CGits::Instance().IsStateRestoration()) {
    program_ = ProgramOverride();
  } else {
    throw std::runtime_error("invalid configuration of program override in CGLUniformLocation");
  }
}
CGLUniformLocation::CGLUniformLocation(GLint program, GLint location)
    : program_(program), location_(location) {}

void CGLUniformLocation::Write(CBinOStream& stream) const {
  write_to_stream(stream, program_);
  write_to_stream(stream, location_);
}

void CGLUniformLocation::Read(CBinIStream& stream) {
  read_from_stream(stream, program_);
  read_from_stream(stream, location_);
}

void CGLUniformLocation::Write(CCodeOStream& stream) const {
  stream << "uloc(" << program_ << ", " << location_ << ")";
}

GLint CGLUniformLocation::operator*() const {
  auto& locationsMap = getLocationsMap()[program_];
  auto optionalLocationData = locationsMap.find(location_);
  if (!optionalLocationData) {
    // If location is not mapped we assume shader defined location
    return location_;
  }

  auto actualBase = optionalLocationData->currentBase;
  if (actualBase < 0) {
    return -1;
  }

  auto base = optionalLocationData->originalBase;
  return location_ - base + actualBase;
}

void CGLUniformLocation::AddMapping(GLint program,
                                    GLint location,
                                    GLint loc_size,
                                    GLint actual_location) {
  auto& locationsMap = getLocationsMap()[program];
  locationsMap.insert(location, location + loc_size, actual_location);
}

void CGLUniformLocation::RemoveMappings(GLint program) {
  getLocationsMap().erase(program);
}

CGLUniformLocation::program_locations_map_t& CGLUniformLocation::getLocationsMap() {
  typedef std::unordered_map<unsigned int, program_locations_map_t> location_maps_map_t;
  INIT_NEW_STATIC_OBJ(uniformLocationMaps, location_maps_map_t)
  return uniformLocationMaps[SD().GetCurrentContextSharedGroupId()];
}

/////////////////////////////////////////   CGLUniformSubroutineLocation   ////////////////////////////////////////////

CGLUniformSubroutineLocation::CGLUniformSubroutineLocation()
    : program_(0), type_(0), location_(0) {}

CGLUniformSubroutineLocation::CGLUniformSubroutineLocation(current_program_tag_t,
                                                           GLenum type,
                                                           GLint location)
    : program_(0), type_(type), location_(location) {
  drv.gl.glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
}

CGLUniformSubroutineLocation::CGLUniformSubroutineLocation(GLint program,
                                                           GLenum type,
                                                           GLint location)
    : program_(program), type_(type), location_(location) {}

void CGLUniformSubroutineLocation::Write(CBinOStream& stream) const {
  write_to_stream(stream, program_);
  write_to_stream(stream, type_);
  write_to_stream(stream, location_);
}

void CGLUniformSubroutineLocation::Read(CBinIStream& stream) {
  read_from_stream(stream, program_);
  read_from_stream(stream, type_);
  read_from_stream(stream, location_);
}

void CGLUniformSubroutineLocation::Write(CCodeOStream& stream) const {
  stream << "sloc(" << program_ << ", " << type_ << ", " << location_ << ")";
}

GLint CGLUniformSubroutineLocation::operator*() const {
  auto& shadersMap = getShadersMap()[program_];
  auto& locationsMap = shadersMap[type_];
  auto optionalLocationData = locationsMap.find(location_);
  if (!optionalLocationData) {
    LOG_WARNING << "Uniform subroutine location couldn't be found, returning -1";
    return -1;
  }

  auto actualBase = optionalLocationData->currentBase;
  if (actualBase < 0) {
    return -1;
  }

  auto base = optionalLocationData->originalBase;
  return location_ - base + actualBase;
}

void CGLUniformSubroutineLocation::AddMapping(
    GLint program, GLenum type, GLint location, GLint loc_size, GLint actual_location) {
  auto& shadersMap = getShadersMap()[program];
  auto& locationsMap = shadersMap[type];
  locationsMap.insert(location, location + loc_size, actual_location);
}

CGLUniformSubroutineLocation::program_locations_map_t& CGLUniformSubroutineLocation::
    getShadersMap() {
  typedef std::map<unsigned int, program_locations_map_t> location_maps_map_t;
  INIT_NEW_STATIC_OBJ(uniformLocationMaps, location_maps_map_t)
  return uniformLocationMaps[SD().GetCurrentContextSharedGroupId()];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGLSubroutineIndex::AddMapping(GLint program, GLenum type, GLuint index, GLuint actual_index) {
  getShadersMap()[program][type][index] = actual_index;
}

GLuint CGLSubroutineIndex::GetMapping(GLint program, GLenum type, GLuint index) {
  auto& idx_map = getShadersMap()[program][type];
  auto iter = idx_map.find(index);
  if (iter == idx_map.end()) {
    LOG_WARNING << "subroutine index mapping not found";
    return index;
  }
  return iter->second;
}

CGLSubroutineIndex::program_indices_map_t& CGLSubroutineIndex::getShadersMap() {
  typedef std::map<unsigned int, program_indices_map_t> indices_maps_map_t;
  INIT_NEW_STATIC_OBJ(uniformLocationMaps, indices_maps_map_t)
  return uniformLocationMaps[SD().GetCurrentContextSharedGroupId()];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CGLUniformBlockIndex::CGLUniformBlockIndex() : program_(0), index_(0) {}

CGLUniformBlockIndex::CGLUniformBlockIndex(GLint program, GLint index)
    : program_(program), index_(index) {}

void CGLUniformBlockIndex::Write(CBinOStream& stream) const {
  write_to_stream(stream, program_);
  write_to_stream(stream, index_);
}

void CGLUniformBlockIndex::Read(CBinIStream& stream) {
  read_from_stream(stream, program_);
  read_from_stream(stream, index_);
}

void CGLUniformBlockIndex::Write(CCodeOStream& stream) const {
  stream << "bidx(" << program_ << ", " << index_ << ")";
}

GLint CGLUniformBlockIndex::operator*() const {
  auto& idx_map = getProgramsMap()[program_];
  auto iter = idx_map.find(index_);
  if (iter == idx_map.end()) {
    LOG_WARNING << "Couldn't map uniform block index";
    return index_;
  }
  return iter->second;
}

void CGLUniformBlockIndex::AddMapping(GLint program, GLint index, GLint actual_index) {
  getProgramsMap()[program][index] = actual_index;
}

CGLUniformBlockIndex::program_indices_map_t& CGLUniformBlockIndex::getProgramsMap() {
  typedef std::map<unsigned int, program_indices_map_t> indices_maps_map_t;
  INIT_NEW_STATIC_OBJ(uniformLocationMaps, indices_maps_map_t)
  return uniformLocationMaps[SD().GetCurrentContextSharedGroupId()];
}

CGLStorageBlockIndex::CGLStorageBlockIndex() : program_(0), index_(0) {}

CGLStorageBlockIndex::CGLStorageBlockIndex(GLint program, GLuint index)
    : program_(program), index_(index) {}

void CGLStorageBlockIndex::Write(CBinOStream& stream) const {
  write_to_stream(stream, program_);
  write_to_stream(stream, index_);
}

void CGLStorageBlockIndex::Read(CBinIStream& stream) {
  read_from_stream(stream, program_);
  read_from_stream(stream, index_);
}

void CGLStorageBlockIndex::Write(CCodeOStream& stream) const {
  stream << "bidx(" << program_ << ", " << index_ << ")";
}

GLuint CGLStorageBlockIndex::operator*() const {
  auto& idx_map = getProgramsMap()[program_];
  auto iter = idx_map.find(index_);
  if (iter == idx_map.end()) {
    LOG_WARNING << "Couldn't map storage block index";
    return index_;
  }
  return iter->second;
}

void CGLStorageBlockIndex::AddMapping(GLint program, GLuint index, GLuint actual_index) {
  getProgramsMap()[program][index] = actual_index;
}

CGLStorageBlockIndex::program_indices_map_t& CGLStorageBlockIndex::getProgramsMap() {
  typedef std::map<unsigned int, program_indices_map_t> indices_maps_map_t;
  INIT_NEW_STATIC_OBJ(storageLocationMaps, indices_maps_map_t)
  return storageLocationMaps[SD().GetCurrentContextSharedGroupId()];
}

} // namespace OpenGL
} // namespace gits
