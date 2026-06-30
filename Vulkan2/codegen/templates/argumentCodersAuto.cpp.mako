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

<%
pnext_input_structs  = [s for s in structures if s.pnext_input  and s.stype_value]
pnext_output_structs = [s for s in structures if s.pnext_output and s.stype_value]
%>\
// ============================================================================
// pNext chain - encoding format per node: [VkStructureType][struct_bytes...]
// terminated by VK_STRUCTURE_TYPE_MAX_ENUM
// ============================================================================

<%def name="pnext_switch_cases(structs, action)">\
% for s in structs:
<% define = get_define(s.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${s.stype_value}: {
${action(s)}\
        break;
      }
% if define:
#endif
% endif
% endfor
</%def>\

<%def name="getsize_action(s)">\
% if struct_needs_coder(s, structures):
        size += GetSize(reinterpret_cast<const ${s.name}*>(node), 1);
% else:
        size += static_cast<uint32_t>(sizeof(${s.name}));
% endif
        size += sizeof(VkStructureType);
</%def>\

<%def name="encode_action(s)">\
        std::memcpy(dst + offset, &node->sType, sizeof(VkStructureType));
        offset += sizeof(VkStructureType);
% if struct_needs_coder(s, structures):
        Encode(reinterpret_cast<const ${s.name}*>(node), 1, dst, offset);
% else:
        std::memcpy(dst + offset, node, sizeof(${s.name}));
        offset += static_cast<uint32_t>(sizeof(${s.name}));
% endif
</%def>\

uint32_t GetPNextChainSizeInput(const void* pNext) {
  uint32_t size = sizeof(VkStructureType);
  const auto* node = reinterpret_cast<const VkBaseInStructure*>(pNext);
  while (node) {
    switch (node->sType) {
${pnext_switch_cases(pnext_input_structs, getsize_action)}\
      default:
        break;
    }
    node = node->pNext;
  }
  return size;
}

uint32_t GetPNextChainSizeOutput(const void* pNext) {
  uint32_t size = sizeof(VkStructureType);
  const auto* node = reinterpret_cast<const VkBaseOutStructure*>(pNext);
  while (node) {
    switch (node->sType) {
${pnext_switch_cases(pnext_output_structs, getsize_action)}\
      default:
        break;
    }
    node = node->pNext;
  }
  return size;
}

void EncodePNextChainInput(char* dst, uint32_t& offset, const void* pNext) {
  const auto* node = reinterpret_cast<const VkBaseInStructure*>(pNext);
  while (node) {
    switch (node->sType) {
${pnext_switch_cases(pnext_input_structs, encode_action)}\
      default:
        break;
    }
    node = node->pNext;
  }
  VkStructureType terminator = VK_STRUCTURE_TYPE_MAX_ENUM;
  std::memcpy(dst + offset, &terminator, sizeof(VkStructureType));
  offset += sizeof(VkStructureType);
}

void EncodePNextChainOutput(char* dst, uint32_t& offset, const void* pNext) {
  const auto* node = reinterpret_cast<const VkBaseOutStructure*>(pNext);
  while (node) {
    switch (node->sType) {
${pnext_switch_cases(pnext_output_structs, encode_action)}\
      default:
        break;
    }
    node = node->pNext;
  }
  VkStructureType terminator = VK_STRUCTURE_TYPE_MAX_ENUM;
  std::memcpy(dst + offset, &terminator, sizeof(VkStructureType));
  offset += sizeof(VkStructureType);
}

void DecodePNextChainInput(char* src, uint32_t& offset, void** pNext) {
  VkStructureType sType = VK_STRUCTURE_TYPE_MAX_ENUM;
  VkBaseInStructure** chain = reinterpret_cast<VkBaseInStructure**>(pNext);
  *chain = nullptr;
  VkBaseInStructure** tail = chain;

  for (;;) {
    std::memcpy(&sType, src + offset, sizeof(VkStructureType));
    if (sType == VK_STRUCTURE_TYPE_MAX_ENUM) {
      offset += sizeof(VkStructureType);
      break;
    }
    offset += sizeof(VkStructureType);
    switch (sType) {
% for s in pnext_input_structs:
<% define = get_define(s.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${s.stype_value}: {
        auto* node = reinterpret_cast<${s.name}*>(src + offset);
% if struct_needs_coder(s, structures):
        Decode(node, 1, src, offset);
% else:
        offset += static_cast<uint32_t>(sizeof(${s.name}));
% endif
        *tail = reinterpret_cast<VkBaseInStructure*>(node);
        tail = const_cast<VkBaseInStructure**>(&(*tail)->pNext);
        *tail = nullptr;
        break;
      }
% if define:
#endif
% endif
% endfor
      default:
        break;
    }
  }
}

void DecodePNextChainOutput(char* src, uint32_t& offset, void** pNext) {
  // Output pNext chains are pre-allocated by the caller before the Vulkan call.
  // Walk the existing chain and decode data into each matching slot by sType.
  for (;;) {
    VkStructureType sType = VK_STRUCTURE_TYPE_MAX_ENUM;
    std::memcpy(&sType, src + offset, sizeof(VkStructureType));
    if (sType == VK_STRUCTURE_TYPE_MAX_ENUM) {
      offset += sizeof(VkStructureType);
      break;
    }
    offset += sizeof(VkStructureType);
    auto* node = reinterpret_cast<VkBaseOutStructure*>(*pNext);
    while (node && node->sType != sType) {
      node = node->pNext;
    }
    switch (sType) {
% for s in pnext_output_structs:
<% define = get_define(s.platform) %>
% if define:
#ifdef ${define}
% endif
      case ${s.stype_value}: {
% if struct_needs_coder(s, structures):
        if (node) {
          Decode(reinterpret_cast<${s.name}*>(node), 1, src, offset);
        } else {
          ${s.name} tmp{};
          Decode(&tmp, 1, src, offset);
        }
% else:
        offset += static_cast<uint32_t>(sizeof(${s.name}));
% endif
        break;
      }
% if define:
#endif
% endif
% endfor
      default:
        break;
    }
  }
}

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