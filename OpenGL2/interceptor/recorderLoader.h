// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "recorderInterface.h"

#include <exception>
#include <utility>

namespace gits::opengl2 {

IRecorder& GetRecorder();
void* GetRecorderWrapper(const char* name);
[[noreturn]] void FatalRecorderError(const char* action, const char* details);

// Prevents C++ exceptions from escaping through exported C API entry points.
// commandName is only used for logging.
template <typename Callable>
decltype(auto) InvokeRecorderOrDie(const char* commandName, Callable&& callable) noexcept {
  try {
    return std::forward<Callable>(callable)();
  } catch (const std::exception& exception) {
    FatalRecorderError(commandName, exception.what());
  } catch (...) {
    FatalRecorderError(commandName, "Unknown error");
  }
}

} // namespace gits::opengl2
