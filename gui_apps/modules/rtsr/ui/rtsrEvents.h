// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "baseEventBus.h"
#include "rtsr/shared/protocol/rtsrWireTypes.h"

#include <cstddef>
#include <string>

namespace gits::gui {

// One thing that happened in the RTSR session. Raised on a transport receive
// thread and delivered on the UI thread by EventBus::processEvents, which the
// frame loop drains before rendering.
//
// Only what a view needs to draw a row travels here. Shader bytecode and
// serialized pipeline state stay in the SessionStore, which the panel reads for
// the one pipeline the user has selected.
struct RtsrEvent : Event {
  enum class Type {
    SessionOpened,
    SessionClosed,
    PipelineCreated,
    PipelineNamed,
    PipelineDeleted,
    PipelineEligibilityChanged,
    ShaderCreated,
    // A source-cache request completed. Rendering reads the current cache;
    // no source text or widget reference crosses the event bus.
    ShaderSourceReady,
    // A compile/apply/reset operation changed state. The panel reads the
    // operation result from RtsrReplacementService.
    ReplacementUpdated,
    // One more frame of timings has been aggregated. Carries nothing: the panel
    // re-reads PerformanceStats, which is cheaper than putting a frame's worth
    // of pipeline timings on the bus once per frame.
    TimeStatsUpdated,
    Log
  };

  Type EventType{Type::Log};

  // SessionOpened.
  std::string ApplicationName;
  std::string ApplicationPath;
  rtsr::RTSR_API UsedApi{rtsr::RTSR_API_UNKNOWN};

  // PipelineCreated, PipelineNamed and PipelineDeleted. Name is only set by
  // PipelineNamed; a pipeline may never get one.
  rtsr::RTSR_ID PipelineId{};
  rtsr::RTSR_PIPELINE_TYPE PipelineType{rtsr::RTSR_PIPELINE_TYPE_UNKNOWN};
  size_t StageCount{};
  std::string Name;

  // ShaderCreated, raised once per distinct bytecode.
  rtsr::RTSR_HASH ShaderHash{};
  rtsr::RTSR_SHADER_STAGE ShaderStage{rtsr::RTSR_SHADER_STAGE_UNKNOWN};

  // Log, either a server diagnostic or a message the client forwarded.
  std::string Text;
};

} // namespace gits::gui
