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
  void Pre(MappedDataMetaCommand& command) override;
  void Post(MappedDataMetaCommand& command) override;
  void Pre(WindowGeometryMetaCommand& command) override;
  void Post(WindowGeometryMetaCommand& command) override;
  void Pre(ClientArraysMetaCommand& command) override;
  void Post(ClientArraysMetaCommand& command) override;
  void Pre(UniformArrayMetaCommand& command) override;
  void Post(UniformArrayMetaCommand& command) override;
  void Pre(MappedTextureDataMetaCommand& command) override;
  void Post(MappedTextureDataMetaCommand& command) override;
  void Pre(PixelFormatMetaCommand& command) override;
  void Post(PixelFormatMetaCommand& command) override;
  void Pre(UnsupportedCommand& command) override;
  void Post(UnsupportedCommand& command) override;

private:
  FastOStream& m_PreStream;
  FastOStream& m_PostStream;
  std::mutex& m_Mutex;
  bool m_Flush;
};

} // namespace gits::opengl