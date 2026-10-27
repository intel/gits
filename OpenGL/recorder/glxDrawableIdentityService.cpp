// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glxDrawableIdentityService.h"

#include "systemApi.h"

#ifndef _WIN32
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <random>
#include <sstream>
#endif

namespace gits::opengl {

GlxDrawableIdentityService& GlxDrawableIdentityService::Get() {
  static GlxDrawableIdentityService instance;
  return instance;
}

uint64_t GlxDrawableIdentityService::Identity(Display* display, uint64_t drawable) {
  if (drawable == 0 || display == nullptr) {
    return drawable;
  }
  return (static_cast<uint64_t>(Server(display)) << 32) | drawable;
}

#ifdef _WIN32
// GLX drawables exist only on Linux.
GlxDrawableIdentityService::GlxDrawableIdentityService() = default;

uint32_t GlxDrawableIdentityService::Server(Display*) {
  return 0;
}
#else
GlxDrawableIdentityService::GlxDrawableIdentityService() {
  std::random_device device;
  std::ostringstream prefix;
  prefix << "_GITS_OPENGL_SERVER_" << getpid() << '_' << std::hex << device() << '_'
         << std::chrono::steady_clock::now().time_since_epoch().count() << '_';
  m_ServerAtomPrefix = prefix.str();
}

uint32_t GlxDrawableIdentityService::Server(Display* display) {
  const LastSystemErrorGuard lastErrorGuard;
  const int socket = ConnectionNumber(display);
  const char* name = DisplayString(display) != nullptr ? DisplayString(display) : "";
  std::lock_guard<std::mutex> lock(m_Mutex);
  Connection& connection = m_Connections[display];
  if (connection.Socket == socket && connection.Name == name) {
    return connection.Server;
  }

  // The replies carry the errors, so they never reach the application's X error handler.
  xcb_connection_t* xcb = XGetXCBConnection(display);
  std::vector<xcb_intern_atom_cookie_t> cookies;
  cookies.reserve(m_ServerAtoms.size());
  for (const std::string& atom : m_ServerAtoms) {
    cookies.push_back(xcb_intern_atom(xcb, 1, static_cast<uint16_t>(atom.size()), atom.data()));
  }
  std::optional<uint32_t> server;
  for (size_t index = 0; index < cookies.size(); ++index) {
    xcb_generic_error_t* error = nullptr;
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(xcb, cookies[index], &error);
    if (!server && reply != nullptr && reply->atom != XCB_ATOM_NONE) {
      server = static_cast<uint32_t>(index);
    }
    std::free(reply);
    std::free(error);
  }
  if (!server) {
    server = static_cast<uint32_t>(m_ServerAtoms.size());
    std::string atom = m_ServerAtomPrefix + std::to_string(*server);
    xcb_generic_error_t* error = nullptr;
    std::free(xcb_intern_atom_reply(
        xcb, xcb_intern_atom(xcb, 0, static_cast<uint16_t>(atom.size()), atom.data()), &error));
    std::free(error);
    m_ServerAtoms.push_back(std::move(atom));
  }
  connection = {socket, name, *server};
  return *server;
}
#endif

} // namespace gits::opengl
