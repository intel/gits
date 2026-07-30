// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "metaDataActions.h"
#include "streams.h"
#include "gits.h"
#include "streamHeader.h"
#include "common.h"
#include "labels.h"

#include <plog/Log.h>

namespace {
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

std::optional<std::filesystem::path> normalizeStreamPath(std::filesystem::path streamPath) {
  if (streamPath.empty()) {
    LOG_DEBUG << "Couldn't read stream meta data. No stream path was provided.";
    return std::nullopt;
  }

  if (std::filesystem::exists(streamPath) && std::filesystem::is_directory(streamPath)) {
    streamPath = streamPath / gits::gui::filesystem_names::GITS_STREAM;
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
Api StreamHeaderApiToApi(stream::StreamHeader::Api api) {
  switch (api) {
  case stream::StreamHeader::Api::API_DIRECTX:
    return Api::DIRECTX;
  case stream::StreamHeader::Api::API_VULKAN:
    return Api::VULKAN;
  case stream::StreamHeader::Api::API_VULKAN_LEGACY:
    return Api::VULKAN_LEGACY;
  case stream::StreamHeader::Api::API_OPENGL:
    return Api::OPENGL;
  case stream::StreamHeader::Api::API_OPENCL:
    return Api::OPENCL;
  case stream::StreamHeader::Api::API_LEVELZERO:
    return Api::LEVELZERO;
  default:
    return Api::UNKNOWN;
  }
}

namespace {
STREAM_META_DATA GetLegacyStreamMetaData(const std::filesystem::path& streamPath) {
  // This function consists (mostly) of an already existing gits code with some calls removed
  // That way we can operate purely on the file, without engaging unnecessary GITS logic
  // In the future potentially GITS and the launcher could share one code path for this
  STREAM_META_DATA metaData;

  try {
    gits::CBinIStream stream(streamPath);
    gits::CVersion version;
    stream >> version;
    metaData.IsValid = true;
    metaData.Version = version;
    metaData.IsLegacyStream = true;

    // We don't make use of this but we read it in order to get to the recorder diagnostics
    uint32_t skipNum = 0U;
    stream.ReadHelper(reinterpret_cast<char*>(&skipNum), sizeof(skipNum));
    if (skipNum <= UINT32_MAX) {
      for (uint32_t i = 0; i < skipNum; i++) {
        uint32_t id = 0U;
        stream.ReadHelper(reinterpret_cast<char*>(&id), sizeof(id));

        uint32_t num = 0U;
        stream.ReadHelper(reinterpret_cast<char*>(&num), sizeof(num));
      }
    }

    uint32_t propsLength = 0;
    stream.ReadHelper(reinterpret_cast<char*>(&propsLength), sizeof(propsLength));
    if (propsLength <= UINT32_MAX) {
      std::string props(propsLength, '\0');
      stream.ReadHelper(&props[0], propsLength);
      bool result = false;
      if (props.find("<?xml version") != std::string::npos) {
        metaData.LegacyRecorderDiags = std::move(props);
        result = true;
      } else {
        nlohmann::ordered_json j = nlohmann::ordered_json::parse(props, nullptr, false);
        if (!j.is_discarded()) {
          if (!j.empty()) {
            if (j.contains("diag") && j["diag"].contains("gits") &&
                j["diag"]["gits"].contains("config")) {
              metaData.RecorderConfig = j["diag"]["gits"]["config"];
              if (metaData.RecorderConfig.size() >= 2 && metaData.RecorderConfig.front() == '"' &&
                  metaData.RecorderConfig.back() == '"') {
                metaData.RecorderConfig =
                    metaData.RecorderConfig.substr(1, metaData.RecorderConfig.size() - 2);
              }
              if (gits::Configurator::Instance().Load(metaData.RecorderConfig)) {
                metaData.IsASerializedSubcapture =
                    gits::Configurator::Get()
                        .common.player.subcapture.directx.executionSerialization;
              }
              j["diag"]["gits"].erase("config");
            }
            metaData.RecorderDiags = std::move(j);
            if (metaData.RecorderDiags.is_object() && metaData.RecorderDiags.contains("diag")) {
              metaData.RecorderDiags = metaData.RecorderDiags["diag"];
            }
            metaData.RecorderDiagsYAML = orderedJsonToYaml(metaData.RecorderDiags);
          }
          result = true;
        }
      }
      if (!result) {
        LOG_WARNING << "Error occured when parsing diagnostic meta data";
        LOG_WARNING
            << "Recapturing the stream with option Common.Recorder.ExtendedDiagnostic disabled "
               "might help.";
      }
    }

    if (version.version() >= GITS_API_INFO) {
      gits::ApisIface::TApi api3D = gits::ApisIface::TApi::ApiNotSet;
      stream.ReadHelper(reinterpret_cast<char*>(&api3D), sizeof(gits::ApisIface::TApi));

      gits::ApisIface::TApi apiCompute = gits::ApisIface::TApi::ApiNotSet;
      stream.ReadHelper(reinterpret_cast<char*>(&apiCompute), sizeof(gits::ApisIface::TApi));

      // Use 3D API if present, otherwise use compute API
      if (api3D != gits::ApisIface::TApi::ApiNotSet) {
        metaData.StreamApi = TApiToApi(api3D);
        if (apiCompute != gits::ApisIface::TApi::ApiNotSet) {
          LOG_WARNING << Labels::METADATA_BOTH_APIS_FOUND_WARNING;
        }
      } else if (apiCompute != gits::ApisIface::TApi::ApiNotSet) {
        metaData.StreamApi = TApiToApi(apiCompute);
      } else {
        LOG_WARNING << Labels::METADATA_NO_API_FOUND_WARNING;
        LOG_WARNING << Labels::METADATA_MALFORMED_STREAM_WARNING;
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't get stream metadata. Error: " << e.what();
    return STREAM_META_DATA();
  }

  return metaData;
}

STREAM_META_DATA GetStreamMetaData(const std::filesystem::path& streamPath) {
  STREAM_META_DATA metaData;

  try {
    std::ifstream stream(streamPath, std::ios::binary);
    if (!stream.is_open()) {
      LOG_ERROR << "Couldn't open stream file for reading metadata. Stream path: " << streamPath;
      return STREAM_META_DATA();
    }

    auto& streamHeader = stream::StreamHeader::Get();
    streamHeader.ReadHeader(stream);

    metaData.IsValid = true;
    metaData.IsLegacyStream = false;
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
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't get stream metadata from new stream header. Error: " << e.what();
    return STREAM_META_DATA();
  }

  return metaData;
}
} // namespace

STREAM_META_DATA LoadStreamMetaData(std::filesystem::path streamPath) {
  auto normalizedPath = normalizeStreamPath(streamPath);
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
    if (header.IsLegacyStream()) {
      return GetLegacyStreamMetaData(*normalizedPath);
    }
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't determine stream metadata loading path. Error: " << e.what();
    return STREAM_META_DATA();
  }

  return GetStreamMetaData(*normalizedPath);
}

} // namespace gits::gui
