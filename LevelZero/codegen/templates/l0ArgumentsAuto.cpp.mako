// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Structs.h"
#include "l0ArgumentsAuto.h"

namespace gits {
namespace l0 {
%for name, arg in arguments.items():
  %if not arg.get('enabled', True):
<% continue %>
  %endif
const char* C${name}::NAME = "${arg.get('name')}";
  %if not arg.get('custom', False):
    %if not arg.get('obj', False):
C${name}::C${name}() {}
C${name}::C${name}(const L0Type& value) :
      %for field in arg['vars']:
  ${get_field_name(field, prefix='_')}(${get_field_name(field, prefix='value.', wrap_params=field.get('wrapParams'))})${'' if loop.last else ','}
      %endfor
{}
C${name}::C${name}(const L0Type* value) :
      %for field in arg['vars']:
  ${get_field_name(field, prefix='_')}(${get_field_name(field, prefix='value->', wrap_params=field.get('wrapParams'))})${'' if loop.last else ','}
      %endfor
{}
void C${name}::Write(CBinOStream& stream) const {
      %for field in arg['vars']:
  ${get_field_name(field, prefix='_')}.Write(stream);
      %endfor
}
void C${name}::Read(CBinIStream& stream) {
      %for field in arg['vars']:
  ${get_field_name(field, prefix='_')}.Read(stream);
      %endfor
}
C${name}::L0Type* C${name}::Ptr() {
      %for field in arg['vars']:
        %if '[' not in field['name']:
  ${get_field_name(field, prefix='_struct.')} = ${get_field_name(field, prefix='*_')};
        %else:
  std::copy_n(${get_field_name(field, prefix='*_')}, ${get_field_array_size(field)}, ${get_field_name(field, '_struct.')});
        %endif
      %endfor
  return &_struct;
}
    %else:
void C${arg.get('name')}::AddMutualMapping(${arg.get('name')} key, ${arg.get('name')} value) {
      %for name in arg.get('mutual_mapping', []):
  if (!C${name}::CheckMapping(reinterpret_cast<${name}>(key))) {
    C${name}::AddMapping(reinterpret_cast<${name}>(key), reinterpret_cast<${name}>(value));
  }
      %endfor
      %if not arg.get('mutual_mapping'):
  (void)key;
  (void)value;
      %endif
}
void C${arg.get('name')}::RemoveMutualMapping(${arg.get('name')} key) {
      %for name in arg.get('mutual_mapping', []):
  if (C${name}::CheckMapping(reinterpret_cast<${name}>(key))) {
    C${name}::RemoveMapping(reinterpret_cast<${name}>(key));
  }
      %endfor
      %if not arg.get('mutual_mapping'):
  (void)key;
      %endif
}
    %endif
  %endif
%endfor

%for name, enum in enums.items():
const char* C${name}::NAME = "${enum.get('name')}";
%endfor
%for name, enum in enums.items():
  %if "_structure_type_t" in name:
template<>
gits::CArgument *AllocateExtensionStructure(Cuint32_t *version, const ${name} &stype, ${get_namespace(name)}_base_properties_t *extendedProperties) {
  switch (stype) {
      %for structure in enum['vars']:
  case ${structure['name']}: {
        %for struct_name, kwargs in get_object_versions(arguments, structure['struct']).items():
          %if is_latest_version(arguments, kwargs):
    if (extendedProperties != nullptr) {
      *version = ${kwargs.get('version')};
      return new C${struct_name}(reinterpret_cast<${structure['struct']} *>(extendedProperties));
    } else {
            %for i in range(0, kwargs.get('version')+1):
      if (**version == ${i}) {
        return new C${cut_version(struct_name, kwargs.get('version'))}${f"_V{i}" if i else ""}();
      }
            %endfor
    }
          %endif
        %endfor
    throw ENotImplemented("Extension-specific structure is not implemented: " + ToStringHelper(stype));
  }
      %endfor
  }
  throw ENotImplemented("Extension-specific structure is not implemented: " + ToStringHelper(stype));
}
    %endif
 %endfor
const char* CExtensionStructBase::NAME = "void *";

} // namespace l0
} // namespace gits
