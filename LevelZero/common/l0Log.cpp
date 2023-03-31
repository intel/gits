// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Log.h"

namespace gits {
namespace l0 {

CL0Log& CL0Log::operator<<(manip t) {
  _buffer << t;
  return *this;
}

CL0Log& CL0Log::operator<<(const char c) {
  return operator<< <int>(c);
}

CL0Log& CL0Log::operator<<(const unsigned char c) {
  return operator<< <unsigned>(c);
}

CL0Log& CL0Log::operator<<(const char* c) {
  if (c != nullptr) {
    _buffer << c;
  } else {
    _buffer << (const void*)c;
  }
  return *this;
}

CL0Log& CL0Log::operator<<(char* c) {
  _buffer << (const void*)c;
  return *this;
}

CL0Log& CL0Log::operator<<(std::string s) {
  _buffer << s;
  return *this;
}
} // namespace l0
} // namespace gits
