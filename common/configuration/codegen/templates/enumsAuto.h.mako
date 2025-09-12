// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
// This file is auto-generated, manual changes will be lost on next run.
//
// generated @ ${time}

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace gits {
template <typename T>
T stringTo(const std::string& str);

template <typename T>
std::string stringFrom(const T& value);
} // namespace gits

%for enum in enums:
namespace gits {
% if enum.description:
## /*! @brief ${enum.description} */
% endif
% if enum.type:
enum class ${enum.name} : ${enum.type} {
% else:
enum class ${enum.name} {
% endif
%  for i in range(len(enum.values)):
%    if enum.values[i].short_description:
## /*! ${enum.values[i].short_description} */
%    endif
%    if enum.values[i].description:
## /*! ${enum.values[i].description} */
%    endif
%    if i == 0:
  ${enum.values[i].value} = 0,
%    elif i == len(enum.values) - 1:
  ${enum.values[i].value}
%    else:
  ${enum.values[i].value},
%    endif
%  endfor
};

template<>
${enum.name} stringTo<${enum.name}>(const std::string& str);

template<>
std::string stringFrom<${enum.name}>(const ${enum.name}& value);

std::ostream& operator<<(std::ostream& out, const gits::${enum.name}& value);
} // namespace gits

// this is for args.hxx which requires this to be in the global namespace
std::istream& operator>>(std::istream& in, gits::${enum.name}& value);

%endfor
