// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "include/messageBus.h"

namespace gits {

MessageBus::MessageBus() {
  // workaround for reallocations from plugins dlls
  for (unsigned i = 0; i < PUBLISHER_COUNT; ++i) {
    for (unsigned j = 0; j < TOPIC_COUNT; ++j) {
      subscribers_[{static_cast<PublisherId>(PUBLISHER_PLAYER + i),
                    static_cast<TopicId>(TOPIC_NONE + j)}]
          .reserve(8);
    }
  }
}

unsigned MessageBus::subscribe(Topic topic, SubscriberCb callback) {
  unsigned id = currentSubscriptionId_++;
  subscribers_[topic].emplace_back(id, callback);
  return id;
}

void MessageBus::unsubscribe(unsigned id) {
  for (auto& [topic, subs] : subscribers_) {
    for (auto it = subs.begin(); it != subs.end(); ++it) {
      if (it->first == id) {
        subs.erase(it);
        return;
      }
    }
  }
}

void MessageBus::publish(Topic topic, const MessagePtr& message) {
  auto it = subscribers_.find(topic);
  if (it != subscribers_.end()) {
    for (const auto& sub : it->second) {
      sub.second(topic, message);
    }
  }
}

} // namespace gits
