// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directx.h"

namespace gits {
namespace DirectX {
namespace dml {

// Basic types
<%
pod_field_types = get_pod_field_types(structures)
%>
%for field_type in pod_field_types:
unsigned GetSize(const ${field_type}* src, unsigned count);
void Encode(const ${field_type}* src, unsigned count, char* dst, unsigned& offset);
void Decode(const ${field_type}* src, unsigned count, char* dst, unsigned& offset);

%endfor
%for struct in structures:
%if not to_serialize(struct):
<% continue %>
%endif
// ${struct.name}

unsigned GetSize(const ${struct.name}* src, unsigned count);
void Encode(const ${struct.name}* src, unsigned count, char* dst, unsigned& offset);
void Decode(const ${struct.name}* dst, unsigned count, char* src, unsigned& offset);

%endfor

} // namespace dml
} // namespace DirectX
} // namespace gits