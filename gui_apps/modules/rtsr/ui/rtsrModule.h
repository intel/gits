// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/server/rtsrServer.h"
#include "rtsr/shared/server/rtsrSessionStore.h"
#include "rtsr/ui/rtsrPerformanceStats.h"
#include "rtsr/ui/rtsrReplacementService.h"

namespace gits::gui {

// Backend of the RTSR module: it owns the RTSR server and the session that
// server harvests, so this application takes the place of RTSREditor. The panel
// drives its lifetime and reads the store for the pipeline the user selected;
// everything else reaches the UI as an RtsrEvent on the EventBus.
class RtsrModule {
public:
  RtsrModule();
  ~RtsrModule();

  RtsrModule(const RtsrModule&) = delete;
  RtsrModule& operator=(const RtsrModule&) = delete;

  // Binds the RTSR ports so an instrumented process can connect. Returns false
  // when a port is taken, which the panel reports rather than looking idle.
  bool Start();

  // Ends any session and releases the ports. Also runs from the destructor,
  // which is why the module must not outlive the application: the sockets have
  // to close before winsock is unloaded.
  void Stop();

  bool IsListening() const {
    return m_Server.IsListening();
  }

  // False once a run has ended, while everything it harvested is still on
  // screen, so the panel can tell a finished run from a live one.
  bool IsSessionOpen() const {
    return m_Store.IsSessionOpen();
  }

  const rtsr::SessionStore& Store() const {
    return m_Store;
  }

  const PerformanceStats& Performance() const {
    return m_Performance;
  }

  RtsrReplacementService& Replacements() {
    return m_Replacements;
  }

  // Asks the connected client to start or stop timing its pipelines, reporting
  // what it answered. False while nothing is connected, and false for a client
  // whose graphics API has no timing support yet, because it refuses the
  // request rather than accepting one it cannot honour.
  bool EnablePerformanceMeasurements(bool enable);

private:
  // Declared before the server, which holds a reference to the store and whose
  // receive threads feed both. Reverse destruction order therefore stops those
  // threads before either of these goes away.
  rtsr::SessionStore m_Store;
  PerformanceStats m_Performance;
  rtsr::RtsrServer m_Server;
  // Last: the worker may wait on the server, so it is destroyed first. Stop()
  // closes the server request socket before member destruction joins it.
  RtsrReplacementService m_Replacements;
};

} // namespace gits::gui
