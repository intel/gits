// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits::opengl {

// Imports the handle the replay of the recorded export command returned. The stream stores the
// key of the export command instead of the handle. The layer runs last before and first after the
// command, so the other layers, such as the subcapture ones, see the key. See
// ExternalHandleService.
class ExternalHandleLayer : public Layer {
public:
  ExternalHandleLayer() : Layer("ExternalHandle") {}

  void Pre(glImportMemoryWin32HandleEXTCommand& command) override;
  void Post(glImportMemoryWin32HandleEXTCommand& command) override;
  void Pre(glImportSemaphoreWin32HandleEXTCommand& command) override;
  void Post(glImportSemaphoreWin32HandleEXTCommand& command) override;
};

} // namespace gits::opengl
