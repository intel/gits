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

namespace gits::gui {
STREAM_META_DATA GetStreamMetaData(std::filesystem::path streamPath) {
  // This function consists (mostly) of an already existing gits code with some calls removed
  // That way we can operate purely on the file, without engaging unnecessary GITS logic
  // In the future potentially GITS and the launcher could share one code path for this
  STREAM_META_DATA metaData;
  if (streamPath.empty()) {
    LOG_DEBUG << "Couldn't read stream meta data. No stream path was provided.";
    return metaData;
  }

  if (!std::filesystem::exists(streamPath)) {
    LOG_DEBUG << "Couldn't read stream meta data. Stream path:  " << streamPath
              << " doesn't exist.";
    return metaData;
  }

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
            metaData.RecorderConfig = j["diag"]["gits"]["config"].dump();
            for (size_t pos = 0;
                 (pos = metaData.RecorderConfig.find("\\n", pos)) != std::string::npos; pos += 1) {
              metaData.RecorderConfig.replace(pos, 2, "\n");
            } // This might sometimes wrongly replace "\n" in strings or Windows paths
            // It would be hard to cover every possibility
            // Since this is just a print I think it's fine
            j["diag"]["gits"].erase("config");
          }
          metaData.RecorderDiags = std::move(j);
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
  return metaData;
}
} // namespace gits::gui
