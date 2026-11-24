// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recorderInterface.h"

#include "captureManager.h"
#include "clientArraysService.h"
#include "configurator.h"
#include "driverExports.h"
#include "glDispatchAuto.h"
#include "log.h"
#include "mapTrackingService.h"
#include "realDriver.h"
#include "recorderWrappers.h"

#include <stdexcept>

namespace gits::opengl {

bool RecordingEnabled() {
  static const bool enabled = Configurator::Get().common.recorder.enabled;
  return enabled;
}

bool PluginLayersLoaded() {
  static const bool loaded = !RecordingEnabled() && CaptureManager::Get().HasPluginLayers();
  return loaded;
}

class Recorder final : public IRecorder {
public:
  Recorder() {
    InitializeRealDriver();
    if (!RecordingEnabled()) {
      if (PluginLayersLoaded()) {
        LOG_INFO << "No stream is recorded. OpenGL calls run only the recorder plugins.";
      } else {
        LOG_INFO << "No stream is recorded. OpenGL calls go directly to the driver.";
      }
      return;
    }
    // Register both services' exit callbacks during serialized startup.
    // Lazy construction on separate GL threads would race in MessageBus::subscribe().
    ClientArraysService::Get();
    MapTrackingService::Get();
  }

  void* GetWrapper(const char* name) override {
    if (!WrapsCalls()) {
      if (void* real = dispatch::GetDriverExport(name)) {
        return real;
      }
    }
    void* wrapper = GetGeneratedRecorderWrapper(name);
    if (wrapper == nullptr) {
      throw std::runtime_error("Missing generated recorder wrapper");
    }
    return wrapper;
  }

  void* ResolveRealFunction(const char* name) override {
    return dispatch::GetDriverExport(name);
  }

  bool SetResolvedFunction(const char* name, void* pointer) override {
    return TryCacheRealFunction(name, pointer);
  }

  bool WrapsCalls() override {
    return RecordingEnabled() || PluginLayersLoaded();
  }

  void Close() override {
    // TODO: Route shutdown through a common modern-backend lifecycle service
    // once one exists; this forwarding-only stage has no stream to finalize.
    FinalizeRealDriver();
  }
};

} // namespace gits::opengl

extern "C" gits::opengl::IRecorder* OPENGL_RECORDER_CALL GITSRecorderOpenGL() {
  static gits::opengl::Recorder recorder;
  return &recorder;
}
