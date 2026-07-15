// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"
#include "playerManager.h"
#include "dispatchTablesHolder.h"
#include "pluginService.h"
#include "replayCustomizationLayer.h"
#include "portabilityLayer.h"
#include "log.h"
#include "subcaptureLayer.h"
#include "recordingLayerAuto.h"
#include "analyzerLayerAuto.h"
#include "analyzerResults.h"
#include "configurator.h"
#include "logVkErrorLayerAuto.h"

namespace gits {
namespace vulkan {

void PlayerLayerManager::LoadLayers(PlayerManager& playerManager, PluginService& pluginService) {

  std::unique_ptr<Layer> replayCustomizationLayer =
      std::make_unique<ReplayCustomizationLayer>(playerManager);
  std::unique_ptr<Layer> logVkErrorLayer = std::make_unique<LogVkErrorLayer>();

  std::unique_ptr<Layer> portabilityLayer;
  if (Configurator::Get().vulkan.player.portability.remapQueueFamilies) {
    portabilityLayer = std::make_unique<PortabilityLayer>();
  }

  const auto& traceCfg = Configurator::Get().common.shared.trace;
  const auto& screenshotsCfg = Configurator::Get().common.shared.screenshots;

  std::unique_ptr<Layer> traceLayer;
  if (traceCfg.enabled) {
    m_TraceLayerGroup = std::make_unique<TraceLayerGroup>();
    traceLayer = m_TraceLayerGroup->GetTraceLayer();
  }

  Layer* screenshotLayer = nullptr;
  m_ResourceDumpingLayerGroup =
      std::make_unique<ResourceDumpingLayerGroup>(playerManager.GetDispatchTablesHolder());
  if (screenshotsCfg.enabled) {
    m_ResourceDumpingLayerGroup->loadLayers();
    screenshotLayer = m_ResourceDumpingLayerGroup->getScreenshotLayer();
  }

  // Subcapture-related layers (SubcaptureLayer, RecordingLayer) only make sense
  // when the user has actually requested a subcapture run.  Outside of that
  // mode we must not instantiate them: they install Post overrides for every
  // Vulkan command, which adds per-call overhead, allocates state tracking
  // tables, and (in SubcaptureRecorder's ctor) opens the output stream.
  const auto& cfg = Configurator::Get();
  const bool subcaptureEnabled =
      cfg.common.player.subcapture.enabled && !cfg.common.player.subcapture.frames.empty();
  // Two-pass optimization is only engaged when optimize is enabled and no
  // analysis file exists yet.  When optimize is off (or an analysis file is
  // present) we go straight to the recording pass, restoring everything exactly
  // like the legacy single-pass flow.
  const bool subcaptureAnalysis =
      subcaptureEnabled && cfg.common.player.subcapture.optimize && !AnalyzerResults::IsAnalysis();

  std::unique_ptr<Layer> subcaptureLayer;
  std::unique_ptr<Layer> analyzerLayer;
  std::unique_ptr<Layer> recordingLayer;
  if (subcaptureAnalysis) {
    // Analysis pass: track state and collect in-range object usage, then dump
    // the analysis file.  No output stream is produced; the user must run again.
    subcaptureLayer = std::make_unique<SubcaptureLayer>(
        playerManager, cfg.common.player.subcapture.frames, /*analysisMode=*/true);
    auto* sc = static_cast<SubcaptureLayer*>(subcaptureLayer.get());
    analyzerLayer = std::make_unique<AnalyzerLayer>(*sc->GetAnalyzerService());
    LOG_INFO << "SUBCAPTURE ANALYSIS. RUN AGAIN FOR SUBCAPTURE RECORDING.";
  } else if (subcaptureEnabled) {
    subcaptureLayer =
        std::make_unique<SubcaptureLayer>(playerManager, cfg.common.player.subcapture.frames);

    // RecordingLayer shares the recorder and range owned by SubcaptureLayer so
    // it can write every in-range command to the output stream.  It also receives
    // the StateTrackingService pointer so it can store vkCmd* commands into the
    // owning command buffer's recorded-commands log for state restore.
    auto* sc = static_cast<SubcaptureLayer*>(subcaptureLayer.get());
    recordingLayer = std::make_unique<RecordingLayer>(sc->GetRecorder(), sc->GetRange(),
                                                      &sc->GetStateTrackingService());
  }

  auto enablePreLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PreLayers.push_back(layer.get());
    }
  };

  // Portability must rewrite queue family indices before any other layer
  enablePreLayer(portabilityLayer);
  enablePreLayer(replayCustomizationLayer);
  if (traceCfg.enabled && traceCfg.print.preCalls) {
    enablePreLayer(traceLayer);
  }
  if (screenshotsCfg.enabled) {
    m_PreLayers.push_back(screenshotLayer);
  }
  enablePreLayer(subcaptureLayer);
  enablePreLayer(analyzerLayer);
  enablePreLayer(logVkErrorLayer);

  auto enablePostLayer = [this](std::unique_ptr<Layer>& layer) {
    if (layer) {
      m_PostLayers.push_back(layer.get());
    }
  };

  enablePostLayer(portabilityLayer);
  enablePostLayer(logVkErrorLayer);
  enablePostLayer(replayCustomizationLayer);
  if (traceCfg.enabled && traceCfg.print.postCalls) {
    enablePostLayer(traceLayer);
  }
  if (screenshotsCfg.enabled) {
    m_PostLayers.push_back(screenshotLayer);
  }
  // AnalyzerLayer must run before SubcaptureLayer in the post order: the latter
  // advances the frame counter and dumps the analysis file at range end, so the
  // analyzer needs to observe the final in-range present first.
  enablePostLayer(analyzerLayer);
  enablePostLayer(subcaptureLayer);
  enablePostLayer(recordingLayer);

  auto retainLayer = [this](std::unique_ptr<Layer>&& layer) {
    if (layer) {
      m_LayersOwner.push_back(std::move(layer));
    }
  };

  retainLayer(std::move(portabilityLayer));
  retainLayer(std::move(replayCustomizationLayer));
  retainLayer(std::move(logVkErrorLayer));
  retainLayer(std::move(traceLayer));
  // SubcaptureLayer owns the AnalyzerService that AnalyzerLayer references, so it
  // must be retained (and therefore destroyed) after the AnalyzerLayer.
  retainLayer(std::move(subcaptureLayer));
  retainLayer(std::move(analyzerLayer));
  retainLayer(std::move(recordingLayer));

  for (const auto& plugin : pluginService.GetPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.Impl->getImpl());
    if (layer) {
      m_PreLayers.push_back(layer);
      m_PostLayers.push_back(layer);
    }
  }

  PLOGI << "PlayerManager: Initialized with " << m_PreLayers.size() << " pre-layers and "
        << m_PostLayers.size() << " post-layers";
}

void PlayerLayerManager::Shutdown() {
  // Drop the raw Layer* views first so nothing can dereference a layer that is
  // about to be destroyed (plugin layers in these vectors are owned elsewhere).
  m_PreLayers.clear();
  m_PostLayers.clear();
  // Destroy the owned layers and layer groups now, while the caller guarantees
  // the device dispatch tables and driver library are still alive.  Destroying
  // the ResourceDumpingLayerGroup tears down the ScreenshotsLayer, whose
  // SwapchainImagesDumper destructors join their worker threads and so flush the
  // final (and, for a single-frame stream, only) screenshot to disk.
  m_LayersOwner.clear();
  m_ResourceDumpingLayerGroup.reset();
  m_TraceLayerGroup.reset();
}

} // namespace vulkan
} // namespace gits
