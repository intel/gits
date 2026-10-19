// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/shared/client/rtsrClient.h"
#include "rtsr/shared/protocol/rtsrSerialization.h"

#include <optional>
#include <utility>

namespace gits::rtsr {

namespace {

void Report(const LogSink& log, const std::string& message) {
  if (log) {
    log(message);
  }
}

// A future whose promise was destroyed unanswered throws on get(). The transport
// guards against that, but a torn-down sender must never surface as an exception
// crossing into GITS.
std::optional<Message> AwaitResponse(std::future<Message>& future,
                                     std::chrono::milliseconds timeout) {
  try {
    if (future.wait_for(timeout) != std::future_status::ready) {
      return std::nullopt;
    }
    return future.get();
  } catch (...) {
    return std::nullopt;
  }
}

} // namespace

RtsrClient::RtsrClient(EndpointConfig endpoints, LogSink log)
    : m_Endpoints(std::move(endpoints)), m_Log(std::move(log)) {}

RtsrClient::~RtsrClient() {
  Disconnect();
}

Message RtsrClient::MakeResponse(RTSR_RESULT result) const {
  return Serialize(RTSR_BASIC_CALLBACK_RESPONSE{m_Context.load(), result},
                   RTSR_BASIC_CALLBACK_RESPONSE_MESSAGE_TYPE);
}

void RtsrClient::SendPush(Message message, const char* description) {
  if (message.Header.Len <= 0) {
    Report(m_Log, std::string("Rtsr client: dropping a ") + description +
                      " message that could not be serialized");
    return;
  }
  std::lock_guard<std::mutex> lock(m_SendMutex);
  if (m_PushSender) {
    m_PushSender->Send(std::move(message));
  }
}

void RtsrClient::RegisterHandlers() {
  m_RequestReceiver->RegisterCallback(
      RTSR_BIND_CONTEXT_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        const auto request = Deserialize<RTSR_BIND_CONTEXT_REQUEST>(message);
        if (!request.has_value() || request->Context == 0) {
          return MakeResponse(RTSR_RESULT_INVALID_ARGUMENT);
        }
        const State state = m_State.load();
        if (state == State::Connecting) {
          const RTSR_CONTEXT current = m_Context.load();
          if (current == 0 || current == request->Context) {
            m_Context.store(request->Context);
            return MakeResponse(RTSR_RESULT_SUCCESS);
          }
        }
        return MakeResponse(state == State::Connected && m_Context.load() == request->Context
                                ? RTSR_RESULT_SUCCESS
                                : RTSR_RESULT_FAIL);
      });

  m_RequestReceiver->RegisterCallback(
      RTSR_UPDATE_PIPELINES_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        auto request = Deserialize<RTSR_UPDATE_PIPELINES_REQUEST>(message);
        if (!request.has_value()) {
          Report(m_Log, "Rtsr client: malformed update pipelines request");
          return MakeResponse(RTSR_RESULT_INVALID_ARGUMENT);
        }
        if (!m_Callbacks.UpdatePipelines) {
          return MakeResponse(RTSR_RESULT_FAIL);
        }
        return MakeResponse(m_Callbacks.UpdatePipelines(*request));
      });

  m_RequestReceiver->RegisterCallback(
      RTSR_RESET_PIPELINES_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        auto request = Deserialize<RTSR_RESET_PIPELINES_REQUEST>(message);
        if (!request.has_value()) {
          Report(m_Log, "Rtsr client: malformed reset pipelines request");
          return MakeResponse(RTSR_RESULT_INVALID_ARGUMENT);
        }
        if (!m_Callbacks.ResetPipelines) {
          return MakeResponse(RTSR_RESULT_FAIL);
        }
        return MakeResponse(m_Callbacks.ResetPipelines(*request));
      });

  m_RequestReceiver->RegisterCallback(
      RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        auto request = Deserialize<RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST>(message);
        if (!request.has_value()) {
          return MakeResponse(RTSR_RESULT_INVALID_ARGUMENT);
        }
        if (!m_Callbacks.EnablePerformanceMeasurements) {
          return MakeResponse(RTSR_RESULT_FAIL);
        }
        return MakeResponse(m_Callbacks.EnablePerformanceMeasurements(request->enable));
      });

  // Answer, mark the session closed and stop sending. The teardown itself is
  // left to the owner. RTSR instead deletes its whole client from a detached
  // thread while the render thread may still be inside a send call.
  m_RequestReceiver->RegisterCallback(RTSR_CLOSE_CONNECTION_REQUEST_MESSAGE_TYPE,
                                      [this](const Message&) {
                                        m_State.store(State::Disconnected);
                                        Report(m_Log, "Rtsr client: the server closed the session");
                                        RTSR_RESULT result = RTSR_RESULT_SUCCESS;
                                        if (m_Callbacks.CloseConnection) {
                                          result = m_Callbacks.CloseConnection();
                                        }
                                        return MakeResponse(result);
                                      });

  m_RequestReceiver->RegisterCallback(
      RTSR_CHECK_IF_ALIVE_REQUEST_MESSAGE_TYPE,
      [this](const Message&) { return MakeResponse(RTSR_RESULT_SUCCESS); });
}

