// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "captureManager.h"
#include "handleMapService.h"

namespace gits {
namespace vulkan {

template <typename T>
void UpdateHandle(CaptureManager& manager, HandleArgument<T>& arg) {
  if (!arg.Value) {
    return;
  }
  arg.Key = HandleMapService::Get().GetKey(reinterpret_cast<std::uint64_t>(arg.Value));
}

template <typename T>
void UpdateHandle(CaptureManager& manager, HandleArrayArgument<T>& arg) {
  if (!arg.Value) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    arg.Keys[i] = HandleMapService::Get().GetKey(reinterpret_cast<std::uint64_t>(arg.Value[i]));
  }
}

template <typename T>
void UpdateOutputHandle(CaptureManager& manager, HandleOutputArgument<T>& arg) {
  if (!arg.Value || !*arg.Value) {
    return;
  }

  arg.Key = manager.CreateHandleKey();
  auto handle = reinterpret_cast<std::uint64_t>(*arg.Value);
  HandleMapService::Get().SetKey(handle, arg.Key);
}

template <typename T>
void UpdateOutputHandle(CaptureManager& manager, HandleArrayOutputArgument<T>& arg) {
  if (!arg.Value || arg.Size == 0) {
    return;
  }

  for (uint32_t i = 0; i < arg.Size; ++i) {
    arg.Keys[i] = manager.CreateHandleKey();
    auto handle = reinterpret_cast<std::uint64_t>(arg.Value[i]);
    HandleMapService::Get().SetKey(handle, arg.Keys[i]);
  }
}

} // namespace vulkan
} // namespace gits
