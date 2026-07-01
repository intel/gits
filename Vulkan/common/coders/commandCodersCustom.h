// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsCustom.h"

namespace gits {
namespace vulkan {

uint32_t GetSize(const MarkerUInt64Command& command);
void Encode(const MarkerUInt64Command& command, char* dest);
void Decode(char* src, MarkerUInt64Command& command);

uint32_t GetSize(const CreateWindowMetaCommand& command);
void Encode(const CreateWindowMetaCommand& command, char* dest);
void Decode(char* src, CreateWindowMetaCommand& command);

uint32_t GetSize(const UpdateWindowMetaCommand& command);
void Encode(const UpdateWindowMetaCommand& command, char* dest);
void Decode(char* src, UpdateWindowMetaCommand& command);

uint32_t GetSize(const MappedDataMetaCommand& command);
void Encode(const MappedDataMetaCommand& command, char* dest);
void Decode(char* src, MappedDataMetaCommand& command);

} // namespace vulkan
} // namespace gits
