// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "config.h"
#include "vulkanLog.h"
#include "vulkanTools_lite.h"

namespace gits {
namespace Vulkan {

% for struct in vk_structs:
std::string ToStr(const ${struct.name}& c) {
  std::ostringstream oss;
  ${make_struct_log_code(struct.fields)}
  return oss.str();
}

std::string ToStr(const ${struct.name}* c) {
  std::ostringstream oss;
  if (c != nullptr) {
    if (Configurator::IsTraceDataOptPresent(TraceData::VK_STRUCTS))
      oss << ToStr(*c);
    else
      oss << "{ " << (void*)c << " }";
  } else {
    oss << "{ 0 }";
  }
  return oss.str();
}

% endfor
% for enum in vk_enums:
<%
  sorted_enumerators: list[Enumerator] = sorted(
      enum.enumerators,
      key=lambda e: int(e.value),
      reverse=True
  )

  zero_enumerator: Enumerator | None = None
  if sorted_enumerators[-1].value == '0':
      zero_enumerator = sorted_enumerators[-1]
      sorted_enumerators = sorted_enumerators[:-1]
%>\
% if 'Bits' in enum.name:
std::string ToStr(const ${enum.name}& c) {
  std::underlying_type_t<${enum.name}> e = c;
  std::ostringstream oss;
  % if zero_enumerator is not None:
  if (e == 0) {
    oss << "${zero_enumerator.name} | ";
  }
  % endif
  % for enumerator in sorted_enumerators:
  if (isBitSet(e, ${enumerator.name})) {
    oss << "${enumerator.name} | ";
    e &= ~${enumerator.name};
  }
  % endfor
  for (decltype(e) i = 1; i <= e; i <<= 1) {
    if (i & e) {
      oss << i << " | ";
      LOG_WARNING << "Unknown enum number: " << i << " for ${enum.name}";
      break;
    }
  }
  std::string str = oss.str();
  if (str.size() > 3) {
    str = str.substr(0, str.size() - 3);
  } else {
    str = "0";
  }
  std::ostringstream result;
  result << "{ " << str << " }";
  return result.str();
}
% else:
std::string ToStr(const ${enum.name}& c) {
  std::ostringstream oss;
  switch (c) {
    % for enumerator in enum.enumerators:
    case ${enumerator.value}:
      oss << "${enumerator.name}";
      break;
    %endfor
    default:
      oss << c;
      LOG_WARNING << "Unknown enum number: " << c << " for ${enum.name}";
      break;
  }
  return oss.str();
}
% endif

% endfor
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
% for type_name in vulkan_mapped_types_nondisp:
std::string ToStr(const ${type_name}& c) {
  std::ostringstream oss;
  oss << "{ " << (void*)c << " }";
  return oss.str();
}

std::string ToStr(const ${type_name}* c) {
  std::ostringstream oss;
  if (c != nullptr) {
    if (Configurator::IsTraceDataOptPresent(TraceData::VK_STRUCTS))
      oss << ToStr(*c);
    else
      oss << "{ " << (void*)c << " }";
  } else {
    oss << "{ 0 }";
  }
  return oss.str();
}

% endfor
#endif
% for type_name in vulkan_mapped_types:
std::string ToStr(const ${type_name}& c) {
  std::ostringstream oss;
  oss << "{ " << (void *)c << " }";
  return oss.str();
}

std::string ToStr(const ${type_name}* c) {
  std::ostringstream oss;
  if (c != nullptr) {
    if (Configurator::IsTraceDataOptPresent(TraceData::VK_STRUCTS))
      oss << ToStr(*c);
    else
      oss << "{ " << (void*)c << " }";
  } else {
    oss << "{ 0 }";
  }
  return oss.str();
}

% endfor

} // namespace Vulkan
} // namespace gits
