// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <memory>
#include <mutex>

namespace gits {
namespace DirectX {

class FastOStream;
class FastOStringStream;

/*
 * Encapsulates creation logic of tracing-related Layers.
 * Assembles file paths, creates output streams and decides which layers to create.
 */
class TraceFactory {
public:
  TraceFactory();
  ~TraceFactory();

  TraceFactory(const TraceFactory&) = delete;
  TraceFactory& operator=(const TraceFactory&) = delete;

  std::unique_ptr<Layer> getTraceLayer() {
    return std::move(traceLayer_);
  }
  std::unique_ptr<Layer> getShowExecutionLayer() {
    return std::move(showExecutionLayer_);
  }

private:
  std::unique_ptr<FastOStream> traceStream_;
  std::unique_ptr<FastOStream> traceStreamPre_;
  std::unique_ptr<FastOStringStream> showExecutionStream_;
  std::unique_ptr<Layer> traceLayer_;
  std::unique_ptr<Layer> showExecutionLayer_;
  std::mutex traceMutex_;
};

} // namespace DirectX
} // namespace gits