bool RtsrClient::Connect(const ClientInfo& info,
                         ClientCallbacks callbacks,
                         std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);
  if (m_State.load() == State::Connected) {
    return true;
  }

  // A server-initiated close only marks the connection from the receive
  // callback; teardown is done here, from a thread that can safely join it.
  if (m_RequestReceiver) {
    m_RequestReceiver->Stop();
  }
  m_RequestReceiver.reset();
  m_RequestSender.reset();
  {
    std::lock_guard<std::mutex> sendLock(m_SendMutex);
    m_PushSender.reset();
  }
  m_Callbacks = {};
  m_Context.store(0);
  m_State.store(State::Disconnected);

  State expected = State::Disconnected;
  if (!m_State.compare_exchange_strong(expected, State::Connecting)) {
    return expected == State::Connected;
  }

  const auto fail = [this]() {
    m_RequestReceiver.reset();
    m_RequestSender.reset();
    {
      std::lock_guard<std::mutex> lock(m_SendMutex);
      m_PushSender.reset();
    }
    m_Callbacks = {};
    m_Context.store(0);
    m_State.store(State::Disconnected);
    return false;
  };

  m_Callbacks = std::move(callbacks);

  // Callbacks first, then handlers, then the listening thread. Starting the
  // receiver before its handlers exist leaves a window in which an incoming
  // request dereferences nothing, which is a real defect in RTSR's client.
  m_RequestReceiver = CreateRequestReceiver(m_Endpoints.ClientRequestPort, m_Log);
  if (!m_RequestReceiver) {
    Report(m_Log, "Rtsr client: could not listen for server requests, is another client running?");
    return fail();
  }
  RegisterHandlers();

  m_PushSender = CreatePushSender(m_Endpoints.Address, m_Endpoints.ServerPushPort, m_Log);
  m_RequestSender = CreateRequestSender(m_Endpoints.Address, m_Endpoints.ServerRequestPort, m_Log);
  if (!m_PushSender || !m_RequestSender) {
    Report(m_Log, "Rtsr client: could not create the outbound connections");
    return fail();
  }

  m_RequestReceiver->Start();
  m_PushSender->Start();
  m_RequestSender->Start();

  RTSR_NEGOTIATE_PROTOCOL_REQUEST protocol;
  protocol.Minor = info.UsedApi >= RTSR_API_VULKAN_1_0 && info.UsedApi <= RTSR_API_VULKAN_1_3
                       ? RTSR_PROTOCOL_MINOR
                       : 0;
  auto negotiationFuture =
      m_RequestSender->Send(Serialize(protocol, RTSR_NEGOTIATE_PROTOCOL_REQUEST_MESSAGE_TYPE));
  auto negotiationReply = AwaitResponse(negotiationFuture, timeout);
  if (!negotiationReply.has_value()) {
    Report(m_Log, "Rtsr client: the server did not negotiate the wire protocol");
    return fail();
  }
  const auto negotiation = Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(*negotiationReply);
  if (!negotiation.has_value() || RtsrFailed(negotiation->Result)) {
    Report(m_Log, "Rtsr client: the server uses an incompatible wire protocol");
    return fail();
  }

  RTSR_CREATE_CONTEXT_MESSAGE request;
  request.ApplicationPath = info.ApplicationPath;
  request.ApplicationName = info.ApplicationName;
  request.UsedApi = info.UsedApi;

  auto future = m_RequestSender->Send(Serialize(request, RTSR_CREATE_CONTEXT_REQUEST_MESSAGE_TYPE));
  auto reply = AwaitResponse(future, timeout);
  if (!reply.has_value()) {
    Report(m_Log, "Rtsr client: no server answered the context handshake");
    return fail();
  }

  auto response = Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(*reply);
  if (!response.has_value() || RtsrFailed(response->Result)) {
    Report(m_Log, "Rtsr client: the server rejected the context handshake");
    return fail();
  }

  if (response->Context == 0 || m_Context.load() != response->Context) {
    Report(m_Log, "Rtsr client: the server bound a different callback connection");
    return fail();
  }
  expected = State::Connecting;
  if (!m_State.compare_exchange_strong(expected, State::Connected)) {
    Report(m_Log, "Rtsr client: the session was closed during its context handshake");
    return fail();
  }
  Report(m_Log, "Rtsr client: session established");
  return true;
}

