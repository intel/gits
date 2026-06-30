// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandPlayer.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandCodersAuto.h"
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
class ${command.name}Player : public CommandPlayer {
public:
  uint32_t Id() const override {
	return static_cast<uint32_t>(CommandId::ID_${command.name.upper()});
  }

  const char* Name() const override {
    return "${command.name}";
  }

  void Run() override;

protected:
  void DecodeCommand() override {
	Decode(m_Data.get(), command);
  }

private:
  ${command.name}Command command;
};
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
