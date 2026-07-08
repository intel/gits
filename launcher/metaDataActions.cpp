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
#include "common.h"

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

} // namespace

namespace gits::gui {
STREAM_META_DATA GetStreamMetaData(std::filesystem::path streamPath) {
  // This function consists (mostly) of an already existing gits code with some calls removed
  // That way we can operate purely on the file, without engaging unnecessary GITS logic
  // In the future potentially GITS and the launcher could share one code path for this
  STREAM_META_DATA metaData;
  if (streamPath.empty()) {
    LOG_DEBUG << "Couldn't read stream meta data. No stream path was provided.";
    return STREAM_META_DATA();
  }

  if (std::filesystem::exists(streamPath) && std::filesystem::is_directory(streamPath)) {
    // If the streamPath is a directory rather than the stream filename, we try to find the stream under the default name
    streamPath = streamPath / filesystem_names::GITS_STREAM;
  }

  if (!std::filesystem::exists(streamPath)) {
    LOG_DEBUG << "Couldn't read stream meta data. Stream path:  " << streamPath
              << " doesn't exist.";
    return STREAM_META_DATA();
  }

  try {
    CBinIStream stream(streamPath);
    CVersion version;
    stream >> version;
    metaData.Version = version;

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
    nlohmann::ordered_json properties;
    if (propsLength <= UINT32_MAX) {
      std::string props(propsLength, '\0');
      stream.ReadHelper(&props[0], propsLength);
      bool result = false;
      if (props.find("<?xml version") != std::string::npos) {
        metaData.LegacyRecorderDiags = std::move(props);
        result = true;
      } else {
        nlohmann::ordered_json j = nlohmann::ordered_json::parse(props, nullptr, false);
        if (j.is_discarded()) {
          result = false;
        } else {
          if (!j.empty()) {
            // After getting the recorder config from the diags, we store it and erase it from diags
            if (j.contains("diag") && j["diag"].contains("gits") &&
                j["diag"]["gits"].contains("config")) {
              metaData.RecorderConfig = j["diag"]["gits"]["config"];
              if (metaData.RecorderConfig.size() >= 2 && metaData.RecorderConfig.front() == '"' &&
                  metaData.RecorderConfig.back() == '"') {
                metaData.RecorderConfig =
                    metaData.RecorderConfig.substr(1, metaData.RecorderConfig.size() - 2);
              }
              if (Configurator::Instance().Load(metaData.RecorderConfig)) {
                metaData.IsASerializedSubcapture =
                    Configurator::Get().directx.features.subcapture.executionSerialization;
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
      ApisIface::TApi api3D = ApisIface::TApi::ApiNotSet;
      stream.ReadHelper(reinterpret_cast<char*>(&api3D), sizeof(ApisIface::TApi));
      metaData.Api3D = api3D;

      ApisIface::TApi apiCompute = ApisIface::TApi::ApiNotSet;
      stream.ReadHelper(reinterpret_cast<char*>(&apiCompute), sizeof(ApisIface::TApi));
      metaData.ApiCompute = apiCompute;

      if (api3D == ApisIface::TApi::ApiNotSet && apiCompute == ApisIface::TApi::ApiNotSet) {
        LOG_WARNING << "No 3D or Compute API meta data could be read";
        LOG_WARNING << "The stream file might be malformed or corrupted";
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't get stream metadata. Error: " << e.what();
    return STREAM_META_DATA();
  }

  return metaData;
}
} // namespace gits::gui
