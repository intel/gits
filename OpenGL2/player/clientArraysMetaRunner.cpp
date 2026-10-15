// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "clientArraysMetaRunner.h"

#include "glEnumsAuto.h"
#include "layerAuto.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "replayContextCaps.h"
#include "threadContextService.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace gits::opengl2 {

namespace {

// Player-owned copies of client memory, by recorder-assigned block id. The driver keeps only the
// pointer and reads it at the draw, so a block stays alive until the recorder releases it.
std::mutex g_BlocksMutex;
std::unordered_map<uint64_t, std::vector<uint8_t>> g_Blocks;

// Guarded by g_BlocksMutex.
bool g_WarnedSecondaryColor = false;
bool g_WarnedFogCoord = false;
bool g_WarnedTexCoordUnit = false;
bool g_WarnedGenericAttribs = false;
bool g_WarnedIntegerAttribs = false;
bool g_WarnedLongAttribs = false;

void WarnUnsupportedOnce(bool& warned, const char* what) {
  if (!warned) {
    warned = true;
    LOG_WARNING << "ClientArraysMetaRunner: the replay context does not support " << what
                << " - such arrays are skipped";
  }
}

bool IsWellFormed(const ClientArraysMetaCommand& command) {
  const size_t arrays = command.m_Kinds.Data.size();
  if (command.m_Slots.Data.size() != arrays || command.m_Sizes.Data.size() != arrays ||
      command.m_Types.Data.size() != arrays || command.m_Strides.Data.size() != arrays ||
      command.m_Flags.Data.size() != arrays || command.m_ArrayBlocks.Data.size() != arrays ||
      command.m_ArrayOffsets.Data.size() != arrays ||
      command.m_BlockIds.Data.size() != command.m_BlockSizes.Data.size()) {
    return false;
  }
  uint64_t dataSize = 0;
  for (uint64_t size : command.m_BlockSizes.Data) {
    dataSize += size;
  }
  return dataSize == command.m_Data.Data.size();
}

void ReplayClientArrays(const ClientArraysMetaCommand& command) {
  if (!IsWellFormed(command)) {
    LOG_ERROR << "ClientArraysMetaRunner: malformed command - skipped";
    return;
  }

  std::lock_guard<std::mutex> lock(g_BlocksMutex);
  for (uint64_t id : command.m_ReleasedBlocks.Data) {
    g_Blocks.erase(id);
  }
  uint64_t dataOffset = 0;
  for (size_t i = 0; i < command.m_BlockIds.Data.size(); ++i) {
    const auto begin = command.m_Data.Data.begin() + dataOffset;
    dataOffset += command.m_BlockSizes.Data[i];
    g_Blocks[command.m_BlockIds.Data[i]].assign(begin, command.m_Data.Data.begin() + dataOffset);
  }

  auto& table = player::g_DispatchTable;
  const ReplayContextCaps& caps = CurrentReplayContextCaps();
  GLint previousArrayBuffer = 0;
  auto bindBuffer = caps.BufferObjectsCore ? table.glBindBuffer.load(std::memory_order_acquire)
                                           : table.glBindBufferARB.load(std::memory_order_acquire);
  if (caps.BufferObjects) {
    table.glGetIntegerv.load(std::memory_order_acquire)(GL_ARRAY_BUFFER_BINDING,
                                                        &previousArrayBuffer);
    if (previousArrayBuffer != 0) {
      bindBuffer(GL_ARRAY_BUFFER, 0);
    }
  }
  GLint previousClientActiveTexture = 0;
  bool clientActiveTextureChanged = false;
  auto clientActiveTexture = caps.MultiTextureCore
                                 ? table.glClientActiveTexture.load(std::memory_order_acquire)
                                 : table.glClientActiveTextureARB.load(std::memory_order_acquire);

  for (size_t i = 0; i < command.m_Kinds.Data.size(); ++i) {
    auto block = g_Blocks.find(command.m_ArrayBlocks.Data[i]);
    if (block == g_Blocks.end()) {
      LOG_ERROR << "ClientArraysMetaRunner: array references unknown block "
                << command.m_ArrayBlocks.Data[i] << " - skipped";
      continue;
    }
    const void* pointer =
        reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(block->second.data()) +
                                      static_cast<uintptr_t>(command.m_ArrayOffsets.Data[i]));
    const uint32_t slot = command.m_Slots.Data[i];
    const GLint components = command.m_Sizes.Data[i];
    const GLenum type = command.m_Types.Data[i];
    const GLsizei stride = command.m_Strides.Data[i];
    const uint32_t flags = command.m_Flags.Data[i];

    switch (static_cast<ClientArrayKind>(command.m_Kinds.Data[i])) {
    case ClientArrayKind::Vertex:
      table.glVertexPointer.load(std::memory_order_acquire)(components, type, stride, pointer);
      break;
    case ClientArrayKind::Normal:
      table.glNormalPointer.load(std::memory_order_acquire)(type, stride, pointer);
      break;
    case ClientArrayKind::Color:
      table.glColorPointer.load(std::memory_order_acquire)(components, type, stride, pointer);
      break;
    case ClientArrayKind::SecondaryColor:
      if (!caps.SecondaryColor) {
        WarnUnsupportedOnce(g_WarnedSecondaryColor, "secondary color arrays");
      } else if (caps.SecondaryColorCore) {
        table.glSecondaryColorPointer.load(std::memory_order_acquire)(components, type, stride,
                                                                      pointer);
      } else {
        table.glSecondaryColorPointerEXT.load(std::memory_order_acquire)(components, type, stride,
                                                                         pointer);
      }
      break;
    case ClientArrayKind::FogCoord:
      if (!caps.FogCoord) {
        WarnUnsupportedOnce(g_WarnedFogCoord, "fog coordinate arrays");
      } else if (caps.FogCoordCore) {
        table.glFogCoordPointer.load(std::memory_order_acquire)(type, stride, pointer);
      } else {
        table.glFogCoordPointerEXT.load(std::memory_order_acquire)(type, stride, pointer);
      }
      break;
    case ClientArrayKind::Index:
      table.glIndexPointer.load(std::memory_order_acquire)(type, stride, pointer);
      break;
    case ClientArrayKind::EdgeFlag:
      table.glEdgeFlagPointer.load(std::memory_order_acquire)(stride, pointer);
      break;
    case ClientArrayKind::TexCoord:
      // An out-of-range glClientActiveTexture fails and leaves the previous unit active, so the
      // pointer below would overwrite that unit's array instead. Without multitexture the only
      // set is unit 0, already the client active one.
      if (slot >= static_cast<uint32_t>(caps.TextureCoordUnits)) {
        WarnUnsupportedOnce(g_WarnedTexCoordUnit,
                            "as many texture coordinate sets as the capture context");
        break;
      }
      if (caps.MultiTexture) {
        if (!clientActiveTextureChanged) {
          table.glGetIntegerv.load(std::memory_order_acquire)(GL_CLIENT_ACTIVE_TEXTURE,
                                                              &previousClientActiveTexture);
          clientActiveTextureChanged = true;
        }
        clientActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + slot));
      }
      table.glTexCoordPointer.load(std::memory_order_acquire)(components, type, stride, pointer);
      break;
    case ClientArrayKind::VertexAttrib:
      if ((flags & CLIENT_ARRAY_FLAG_LONG) != 0) {
        if (!caps.LongAttribs) {
          WarnUnsupportedOnce(g_WarnedLongAttribs, "64-bit vertex attribute arrays");
        } else if (caps.LongAttribsCore) {
          table.glVertexAttribLPointer.load(std::memory_order_acquire)(slot, components, type,
                                                                       stride, pointer);
        } else {
          table.glVertexAttribLPointerEXT.load(std::memory_order_acquire)(slot, components, type,
                                                                          stride, pointer);
        }
      } else if ((flags & CLIENT_ARRAY_FLAG_INTEGER) != 0) {
        if (!caps.IntegerAttribs) {
          WarnUnsupportedOnce(g_WarnedIntegerAttribs, "integer vertex attribute arrays");
        } else if (caps.IntegerAttribsCore) {
          table.glVertexAttribIPointer.load(std::memory_order_acquire)(slot, components, type,
                                                                       stride, pointer);
        } else {
          table.glVertexAttribIPointerEXT.load(std::memory_order_acquire)(slot, components, type,
                                                                          stride, pointer);
        }
      } else {
        const auto normalized = static_cast<GLboolean>((flags & CLIENT_ARRAY_FLAG_NORMALIZED) != 0);
        if (!caps.GenericAttribs) {
          WarnUnsupportedOnce(g_WarnedGenericAttribs, "generic vertex attribute arrays");
        } else if (caps.GenericAttribsCore) {
          table.glVertexAttribPointer.load(std::memory_order_acquire)(slot, components, type,
                                                                      normalized, stride, pointer);
        } else {
          table.glVertexAttribPointerARB.load(std::memory_order_acquire)(
              slot, components, type, normalized, stride, pointer);
        }
      }
      break;
    default:
      LOG_ERROR << "ClientArraysMetaRunner: unknown array kind " << command.m_Kinds.Data[i]
                << " - skipped";
      break;
    }
  }

  if (clientActiveTextureChanged) {
    clientActiveTexture(static_cast<GLenum>(previousClientActiveTexture));
  }
  if (previousArrayBuffer != 0) {
    bindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
  }
}

} // namespace

void ClientArraysMetaRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    ReplayClientArrays(command);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
