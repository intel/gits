// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gitsRecorder.h"
#include "messageBus.h"
#include "commandSerializersCustom.h"
#include "configurator.h"

namespace gits {
namespace DirectX {

GitsRecorder::GitsRecorder() {
  recorder_.reset(new stream::StreamWriter(Configurator::Get().common.recorder.dumpPath));
  gits::MessageBus::get().subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT},
                                    [this](Topic t, const MessagePtr& m) { close(); });
  gits::MessageBus::get().subscribe({PUBLISHER_PLUGIN, TOPIC_CLOSE_RECORDER},
                                    [this](Topic t, const MessagePtr& m) { close(); });
}

void GitsRecorder::record(unsigned tokenKey, stream::CommandSerializer* token) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (closed_) {
    delete token;
    return;
  }
  orderedSchedule({tokenKey, token});
}

void GitsRecorder::close() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (closed_) {
    return;
  }
  closed_ = true;
  checkPendingTokens();
  recorder_->Close();

  gits::MessageBus::get().publish(
      {PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
      std::make_shared<StreamSavedMessage>(Configurator::Get().common.recorder.dumpPath.string()));
  gits::MessageBus::get().publish({PUBLISHER_RECORDER, TOPIC_END},
                                  std::make_shared<EndOfRecordingMessage>());
}

void GitsRecorder::frameEnd(unsigned tokenKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  frameEndKeys_.insert(tokenKey);
}

void GitsRecorder::skip(unsigned tokenKey) {
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
    recorder_->Record(*token.token);
    delete token.token;
    checkFrameEnd(token.key);
    updateNextKey();
    checkPendingTokens();
  } else if (token.key > nextKey_ && getRangeIt(token.key) == skippedKeyRanges_.end()) {
    tokens_.push(token);
  } else {
    delete token.token;
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
    recorder_->Record(*tokens_.top().token);
    delete tokens_.top().token;
    checkFrameEnd(tokens_.top().key);
    tokens_.pop();
    updateNextKey();
  }
}

void GitsRecorder::checkFrameEnd(unsigned key) {
  auto keyIt = frameEndKeys_.find(key);
  if (keyIt != frameEndKeys_.end()) {
    recorder_->Record(FrameEndSerializer(FrameEndCommand()));
    frameEndKeys_.erase(keyIt);
  }
}

std::map<unsigned, GitsRecorder::KeyRange>::iterator GitsRecorder::getRangeIt(unsigned tokenKey) {
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

} // namespace DirectX
} // namespace gits
