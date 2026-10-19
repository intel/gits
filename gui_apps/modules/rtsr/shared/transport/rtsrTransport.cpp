// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/shared/transport/rtsrTransport.h"
#include "rtsr/shared/protocol/rtsrSerialization.h"

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

namespace gits::rtsr {

namespace {

using namespace std::chrono_literals;

// A saturated queue means the peer stopped consuming. GITS replay creates
// pipelines far faster than a stalled socket drains, and each pipeline message
// carries every shader blob of one PSO, so an unbounded queue is an
// out-of-memory waiting to happen.
constexpr size_t MAX_QUEUED_PUSH_MESSAGES = 1024;
constexpr size_t MAX_QUEUED_REQUESTS = 64;

// RTSR retries a refused connect with no delay at all, which pegs a core for as
// long as the peer is absent.
constexpr auto MIN_RECONNECT_DELAY = 50ms;
constexpr auto MAX_RECONNECT_DELAY = 2000ms;

// How long a worker blocks before re-checking its running flag.
constexpr auto QUEUE_WAIT_SLICE = 100ms;

void Report(const LogSink& log, const std::string& message) {
  if (log) {
    log(message);
  }
}

asio::ip::tcp::endpoint MakeListenEndpoint(uint16_t port) {
  // RTSR listens on every interface. Every connection in this protocol is
  // loopback, so binding wider only exposes the port needlessly.
  return asio::ip::tcp::endpoint{asio::ip::make_address(DEFAULT_ADDRESS), port};
}

bool WriteMessage(asio::ip::tcp::socket& socket, const Message& message, asio::error_code& ec) {
  asio::write(socket, asio::buffer(&message.Header, sizeof(message.Header)), ec);
  if (ec) {
    return false;
  }
  if (message.Header.Len > 0) {
    asio::write(socket, asio::buffer(message.Data.data(), message.Data.size()), ec);
    if (ec) {
      return false;
    }
  }
  return true;
}

// Returns false when the connection should be dropped, which covers both a
// socket error and an implausible declared length.
bool ReadMessage(asio::ip::tcp::socket& socket,
                 Message& message,
                 asio::error_code& ec,
                 const LogSink& log) {
  asio::read(socket, asio::buffer(&message.Header, sizeof(message.Header)), ec);
  if (ec) {
    return false;
  }

  if (!IsPlausibleLength(message.Header.Len)) {
    std::ostringstream oss;
    oss << "Rtsr transport: rejecting message of declared length " << message.Header.Len
        << ", dropping the connection";
    Report(log, oss.str());
    return false;
  }

  if (message.Header.Len == 0) {
    message.Data.clear();
    return true;
  }

  try {
    message.Data.resize(static_cast<size_t>(message.Header.Len));
  } catch (const std::bad_alloc&) {
    Report(log, "Rtsr transport: allocation for an incoming message failed");
    return false;
  }

  asio::read(socket, asio::buffer(message.Data.data(), message.Data.size()), ec);
  return !ec;
}

// Guarantees a promise is answered exactly once, whichever way its scope exits.
class PromiseGuard {
public:
  explicit PromiseGuard(std::promise<Message>& promise) : m_Promise(promise) {}

  ~PromiseGuard() {
    if (!m_Fulfilled) {
      m_Promise.set_value(Message{});
    }
  }

  PromiseGuard(const PromiseGuard&) = delete;
  PromiseGuard& operator=(const PromiseGuard&) = delete;

  void Fulfill(Message message) {
    m_Promise.set_value(std::move(message));
    m_Fulfilled = true;
  }

private:
  std::promise<Message>& m_Promise;
  bool m_Fulfilled{false};
};

// Sleeps in slices so shutdown stays responsive while a peer is absent.
void BackOff(std::chrono::milliseconds& delay, const std::atomic<bool>& running) {
  const auto deadline = std::chrono::steady_clock::now() + delay;
  while (running.load() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::min<std::chrono::milliseconds>(delay, QUEUE_WAIT_SLICE));
  }
  delay = std::min(delay * 2, std::chrono::milliseconds(MAX_RECONNECT_DELAY));
}

// **********************************************************************************************
// * Push sender                                                                                *
// **********************************************************************************************

class PushSender final : public IPushSender {
public:
  PushSender(const std::string& address, uint16_t port, LogSink log)
      : m_Log(std::move(log)),
        m_Endpoint(asio::ip::make_address(address), port),
        m_Socket(m_Context) {}

  ~PushSender() override {
    Shutdown();
  }

  void Start() override {
    if (m_Running.exchange(true)) {
      return;
    }
    m_Thread = std::thread([this]() { Run(); });
  }

