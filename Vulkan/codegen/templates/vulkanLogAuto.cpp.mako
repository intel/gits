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
CVkLog & CVkLog::operator<<(const ${struct.name}& c) {
  ${make_struct_log_code(struct.fields)}
  return *this;
}

CVkLog & CVkLog::operator<<(const ${struct.name}* c) {
  if (c != nullptr) {
    if (isTraceDataOptPresent(TraceData::VK_STRUCTS))
      *this << *c;
    else
      _buffer << "{ " << (void*)c << " }";
  } else {
    _buffer << "{ 0 }";
  }
  return *this;
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
CVkLog & CVkLog::operator<<(const ${enum.name}& c) {
  std::underlying_type_t<${enum.name}> e = c;
  std::ostringstream os;
  % if zero_enumerator is not None:
  if (e == 0) {
    os << "${zero_enumerator.name} | ";
  }
  % endif
  % for enumerator in sorted_enumerators:
  if (isBitSet(e, ${enumerator.name})) {
    os << "${enumerator.name} | ";
    e &= ~${enumerator.name};
  }
  % endfor
  for (decltype(e) i = 1; i <= e; i <<= 1) {
    if (i & e) {
      os << i << " | ";
      Log(WARN) << "Unknown enum number: " << i << " for ${enum.name}";
      break;
    }
  }
  std::string str = os.str();
  if (str.size() > 3) {
    str = str.substr(0, str.size() - 3);
  } else {
    str = "0";
  }
  _buffer << "{ " << str << " }";
  return *this;
}
% else:
CVkLog & CVkLog::operator<<(const ${enum.name}& c) {
  switch (c) {
    % for enumerator in enum.enumerators:
    case ${enumerator.value}:
      _buffer << "${enumerator.name}";
      break;
    %endfor
    default:
      _buffer << c;
      Log(WARN) << "Unknown enum number: " << c << " for ${enum.name}";
      break;
  }
  return *this;
}
% endif

% endfor
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
% for type_name in vulkan_mapped_types_nondisp:
CVkLog & CVkLog::operator<<(const ${type_name}& c) {
  *this << "{ " << (void*)c << " }";
  return *this;
}

CVkLog & CVkLog::operator<<(const ${type_name}* c) {
  if (c != nullptr) {
    if (isTraceDataOptPresent(TraceData::VK_STRUCTS))
      *this << *c;
    else
      _buffer << "{ " << (void*)c << " }";
  } else {
    _buffer << "{ 0 }";
  }
  return *this;
}

% endfor
#endif
% for type_name in vulkan_mapped_types:
CVkLog & CVkLog::operator<<(const ${type_name}& c) {
  *this << "{ " << (void *)c << " }";
  return *this;
}

CVkLog & CVkLog::operator<<(const ${type_name}* c) {
  if (c != nullptr) {
    if (isTraceDataOptPresent(TraceData::VK_STRUCTS))
      *this << *c;
    else
      _buffer << "{ " << (void*)c << " }";
  } else {
    _buffer << "{ 0 }";
  }
  return *this;
}

% endfor

} // namespace Vulkan
} // namespace gits
