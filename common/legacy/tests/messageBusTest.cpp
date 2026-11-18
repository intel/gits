// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for the message bus: unsubscribing while a topic is published, by another
// subscriber, by the callback itself and by another thread, subscribing and publishing from a
// callback, subscribing to a topic while another is published, and publishing from several
// threads while others subscribe and unsubscribe.

#include "messageBus.h"

#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>

namespace {

using gits::MessageBus;
using gits::MessagePtr;
using gits::Topic;

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

const Topic kTopic{gits::PUBLISHER_PLAYER, gits::TOPIC_END};
const Topic kOtherTopic{gits::PUBLISHER_PLAYER, gits::TOPIC_FRAME_END};

void TestUnsubscribeOtherWhilePublishing() {
  MessageBus bus;
  auto token = std::make_shared<int>();
  std::weak_ptr<int> weakToken = token;
  int laterCalls = 0;
  bool removedCalled = false;
  bool destroyedOnUnsubscribe = false;
  unsigned removedId = 0;
  bus.subscribe(kTopic, [&](Topic, const MessagePtr&) {
    bus.unsubscribe(removedId);
    destroyedOnUnsubscribe = weakToken.expired();
  });
  removedId = bus.subscribe(kTopic, [&, token](Topic, const MessagePtr&) { removedCalled = true; });
  token.reset();
  bus.subscribe(kTopic, [&](Topic, const MessagePtr&) { ++laterCalls; });

  bus.publish(kTopic, nullptr);
  Check(!removedCalled, "a callback unsubscribed during the publication is not called");
  Check(destroyedOnUnsubscribe, "a callback that does not run is destroyed on unsubscribe");
  Check(laterCalls == 1, "subscribers after an unsubscribed one are still called");

  bus.publish(kTopic, nullptr);
  Check(laterCalls == 2, "the remaining subscribers are called by the next publication");
}

void TestUnsubscribeSelfWhilePublishing() {
  MessageBus bus;
  auto token = std::make_shared<int>(1);
  std::weak_ptr<int> weakToken = token;
  unsigned id = 0;
  int calls = 0;
  bool aliveAfterUnsubscribe = false;
  id = bus.subscribe(kTopic, [&, token](Topic, const MessagePtr&) {
    ++calls;
    bus.unsubscribe(id);
    aliveAfterUnsubscribe = !weakToken.expired() && *token == 1;
  });
  token.reset();

  bus.publish(kTopic, nullptr);
  Check(aliveAfterUnsubscribe, "a running callback is not destroyed by its own unsubscribe");
  Check(weakToken.expired(), "a callback unsubscribed while it runs is destroyed after it");
  bus.publish(kTopic, nullptr);
  Check(calls == 1, "an unsubscribed callback is not called again");
}

void TestUnsubscribeFromOtherThreadWhileRunning() {
  MessageBus bus;
  auto token = std::make_shared<int>();
  std::weak_ptr<int> weakToken = token;
  std::atomic<bool> running = false;
  std::atomic<bool> release = false;
  bool aliveAfterUnsubscribe = false;
  const unsigned id = bus.subscribe(kTopic, [&, token](Topic, const MessagePtr&) {
    running = true;
    while (!release) {
      std::this_thread::yield();
    }
    aliveAfterUnsubscribe = !weakToken.expired();
  });
  token.reset();

  std::thread publisher([&] { bus.publish(kTopic, nullptr); });
  while (!running) {
    std::this_thread::yield();
  }
  bus.unsubscribe(id);
  release = true;
  publisher.join();
  Check(aliveAfterUnsubscribe, "a callback running on another thread outlives its unsubscribe");
  Check(weakToken.expired(), "the callback is destroyed after the publication that runs it");
}

void TestSubscribeAndPublishFromCallback() {
  MessageBus bus;
  int otherCalls = 0;
  int addedCalls = 0;
  bool subscribed = false;
  bus.subscribe(kOtherTopic, [&](Topic, const MessagePtr&) { ++otherCalls; });
  bus.subscribe(kTopic, [&](Topic, const MessagePtr&) {
    if (!subscribed) {
      subscribed = true;
      bus.subscribe(kTopic, [&](Topic, const MessagePtr&) { ++addedCalls; });
    }
    bus.publish(kOtherTopic, nullptr);
  });

  bus.publish(kTopic, nullptr);
  Check(otherCalls == 1, "a callback publishes another topic");
  Check(addedCalls == 0, "a subscriber added during a publication is not called by it");
  bus.publish(kTopic, nullptr);
  Check(addedCalls == 1, "a subscriber added during a publication is called by the next one");
}

void TestSubscribeOtherTopicWhilePublishing() {
  MessageBus bus;
  int addedCalls = 0;
  bool subscribed = false;
  bus.subscribe(kTopic, [&](Topic, const MessagePtr&) {
    if (!subscribed) {
      subscribed = true;
      bus.subscribe(kOtherTopic, [&](Topic, const MessagePtr&) { ++addedCalls; });
    }
    bus.publish(kOtherTopic, nullptr);
  });

  bus.publish(kTopic, nullptr);
  Check(addedCalls == 1, "a subscriber to a topic not being published is called right away");
}

void TestSubscribeOtherTopicWhileAnotherThreadPublishes() {
  MessageBus bus;
  std::atomic<bool> running = false;
  std::atomic<bool> release = false;
  bus.subscribe(kTopic, [&](Topic, const MessagePtr&) {
    running = true;
    while (!release) {
      std::this_thread::yield();
    }
  });
  std::thread publisher([&] { bus.publish(kTopic, nullptr); });
  while (!running) {
    std::this_thread::yield();
  }

  int otherCalls = 0;
  bus.subscribe(kOtherTopic, [&](Topic, const MessagePtr&) { ++otherCalls; });
  bus.publish(kOtherTopic, nullptr);
  release = true;
  publisher.join();
  Check(otherCalls == 1, "a publication of another topic on another thread does not hold back a "
                         "subscription");
}

void TestConcurrentPublication() {
  MessageBus bus;
  constexpr int kPublishers = 4;
  constexpr int kPublications = 20000;
  std::atomic<int> calls = 0;
  std::atomic<int> nestedCalls = 0;
  std::atomic<bool> stop = false;
  bus.subscribe(kOtherTopic, [&](Topic, const MessagePtr&) { ++nestedCalls; });
  bus.subscribe(kTopic, [&](Topic, const MessagePtr&) {
    ++calls;
    bus.publish(kOtherTopic, nullptr);
  });

  std::thread churn([&] {
    while (!stop) {
      const unsigned id = bus.subscribe(kTopic, [](Topic, const MessagePtr&) {});
      const unsigned remover =
          bus.subscribe(kTopic, [&bus, id](Topic, const MessagePtr&) { bus.unsubscribe(id); });
      bus.unsubscribe(remover);
      bus.unsubscribe(id);
    }
  });
  std::vector<std::thread> publishers;
  for (int i = 0; i < kPublishers; ++i) {
    publishers.emplace_back([&] {
      for (int j = 0; j < kPublications; ++j) {
        bus.publish(kTopic, nullptr);
      }
    });
  }
  for (std::thread& publisher : publishers) {
    publisher.join();
  }
  stop = true;
  churn.join();
  Check(calls == kPublishers * kPublications, "every publication calls a lasting subscriber");
  Check(nestedCalls == kPublishers * kPublications, "every nested publication is delivered");
}

} // namespace

int main() {
  TestUnsubscribeOtherWhilePublishing();
  TestUnsubscribeSelfWhilePublishing();
  TestUnsubscribeFromOtherThreadWhileRunning();
  TestSubscribeAndPublishFromCallback();
  TestSubscribeOtherTopicWhilePublishing();
  TestSubscribeOtherTopicWhileAnotherThreadPublishes();
  TestConcurrentPublication();
  if (g_Failures != 0) {
    std::printf("%d failure(s)\n", g_Failures);
    return 1;
  }
  std::printf("All message bus tests passed\n");
  return 0;
}
