// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "oclocArguments.h"
#include "oclocDrivers.h"
#include "oclocHeader.h"
#include "oclocStateDynamic.h"

#include <string>
#include <vector>

namespace gits {
namespace ocloc {
namespace {
std::vector<std::string> GetStringVector(const uint32_t count, const char** arr) {
  std::vector<std::string> vector;
  for (uint32_t i = 0; i < count; ++i) {
    vector.emplace_back(arr[i]);
  }
  return vector;
}
} // namespace
inline void oclocInvoke_SD(int return_value,
                           unsigned int argc,
                           const char** argv,
                           const uint32_t numSources,
                           const uint8_t** sources,
                           const uint64_t* sourceLens,
                           const char** sourcesNames,
                           const uint32_t numInputHeaders,
                           const uint8_t** dataInputHeaders,
                           const uint64_t* lenInputHeaders,
                           const char** nameInputHeaders,
                           uint32_t* numOutputs,
                           uint8_t*** dataOutputs,
                           uint64_t** lenOutputs,
                           char*** nameOutputs) {
  if (return_value == 0 && dataOutputs != nullptr) {
    for (uint32_t i = 0; i < *numOutputs; ++i) {
      uint64_t hash = ComputeHash((*dataOutputs)[i], (*lenOutputs)[i], THashType::XX);
      const auto args = GetStringVector(argc, argv);
      if (Config::IsRecorder()) {
        SD().recorder[hash] = std::string((*nameOutputs)[i]);
      } else {
        const auto moduleData =
            std::vector<uint8_t>((*dataOutputs)[i], (*dataOutputs)[i] + (*lenOutputs)[i]);
        SD().player[std::string((*nameOutputs)[i])] = moduleData;
      }
      SD().oclocStates[hash] = std::make_shared<COclocState>(
          args, std::vector<const uint8_t*>(sources, sources + numSources),
          std::vector<uint64_t>(sourceLens, sourceLens + numSources),
          std::vector<const char*>(sourcesNames, sourcesNames + numSources),
          std::vector<const uint8_t*>(dataInputHeaders, dataInputHeaders + numInputHeaders),
          std::vector<uint64_t>(lenInputHeaders, lenInputHeaders + numInputHeaders),
          std::vector<const char*>(nameInputHeaders, nameInputHeaders + numInputHeaders),
          std::vector<uint8_t*>(*dataOutputs, *dataOutputs + *numOutputs),
          std::vector<uint64_t>(*lenOutputs, *lenOutputs + *numOutputs),
          std::vector<char*>(*nameOutputs, *nameOutputs + *numOutputs));
    }
  }
}
} // namespace ocloc
} // namespace gits