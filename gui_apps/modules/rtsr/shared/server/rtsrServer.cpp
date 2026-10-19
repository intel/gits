// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/shared/server/rtsrServer.h"
#include "rtsr/shared/protocol/rtsrSerialization.h"

#include <chrono>
#include <future>
#include <optional>
#include <sstream>
#include <utility>

namespace gits::rtsr {

namespace {

void Report(const LogSink& log, const std::string& message) {
  if (log) {
    log(message);
  }
}

// A future whose promise died unanswered throws on get(), so no response is ever
// allowed to surface as an exception.
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

// How long the client gets to answer a request. It is only ever asked to flip a
// flag or to stop, never to do work, so this is short on purpose.
constexpr auto REQUEST_TIMEOUT = std::chrono::milliseconds(1000);
constexpr auto REQUEST_WAIT_SLICE = std::chrono::milliseconds(100);
constexpr auto SLOW_REPLACEMENT_REQUEST = std::chrono::seconds(5);

class RequestActivity {
public:
  explicit RequestActivity(std::atomic<bool>& active) : m_Active(active) {}
  ~RequestActivity() {
    m_Active.store(false);
  }

private:
  std::atomic<bool>& m_Active;
};

} // namespace

RtsrServer::RtsrServer(SessionStore& store, LogSink log) : m_Store(store), m_Log(std::move(log)) {}

RtsrServer::~RtsrServer() {
  Stop();
}

bool RtsrServer::StartRequestSender() {
  std::lock_guard<std::mutex> lock(m_RequestSenderMutex);
  if (m_RequestSender) {
    return true;
  }

  auto sender =
      CreateRequestSender(m_Endpoints.Address, m_Endpoints.ClientRequestPort, m_Log, true);
  if (!sender) {
    return false;
  }
  sender->Start();
  m_RequestSender = std::move(sender);
  return true;
}

void RtsrServer::StopRequestSender() {
  std::unique_ptr<IRequestSender> sender;
  {
    std::lock_guard<std::mutex> lock(m_RequestSenderMutex);
    sender = std::move(m_RequestSender);
  }
  if (sender) {
    sender->Stop();
  }
}

bool RtsrServer::IsCurrentContext(RTSR_CONTEXT context) const {
  const auto current = m_Context.load();
  return current != 0 && context == current;
}

Message RtsrServer::MakeResponse(RTSR_CONTEXT context, RTSR_RESULT result) const {
  return Serialize(RTSR_BASIC_CALLBACK_RESPONSE{context, result},
                   RTSR_BASIC_CALLBACK_RESPONSE_MESSAGE_TYPE);
}

void RtsrServer::RegisterHandlers() {
  // **********************************************************************************************
  // * Session lifetime, on the request port                                                      *
  // **********************************************************************************************

  m_RequestReceiver->RegisterCallback(
      RTSR_NEGOTIATE_PROTOCOL_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        const auto request = Deserialize<RTSR_NEGOTIATE_PROTOCOL_REQUEST>(message);
        const bool compatible = request.has_value() && request->Major == RTSR_PROTOCOL_MAJOR &&
                                request->Minor <= RTSR_PROTOCOL_MINOR;
        m_ProtocolNegotiated.store(compatible);
        if (!compatible) {
          Report(m_Log, "Rtsr server: incompatible wire protocol");
        }
        return MakeResponse(0, compatible ? RTSR_RESULT_SUCCESS : RTSR_RESULT_INVALID_ARGUMENT);
      });

