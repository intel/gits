// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclHeader.h"
#include "openclTools.h"

#include "argument.h"
#include "openclArgumentsHelper.h"

#include <string>
#include <vector>

namespace gits {
namespace OpenCL {
%for name, enum in enums.items():
std::string ${name}ToString(const ${enum.get('type') if enum.get('type') else name} value);
%endfor

%for name, enum in without_field(enums, 'custom_argument').items():
class C${name} : public CCLArg<${enum.get('type') if enum.get('type') else name}, C${name}> {
public:
  static const char* NAME;
  C${name}(): CCLArg() {}
  C${name}(CLType value): CCLArg(value) {}
  %if enum.get('custom_tostring'):
  virtual std::string ToString() const override;
  %else:
  virtual std::string ToString() const { return ${name}ToString(Value()); }
  %endif
};

%endfor

%for name, arg in arguments.items():
class C${name} : public ${'CCLArgObj' if arg.get('obj') else 'CCLArg'}<${name}, C${name}> {
public:
  static const char* NAME;
  C${name}(): ${'CCLArgObj' if arg.get('obj') else 'CCLArg'}() {}
  C${name}(CLType value): ${'CCLArgObj' if arg.get('obj') else 'CCLArg'}(value) {}
  %if arg.get('custom_tostring'):
  virtual std::string ToString() const override;
  %endif
};

%endfor
} // namespace OpenCL
} // namespace gits
