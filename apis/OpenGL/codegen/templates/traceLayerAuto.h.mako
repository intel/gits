// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "layerAuto.h"
#include "commandPrinter.h"

namespace gits::opengl {

// Pre hooks print to preStream and Post hooks to postStream.
class TraceLayer : public Layer {
public:
  TraceLayer(FastOStream& preStream, FastOStream& postStream, std::mutex& mutex, bool flush)
      : Layer("Trace"),
        m_PreStream(preStream),
        m_PostStream(postStream),
        m_Mutex(mutex),
        m_Flush(flush) {}

  void Pre(Command& command) override;
  void Post(Command& command) override;

% for cmd in commands:
% if cmd.enabled:
  void Pre(${cmd.name}Command& command) override;
  void Post(${cmd.name}Command& command) override;
% endif
% endfor
<%
  meta_commands = ['MappedDataMetaCommand', 'WindowGeometryMetaCommand', 'ClientArraysMetaCommand',
                   'UniformArrayMetaCommand', 'MappedTextureDataMetaCommand',
                   'PixelFormatMetaCommand', 'UnsupportedCommand']
%>\
% for command_class in meta_commands:
  void Pre(${command_class}& command) override;
  void Post(${command_class}& command) override;
% endfor

private:
  FastOStream& m_PreStream;
  FastOStream& m_PostStream;
  std::mutex& m_Mutex;
  bool m_Flush;
};

} // namespace gits::opengl