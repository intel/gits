// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "orderingRecorder.h"
#include "messageBus.h"
#include "configurator.h"

namespace gits {
namespace stream {

OrderingRecorder::OrderingRecorder() {
  m_Recorder.reset(new stream::StreamWriter(Configurator::Get().common.recorder.dumpPath,
                                            Configurator::Get().common.recorder.compression.type));
  gits::MessageBus::get().subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT},
                                    [this](Topic t, const MessagePtr& m) { Close(); });
  gits::MessageBus::get().subscribe({PUBLISHER_PLUGIN, TOPIC_CLOSE_RECORDER},
                                    [this](Topic t, const MessagePtr& m) { Close(); });
  m_Thread = std::thread{&OrderingRecorder::ProcessQueue, this};
}

OrderingRecorder::~OrderingRecorder() {
  Close();
}

void OrderingRecorder::Record(size_t key, stream::CommandSerializer* serializer) {
  if (m_Closed) {
    delete serializer;
    return;
  }

  m_ScheduledBytes += serializer->Size();

  RecorderTask task;
  task.Type = RecorderTask::Type::OrderedSerializer;
  task.Serializer = {key, serializer};
  m_ConcurrentQueue.push(task);

  ConditionalFlush();
}

void OrderingRecorder::Skip(size_t key) {
  if (m_Closed) {
    return;
  }

  RecorderTask task;
  task.Type = RecorderTask::Type::SkippedKey;
  task.SkippedKey = key;
  m_ConcurrentQueue.push(task);
}

void OrderingRecorder::Close() {
  if (m_Closed) {
    return;
  }

  m_Closed = true;
  if (m_Thread.joinable()) {
    m_Thread.join();
  }
  m_Recorder->Close();

  gits::MessageBus::get().publish(
      {PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
      std::make_shared<StreamSavedMessage>(Configurator::Get().common.recorder.dumpPath.string()));
  gits::MessageBus::get().publish({PUBLISHER_RECORDER, TOPIC_END},
                                  std::make_shared<EndOfRecordingMessage>());
}

void OrderingRecorder::ConditionalFlush() {
  size_t processedBytes;
  size_t lastCheckedBytes = m_LastCheckedBytes;
  size_t scheduledBytes = m_ScheduledBytes;
  if (scheduledBytes - lastCheckedBytes > BYTES_CHECK_INTERVAL) {
    do {
      processedBytes = m_ProcessedBytes;
      scheduledBytes = m_ScheduledBytes;
      GITS_ASSERT(scheduledBytes >= processedBytes);
      if (scheduledBytes - processedBytes > BYTES_ALLOWED_DIFFERENCE) {
        std::this_thread::yield();
      }
    } while (scheduledBytes - processedBytes > BYTES_ALLOWED_DIFFERENCE);
    m_LastCheckedBytes.store(scheduledBytes);
  }
}

void OrderingRecorder::ProcessQueue() {
  const auto processTask = [this](const RecorderTask& task) {
    switch (task.Type) {
    case RecorderTask::Type::OrderedSerializer:
      OrderedRecord(task.Serializer);
      break;
    case RecorderTask::Type::SkippedKey:
      SkipImpl(task.SkippedKey);
      break;
    default:
      GITS_ASSERT(false && "Invalid task type");
    }
  };

  while (!m_Closed) {
    RecorderTask task;
    if (m_ConcurrentQueue.try_pop(task)) {
      processTask(task);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  for (RecorderTask task; m_ConcurrentQueue.try_pop(task);) {
    processTask(task);
  }
}

void OrderingRecorder::SkipImpl(size_t key) {
  if (key == m_NextKey) {
    UpdateNextKey();
    CheckPendingSerializers();
  } else {
    auto itKey = GetSkippedRangeIt(key - 1);
    if (itKey != m_SkippedKeyRanges.end()) {
      ++itKey->second.EndKey;
    } else {
      m_SkippedKeyRanges[key] = {key, key};
    }
  }
}

void OrderingRecorder::OrderedRecord(OrderedSerializer serializer) {
  m_ProcessedBytes += serializer.Serializer->Size();
  if (serializer.Key == m_NextKey) {
    m_Recorder->Record(*serializer.Serializer);
    delete serializer.Serializer;
    UpdateNextKey();
    CheckPendingSerializers();
  } else if (serializer.Key > m_NextKey &&
             GetSkippedRangeIt(serializer.Key) == m_SkippedKeyRanges.end()) {
    m_Serializers.push(serializer);
  } else {
    delete serializer.Serializer;
  }
}

void OrderingRecorder::UpdateNextKey() {
  while (true) {
    ++m_NextKey;
    if (m_SkippedKeyRanges.empty()) {
      break;
    }
    auto keyIt = GetSkippedRangeIt(m_NextKey);
    if (keyIt == m_SkippedKeyRanges.end()) {
      break;
    }

    if (m_NextKey == keyIt->second.StartKey) {
      m_NextKey = keyIt->second.EndKey;
      m_SkippedKeyRanges.erase(keyIt);
    }
  }
}

void OrderingRecorder::CheckPendingSerializers() {
  while (!m_Serializers.empty()) {
    if (m_Serializers.top().Key != m_NextKey) {
      break;
    }
    m_Recorder->Record(*m_Serializers.top().Serializer);
    delete m_Serializers.top().Serializer;
    m_Serializers.pop();
    UpdateNextKey();
  }
}

std::map<size_t, OrderingRecorder::KeyRange>::iterator OrderingRecorder::GetSkippedRangeIt(
    size_t key) {
  auto itKey = m_SkippedKeyRanges.upper_bound(key);
  if (itKey == m_SkippedKeyRanges.begin()) {
    return m_SkippedKeyRanges.end();
  }

  --itKey;
  if (itKey->second.StartKey <= key && itKey->second.EndKey >= key) {
    return itKey;
  } else {
    return m_SkippedKeyRanges.end();
  }
}

} // namespace stream
} // namespace gits