  m_RequestReceiver->RegisterCallback(
      RTSR_CREATE_CONTEXT_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        if (!m_ProtocolNegotiated.load()) {
          Report(m_Log, "Rtsr server: create context arrived before protocol negotiation");
          return MakeResponse(0, RTSR_RESULT_FAIL);
        }
        auto request = Deserialize<RTSR_CREATE_CONTEXT_MESSAGE>(message);
        if (!request.has_value() || request->ProtocolMajor != RTSR_PROTOCOL_MAJOR) {
          Report(m_Log, "Rtsr server: malformed create context request");
          return MakeResponse(0, RTSR_RESULT_INVALID_ARGUMENT);
        }

        SessionInfo info;
        info.ApplicationPath = std::move(request->ApplicationPath);
        info.ApplicationName = std::move(request->ApplicationName);
        info.UsedApi = request->UsedApi;

        RTSR_CONTEXT context{};
        {
          std::lock_guard<std::mutex> lock(m_SessionMutex);
          if (!m_Listening.load()) {
            return MakeResponse(0, RTSR_RESULT_FAIL);
          }
          if (m_Context.load() != 0) {
            Report(m_Log, "Rtsr server: refusing " + info.ApplicationName +
                              ", a session is already open. RTSR supports one instrumented process "
                              "at a time");
            return MakeResponse(0, RTSR_RESULT_FAIL);
          }

          // The store is emptied before the handle becomes valid, so a
          // pipeline arriving during the handshake cannot be wiped by the
          // session it belongs to.
          m_Store.OpenSession(info);
          context = m_NextContext.fetch_add(1);
          m_Context.store(context);
          if (!StartRequestSender()) {
            Report(m_Log, "Rtsr server: could not create the outbound connection to " +
                              m_Endpoints.Address);
            m_Context.store(0);
            m_ProtocolNegotiated.store(false);
            m_Store.CloseSession();
            return MakeResponse(0, RTSR_RESULT_FAIL);
          }

          std::future<Message> bindFuture;
          {
            std::lock_guard<std::mutex> senderLock(m_RequestSenderMutex);
            bindFuture = m_RequestSender->Send(Serialize(RTSR_BIND_CONTEXT_REQUEST{context},
                                                         RTSR_BIND_CONTEXT_REQUEST_MESSAGE_TYPE));
          }
          const auto bindReply = AwaitResponse(bindFuture, REQUEST_TIMEOUT);
          const auto bindResponse = bindReply.has_value()
                                        ? Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(*bindReply)
                                        : std::nullopt;
          if (!bindResponse.has_value() || RtsrFailed(bindResponse->Result) ||
              bindResponse->Context != context) {
            Report(m_Log, "Rtsr server: the client callback connection could not be bound");
            m_Context.store(0);
            m_ProtocolNegotiated.store(false);
            StopRequestSender();
            m_Store.CloseSession();
            return MakeResponse(0, RTSR_RESULT_FAIL);
          }

          Report(m_Log, "Rtsr server: session opened for " + info.ApplicationName);
          if (m_Callbacks.SessionOpened) {
            m_Callbacks.SessionOpened(info);
          }
        }
        return MakeResponse(context, RTSR_RESULT_SUCCESS);
      });

  m_RequestReceiver->RegisterCallback(
      RTSR_DELETE_CONTEXT_REQUEST_MESSAGE_TYPE, [this](const Message& message) {
        auto request = Deserialize<RTSR_DELETE_CONTEXT_MESSAGE>(message);
        if (!request.has_value()) {
          Report(m_Log, "Rtsr server: malformed delete context request");
          return MakeResponse(0, RTSR_RESULT_INVALID_ARGUMENT);
        }
        {
          std::lock_guard<std::mutex> lock(m_SessionMutex);
          if (!IsCurrentContext(request->Context)) {
            Report(m_Log, "Rtsr server: delete context for a session that is not open");
            return MakeResponse(request->Context, RTSR_RESULT_INVALID_ARGUMENT);
          }

          // Invalidate first: no request or late push from this client belongs
          // to a live session after this point.
          m_Context.store(0);
          m_ProtocolNegotiated.store(false);
          StopRequestSender();
          m_Store.CloseSession();

          Report(m_Log, "Rtsr server: session closed by the client");
          if (m_Callbacks.SessionClosed) {
            m_Callbacks.SessionClosed();
          }
        }
        return MakeResponse(request->Context, RTSR_RESULT_SUCCESS);
      });

  // **********************************************************************************************
  // * Harvest, on the push port                                                                  *
  // **********************************************************************************************

  // A push carrying a context we no longer recognise is dropped without a word.
  // It is the normal consequence of a client flushing its send queue after the
  // session ended, and one log line per queued message would bury everything
  // else. A request with a stale context is reported, because it is answered.

  m_PushReceiver->RegisterCallback(
      RTSR_PIPELINE_CREATED_MESSAGE_TYPE, [this](const Message& message) {
        auto pipeline = Deserialize<RTSR_PIPELINE_CREATED_MESSAGE>(message);
        if (!pipeline.has_value()) {
          Report(m_Log, "Rtsr server: malformed pipeline created message");
          return;
        }
        m_PushContext.store(pipeline->Context);
        std::lock_guard<std::mutex> lock(m_SessionMutex);
        if (!IsCurrentContext(pipeline->Context)) {
          return;
        }

        // Read before the message is moved into the store.
        const auto id = pipeline->Id;
        const auto type = pipeline->Type;
        const auto stageCount = pipeline->ShaderStages.size();

        const auto newShaders = m_Store.AddPipeline(std::move(*pipeline));

        if (m_Callbacks.PipelineCreated) {
          m_Callbacks.PipelineCreated(id, type, stageCount);
        }
        if (m_Callbacks.ShaderCreated) {
          for (const auto& stage : newShaders) {
            m_Callbacks.ShaderCreated(stage.BytecodeHash, stage.Stage);
          }
        }
      });

  m_PushReceiver->RegisterCallback(
      RTSR_PIPELINE_ELIGIBILITY_MESSAGE_TYPE, [this](const Message& message) {
        const auto update = Deserialize<RTSR_PIPELINE_ELIGIBILITY_MESSAGE>(message);
        if (!update) {
          Report(m_Log, "Rtsr server: malformed pipeline eligibility message");
          return;
        }
        std::lock_guard<std::mutex> lock(m_SessionMutex);
        if (!IsCurrentContext(update->Context)) {
          return;
        }
        m_PushContext.store(update->Context);
        if (m_Store.SetPipelineEligibility(*update) && m_Callbacks.PipelineEligibilityChanged) {
          m_Callbacks.PipelineEligibilityChanged(update->PipelineId);
        }
      });

  m_PushReceiver->RegisterCallback(
      RTSR_SET_PIPELINE_NAME_MESSAGE_TYPE, [this](const Message& message) {
        auto request = Deserialize<RTSR_SET_PIPELINE_NAME_MESSAGE>(message);
        if (!request.has_value()) {
          Report(m_Log, "Rtsr server: malformed set pipeline name message");
          return;
        }
        m_PushContext.store(request->Context);
        std::lock_guard<std::mutex> lock(m_SessionMutex);
        if (!IsCurrentContext(request->Context)) {
          return;
        }
        // A name for a pipeline we do not have means it was already released.
        if (m_Store.SetPipelineName(request->PipelineId, request->Name) &&
            m_Callbacks.PipelineNamed) {
          m_Callbacks.PipelineNamed(request->PipelineId, request->Name);
        }
      });

  m_PushReceiver->RegisterCallback(
      RTSR_PIPELINE_DELETED_MESSAGE_TYPE, [this](const Message& message) {
        auto request = Deserialize<RTSR_PIPELINE_DELETED_MESSAGE>(message);
        if (!request.has_value()) {
          Report(m_Log, "Rtsr server: malformed pipeline deleted message");
          return;
        }
        m_PushContext.store(request->Context);
        std::lock_guard<std::mutex> lock(m_SessionMutex);
        if (!IsCurrentContext(request->Context)) {
          return;
        }
        if (m_Store.RemovePipeline(request->PipelineId) && m_Callbacks.PipelineDeleted) {
          m_Callbacks.PipelineDeleted(request->PipelineId);
        }
      });

  m_PushReceiver->RegisterCallback(RTSR_LOG_MESSAGE_TYPE, [this](const Message& message) {
    auto request = Deserialize<RTSR_LOG_MESSAGE>(message);
    if (!request.has_value()) {
      return;
    }
    m_PushContext.store(request->Context);
    std::lock_guard<std::mutex> lock(m_SessionMutex);
    if (!IsCurrentContext(request->Context)) {
      return;
    }
    if (m_Callbacks.ClientLog) {
      m_Callbacks.ClientLog(request->Message);
    }
  });

  m_PushReceiver->RegisterCallback(
      RTSR_REPORT_TIME_STATS_MESSAGE_TYPE, [this](const Message& message) {
        auto stats = Deserialize<RTSR_REPORT_TIME_STATS_MESSAGE>(message);
        if (!stats.has_value()) {
          Report(m_Log, "Rtsr server: malformed time stats message");
          return;
        }
        m_PushContext.store(stats->Context);
        // Avoid locking and publishing when the module has no performance
        // consumer, while still binding this socket to its session context.
        if (!m_Callbacks.TimeStatsReported) {
          return;
        }
        std::lock_guard<std::mutex> lock(m_SessionMutex);
        if (!IsCurrentContext(stats->Context)) {
          return;
        }
        m_Callbacks.TimeStatsReported(std::move(*stats));
      });

  m_PushReceiver->SetPeerLostCallback([this]() { OnPeerLost(); });
}

