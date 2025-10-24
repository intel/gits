// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclTracing.h"

namespace gits {
namespace OpenCL {
%for name, enum in without_field(enums, 'type').items():
extern std::string ${name}ToString(${name});
%endfor
extern std::string CLResultToString(cl_int);

%for name, enum in without_field(enums, 'type').items():
class T${name} : public TOclType {                                                                   
private:
  ${name} value;

public:
  T${name}(${name} val) : value(val) {}
  virtual std::string ToString() const {
    return ${name}ToString(value);
  }
};

%endfor
class TCLResult : public TOclType {
private:
  cl_int value;

public:
  TCLResult(cl_int val) : value(val) {}
  virtual std::string ToString() const {
    return CLResultToString(value);
  }
};
} // namespace OpenCL
} // namespace gits