  bool Send(Message message) override {
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      if (m_Queue.size() >= MAX_QUEUED_PUSH_MESSAGES) {
        ++m_Dropped;
        if (m_Dropped == 1 || (m_Dropped % 1000) == 0) {
          std::ostringstream oss;
          oss << "Rtsr transport: push queue saturated, " << m_Dropped << " message(s) dropped";
          Report(m_Log, oss.str());
        }
        return false;
      }
      m_Queue.push_back(std::move(message));
    }
    m_Signal.notify_one();
    return true;
  }

private:
  void Shutdown() {
    m_Running.store(false);
    m_Signal.notify_all();
    asio::error_code ec;
    m_Socket.close(ec);
    if (m_Thread.joinable()) {
      m_Thread.join();
    }
  }

  bool NextMessage(Message& message) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Signal.wait_for(lock, QUEUE_WAIT_SLICE,
                      [this]() { return !m_Queue.empty() || !m_Running.load(); });
    if (m_Queue.empty()) {
      return false;
    }
    message = std::move(m_Queue.front());
    m_Queue.pop_front();
    return true;
  }

  void Run() {
    auto delay = MIN_RECONNECT_DELAY;

    while (m_Running.load()) {
      asio::error_code ec;
      m_Socket.close(ec);
      m_Socket.connect(m_Endpoint, ec);
      if (ec) {
        BackOff(delay, m_Running);
        continue;
      }
      delay = MIN_RECONNECT_DELAY;

      while (m_Running.load()) {
        Message message;
        if (!NextMessage(message)) {
          continue;
        }
        if (!WriteMessage(m_Socket, message, ec)) {
          break;
        }
      }
    }
  }

  LogSink m_Log;
  asio::io_context m_Context;
  asio::ip::tcp::endpoint m_Endpoint;
  asio::ip::tcp::socket m_Socket;

  std::atomic<bool> m_Running{false};
  std::thread m_Thread;

  std::mutex m_Mutex;
  std::condition_variable m_Signal;
  std::deque<Message> m_Queue;
  size_t m_Dropped{0};
};

// **********************************************************************************************
// * Push receiver                                                                              *
// **********************************************************************************************

class PushReceiver final : public IPushReceiver {
public:
  PushReceiver(uint16_t port, LogSink log)
      : m_Log(std::move(log)),
        m_Acceptor(m_Context, MakeListenEndpoint(port)),
        m_Socket(m_Context) {}

  ~PushReceiver() override {
    m_Running.store(false);
    // Same reasoning as RequestReceiver::Terminate: the worker is blocked in
    // accept() or in a read, so the acceptor and the socket both have to go
    // before the join can succeed.
    asio::error_code ec;
    m_Acceptor.close(ec);
    m_Socket.shutdown(asio::socket_base::shutdown_both, ec);
    m_Socket.close(ec);
    if (m_Thread.joinable()) {
      m_Thread.join();
    }
  }

  void RegisterCallback(RTSR_MESSAGE_TYPE type, MessageCallback callback) override {
    m_Handler.RegisterCallback(type, std::move(callback));
  }

  void SetPeerLostCallback(std::function<void()> callback) override {
    m_PeerLost = std::move(callback);
  }

  void Start() override {
    if (m_Running.exchange(true)) {
      return;
    }
    m_Thread = std::thread([this]() { Run(); });
  }

private:
  void Run() {
    while (m_Running.load()) {
      asio::error_code ec;
      m_Socket.close(ec);
      m_Acceptor.accept(m_Socket, ec);
      if (ec) {
        continue;
      }

      while (m_Running.load()) {
        Message message;
        if (!ReadMessage(m_Socket, message, ec, m_Log)) {
          break;
        }
        // A handler that throws must not take the process down with it.
        try {
          if (!m_Handler.HandleMessage(message)) {
            std::ostringstream oss;
            oss << "Rtsr transport: ignoring unhandled push message type " << message.Header.Type;
            Report(m_Log, oss.str());
          }
        } catch (const std::exception& e) {
          Report(m_Log, std::string("Rtsr transport: push handler threw: ") + e.what());
        } catch (...) {
          Report(m_Log, "Rtsr transport: push handler threw an unknown exception");
        }
      }

      // The peer is gone, either because it closed the socket or because the
      // process holding it died. Silent during our own shutdown, where the
      // socket was closed from this side and nothing is left to tell.
      if (m_Running.load() && m_PeerLost) {
        try {
          m_PeerLost();
        } catch (const std::exception& e) {
          Report(m_Log, std::string("Rtsr transport: peer lost handler threw: ") + e.what());
        } catch (...) {
          Report(m_Log, "Rtsr transport: peer lost handler threw an unknown exception");
        }
      }
    }
  }

  LogSink m_Log;
  asio::io_context m_Context;
  asio::ip::tcp::acceptor m_Acceptor;
  asio::ip::tcp::socket m_Socket;
  MessageHandler m_Handler;
  std::function<void()> m_PeerLost;