void RtsrServer::OnPeerLost() {
  const RTSR_CONTEXT lostContext = m_PushContext.exchange(0);
  const RTSR_CONTEXT context = m_Context.load();
  if (context == 0 || (lostContext != 0 && lostContext != context)) {
    return;
  }

  const auto api = m_Store.Info().UsedApi;
  if (api >= RTSR_API_VULKAN_1_0 && api <= RTSR_API_VULKAN_1_3) {
    {
      std::lock_guard<std::mutex> lock(m_PeerMutex);
      m_PendingPeerProbe = context;
    }
    m_PeerChanged.notify_one();
    return;
  }

  std::future<Message> aliveFuture;
  {
    std::lock_guard<std::mutex> senderLock(m_RequestSenderMutex);
    if (m_RequestSender && m_Context.load() == context) {
      aliveFuture = m_RequestSender->Send(
          Serialize(RTSR_EMPTY_REQUEST{}, RTSR_CHECK_IF_ALIVE_REQUEST_MESSAGE_TYPE));
    }
  }
  if (aliveFuture.valid()) {
    const auto reply = AwaitResponse(aliveFuture, REQUEST_TIMEOUT);
    const auto response =
        reply.has_value() ? Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(*reply) : std::nullopt;
    if (response.has_value() && !RtsrFailed(response->Result) && response->Context == context) {
      Report(m_Log, "Rtsr server: push connection dropped; waiting for its reconnect");
      return;
    }
  }

  std::lock_guard<std::mutex> lock(m_SessionMutex);
  if (m_Context.load() != context) {
    return;
  }
  m_Context.store(0);
  m_ProtocolNegotiated.store(false);
  StopRequestSender();
  m_Store.CloseSession();

  Report(m_Log, "Rtsr server: the client disappeared without ending its session");
  if (m_Callbacks.SessionClosed) {
    m_Callbacks.SessionClosed();
  }
}

