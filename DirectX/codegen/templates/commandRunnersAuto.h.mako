// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandRunner.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandDecodersAuto.h"
#include "commandDecodersCustom.h"

#include <vector>

namespace gits {
namespace DirectX {

%for function in functions:
class ${function.name}Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  ${function.name}Command command;
};

%endfor
%for interface in interfaces:
%for function in interface.functions:
class ${interface.name}${function.name}Runner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  ${interface.name}${function.name}Command command;
};

%endfor
%endfor
} // namespace DirectX
} // namespace gits
