// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <typeindex>
#include <any>
#include <filesystem>
#include <queue>
#include <mutex>

#include "common.h"

// GITS Common includes
#include "tools_lite.h"

namespace gits::gui {

struct Event {
  virtual ~Event() = default;
};

struct AppEvent : Event {
  enum class Type {
    ThemeChanged,
    UIScaleChanged,
    ModeChanged
  };

  Type EventType;
};

struct ContextEvent : Event {
  enum class Type {
    ConfigEdited,
    ConfigValidated,
    CaptureAPIChanged,
    CLIUpdated,
    AppArgumentsChanged,
    PluginsUpdated,
    MetadataLoaded
  };

  Type EventType;
};

struct PathEvent : Event {
  using Type = Path;

  Type EventType;
  std::optional<Mode> Mode = std::nullopt;
  std::optional<std::filesystem::path> CustomPath = std::nullopt;

  PathEvent() {
    EventType = Type::GITS_BASE;
  }

  PathEvent(Path type,
            std::optional<gits::gui::Mode> mode = std::nullopt,
            std::optional<std::filesystem::path> path = std::nullopt) {
    EventType = type;
    Mode = mode;
    CustomPath = path;
  }
};

struct ActionEvent : Event {
  enum class Type {
    Capture,
    Playback,
    SubcaptureAnalysis,
    SubcaptureRecording,
    CCodeGeneration
  };

  enum class State {
    Started,
    Ended
  };

  enum class Status {
    Unknown,
    Success,
    Failure
  };

  Type EventType;
  State ActionState;
  Status ActionStatus = Status::Unknown;

  std::string Details;
};

struct FileDropEvent : Event {
  FileDropEvent(const std::filesystem::path& path) : FilePath(path) {}

  std::filesystem::path FilePath;
};

// Type trait to extract Type type from event classes
template <typename T>
struct EventType;

template <>
struct EventType<AppEvent> {
  using Type = AppEvent::Type;
};

template <>
struct EventType<ContextEvent> {
  using Type = ContextEvent::Type;
};

template <>
struct EventType<PathEvent> {
  using Type = PathEvent::Type;
};

template <>
struct EventType<ActionEvent> {
  using Type = ActionEvent::Type;
};

class EventBus : gits::noncopyable {
public:
  using EventHandler = std::function<void(const Event&)>;

  // Singleton
  static EventBus& GetInstance() {
    static EventBus instance;

    return instance;
  }

  template <typename T>
  void subscribe(const EventHandler& handler) {
    std::lock_guard guard{m_EventBusMutex};
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    auto it = subscribers_.find(std::type_index(typeid(T)));
    // NOTE: direct comparison of std::function objects is not generally
    // supported/portable. We accept duplicate handlers; callers that need
    // precise unsubscribe semantics should keep the returned token (future).
    subscribers_[std::type_index(typeid(T))].push_back(handler);
  }

  // Generic subscribe with type filtering
  template <typename T>
  void subscribe(const EventHandler& handler,
                 const std::vector<typename EventType<T>::Type>& types) {
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");

    const auto sharedTypes = std::make_shared<std::vector<typename EventType<T>::Type>>(types);
    auto filteredHandler = [handler, sharedTypes](const Event& e) {
      const T& typedEvent = static_cast<const T&>(e);
      if (std::find(sharedTypes->begin(), sharedTypes->end(), typedEvent.EventType) !=
          sharedTypes->end()) {
        handler(e);
      }
    };
    subscribe<T>(filteredHandler);
  }

  template <typename T>
  void unsubscribe(const EventHandler& handler) {
    std::lock_guard guard{m_EventBusMutex};
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    auto it = subscribers_.find(std::type_index(typeid(T)));
    if (it != subscribers_.end()) {
      auto& handlers = it->second;
      handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                                    [&](const EventHandler& h) {
                                      // best-effort: compare target type and function pointer target
                                      if (h.target_type() != handler.target_type()) {
                                        return false;
                                      }
                                      auto pa = h.template target<void (*)(const Event&)>();
                                      auto pb = handler.template target<void (*)(const Event&)>();
                                      if (pa && pb) {
                                        return *pa == *pb;
                                      }
                                      return false;
                                    }),
                     handlers.end());
    }
  }

  template <typename T>
  void publish(const T& event) {
    std::lock_guard guard{m_EventQueueMutex};
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    m_EventQueue.push(std::make_unique<T>(event));
  }

  template <typename T>
  void publish(typename EventType<T>::Type type) {
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    T event;
    event.EventType = type;
    publish<T>(event);
  }

  void publish(PathEvent::Type type, std::optional<Mode> mode = std::nullopt) {
    publish<PathEvent>(PathEvent(type, mode));
  }

  void publish(ActionEvent::Type type,
               ActionEvent::State state,
               ActionEvent::Status status,
               const std::string& details = "") {
    ActionEvent event;
    event.EventType = type;
    event.ActionState = state;
    event.ActionStatus = status;
    event.Details = details;
    publish<ActionEvent>(event);
  }

  void processEvents() {
    std::lock_guard guard{m_EventBusMutex};

    // Before processing the events, we move them to a "temporary processing queue"
    {
      std::lock_guard eventQueueGuard{m_EventQueueMutex};
      m_ProcessingQueue.swap(m_EventQueue);
    }

    while (!m_ProcessingQueue.empty()) {
      const auto& event = m_ProcessingQueue.front();
      auto it = subscribers_.find(std::type_index(typeid(*event)));
      if (it != subscribers_.end()) {
        for (const auto& handler : it->second) {
          handler(*event);
        }
      }
      m_ProcessingQueue.pop();
    }
  }

private:
  EventBus(){};
  std::unordered_map<std::type_index, std::vector<EventHandler>> subscribers_;
  std::queue<std::unique_ptr<Event>> m_EventQueue;
  std::queue<std::unique_ptr<Event>>
      m_ProcessingQueue; // We need two queues to handle callbacks that publish new events
  std::mutex m_EventBusMutex;
  std::mutex m_EventQueueMutex;
};

} // namespace gits::gui
