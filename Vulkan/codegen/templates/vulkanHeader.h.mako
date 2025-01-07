// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "vulkan_basic.h"
#ifdef GITS_PLATFORM_WINDOWS
#ifdef BUILD_FOR_CCODE
// If we include windows.h, it includes further headers. One of them defines
// WGL functions which we have already defined ourselves. This breaks OGL and
// OCL CCode. So instead of including windows.h, we just declare the types we
// need from it.
struct _SECURITY_ATTRIBUTES;
typedef _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
#else
#include <windows.h>
#endif  // BUILD_FOR_CCODE
#endif  // GITS_PLATFORM_WINDOWS

typedef uint32_t VkFlags;
% for flag in vulkan_flags:
typedef VkFlags ${flag};
% endfor

typedef uint64_t VkFlags64;
% for flag in vulkan_flags64:
typedef VkFlags64 ${flag};
% endfor

% for enum in vk_enums:
<%
    parent_type: str = ': VkFlags64 ' if enum.size == 64 else ''
%>\
typedef enum ${enum.name}_ ${parent_type}{
  % for enumerator in enum.enumerators:
  ${enumerator.name} = ${decimal_str_to_hex(enumerator.value)},
  % endfor
} ${enum.name};

% endfor
\
% for struct in vk_structs:
${struct.type} ${struct.name}_;
typedef ${struct.name}_ ${struct.name};

% endfor
\
% for token in vk_functions:
<%
    params: str = args_to_str(token.args, '{type} {name}{array}, ', ', ')
%>\
typedef ${token.return_value.type} (VKAPI_PTR *PFN_${token.name}) (${params});
% endfor

#ifdef VK_PROTOTYPES
% for token in vk_functions:
<%
    multiline_params: str = args_to_str(token.args, '{type} {name}{array},\n', ', \n')
%>\
VKAPI_ATTR ${token.return_value.type} VKAPI_CALL ${token.name}(
${multiline_params});

% endfor
#endif
\
% for struct in dependency_ordered(vk_structs):
${struct.type} ${struct.name}_ {
${fields_to_str(struct.fields, '  {type} {name}{bitfield}{array};\n', '\n')}
};

% endfor
