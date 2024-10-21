// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools_lite.cpp
 *
 * @brief Common dependencyless tools for GITS project.
 *
 * This file contains various tools and utilities that do not require including
 * headers other than the standard library. This is in contrast to the regular
 * tools.cpp, which does not have this constraint.
 */

#include "tools_lite.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace gits {

int log2i(int value) {
  int result = 0;
  while (value >>= 1) {
    result++;
  }
  return result;
}

std::string bytesToHex(const uint8_t* const bytes, const size_t length) {
  std::string ret(2 * length, ' ');
  const char digits[] = "0123456789abcdef";
  auto iter = ret.begin();
  for (size_t i = 0; i < length; ++i) {
    *iter++ = digits[bytes[i] >> 4];
    *iter++ = digits[bytes[i] & 0x0f];
  }
  return ret;
}

void ReverseByPairs(std::string& str) {
  if (str.size() & 1) {
    return;
  }
  std::reverse(str.begin(), str.end());
  for (size_t i = 0; i + 1 < str.size(); i += 2) {
    std::swap(str[i], str[i + 1]);
  }
}

bool caseInsensitiveEquals(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (auto iterA = a.begin(), iterB = b.begin(); iterA != a.end(); ++iterA, ++iterB) {
    if (tolower(*iterA) != tolower(*iterB)) {
      return false;
    }
  }

  return true;
}

std::string ToLowerCopy(const std::string& s) {
  std::string lowercase;
  lowercase.reserve(s.size());

  // Select the right overload.
  std::transform(s.begin(), s.end(), std::back_inserter(lowercase), ::tolower);

  return lowercase;
}

const std::string getVarName(const std::string prefix, const void* ptr) {
  std::stringstream ss;
  ss << ptr;
  return prefix + ss.str();
}
const std::string getVarName(const void* ptr) {
  return getVarName("var_", ptr);
}

// Non-Windows getline implementations cannot deal with specific line endings.
std::istream& uniGetLine(std::istream& is, std::string& line) {
  line.clear();
  std::istream::sentry sen(is);
  std::streambuf* sb = is.rdbuf();
  while (1) {
    const int c = sb->sbumpc();
    switch (c) {
    case '\r':
      if (sb->sgetc() == '\n') {
        sb->sbumpc();
      }
      [[fallthrough]];
    case '\n':
    case EOF:
      return is;
    default:
      line += (char)c;
    }
  }
}

bool StringEndsWith(const std::string& name, const std::string& suffix) {
  if (suffix.size() > name.size()) {
    return false;
  }
  return std::equal(suffix.rbegin(), suffix.rend(), name.rbegin());
}

void sleep_millisec(int duration) {
  std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

std::vector<char> GetBinaryFileContents(const std::string& filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (file.fail()) {
    std::cout << "Could not open \"" << filename << "\" file." << std::endl;
    return std::vector<char>();
  }

  file.seekg(0, std::ios::end);
  const std::streampos fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> result(static_cast<size_t>(fileSize));
  file.read(result.data(), fileSize);

  return result;
}
void SaveBinaryFileContents(const std::string& filename, const std::vector<char>& data) {
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  file.write(data.data(), data.size() * sizeof(char));
}

void hex::Write(std::ostream& stream) const {
  // setw sets MINIMUM output width; if the value has high bits != 0 it will
  // print all of them. On the other hand snprintf can truncate the output, but
  // it will display high bits when we want low bits. We can't use bytesToHex
  // because it prints values as is, without considering endianness so it's
  // useless here.
  uintptr_t masked_value = _value;
  switch (_width) {
  case 2:
    masked_value = _value & 0xff;
    break;
  case 4:
    masked_value = _value & 0xffff;
    break;
  case 8:
    masked_value = _value & 0xffffffff;
    break;
  case 16:
  default:
    // Nothing to do.
    break;
  }
  std::ios state(nullptr);
  state.copyfmt(stream);
  stream << "0x";
  stream << std::hex << std::uppercase << std::noshowbase << std::setw(_width) << std::setfill('0');
  stream << masked_value;
  stream.copyfmt(state);
}
std::string hex::ToString() const {
  std::stringstream s;
  s.str("");
  s.clear();
  this->Write(s);
  return s.str();
}
std::ostream& operator<<(std::ostream& stream, const hex& h) {
  h.Write(stream);
  return stream;
}

} // namespace gits
