// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "include/messageBus.h"

namespace gits {

unsigned MessageBus::subscribe(Topic topic, SubscriberCb callback) {
  unsigned id = currentSubscriptionId_++;
  subscribers_[topic].emplace_back(id, callback);
  return id;
}

void MessageBus::unsubscribe(unsigned id) {
  auto isEqual = [id](const auto& sub) { return sub.first == id; };
  std::vector<Topic> topicsToRemove;
  // Remove all subscriptions with the given id
  for (auto& [topic, subs] : subscribers_) {
    subs.erase(std::remove_if(subs.begin(), subs.end(), isEqual), subs.end());
    if (subs.empty()) {
      topicsToRemove.push_back(topic);
    }
  }
  // Remove all empty topics
  for (const auto& topic : topicsToRemove) {
    subscribers_.erase(topic);
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
