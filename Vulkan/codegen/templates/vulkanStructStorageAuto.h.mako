// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "vulkanStructStorageBasic.h"

namespace gits{
namespace Vulkan {
## ---------------------- MAPPED TYPES (OPAQUE HANDLES) ----------------------
% for type_name in vulkan_mapped_types_nondisp + vulkan_mapped_types:
    typedef CSimpleMappedData<${type_name}> C${type_name}Data;
    typedef CDataArray<${type_name}, C${type_name}Data> C${type_name}DataArray;
% endfor
## ---------------------------------- ENUMS ----------------------------------
% for enum in vk_enums:
    typedef CSimpleData<${enum.name}> C${enum.name}Data;
    typedef CDataArray<${enum.name}, C${enum.name}Data> C${enum.name}DataArray;
% endfor
## --------------------------------- STRUCTS ---------------------------------
% for struct in vk_structs:
<%
    vkless_name: str = struct.name.removeprefix('Vk')
    cnamedata: str = f'C{struct.name}Data'

    constructor_params: str = f'const {struct.name}* {vkless_name.lower()}'
    if struct.constructor_arguments:
        constructor_params = struct.constructor_arguments

    import textwrap
    fields: str = fields_to_str(struct.fields, 'std::unique_ptr<{ctypedata}> _{name};\n')
    fields = textwrap.indent(fields, ' ' * 6)
%>\
    struct ${cnamedata} : public CBaseDataStruct, gits::noncopyable {
      ${fields.strip()}
    % if struct.pass_struct_storage:
      VkStructStoragePointerGITS _baseIn;
    % endif

      std::unique_ptr<${struct.name}> _${vkless_name};
      CboolData _isNullPtr;

      ${cnamedata}(${constructor_params});
      ${struct.name}* Value();

      PtrConverter<${struct.name}> operator*() {
        return PtrConverter<${struct.name}>(Value());
      }
      void * GetPtrType() override { return (void *)Value(); }
      std::set<uint64_t> GetMappedPointers();
    };
    typedef CDataArray<${struct.name}, ${cnamedata}> ${cnamedata}Array;
    typedef CDataArrayOfArrays<${struct.name}, ${cnamedata}> ${cnamedata}ArrayOfArrays;


% endfor

  } // namespace Vulkan
} // namespace gits
