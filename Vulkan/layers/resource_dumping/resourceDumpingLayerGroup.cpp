// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpingLayerGroup.h"
#include "screenshotsLayer.h"
#include "log.h"
#include "configurator.h"

namespace gits {
namespace vulkan {

ResourceDumpingLayerGroup::ResourceDumpingLayerGroup(DispatchTablesHolder& dispatchTablesHolder)
    : m_DispatchTablesHolder(dispatchTablesHolder) {}

void ResourceDumpingLayerGroup::loadLayers() {
  if (Configurator::Get().common.shared.screenshots.enabled ||
      !Configurator::Get().common.player.captureFrames.empty()) {
    addLayer(std::make_unique<ScreenshotsLayer>(m_DispatchTablesHolder));
  }
}

} // namespace vulkan
} // namespace gits
