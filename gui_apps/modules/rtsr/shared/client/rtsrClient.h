// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrWireTypes.h"
#include "rtsr/shared/transport/rtsrTransport.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Graphics API agnostic client half of the RTSR protocol: it owns the three
// connections, the context handshake and the message plumbing, and knows nothing
// about D3D12 or Vulkan. The API specific tracking lives in each plugin.

namespace gits::rtsr {

// Every one of these runs on the transport's receive thread while the server
// blocks on the response. API-specific implementations may call thread-safe
// driver creation APIs, but must synchronize object lifetime with the thread
// driving the graphics API and return a result only after the operation is
// committed or rejected.
struct ClientCallbacks {
  std::function<RTSR_RESULT(const RTSR_UPDATE_PIPELINES_REQUEST&)> UpdatePipelines;
  std::function<RTSR_RESULT(const RTSR_RESET_PIPELINES_REQUEST&)> ResetPipelines;
  std::function<RTSR_RESULT()> CloseConnection;
  std::function<RTSR_RESULT(bool)> EnablePerformanceMeasurements;
};

struct ClientInfo {
  std::string ApplicationPath;
  std::string ApplicationName;
  RTSR_API UsedApi{RTSR_API_DX12};
};

class RtsrClient {
public:
  RtsrClient(EndpointConfig endpoints, LogSink log);
  ~RtsrClient();

  RtsrClient(const RtsrClient&) = delete;
  RtsrClient& operator=(const RtsrClient&) = delete;

  // Performs the context handshake. Unlike RTSR, which latches into passive mode
  // forever if the editor was not up at the exact moment the first pipeline was
  // created, this may be called again after a failure.
  bool Connect(const ClientInfo& info,
               ClientCallbacks callbacks,
               std::chrono::milliseconds timeout = std::chrono::seconds(2));

  // Releases the context and tears down the connections. Must be called while
  // the process is still healthy, because it round-trips a request: winsock is
  // gone by the time a DLL's static destructors run.
  void Disconnect();

  bool IsConnected() const {
    return m_State.load() == State::Connected;
  }

  // Telemetry towards the server. All are fire and forget and safe to call from
  // the thread driving the graphics API. Silently ignored when not connected.
  void PipelineCreated(RTSR_ID id,
                       RTSR_PIPELINE_TYPE type,
                       const RTSR_SERIALIZED_PIPELINE_STATE& state,
                       RTSR_HASH stateHash,
                       RTSR_PIPELINE_INCARNATION incarnation,
                       bool replacementEligible,
                       std::string replacementDiagnostic,
                       std::vector<RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages);
  void SetPipelineName(RTSR_ID id, const std::string& name);
  void PipelineDeleted(RTSR_ID id);
  void SetPipelineEligibility(RTSR_ID id,
                              RTSR_PIPELINE_INCARNATION incarnation,
                              bool eligible,
                              const std::string& diagnostic);
  void LogMessage(const std::string& message);
  void ReportTimeStats(RTSR_REPORT_TIME_STATS_MESSAGE stats);

private:
  enum class State {
    Disconnected,
    Connecting,
    Connected
  };

  void RegisterHandlers();
  Message MakeResponse(RTSR_RESULT result) const;
  // Drops a message whose payload failed to serialize. Forwarding one would put
  // a zero length body on the wire, and a peer that feeds that to cereal throws
  // where it cannot recover.
  void SendPush(Message message, const char* description);

  EndpointConfig m_Endpoints;
  LogSink m_Log;
  ClientCallbacks m_Callbacks;

  std::unique_ptr<IPushSender> m_PushSender;
  std::unique_ptr<IRequestSender> m_RequestSender;
  std::unique_ptr<IRequestReceiver> m_RequestReceiver;
  std::mutex m_LifecycleMutex;
  std::mutex m_SendMutex;

  // Read on the transport's receive thread while building responses, written on
  // the thread that connects.
  std::atomic<RTSR_CONTEXT> m_Context{};
  std::atomic<State> m_State{State::Disconnected};
};

} // namespace gits::rtsr
