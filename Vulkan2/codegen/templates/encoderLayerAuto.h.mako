// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "gitsRecorder.h"

namespace gits {
namespace vulkan {

class EncoderLayer : public Layer {
public:
  EncoderLayer(GitsRecorder& recorder) : Layer("Encoder"), m_Recorder(recorder) {}

  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  void Post(${command.name}Command& command) override;
  % if define:
  #endif
  % endif
  % endfor

private:
  GitsRecorder& m_Recorder;
};

} // namespace vulkan
} // namespace gits
