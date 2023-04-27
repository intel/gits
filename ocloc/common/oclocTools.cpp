// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocTools.h"
#include "tools_lite.h"

namespace gits {
namespace ocloc {

COclocLog& COclocLog::operator<<(manip t) {
  _buffer << t;
  return *this;
}

COclocLog& COclocLog::operator<<(const char c) {
  return operator<< <int>(c);
}

COclocLog& COclocLog::operator<<(const unsigned char c) {
  return operator<< <unsigned>(c);
}

COclocLog& COclocLog::operator<<(const char* c) {
  if (c != nullptr) {
    _buffer << c;
  } else {
    _buffer << (const void*)c;
  }
  return *this;
}

COclocLog& COclocLog::operator<<(char* c) {
  _buffer << (const void*)c;
  return *this;
}

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
  OclocLog(TRACE, NO_NEWLINE) << "oclocInvoke(";
  OclocLog(TRACE, RAW) << argc << ", { ";
  for (unsigned int i = 0; i < argc; i++) {
    OclocLog(TRACE, RAW) << argv[i] << ((i < argc - 1) ? ", " : " }, ");
  }
  OclocLog(TRACE, RAW) << numSources << ", ";
  if (numSources != 0U) {
    for (uint32_t i = 0; i < numSources; i++) {
      std::basic_string<uint8_t> str(sources[i], 20);
      OclocLog(TRACE, RAW) << "{ " << str.c_str() << " [...]"
                           << ((i < numSources - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numSources; i++) {
      OclocLog(TRACE, RAW) << sourceLens[i] << ((i < numSources - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numSources; i++) {
      OclocLog(TRACE, RAW) << sourcesNames[i] << ((i < numSources - 1) ? ", " : " }, ");
    }
  } else {
    OclocLog(TRACE, RAW) << sources << ", " << sourceLens << ", " << sourcesNames << ", ";
  }
  OclocLog(TRACE, RAW) << numInputHeaders << ", ";
  if (numInputHeaders != 0U) {
    for (uint32_t i = 0; i < numInputHeaders; i++) {
      std::basic_string<uint8_t> str(dataInputHeaders[i], 20);
      OclocLog(TRACE, RAW) << "{ " << str.c_str() << " [...]"
                           << ((i < numInputHeaders - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numInputHeaders; i++) {
      OclocLog(TRACE, RAW) << lenInputHeaders[i] << ((i < numInputHeaders - 1) ? ", " : " }, { ");
    }
    for (uint32_t i = 0; i < numInputHeaders; i++) {
      OclocLog(TRACE, RAW) << nameInputHeaders[i] << ((i < numInputHeaders - 1) ? ", " : " } ");
    }
  } else {
    OclocLog(TRACE, RAW) << dataInputHeaders << ", " << lenInputHeaders << ", " << nameInputHeaders;
  }
  OclocLog(TRACE, RAW) << ", ... )";
}

void LogOclocInvokeOutput(int ret,
                          uint32_t* numOutputs,
                          uint8_t*** dataOutputs,
                          uint64_t** lenOutputs,
                          char*** nameOutputs) {
  OclocLog(TRACE, NO_PREFIX) << " = " << ret;
  if (numOutputs != nullptr) {
    OclocLog(TRACEV, NO_PREFIX) << ">>>> out numOutputs: " << *numOutputs;
    if (dataOutputs != nullptr) {
      OclocLog(TRACEV, RAW) << ">>>> out dataOutputs: { ";
      for (uint32_t i = 0; i < *numOutputs; i++) {
        const auto nameOutput = std::string(static_cast<const char*>((*nameOutputs)[i]));
        if (StringEndsWith(nameOutput, std::string(".log")) ||
            StringEndsWith(nameOutput, std::string(".txt"))) {
          OclocLog(TRACEV, RAW) << static_cast<const uint8_t*>((*dataOutputs)[i])
                                << ((i < *numOutputs - 1) ? ", " : " }\n");
        } else {
          OclocLog(TRACEV, RAW) << static_cast<const void*>((*dataOutputs)[i])
                                << ((i < *numOutputs - 1) ? ", " : " }\n");
        }
      }
    }
    if (lenOutputs != nullptr) {
      OclocLog(TRACEV, RAW) << ">>>> out lenOutputs: { ";
      for (uint32_t i = 0; i < *numOutputs; i++) {
        OclocLog(TRACEV, RAW) << (*lenOutputs)[i] << ((i < *numOutputs - 1) ? ", " : " }\n");
      }
    }
    if (nameOutputs != nullptr) {
      OclocLog(TRACEV, RAW) << ">>>> out nameOutputs: { ";
      for (uint32_t i = 0; i < *numOutputs; i++) {
        OclocLog(TRACEV, RAW) << static_cast<const char*>((*nameOutputs)[i])
                              << ((i < *numOutputs - 1) ? ", " : " }\n");
      }
    }
  }
}

} // namespace ocloc
} // namespace gits
