// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

% for struct in vk_structs:
std::string ToStr(const ${struct.name}& c);
std::string ToStr(const ${struct.name}* c);
% endfor
% for enum in vk_enums:
std::string ToStr(const ${enum.name}& c);
% endfor
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
% for type_name in vulkan_mapped_types_nondisp:
std::string ToStr(const ${type_name}& c);
std::string ToStr(const ${type_name}* c);
% endfor
#endif
% for type_name in vulkan_mapped_types:
std::string ToStr(const ${type_name}& c);
std::string ToStr(const ${type_name}* c);
% endfor
