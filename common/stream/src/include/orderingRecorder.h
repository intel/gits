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
#include "tbb/concurrent_queue.h"

#include <queue>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>

namespace gits {
namespace stream {

class OrderingRecorder {
public:
  OrderingRecorder();
  ~OrderingRecorder();
  OrderingRecorder(const OrderingRecorder&) = delete;
  OrderingRecorder& operator=(const OrderingRecorder&) = delete;

  void Record(size_t key, stream::CommandSerializer* serializer);
  void Skip(size_t key);

private:
  struct OrderedSerializer {
    size_t Key;
    stream::CommandSerializer* Serializer;

    bool operator>(const OrderedSerializer& other) const {
      return Key > other.Key;
    }
  };

  struct KeyRange {
    size_t StartKey;
    size_t EndKey;
  };

  struct RecorderTask {
    union {
      OrderedSerializer Serializer;
      size_t SkippedKey;
    };
    enum class Type {
      OrderedSerializer,
      SkippedKey
    } Type;
  };

  void Close();
  void ConditionalFlush();
  void ProcessQueue();
  void SkipImpl(size_t key);
  void OrderedRecord(OrderedSerializer serializer);
  void UpdateNextKey();
  void CheckPendingSerializers();
  std::map<size_t, KeyRange>::iterator GetSkippedRangeIt(size_t key);

  std::unique_ptr<stream::StreamWriter> m_Recorder;
  std::atomic<bool> m_Closed{};
  size_t m_NextKey{1};
  std::map<size_t, KeyRange> m_SkippedKeyRanges;
  std::priority_queue<OrderedSerializer,
                      std::vector<OrderedSerializer>,
                      std::greater<OrderedSerializer>>
      m_Serializers;

  tbb::concurrent_queue<RecorderTask> m_ConcurrentQueue;
  std::thread m_Thread;

  static constexpr size_t BYTES_CHECK_INTERVAL = 64 * 1024 * 1024;
  static constexpr size_t BYTES_ALLOWED_DIFFERENCE = 256 * 1024 * 1024;
  std::atomic<size_t> m_ScheduledBytes{};
  std::atomic<size_t> m_ProcessedBytes{};
  std::atomic<size_t> m_LastCheckedBytes{};
};

} // namespace stream
} // namespace gits
