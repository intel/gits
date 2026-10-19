// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/protocol/rtsrMessage.h"
#include "rtsr/shared/transport/rtsrTransport.h"

#include <cstdint>
#include <filesystem>

namespace gits::rtsr {

constexpr uint32_t DEFAULT_CONNECT_TIMEOUT_MS = 2000;

struct PluginConfig {
  EndpointConfig Endpoints;
  uint32_t ConnectTimeoutMs{DEFAULT_CONNECT_TIMEOUT_MS};
};

// Reads the Config map beside a plugin DLL. Missing/malformed fields retain
// their defaults; a malformed file is reported through the supplied sink.
PluginConfig ReadPluginConfig(const std::filesystem::path& path, LogSink log = {});

} // namespace gits::rtsr
