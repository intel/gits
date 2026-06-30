// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandCodersCustom.h"
#include "argumentCodersAuto.h"

namespace gits {
namespace vulkan {

uint32_t GetSize(const CreateWindowMetaCommand& command) {
  return GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_X) +
         GetSize(command.m_Y) + GetSize(command.m_Width) + GetSize(command.m_Height) +
         GetSize(command.m_Visible) + GetSize(command.m_Hwnd) + GetSize(command.m_Hinstance);
}

void Encode(const CreateWindowMetaCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
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
  Decode(src, offset, command.m_X);
  Decode(src, offset, command.m_Y);
  Decode(src, offset, command.m_Width);
  Decode(src, offset, command.m_Height);
  Decode(src, offset, command.m_Visible);
  Decode(src, offset, command.m_Hwnd);
  Decode(src, offset, command.m_Hinstance);
}

uint32_t GetSize(const MappedDataMetaCommand& command) {
  uint32_t size = GetSize(command.m_Key) + GetSize(command.m_ThreadId) + GetSize(command.m_Device) +
                  GetSize(command.m_Memory) + GetSize(command.m_TotalSize) +
                  GetSize(command.m_RegionCount);

  for (const auto& region : command.m_Regions) {
    size += sizeof(region.Offset) + sizeof(region.Size) + static_cast<uint32_t>(region.Size);
  }
  return size;
}

void Encode(const MappedDataMetaCommand& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  Encode(dest, offset, command.m_Device);
  Encode(dest, offset, command.m_Memory);
  Encode(dest, offset, command.m_TotalSize);
  Encode(dest, offset, command.m_RegionCount);
  for (const auto& r : command.m_Regions) {
    Encode(dest, offset, r.Offset);
    Encode(dest, offset, r.Size);
    std::memcpy(dest + offset, r.Data, r.Size);
    offset += static_cast<uint32_t>(r.Size);
  }
}

void Decode(char* src, MappedDataMetaCommand& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  Decode(src, offset, command.m_Device);
  Decode(src, offset, command.m_Memory);
  Decode(src, offset, command.m_TotalSize);
  Decode(src, offset, command.m_RegionCount);
  uint32_t count = command.m_RegionCount.Value;
  command.m_Regions.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    Decode(src, offset, command.m_Regions[i].Offset);
    Decode(src, offset, command.m_Regions[i].Size);
    command.m_Regions[i].Data = src + offset;
    offset += static_cast<uint32_t>(command.m_Regions[i].Size);
  }
}

} // namespace vulkan
} // namespace gits
