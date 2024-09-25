// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

void MessageBus::unsubscribe(Topic topic, unsigned id) {
  auto isEqual = [id](const auto& subscription) { return subscription.first == id; };
  auto& subscriptions = subscribers_[topic];
  subscriptions.erase(std::remove_if(subscriptions.begin(), subscriptions.end(), isEqual),
                      subscriptions.end());
}

void MessageBus::publish(Topic topic, const MessagePtr& message) {
  auto it = subscribers_.find(topic);
  if (it != subscribers_.end()) {
    for (const auto& subscription : it->second) {
      subscription.second(topic, message);
    }
  }
}

} // namespace gits