void RtsrServer::ProbeVulkanPeer(std::stop_token stop) {
  while (!stop.stop_requested()) {
    RTSR_CONTEXT context{};
    {
      std::unique_lock<std::mutex> lock(m_PeerMutex);
      m_PeerChanged.wait(lock, [&] { return stop.stop_requested() || m_PendingPeerProbe != 0; });
      if (stop.stop_requested()) {
        return;
      }
      context = std::exchange(m_PendingPeerProbe, 0);
    }
    std::future<Message> future;
    {
      std::lock_guard<std::mutex> lock(m_RequestSenderMutex);
      if (m_RequestSender && IsCurrentContext(context) && m_PushContext.load() != context) {
        future = m_RequestSender->Send(
            Serialize(RTSR_EMPTY_REQUEST{}, RTSR_CHECK_IF_ALIVE_REQUEST_MESSAGE_TYPE));
      }
    }
    if (!future.valid()) {
      continue;
    }
    while (!stop.stop_requested() && IsCurrentContext(context) && m_PushContext.load() != context &&
           future.wait_for(REQUEST_WAIT_SLICE) != std::future_status::ready) {
    }
    if (stop.stop_requested() || !IsCurrentContext(context) || m_PushContext.load() == context) {
      continue;
    }
    const auto reply = AwaitResponse(future, std::chrono::milliseconds(0));
    const auto response = reply ? Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(*reply) : std::nullopt;
    if (response && RtsrSucceeded(response->Result) && response->Context == context) {
      Report(m_Log, "Rtsr server: push connection dropped; waiting for its reconnect");
      continue;
    }
    std::lock_guard<std::mutex> lock(m_SessionMutex);
    if (!IsCurrentContext(context) || m_PushContext.load() == context) {
      continue;
    }
    m_Context.store(0);
    m_ProtocolNegotiated.store(false);
    StopRequestSender();
    m_Store.CloseSession();
    Report(m_Log, "Rtsr server: the Vulkan client connection was lost");
    if (m_Callbacks.SessionClosed) {
      m_Callbacks.SessionClosed();
    }
  }
}

