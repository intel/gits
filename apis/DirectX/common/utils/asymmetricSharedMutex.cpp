// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "asymmetricSharedMutex.h"

#include <windows.h>

namespace gits {
namespace DirectX {

namespace {

std::atomic<uint64_t> g_NextMutexId{1};

class Backoff {
public:
  void operator()() {
    if (m_Count < m_SpinCount) {
      ++m_Count;
      YieldProcessor();
    } else if (!SwitchToThread()) {
      Sleep(0);
    }
  }

private:
  static constexpr unsigned m_SpinCount = 64;
  unsigned m_Count{};
};

} // namespace

thread_local AsymmetricSharedMutex::ThreadCacheEntry
    AsymmetricSharedMutex::t_Cache[m_ThreadCacheSize]{};
thread_local AsymmetricSharedMutex::ThreadSlotReleaser AsymmetricSharedMutex::t_SlotReleaser;

AsymmetricSharedMutex::ThreadSlotReleaser::~ThreadSlotReleaser() {
  for (ThreadCacheEntry& entry : t_Cache) {
    if (entry.Slot) {
      entry.Slot->OwnerThreadId.store(0, std::memory_order_release);
      ReleaseSlot(entry.Slot);
      entry = ThreadCacheEntry{};
    }
  }
}

AsymmetricSharedMutex::AsymmetricSharedMutex()
    : m_Id(g_NextMutexId.fetch_add(1, std::memory_order_relaxed)) {}

// Mutex ids are never reused, so a cache entry left by a destroyed mutex is never hit again and
// its reference is only dropped on eviction or thread exit
AsymmetricSharedMutex::~AsymmetricSharedMutex() {
  Slot* slot = m_Slots.load(std::memory_order_acquire);
  while (slot) {
    Slot* next = slot->Next;
    ReleaseSlot(slot);
    slot = next;
  }
}

void AsymmetricSharedMutex::ReleaseSlot(Slot* slot) {
  if (slot->References.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    delete slot;
  }
}

void AsymmetricSharedMutex::lock() {
  const uint32_t threadId = GetCurrentThreadId();
  // Only the owner stores its own id, so a relaxed load cannot match on any other thread
  if (m_WriterThreadId.load(std::memory_order_relaxed) == threadId) {
    ++m_WriterDepth;
    return;
  }

  m_WriterMutex.lock();
  m_WriterThreadId.store(threadId, std::memory_order_relaxed);
  m_WriterDepth = 1;
  m_WriterActive.store(true, std::memory_order_seq_cst);
  // Makes the flag visible to all readers and their slot updates visible to this thread
  FlushProcessWriteBuffers();

  for (Slot* slot = m_Slots.load(std::memory_order_acquire); slot; slot = slot->Next) {
    if (slot->OwnerThreadId.load(std::memory_order_relaxed) == threadId) {
      continue;
    }
    Backoff backoff;
    while (slot->Readers.load(std::memory_order_acquire) != 0) {
      backoff();
    }
  }
}

void AsymmetricSharedMutex::unlock() {
  if (--m_WriterDepth != 0) {
    return;
  }
  m_WriterThreadId.store(0, std::memory_order_relaxed);
  m_WriterActive.store(false, std::memory_order_release);
  m_WriterMutex.unlock();
}

AsymmetricSharedMutex::Slot* AsymmetricSharedMutex::AcquireThreadSlot() {
  const uint32_t threadId = GetCurrentThreadId();
  Slot* slot = nullptr;

  for (Slot* it = m_Slots.load(std::memory_order_acquire); it; it = it->Next) {
    if (it->OwnerThreadId.load(std::memory_order_relaxed) == threadId) {
      slot = it;
      break;
    }
  }
  if (!slot) {
    for (Slot* it = m_Slots.load(std::memory_order_acquire); it; it = it->Next) {
      uint32_t expected = 0;
      if (it->OwnerThreadId.compare_exchange_strong(expected, threadId,
                                                    std::memory_order_acquire)) {
        slot = it;
        break;
      }
    }
  }
  if (slot) {
    slot->References.fetch_add(1, std::memory_order_relaxed);
  } else {
    slot = new Slot();
    slot->OwnerThreadId.store(threadId, std::memory_order_relaxed);
    slot->References.store(2, std::memory_order_relaxed);
    Slot* head = m_Slots.load(std::memory_order_relaxed);
    do {
      slot->Next = head;
    } while (!m_Slots.compare_exchange_weak(head, slot, std::memory_order_release,
                                            std::memory_order_relaxed));
  }

  ThreadCacheEntry& entry = t_Cache[m_Id % m_ThreadCacheSize];
  if (entry.Slot) {
    ReleaseSlot(entry.Slot);
  }
  entry = ThreadCacheEntry{m_Id, slot};
  t_SlotReleaser.Active = true;
  return slot;
}

void AsymmetricSharedMutex::WaitForWriter(Slot* slot) {
  if (m_WriterThreadId.load(std::memory_order_relaxed) == GetCurrentThreadId()) {
    return;
  }
  do {
    slot->Readers.store(0, std::memory_order_release);
    // Exclusive sections are short, spinning is cheaper than sleeping and waking up all readers
    Backoff backoff;
    while (m_WriterActive.load(std::memory_order_acquire)) {
      backoff();
    }
    slot->Readers.store(1, std::memory_order_relaxed);
    std::atomic_signal_fence(std::memory_order_seq_cst);
  } while (m_WriterActive.load(std::memory_order_acquire));
}

} // namespace DirectX
} // namespace gits
