// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
// This file is auto-generated, manual changes will be lost on next run.
//
// generated @ ${time}

#include "enumsAuto.h"
#include "log.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cctype>

%for enum in enums:
namespace gits {

template<>
${enum.name} stringTo<${enum.name}>(const std::string& str) {
  static const std::unordered_map<std::string, ${enum.name}> stringToEnumMap = {
% for value in enum.values:
% for label in value.labels:
    {"${label.upper()}", ${enum.name}::${value.value}},
% endfor
% endfor
  };

  std::string key = str;
  const std::string prefix = "${enum.name}::";
  if (key.rfind(prefix, 0) == 0) {
    key = key.substr(prefix.length());
  }
  std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
    return std::toupper(c);
  });
  auto it = stringToEnumMap.find(key);
  if (it != stringToEnumMap.end()) {
    return it->second;
  } else {
    throw std::invalid_argument("Invalid string for enum ${enum.name}: " + str);
  }
}

template<>
std::string stringFrom<${enum.name}>(const ${enum.name}& value) {
  static const std::unordered_map<${enum.name}, std::string> enumToStringMap = {
% for value in enum.values:
    {${enum.name}::${value.value}, "${value.labels[0]}"},
% endfor
  };
  auto it = enumToStringMap.find(value);
  if (it != enumToStringMap.end()) {
    return it->second;
  } else {
    throw std::invalid_argument("Invalid enum value for ${enum.name}");
  }
}

std::ostream& operator<<(std::ostream& out, const gits::${enum.name}& value) {
  try {
    out << gits::stringFrom<gits::${enum.name}>(value);
  } catch (const std::invalid_argument& e) {
    LOG_ERROR << "Caught an invalid_argument exception: " << e.what() << std::endl;
    out.setstate(std::ios::failbit);
  } catch (const std::exception& e) {
    LOG_ERROR << "Caught a general exception: " << e.what() << std::endl;
    out.setstate(std::ios::failbit);
  }
  return out;
}
} // namespace gits

// this is for args.hxx which requires this to be in the global namespace
std::istream& operator>>(std::istream& in, gits::${enum.name}& value) {
  std::string token;
  in >> token;
  try {
    value = gits::stringTo<gits::${enum.name}>(token);
  } catch (const std::invalid_argument& e) {
    LOG_ERROR << "Caught an invalid_argument exception ('" << token << "' as gits::${enum.name}): " << e.what() << std::endl;
    in.setstate(std::ios::failbit);
  } catch (const std::exception& e) {
    LOG_ERROR << "Caught a general exception: " << e.what() << std::endl;
    in.setstate(std::ios::failbit);
  }
  return in;
}

%endfor