void RtsrClient::Disconnect() {
  std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);
  const bool wasConnected = m_State.exchange(State::Disconnected) == State::Connected;
  if (m_RequestSender && wasConnected) {
    // Best effort: the server may already be gone, in which case the future
    // simply never becomes ready.
    auto future = m_RequestSender->Send(Serialize(RTSR_DELETE_CONTEXT_MESSAGE{m_Context.load()},
                                                  RTSR_DELETE_CONTEXT_REQUEST_MESSAGE_TYPE));
    AwaitResponse(future, std::chrono::seconds(2));
  }

  // The receiver stops softly so a response in flight is delivered first.
  if (m_RequestReceiver) {
    m_RequestReceiver->Stop();
  }
  m_RequestReceiver.reset();
  m_RequestSender.reset();
  {
    std::lock_guard<std::mutex> sendLock(m_SendMutex);
    m_PushSender.reset();
  }
  m_Callbacks = {};
  m_Context.store(0);
}

void RtsrClient::PipelineCreated(RTSR_ID id,
                                 RTSR_PIPELINE_TYPE type,
                                 const RTSR_SERIALIZED_PIPELINE_STATE& state,
                                 RTSR_HASH stateHash,
                                 RTSR_PIPELINE_INCARNATION incarnation,
                                 bool replacementEligible,
                                 std::string replacementDiagnostic,
                                 std::vector<RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages) {
  if (!IsConnected()) {
    return;
  }

  RTSR_PIPELINE_CREATED_MESSAGE message;
  message.Context = m_Context.load();
  message.Id = id;
  message.Type = type;
  message.State = state;
  message.StateHash = stateHash;
  message.Incarnation = incarnation;
  message.ReplacementEligible = replacementEligible;
  message.ReplacementDiagnostic = std::move(replacementDiagnostic);
  message.ShaderStages = std::move(stages);

  SendPush(Serialize(message, RTSR_PIPELINE_CREATED_MESSAGE_TYPE), "pipeline created");
}

void RtsrClient::SetPipelineEligibility(RTSR_ID id,
                                        RTSR_PIPELINE_INCARNATION incarnation,
                                        bool eligible,
                                        const std::string& diagnostic) {
  if (!IsConnected()) {
    return;
  }
  SendPush(Serialize(RTSR_PIPELINE_ELIGIBILITY_MESSAGE{m_Context.load(), id, incarnation, eligible,
                                                       diagnostic},
                     RTSR_PIPELINE_ELIGIBILITY_MESSAGE_TYPE),
           "pipeline eligibility");
}

void RtsrClient::SetPipelineName(RTSR_ID id, const std::string& name) {
  if (!IsConnected()) {
    return;
  }
  SendPush(Serialize(RTSR_SET_PIPELINE_NAME_MESSAGE{m_Context.load(), id, name},
                     RTSR_SET_PIPELINE_NAME_MESSAGE_TYPE),
           "pipeline name");
}

void RtsrClient::PipelineDeleted(RTSR_ID id) {
  if (!IsConnected()) {
    return;
  }
  SendPush(Serialize(RTSR_PIPELINE_DELETED_MESSAGE{m_Context.load(), id},
                     RTSR_PIPELINE_DELETED_MESSAGE_TYPE),
           "pipeline deleted");
}

void RtsrClient::LogMessage(const std::string& message) {
  if (!IsConnected()) {
    return;
  }
  SendPush(Serialize(RTSR_LOG_MESSAGE{m_Context.load(), message}, RTSR_LOG_MESSAGE_TYPE), "log");
}

void RtsrClient::ReportTimeStats(RTSR_REPORT_TIME_STATS_MESSAGE stats) {
  if (!IsConnected()) {
    return;
  }
  stats.Context = m_Context.load();
  SendPush(Serialize(stats, RTSR_REPORT_TIME_STATS_MESSAGE_TYPE), "time stats");
}

} // namespace gits::rtsr
