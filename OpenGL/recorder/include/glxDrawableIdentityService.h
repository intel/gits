// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct _XDisplay;
typedef struct _XDisplay Display;

namespace gits::opengl {

// Two X servers can give different drawables the same XID, so the recorder records a GLX drawable,
// X window or X pixmap as its identity: the XID, with the index of its X server among those the
// application used in the high 32 bits. The first server has index 0, so its identities are its
// XIDs. Connections to the same server share its index. Replay and subcapture treat identities as
// opaque drawables.
class GlxDrawableIdentityService {
public:
  static GlxDrawableIdentityService& Get();

  // The index of the server `display` is connected to.
  uint32_t Server(Display* display);
  // The identity of `drawable`, an XID on the server `display` is connected to; 0 for 0.
  uint64_t Identity(Display* display, uint64_t drawable);

private:
  GlxDrawableIdentityService();

  struct Connection {
    int Socket{-1};
    std::string Name;
    uint32_t Server{};
  };

  std::mutex m_Mutex;
  // A Display that is closed can be reused for a new connection, which is told apart by its socket
  // or its display name.
  std::unordered_map<Display*, Connection> m_Connections;
  // The name of an atom interned on each server, unique to this process, by server index. A new
  // connection's server is the one on which one of these atoms exists.
  std::vector<std::string> m_ServerAtoms;
  std::string m_ServerAtomPrefix;
};

} // namespace gits::opengl
