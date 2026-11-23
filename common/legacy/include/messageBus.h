// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"
#include "log.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
namespace gits {
enum PublisherId {
  PUBLISHER_PLAYER = 0,
  PUBLISHER_RECORDER,
  PUBLISHER_PLUGIN,
  PUBLISHER_COUNT
};
enum TopicId {
  TOPIC_NONE = 0,
  TOPIC_LOG,
  TOPIC_END,
  TOPIC_STREAM_SAVED,
  TOPIC_CLOSE_RECORDER,
  TOPIC_CLOSE_PLAYER, // Stops playback at the current token
  TOPIC_GITS_WORKLOAD_BEGIN,
  TOPIC_GITS_WORKLOAD_END,
  TOPIC_PROGRAM_START,
  TOPIC_PROGRAM_EXIT,
  TOPIC_SKIP_XESS,
  // A statistics plugin wrote its statistics file, see StatisticsWrittenMessage
  TOPIC_STATISTICS_WRITTEN,
  // The player replayed a frame end of the stream, see FrameEndMessage
  TOPIC_FRAME_END,
  TOPIC_COUNT
};
struct Topic {
  PublisherId publisherId{};
  TopicId topicId{};

  bool operator==(const Topic& other) const {
    return publisherId == other.publisherId && topicId == other.topicId;
  }
};
} // namespace gits

namespace std {
template <>
struct hash<gits::Topic> {
  std::size_t operator()(const gits::Topic& topic) const {
    std::size_t h1 = std::hash<gits::PublisherId>{}(topic.publisherId);
    std::size_t h2 = std::hash<unsigned>{}(topic.topicId);
    return h1 ^ (h2 << 1);
  }
};
} // namespace std

namespace gits {
class Message;
using MessagePtr = std::shared_ptr<Message>;
using SubscriberCb = std::function<void(Topic, const MessagePtr&)>;

struct Subscription {
  unsigned Id{};
  SubscriberCb Callback;
  // Publications running the callback. An unsubscribed callback is destroyed when the last of
  // them returns.
  unsigned Calls{};
  bool Removed{};
};

struct TopicSubscribers {
  std::vector<Subscription> Active;
  // Publications of the topic running. They call the callbacks in Active without the bus locked,
  // so Active does not change while any of them runs.
  unsigned Publications{};
  // Subscriptions to the topic made while it is published, added to Active after the publications.
  std::vector<Subscription> Pending;
};

// Callbacks run without the bus locked, so they may publish, subscribe and unsubscribe, and
// several threads may publish at once.
class MessageBus : gits::noncopyable {
public:
  static MessageBus& get() {
    static MessageBus instance;
    return instance;
  }
  MessageBus();

  unsigned subscribe(Topic topic, SubscriberCb callback);
  void unsubscribe(unsigned id);
  void publish(Topic topic, const MessagePtr& message);

private:
  std::pair<TopicSubscribers*, Subscription*> find(unsigned id);
  static void compact(TopicSubscribers& subs);

  std::mutex mutex_;
  unsigned currentSubscriptionId_ = 0;
  std::unordered_map<Topic, TopicSubscribers> subscribers_;
};

class Message : gits::noncopyable {
public:
  Message() = default;
  virtual ~Message() = default;
};

class LogMessage : public Message {
public:
  LogMessage(LogLevel level, const std::string& text) : level_(level), text_(text) {}
  LogMessage(LogLevel level, const std::ostringstream& os) : level_(level), text_(os.str()) {}

  // Variadic template constructor for formatted messages
  template <typename... Args>
  LogMessage(LogLevel level, Args&&... args)
      : level_(level), text_(fold(std::forward<Args>(args)...)) {}

  LogLevel getLevel() const {
    return level_;
  }
  const std::string& getText() const {
    return text_;
  }

private:
  LogLevel level_{};
  std::string text_{};

  template <typename... Args>
  static std::string fold(Args&&... args) {
    std::ostringstream stream;
    (stream << ... << args); // Fold expression (C++17)
    return stream.str();
  }
};

class EndOfRecordingMessage : public Message {
public:
  EndOfRecordingMessage() {}
};

class StreamSavedMessage : public Message {
public:
  StreamSavedMessage(const std::string& path) : path_(path) {}

  const std::string& getPath() const {
    return path_;
  }

private:
  std::string path_;
};

class CloseRecorderMessage : public Message {
public:
  CloseRecorderMessage() {}
};

// Brackets GPU commands issued by GITS (not part of the original workload)
// context: API command list pointer (e.g., ID3D12GraphicsCommandList*)
// value: Stream command list key (use 0 when the list is not a stream object)
class GitsWorkloadMessage : public Message {
public:
  GitsWorkloadMessage(void* context, const std::string& label, unsigned value)
      : context_(context), label_(label), value_(value) {}

  void* getContext() const {
    return context_;
  }

  const std::string& getLabel() const {
    return label_;
  }

  unsigned getValue() const {
    return value_;
  }

private:
  void* context_;
  std::string label_;
  unsigned value_;
};

class ProgramMessage : public Message {
public:
  ProgramMessage() {}
};

struct StatisticsSummary {
  unsigned ApiFunctionsNum{};
  unsigned FramesNum{};
  unsigned CallsNum{};
  unsigned StateInitCallsNum{};
};

// The summary is that of the API alone.
class StatisticsWrittenMessage : public Message {
public:
  StatisticsWrittenMessage(const std::string& api,
                           const std::string& path,
                           const StatisticsSummary& summary)
      : api_(api), path_(path), summary_(summary) {}

  const std::string& getApi() const {
    return api_;
  }
  const std::string& getPath() const {
    return path_;
  }
  const StatisticsSummary& getSummary() const {
    return summary_;
  }

private:
  std::string api_;
  std::string path_;
  StatisticsSummary summary_;
};

class FrameEndMessage : public Message {
public:
  explicit FrameEndMessage(unsigned frameNumber) : frameNumber_(frameNumber) {}

  unsigned getFrameNumber() const {
    return frameNumber_;
  }

private:
  unsigned frameNumber_{};
};

} // namespace gits
