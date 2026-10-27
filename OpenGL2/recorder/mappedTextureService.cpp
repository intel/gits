// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mappedTextureService.h"

#include "captureManager.h"
#include "clientMemoryCapture.h"
#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedTextureDataMetaTypes.h"
#include "openglDriversAuto.h"
#include "systemApi.h"

#include "xxhash.h"

namespace gits::opengl2 {

namespace {

struct LevelSize {
  GLint Height{0};
  GLint Compressed{GL_FALSE};
  GLint InternalFormat{0};
  GLint CompressedImageSize{0};
};

// Queried through DSA when the context has it; otherwise through a temporary GL_TEXTURE_2D bind
// on the active unit, restored afterwards.
LevelSize QueryLevelSize(GLuint texture, GLint level) {
  LevelSize size;
  auto query = [&](auto get, auto target) {
    get(target, level, GL_TEXTURE_HEIGHT, &size.Height);
    get(target, level, GL_TEXTURE_COMPRESSED, &size.Compressed);
    if (size.Compressed != GL_FALSE) {
      get(target, level, GL_TEXTURE_INTERNAL_FORMAT, &size.InternalFormat);
      get(target, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size.CompressedImageSize);
    }
  };
  if (ContextSupportsDirectStateAccess()) {
    query(g_DispatchTable.glGetTextureLevelParameteriv.load(std::memory_order_acquire), texture);
    return size;
  }
  GLint previous = 0;
  g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(GL_TEXTURE_BINDING_2D, &previous);
  auto bind = g_DispatchTable.glBindTexture.load(std::memory_order_acquire);
  bind(GL_TEXTURE_2D, texture);
  query(g_DispatchTable.glGetTexLevelParameteriv.load(std::memory_order_acquire),
        static_cast<GLenum>(GL_TEXTURE_2D));
  bind(GL_TEXTURE_2D, static_cast<GLuint>(previous));
  return size;
}

// A compressed mapping's stride covers one row of blocks, so the row count is the level's height
// in blocks. Block heights differ per format (ASTC has several), so the height comes from the
// format itself; without ARB_internalformat_query2 the rows are derived from the level's
// compressed image size instead.
GLint MappedRows(const LevelSize& size, GLint stride) {
  if (size.Compressed == GL_FALSE) {
    return size.Height;
  }
  GLint blockHeight = 0;
  if (ContextSupportsInternalformatQuery2()) {
    g_DispatchTable.glGetInternalformativ.load(std::memory_order_acquire)(
        GL_TEXTURE_2D, static_cast<GLenum>(size.InternalFormat), GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT,
        1, &blockHeight);
  }
  if (blockHeight > 0) {
    return (size.Height + blockHeight - 1) / blockHeight;
  }
  return stride > 0 ? (size.CompressedImageSize + stride - 1) / stride : 0;
}

size_t MappingBytes(GLint stride, GLint rows) {
  return static_cast<size_t>(stride) * static_cast<size_t>(rows);
}

} // namespace

MappedTextureService& MappedTextureService::Get() {
  static MappedTextureService instance;
  return instance;
}

void MappedTextureService::OnMap(
    GLuint texture, GLint level, GLbitfield access, void* pointer, GLint stride) {
  if (pointer == nullptr || (access & GL_MAP_WRITE_BIT) == 0) {
    return;
  }
  const GLint rows = MappedRows(QueryLevelSize(texture, level), stride);
  if (stride <= 0 || rows <= 0) {
    LOG_WARNING << "MappedTextureService: glMapTexture2DINTEL of texture " << texture << " level "
                << level << " has stride " << stride << " and " << rows
                << " rows - its contents are not captured";
    return;
  }
  Mapping mapping;
  mapping.Pointer = static_cast<const uint8_t*>(pointer);
  mapping.Stride = stride;
  mapping.Rows = rows;
  mapping.Hash = XXH3_64bits(mapping.Pointer, MappingBytes(stride, rows));
  const std::lock_guard lock(m_Mutex);
  m_Mappings[Key{CurrentShareGroup(), texture, level}] = mapping;
}

void MappedTextureService::CaptureOnSync(GLuint texture) {
  const uint64_t shareGroup = CurrentShareGroup();
  const std::lock_guard lock(m_Mutex);
  std::vector<Entry*> entries;
  for (auto& entry : m_Mappings) {
    if (entry.first.ShareGroup == shareGroup && entry.first.Texture == texture) {
      entries.push_back(&entry);
    }
  }
  if (!entries.empty()) {
    CaptureChanged(entries, CaptureManager::Get().CreateCommandKey());
  }
}

void MappedTextureService::CaptureOnUnmap(GLuint texture, GLint level) {
  const std::lock_guard lock(m_Mutex);
  auto it = m_Mappings.find(Key{CurrentShareGroup(), texture, level});
  if (it == m_Mappings.end()) {
    return;
  }
  CaptureChanged({&*it}, CaptureManager::Get().CreateCommandKey());
  m_Mappings.erase(it);
}

void MappedTextureService::CaptureOnSwap(uint64_t key) {
  const uint64_t shareGroup = CurrentShareGroup();
  const std::lock_guard lock(m_Mutex);
  std::vector<Entry*> entries;
  for (auto& entry : m_Mappings) {
    if (entry.first.ShareGroup == shareGroup) {
      entries.push_back(&entry);
    }
  }
  CaptureChanged(entries, key);
}

void MappedTextureService::OnDelete(GLsizei n, const GLuint* textures) {
  if (n <= 0 || textures == nullptr) {
    return;
  }
  const uint64_t shareGroup = CurrentShareGroup();
  const std::lock_guard lock(m_Mutex);
  if (m_Mappings.empty()) {
    return;
  }
  for (GLsizei i = 0; i < n; ++i) {
    std::erase_if(m_Mappings, [&](const auto& entry) {
      return entry.first.ShareGroup == shareGroup && entry.first.Texture == textures[i];
    });
  }
}

void MappedTextureService::CaptureChanged(const std::vector<Entry*>& entries, uint64_t key) {
  std::vector<GLuint> textures;
  std::vector<GLint> levels;
  std::vector<GLint> strides;
  std::vector<GLint> rows;
  std::vector<uint8_t> data;
  for (Entry* entry : entries) {
    Mapping& mapping = entry->second;
    const size_t bytes = MappingBytes(mapping.Stride, mapping.Rows);
    const uint64_t hash = XXH3_64bits(mapping.Pointer, bytes);
    if (hash == mapping.Hash) {
      continue;
    }
    mapping.Hash = hash;
    textures.push_back(entry->first.Texture);
    levels.push_back(entry->first.Level);
    strides.push_back(mapping.Stride);
    rows.push_back(mapping.Rows);
    data.insert(data.end(), mapping.Pointer, mapping.Pointer + bytes);
  }
  if (textures.empty()) {
    CaptureManager::Get().SkipCommandKey(key);
    return;
  }

  MappedTextureDataMetaCommand command(CurrentThreadId());
  command.m_Textures = HandleArrayArgument<GLuint>(textures.data(), textures.size());
  UpdateHandleArray(command.m_Textures, GLObjectKind::Texture);
  command.m_Levels = ArrayArgument<GLint>(levels.data(), levels.size());
  command.m_Strides = ArrayArgument<GLint>(strides.data(), strides.size());
  command.m_Rows = ArrayArgument<GLint>(rows.data(), rows.size());
  command.m_Data.Data = std::move(data);
  command.m_Data.HasValue = true;
  command.m_Key = key;
  CaptureManager::Get().RecordCommand(command, new MappedTextureDataMetaSerializer(command));
}

} // namespace gits::opengl2
