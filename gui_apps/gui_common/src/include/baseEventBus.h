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

// GITS Common includes
#include "tools_lite.h"
#include "baseCommon.h"
namespace gits::gui {

struct Event {
  virtual ~Event() = default;
};

struct AppEvent : Event {
  enum class Type {
    ThemeChanged,
    UIScaleChanged,
    ModeChanged,
    ShowGITSLog,
    ShowAppLog
  };

  Type EventType;
};

struct ContextEvent : Event {
  enum class Type {
    ConfigValidated,
    CaptureAPIChanged,
    CLIUpdated,
    AppArgumentsChanged,
    MetadataLoaded,
    InMemoryConfigurationChanged,
    ConfigFileLoaded
  };

  Type EventType{};
  std::optional<gits::gui::GitsAction> Mode = std::nullopt;

  ContextEvent() {}

  ContextEvent(Type type, std::optional<gits::gui::GitsAction> gitsActionVal = std::nullopt)
      : EventType(type), Mode(gitsActionVal) {}
};

struct PathEvent : Event {
  using Type = Path;

  Type EventType;
  std::optional<gits::gui::GitsAction> Mode = std::nullopt;
  std::optional<std::filesystem::path> CustomPath = std::nullopt;

  PathEvent() {
    EventType = Type::GITS_BASE;
  }

  PathEvent(Path type,
            std::optional<gits::gui::GitsAction> gitsActionVal = std::nullopt,
            std::optional<std::filesystem::path> path = std::nullopt) {
    EventType = type;
    Mode = gitsActionVal;
    CustomPath = path;
  }
};

struct ActionEvent : Event {
  enum class Type {
    Capture,
    Playback,
    SubcaptureAnalysis,
    SubcaptureRecording,
    CCodeGeneration,
    StudioAnalysisExecuted,
    StudioAnalysisFinished
  };

  enum class State {
    Started,
    Ended
  };

  enum class Result {
    RESULT_UNKNOWN,
    RESULT_SUCCESS,
    RESULT_FAILURE
  };

  Type EventType;
  State ActionState;
  Result ActionResult = Result::RESULT_UNKNOWN;

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
    return *Current();
  }

  // Every binary has its own instance; a Studio module points its copy at Studio's bus before
  // it creates its panel.
  static void SetInstance(EventBus& bus) {
    Current() = &bus;
  }

  template <typename T>
  void subscribe(const EventHandler& handler) {
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    // NOTE: direct comparison of std::function objects is not generally
    // supported/portable. We accept duplicate handlers; callers that need
    // precise unsubscribe semantics should keep the returned token (future).
    AddSubscriber(std::type_index(typeid(T)), handler);
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
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    RemoveSubscriber(std::type_index(typeid(T)), handler);
  }

  template <typename T>
  void publish(const T& event) {
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    Enqueue(std::make_unique<T>(event));
  }

  template <typename T>
  void publish(typename EventType<T>::Type type) {
    static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
    T event;
    event.EventType = type;
    publish<T>(event);
  }

  void publish(ContextEvent::Type type, std::optional<GitsAction> gitsAction = std::nullopt) {
    publish<ContextEvent>(ContextEvent(type, gitsAction));
  }

  void publish(PathEvent::Type type, std::optional<GitsAction> gitsAction = std::nullopt) {
    publish<PathEvent>(PathEvent(type, gitsAction));
  }

  void publish(ActionEvent::Type type,
               ActionEvent::State state,
               ActionEvent::Result result,
               const std::string& details = "") {
    ActionEvent event;
    event.EventType = type;
    event.ActionState = state;
    event.ActionResult = result;
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

  static EventBus*& Current() {
    static EventBus instance;
    static EventBus* current = &instance;
    return current;
  }

  // A Studio module calls the templates above on Studio's bus, but every binary has its own C++
  // runtime. These are virtual so that only code of the binary that created the bus changes its
  // containers.
  virtual void AddSubscriber(std::type_index type, const EventHandler& handler) {
    std::lock_guard guard{m_EventBusMutex};
    subscribers_[type].push_back(handler);
  }

  virtual void RemoveSubscriber(std::type_index type, const EventHandler& handler) {
    std::lock_guard guard{m_EventBusMutex};
    auto it = subscribers_.find(type);
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

  virtual void Enqueue(std::unique_ptr<Event> event) {
    std::lock_guard guard{m_EventQueueMutex};
    m_EventQueue.push(std::move(event));
  }

  std::unordered_map<std::type_index, std::vector<EventHandler>> subscribers_;
  std::queue<std::unique_ptr<Event>> m_EventQueue;
  std::queue<std::unique_ptr<Event>>
      m_ProcessingQueue; // We need two queues to handle callbacks that publish new events
  std::mutex m_EventBusMutex;
  std::mutex m_EventQueueMutex;
};

} // namespace gits::gui
