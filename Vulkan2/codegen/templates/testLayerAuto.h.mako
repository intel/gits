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

namespace gits {
namespace vulkan {

class TestLayerAuto : public Layer {
public:
  TestLayerAuto() : Layer("TestLayerAuto") {}

  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  void Pre(${command.name}Command& command) override;
  void Post(${command.name}Command& command) override;
  % if define:
  #endif
  % endif
  % endfor
};

} // namespace vulkan
} // namespace gits