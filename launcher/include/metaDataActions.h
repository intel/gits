// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "version.h"
#include "nlohmann/json.hpp"
#include "apis_iface.h"

#include <filesystem>

namespace gits::gui {
struct STREAM_META_DATA {
  CVersion Version = 0; // Version of GITS that the stream was captured with
  nlohmann::ordered_json RecorderDiags = nlohmann::ordered_json();
  std::string LegacyRecorderDiags = std::string(); // Legacy diagnostics found in old streams
  std::string RecorderConfig = std::string();      // Config that the stream was captured with
  ApisIface::TApi Api3D = ApisIface::TApi::ApiNotSet;
  ApisIface::TApi ApiCompute = ApisIface::TApi::ApiNotSet;
};

STREAM_META_DATA GetStreamMetaData(std::filesystem::path streamPath);
} // namespace gits::gui
