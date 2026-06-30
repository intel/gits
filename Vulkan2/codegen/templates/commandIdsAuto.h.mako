// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandId.h"

namespace gits {
namespace vulkan {

enum class CommandId {
  ID_INIT_START = stream::CommonCommandId::ID_INIT_START,
  ID_INIT_END = stream::CommonCommandId::ID_INIT_END,
  ID_FRAME_END = stream::CommonCommandId::ID_FRAME_END,
  ID_MARKER_UINT64 = stream::CommonCommandId::ID_MARKER_UINT64,

  // gits::ApiId::ID_VULKAN2 * 0x10000 = 0xe0000
  ID_META_BEGIN = 0xe0000,
  ID_META_CREATE_WINDOW = 0xe0001,
  ID_META_MAPPED_DATA = 0xe0002,
  ID_META_UPDATE_WINDOW = 0xe0003,

  ID_COMMON_BEGIN = 0xe0500,
  %for cmd_name, cmd_id in command_ids.items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  ID_END
};

} // namespace vulkan
} // namespace gits
