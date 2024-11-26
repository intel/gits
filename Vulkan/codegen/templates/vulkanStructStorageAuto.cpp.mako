// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "vulkanTools.h"
#include "vulkanTools_lite.h"
#include "vulkanStructStorageAuto.h"

% for struct in vk_structs:
<%
    vkless_name: str = struct.name.removeprefix('Vk')
    cnamedata: str = f'C{struct.name}Data'

    constructor_params: str = f'const {struct.name}* {vkless_name.lower()}'
    if struct.constructor_arguments:
        constructor_params = struct.constructor_arguments

    import textwrap

    init_normal: str = '_{name} = std::make_unique<{ctypedata}>({arguments});\n'
    inits_normal: str = fields_to_str(struct.fields, init_normal, struct_name=struct.name)
    inits_normal = textwrap.indent(inits_normal, ' ' * 4)

    inits_null: str = fields_to_str(struct.fields, '_{name} = nullptr;\n')
    inits_null = textwrap.indent(inits_null, ' ' * 4)
%>\
  % if struct.constructor_wrap:
// for constructor see vulkanStructStorageWrap.cpp
  % else:
gits::Vulkan::${cnamedata}::${cnamedata}(${constructor_params}) : _${vkless_name}(nullptr), _isNullPtr(${vkless_name.lower()} == nullptr) {
  if (!*_isNullPtr)  {
    ${inits_normal.strip()}
  } else {
    ${inits_null.strip()}
  }
}
  % endif  # struct.constructor_wrap

${struct.name}* gits::Vulkan::${cnamedata}::Value() {
  if (*_isNullPtr)
    return nullptr;
  if (_${vkless_name} == nullptr) {
  % if struct.pass_struct_storage:

    // Pass this structure through pNext
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
        **_pNext,                                      // const void* pNext;
        this                                           // const void* pStructStorage;
    };

  % endif
    _${vkless_name} = std::make_unique<${struct.name}>();
  % for field in struct.fields:
## This terrible lack of indent prevents the indent from spilling over to the next line.
<%
    arrayless_type, array = split_arrays_from_name(field.type)
    array_length: str = array.strip('[]')
    values_name: str = f'{field.name}Values'
%>\
    % if array:
    auto ${values_name} = **_${field.name};
    if (${values_name} != nullptr) {
      for (int i = 0; i < ${array_length}; i++)
        _${vkless_name}->${field.name}[i] = ${values_name}[i];
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    %elif struct.pass_struct_storage and field.name == 'pNext':
    _${vkless_name}->${field.name} = &_baseIn;
    % else:
    _${vkless_name}->${field.name} = **_${field.name};
    % endif
  % endfor
  }
  return _${vkless_name}.get();
}

std::set<uint64_t> gits::Vulkan::${cnamedata}::GetMappedPointers() {
  std::set<uint64_t> returnMap;
  if (!*_isNullPtr) {
  % for field in struct.fields:
    ## Here, treat void as a primitive type. We don't want to process pNext, etc.
    % if undecorated_type(field.type) not in primitive_types + ['void']:
    for (auto obj : _${field.name}->GetMappedPointers())
      returnMap.insert((uint64_t)obj);
    % endif
  % endfor
  }
  return returnMap;
}

% endfor
