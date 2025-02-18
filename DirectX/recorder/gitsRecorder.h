// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
namespace DirectX {

class GitsRecorder {
public:
  GitsRecorder();
  void record(unsigned tokenKey, CToken* token);
  void frameEnd(unsigned tokenKey);
  void skip(unsigned tokenKey);

private:
  struct OrderedToken {
    unsigned key;
    CToken* token;

    bool operator>(const OrderedToken& other) const {
      return key > other.key;
    }
  };

  struct KeyRange {
    unsigned startKey;
    unsigned endKey;
  };

  void orderedSchedule(OrderedToken token);
  void updateNextKey();
  void checkPendingTokens();
  void checkFrameEnd(unsigned key);
  std::map<unsigned, KeyRange>::iterator getRangeIt(unsigned tokenKey);

  gits::CRecorder* recorder_{nullptr};
  std::mutex mutex_;
  unsigned nextKey_{1};
  std::set<unsigned> frameEndKeys_;
  std::map<unsigned, KeyRange> skippedKeyRanges_;
  std::priority_queue<OrderedToken, std::vector<OrderedToken>, std::greater<OrderedToken>> tokens_;
};

} // namespace DirectX
} // namespace gits
