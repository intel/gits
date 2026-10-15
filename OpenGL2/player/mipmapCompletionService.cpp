// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mipmapCompletionService.h"

#include "glEnumsAuto.h"
#include "openglDriversAuto.h"
#include "pixelTransferSize.h"
#include "replayContextCaps.h"

#include <cstring>

namespace gits::opengl2 {

namespace {
// Only invoked to switch away from the active unit, which cannot happen without MultiTexture:
// then the only unit is GL_TEXTURE0.
player::OpenGLDispatchTable::glActiveTextureARBPtr ActiveTextureFunction(
    const ReplayContextCaps& caps) {
  auto& table = player::g_DispatchTable;
  return caps.MultiTextureCore ? table.glActiveTexture.load(std::memory_order_acquire)
                               : table.glActiveTextureARB.load(std::memory_order_acquire);
}

// The non-mipmap filter sampling exactly like `minFilter` when only level 0 can be selected, or 0
// if `minFilter` is not a mipmap filter.
GLint NonMipmapFilter(GLint minFilter) {
  switch (minFilter) {
  case GL_NEAREST_MIPMAP_NEAREST:
  case GL_NEAREST_MIPMAP_LINEAR:
    return GL_NEAREST;
  case GL_LINEAR_MIPMAP_NEAREST:
  case GL_LINEAR_MIPMAP_LINEAR:
    return GL_LINEAR;
  default:
    return 0;
  }
}

// The display list being compiled on the current context, or 0.
GLuint CompiledList(const ReplayContextCaps& caps) {
  if (!caps.DisplayLists) {
    return 0;
  }
  GLint list = 0;
  player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_LIST_INDEX, &list);
  return static_cast<GLuint>(list);
}

// Between glNewList and glEndList, glActiveTexture and glTexParameteri would be compiled into
// the application's list instead of, or as well as, running now. glGetIntegerv never is.
bool CompilingDisplayList(const ReplayContextCaps& caps) {
  return CompiledList(caps) != 0;
}

// The list compiled on this thread's context, as reported by the driver after glNewList and
// glEndList.
thread_local GLuint t_CompilingList = 0;

// The lists glCallLists executes: GL_LIST_BASE plus each offset of `type`.
std::vector<GLuint> ListNames(GLsizei n, GLenum type, const uint8_t* lists, size_t size) {
  std::vector<GLuint> names;
  const uint64_t bytes = ComputeListNameArrayByteSize(n, type);
  if (lists == nullptr || n <= 0 || bytes == 0 || bytes > size ||
      !CurrentReplayContextCaps().DisplayLists) {
    return names;
  }
  GLint base = 0;
  player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_LIST_BASE, &base);
  const size_t elementBytes = static_cast<size_t>(bytes / static_cast<uint64_t>(n));
  names.reserve(static_cast<size_t>(n));
  for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
    const uint8_t* element = lists + i * elementBytes;
    int64_t offset = 0;
    switch (type) {
    case GL_BYTE:
      offset = static_cast<int8_t>(element[0]);
      break;
    case GL_SHORT: {
      int16_t value = 0;
      std::memcpy(&value, element, sizeof(value));
      offset = value;
      break;
    }
    case GL_UNSIGNED_SHORT: {
      uint16_t value = 0;
      std::memcpy(&value, element, sizeof(value));
      offset = value;
      break;
    }
    case GL_INT: {
      int32_t value = 0;
      std::memcpy(&value, element, sizeof(value));
      offset = value;
      break;
    }
    case GL_UNSIGNED_INT: {
      uint32_t value = 0;
      std::memcpy(&value, element, sizeof(value));
      offset = value;
      break;
    }
    case GL_FLOAT: {
      float value = 0;
      std::memcpy(&value, element, sizeof(value));
      offset = value >= 0.0f && value < 4294967296.0f ? static_cast<int64_t>(value) : 0;
      break;
    }
    default: // GL_UNSIGNED_BYTE and GL_2_BYTES..GL_4_BYTES: big-endian bytes.
      for (size_t b = 0; b < elementBytes; ++b) {
        offset = offset * 256 + element[b];
      }
      break;
    }
    names.push_back(static_cast<GLuint>(static_cast<int64_t>(base) + offset));
  }
  return names;
}
} // namespace

MipmapCompletionService& MipmapCompletionService::Get() {
  static MipmapCompletionService instance;
  return instance;
}

