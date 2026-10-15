// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "argumentCoders.h"
#include "command.h"
#include "commandSerializer.h"

#include <cstdint>

namespace gits::opengl2 {

// Captures the application's REAL Win32 window geometry at record time, so the player can
// size/position its single skeleton window (HdcVirtualizationService::Resolve()) correctly
// from the very first frame instead of starting at a hardcoded 640x480 and inferring the real
// size later from glViewport calls (see GlViewportRunner/HdcVirtualizationService::GrowToFit(),
// player/include/commandRunnersCustom.h - kept as a defensive fallback for streams recorded
// before this command existed, or where WindowFromDC/GetClientRect fail).
//
// Mirrors the same "capture real window geometry into a dedicated meta-command" idiom
// DirectX (DirectX/common/layer_interface/src/include/commandsCustom.h's own
// CreateWindowMetaCommand, captured inline in DirectX/recorder/captureCustomizationLayer.cpp
// via GetClientRect on the swapchain's HWND) and Vulkan (Vulkan/common/layer_interface/
// commandsCustom.h's CreateWindowMetaCommand/UpdateWindowMetaCommand, captured by
// Vulkan/recorder/windowTrackingService.h's WindowTrackingService) already use - see
// docs/development/modern-opengl-backend-architecture.md's explicit callout of that Vulkan
// precedent for exactly this kind of "capture a native HWND/geometry with no gl.xml/vk.xml
// counterpart of its own" case. Collapsed into a SINGLE command here, rather than DX12/
// Vulkan's separate Create/Update pair, because those backends must decide whether a given
// swapchain/surface needs a brand-new player window (there can be several); OpenGL2 has
// exactly one, always-one, player-owned skeleton window for the whole replay
// (HdcVirtualizationService's own "Decision 1" - see that header's class comment), so there
// is nothing to distinguish "create" from "update" - one idempotent "here is the current real
// geometry" record, captured on change, covers both.
//
// This backend's own investigation (see docs/development/modern-opengl-backend-architecture.md
// and this command's own capture-side WindowGeometryTrackingService) confirmed there is no
// single, literally-shared (i.e. actually-the-same-code) recorder-side window-tracking
// component across DX12/Vulkan/OpenGL2 - each backend independently captures its own real
// geometry into its own, independently-defined meta-command type. What genuinely IS shared
// across all three backends is the PLAYER-side low-level window primitive underneath: DX12's
// DirectX::WindowService and Vulkan's vulkan::WindowService both delegate window creation/
// resize to common/windowing's gits::windowing::WindowManager - exactly the same one
// HdcVirtualizationService::Resolve()/GrowToFit() (player/hdcVirtualizationService.cpp)
// already uses. This command's own player-side consumption (windowGeometryMetaRunner.cpp)
// therefore continues to go through HdcVirtualizationService/WindowManager - no new player-side
// window-creation code, only a new, real (not hardcoded/inferred) size input to feed it.
//
// Kept in its own dedicated file rather than folded into mappedDataMetaTypes.h/
// unsupportedCommandTypes.h, matching this backend's own established one-file-per-token
// convention for a structured meta-command (each of MappedDataMetaCommand and this command gets
// its own types header and its own Runner pair) - only UnsupportedCommand and the
// closely-related FrameEndCommand are grandfathered into sharing a single
// unsupportedCommandTypes.h.
class WindowGeometryMetaCommand : public Command {
public:
  WindowGeometryMetaCommand(uint32_t threadId)
      : Command(CommandId::ID_META_WINDOW_GEOMETRY, threadId) {}
  WindowGeometryMetaCommand() : Command(CommandId::ID_META_WINDOW_GEOMETRY) {}

public:
  Argument<int32_t> m_X{};
  Argument<int32_t> m_Y{};
  Argument<int32_t> m_Width{};
  Argument<int32_t> m_Height{};
  Argument<bool> m_Visible{};
};

// Hand-assembled from the same generic, per-field GetSize/Encode/Decode overloads every
// generated command's own coder composes - mirrors MappedDataMetaCommand's identical
// rationale (mappedDataMetaTypes.h): there is no gl.xml/wgl-registry counterpart for this
// command, so there is no per-command loop to add it to.
inline uint64_t GetSize(const WindowGeometryMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_X) +
         GetSize(command.m_Y) + GetSize(command.m_Width) + GetSize(command.m_Height) +
         GetSize(command.m_Visible);
}

inline void Encode(const WindowGeometryMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_X);
  Encode(dest, offset, command.m_Y);
  Encode(dest, offset, command.m_Width);
  Encode(dest, offset, command.m_Height);
  Encode(dest, offset, command.m_Visible);
}

inline void Decode(const char* src, WindowGeometryMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_X);
  Decode(src, offset, command.m_Y);
  Decode(src, offset, command.m_Width);
  Decode(src, offset, command.m_Height);
  Decode(src, offset, command.m_Visible);
}

class WindowGeometryMetaSerializer : public stream::CommandSerializer {
public:
  explicit WindowGeometryMetaSerializer(const WindowGeometryMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
    Encode(command, m_Data.get());
  }

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_WINDOW_GEOMETRY);
  }
};

} // namespace gits::opengl2
