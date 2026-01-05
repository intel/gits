// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printStructuresAuto.h"
#include "printEnumsAuto.h"
#include "printCustom.h"

namespace gits {
namespace DirectX {

<%
custom = [
  'D3D12_RENDER_TARGET_VIEW_DESC',
  'D3D12_COMMAND_SIGNATURE_DESC',
  'D3D12_VERSIONED_ROOT_SIGNATURE_DESC',
  'D3D12_ROOT_SIGNATURE_DESC',
  'D3D12_ROOT_SIGNATURE_DESC1',
  'D3D12_ROOT_SIGNATURE_DESC2',
  'D3D12_ROOT_DESCRIPTOR_TABLE',
  'D3D12_ROOT_DESCRIPTOR_TABLE1',
  'D3D12_ROOT_PARAMETER',
  'D3D12_ROOT_PARAMETER1',
  'D3D12_GRAPHICS_PIPELINE_STATE_DESC',
  'D3D12_COMPUTE_PIPELINE_STATE_DESC',
  'D3D12_SHADER_BYTECODE',
  'D3D12_UNORDERED_ACCESS_VIEW_DESC',
  'D3D12_DEPTH_STENCIL_VIEW_DESC',
  'D3D12_CLEAR_VALUE',
  'D3D12_EXPORT_DESC',
  'D3D12_STREAM_OUTPUT_DESC',
  'D3D12_INPUT_LAYOUT_DESC',
  'D3D12_CACHED_PIPELINE_STATE',
  'D3D12_SO_DECLARATION_ENTRY',
  'D3D12_INPUT_ELEMENT_DESC',
  'DSTORAGE_REQUEST',
  'xess_d3d12_init_params_t',
  'xess_d3d12_execute_params_t'
]
%>\
%for structure in structures:
%if dml_struct_is_custom(structure):
FastOStream& operator<<(FastOStream& stream, const ${structure.name}& value) {
  stream << "${structure.name}{";
  stream << value.Type << ", ";
  switch (value.Type) {
<%
desc_types = dml_enum_get_type(enums, structure.name)
%>\
%for desc_type in desc_types.values:
%if dml_enum_is_valid(desc_type):
  case ${desc_type}:
    stream << static_cast<const ${dml_enum_get_desc_type(desc_type)}*>(value.Desc);
    break;
%endif
%endfor
  default:
    stream << "unknown";
    break;
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const ${structure.name}* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

%elif not structure.name in custom:
FastOStream& operator<<(FastOStream& stream, const ${structure.name}& value) {
${print_struct_values(structure)}\
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const ${structure.name}* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}
%endif

%endfor

} // namespace DirectX
} // namespace gits
