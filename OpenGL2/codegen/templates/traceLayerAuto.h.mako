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

namespace gits::opengl2 {

class TraceLayer : public Layer {
public:
  TraceLayer(FastOStream& stream, std::mutex& mutex, bool flush)
      : Layer("Trace"), m_Stream(stream), m_Mutex(mutex), m_Flush(flush) {}

  void Pre(Command& command) override;

% for cmd in commands:
% if cmd.enabled:
  void Pre(${cmd.name}Command& command) override;
% endif
% endfor
  void Pre(MappedDataMetaCommand& command) override;
  void Pre(WindowGeometryMetaCommand& command) override;
  void Pre(UniformArrayMetaCommand& command) override;
  void Pre(UnsupportedCommand& command) override;

private:
  FastOStream& m_Stream;
  std::mutex& m_Mutex;
  bool m_Flush;
};

} // namespace gits::opengl2