// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "recorder.h"
#include "token.h"

#include <mutex>
#include <queue>
#include <map>
#include <set>

namespace gits {
namespace vulkan {

class GitsRecorder {
public:
  GitsRecorder();
  void record(uint64_t tokenKey, CToken* token);
  void frameEnd(uint64_t tokenKey);
  void skip(uint64_t tokenKey);

private:
  struct OrderedToken {
    uint64_t key;
    CToken* token;

    bool operator>(const OrderedToken& other) const {
      return key > other.key;
    }
  };

  struct KeyRange {
    uint64_t startKey;
    uint64_t endKey;
  };

  void orderedSchedule(OrderedToken token);
  void updateNextKey();
  void checkPendingTokens();
  void checkFrameEnd(uint64_t key);
  std::map<uint64_t, KeyRange>::iterator getRangeIt(uint64_t tokenKey);

  gits::CRecorder* recorder_{nullptr};
  std::mutex mutex_;
  uint64_t nextKey_{1};
  std::set<uint64_t> frameEndKeys_;
  std::map<uint64_t, KeyRange> skippedKeyRanges_;
  std::priority_queue<OrderedToken, std::vector<OrderedToken>, std::greater<OrderedToken>> tokens_;
};

} // namespace vulkan
} // namespace gits
