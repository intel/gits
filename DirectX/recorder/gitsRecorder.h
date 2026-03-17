// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "streamWriter.h"

#include <mutex>
#include <queue>
#include <map>
#include <set>
#include <memory>
#include <atomic>

namespace gits {
namespace DirectX {

class GitsRecorder {
public:
  GitsRecorder();
  void record(unsigned tokenKey, stream::CommandSerializer* token);
  void frameEnd(unsigned tokenKey);
  void skip(unsigned tokenKey);

private:
  struct OrderedToken {
    unsigned key;
    stream::CommandSerializer* token;

    bool operator>(const OrderedToken& other) const {
      return key > other.key;
    }
  };

  struct KeyRange {
    unsigned startKey;
    unsigned endKey;
  };

  void close();
  void orderedSchedule(OrderedToken token);
  void updateNextKey();
  void checkPendingTokens();
  void checkFrameEnd(unsigned key);
  std::map<unsigned, KeyRange>::iterator getRangeIt(unsigned tokenKey);

  std::unique_ptr<stream::StreamWriter> recorder_;
  std::atomic<bool> closed_{};
  std::mutex mutex_;
  unsigned nextKey_{1};
  std::set<unsigned> frameEndKeys_;
  std::map<unsigned, KeyRange> skippedKeyRanges_;
  std::priority_queue<OrderedToken, std::vector<OrderedToken>, std::greater<OrderedToken>> tokens_;
};

} // namespace DirectX
} // namespace gits
