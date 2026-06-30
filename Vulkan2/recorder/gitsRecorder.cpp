// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gitsRecorder.h"

namespace gits {
namespace vulkan {

void GitsRecorder::record(uint64_t tokenKey, CToken* token) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!recorder_) {
    return;
  } else if (recorder_->IsMarkedForDeletion()) {
    recorder_->Close();
    return;
  }
  orderedSchedule({tokenKey, token});
}

GitsRecorder::GitsRecorder() {
  recorder_ = &gits::CRecorder::Instance();
}

void GitsRecorder::frameEnd(uint64_t tokenKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  frameEndKeys_.insert(tokenKey);
}

void GitsRecorder::skip(uint64_t tokenKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (tokenKey == nextKey_) {
    updateNextKey();
    checkPendingTokens();
  } else {
    auto itKey = getRangeIt(tokenKey - 1);
    if (itKey != skippedKeyRanges_.end()) {
      ++itKey->second.endKey;
    } else {
      skippedKeyRanges_[tokenKey] = {tokenKey, tokenKey};
    }
  }
}

void GitsRecorder::orderedSchedule(OrderedToken token) {
  if (token.key == nextKey_) {
    recorder_->Schedule(token.token);
    checkFrameEnd(token.key);
    updateNextKey();
    checkPendingTokens();
  } else if (token.key > nextKey_ && getRangeIt(token.key) == skippedKeyRanges_.end()) {
    tokens_.push(token);
  }
}

void GitsRecorder::updateNextKey() {
  while (true) {
    ++nextKey_;
    if (skippedKeyRanges_.empty()) {
      break;
    }
    auto keyIt = getRangeIt(nextKey_);
    if (keyIt == skippedKeyRanges_.end()) {
      break;
    }

    if (nextKey_ == keyIt->second.startKey) {
      nextKey_ = keyIt->second.endKey;
      skippedKeyRanges_.erase(keyIt);
    }
  }
}

void GitsRecorder::checkPendingTokens() {
  while (!tokens_.empty()) {
    if (tokens_.top().key != nextKey_) {
      break;
    }
    recorder_->Schedule(tokens_.top().token);
    checkFrameEnd(tokens_.top().key);
    tokens_.pop();
    updateNextKey();
  }
}

void GitsRecorder::checkFrameEnd(uint64_t key) {
  auto keyIt = frameEndKeys_.find(key);
  if (keyIt != frameEndKeys_.end()) {
    recorder_->FrameEnd();
    frameEndKeys_.erase(keyIt);
  }
}

std::map<uint64_t, GitsRecorder::KeyRange>::iterator GitsRecorder::getRangeIt(uint64_t tokenKey) {
  auto itKey = skippedKeyRanges_.upper_bound(tokenKey);
  if (itKey == skippedKeyRanges_.begin()) {
    return skippedKeyRanges_.end();
  }

  --itKey;
  if (itKey->second.startKey <= tokenKey && itKey->second.endKey >= tokenKey) {
    return itKey;
  } else {
    return skippedKeyRanges_.end();
  }
}

} // namespace vulkan
} // namespace gits
