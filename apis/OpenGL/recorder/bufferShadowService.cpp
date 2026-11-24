// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bufferShadowService.h"

#include "glBufferQueries.h"
#include "glContextCapabilities.h"
#include "glContextQueries.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mapTrackingService.h"
#include "glDispatchAuto.h"

#include <cstring>

namespace gits::opengl {

namespace {

GLint GetInteger(GLenum pname) {
  GLint value = 0;
  g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(pname, &value);
  return value;
}

// Targets whose buffer the application can write outside GL: its memory is the client pointer
// given at creation.
bool IsClientMemoryTarget(GLenum target) {
  return target == GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD;
}

} // namespace

BufferShadowService& BufferShadowService::Get() {
  static BufferShadowService instance;
  return instance;
}

BufferShadowService::Key BufferShadowService::Resolve(BufferRef ref) {
  const uint64_t shareGroup = CurrentShareGroup();
  if (ref.Target == 0) {
    return {shareGroup, ref.Name};
  }
  const GLenum pname = BufferBindingPname(ref.Target);
  const GLuint buffer = pname != 0 ? static_cast<GLuint>(GetInteger(pname)) : 0u;
  return {shareGroup, buffer};
}

void BufferShadowService::MarkUntrusted(const Key& key, const char* unhookedWriter) {
  const char*& writer = m_Untrusted[key];
  if (writer == nullptr) {
    writer = unhookedWriter;
  }
  m_Shadows.erase(key);
}

void BufferShadowService::OnData(const char* command,
                                 BufferRef ref,
                                 uint64_t size,
                                 const void* data) {
  const bool clientMemoryTarget = IsClientMemoryTarget(ref.Target);
  // Without a staging copy a buffer never seen on GL_ELEMENT_ARRAY_BUFFER could not be read later
  // if it gets bound there, so every upload is kept.
  const bool retain = ref.Target == GL_ELEMENT_ARRAY_BUFFER ||
                      (!clientMemoryTarget && !ContextSupportsCopyBuffer());
  if (!retain && !clientMemoryTarget) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Shadows.empty()) {
      return;
    }
  }
  const Key key = Resolve(ref);
  if (key.Buffer == 0) {
    return;
  }
  if (clientMemoryTarget) {
    MapTrackingService::Get().AssertNoWritablePersistentMap(command, ref.Target, key.ShareGroup,
                                                            key.Buffer);
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (clientMemoryTarget) {
    MarkUntrusted(key, command);
    return;
  }
  if (m_Untrusted.count(key) != 0) {
    return;
  }
  auto it = m_Shadows.find(key);
  if (it == m_Shadows.end()) {
    if (!retain) {
      return;
    }
    it = m_Shadows.try_emplace(key).first;
  }
  Shadow& shadow = it->second;
  if (data != nullptr) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    shadow.Bytes.assign(bytes, bytes + size);
    shadow.Known = true;
  } else {
    shadow.Bytes.assign(size, 0);
    shadow.Known = false;
  }
  shadow.KnownBeforeMap = false;
}

void BufferShadowService::BeforeSubData(BufferRef ref,
                                        uint64_t offset,
                                        uint64_t size,
                                        const void* data) {
  if (!MapTrackingService::Get().MayDeferInvalidation()) {
    return;
  }
  const Key key = Resolve(ref);
  if (key.Buffer != 0) {
    MapTrackingService::Get().PublishInvalidationBeforeSubData(key.ShareGroup, key.Buffer, offset,
                                                               size, data);
  }
}

void BufferShadowService::OnSubData(BufferRef ref,
                                    uint64_t offset,
                                    uint64_t size,
                                    const void* data) {
  const Key key = Resolve(ref);
  MapTrackingService::Get().OnSubData(key.ShareGroup, key.Buffer, offset, size, data);
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Shadows.find(key);
  if (it == m_Shadows.end()) {
    return;
  }
  Shadow& shadow = it->second;
  if (data == nullptr || offset + size > shadow.Bytes.size()) {
    shadow.Known = false;
    return;
  }
  std::memcpy(shadow.Bytes.data() + offset, data, size);
}

void BufferShadowService::OnGpuWrite(const char* command, BufferRef ref) {
  const Key key = Resolve(ref);
  if (key.Buffer == 0) {
    return;
  }
  MarkUntrustedBeforeMapCheck(key);
  MapTrackingService::Get().AssertNoWritablePersistentMap(command, ref.Target, key.ShareGroup,
                                                          key.Buffer);
}

void BufferShadowService::OnGpuResultWrite(const char* command, BufferRef ref) {
  const Key key = Resolve(ref);
  if (key.Buffer == 0) {
    return;
  }
  MarkUntrustedBeforeMapCheck(key);
  MapTrackingService::Get().OnGpuResultWrite(command, ref.Target, key.ShareGroup, key.Buffer);
}

