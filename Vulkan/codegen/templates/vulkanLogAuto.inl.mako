// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

% for struct in vk_structs:
CVkLog & operator<<(const ${struct.name}& c);
CVkLog & operator<<(const ${struct.name}* c);
% endfor
% for enum in vk_enums:
CVkLog & operator<<(const ${enum.name}& c);
% endfor
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
% for type_name in vulkan_mapped_types_nondisp:
CVkLog & operator<<(const ${type_name}& c);
CVkLog & operator<<(const ${type_name}* c);
% endfor
#endif
% for type_name in vulkan_mapped_types:
CVkLog & operator<<(const ${type_name}& c);
CVkLog & operator<<(const ${type_name}* c);
% endfor
