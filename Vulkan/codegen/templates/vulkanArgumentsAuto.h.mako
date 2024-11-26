// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "vulkanArgumentsBasic.h"

namespace gits {
namespace Vulkan {
% for enum in vk_enums:
    typedef CVulkanEnum<${enum.name}> C${enum.name};
    template<>
    class CVulkanEnumTypeTraits<${enum.name}> {
    public:
      static const char* Name();
      static std::string GetVariantName(${enum.name} variant);
    };

% endfor

    // On 32-bit, all nondispatchable handle types are typedef'd to uint64_t.
    // This means compiler sees e.g. CVulkanObj<VkBuffer> as identical to
    // CVulkanObj<VkEvent>. On the other hand, type tags below are seen by the
    // compiler as distinct types. We use them to instantiate the template for
    // each handle type.
% for type_name in sorted(vulkan_mapped_types) + sorted(vulkan_mapped_types_nondisp):
    typedef struct ${type_name}_T* ${type_name}TypeTag;
    typedef CVulkanObj<${type_name}, ${type_name}TypeTag> C${type_name};

% endfor
% for struct in vk_structs:
<%
    cname: str = make_cname(struct.name, struct.version)
%>\
    % if not struct.custom:
    class ${cname};
    % endif
    % if struct.declare_array:
    typedef CStructArray<${struct.name}, ${cname}> ${cname}Array;
    % endif
    % if struct.declare_array_of_arrays:
    typedef CStructArrayOfArrays<${struct.name}, ${cname}> ${cname}ArrayOfArrays;
    % endif
% endfor

## Skip custom structs, they are implemented manually.
% for struct in (s for s in vk_structs if not s.custom):
<%
    cname: str = make_cname(struct.name, struct.version)
    fields: str = fields_to_str(
        struct.fields,
        '      std::unique_ptr<{ctype}> _{name};\n',
        '\n'
    ).lstrip(' ')
    vkless_name: str = struct.name.removeprefix('Vk')
    constructor_params: str = f'const {struct.name}* {vkless_name.lower()}'
    if struct.constructor_arguments:
        constructor_params = struct.constructor_arguments
%>\
    class ${cname} : public CArgument, gits::noncopyable {
      ${fields}

      std::unique_ptr<${struct.name}> _${vkless_name};
      std::unique_ptr<${struct.name}> _${vkless_name}Original;
      Cbool _isNullPtr;

    public:
      ${cname}();
      ${cname}(${constructor_params});
      static const char* NAME;
      virtual const char* Name() const override { return NAME; }
      ${struct.name}* Value();

      PtrConverter<${struct.name}> operator*() {
        return PtrConverter<${struct.name}>(Value());
      }
      PtrConverter<${struct.name}> Original();
      void * GetPtrType() override { return (void *)Value(); }
      virtual std::set<uint64_t> GetMappedPointers();
      virtual void Write(CBinOStream& stream) const override;
      virtual void Read(CBinIStream& stream) override;
      virtual void Write(CCodeOStream& stream) const override;
      virtual bool AmpersandNeeded() const override;
    % if struct.declaration_needed_wrap:
      virtual bool DeclarationNeeded() const override; // see vulkanArgumentsWrap.cpp for definition
    % else:
      virtual bool DeclarationNeeded() const override { return true; }
    % endif
      virtual void Declare(CCodeOStream &stream) const override;
    };

% endfor
} // namespace Vulkan
} // namespace gits