void MipmapCompletionService::NoteUnit(GLuint unit) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Units.insert(unit);
}

void MipmapCompletionService::BeforeSampling() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  BeforeSamplingLocked();
}

void MipmapCompletionService::BeforeCallList(GLuint list) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (t_CompilingList != 0) {
    m_Lists[t_CompilingList].Calls.insert(list);
    return;
  }
  BeforeSamplingLocked();
  OverrideListTextures({list});
}

void MipmapCompletionService::BeforeCallLists(GLsizei n,
                                              GLenum type,
                                              const uint8_t* lists,
                                              size_t size) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const std::vector<GLuint> names =
      m_InBeginEnd ? std::vector<GLuint>{} : ListNames(n, type, lists, size);
  if (t_CompilingList != 0) {
    m_Lists[t_CompilingList].Calls.insert(names.begin(), names.end());
    return;
  }
  BeforeSamplingLocked();
  OverrideListTextures(names);
}

void MipmapCompletionService::BeginListCompile(GLuint list) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  t_CompilingList = m_InBeginEnd ? 0 : CompiledList(CurrentReplayContextCaps());
  if (t_CompilingList == list) {
    m_Lists.erase(list);
  }
}

void MipmapCompletionService::EndListCompile() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_InBeginEnd) {
    t_CompilingList = CompiledList(CurrentReplayContextCaps());
  }
}

void MipmapCompletionService::NoteListTexture(GLuint texture) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (t_CompilingList != 0 && texture != 0) {
    m_Lists[t_CompilingList].Textures.insert(texture);
  }
}

void MipmapCompletionService::BeforeSamplingLocked() {
  if (m_InBeginEnd || m_Units.empty()) {
    return;
  }
  // A list being compiled is fixed when glCallList executes it, itself a sampling command.
  const ReplayContextCaps& caps = CurrentReplayContextCaps();
  if (CompilingDisplayList(caps)) {
    return;
  }
  // Left over only if a glBegin never reached its glEnd.
  RestoreFilters();

  if (!caps.TextureMaxLevel) {
    return; // Without GL_TEXTURE_MAX_LEVEL the edge case cannot arise.
  }
  auto& table = player::g_DispatchTable;
  auto realGetIntegerv = table.glGetIntegerv.load(std::memory_order_acquire);
  auto realActiveTexture = ActiveTextureFunction(caps);

  // A noted unit can be out of range for this context (its bind failed, or it was noted on
  // another context), and would fail the glActiveTexture below too, leaving the queries and the
  // override on whichever unit is active instead.
  const GLint unitCount = caps.TextureUnits;

  GLint savedActive = GL_TEXTURE0;
  if (caps.MultiTexture) {
    realGetIntegerv(GL_ACTIVE_TEXTURE, &savedActive);
  }
  auto activeUnit = static_cast<GLenum>(savedActive);
  for (GLuint unit : m_Units) {
    if (unit >= static_cast<GLuint>(unitCount)) {
      break; // m_Units is ordered, so every later unit is out of range too.
    }
    const auto targetUnit = static_cast<GLenum>(GL_TEXTURE0 + unit);
    if (activeUnit != targetUnit) {
      realActiveTexture(targetUnit);
      activeUnit = targetUnit;
    }
    OverrideBoundTexture2D(targetUnit);
  }
  if (activeUnit != static_cast<GLenum>(savedActive)) {
    realActiveTexture(static_cast<GLenum>(savedActive));
  }
}

void MipmapCompletionService::AfterSampling() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_InBeginEnd) {
    RestoreFilters();
  }
}

void MipmapCompletionService::EnterBeginEnd() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_InBeginEnd = true;
}

void MipmapCompletionService::LeaveBeginEnd() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_InBeginEnd = false;
  RestoreFilters();
}

