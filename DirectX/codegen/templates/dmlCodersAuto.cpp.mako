// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "dmlCodersAuto.h"
#include "dmlCodersCustom.h"

namespace gits {
namespace DirectX {
namespace dml {
    
// Basic types
<%
pod_field_types = get_pod_field_types(structures)
%>\
%for field_type in pod_field_types:

unsigned GetSize(const ${field_type}* src, unsigned count) {
  return GetSizeT<${field_type}>(src, count);
}

void Encode(const ${field_type}* src, unsigned count, char* dst, unsigned& offset) {
  EncodeT<${field_type}>(src, count, dst, offset);
}

void Decode(const ${field_type}* dst, unsigned count, char* src, unsigned& offset) {
  DecodeT<${field_type}>(dst, count, src, offset);
}
%endfor
%for struct in structures:
%if not dml_struct_is_custom(struct):
<% continue %>
%endif
<%
desc_types = dml_enum_get_type(enums, struct.name)
%>
// ${struct.name}

unsigned GetSize(const ${struct.name}* src, unsigned count) {
  if (!src) {
    return 0;
  }

  auto blobSize = sizeof(${struct.name}) * count;
  for (unsigned i = 0; i < count; ++i) {
    auto* currentSrcDesc = &src[i];
    GITS_ASSERT(currentSrcDesc);
    switch (currentSrcDesc->Type) {
%for desc_type in desc_types.values:
%if dml_enum_is_valid(desc_type):
    case ${desc_type}:
      blobSize += GetSize(castTo<${dml_enum_get_desc_type(desc_type)}>(currentSrcDesc->Desc), 1);
      break;
%endif
%endfor
    default:
      blobSize += 0; 
      break;
    }
  }
  return blobSize;
}

void Encode(const ${struct.name}* src, unsigned count, char* dst, unsigned& offset) {
  auto blobSize = GetSize(src, count);
  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<${struct.name}*>(&dst[offset]);
  offset += sizeof(${struct.name}) * count;

  for (unsigned i = 0; i < count; ++i) {
    auto* currentSrcDesc = const_cast<${struct.name}*>(&srcDesc[i]);
    auto* currentDstDesc = const_cast<${struct.name}*>(&dstDesc[i]);
    GITS_ASSERT(currentSrcDesc);
    currentDstDesc->Type = currentSrcDesc->Type;
    currentDstDesc->Desc = nullptr;
    if (currentSrcDesc->Desc) {
      currentDstDesc->Desc = (const void*)offset;
    }

    switch (currentSrcDesc->Type) {
%for desc_type in desc_types.values:
%if dml_enum_is_valid(desc_type):
    case ${desc_type}:
      Encode(castTo<${dml_enum_get_desc_type(desc_type)}>(currentSrcDesc->Desc), 1, dst, offset);
      break;
%endif
%endfor
    default: 
      break;
    }
  }
}

void Decode(const ${struct.name}* dst, unsigned count, char* src, unsigned& offset) {
  offset += sizeof(${struct.name}) * count;

  for (unsigned i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<${struct.name}*>(&dst[i]);
    GITS_ASSERT(currentDstDesc);
    if (currentDstDesc->Desc) {
      currentDstDesc->Desc = addPtrs(currentDstDesc->Desc, src);
    }

    switch (currentDstDesc->Type) {
%for desc_type in desc_types.values:
%if dml_enum_is_valid(desc_type):
    case ${desc_type}:
      Decode(castTo<${dml_enum_get_desc_type(desc_type)}>(currentDstDesc->Desc), 1, src, offset);
      break;
%endif
%endfor
    default:
      break;
    }
  }
}
%endfor

%for struct in structures:
%if (not to_serialize(struct)) or (dml_struct_is_custom(struct)):
<% continue %>
%endif
unsigned GetSize(const ${struct.name}* src, unsigned count) {
  if (!src) {
    return 0;
  }

  auto blobSize = sizeof(${struct.name}) * count;
  for (unsigned i = 0; i < count; ++i) {
    auto* currentSrcDesc = &src[i];
%for field in struct.fields:
%if (not field.is_pointer) or (field.is_interface):
<% continue %>
%endif
    if (currentSrcDesc->${field.name}) {
      blobSize += GetSize(currentSrcDesc->${field.name}, ${get_field_count(field, 'currentSrcDesc->')});
    }
%endfor
  }
  return blobSize;
}

void Encode(const ${struct.name}* src, unsigned count, char* dst, unsigned& offset) {
  auto blobSize = GetSize(src, count);
  if (!src || !dst) {
    GITS_ASSERT(0 && "Unexpected input");
    return;
  }

  auto* srcDesc = src;
  auto* dstDesc = reinterpret_cast<${struct.name}*>(&dst[offset]);
  writeData((char*)src, sizeof(${struct.name}) * count, dst, offset);

  for (unsigned i = 0; i < count; ++i) {
    auto* currentSrcDesc = &srcDesc[i];
    auto* currentDstDesc = &dstDesc[i];
    GITS_ASSERT(currentSrcDesc);
%for field in struct.fields:
%if (not field.is_pointer) or (field.is_interface):
<% continue %>
%endif
    if (currentSrcDesc->${field.name}) {
      currentDstDesc->${field.name} = (const ${field.type}*) offset;
      Encode(currentSrcDesc->${field.name}, ${get_field_count(field, 'currentSrcDesc->')}, dst, offset);
    }
%endfor
  }
}

void Decode(const ${struct.name}* dst, unsigned count, char* src, unsigned& offset) {
  offset += sizeof(${struct.name}) * count;

  for (unsigned i = 0; i < count; ++i) {
    auto* currentDstDesc = const_cast<${struct.name}*>(&dst[i]);
    GITS_ASSERT(currentDstDesc);
%for field in struct.fields:
%if (not field.is_pointer) or (field.is_interface):
<% continue %>
%endif
    if (currentDstDesc->${field.name}) {
      currentDstDesc->${field.name} = addPtrs(currentDstDesc->${field.name}, src);
      Decode(currentDstDesc->${field.name}, ${get_field_count(field, 'currentDstDesc->')}, src, offset);
    }
%endfor
  }
}
%endfor

} // namespace dml
} // namespace DirectX
} // namespace gits