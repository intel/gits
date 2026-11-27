// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

namespace gits {
namespace DirectX {

// Reader-writer mutex for very frequent shared locking on many threads and rare exclusive locking.
// A reader only writes its own per-thread slot (no shared cache line, no interlocked operation).
// The writer pays instead: a process-wide memory barrier and a wait for all reader slots to drain.
// Both locks are reentrant, and the exclusive owner may also lock shared, so callbacks invoked while
// holding the exclusive lock can use the mutex freely.
class AsymmetricSharedMutex {
public:
  AsymmetricSharedMutex();
  ~AsymmetricSharedMutex();
  AsymmetricSharedMutex(const AsymmetricSharedMutex&) = delete;
  AsymmetricSharedMutex& operator=(const AsymmetricSharedMutex&) = delete;

  void lock_shared() {
    Slot* slot = GetThreadSlot();
    uint32_t depth = slot->Readers.load(std::memory_order_relaxed);
    slot->Readers.store(depth + 1, std::memory_order_relaxed);
    if (depth != 0) {
      return;
    }
    // Only a compiler barrier, the store-load ordering is enforced by the writer
    std::atomic_signal_fence(std::memory_order_seq_cst);
    if (m_WriterActive.load(std::memory_order_acquire)) [[unlikely]] {
      WaitForWriter(slot);
    }
  }

  void unlock_shared() {
    Slot* slot = GetThreadSlot();
    slot->Readers.store(slot->Readers.load(std::memory_order_relaxed) - 1,
                        std::memory_order_release);
  }

  void lock();
  void unlock();

private:
  // Referenced by the mutex and by the thread cache entry pointing to it, so the slot outlives
  // whichever of the mutex and the owner thread goes away first
  struct alignas(64) Slot {
    std::atomic<uint32_t> Readers{0};
    std::atomic<uint32_t> OwnerThreadId{0};
    std::atomic<uint32_t> References{0};
    Slot* Next{};
  };

  // Direct-mapped by mutex id. Mutexes sharing an entry stay correct but take the slow path,
  // and an evicted slot stays owned by its thread until the mutex is destroyed.
  struct ThreadCacheEntry {
    uint64_t MutexId;
    Slot* Slot;
  };
  static constexpr unsigned m_ThreadCacheSize = 16;

  struct ThreadSlotReleaser {
    bool Active{};
    ~ThreadSlotReleaser();
  };

  Slot* GetThreadSlot() {
    const ThreadCacheEntry& entry = t_Cache[m_Id % m_ThreadCacheSize];
    return entry.MutexId == m_Id ? entry.Slot : AcquireThreadSlot();
  }
  Slot* AcquireThreadSlot();
  void WaitForWriter(Slot* slot);
  static void ReleaseSlot(Slot* slot);

  // Read by every reader, so kept apart from the fields written by writers and new threads
  alignas(64) const uint64_t m_Id;
  std::atomic<bool> m_WriterActive{false};
  alignas(64) std::atomic<Slot*> m_Slots{nullptr};
  std::mutex m_WriterMutex;
  std::atomic<uint32_t> m_WriterThreadId{0};
  unsigned m_WriterDepth{};
  static thread_local ThreadCacheEntry t_Cache[m_ThreadCacheSize];
  static thread_local ThreadSlotReleaser t_SlotReleaser;
};

} // namespace DirectX
} // namespace gits
