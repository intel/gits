// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrModule.h"
#include "rtsr/ui/rtsrEvents.h"

#include "baseEventBus.h"
#include "log.h"

#include <string>

namespace gits::gui {

namespace {

void Publish(const RtsrEvent& event) {
  EventBus::GetInstance().publish<RtsrEvent>(event);
}

void PublishLog(std::string text) {
  RtsrEvent event;
  event.EventType = RtsrEvent::Type::Log;
  event.Text = std::move(text);
  Publish(event);
}

// Diagnostics from the server go to the GITS log and to the panel's log pane.
// Both are reached from a receive thread: plog serializes its own writes, and
// the event bus is a queue drained by the frame loop. Nothing here touches a
// widget, which is the mistake RTSREditor makes when it writes its status bar
// straight from the socket thread.
rtsr::LogSink MakeLogSink() {
  return [](std::string_view message) {
    std::string text(message);
    LOG_INFO << text;
    PublishLog(std::move(text));
  };
}

} // namespace

RtsrModule::RtsrModule() : m_Server(m_Store, MakeLogSink()), m_Replacements(m_Store, m_Server) {}

RtsrModule::~RtsrModule() {
  Stop();
}

bool RtsrModule::Start() {
  rtsr::ServerCallbacks callbacks;

  callbacks.SessionOpened = [this](const rtsr::SessionInfo& info) {
    // Dropped where the store is emptied, so a new run never appears appended
    // to the previous one's frames. Nothing can have arrived for this run yet,
    // even though the session is already accepting: a client reports timings
    // only after being asked to, and asking takes a click.
    m_Performance.Clear();
    m_Replacements.Clear();
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::SessionOpened;
    event.ApplicationName = info.ApplicationName;
    event.ApplicationPath = info.ApplicationPath;
    event.UsedApi = info.UsedApi;
    Publish(event);
  };

  callbacks.SessionClosed = [this]() {
    // The timings this run produced are kept: the next run clears them, and so
    // does disabling the module.
    m_Replacements.Clear();
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::SessionClosed;
    Publish(event);
  };

  callbacks.PipelineCreated = [this](rtsr::RTSR_ID id, rtsr::RTSR_PIPELINE_TYPE type,
                                     size_t stageCount) {
    m_Replacements.ForgetPipeline(id);
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::PipelineCreated;
    event.PipelineId = id;
    event.PipelineType = type;
    event.StageCount = stageCount;
    Publish(event);
  };

  callbacks.PipelineEligibilityChanged = [](rtsr::RTSR_ID id) {
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::PipelineEligibilityChanged;
    event.PipelineId = id;
    Publish(event);
  };

  callbacks.PipelineNamed = [](rtsr::RTSR_ID id, const std::string& name) {
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::PipelineNamed;
    event.PipelineId = id;
    event.Name = name;
    Publish(event);
  };

  callbacks.PipelineDeleted = [this](rtsr::RTSR_ID id) {
    m_Replacements.ForgetPipeline(id);
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::PipelineDeleted;
    event.PipelineId = id;
    Publish(event);
  };

  callbacks.ShaderCreated = [](rtsr::RTSR_HASH hash, rtsr::RTSR_SHADER_STAGE stage) {
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::ShaderCreated;
    event.ShaderHash = hash;
    event.ShaderStage = stage;
    Publish(event);
  };

  // Aggregated here on the receive thread, which is bounded work, and only a
  // bare notification crosses to the UI thread.
  callbacks.TimeStatsReported = [this](rtsr::RTSR_REPORT_TIME_STATS_MESSAGE stats) {
    m_Performance.Add(stats);
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::TimeStatsUpdated;
    Publish(event);
  };

  // Prefixed so the pane shows whose voice it is: everything else in there is
  // ours.
  callbacks.ClientLog = [](const std::string& message) { PublishLog("client: " + message); };

  return m_Server.Start(rtsr::EndpointConfig{}, std::move(callbacks));
}

void RtsrModule::Stop() {
  // Invalidate module-side work first. Stopping the server then closes the
  // outbound request socket (which releases a worker waiting for a client),
  // joins the receive threads and empties the store.
  m_Replacements.Clear();
  m_Server.Stop();
  m_Performance.Clear();
}

bool RtsrModule::EnablePerformanceMeasurements(bool enable) {
  return m_Server.EnablePerformanceMeasurements(enable);
}

} // namespace gits::gui
