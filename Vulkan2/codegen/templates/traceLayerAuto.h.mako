// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"

#include "commandPrinter.h"
#include <mutex>

namespace gits {
namespace vulkan {

class TraceLayer : public Layer {
public:
  TraceLayer(FastOStream& streamPre, FastOStream& streamPost, std::mutex& mutex, bool flush)
      : Layer("Trace"),
        statePre_(mutex),
        statePost_(mutex),
        streamPre_(streamPre),
        streamPost_(streamPost),
        printPre_(streamPre.IsOpen()),
        printPost_(streamPost.IsOpen()),
        flush_(flush) {}

  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  void Pre(${command.name}Command& command) override;
  void Post(${command.name}Command& command) override;
  % if define:
  #endif
  % endif
  % endfor

  // TraceLayerCustom
  void Pre(StateRestoreBeginCommand& command);
  void Post(StateRestoreBeginCommand& command);

  void Pre(StateRestoreEndCommand& command);
  void Post(StateRestoreEndCommand& command);

  void Pre(MarkerUInt64Command& command);
  void Post(MarkerUInt64Command& command);

  void Pre(CreateWindowMetaCommand& command) override;
  void Post(CreateWindowMetaCommand& command) override;

  void Pre(MappedDataMetaCommand& command) override;
  void Post(MappedDataMetaCommand& command) override;

protected:
  CommandPrinterState statePre_;
  CommandPrinterState statePost_;
  FastOStream& streamPre_;
  FastOStream& streamPost_;
  bool printPre_;
  bool printPost_;
  bool flush_;
};

} // namespace vulkan
} // namespace gits