bool RtsrServer::Start(const EndpointConfig& endpoints, ServerCallbacks callbacks) {
  if (m_Listening.load()) {
    return true;
  }

  m_Endpoints = endpoints;
  m_Callbacks = std::move(callbacks);
  m_ProtocolNegotiated.store(false);
  m_PushContext.store(0);

  // Callbacks, then the sockets, then the handlers, then the threads. A receiver
  // that is already accepting before its handlers exist can be handed a message
  // it has nowhere to put.
  m_PushReceiver = CreatePushReceiver(m_Endpoints.ServerPushPort, m_Log);
  m_RequestReceiver = CreateRequestReceiver(m_Endpoints.ServerRequestPort, m_Log);
  if (!m_PushReceiver || !m_RequestReceiver) {
    std::ostringstream oss;
    oss << "Rtsr server: could not listen on ports " << m_Endpoints.ServerPushPort << " and "
        << m_Endpoints.ServerRequestPort << ", is another RTSR server running?";
    Report(m_Log, oss.str());
    m_PushReceiver.reset();
    m_RequestReceiver.reset();
    m_Callbacks = {};
    return false;
  }

  RegisterHandlers();

  // Set before the receiver threads start: CREATE_CONTEXT rejects work while
  // Stop is in progress, and must not mistake the first request for that state.
  m_Listening.store(true);
  m_PeerWorker = std::jthread([this](std::stop_token stop) { ProbeVulkanPeer(stop); });
  m_PushReceiver->Start();
  m_RequestReceiver->Start();

  std::ostringstream oss;
  oss << "Rtsr server: listening on " << m_Endpoints.Address << " ports "
      << m_Endpoints.ServerPushPort << " and " << m_Endpoints.ServerRequestPort;
  Report(m_Log, oss.str());
  return true;
}

ServerRequestResult RtsrServer::SendReplacementRequest(Message message, const char* description) {
  if (message.Header.Len <= 0) {
    return {RTSR_RESULT_FAIL, std::string("Could not serialize ") + description + "."};
  }
  if (m_ReplacementRequestActive.exchange(true)) {
    return {RTSR_RESULT_FAIL, "Another pipeline operation is already in progress."};
  }
  const RequestActivity activity(m_ReplacementRequestActive);

  const RTSR_CONTEXT context = m_Context.load();
  std::future<Message> future;
  {
    std::lock_guard<std::mutex> lock(m_RequestSenderMutex);
    if (context == 0 || m_Context.load() != context || !m_RequestSender) {
      return {RTSR_RESULT_FAIL, "No RTSR client session is open."};
    }
    future = m_RequestSender->Send(std::move(message));
  }

  const auto started = std::chrono::steady_clock::now();
  bool slowReported = false;
  try {
    while (future.wait_for(REQUEST_WAIT_SLICE) != std::future_status::ready) {
      if (!slowReported && std::chrono::steady_clock::now() - started >= SLOW_REPLACEMENT_REQUEST) {
        slowReported = true;
        Report(m_Log, std::string("Rtsr server: still waiting for the client to ") + description);
      }
    }

    const Message reply = future.get();
    auto response = Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(reply);
    if (!response.has_value()) {
      return {RTSR_RESULT_FAIL,
              std::string("The client did not return a valid response for ") + description + "."};
    }
    if (response->Context != context || !IsCurrentContext(context)) {
      return {RTSR_RESULT_FAIL,
              std::string("The client session changed while trying to ") + description + "."};
    }
    if (RtsrFailed(response->Result)) {
      return {response->Result, std::string("The client failed to ") + description + "."};
    }
    return {response->Result, {}};
  } catch (...) {
    return {RTSR_RESULT_FAIL,
            std::string("The client connection was lost while trying to ") + description + "."};
  }
}

