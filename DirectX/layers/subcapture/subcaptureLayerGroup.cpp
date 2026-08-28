// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureLayerGroup.h"
#include "configurator.h"
#include "stateTrackingLayer.h"
#include "recordingLayerAuto.h"
#include "commandPreservationLayer.h"
#include "analyzerLayerAuto.h"
#include "analyzerResults.h"
#include "streamHeader.h"
#include "log.h"

#include <filesystem>
#include <vector>

#include <Windows.h>

namespace gits {
namespace DirectX {

void SubcaptureLayerGroup::LoadLayers() {
  const auto& cfg = Configurator::Get();

  if (!cfg.common.player.subcapture.enabled ||
      cfg.common.player.subcapture.directx.executionSerialization ||
      !cfg.common.player.subcapture.directx.commandListSplit.empty()) {
    return;
  }

  // Check if the executable name matches the application name stored in the stream
  // Note: ApplicationInfoOverride is used to set custom application info for the stream and will disable this check (so the user can explictly set the executable name)
  if (!cfg.directx.player.applicationInfoOverride.enabled) {
    std::vector<char> moduleFilename(MAX_PATH + 1, 0);
    GetModuleFileNameA(nullptr, moduleFilename.data(), static_cast<DWORD>(moduleFilename.size()));
    const std::string exeName = std::filesystem::path(moduleFilename.data()).filename().string();
    const std::string appName = stream::StreamHeader::Get().GetApplicationName();
    if (exeName != appName) {
      LOG_ERROR << "Subcapture - Executable name \"" << exeName
                << "\" does not match the application name stored in the stream \"" << appName
                << "\". Set Common.Player.ExecutableNameOverride.Enabled.";
      exit(EXIT_FAILURE);
    }
  }

  const std::string& frames = cfg.common.player.subcapture.frames;
  const std::string& executions = cfg.common.player.subcapture.directx.commandListExecutions;
  bool trimmingMode = false;
  try {
    if (executions.empty()) {
      int startFrame = std::stoi(frames);
      if (startFrame == 1) {
        trimmingMode = true;
        Configurator::GetMutable().common.player.execute = false;
        LOG_INFO << "Subcapture in trimming mode. Execution disabled.";
      }
    } else {
      if (frames.find("-") != std::string::npos) {
        LOG_ERROR << "Subcapture of Command list executions must have one frame range";
        exit(EXIT_FAILURE);
      }
    }
  } catch (...) {
    LOG_ERROR << "Invalid subcapture range: '" + cfg.common.player.subcapture.frames + "'";
    exit(EXIT_FAILURE);
  }

  m_SubcaptureRange = std::make_unique<SubcaptureRange>();

  if (trimmingMode) {
    m_Recorder = std::make_unique<SubcaptureRecorder>();
    AddLayer(std::make_unique<RecordingLayer>(*m_Recorder, *m_SubcaptureRange));
  } else if (AnalyzerResults::IsAnalysis()) {
    m_Recorder = std::make_unique<SubcaptureRecorder>();
    AddLayer(std::make_unique<StateTrackingLayer>(*m_Recorder, *m_SubcaptureRange));
    AddLayer(std::make_unique<RecordingLayer>(*m_Recorder, *m_SubcaptureRange));
    AddLayer(std::make_unique<CommandPreservationLayer>());
    const_cast<gits::Configuration&>(cfg).directx.player.multithreadedShaderCompilation = false;
  } else {
    AddLayer(std::make_unique<AnalyzerLayer>(*m_SubcaptureRange));
    LOG_INFO << "SUBCAPTURE ANALYSIS. RUN AGAIN FOR SUBCAPTURE RECORDING.";
  }
}

} // namespace DirectX
} // namespace gits