  std::atomic<bool> m_Running{false};
  std::thread m_Thread;
};

// **********************************************************************************************
// * Request sender                                                                             *
// **********************************************************************************************

class RequestSender final : public IRequestSender {
public:
  RequestSender(const std::string& address, uint16_t port, LogSink log, bool queueBeforeConnect)
      : m_Log(std::move(log)),
        m_Endpoint(asio::ip::make_address(address), port),
        m_Socket(m_Context),
        m_QueueBeforeConnect(queueBeforeConnect) {}

  ~RequestSender() override {
    Stop();
  }

  void Stop() override {
    m_Running.store(false);
    m_Connected.store(false);
    m_Signal.notify_all();
    asio::error_code ec;
    m_Socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    m_Socket.close(ec);
    if (m_Thread.joinable()) {
      m_Thread.join();
    }
    FailQueuedRequests();
  }

  void Start() override {
    if (m_Running.exchange(true)) {
      return;
    }
    m_Connected.store(false);
    m_Thread = std::thread([this]() { Run(); });
  }

  std::future<Message> Send(Message message) override {
    std::promise<Message> promise;
    auto future = promise.get_future();

    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      if (!m_Running.load() || (!m_Connected.load() && !m_QueueBeforeConnect)) {
        promise.set_value(Message{});
        return future;
      }
      if (m_Queue.size() >= MAX_QUEUED_REQUESTS) {
        Report(m_Log, "Rtsr transport: request queue saturated, failing the request immediately");
        // An empty message deserializes to nullopt, which callers treat as a
        // failed request. Better than a future that never completes.
        promise.set_value(Message{});
        return future;
      }
      m_Queue.emplace_back(std::move(promise), std::move(message));
    }
    m_Signal.notify_one();
    return future;
  }

private:
  void FailQueuedRequests() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    // Nothing will ever answer these now, so release their waiters.
    while (!m_Queue.empty()) {
      m_Queue.front().first.set_value(Message{});
      m_Queue.pop_front();
    }
  }

  bool NextRequest(std::promise<Message>& promise, Message& request) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Signal.wait_for(lock, QUEUE_WAIT_SLICE,
                      [this]() { return !m_Queue.empty() || !m_Running.load(); });
    if (m_Queue.empty()) {
      return false;
    }
    promise = std::move(m_Queue.front().first);
    request = std::move(m_Queue.front().second);
    m_Queue.pop_front();
    return true;
  }

  void Run() {
    auto delay = MIN_RECONNECT_DELAY;
    bool connected = false;

    while (m_Running.load()) {
      asio::error_code ec;
      m_Socket.close(ec);
      m_Socket.connect(m_Endpoint, ec);
      if (ec) {
        BackOff(delay, m_Running);
        continue;
      }
      connected = true;
      m_Connected.store(true);
      delay = MIN_RECONNECT_DELAY;

      while (m_Running.load()) {
        std::promise<Message> promise;
        Message request;
        if (!NextRequest(promise, request)) {
          continue;
        }

        // A promise dropped without a value makes the waiter's get() throw
        // broken_promise, so every exit path has to answer it. An empty message
        // deserializes to nullopt, which callers already treat as failure.
        PromiseGuard guard{promise};

        // Requests are strictly serialised: one outstanding at a time, matched
        // by arrival order because the protocol has no correlation id.
        if (!WriteMessage(m_Socket, request, ec)) {
          break;
        }

        Message response;
        if (!ReadMessage(m_Socket, response, ec, m_Log)) {
          break;
        }
        guard.Fulfill(std::move(response));
      }

      // Retry until the first connection because the client listener may still
      // be starting. Once a session connection breaks, never carry queued
      // context-free requests into whichever process listens on this port next.
      if (connected && m_Running.exchange(false)) {
        m_Connected.store(false);
        Report(m_Log, "Rtsr transport: request peer disconnected");
      }
    }
    FailQueuedRequests();
  }

  LogSink m_Log;
  asio::io_context m_Context;
  asio::ip::tcp::endpoint m_Endpoint;
  asio::ip::tcp::socket m_Socket;
  const bool m_QueueBeforeConnect;

  std::atomic<bool> m_Running{false};
  std::atomic<bool> m_Connected{false};
  std::thread m_Thread;

  std::mutex m_Mutex;
  std::condition_variable m_Signal;
  std::deque<std::pair<std::promise<Message>, Message>> m_Queue;
};

// **********************************************************************************************
// * Request receiver                                                                           *
// **********************************************************************************************

class RequestReceiver final : public IRequestReceiver {
public:
  RequestReceiver(uint16_t port, LogSink log)
      : m_Log(std::move(log)),
        m_Acceptor(m_Context, MakeListenEndpoint(port)),
        m_Socket(m_Context) {}

  ~RequestReceiver() override {
    Terminate();
  }

