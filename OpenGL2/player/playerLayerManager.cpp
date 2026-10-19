// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerLayerManager.h"

#include "analyzerLayerAuto.h"
#include "analyzerResults.h"
#include "configurator.h"
#include "layerAuto.h"
#include "log.h"
#include "pluginService.h"
#include "recordingLayerAuto.h"
#include "replayCustomizationLayer.h"
#include "screenshotsLayer.h"
#include "stateTrackingLayer.h"
#include "subcaptureLayer.h"
#include "threadContextService.h"
#include "traceLayerAuto.h"
#include "traceLayerGroup.h"

namespace gits::opengl2 {

PlayerLayerManager::PlayerLayerManager() = default;

PlayerLayerManager::~PlayerLayerManager() = default;

void PlayerLayerManager::LoadLayers() {
  m_TraceLayerGroup = std::make_unique<TraceLayerGroup>();
  if (m_TraceLayerGroup->IsEnabled()) {
    auto layer = m_TraceLayerGroup->CreateLayer();
    m_PreLayers.push_back(layer.get());
    m_LayersOwner.push_back(std::move(layer));
  }

  std::unique_ptr<AnalyzerLayer> analyzerLayer;
  std::unique_ptr<SubcaptureLayer> subcaptureLayer;
  std::unique_ptr<RecordingLayer> recordingLayer;
  const auto& subcaptureCfg = Configurator::Get().common.player.subcapture;
  const bool subcapture = subcaptureCfg.enabled && !subcaptureCfg.frames.empty();
  // An optimized subcapture of a range needing state restore first runs an analysis pass,
  // which writes no stream.
  if (subcapture && SubcaptureRange(subcaptureCfg.frames).StartFrame() > 1 &&
      AnalyzerResults::NeedsAnalysis()) {
    analyzerLayer = std::make_unique<AnalyzerLayer>(SubcaptureRange(subcaptureCfg.frames));
    m_PreLayers.push_back(analyzerLayer.get());
    LOG_INFO << "Subcapture: analysis pass, run again to record the subcapture";
  } else if (subcapture) {
    subcaptureLayer = std::make_unique<SubcaptureLayer>(SubcaptureRange(subcaptureCfg.frames));
    StateTrackingService* stateTracking = subcaptureLayer->GetRange().StartFrame() > 1
                                              ? &subcaptureLayer->GetStateTracking()
                                              : nullptr;
    recordingLayer = std::make_unique<RecordingLayer>(subcaptureLayer->GetRecorder(),
                                                      subcaptureLayer->GetRange(), stateTracking);
    m_PreLayers.push_back(subcaptureLayer.get());
    m_PreLayers.push_back(recordingLayer.get());
  }

  auto threadContextLayer = std::make_unique<ThreadContextLayer>();
  m_PostLayers.push_back(threadContextLayer.get());
  m_LayersOwner.push_back(std::move(threadContextLayer));

  auto replayCustomizationLayer = std::make_unique<ReplayCustomizationLayer>();
  m_PostLayers.push_back(replayCustomizationLayer.get());
  m_LayersOwner.push_back(std::move(replayCustomizationLayer));

  const auto& config = Configurator::Get().common;
  if (config.shared.screenshots.enabled || !config.player.captureFrames.empty()) {
    auto screenshotsLayer = std::make_unique<ScreenshotsLayer>();
    m_PreLayers.push_back(screenshotsLayer.get());
    m_PostLayers.push_back(screenshotsLayer.get());
    m_LayersOwner.push_back(std::move(screenshotsLayer));
  }

  if (analyzerLayer) {
    m_PostLayers.push_back(analyzerLayer.get());
    m_LayersOwner.push_back(std::move(analyzerLayer));
  }

  // State is tracked before SubcaptureLayer restores it at the end of the frame before the
  // range, and RecordingLayer writes the frame end after SubcaptureLayer has advanced the frame.
  if (subcaptureLayer) {
    if (subcaptureLayer->GetRange().StartFrame() > 1) {
      auto stateTrackingLayer = std::make_unique<StateTrackingLayer>(
          subcaptureLayer->GetStateTracking(), subcaptureLayer->GetRecorder(),
          subcaptureLayer->GetAnalysis(), subcaptureLayer->GetRange());
      m_PreLayers.push_back(stateTrackingLayer.get());
      m_PostLayers.push_back(stateTrackingLayer.get());
      m_LayersOwner.push_back(std::move(stateTrackingLayer));
    }
    m_PostLayers.push_back(subcaptureLayer.get());
    m_PostLayers.push_back(recordingLayer.get());
    m_LayersOwner.push_back(std::move(recordingLayer));
    m_LayersOwner.push_back(std::move(subcaptureLayer));
  }

  m_PluginService = std::make_unique<PluginService>();
  m_PluginService->LoadPlugins();
  for (const auto& plugin : m_PluginService->GetPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.Impl->getImpl());
    if (layer) {
      m_PreLayers.push_back(layer);
      m_PostLayers.push_back(layer);
    }
  }
}

void PlayerLayerManager::Shutdown() {
  // Raw pointers are views into m_LayersOwner and must be invalidated before ownership ends.
  m_PreLayers.clear();
  m_PostLayers.clear();
  m_LayersOwner.clear();
  m_TraceLayerGroup.reset();
}

} // namespace gits::opengl2
