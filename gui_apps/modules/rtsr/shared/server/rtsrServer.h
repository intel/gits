// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrWireTypes.h"
#include "rtsr/shared/server/rtsrSessionStore.h"
#include "rtsr/shared/transport/rtsrTransport.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Server half of the RTSR protocol: it listens for an instrumented process,
// records what that process harvests into a SessionStore, and reports each
// change to its owner. Graphics API agnostic, and unaware of any GUI.

namespace gits::rtsr {

// Raised while a message is being handled, so every one of these runs on a
// transport receive thread. An implementation must return promptly and must not
// touch GUI state directly: copy what it needs and hand it to the UI thread.
// RTSREditor does the opposite and writes its status bar straight from the
// socket thread.
//
// The store has already been updated by the time a callback runs, so a handler
// may read it, but it is simpler to use the values passed in.
struct ServerCallbacks {
  std::function<void(const SessionInfo&)> SessionOpened;
  std::function<void()> SessionClosed;
  std::function<void(RTSR_ID id, RTSR_PIPELINE_TYPE type, size_t stageCount)> PipelineCreated;
  std::function<void(RTSR_ID id, const std::string& name)> PipelineNamed;
  std::function<void(RTSR_ID id)> PipelineDeleted;
  std::function<void(RTSR_ID id)> PipelineEligibilityChanged;
  std::function<void(RTSR_HASH hash, RTSR_SHADER_STAGE stage)> ShaderCreated;
  // A diagnostic the client chose to forward to us.
  std::function<void(const std::string& message)> ClientLog;
  // One frame's GPU timings. Arrives per frame while measurement is on, so an
  // implementation must be cheap. Not called at all while unassigned, which
  // also skips deserializing the payload.
  std::function<void(RTSR_REPORT_TIME_STATS_MESSAGE stats)> TimeStatsReported;
};

struct ServerRequestResult {
  RTSR_RESULT Result{RTSR_RESULT_FAIL};
  std::string Diagnostic;

  explicit operator bool() const {
    return RtsrSucceeded(Result);
  }
};

class RtsrServer {
public:
  RtsrServer(SessionStore& store, LogSink log);
  ~RtsrServer();

  RtsrServer(const RtsrServer&) = delete;
  RtsrServer& operator=(const RtsrServer&) = delete;

  // Binds the two listening ports and begins accepting. Returns false when
  // either is already taken, which means another RTSR server is running - so
  // the caller can report that rather than appear to work. Returns true without
  // doing anything, endpoints included, when already listening.
  bool Start(const EndpointConfig& endpoints, ServerCallbacks callbacks);

  // Asks a connected client to end its session, then stops listening and
  // discards what was harvested. Safe to call when not started, and more than
  // once. Must run while the process is healthy, because it round-trips a
  // request. Does not raise SessionClosed, because the caller is the one ending
  // the session.
  void Stop();

  bool IsListening() const {
    return m_Listening.load();
  }

  // Asks the client to start or stop timing its pipelines, and reports what it
  // answered rather than what was asked for. Returns false when no session is
  // open, when the client does not answer in time, or when it refuses.
  //
  // Blocks the calling thread for up to a second waiting for that answer. It is
  // driven by a deliberate click, and knowing whether measurement is actually
  // running matters more than the hitch.
  bool EnablePerformanceMeasurements(bool enable);

  // These wait for the client because its response is the only proof that a
  // replacement PSO was created. Call them from a module worker, never the UI
  // thread. Stop() cancels an outstanding wait by closing the request sender.
  ServerRequestResult UpdatePipelines(const RTSR_UPDATE_PIPELINES_REQUEST& request);
  ServerRequestResult ResetPipelines(const RTSR_RESET_PIPELINES_REQUEST& request);

private:
  void RegisterHandlers();
  bool StartRequestSender();
  void StopRequestSender();
  ServerRequestResult SendReplacementRequest(Message message, const char* description);

  // Vulkan probes run off the push thread so queued replacement work cannot
  // prevent push reconnection or be mistaken for a dead client.
  void OnPeerLost();
  void ProbeVulkanPeer(std::stop_token stop);

  // A context handle is a monotonic counter, never a pointer, and is only
  // accepted while it is the current one. RTSR ships the address of its own
  // singleton to the client and reinterpret_casts whatever comes back, so a
  // stale or forged handle is an arbitrary pointer write.
  bool IsCurrentContext(RTSR_CONTEXT context) const;
  Message MakeResponse(RTSR_CONTEXT context, RTSR_RESULT result) const;

  SessionStore& m_Store;
  LogSink m_Log;
  ServerCallbacks m_Callbacks;
  EndpointConfig m_Endpoints;

  std::unique_ptr<IPushReceiver> m_PushReceiver;
  std::unique_ptr<IRequestReceiver> m_RequestReceiver;
  // Moved out under this lock before destruction, so Stop/session close can
  // cancel a request while a module worker is awaiting its future.
  std::mutex m_RequestSenderMutex;
  std::unique_ptr<IRequestSender> m_RequestSender;
  std::atomic<bool> m_ReplacementRequestActive{false};

  // Serializes create/delete/peer-loss/Stop transitions and their callbacks, so
  // an event from one transition cannot be published after a newer session.
  std::mutex m_SessionMutex;
  // Non-zero while one client owns the live session.
  std::atomic<RTSR_CONTEXT> m_Context{0};
  // Context carried by the last message on the currently accepted push socket.
  std::atomic<RTSR_CONTEXT> m_PushContext{0};
  std::atomic<RTSR_CONTEXT> m_NextContext{1};
  std::atomic<bool> m_Listening{false};
  std::atomic<bool> m_ProtocolNegotiated{false};
  std::mutex m_PeerMutex;
  std::condition_variable m_PeerChanged;
  RTSR_CONTEXT m_PendingPeerProbe{};
  std::jthread m_PeerWorker;
};

} // namespace gits::rtsr