void BufferShadowService::OnUnhookedGpuWriter(const char* command, BufferRef ref) {
  const Key key = Resolve(ref);
  if (key.Buffer == 0) {
    return;
  }
  MarkUntrustedBeforeMapCheck(key, command);
  MapTrackingService::Get().AssertNoWritablePersistentMap(command, ref.Target, key.ShareGroup,
                                                          key.Buffer);
}

void BufferShadowService::MarkUntrustedBeforeMapCheck(const Key& key, const char* unhookedWriter) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  MarkUntrusted(key, unhookedWriter);
}

void BufferShadowService::OnUndefined(BufferRef ref) {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Shadows.empty()) {
      return;
    }
  }
  const Key key = Resolve(ref);
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Shadows.find(key);
  if (it != m_Shadows.end()) {
    it->second.Known = false;
  }
}

void BufferShadowService::OnBind(const char* command, GLenum target, GLuint buffer) {
  // The GPU writes the buffer bound here (pixel packs, query results); every other generic
  // binding point only reads, or needs an indexed binding to write.
  if (buffer == 0 || (target != GL_PIXEL_PACK_BUFFER && target != GL_QUERY_BUFFER &&
                      !IsClientMemoryTarget(target))) {
    return;
  }
  const Key key{CurrentShareGroup(), buffer};
  MarkUntrustedBeforeMapCheck(key, IsClientMemoryTarget(target) ? command : nullptr);
  if (IsClientMemoryTarget(target)) {
    MapTrackingService::Get().AssertNoWritablePersistentMap(command, target, key.ShareGroup,
                                                            key.Buffer);
  } else {
    MapTrackingService::Get().OnGpuResultWrite(command, target, key.ShareGroup, key.Buffer);
  }
}

void BufferShadowService::OnBindIndexed(const char* command, GLenum target, GLuint buffer) {
  OnBindIndexed(command, target, 1, &buffer);
}

void BufferShadowService::OnBindIndexed(const char* command,
                                        GLenum target,
                                        GLsizei count,
                                        const GLuint* buffers) {
  if (buffers == nullptr ||
      (target != GL_TRANSFORM_FEEDBACK_BUFFER && target != GL_SHADER_STORAGE_BUFFER &&
       target != GL_ATOMIC_COUNTER_BUFFER)) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (GLsizei i = 0; i < count; ++i) {
      if (buffers[i] != 0) {
        MarkUntrusted({shareGroup, buffers[i]});
      }
    }
  }
  // After marking them, as for MarkUntrustedBeforeMapCheck().
  for (GLsizei i = 0; i < count; ++i) {
    if (buffers[i] != 0) {
      MapTrackingService::Get().OnGpuResultWrite(command, target, shareGroup, buffers[i]);
    }
  }
}

void BufferShadowService::OnDelete(GLsizei n, const GLuint* buffers) {
  if (buffers == nullptr) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (GLsizei i = 0; i < n; ++i) {
    const Key key{shareGroup, buffers[i]};
    m_Shadows.erase(key);
    m_Untrusted.erase(key);
  }
}

bool BufferShadowService::IsUntrusted(uint64_t shareGroup, GLuint buffer) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Untrusted.count({shareGroup, buffer}) != 0;
}

const char* BufferShadowService::UnhookedGpuWriter(uint64_t shareGroup, GLuint buffer) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = m_Untrusted.find({shareGroup, buffer});
  return it != m_Untrusted.end() ? it->second : nullptr;
}

void BufferShadowService::OnMapWrite(uint64_t shareGroup, GLuint buffer, GLbitfield access) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Shadows.find({shareGroup, buffer});
  if (it != m_Shadows.end()) {
    it->second.KnownBeforeMap =
        it->second.Known &&
        (access & (GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT)) == 0;
    it->second.Known = false;
  }
}

void BufferShadowService::OnMapUnchanged(uint64_t shareGroup, GLuint buffer) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Shadows.find({shareGroup, buffer});
  if (it != m_Shadows.end()) {
    it->second.Known = it->second.KnownBeforeMap;
    it->second.KnownBeforeMap = false;
  }
}

void BufferShadowService::OnMappedBytes(uint64_t shareGroup,
                                        GLuint buffer,
                                        uint64_t offset,
                                        std::span<const uint8_t> bytes) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Shadows.find({shareGroup, buffer});
  if (it == m_Shadows.end()) {
    return;
  }
  Shadow& shadow = it->second;
  if (offset + bytes.size() > shadow.Bytes.size()) {
    shadow.Known = false;
    return;
  }
  std::memcpy(shadow.Bytes.data() + offset, bytes.data(), bytes.size());
  shadow.Known = shadow.KnownBeforeMap || (offset == 0 && bytes.size() == shadow.Bytes.size());
}

