// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Structs.h"
#include "l0ArgumentsAuto.h"

namespace gits {
namespace l0 {
%for name, arg in arguments.items():
const char* C${name}::NAME = "${arg.get('name')}";
  %if not arg.get('custom', False):
    %if not arg.get('obj', False):
C${name}::C${name}() {}
C${name}::C${name}(const L0Type& value) :
      %for field in arg['vars']:
  _${get_field_name(field)}(value.${get_field_name(field)})${'' if loop.last else ','}
      %endfor
{}
C${name}::C${name}(const L0Type* value) :
      %for field in arg['vars']:
  _${get_field_name(field)}(value->${get_field_name(field)})${'' if loop.last else ','}
      %endfor
{}
void C${name}::Write(CBinOStream& stream) const {
      %for field in arg['vars']:
  _${get_field_name(field)}.Write(stream);
      %endfor
}
void C${name}::Read(CBinIStream& stream) {
      %for field in arg['vars']:
  _${get_field_name(field)}.Read(stream);
      %endfor
}
void C${name}::Declare(CCodeOStream& stream) const {
  VariableNameRegister(stream, false);
      %for field in arg['vars']:
  if (_${get_field_name(field)}.DeclarationNeeded()) {
    _${get_field_name(field)}.VariableNameRegister(stream, false);
    _${get_field_name(field)}.Declare(stream);
  }
      %endfor
  std::string varName = stream.VariableName(ScopeKey());
  stream.Indent() << Name() << " " << varName << ";\n";
      %for field in arg['vars']:
  stream.Indent() << varName << ".${get_field_name(field)} = ";
  if (_${get_field_name(field)}.DeclarationNeeded()) {
    if (dynamic_cast<const CSArray*>(&_${get_field_name(field)})) {
      stream << "&";
    }
    stream << stream.VariableName(_${get_field_name(field)}.ScopeKey());
  } else {
    _${get_field_name(field)}.Write(stream);
  }
  stream << ";\n";
      %endfor
}
C${name}::L0Type* C${name}::Ptr() {
      %for field in arg['vars']:
        %if '[' not in field['name']:
  _struct.${get_field_name(field)} = *_${get_field_name(field)};
        %else:
  std::copy_n(*_${get_field_name(field)}, ${get_field_array_size(field)}, _struct.${get_field_name(field)});
        %endif
      %endfor
  return &_struct;
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
