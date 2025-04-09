// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"
#include "log.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace gits {
enum PublisherId {
  PUBLISHER_PLAYER = 0,
  PUBLISHER_RECORDER,
  PUBLISHER_PLUGIN
};
enum TopicId {
  TOPIC_NONE = 0,
  TOPIC_LOG,
  TOPIC_END,
  TOPIC_STREAM_SAVED,
  TOPIC_CLOSE_RECORDER,
  TOPIC_GITS_WORKLOAD_BEGIN,
  TOPIC_GITS_WORKLOAD_END
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
using Subscription = std::pair<unsigned, SubscriberCb>;

class MessageBus : gits::noncopyable {
public:
  MessageBus() = default;
  ~MessageBus() = default;

  unsigned subscribe(Topic topic, SubscriberCb callback);
  void unsubscribe(Topic topic, unsigned id);
  void publish(Topic topic, const MessagePtr& message);

private:
  unsigned currentSubscriptionId_ = 0;
  std::unordered_map<Topic, std::vector<Subscription>> subscribers_;
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

} // namespace gits
