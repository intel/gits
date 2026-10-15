// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mappedTextureDataMetaRunner.h"

#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "layerAuto.h"
#include "log.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "playerManager.h"

#include <algorithm>
#include <cstring>
#include <mutex>

namespace gits::opengl2 {

namespace {
void ReplayLevel(
    GLuint texture, GLint level, GLint capturedStride, GLint rows, const uint8_t* source) {
  auto map = player::g_DispatchTable.glMapTexture2DINTEL.load(std::memory_order_acquire);
  GLint stride = 0;
  GLenum layout = 0;
  auto* pointer = static_cast<uint8_t*>(map(texture, level, GL_MAP_WRITE_BIT, &stride, &layout));
  if (pointer == nullptr || stride <= 0) {
    static std::once_flag warned;
    std::call_once(warned, [] {
      LOG_ERROR << "MappedTextureDataMetaRunner: glMapTexture2DINTEL failed - mapped texture "
                   "updates are not replayed (GL_INTEL_map_texture is required)";
    });
    if (pointer != nullptr) {
      player::g_DispatchTable.glUnmapTexture2DINTEL.load(std::memory_order_acquire)(texture, level);
    }
    return;
  }

  const size_t rowBytes = static_cast<size_t>((std::min)(capturedStride, stride));
  for (GLint row = 0; row < rows; ++row) {
    std::memcpy(pointer + static_cast<size_t>(row) * static_cast<size_t>(stride),
                source + static_cast<size_t>(row) * static_cast<size_t>(capturedStride), rowBytes);
  }
  player::g_DispatchTable.glUnmapTexture2DINTEL.load(std::memory_order_acquire)(texture, level);
}

// Levels of textures the recorder never saw created resolve to name 0 and are left out.
void ReplayMappedTextureData(const MappedTextureDataMetaCommand& command) {
  const size_t count = command.m_Textures.Value.size();
  if (command.m_Levels.Data.size() != count || command.m_Strides.Data.size() != count ||
      command.m_Rows.Data.size() != count) {
    LOG_WARNING << "MappedTextureDataMetaRunner: malformed update - skipping it";
    return;
  }
  size_t offset = 0;
  for (size_t i = 0; i < count; ++i) {
    const GLint stride = command.m_Strides.Data[i];
    const GLint rows = command.m_Rows.Data[i];
    const size_t bytes = static_cast<size_t>(stride) * static_cast<size_t>(rows);
    if (stride <= 0 || rows <= 0 || command.m_Data.Data.size() - offset < bytes) {
      LOG_WARNING << "MappedTextureDataMetaRunner: malformed update - skipping it";
      return;
    }
    if (command.m_Textures.Value[i] != 0) {
      ReplayLevel(command.m_Textures.Value[i], command.m_Levels.Data[i], stride, rows,
                  command.m_Data.Data.data() + offset);
    }
    offset += bytes;
  }
}
} // namespace

void MappedTextureDataMetaRunner::Run() {
  auto& manager = PlayerManager::Get();
  if (!UpdateHandleArray(command.m_Textures, GLObjectKind::Texture)) {
    static std::once_flag warned;
    std::call_once(warned, [] {
      LOG_WARNING << "MappedTextureDataMetaCommand references a texture that was not created "
                     "during recording; its updates are skipped";
    });
  }
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    ReplayMappedTextureData(command);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
