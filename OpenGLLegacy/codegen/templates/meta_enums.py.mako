## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2026 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
% for registry, enum_list in enums_by_api.items():
# ${registry} enums
% for enum in enum_list:
<%
    type = f', type=EnumType.{enum.type.name}' if enum.type is not EnumType.INT else ''
    # Comments can contain apostrophes.
    comment = f', comment="{enum.comment}"' if enum.comment else ''
    groups = f', groups={enum.groups}' if enum.groups else ''
    alias = f", alias='{enum.alias}'" if enum.alias else ''
    # In OGL generator, two concepts are named "API". Don't confuse them.
    # This is the OpenGL 'api' annotation found on enums in the XML.
    api = f", api='{enum.api}'" if enum.api else ''
    # This is the prefix indicating a registry (base GL or a native/windowing API).
    # Here we name it "registry" instead of "api" to avoid confusion. Type is `Api`.
    prefix: str = registry.lower()
%>\
${prefix}_enum(name='${enum.name}', value='${enum.value}'${type}${comment}${groups}${alias}${api})
% endfor  # for enum

% endfor  # for registry, enum_list