bool RtsrServer::EnablePerformanceMeasurements(bool enable) {
  RTSR_CONTEXT context{};
  std::future<Message> future;
  {
    std::lock_guard<std::mutex> lock(m_RequestSenderMutex);
    context = m_Context.load();
    if (m_ReplacementRequestActive.load() || !m_RequestSender || !IsCurrentContext(context)) {
      return false;
    }
    future =
        m_RequestSender->Send(Serialize(RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST{enable},
                                        RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST_MESSAGE_TYPE));
  }

  auto reply = AwaitResponse(future, REQUEST_TIMEOUT);
  if (!reply.has_value()) {
    Report(m_Log, "Rtsr server: the client did not answer the measurement request");
    return false;
  }

  auto response = Deserialize<RTSR_BASIC_CALLBACK_RESPONSE>(*reply);
  if (!response.has_value() || response->Context != context || !IsCurrentContext(context) ||
      RtsrFailed(response->Result)) {
    Report(m_Log, enable ? "Rtsr server: the client refused to start measuring"
                         : "Rtsr server: the client refused to stop measuring");
    return false;
  }
  return true;
}

ServerRequestResult RtsrServer::UpdatePipelines(const RTSR_UPDATE_PIPELINES_REQUEST& request) {
  if (request.UpdatedPipelines.size() != 1 || request.ShaderStages.empty()) {
    return {RTSR_RESULT_INVALID_ARGUMENT,
            "A pipeline update requires exactly one pipeline and at least one shader."};
  }
  return SendReplacementRequest(Serialize(request, RTSR_UPDATE_PIPELINES_REQUEST_MESSAGE_TYPE),
                                "update pipelines");
}

ServerRequestResult RtsrServer::ResetPipelines(const RTSR_RESET_PIPELINES_REQUEST& request) {
  if (request.Pipelines.size() != 1) {
    return {RTSR_RESULT_INVALID_ARGUMENT, "A pipeline reset requires exactly one pipeline."};
  }
  return SendReplacementRequest(Serialize(request, RTSR_RESET_PIPELINES_REQUEST_MESSAGE_TYPE),
                                "reset pipelines");
}

void RtsrServer::Stop() {
  if (!m_Listening.exchange(false)) {
    return;
  }
  m_ProtocolNegotiated.store(false);
  m_PushContext.store(0);

  bool hadSession = false;
  {
    std::lock_guard<std::mutex> lock(m_SessionMutex);
    hadSession = m_Context.exchange(0) != 0;
  }
  std::future<Message> closeFuture;
  {
    std::lock_guard<std::mutex> lock(m_RequestSenderMutex);
    if (hadSession && m_RequestSender) {
      Report(m_Log, "Rtsr server: asking the client to end its session");
      closeFuture = m_RequestSender->Send(
          Serialize(RTSR_EMPTY_REQUEST{}, RTSR_CLOSE_CONNECTION_REQUEST_MESSAGE_TYPE));
    }
  }
  if (closeFuture.valid()) {
    auto future = std::move(closeFuture);
    AwaitResponse(future, REQUEST_TIMEOUT);
  }

  // Tearing the receivers down joins their threads, so no callback can be
  // running by the time the store is cleared and the callbacks are dropped.
  if (m_RequestReceiver) {
    m_RequestReceiver->Stop();
  }
  // No create-context callback can install a new sender after the receiver has
  // joined, so moving it out here closes every current and pending request.
  StopRequestSender();
  m_RequestReceiver.reset();
  m_PushReceiver.reset();
  m_PeerWorker.request_stop();
  m_PeerChanged.notify_all();
  if (m_PeerWorker.joinable()) {
    m_PeerWorker.join();
  }
  {
    std::lock_guard<std::mutex> lock(m_PeerMutex);
    m_PendingPeerProbe = 0;
  }

  // Stopping discards what was harvested, unlike a run ending. SessionClosed is
  // deliberately not raised: whoever called Stop is tearing the module down and
  // has already dealt with its own state, so the notification would only arrive
  // after the fact and undo that.
  m_Store.Clear();

  m_Callbacks = {};
  Report(m_Log, "Rtsr server: stopped listening");
}

} // namespace gits::rtsr
