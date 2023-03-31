// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

#include <utility>

namespace gits {
namespace ocloc {
COclocState::COclocState(std::vector<std::string> arguments,
                         std::vector<const uint8_t*> sourceData,
                         std::vector<uint64_t> sourceLens,
                         std::vector<const char*> sourceNames,
                         std::vector<const uint8_t*> headerData,
                         std::vector<uint64_t> headerLens,
                         std::vector<const char*> headerNames,
                         std::vector<uint8_t*> outputData,
                         std::vector<uint64_t> outputLens,
                         std::vector<char*> outputNames)
    : args(std::move(arguments)),
      sourceData(std::move(sourceData)),
      sourceLens(std::move(sourceLens)),
      sourceNames(std::move(sourceNames)),
      headerData(std::move(headerData)),
      headerLens(std::move(headerLens)),
      headerNames(std::move(headerNames)),
      outputData(std::move(outputData)),
      outputLens(std::move(outputLens)),
      outputNames(std::move(outputNames)) {}
} // namespace ocloc
} // namespace gits