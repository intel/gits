// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <cstdint>
#include <vector>

namespace gits {
namespace DirectX {

// Timestamp queries for one command list recording, modelled on the timer in the
// gpu_command_benchmark plugin: query heaps grow geometrically on demand, the
// resolve is batched into one call per heap, and the readback buffer is mapped
// once per heap to collect the results.
//
// A timer belongs to a single recording of a single command list and is reused
// only after Reset, which is what lets slots be handed out as a plain sequence
// instead of a free list. That makes both the resolve and the readback operate
// on contiguous ranges by construction rather than by coincidence, and it means
// a slot is simply the position of its timestamp in the recording.
//
// Not thread safe: every method has to run on the thread recording the command
// list, except Gather, which may run anywhere once the GPU has finished
// executing that command list.
class GpuTimer {
public:
  using Tick = uint64_t;

  // Position of a timestamp within the recording, and therefore also its index
  // in the vector Gather returns.
  using Slot = unsigned;
  static constexpr Slot INVALID_SLOT = ~0u;

  // Records a timestamp into the command list. Returns INVALID_SLOT when no slot
  // could be allocated, which leaves the command list untouched and is the only
  // failure mode.
  Slot InsertTimestamp(ID3D12GraphicsCommandList* commandList);

  // Copies every timestamp recorded since Reset into its readback buffer. Has to
  // be the last thing recorded before the command list is closed.
  void InsertResolve(ID3D12GraphicsCommandList* commandList);

  // The resolved ticks in the order they were recorded, so Slot indexes it
  // directly. Only meaningful once the GPU has finished executing the command
  // list the resolve was recorded into; empty when the readback failed.
  std::vector<Tick> Gather() const;

  // Returns every slot to the pool, keeping the heaps for the next recording.
  void Reset();

  Slot Count() const {
    return m_Count;
  }

private:
  struct Heap {
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> Queries;
    Microsoft::WRL::ComPtr<ID3D12Resource> Readback;
    unsigned Capacity{};
    // Slots handed out since the last Reset, which is also the number of
    // timestamps to resolve and read back from this heap.
    unsigned Used{};
  };

  // The first heap covers a command list with a few hundred pipeline bindings,
  // which is the common case; each further one doubles.
  static constexpr unsigned FIRST_HEAP_CAPACITY = 0x100;
  // Bounds the memory a single timer can claim to roughly 4MB of query heap plus
  // as much readback. A recording that needs more is dropped by the caller.
  static constexpr size_t MAX_HEAPS = 8;

  bool Grow();

  bool m_Initialized{false};
  ID3D12Device* m_Device{};
  std::vector<Heap> m_Heaps;
  Slot m_Count{};
};

} // namespace DirectX
} // namespace gits
