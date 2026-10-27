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
// size/position its window for each recorded window (HdcVirtualizationService) correctly
// from the very first frame instead of starting at a hardcoded 640x480 and inferring the real
// size later from glViewport calls (see HdcVirtualizationService::GrowToFit(), called from
// player/include/replayCustomizationLayer.h - kept as a defensive fallback for streams recorded
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
// Vulkan's separate Create/Update pair: the OpenGL2 player creates a recorded window's player
// window lazily, when a DC bound to it is first used (see HdcVirtualizationService), so there
// is nothing to distinguish "create" from "update" - one idempotent "here is the current real
// geometry" record covers both.
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
  // The first m_CallSamples samples are one per DC of the WGL call this command is recorded right
  // before: two for wglMakeContextCurrentARB/EXT, draw DC first, and one for the other sampled
  // calls. A sample of every other live window sampled so far follows, so a window that records
  // no sample of its own again still follows its recorded window's geometry and visibility. Every
  // command reports them all, so a report replayed out of real-time order is corrected by the
  // next one. None in a command that only reports destroyed windows.
  //
  // m_Windows holds each sampled window's identity: its capture-time HWND qualified by its
  // lifetime, so a window reusing a destroyed window's HWND is a different window (see
  // WindowGeometryTrackingService::Track). Opaque (see the architecture page's "Capture native OS
  // handles as opaque scalar values"), so the player can tell the recorded windows apart. 0 when
  // the HDC had no window, or its geometry could not be queried for that window's lifetime; its
  // other fields are then 0, and the sample only tells the player the call's window is unknown.
  //
  // A command recorded before a GLX call has the one sample of its drawable, and no others
  // (see WindowGeometryTrackingService::CaptureGlx). A GLX sample is identified as the GLX
  // calls record drawables: by the XID qualified by its X server (see
  // GlxDrawableIdentityService). When a new X window reuses the XID of one whose destruction the
  // recorder did not see, the player keeps the same player window for both.
  Argument<uint32_t> m_CallSamples{};
  ArrayArgument<uint64_t> m_Windows{};
  ArrayArgument<int32_t> m_X{};
  ArrayArgument<int32_t> m_Y{};
  ArrayArgument<int32_t> m_Width{};
  ArrayArgument<int32_t> m_Height{};
  ArrayArgument<uint8_t> m_Visible{};
  // Sampled windows found destroyed, each reported once, in a command of its own keyed after the
  // destruction was seen and so after every recorded use of those windows. The player hides their
  // player windows. A destroyed WGL identity never comes back; a destroyed X window's XID can be
  // reused, and gets a new player window.
  ArrayArgument<uint64_t> m_DestroyedWindows{};
};

// Hand-assembled from the same generic, per-field GetSize/Encode/Decode overloads every
// generated command's own coder composes - mirrors MappedDataMetaCommand's identical
// rationale (mappedDataMetaTypes.h): there is no gl.xml/wgl-registry counterpart for this
// command, so there is no per-command loop to add it to.
inline uint64_t GetSize(const WindowGeometryMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_CallSamples) +
         GetSize(command.m_Windows) + GetSize(command.m_X) + GetSize(command.m_Y) +
         GetSize(command.m_Width) + GetSize(command.m_Height) + GetSize(command.m_Visible) +
         GetSize(command.m_DestroyedWindows);
}

inline void Encode(const WindowGeometryMetaCommand& command, char* dest) {
  uint64_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_CallSamples);
  Encode(dest, offset, command.m_Windows);
  Encode(dest, offset, command.m_X);
  Encode(dest, offset, command.m_Y);
  Encode(dest, offset, command.m_Width);
  Encode(dest, offset, command.m_Height);
  Encode(dest, offset, command.m_Visible);
  Encode(dest, offset, command.m_DestroyedWindows);
}

inline void Decode(const char* src, WindowGeometryMetaCommand& command) {
  uint64_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_CallSamples);
  Decode(src, offset, command.m_Windows);
  Decode(src, offset, command.m_X);
  Decode(src, offset, command.m_Y);
  Decode(src, offset, command.m_Width);
  Decode(src, offset, command.m_Height);
  Decode(src, offset, command.m_Visible);
  Decode(src, offset, command.m_DestroyedWindows);
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
