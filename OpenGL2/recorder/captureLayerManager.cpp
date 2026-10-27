// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureLayerManager.h"

#include "captureCustomizationLayer.h"
#include "clientArraysLayerAuto.h"
#include "configurator.h"
#include "encoderLayerAuto.h"
#include "mappedTextureLayer.h"
#include "openglDriversAuto.h"
#include "pixelFormatCaptureLayer.h"
#include "pluginService.h"
#include "recorderScreenshotsLayer.h"
#include "uniformArrayCaptureLayer.h"
#include "windowGeometryTrackingLayer.h"

namespace gits::opengl2 {

CaptureLayerManager::CaptureLayerManager() = default;

CaptureLayerManager::~CaptureLayerManager() = default;

void CaptureLayerManager::LoadLayers() {
  auto captureCustomizationLayer = std::make_unique<CaptureCustomizationLayer>();
  auto windowGeometryTrackingLayer = std::make_unique<WindowGeometryTrackingLayer>();
  // Records client-memory vertex arrays ahead of the draws that read them.
  auto clientArraysLayer = std::make_unique<ClientArraysLayer>();
  // Captures still-mapped GL_INTEL_map_texture textures at swaps.
  auto mappedTextureLayer = std::make_unique<MappedTextureLayer>();
  // Pre() for wglCreatePbufferARB/EXT, wglSetPixelFormat and the GLX commands taking a
  // GLXFBConfig, Post() for wglSetPixelFormat and wglMakeCurrent, which has to run before
  // EncoderLayer.
  auto pixelFormatCaptureLayer = std::make_unique<PixelFormatCaptureLayer>();
  auto uniformArrayCaptureLayer = std::make_unique<UniformArrayCaptureLayer>();
  std::unique_ptr<RecorderScreenshotsLayer> screenshotsLayer;
  if (Configurator::Get().common.shared.screenshots.enabled) {
    screenshotsLayer = std::make_unique<RecorderScreenshotsLayer>();
  }
  auto encoderLayer = std::make_unique<EncoderLayer>();

  m_PreLayers.push_back(captureCustomizationLayer.get());
  m_PreLayers.push_back(windowGeometryTrackingLayer.get());
  m_PreLayers.push_back(clientArraysLayer.get());
  m_PreLayers.push_back(mappedTextureLayer.get());
  m_PreLayers.push_back(pixelFormatCaptureLayer.get());
  if (screenshotsLayer) {
    m_PreLayers.push_back(screenshotsLayer.get());
  }

  m_PostLayers.push_back(captureCustomizationLayer.get());
  m_PostLayers.push_back(windowGeometryTrackingLayer.get());
  m_PostLayers.push_back(pixelFormatCaptureLayer.get());
  m_PostLayers.push_back(uniformArrayCaptureLayer.get());
  if (screenshotsLayer) {
    m_PostLayers.push_back(screenshotsLayer.get());
  }
  // The encoder must run after the other built-in layers. Plugin layers follow it.
  m_PostLayers.push_back(encoderLayer.get());

  m_LayersOwner.push_back(std::move(captureCustomizationLayer));
  m_LayersOwner.push_back(std::move(windowGeometryTrackingLayer));
  m_LayersOwner.push_back(std::move(clientArraysLayer));
  m_LayersOwner.push_back(std::move(mappedTextureLayer));
  m_LayersOwner.push_back(std::move(pixelFormatCaptureLayer));
  m_LayersOwner.push_back(std::move(uniformArrayCaptureLayer));
  if (screenshotsLayer) {
    m_LayersOwner.push_back(std::move(screenshotsLayer));
  }
  m_LayersOwner.push_back(std::move(encoderLayer));

  m_PluginService = std::make_unique<PluginService>(&GetRealFunction);
  m_PluginService->LoadPlugins();
  for (const auto& plugin : m_PluginService->GetPlugins()) {
    Layer* layer = static_cast<Layer*>(plugin.Impl->getImpl());
    if (layer) {
      m_PreLayers.push_back(layer);
      m_PostLayers.push_back(layer);
    }
  }
}

} // namespace gits::opengl2
