// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "metaDataActions.h"
#include "gits.h"
#include "streamFile.h"
#include "baseCommon.h"
#include "removedApis.h"

#include <plog/Log.h>

#include <fstream>

namespace {
std::optional<std::filesystem::path> normalizeStreamPath(std::filesystem::path streamPath) {
  if (streamPath.empty()) {
    LOG_DEBUG << "Couldn't read stream meta data. No stream path was provided.";
    return std::nullopt;
  }

  if (std::filesystem::exists(streamPath) && std::filesystem::is_directory(streamPath)) {
    streamPath = gits::StreamFileInDirectory(streamPath);
  }

  if (!std::filesystem::exists(streamPath)) {
    LOG_DEBUG << "Couldn't read stream meta data. Stream path:  " << streamPath
              << " doesn't exist.";
    return std::nullopt;
  }

  return streamPath;
}

} // namespace

namespace gits::gui {
YAML::Node orderedJsonStrToYaml(const std::string& str) {
  const auto json = nlohmann::ordered_json::parse(str, nullptr, false);
  return orderedJsonToYaml(json);
}

YAML::Node orderedJsonToYaml(const nlohmann::ordered_json& j) {
  YAML::Node node;
  if (j.is_object()) {
    for (const auto& [key, value] : j.items()) {
      node[key] = orderedJsonToYaml(value);
    }
  } else if (j.is_array()) {
    for (const auto& element : j) {
      node.push_back(orderedJsonToYaml(element));
    }
  } else if (j.is_string()) {
    node = j.get<std::string>();
  } else if (j.is_boolean()) {
    node = j.get<bool>();
  } else if (j.is_number_integer()) {
    node = j.get<int64_t>();
  } else if (j.is_number_unsigned()) {
    node = j.get<uint64_t>();
  } else if (j.is_number_float()) {
    node = j.get<double>();
  } else {
    // null or discarded
    node = YAML::Node(YAML::NodeType::Null);
  }
  return node;
}

Api StreamHeaderApiToApi(stream::StreamHeader::Api api) {
  switch (api) {
  case stream::StreamHeader::Api::API_DIRECTX:
    return Api::DIRECTX;
  case stream::StreamHeader::Api::API_VULKAN:
    return Api::VULKAN;
  case stream::StreamHeader::Api::API_OPENGL:
    return Api::OPENGL;
  default:
    return Api::UNKNOWN;
  }
}

namespace {
STREAM_META_DATA GetStreamMetaData(const stream::StreamHeader& streamHeader) {
  STREAM_META_DATA metaData;
  metaData.IsValid = true;
  const auto version = streamHeader.GetVersion();
  metaData.Version =
      gits::CVersion(GITS_MAKE_VERSION4(version[0], version[1], version[2], version[3]));
  metaData.StreamApi = StreamHeaderApiToApi(streamHeader.GetApi());
  metaData.Compression = streamHeader.GetCompressionType();

  if (auto recorderConfig = streamHeader.FindProperty("diag.gits.config"); recorderConfig) {
    if (recorderConfig->is_string()) {
      metaData.RecorderConfig = recorderConfig->get<std::string>();
    } else {
      metaData.RecorderConfig = recorderConfig->dump();
    }
    if (gits::Configurator::Instance().Load(metaData.RecorderConfig)) {
      metaData.IsASerializedSubcapture =
          gits::Configurator::Get().common.player.subcapture.directx.executionSerialization;
    }
  }

  if (auto recorderDiags = streamHeader.FindProperty("diag"); recorderDiags) {
    metaData.RecorderDiags = *recorderDiags;
    metaData.RecorderDiagsYAML = orderedJsonToYaml(metaData.RecorderDiags);
  }
  return metaData;
}
} // namespace

STREAM_META_DATA LoadStreamMetaData(std::filesystem::path streamPath) {
  auto normalizedPath = normalizeStreamPath(std::move(streamPath));
  if (!normalizedPath) {
    return STREAM_META_DATA();
  }

  try {
    std::ifstream stream(*normalizedPath, std::ios::binary);
    if (!stream.is_open()) {
      LOG_ERROR << "Couldn't open stream file for reading metadata. Stream path: "
                << *normalizedPath;
      return STREAM_META_DATA();
    }

    auto& header = stream::StreamHeader::Get();
    header.ReadHeader(stream);
    if (!stream) {
      LOG_ERROR << "Couldn't get stream metadata, the stream header is incomplete. Stream path: "
                << *normalizedPath;
      return STREAM_META_DATA();
    }

    if (auto error = stream::FindRemovedApiError(*normalizedPath)) {
      LOG_WARNING << *error;
      STREAM_META_DATA metaData;
      metaData.UnsupportedStreamError = std::move(*error);
      return metaData;
    }
    return GetStreamMetaData(header);
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't get stream metadata. Error: " << e.what();
    return STREAM_META_DATA();
  }
}
} // namespace gits::gui
