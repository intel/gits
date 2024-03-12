// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   oclocStateDynamic.cpp
 *
 * @brief Definition of ocloc state dynamic implementation.
 *
 */

#include "oclocStateDynamic.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace gits {
namespace ocloc {
COclocState::COclocState(std::vector<std::string> arguments,
                         std::vector<const uint8_t*> srcData,
                         std::vector<uint64_t> srcLens,
                         std::vector<const char*> srcNames,
                         std::vector<const uint8_t*> hdrData,
                         std::vector<uint64_t> hdrLens,
                         std::vector<const char*> hdrNames,
                         std::vector<uint8_t*> outData,
                         std::vector<uint64_t> outLens,
                         std::vector<char*> outNames)
    : args(std::move(arguments)),
      sourceData(srcData.size()),
      sourceLens(std::move(srcLens)),
      headerData(hdrData.size()),
      headerLens(std::move(hdrLens)),
      outputData(outData.size()),
      outputLens(std::move(outLens)) {
  std::transform(
      srcData.begin(), srcData.end(), sourceLens.begin(), sourceData.begin(),
      [](const uint8_t* ptr, size_t size) { return std::vector<uint8_t>(ptr, ptr + size); });

  std::transform(srcNames.begin(), srcNames.end(), std::back_inserter(sourceNames),
                 [](const char* ptr) { return std::string(ptr); });

  std::transform(
      hdrData.begin(), hdrData.end(), headerLens.begin(), headerData.begin(),
      [](const uint8_t* ptr, size_t size) { return std::vector<uint8_t>(ptr, ptr + size); });

  std::transform(hdrNames.begin(), hdrNames.end(), std::back_inserter(headerNames),
                 [](const char* ptr) { return std::string(ptr); });

  std::transform(
      outData.begin(), outData.end(), outputLens.begin(), outputData.begin(),
      [](const uint8_t* ptr, size_t size) { return std::vector<uint8_t>(ptr, ptr + size); });

  std::transform(outNames.begin(), outNames.end(), std::back_inserter(outputNames),
                 [](const char* ptr) { return std::string(ptr); });
}
} // namespace ocloc
} // namespace gits
