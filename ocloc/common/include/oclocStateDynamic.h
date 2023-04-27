// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace gits {
namespace ocloc {
struct COclocState {
  using type = uint64_t;
  using states_type = std::unordered_map<type, std::shared_ptr<COclocState>>;
  std::vector<std::string> args;
  std::vector<std::vector<uint8_t>> sourceData;
  std::vector<uint64_t> sourceLens;
  std::vector<std::string> sourceNames;
  std::vector<std::vector<uint8_t>> headerData;
  std::vector<uint64_t> headerLens;
  std::vector<std::string> headerNames;
  std::vector<std::vector<uint8_t>> outputData;
  std::vector<uint64_t> outputLens;
  std::vector<std::string> outputNames;
  std::unordered_set<std::string> savedFileNames;
  std::vector<uint64_t> originalHashes;
  std::vector<uint64_t> hashes;

public:
  COclocState() = default;
  COclocState(std::vector<std::string> arguments,
              std::vector<const uint8_t*> sourceData,
              std::vector<uint64_t> sourceLens,
              std::vector<const char*> sourceNames,
              std::vector<const uint8_t*> headerData,
              std::vector<uint64_t> headerLens,
              std::vector<const char*> headerNames,
              std::vector<uint8_t*> outputData,
              std::vector<uint64_t> outputLens,
              std::vector<char*> outputNames);
};
class CStateDynamic {
private:
  CStateDynamic() = default;

public:
  ~CStateDynamic() = default;

  // filename -> binary
  std::unordered_map<std::string, std::vector<uint8_t>> deprecatedPlayer;
  // binary hash -> oclocState
  typename COclocState::states_type oclocStates;

  static CStateDynamic& Instance() {
    static CStateDynamic instance;
    return instance;
  }
};
inline CStateDynamic& SD() {
  return CStateDynamic::Instance();
}
} // namespace ocloc
} // namespace gits
