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
      TopicSubscribers& subs = subscribers_[{static_cast<PublisherId>(PUBLISHER_PLAYER + i),
                                             static_cast<TopicId>(TOPIC_NONE + j)}];
      subs.Active.reserve(8);
      subs.Pending.reserve(8);
    }
  }
}

unsigned MessageBus::subscribe(Topic topic, SubscriberCb callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  const unsigned id = currentSubscriptionId_++;
  TopicSubscribers& subs = subscribers_[topic];
  (subs.Publications > 0 ? subs.Pending : subs.Active).push_back({id, std::move(callback)});
  return id;
}

void MessageBus::unsubscribe(unsigned id) {
  // A subscriber, e.g. one unloading plugins, may unsubscribe others while a topic is published.
  // The callback is destroyed now, while the module of its code is loaded, unless it runs.
  SubscriberCb callback;
  std::lock_guard<std::mutex> lock(mutex_);
  const auto [subs, subscription] = find(id);
  if (subscription == nullptr) {
    return;
  }
  subscription->Removed = true;
  if (subscription->Calls == 0) {
    callback.swap(subscription->Callback);
  }
  if (subs->Publications == 0) {
    compact(*subs);
  }
}

void MessageBus::publish(Topic topic, const MessagePtr& message) {
  TopicSubscribers* subs = nullptr;
  size_t count = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscribers_.find(topic);
    if (it == subscribers_.end() || it->second.Active.empty()) {
      return;
    }
    subs = &it->second;
    count = subs->Active.size();
    ++subs->Publications;
  }
  struct PublishScope {
    MessageBus& bus;
    TopicSubscribers& subs;
    ~PublishScope() {
      std::lock_guard<std::mutex> lock(bus.mutex_);
      if (--subs.Publications == 0) {
        compact(subs);
      }
    }
  } publishScope{*this, *subs};

  for (size_t i = 0; i < count; ++i) {
    Subscription& subscription = subs->Active[i];
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (subscription.Removed) {
        continue;
      }
      ++subscription.Calls;
    }
    struct CallScope {
      MessageBus& bus;
      Subscription& subscription;
      ~CallScope() {
        SubscriberCb callback;
        std::lock_guard<std::mutex> lock(bus.mutex_);
        if (--subscription.Calls == 0 && subscription.Removed) {
          callback.swap(subscription.Callback);
        }
      }
    } callScope{*this, subscription};
    subscription.Callback(topic, message);
  }
}

std::pair<TopicSubscribers*, Subscription*> MessageBus::find(unsigned id) {
  for (auto& [topic, subs] : subscribers_) {
    for (auto* list : {&subs.Active, &subs.Pending}) {
      for (Subscription& subscription : *list) {
        if (subscription.Id == id && !subscription.Removed) {
          return {&subs, &subscription};
        }
      }
    }
  }
  return {nullptr, nullptr};
}

void MessageBus::compact(TopicSubscribers& subs) {
  std::erase_if(subs.Active, [](const Subscription& subscription) { return subscription.Removed; });
  for (Subscription& subscription : subs.Pending) {
    if (!subscription.Removed) {
      subs.Active.push_back(std::move(subscription));
    }
  }
  subs.Pending.clear();
}

} // namespace gits
