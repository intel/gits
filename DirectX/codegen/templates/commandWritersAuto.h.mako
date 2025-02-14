// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandWriter.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandIdsAuto.h"
#include "commandEncodersCustom.h"
#include "commandEncodersAuto.h"

namespace gits {
namespace DirectX {

%for function in functions:
class ${function.name}Writer : public CommandWriter {
public:
  ${function.name}Writer(${function.name}Command& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${function.name.upper()});
  }
};

%endfor
%for interface in interfaces:
%for function in interface.functions:
class ${interface.name}${function.name}Writer : public CommandWriter {
public:
  ${interface.name}${function.name}Writer(${interface.name}${function.name}Command& command) {
    dataSize_ = getSize(command);
    data_.reset(new char[dataSize_]);
    encode(command, data_.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${interface.name.upper()}_${function.name.upper()});
  }
};

%endfor
%endfor
} // namespace DirectX
} // namespace gits
