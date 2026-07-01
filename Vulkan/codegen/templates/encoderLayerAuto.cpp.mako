// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "encoderLayerAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"
#include "captureManager.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
void EncoderLayer::Post(${command.name}Command& command) {
  m_Recorder.Record(command.m_Key, new ${command.name}Serializer(command));
  % if command.name == 'vkQueuePresentKHR':
  GITSKey key = CaptureManager::Get().CreateCommandKey();
  m_Recorder.Record(key, new FrameEndSerializer(FrameEndCommand()));
  % endif
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
