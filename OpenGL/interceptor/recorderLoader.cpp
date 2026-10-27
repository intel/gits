// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recorderLoader.h"

#include "gitsLoader.h"
#include "logging.h"
#include "log.h"

#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>

namespace gits::opengl {

namespace {

struct RecorderState {
  RecorderState() = default;
  ~RecorderState() {
    if (Recorder != nullptr) {
      Recorder->Close();
    }
    Loader.reset();
  }
  RecorderState(const RecorderState&) = delete;
  RecorderState& operator=(const RecorderState&) = delete;

  std::unique_ptr<CGitsLoader> Loader;
  IRecorder* Recorder = nullptr;
};

RecorderState g_State;
std::once_flag g_InitializationFlag;

void InitializeRecorder() {
  try {
    g_State.Loader = std::make_unique<CGitsLoader>("GITSRecorderOpenGL", false);
    g_State.Recorder = static_cast<IRecorder*>(g_State.Loader->GetRecorderWrapperPtr());
    if (g_State.Recorder == nullptr) {
      FatalRecorderError("recorder initialization", "Factory returned null");
    }
  } catch (const std::exception& exception) {
    FatalRecorderError("recorder initialization", exception.what());
  } catch (...) {
    FatalRecorderError("recorder initialization", "Unknown error");
  }
}

} // namespace

IRecorder& GetRecorder() {
  std::call_once(g_InitializationFlag, InitializeRecorder);
  return *g_State.Recorder;
}

void* GetRecorderWrapper(const char* name) {
  try {
    void* wrapper = GetRecorder().GetWrapper(name);
    if (wrapper == nullptr) {
      FatalRecorderError("recorder wrapper lookup", name);
    }
    return wrapper;
  } catch (const std::exception& exception) {
    FatalRecorderError("recorder wrapper lookup", exception.what());
  } catch (...) {
    FatalRecorderError("recorder wrapper lookup", "Unknown error");
  }
}

[[noreturn]] void FatalRecorderError(const char* action, const char* details) noexcept {
  try {
    EnsureLoggingInitialized();
    LOG_ERROR << "Failure during " << action << ": " << details;
  } catch (...) {
  }
  std::abort();
}

} // namespace gits::opengl
