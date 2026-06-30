// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

namespace gits {
namespace vulkan {

enum class CommandId {
  ID_INIT_START = 0x0,
  ID_INIT_END = 0x1,
  ID_FRAME_START = 0x2,
  ID_FRAME_END = 0x3,
  ID_MARKER_UINT64 = 0x4,

  ID_META_BEGIN = 0xe0000,
  ID_META_CREATE_WINDOW = 0xe0001,
  ID_META_MAPPED_DATA = 0xe0002,

  ID_COMMON_BEGIN = 0xe0500,
  %for cmd_name, cmd_id in command_ids.items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  ID_END
};

} // namespace vulkan
} // namespace gits
