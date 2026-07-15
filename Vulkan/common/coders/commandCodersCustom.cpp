// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandCodersCustom.h"
#include "argumentCodersAuto.h"

#include <cstring>

namespace gits {
namespace vulkan {

uint32_t GetSize(const MarkerUInt64Command& command) {
  return GetSize(command.value_);
}

void Encode(const MarkerUInt64Command& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.value_);
}

void Decode(char* src, MarkerUInt64Command& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.value_);
}

uint32_t GetSize(const CreateWindowMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_DisplayProtocol) +
         GetSize(command.m_X) + GetSize(command.m_Y) + GetSize(command.m_Width) +
         GetSize(command.m_Height) + GetSize(command.m_Visible) + GetSize(command.m_Hwnd) +
         GetSize(command.m_Hinstance);
}

void Encode(const CreateWindowMetaCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_DisplayProtocol);
  Encode(dest, offset, command.m_X);
  Encode(dest, offset, command.m_Y);
  Encode(dest, offset, command.m_Width);
  Encode(dest, offset, command.m_Height);
  Encode(dest, offset, command.m_Visible);
  Encode(dest, offset, command.m_Hwnd);
  Encode(dest, offset, command.m_Hinstance);
}

void Decode(char* src, CreateWindowMetaCommand& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_DisplayProtocol);
  Decode(src, offset, command.m_X);
  Decode(src, offset, command.m_Y);
  Decode(src, offset, command.m_Width);
  Decode(src, offset, command.m_Height);
  Decode(src, offset, command.m_Visible);
  Decode(src, offset, command.m_Hwnd);
  Decode(src, offset, command.m_Hinstance);
}

uint32_t GetSize(const UpdateWindowMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Hwnd) +
         GetSize(command.m_Hinstance) + GetSize(command.m_X) + GetSize(command.m_Y) +
         GetSize(command.m_Width) + GetSize(command.m_Height) + GetSize(command.m_Visible);
}

void Encode(const UpdateWindowMetaCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Hwnd);
  Encode(dest, offset, command.m_Hinstance);
  Encode(dest, offset, command.m_X);
  Encode(dest, offset, command.m_Y);
  Encode(dest, offset, command.m_Width);
  Encode(dest, offset, command.m_Height);
  Encode(dest, offset, command.m_Visible);
}

void Decode(char* src, UpdateWindowMetaCommand& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Hwnd);
  Decode(src, offset, command.m_Hinstance);
  Decode(src, offset, command.m_X);
  Decode(src, offset, command.m_Y);
  Decode(src, offset, command.m_Width);
  Decode(src, offset, command.m_Height);
  Decode(src, offset, command.m_Visible);
}

uint32_t GetSize(const MappedDataMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Device) +
         GetSize(command.m_Memory) + GetSize(command.m_Regions);
}

void Encode(const MappedDataMetaCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Device);
  Encode(dest, offset, command.m_Memory);
  Encode(dest, offset, command.m_Regions);
}

void Decode(char* src, MappedDataMetaCommand& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Device);
  Decode(src, offset, command.m_Memory);
  Decode(src, offset, command.m_Regions);
}

uint32_t GetSize(const RestoreContentManifestCommand& command) {
  uint32_t size = GetSize(command.m_Key) + GetSize(command.m_ThreadId) +
                  GetSize(command.m_DeviceKey) + GetSize(command.m_PhysDevKey) +
                  GetSize(command.m_QueueKey) + GetSize(command.m_CommandPoolKey) +
                  GetSize(command.m_TotalBytes);
  size += static_cast<uint32_t>(sizeof(uint32_t)); // buffer count
  size += static_cast<uint32_t>(command.m_Buffers.size() * (sizeof(uint64_t) + sizeof(uint64_t)));
  size += static_cast<uint32_t>(sizeof(uint32_t)); // image count
  for (const auto& img : command.m_Images) {
    size += static_cast<uint32_t>(sizeof(uint64_t));     // DstImageKey
    size += static_cast<uint32_t>(sizeof(uint32_t) * 3); // Format, FinalLayout, AspectMask
    size += static_cast<uint32_t>(sizeof(uint64_t));     // Size
    size += static_cast<uint32_t>(sizeof(uint32_t));     // region count
    size += static_cast<uint32_t>(img.Regions.size() * sizeof(VkBufferImageCopy));
  }
  return size;
}

void Encode(const RestoreContentManifestCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_DeviceKey);
  Encode(dest, offset, command.m_PhysDevKey);
  Encode(dest, offset, command.m_QueueKey);
  Encode(dest, offset, command.m_CommandPoolKey);
  Encode(dest, offset, command.m_TotalBytes);

  uint32_t bufferCount = static_cast<uint32_t>(command.m_Buffers.size());
  Encode(dest, offset, bufferCount);
  for (const auto& buf : command.m_Buffers) {
    Encode(dest, offset, buf.DstBufferKey);
    Encode(dest, offset, buf.Size);
  }

  uint32_t imageCount = static_cast<uint32_t>(command.m_Images.size());
  Encode(dest, offset, imageCount);
  for (const auto& img : command.m_Images) {
    Encode(dest, offset, img.DstImageKey);
    Encode(dest, offset, img.Format);
    Encode(dest, offset, img.FinalLayout);
    Encode(dest, offset, img.AspectMask);
    Encode(dest, offset, img.Size);
    uint32_t regionCount = static_cast<uint32_t>(img.Regions.size());
    Encode(dest, offset, regionCount);
    if (regionCount > 0) {
      std::memcpy(dest + offset, img.Regions.data(), regionCount * sizeof(VkBufferImageCopy));
      offset += regionCount * static_cast<uint32_t>(sizeof(VkBufferImageCopy));
    }
  }
}

void Decode(char* src, RestoreContentManifestCommand& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_DeviceKey);
  Decode(src, offset, command.m_PhysDevKey);
  Decode(src, offset, command.m_QueueKey);
  Decode(src, offset, command.m_CommandPoolKey);
  Decode(src, offset, command.m_TotalBytes);

  uint32_t bufferCount = 0;
  Decode(src, offset, bufferCount);
  command.m_Buffers.resize(bufferCount);
  for (auto& buf : command.m_Buffers) {
    Decode(src, offset, buf.DstBufferKey);
    Decode(src, offset, buf.Size);
  }

  uint32_t imageCount = 0;
  Decode(src, offset, imageCount);
  command.m_Images.resize(imageCount);
  for (auto& img : command.m_Images) {
    Decode(src, offset, img.DstImageKey);
    Decode(src, offset, img.Format);
    Decode(src, offset, img.FinalLayout);
    Decode(src, offset, img.AspectMask);
    Decode(src, offset, img.Size);
    uint32_t regionCount = 0;
    Decode(src, offset, regionCount);
    img.Regions.resize(regionCount);
    if (regionCount > 0) {
      std::memcpy(img.Regions.data(), src + offset, regionCount * sizeof(VkBufferImageCopy));
      offset += regionCount * static_cast<uint32_t>(sizeof(VkBufferImageCopy));
    }
  }
}

uint32_t GetSize(const RestoreContentDataCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_DeviceKey) +
         GetSize(command.m_Regions);
}

void Encode(const RestoreContentDataCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_DeviceKey);
  Encode(dest, offset, command.m_Regions);
}

void Decode(char* src, RestoreContentDataCommand& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_DeviceKey);
  Decode(src, offset, command.m_Regions);
}

} // namespace vulkan
} // namespace gits
