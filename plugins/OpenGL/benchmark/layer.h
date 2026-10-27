// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "config.h"
#include "cpuFrameBenchmarkService.h"
#include "messageBus.h"

namespace gits::opengl {

class BenchmarkLayer : public Layer {
public:
  BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus);

  void Pre(wglCreateContextCommand& command) override;
  void Pre(wglCreateContextAttribsARBCommand& command) override;
  void Post(wglSwapBuffersCommand& command) override;
  void Post(wglSwapLayerBuffersCommand& command) override;
  void Pre(glXCreateContextCommand& command) override;
  void Pre(glXCreateNewContextCommand& command) override;
  void Pre(glXCreateContextAttribsARBCommand& command) override;
  void Post(glXSwapBuffersCommand& command) override;

private:
  CpuFrameBenchmarkService m_CpuFrameBenchmarkService;
};

} // namespace gits::opengl
