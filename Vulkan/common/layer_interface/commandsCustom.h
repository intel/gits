// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "commandIdsAuto.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

class StateRestoreBeginCommand : public Command {
public:
  StateRestoreBeginCommand() : Command(CommandId::ID_INIT_START) {}
};

class StateRestoreEndCommand : public Command {
public:
  StateRestoreEndCommand() : Command(CommandId::ID_INIT_END) {}
};

class FrameEndCommand : public Command {
public:
  FrameEndCommand() : Command(CommandId::ID_FRAME_END) {}
};

class MarkerUInt64Command : public Command {
public:
  enum Value : uint64_t {
    NONE = 0x10000 + 1, // CTokenMarkerUInt64::COMMON_RESERVED + 1
    STATE_RESTORE_OBJECTS_BEGIN,
    STATE_RESTORE_OBJECTS_END,
    STATE_RESTORE_RTAS_BEGIN,
    STATE_RESTORE_RTAS_END,
    STATE_RESTORE_RESOURCES_BEGIN,
    STATE_RESTORE_RESOURCES_END,
    GPU_EXECUTION_BEGIN,
    GPU_EXECUTION_END
  };
  MarkerUInt64Command(uint64_t value) : Command(CommandId::ID_MARKER_UINT64), value_(value) {}
  MarkerUInt64Command() : Command(CommandId::ID_MARKER_UINT64) {}

public:
  Argument<uint64_t> value_{};
};

class CreateWindowMetaCommand : public Command {
public:
  enum DisplayProtocol : uint32_t {
    WIN,
    XLIB,
    XCB,
    WAYLAND
  };

  CreateWindowMetaCommand(uint32_t threadId)
      : Command{CommandId::ID_META_CREATE_WINDOW, threadId} {}
  CreateWindowMetaCommand() : Command(CommandId::ID_META_CREATE_WINDOW) {}

public:
  Argument<uint32_t> m_DisplayProtocol{};
  Argument<int32_t> m_X{};
  Argument<int32_t> m_Y{};
  Argument<int32_t> m_Width{};
  Argument<int32_t> m_Height{};
  Argument<bool> m_Visible{};
  Argument<uint64_t> m_Hwnd{};
  Argument<uint64_t> m_Hinstance{};
};

class UpdateWindowMetaCommand : public Command {
public:
  UpdateWindowMetaCommand(uint32_t threadId)
      : Command{CommandId::ID_META_UPDATE_WINDOW, threadId} {}
  UpdateWindowMetaCommand() : Command(CommandId::ID_META_UPDATE_WINDOW) {}

public:
  Argument<uint64_t> m_Hwnd{};
  Argument<uint64_t> m_Hinstance{};
  Argument<int32_t> m_X{};
  Argument<int32_t> m_Y{};
  Argument<int32_t> m_Width{};
  Argument<int32_t> m_Height{};
  Argument<bool> m_Visible{};
};

class MappedDataMetaCommand : public Command {
public:
  MappedDataMetaCommand(uint32_t threadId) : Command{CommandId::ID_META_MAPPED_DATA, threadId} {}
  MappedDataMetaCommand() : Command(CommandId::ID_META_MAPPED_DATA) {}

public:
  HandleArgument<VkDevice> m_Device{};
  HandleArgument<VkDeviceMemory> m_Memory{};
  MemoryRegions m_Regions{};
};

// ---------------------------------------------------------------------------
// State-restore content upload (player-side orchestration).
//
// Instead of baking a staging buffer + copy + submit + fence chain into the
// stream for every restored resource, the recorder emits a single per-device
// manifest (RestoreContentManifest) describing every buffer/image to restore,
// followed by exactly one RestoreContentData token per manifest entry carrying
// that resource's readback bytes.  The player owns all transient staging
// objects: it sizes them from live device memory, batches many resources per
// submit, and creates/destroys the staging buffers directly against the
// driver.  The staging objects never appear in the stream, so the stream stays
// small and the batching adapts to the replay platform.
//
// There is no separate "end" token: the manifest declares the resource count,
// the recorder emits one data token per resource (a zero-length region when a
// resource's readback failed), and the player flushes the final batch and
// tears the staging down once it has consumed that many data tokens.
//
// Resources are identified by a dense "resource index" assigned in manifest
// order (buffer entries first, then image entries within a single manifest).
// Each RestoreContentData region carries one resource's bytes keyed by that
// index (Region.Offset reused as the resource index, not a byte offset).
// ---------------------------------------------------------------------------
class RestoreContentManifestCommand : public Command {
public:
  struct BufferEntry {
    uint64_t DstBufferKey{0};
    uint64_t Size{0};
  };
  struct ImageEntry {
    uint64_t DstImageKey{0};
    uint32_t Format{0};                       // VkFormat
    uint32_t FinalLayout{0};                  // VkImageLayout
    uint32_t AspectMask{0};                   // VkImageAspectFlags
    uint64_t Size{0};                         // tightly-packed staging byte size
    std::vector<VkBufferImageCopy> Regions{}; // bufferOffset relative to this image's data start
  };

  RestoreContentManifestCommand(uint32_t threadId)
      : Command{CommandId::ID_META_RESTORE_CONTENT_MANIFEST, threadId} {}
  RestoreContentManifestCommand() : Command(CommandId::ID_META_RESTORE_CONTENT_MANIFEST) {}

public:
  uint64_t m_DeviceKey{0};
  uint64_t m_PhysDevKey{0};
  uint64_t m_QueueKey{0};
  uint64_t m_CommandPoolKey{0};
  uint64_t m_TotalBytes{0};
  std::vector<BufferEntry> m_Buffers{};
  std::vector<ImageEntry> m_Images{};
};

class RestoreContentDataCommand : public Command {
public:
  RestoreContentDataCommand(uint32_t threadId)
      : Command{CommandId::ID_META_RESTORE_CONTENT_DATA, threadId} {}
  RestoreContentDataCommand() : Command(CommandId::ID_META_RESTORE_CONTENT_DATA) {}

public:
  uint64_t m_DeviceKey{0};
  // Each region carries one resource's bytes; Region.Offset holds the manifest
  // resource index (not a byte offset), Region.Size the byte count (0 when the
  // resource's readback failed at record time).
  MemoryRegions m_Regions{};
};

} // namespace vulkan
} // namespace gits
