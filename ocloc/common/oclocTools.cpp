// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocTools.h"
#include "tools_lite.h"
#include "log2.h"
#include "arDecoder.h"

namespace gits {
namespace ocloc {
void LogOclocInvokeInput(unsigned int argc,
                         const char** argv,
                         const uint32_t numSources,
                         const uint8_t** sources,
                         const uint64_t* sourceLens,
                         const char** sourcesNames,
                         const uint32_t numInputHeaders,
                         const uint8_t** dataInputHeaders,
                         const uint64_t* lenInputHeaders,
                         const char** nameInputHeaders) {
  LOG_FORMAT_RAW
  LOG_TRACE << LOG_PREFIX << "oclocInvoke(";
  LOG_TRACE << argc << ", { ";
  for (unsigned int i = 0; i < argc; i++) {
    LOG_TRACE << argv[i] << ((i < argc - 1) ? ", " : " }, ");
  }
  LOG_TRACE << numSources << ", ";
  if (numSources != 0U) {
    for (uint32_t i = 0; i < numSources; i++) {
      std::basic_string<uint8_t> str(sources[i], 20);
      LOG_TRACE << "{ " << str.c_str() << " [...]" << ((i < numSources - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numSources; i++) {
      LOG_TRACE << sourceLens[i] << ((i < numSources - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numSources; i++) {
      LOG_TRACE << sourcesNames[i] << ((i < numSources - 1) ? ", " : " }, ");
    }
  } else {
    LOG_TRACE << sources << ", " << sourceLens << ", " << sourcesNames << ", ";
  }
  LOG_TRACE << numInputHeaders << ", ";
  if (numInputHeaders != 0U) {
    for (uint32_t i = 0; i < numInputHeaders; i++) {
      std::basic_string<uint8_t> str(dataInputHeaders[i], 20);
      LOG_TRACE << "{ " << str.c_str() << " [...]" << ((i < numInputHeaders - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numInputHeaders; i++) {
      LOG_TRACE << lenInputHeaders[i] << ((i < numInputHeaders - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numInputHeaders; i++) {
      LOG_TRACE << nameInputHeaders[i] << ((i < numInputHeaders - 1) ? ", " : " } ");
    }
  } else {
    LOG_TRACE << dataInputHeaders << ", " << lenInputHeaders << ", " << nameInputHeaders;
  }
  LOG_TRACE << ", ... )";
}

void LogOclocInvokeOutput(int ret,
                          uint32_t* numOutputs,
                          uint8_t*** dataOutputs,
                          uint64_t** lenOutputs,
                          char*** nameOutputs) {
  LOG_FORMAT_RAW
  LOG_TRACE << " = " << ret << std::endl;
  if (numOutputs != nullptr) {
    LOG_TRACEV << ">>>> out numOutputs: " << *numOutputs << std::endl;
    if (dataOutputs != nullptr) {
      LOG_TRACEV << ">>>> out dataOutputs: { ";
      for (uint32_t i = 0; i < *numOutputs; i++) {
        const auto nameOutput = std::string(static_cast<const char*>((*nameOutputs)[i]));
        if (StringEndsWith(nameOutput, std::string(".log")) ||
            StringEndsWith(nameOutput, std::string(".txt"))) {
          LOG_TRACEV << static_cast<const uint8_t*>((*dataOutputs)[i])
                     << ((i < *numOutputs - 1) ? ", " : " }\n");
        } else {
          LOG_TRACEV << static_cast<const void*>((*dataOutputs)[i])
                     << ((i < *numOutputs - 1) ? ", " : " }\n");
        }
      }
    }
    if (lenOutputs != nullptr) {
      LOG_TRACEV << ">>>> out lenOutputs: { ";
      for (uint32_t i = 0; i < *numOutputs; i++) {
        LOG_TRACEV << (*lenOutputs)[i] << ((i < *numOutputs - 1) ? ", " : " }\n");
      }
    }
    if (nameOutputs != nullptr) {
      LOG_TRACEV << ">>>> out nameOutputs: { ";
      for (uint32_t i = 0; i < *numOutputs; i++) {
        LOG_TRACEV << static_cast<const char*>((*nameOutputs)[i])
                   << ((i < *numOutputs - 1) ? ", " : " }\n");
      }
    }
  }
}

bool IsAr(const uint8_t* binary) {
  const auto binaryMagic = std::string(reinterpret_cast<const char*>(binary), Ar::arMagic.size());
  return binaryMagic == Ar::arMagic;
}
} // namespace ocloc
} // namespace gits
