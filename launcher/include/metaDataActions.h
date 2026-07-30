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
#include "streamHeader.h"
#include "common.h"

#include <filesystem>
#include <string>
#include <yaml-cpp/yaml.h>

namespace gits::gui {
struct STREAM_META_DATA {
  bool IsValid = false;
  bool IsLegacyStream = true;
  CVersion Version = 0; // Version of GITS that the stream was captured with
  Api StreamApi = Api::UNKNOWN;
  CompressionType Compression = CompressionType::NONE;
  nlohmann::ordered_json RecorderDiags = nlohmann::ordered_json();
  YAML::Node RecorderDiagsYAML;
  std::string LegacyRecorderDiags = std::string(); // Legacy diagnostics found in old streams
  std::string RecorderConfig = std::string();      // Config that the stream was captured with
  bool IsASerializedSubcapture = false;
};

STREAM_META_DATA LoadStreamMetaData(std::filesystem::path streamPath);
} // namespace gits::gui
