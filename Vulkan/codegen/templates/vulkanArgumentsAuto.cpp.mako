// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "vulkanTools.h"
<%
    import operator
%>\

## ---------------------------------- ENUMS ----------------------------------
% for enum in vk_enums:
const char* gits::Vulkan::CVulkanEnumTypeTraits<${enum.name}>::Name() {
  return "${enum.name}";
}
std::string gits::Vulkan::CVulkanEnumTypeTraits<${enum.name}>::GetVariantName(${enum.name} variant) {
  switch (variant) {
  % for enumerator in sorted(enum.enumerators, key=lambda e: int(e.value)):
    % if enum.size == 64:
  case ${enum.name}::${enumerator.name}:
    % else:
  case ${enumerator.name}:
    % endif
    return "${enumerator.name}";
  % endfor
  default:
    Log(WARN) << "Unknown enum variant: " << variant << " of ${enum.name}";
    return "(${enum.name})" + std::to_string((int)variant);
  }
}

% endfor
## ---------------------- MAPPED TYPES (OPAQUE HANDLES) ----------------------
% for type_name in sorted(vulkan_mapped_types) + sorted(vulkan_mapped_types_nondisp):
template<>
const char* gits::Vulkan::CVulkanObj<${type_name}, gits::Vulkan::${type_name}TypeTag>::NAME = "${type_name}";

% endfor
## --------------------------------- STRUCTS ---------------------------------
## Skip custom structs, they are implemented manually.
% for struct in (s for s in vk_structs if not s.custom):
<%
    cname: str = make_cname(struct.name, struct.version)
    vkless_name: str = struct.name.removeprefix('Vk')
    constructor_params: str = f'const {struct.name}* {vkless_name.lower()}'
    if struct.constructor_arguments:
        constructor_params = struct.constructor_arguments
    inits = fields_to_str(struct.fields, '_{name}(std::make_unique<{ctype}>()), ', ', ')

    import textwrap

    field_init: str = '_{name} = std::make_unique<{ctype}>({arguments});\n'
    field_inits: str = fields_to_str(struct.fields, field_init, struct_name=struct.name)
    field_inits = textwrap.indent(field_inits, ' ' * 4)

    field_inits_null: str = fields_to_str(struct.fields, '_{name} = nullptr;\n')
    field_inits_null = textwrap.indent(field_inits_null, ' ' * 4)

    value_field_inits: str = make_value_field_inits(struct, FieldInitType.ARGUMENT_VALUE)
    value_field_inits = textwrap.indent(value_field_inits, ' ' * 4).strip()
    original_field_inits: str = make_value_field_inits(struct, FieldInitType.ARGUMENT_ORIGINAL)
    original_field_inits = textwrap.indent(original_field_inits, ' ' * 4).strip()
%>\

gits::Vulkan::${cname}::${cname}(): ${inits}, _${vkless_name}(nullptr), _${vkless_name}Original(nullptr), _isNullPtr(false) {
}

  % if struct.constructor_wrap:
// for constructor see vulkanArgumentsWrap.cpp
  % else:
gits::Vulkan::${cname}::${cname}(${constructor_params}): _${vkless_name}(nullptr), _${vkless_name}Original(nullptr), _isNullPtr(${vkless_name.lower()} == nullptr) {
  if (!*_isNullPtr) {
    ${field_inits.strip()}
  } else {
    ${field_inits_null.strip()}
  }
}
  % endif

const char* gits::Vulkan::${cname}::NAME = "${struct.name}";

${struct.name}* gits::Vulkan::${cname}::Value() {
  if (*_isNullPtr)
    return nullptr;
  if (_${vkless_name} == nullptr) {
    _${vkless_name} = std::make_unique<${struct.name}>();
    ${value_field_inits}
  }
  return _${vkless_name}.get();
}

gits::PtrConverter<${struct.name}> gits::Vulkan::${cname}::Original() {
  if (*_isNullPtr)
    return PtrConverter<${struct.name}>(nullptr);
  if (_${vkless_name}Original == nullptr) {
    _${vkless_name}Original = std::make_unique<${struct.name}>();
    ${original_field_inits}
  }
  return PtrConverter<${struct.name}>(_${vkless_name}Original.get());
}

std::set<uint64_t> gits::Vulkan::${cname}::GetMappedPointers() {

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

void gits::Vulkan::${cname}::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
  if (!*_isNullPtr) {
  % for field in struct.fields:
    _${field.name}->Write(stream);
  % endfor
  }
}

void gits::Vulkan::${cname}::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
  if (!*_isNullPtr) {
  % for field in struct.fields:
    _${field.name}->Read(stream);
  % endfor
  }
}

void gits::Vulkan::${cname}::Write(CCodeOStream& stream) const {
  if (*_isNullPtr) {
    stream << "nullptr";
  } else {
    stream << stream.VariableName(ScopeKey());
  }
}

bool gits::Vulkan::${cname}::AmpersandNeeded() const {
  return !*_isNullPtr;
}

void gits::Vulkan::${cname}::Declare(CCodeOStream &stream) const {
  if (!*_isNullPtr) {
  % for field in struct.fields:
## This terrible lack of indent prevents the indent from spilling over to the next line.
<%
    _, array = split_arrays_from_name(field.type)
    ctype: str = make_ctype(field.type, field.wrap_type, field.name)
    no_c_type: str = ctype.removeprefix('C')
%>\
    % if no_c_type not in types_not_needing_declaration:
    ## TODO: Finetune `types_needing_name_registration`, only register those types.
    _${field.name}->VariableNameRegister(stream, false);
    _${field.name}->Declare(stream);
    % endif
  % endfor

    stream.Register(ScopeKey(), "vk${vkless_name}", true);
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = {\n";
    stream.ScopeBegin();
  % for field in struct.fields:
## This terrible lack of indent prevents the indent from spilling over to the next line.
<%
    _, array = split_arrays_from_name(field.type)
    ctype: str = make_ctype(field.type, field.wrap_type, field.name)
    no_c_type: str = ctype.removeprefix('C')
%>\
    % if no_c_type in types_not_needing_declaration:
    stream.Indent() << *(this->_${field.name}) << ", // ${field.name}\n";
    % elif array:
    stream.Indent() << "{}, // ${field.name}\n";
    % else:
    stream.Indent() << *_${field.name} << ", // ${field.name}\n";
    % endif
  % endfor
    stream.ScopeEnd();
    stream.Indent() << "};\n";

  % for field in struct.fields:
## This terrible lack of indent prevents the indent from spilling over to the next line.
<%
    _, array = split_arrays_from_name(field.type)
    array_length: str = array.strip('[]')
    count_condition: str = ('' if not field.count or array_length == field.count else
        f' && i < " << stream.VariableName(ScopeKey()) << ".{field.count}')
%>\
    % if array:
    stream.Indent() << "for (int i = 0; i < ${array_length}${count_condition}; ++i)\n";
    stream.ScopeBegin(); // For indentation.
    stream.Indent() << stream.VariableName(ScopeKey()) << ".${field.name}[i] = " << *_${field.name} << "[i];\n";
    stream.ScopeEnd();
    % endif
  % endfor
  }
}


% endfor
