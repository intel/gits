// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandPlayer.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandDecodersAuto.h"
#include "commandDecodersCustom.h"

#include <vector>

namespace gits {
namespace DirectX {

%for function in functions:
class ${function.name}Player : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${function.name.upper()});
  }
  const char* Name() const override {
    return "${function.name}";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  ${function.name}Command command;
};

%endfor
%for interface in interfaces:
%for function in interface.functions:
class ${interface.name}${function.name}Player : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${interface.name.upper()}_${function.name.upper()});
  }
  const char* Name() const override {
    return "${interface.name}::${function.name}";
  }
  void Run() override;

protected:
  void decodeCommand() override {
    decode(data_.get(), command);
  }

private:
  ${interface.name}${function.name}Command command;
};

%endfor
%endfor
} // namespace DirectX
} // namespace gits
