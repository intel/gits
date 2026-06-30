// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"

#include <string>

namespace gits {
namespace vulkan {

class Layer {
public:
  Layer(const std::string& name) : m_Name(name) {}
  virtual ~Layer() {}
  
  const std::string& GetName() const {
    return m_Name;
  }

  virtual void Pre(StateRestoreBeginCommand& command) {}
  virtual void Post(StateRestoreBeginCommand& command) {}

  virtual void Pre(StateRestoreEndCommand& command) {}
  virtual void Post(StateRestoreEndCommand& command) {}

  virtual void Pre(FrameEndCommand& command) {}
  virtual void Post(FrameEndCommand& command) {}

  virtual void Pre(MarkerUInt64Command& command) {}
  virtual void Post(MarkerUInt64Command& command) {}

  virtual void Pre(CreateWindowMetaCommand& command) {}
  virtual void Post(CreateWindowMetaCommand& command) {}

  virtual void Pre(MappedDataMetaCommand& command) {}
  virtual void Post(MappedDataMetaCommand& command) {}
  
  virtual void Pre(UpdateWindowMetaCommand& command) {}
  virtual void Post(UpdateWindowMetaCommand& command) {}

  %for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  virtual void Pre(${command.name}Command& command) {}
  virtual void Post(${command.name}Command& command) {}
  % if define:
  #endif
  % endif

  %endfor
private:
  std::string m_Name;
};

} // namespace vulkan
} // namespace gits
