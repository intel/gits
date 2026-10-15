// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "objectNameMapService.h"

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
  case GLObjectKind::kCount:
    break;
    // No default: (an audit once caught
    // Renderbuffer/Sampler/ProgramPipeline missing here right after they were added, purely
    // because a `default:` branch had silently absorbed them into "Unknown" instead of letting
    // -Wswitch catch the gap at compile time) - an exhaustive switch with no default is the one
    // way a *future* new GLObjectKind enumerator is guaranteed to fail the build here instead
    // of silently misnaming itself in every warning log line.
  }
  return "Unknown";
}
} // namespace

ObjectNameMapService& ObjectNameMapService::Get() {
  // Meyer's singleton - same reasoning as ContextHandleMapService::Get(): neither the recorder
  // nor the player side of this service has any static-destruction-order hazard of its own.
  static ObjectNameMapService instance;
  return instance;
}

ObjectNameMapService::Bucket& ObjectNameMapService::BucketFor(GLObjectKind kind) {
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

void ObjectNameMapService::Delete(GLObjectKind kind, uint64_t shareGroup, GLuint name) {
  if (name == 0) {
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

} // namespace gits::opengl2
