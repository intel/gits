// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrMessage.h"

#include <cstdint>
#include <future>
#include <memory>
#include <string>

// Blocking TCP transport for the RTSR protocol. Asio is confined to the
// implementation file so consumers do not pay for its headers.
//
// Three connections make up a session, which is why there are three ports. The
// client connects out on the push and server-request ports; the server connects
// in on the client-request port, which is how shader replacements reach a
// running client.

namespace gits::rtsr {

// RTSR hardcodes these, which limits a machine to one instrumented process.
// They are defaults here so several players can run side by side.
constexpr uint16_t DEFAULT_SERVER_PUSH_PORT = 9551;
constexpr uint16_t DEFAULT_SERVER_REQUEST_PORT = 9552;
constexpr uint16_t DEFAULT_CLIENT_REQUEST_PORT = 9553;
constexpr const char* DEFAULT_ADDRESS = "127.0.0.1";

struct EndpointConfig {
  std::string Address{DEFAULT_ADDRESS};
  uint16_t ServerPushPort{DEFAULT_SERVER_PUSH_PORT};
  uint16_t ServerRequestPort{DEFAULT_SERVER_REQUEST_PORT};
  uint16_t ClientRequestPort{DEFAULT_CLIENT_REQUEST_PORT};
};

class IPushSender {
public:
  virtual ~IPushSender() = default;
  virtual void Start() = 0;
  // Never blocks on the network. Returns false when the queue is saturated,
  // which means the peer is gone and the message was dropped.
  virtual bool Send(Message message) = 0;
};

class IPushReceiver {
public:
  virtual ~IPushReceiver() = default;
  virtual void RegisterCallback(RTSR_MESSAGE_TYPE type, MessageCallback callback) = 0;
  // Called on the receive thread when an established connection drops, which is
  // the only way to notice a peer that died without saying goodbye. Set it
  // before Start(), like the message callbacks. Not called while shutting down,
  // because by then the socket is being closed from this side.
  virtual void SetPeerLostCallback(std::function<void()> callback) = 0;
  virtual void Start() = 0;
};

class IRequestSender {
public:
  virtual ~IRequestSender() = default;
  virtual void Start() = 0;
  // Closes the socket, joins the worker and fails every pending request. Safe
  // to call more than once.
  virtual void Stop() = 0;
  // The future carries the peer's response. A dropped/stopped connection
  // fulfills it with an empty message, which callers treat as failure.
  virtual std::future<Message> Send(Message message) = 0;
};

class IRequestReceiver {
public:
  virtual ~IRequestReceiver() = default;
  virtual void RegisterCallback(RTSR_MESSAGE_TYPE type, RequestCallback callback) = 0;
  // Register every callback before starting, otherwise a request can arrive
  // before its handler exists.
  virtual void Start() = 0;
  // Stops listening and joins the worker. Safe to call from any thread and more
  // than once. A request being handled at that moment loses its response, so the
  // peer sees a failed request rather than a hang.
  virtual void Stop() = 0;
};

// These own sockets, so they must not have static storage duration: winsock is
// unloaded before a DLL's static destructors run. Tie their lifetime to the
// owning plugin or application object instead.
std::unique_ptr<IPushSender> CreatePushSender(const std::string& address,
                                              uint16_t port,
                                              LogSink log = {});
std::unique_ptr<IPushReceiver> CreatePushReceiver(uint16_t port, LogSink log = {});
std::unique_ptr<IRequestSender> CreateRequestSender(const std::string& address,
                                                    uint16_t port,
                                                    LogSink log = {},
                                                    bool queueBeforeConnect = true);
std::unique_ptr<IRequestReceiver> CreateRequestReceiver(uint16_t port, LogSink log = {});

} // namespace gits::rtsr