void MipmapCompletionService::OverrideBoundTexture2D(GLenum unit) {
  auto& table = player::g_DispatchTable;
  auto realGetIntegerv = table.glGetIntegerv.load(std::memory_order_acquire);
  auto realGetTexParameteriv = table.glGetTexParameteriv.load(std::memory_order_acquire);
  auto realGetTexLevelParameteriv = table.glGetTexLevelParameteriv.load(std::memory_order_acquire);

  GLint boundTexture = 0;
  realGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
  if (boundTexture == 0) {
    return;
  }
  // Only the state the workaround is for. With a higher max level, mipmap filters can select
  // other levels, and missing ones make the texture incomplete by spec. Checked first, since it
  // rules out almost every texture.
  GLint maxLevel = 0;
  realGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  if (maxLevel != 0) {
    return;
  }
  // A bound sampler object's own GL_TEXTURE_MIN_FILTER overrides the texture's own parameter of
  // the same name for sampling purposes. GL_SAMPLER_BINDING (unlike GL_TEXTURE_BINDING_2D) has
  // no per-target variant - there is exactly one sampler binding per unit, independent of which
  // texture target is bound there.
  GLint boundSampler = 0;
  if (CurrentReplayContextCaps().SamplerObjects) {
    realGetIntegerv(GL_SAMPLER_BINDING, &boundSampler);
  }
  FilterOverride filter;
  filter.IsSampler = boundSampler != 0;
  filter.Name = static_cast<GLuint>(filter.IsSampler ? boundSampler : boundTexture);
  filter.Unit = unit;
  if (filter.IsSampler) {
    table.glGetSamplerParameteriv.load(std::memory_order_acquire)(
        filter.Name, GL_TEXTURE_MIN_FILTER, &filter.Original);
  } else {
    realGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter.Original);
  }
  filter.Substitute = NonMipmapFilter(filter.Original);
  if (filter.Substitute == 0) {
    return; // Not a mipmap filter, or already overridden through another unit.
  }
  GLint level1Width = 0;
  realGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &level1Width);
  if (level1Width != 0) {
    return;
  }
  if (filter.IsSampler) {
    table.glSamplerParameteri.load(std::memory_order_acquire)(filter.Name, GL_TEXTURE_MIN_FILTER,
                                                              filter.Substitute);
  } else {
    table.glTexParameteri.load(std::memory_order_acquire)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                                          filter.Substitute);
  }
  m_Overrides.push_back(filter);
}

void MipmapCompletionService::OverrideListTextures(const std::vector<GLuint>& lists) {
  if (m_InBeginEnd || m_Lists.empty() || lists.empty()) {
    return;
  }
  const ReplayContextCaps& caps = CurrentReplayContextCaps();
  if (!caps.TextureMaxLevel) {
    return;
  }
  std::set<GLuint> textures;
  std::set<GLuint> visited;
  std::vector<GLuint> pending(lists.begin(), lists.end());
  while (!pending.empty()) {
    const GLuint list = pending.back();
    pending.pop_back();
    const auto it = m_Lists.find(list);
    if (!visited.insert(list).second || it == m_Lists.end()) {
      continue;
    }
    textures.insert(it->second.Textures.begin(), it->second.Textures.end());
    pending.insert(pending.end(), it->second.Calls.begin(), it->second.Calls.end());
  }
  if (textures.empty()) {
    return;
  }
  GLint activeUnit = GL_TEXTURE0;
  if (caps.MultiTexture) {
    player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_ACTIVE_TEXTURE,
                                                                          &activeUnit);
  }
  for (GLuint texture : textures) {
    OverrideListTexture(texture, static_cast<GLenum>(activeUnit));
  }
}

