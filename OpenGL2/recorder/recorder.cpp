// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recorderInterface.h"

#include "openglDriversAuto.h"
#include "realDriver.h"
#include "recorderWrappers.h"

#include <stdexcept>

namespace gits::opengl2 {

class Recorder final : public IRecorder {
public:
  Recorder() {
    InitializeRealDriver();
  }

  void* GetWrapper(const char* name) override {
    void* wrapper = GetGeneratedRecorderWrapper(name);
    if (wrapper == nullptr) {
      throw std::runtime_error("Missing generated recorder wrapper");
    }
    return wrapper;
  }

  void* ResolveRealFunction(const char* name) override {
    return GetRealProcAddress(name);
  }

  bool SetResolvedFunction(const char* name, void* pointer) override {
    return TryCacheRealFunction(name, pointer);
  }

  void Close() override {
    // TODO: Route shutdown through a common modern-backend lifecycle service
    // once one exists; this forwarding-only stage has no stream to finalize.
    FinalizeRealDriver();
  }
};

} // namespace gits::opengl2

extern "C" gits::opengl2::IRecorder* OPENGL_RECORDER_CALL GITSRecorderOpenGL2() {
  static gits::opengl2::Recorder recorder;
  return &recorder;
}
