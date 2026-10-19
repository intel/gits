// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrShaderSourceCache.h"
#include "rtsr/toolchain/rtsrShaderToolchain.h"
#include "rtsr/ui/rtsrEvents.h"
#include "baseContext.h"

#include <utility>

namespace gits::gui {

size_t RtsrShaderSourceCache::KeyHash::operator()(const Key& key) const {
  const size_t hash = std::hash<rtsr::RTSR_HASH>{}(key.Hash);
  const size_t entrypoint = std::hash<std::string>{}(key.EntrypointName);
  return hash ^ (entrypoint + 0x9e3779b9u + (hash << 6u) + (hash >> 2u));
}

RtsrShaderSourceCache::RtsrShaderSourceCache(const rtsr::SessionStore& store)
    : m_Store(store), m_Worker([this] { Run(); }) {}

RtsrShaderSourceCache::~RtsrShaderSourceCache() {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Stopping = true;
    m_Queue.clear();
  }
  m_Wake.notify_one();
  m_Worker.join();
}

RtsrShaderSourceCache::Entry RtsrShaderSourceCache::Request(rtsr::RTSR_HASH hash,
                                                            std::string_view entrypointName) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  Key key{hash, std::string(entrypointName)};
  const auto existing = m_Entries.find(key);
  if (existing != m_Entries.end()) {
    return existing->second;
  }

  auto blob = m_Store.Shader(hash, entrypointName);
  if (!blob) {
    Entry entry{State::Failed, std::make_shared<const rtsr::ShaderSource>(rtsr::ShaderSource{
                                   rtsr::SourceKind::Disassembly,
                                   {},
                                   "The shader bytecode is no longer in this session."})};
    m_Entries.emplace(std::move(key), entry);
    return entry;
  }

  m_Queue.push_back(Work{key, m_Generation, std::move(*blob),
                         BaseContext::GetBaseInstance().GetGITSPlayerPath()});
  m_Entries.emplace(std::move(key), Entry{});
  m_Wake.notify_one();
  return {};
}

void RtsrShaderSourceCache::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  ++m_Generation;
  m_Queue.clear();
  m_Entries.clear();
}

void RtsrShaderSourceCache::Run() {
  for (;;) {
    Work work;
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_Wake.wait(lock, [this] { return m_Stopping || !m_Queue.empty(); });
      if (m_Stopping) {
        return;
      }
      work = std::move(m_Queue.front());
      m_Queue.pop_front();
    }

    // No cache or store lock is held while third-party code runs.
    auto source =
        std::make_shared<const rtsr::ShaderSource>(rtsr::Decompile(work.Blob, work.PlayerPath));
    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      if (m_Stopping) {
        return;
      }
      if (work.Generation != m_Generation) {
        continue;
      }
      m_Entries.at(work.Shader) =
          Entry{source->Text.empty() ? State::Failed : State::Ready, std::move(source)};
    }

    // A late notification is harmless: it carries no result, and rendering
    // reads the current cache. Never call a widget from this worker.
    RtsrEvent event;
    event.EventType = RtsrEvent::Type::ShaderSourceReady;
    EventBus::GetInstance().publish<RtsrEvent>(event);
  }
}

} // namespace gits::gui