BufferShadowService::ReadSource BufferShadowService::ReadElementBuffer(
    bool copyBuffer, bool mapped, uint64_t offset, uint64_t size, std::vector<uint8_t>& out) {
  out.clear();
  const Key key{CurrentShareGroup(),
                static_cast<GLuint>(GetInteger(GL_ELEMENT_ARRAY_BUFFER_BINDING))};
  if (key.Buffer == 0 || size == 0) {
    return ReadSource::Failed;
  }
  bool untrusted = false;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    untrusted = m_Untrusted.count(key) != 0;
    auto it = m_Shadows.find(key);
    if (it != m_Shadows.end() && it->second.Known && offset <= it->second.Bytes.size() &&
        size <= it->second.Bytes.size() - offset) {
      const uint8_t* begin = it->second.Bytes.data() + offset;
      out.assign(begin, begin + size);
      return ReadSource::Copy;
    }
  }
  if (!copyBuffer) {
    return ReadSource::Failed;
  }
  GLint64 bufferSize = 0;
  if (!GetBoundBufferSize(GL_ELEMENT_ARRAY_BUFFER, bufferSize) || bufferSize < 0 ||
      offset > static_cast<uint64_t>(bufferSize) ||
      size > static_cast<uint64_t>(bufferSize) - offset) {
    return ReadSource::Failed;
  }
  if (untrusted || mapped) {
    return ReadThroughStaging(key.ShareGroup, offset, size, out) ? ReadSource::Staging
                                                                 : ReadSource::Failed;
  }
  std::vector<uint8_t> whole;
  if (!ReadThroughStaging(key.ShareGroup, 0, static_cast<uint64_t>(bufferSize), whole)) {
    return ReadSource::Failed;
  }
  out.assign(whole.begin() + offset, whole.begin() + offset + size);
  std::lock_guard<std::mutex> lock(m_Mutex);
  Shadow& shadow = m_Shadows[key];
  shadow.Bytes = std::move(whole);
  shadow.Known = true;
  shadow.KnownBeforeMap = false;
  return ReadSource::Staging;
}

bool BufferShadowService::ReadThroughStaging(uint64_t shareGroup,
                                             uint64_t offset,
                                             uint64_t size,
                                             std::vector<uint8_t>& out) {
  const StagingKey stagingKey{shareGroup, reinterpret_cast<uintptr_t>(CurrentContext())};
  // ARB_copy_buffer can come with only ARB_vertex_buffer_object before GL 1.5, whose context
  // resolves only the ARB-suffixed buffer entry points.
  const bool coreBuffers = GlVersionAtLeast(CurrentContextGLVersion(), 1, 5);
  GLuint staging = 0;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    GLuint& cached = m_Staging[stagingKey];
    if (cached == 0) {
      auto genBuffers = coreBuffers
                            ? g_DispatchTable.glGenBuffers.load(std::memory_order_acquire)
                            : g_DispatchTable.glGenBuffersARB.load(std::memory_order_acquire);
      genBuffers(1, &cached);
    }
    staging = cached;
  }
  if (staging == 0) {
    return false;
  }
  auto bindBuffer = coreBuffers ? g_DispatchTable.glBindBuffer.load(std::memory_order_acquire)
                                : g_DispatchTable.glBindBufferARB.load(std::memory_order_acquire);
  auto bufferData = coreBuffers ? g_DispatchTable.glBufferData.load(std::memory_order_acquire)
                                : g_DispatchTable.glBufferDataARB.load(std::memory_order_acquire);
  auto getBufferSubData =
      coreBuffers ? g_DispatchTable.glGetBufferSubData.load(std::memory_order_acquire)
                  : g_DispatchTable.glGetBufferSubDataARB.load(std::memory_order_acquire);
  const auto previous = static_cast<GLuint>(GetInteger(GL_COPY_WRITE_BUFFER_BINDING));
  bindBuffer(GL_COPY_WRITE_BUFFER, staging);
  bufferData(GL_COPY_WRITE_BUFFER, static_cast<GLsizeiptr>(size), nullptr, GL_STREAM_READ);
  g_DispatchTable.glCopyBufferSubData.load(std::memory_order_acquire)(
      GL_ELEMENT_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(offset), 0,
      static_cast<GLsizeiptr>(size));
  out.resize(size);
  getBufferSubData(GL_COPY_WRITE_BUFFER, 0, static_cast<GLsizeiptr>(size), out.data());
  bindBuffer(GL_COPY_WRITE_BUFFER, previous);
  return true;
}

} // namespace gits::opengl
