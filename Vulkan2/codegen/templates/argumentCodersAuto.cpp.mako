// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "argumentCodersAuto.h"
#include "argumentCoders.h"
#include "handleMapService.h"

namespace gits {
namespace vulkan {

% for structure in structures:
<% 
define = get_define(structure.platform)
needs_coder = struct_needs_coder(structure, structures)
%>\
% if needs_coder:
% if define:
#ifdef ${define}
% endif
uint32_t GetSize(const ${structure.name}* src, uint32_t count) {
  if (!src) {
    return 0;
  } 
  auto blobSize = sizeof(${structure.name}) * count;
  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = &src[i];
    % for line in get_size_lines(structure, structures, 'currentSrcDesc'):
    ${line}
    % endfor
  }
  return blobSize;
}

void Encode(const ${structure.name}* src, uint32_t count, char* dst, uint32_t& offset) {
  if (!src || !dst) {
    return;
  }
  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<${structure.name}*>(&dst[offset]);
  WriteData(reinterpret_cast<const char*>(src), sizeof(${structure.name}) * count, dst, offset);

  for (uint32_t i = 0; i < count; ++i) {
    auto* currentSrcDesc = const_cast<${structure.name}*>(&srcDesc[i]);
    auto* currentDstDesc = &dstDesc[i];
    % for line in get_encode_lines(structure, structures, 'currentSrcDesc', 'currentDstDesc'):
    ${line}
    % endfor
  }
}

void Decode(const ${structure.name}* dst, uint32_t count, char* src, uint32_t& offset) {
  offset += sizeof(${structure.name}) * count;
  
  for (uint32_t i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<${structure.name}*>(&dst[i]);
    % for line in get_decode_lines(structure, structures, 'currentDstDesc'):
    ${line}
    % endfor
  }
}

// PointerArgument overloads for ${structure.name}
uint32_t GetSize(const PointerArgument<${structure.name}>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + GetSize(arg.Value, 1);
}

void Encode(char* dst, uint32_t& offset, const PointerArgument<${structure.name}>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  Encode(arg.Value, 1, dst, offset);
}

void Decode(char* src, uint32_t& offset, PointerArgument<${structure.name}>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  arg.Value = reinterpret_cast<${structure.name}*>(src + offset);
  Decode(arg.Value, 1, src, offset);
}

// ArrayArgument overloads for ${structure.name}
uint32_t GetSize(const ArrayArgument<${structure.name}>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + GetSize(arg.Value, arg.Size);
}

void Encode(char* dst, uint32_t& offset, const ArrayArgument<${structure.name}>& arg) {
  std::memcpy(dst + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return;
  }
  std::memcpy(dst + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  Encode(arg.Value, arg.Size, dst, offset);
}

void Decode(char* src, uint32_t& offset, ArrayArgument<${structure.name}>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    arg.Size = 0;
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Value = reinterpret_cast<${structure.name}*>(src + offset);
  Decode(arg.Value, arg.Size, src, offset);
}
% if define:
#endif
% endif
% endif

% endfor
} // namespace vulkan
} // namespace gits