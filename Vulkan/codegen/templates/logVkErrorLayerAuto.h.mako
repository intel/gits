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
#include "configurator.h"
#include "vulkanHeader2.h"

#include <string>

namespace gits {
namespace vulkan {

class LogVkErrorLayer : public Layer {
public:
  LogVkErrorLayer() : Layer("LogVkError") {
    m_IsPlayer = Configurator::IsPlayer();
  }

  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if command.return_type == 'VkResult':
  % if define:
#ifdef ${define}
  % endif
  void Pre(${command.name}Command& command) override;
  void Post(${command.name}Command& command) override;
  % if define:
#endif
  % endif
  % endif
  % endfor

private:
  bool IsFailure(VkResult result) const {
    return static_cast<int32_t>(result) < 0 && (!m_IsPlayer || result != m_PreReturn);
  }

private:
  bool m_IsPlayer{};
  VkResult m_PreReturn{};
};

} // namespace vulkan
} // namespace gits
