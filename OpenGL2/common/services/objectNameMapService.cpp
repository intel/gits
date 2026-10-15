// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "objectNameMapService.h"

#include "glEnumsAuto.h"
#include "log.h"

namespace gits::opengl2 {

namespace {
// Text used in log messages to identify which namespace a warning is about - kept separate
// from the enum itself (rather than, say, an ostream operator<<) since this is the only place
// that ever needs a human-readable name.
const char* KindName(GLObjectKind kind) {
  switch (kind) {
  case GLObjectKind::Texture:
    return "Texture";
  case GLObjectKind::Buffer:
    return "Buffer";
  case GLObjectKind::Program:
    return "Program";
  case GLObjectKind::Shader:
    return "Shader";
  case GLObjectKind::Framebuffer:
    return "Framebuffer";
  case GLObjectKind::VertexArray:
    return "VertexArray";
  case GLObjectKind::Query:
    return "Query";
  case GLObjectKind::Renderbuffer:
    return "Renderbuffer";
  case GLObjectKind::Sampler:
    return "Sampler";
  case GLObjectKind::ProgramPipeline:
    return "ProgramPipeline";
  case GLObjectKind::TransformFeedback:
    return "TransformFeedback";
  case GLObjectKind::kCount:
    break;
    // No default: - a `default:` branch would silently absorb any future GLObjectKind
    // enumerator into "Unknown" instead of letting -Wswitch catch the gap at compile time -
    // an exhaustive switch with no default is the one
    // way a *future* new GLObjectKind enumerator is guaranteed to fail the build here instead
    // of silently misnaming itself in every warning log line.
  }
  return "Unknown";
}
} // namespace

GLObjectKind ImageObjectKind(GLenum target) {
  return target == GL_RENDERBUFFER ? GLObjectKind::Renderbuffer : GLObjectKind::Texture;
}

GLObjectKind ObjectIdentifierKind(GLenum identifier) {
  switch (identifier) {
  case GL_BUFFER:
    return GLObjectKind::Buffer;
  case GL_SHADER:
    return GLObjectKind::Shader;
  case GL_PROGRAM:
    return GLObjectKind::Program;
  case GL_VERTEX_ARRAY:
    return GLObjectKind::VertexArray;
  case GL_QUERY:
    return GLObjectKind::Query;
  case GL_PROGRAM_PIPELINE:
    return GLObjectKind::ProgramPipeline;
  case GL_TRANSFORM_FEEDBACK:
    return GLObjectKind::TransformFeedback;
  case GL_SAMPLER:
    return GLObjectKind::Sampler;
  case GL_RENDERBUFFER:
    return GLObjectKind::Renderbuffer;
  case GL_FRAMEBUFFER:
    return GLObjectKind::Framebuffer;
  case GL_TEXTURE:
  default:
    return GLObjectKind::Texture;
  }
}

bool ObjectIdentifierMapsNames(GLenum identifier) {
  return identifier != GL_DISPLAY_LIST;
}

ObjectNameMapService& ObjectNameMapService::Get() {
  // Meyer's singleton - same reasoning as ContextHandleMapService::Get(): neither the recorder
  // nor the player side of this service has any static-destruction-order hazard of its own.
  static ObjectNameMapService instance;
  return instance;
}

ObjectNameMapService::Bucket& ObjectNameMapService::BucketFor(GLObjectKind kind) {
  // Shader and program objects share one name space, and a GL_ARB_shader_objects handle may
  // name either.
  if (kind == GLObjectKind::Shader) {
    kind = GLObjectKind::Program;
  }
  return m_Buckets[static_cast<size_t>(kind)];
}

void ObjectNameMapService::Register(GLObjectKind kind,
                                    uint64_t shareGroup,
                                    GLuint name,
                                    GITSKey key) {
  if (name == 0) {
    return;
  }
  const NameKey nameKey{shareGroup, name};
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  bucket.NameToKey[nameKey] = key;
  bucket.WarnedMissingNames.erase(nameKey);
}

GITSKey ObjectNameMapService::GetKey(GLObjectKind kind, uint64_t shareGroup, GLuint name) {
  if (name == 0) {
    return 0;
  }
  const NameKey nameKey{shareGroup, name};
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  auto it = bucket.NameToKey.find(nameKey);
  if (it != bucket.NameToKey.end()) {
    return it->second;
  }
  if (bucket.WarnedMissingNames.insert(nameKey).second) {
    LOG_WARNING << "ObjectNameMapService: " << KindName(kind) << " name " << name
                << " (share group " << shareGroup
                << ") has no registered key (never created, or created before recording "
                   "started)";
  }
  return 0;
}

GITSKey ObjectNameMapService::TryGetKey(GLObjectKind kind, uint64_t shareGroup, GLuint name) {
  if (name == 0) {
    return 0;
  }
  const NameKey nameKey{shareGroup, name};
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  auto it = bucket.NameToKey.find(nameKey);
  if (it != bucket.NameToKey.end()) {
    return it->second;
  }
  return 0;
}

uint64_t ObjectNameMapService::FindOtherIdentityForName(GLObjectKind kind,
                                                        uint64_t identity,
                                                        GLuint name) {
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  for (const auto& entry : bucket.NameToKey) {
    if (entry.first.Name == name && entry.first.ShareGroup != identity) {
      return entry.first.ShareGroup;
    }
  }
  return 0;
}

GITSKey ObjectNameMapService::FindKeyInOtherIdentity(GLObjectKind kind,
                                                     uint64_t identity,
                                                     GLuint name,
                                                     const std::function<bool(uint64_t)>& accept) {
  if (name == 0) {
    return 0;
  }
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  for (const auto& entry : bucket.NameToKey) {
    if (entry.first.Name == name && entry.first.ShareGroup != identity &&
        accept(entry.first.ShareGroup)) {
      return entry.second;
    }
  }
  return 0;
}

void ObjectNameMapService::Delete(GLObjectKind kind, uint64_t shareGroup, GLuint name) {
  if (name == 0) {
    return;
  }
  // Program/Shader keep their forward mapping alive across a delete - see this method's own
  // header comment for the GL deferred-deletion rationale. TransformFeedback keeps its
  // mapping alive here too, for the related-but-distinct
  // "the driver may have rejected this specific delete outright" reason also documented in
  // this method's own header comment (objectNameMapService.h) - see that comment before
  // changing this condition. Nothing else to do here for any of the three: a still-legal
  // reference keeps resolving against the existing (correct) entry, and an eventual
  // reused-name create overwrites it via Register() exactly as before.
  if (kind == GLObjectKind::Program || kind == GLObjectKind::Shader ||
      kind == GLObjectKind::TransformFeedback) {
    return;
  }
  const NameKey nameKey{shareGroup, name};
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  bucket.NameToKey.erase(nameKey);
  bucket.WarnedMissingNames.erase(nameKey);
}

void ObjectNameMapService::SetHandle(GLObjectKind kind, GITSKey key, GLuint name) {
  if (key == 0 || name == 0) {
    return;
  }
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  bucket.KeyToName[key] = name;
  bucket.Names.insert(name);
}

GLuint ObjectNameMapService::UnusedName(GLObjectKind kind,
                                        const std::function<bool(GLuint)>& isObject) {
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  for (GLuint name = bucket.NextUnusedName; name != 0; ++name) {
    if (bucket.Names.count(name) == 0 && !isObject(name)) {
      bucket.NextUnusedName = name + 1;
      return name;
    }
  }
  return 0;
}

GLuint ObjectNameMapService::GetHandle(GLObjectKind kind, GITSKey key) {
  if (key == 0) {
    return 0;
  }
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  auto it = bucket.KeyToName.find(key);
  if (it != bucket.KeyToName.end()) {
    return it->second;
  }
  if (bucket.WarnedMissingKeys.insert(key).second) {
    LOG_WARNING << "ObjectNameMapService: " << KindName(kind) << " key " << key
                << " has no registered name (never created during this replay, or the "
                   "creating command has not been replayed yet)";
  }
  return 0;
}

GLuint ObjectNameMapService::TryGetHandle(GLObjectKind kind, GITSKey key) {
  if (key == 0) {
    return 0;
  }
  Bucket& bucket = BucketFor(kind);
  std::lock_guard<std::mutex> lock(bucket.Mutex);
  auto it = bucket.KeyToName.find(key);
  if (it != bucket.KeyToName.end()) {
    return it->second;
  }
  return 0;
}

} // namespace gits::opengl2