  void RegisterCallback(RTSR_MESSAGE_TYPE type, RequestCallback callback) override {
    m_Handler.RegisterCallback(type, std::move(callback));
  }

  void Start() override {
    if (m_Running.exchange(true)) {
      return;
    }
    m_Thread = std::thread([this]() { Run(); });
  }

  void Stop() override {
    Terminate();
  }

private:
  // The worker spends its life blocked, either in accept() waiting for the
  // server or in a read on an established connection, and a blocking socket call
  // cannot observe the running flag. Closing the acceptor releases accept(); a
  // blocked read is only released by tearing the socket down underneath it.
  //
  // RTSR also has a variant that leaves the socket open so a response in flight
  // is delivered before the join. That is only sound when the stop comes from
  // the worker's own request handler, which is the only place RTSR calls it.
  // Called from any other thread while the server holds this connection open and
  // idle - which is the normal state - it joins a thread parked in a read that
  // will never return. Idempotent, so the destructor may repeat it after Stop().
  void Terminate() {
    m_Running.store(false);

    asio::error_code ec;
    m_Acceptor.close(ec);
    // shutdown before close: it releases a blocked read while still letting a
    // write that is already under way complete.
    m_Socket.shutdown(asio::socket_base::shutdown_both, ec);
    m_Socket.close(ec);

    if (m_Thread.joinable()) {
      m_Thread.join();
    }
  }

  void Run() {
    while (m_Running.load()) {
      asio::error_code ec;
      m_Socket.close(ec);
      m_Acceptor.accept(m_Socket, ec);
      if (ec) {
        continue;
      }

      while (m_Running.load()) {
        Message request;
        if (!ReadMessage(m_Socket, request, ec, m_Log)) {
          break;
        }

        // The peer is blocked on a response, so every request must be answered
        // even when it is unknown or the handler fails. RTSR throws instead,
        // which kills this process and hangs the peer.
        Message response;
        try {
          auto handled = m_Handler.HandleRequest(request);
          if (handled.has_value()) {
            response = std::move(*handled);
          } else {
            std::ostringstream oss;
            oss << "Rtsr transport: unhandled request type " << request.Header.Type;
            Report(m_Log, oss.str());
            response = Serialize(RTSR_BASIC_CALLBACK_RESPONSE{0, RTSR_RESULT_FAIL},
                                 RTSR_BASIC_CALLBACK_RESPONSE_MESSAGE_TYPE);
          }
        } catch (const std::exception& e) {
          Report(m_Log, std::string("Rtsr transport: request handler threw: ") + e.what());
          response = Serialize(RTSR_BASIC_CALLBACK_RESPONSE{0, RTSR_RESULT_FAIL},
                               RTSR_BASIC_CALLBACK_RESPONSE_MESSAGE_TYPE);
        } catch (...) {
          Report(m_Log, "Rtsr transport: request handler threw an unknown exception");
          response = Serialize(RTSR_BASIC_CALLBACK_RESPONSE{0, RTSR_RESULT_FAIL},
                               RTSR_BASIC_CALLBACK_RESPONSE_MESSAGE_TYPE);
        }

        if (!WriteMessage(m_Socket, response, ec)) {
          break;
        }
      }
    }
  }

  LogSink m_Log;
  asio::io_context m_Context;
  asio::ip::tcp::acceptor m_Acceptor;
  asio::ip::tcp::socket m_Socket;
  RequestHandler m_Handler;

  std::atomic<bool> m_Running{false};
  std::thread m_Thread;
};

} // namespace

std::unique_ptr<IPushSender> CreatePushSender(const std::string& address,
                                              uint16_t port,
                                              LogSink log) {
  try {
    return std::make_unique<PushSender>(address, port, std::move(log));
  } catch (const std::exception&) {
    return nullptr;
  }
}

std::unique_ptr<IPushReceiver> CreatePushReceiver(uint16_t port, LogSink log) {
  try {
    return std::make_unique<PushReceiver>(port, std::move(log));
  } catch (const std::exception&) {
    return nullptr;
  }
}

std::unique_ptr<IRequestSender> CreateRequestSender(const std::string& address,
                                                    uint16_t port,
                                                    LogSink log,
                                                    bool queueBeforeConnect) {
  try {
    return std::make_unique<RequestSender>(address, port, std::move(log), queueBeforeConnect);
  } catch (const std::exception&) {
    return nullptr;
  }
}

// Binding the acceptor throws when the port is already taken, which happens
// whenever a second instrumented process starts. The factory reports that as a
// null result rather than letting asio's exception escape into the caller.
std::unique_ptr<IRequestReceiver> CreateRequestReceiver(uint16_t port, LogSink log) {
  try {
    return std::make_unique<RequestReceiver>(port, std::move(log));
  } catch (const std::exception&) {
    return nullptr;
  }
}

} // namespace gits::rtsr