void MipmapCompletionService::OverrideListTexture(GLuint texture, GLenum activeUnit) {
  auto& table = player::g_DispatchTable;
  // A name the list's own bind creates has no levels yet, and binding it here would create it.
  if (!table.glIsTexture.load(std::memory_order_acquire)(texture)) {
    return;
  }
  const bool direct = CurrentReplayContextCaps().DirectStateAccess;
  auto realBindTexture = table.glBindTexture.load(std::memory_order_acquire);
  GLint bound = 0;
  if (!direct) {
    // The list binds it to GL_TEXTURE_2D, so it is a 2D texture if the list executes correctly.
    table.glGetIntegerv.load(std::memory_order_acquire)(GL_TEXTURE_BINDING_2D, &bound);
    if (static_cast<GLuint>(bound) != texture) {
      realBindTexture(GL_TEXTURE_2D, texture);
    }
  }
  const auto parameter = [&](GLenum pname) {
    GLint value = 0;
    if (direct) {
      table.glGetTextureParameteriv.load(std::memory_order_acquire)(texture, pname, &value);
    } else {
      table.glGetTexParameteriv.load(std::memory_order_acquire)(GL_TEXTURE_2D, pname, &value);
    }
    return value;
  };
  FilterOverride filter;
  filter.Name = texture;
  filter.Unit = activeUnit;
  if (parameter(GL_TEXTURE_MAX_LEVEL) == 0) {
    filter.Original = parameter(GL_TEXTURE_MIN_FILTER);
    filter.Substitute = NonMipmapFilter(filter.Original);
  }
  if (filter.Substitute != 0) {
    GLint level1Width = 0;
    if (direct) {
      table.glGetTextureLevelParameteriv.load(std::memory_order_acquire)(
          texture, 1, GL_TEXTURE_WIDTH, &level1Width);
    } else {
      table.glGetTexLevelParameteriv.load(std::memory_order_acquire)(
          GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &level1Width);
    }
    if (level1Width == 0) {
      if (direct) {
        table.glTextureParameteri.load(std::memory_order_acquire)(texture, GL_TEXTURE_MIN_FILTER,
                                                                  filter.Substitute);
      } else {
        table.glTexParameteri.load(std::memory_order_acquire)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                                              filter.Substitute);
      }
      m_Overrides.push_back(filter);
    }
  }
  if (!direct && static_cast<GLuint>(bound) != texture) {
    realBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(bound));
  }
}

void MipmapCompletionService::RestoreFilters() {
  if (m_Overrides.empty()) {
    return;
  }
  const ReplayContextCaps& caps = CurrentReplayContextCaps();
  if (CompilingDisplayList(caps)) {
    return; // Restored by the first sampling command after glEndList.
  }
  auto& table = player::g_DispatchTable;
  auto realGetIntegerv = table.glGetIntegerv.load(std::memory_order_acquire);
  auto realActiveTexture = ActiveTextureFunction(caps);
  auto realGetTexParameteriv = table.glGetTexParameteriv.load(std::memory_order_acquire);
  auto realTexParameteri = table.glTexParameteri.load(std::memory_order_acquire);

  GLint savedActive = GL_TEXTURE0;
  if (caps.MultiTexture) {
    realGetIntegerv(GL_ACTIVE_TEXTURE, &savedActive);
  }
  auto activeUnit = static_cast<GLenum>(savedActive);
  // A filter the sampling command itself changed (a display list can) is the application's now,
  // and is left alone.
  for (const FilterOverride& filter : m_Overrides) {
    GLint current = 0;
    if (filter.IsSampler) {
      table.glGetSamplerParameteriv.load(std::memory_order_acquire)(
          filter.Name, GL_TEXTURE_MIN_FILTER, &current);
      if (current == filter.Substitute) {
        table.glSamplerParameteri.load(std::memory_order_acquire)(
            filter.Name, GL_TEXTURE_MIN_FILTER, filter.Original);
      }
      continue;
    }
    if (activeUnit != filter.Unit) {
      realActiveTexture(filter.Unit);
      activeUnit = filter.Unit;
    }
    GLint boundTexture = 0;
    realGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
    if (static_cast<GLuint>(boundTexture) == filter.Name) {
      realGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &current);
      if (current == filter.Substitute) {
        realTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter.Original);
      }
    } else if (caps.DirectStateAccess) {
      // The sampling command rebound the unit (a display list can); reach the texture directly.
      table.glGetTextureParameteriv.load(std::memory_order_acquire)(
          filter.Name, GL_TEXTURE_MIN_FILTER, &current);
      if (current == filter.Substitute) {
        table.glTextureParameteri.load(std::memory_order_acquire)(
            filter.Name, GL_TEXTURE_MIN_FILTER, filter.Original);
      }
    } else if (table.glIsTexture.load(std::memory_order_acquire)(filter.Name)) {
      // Rebound, and no direct access: bind the texture briefly. Skipped if the display list
      // deleted it, since binding would create a new texture under its name.
      auto realBindTexture = table.glBindTexture.load(std::memory_order_acquire);
      realBindTexture(GL_TEXTURE_2D, filter.Name);
      realGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &current);
      if (current == filter.Substitute) {
        realTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter.Original);
      }
      realBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(boundTexture));
    }
  }
  if (activeUnit != static_cast<GLenum>(savedActive)) {
    realActiveTexture(static_cast<GLenum>(savedActive));
  }
  m_Overrides.clear();
}

} // namespace gits::opengl2
