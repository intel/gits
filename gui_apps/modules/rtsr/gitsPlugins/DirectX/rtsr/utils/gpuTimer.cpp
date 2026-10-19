// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuTimer.h"

#include "log.h"

#include <cstring>

namespace gits {
namespace DirectX {

GpuTimer::Slot GpuTimer::InsertTimestamp(ID3D12GraphicsCommandList* commandList) {
  if (commandList == nullptr) {
    return INVALID_SLOT;
  }

  if (!m_Initialized) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if (FAILED(commandList->GetDevice(IID_PPV_ARGS(&device)))) {
      return INVALID_SLOT;
    }
    // The device outlives every command list created from it, so the reference
    // is not held: keeping one would keep the device alive past the point where
    // the stream expects it to be gone.
    m_Device = device.Get();
    m_Initialized = true;
  }

  Heap* heap = nullptr;
  for (auto& candidate : m_Heaps) {
    if (candidate.Used < candidate.Capacity) {
      heap = &candidate;
      break;
    }
  }
  if (heap == nullptr) {
    if (!Grow()) {
      return INVALID_SLOT;
    }
    heap = &m_Heaps.back();
  }

  // D3D12 timestamp queries are written by EndQuery alone; there is no matching
  // BeginQuery for this query type.
  commandList->EndQuery(heap->Queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, heap->Used);
  ++heap->Used;
  return m_Count++;
}

void GpuTimer::InsertResolve(ID3D12GraphicsCommandList* commandList) {
  if (commandList == nullptr) {
    return;
  }
  // One call per heap rather than per timestamp, which is what makes the
  // sequential slot allocation worth having.
  for (const auto& heap : m_Heaps) {
    if (heap.Used == 0) {
      continue;
    }
    commandList->ResolveQueryData(heap.Queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, heap.Used,
                                  heap.Readback.Get(), 0);
  }
}

std::vector<GpuTimer::Tick> GpuTimer::Gather() const {
  std::vector<Tick> ticks;
  ticks.reserve(m_Count);

  for (const auto& heap : m_Heaps) {
    if (heap.Used == 0) {
      continue;
    }
    void* mapped = nullptr;
    if (FAILED(heap.Readback->Map(0, nullptr, &mapped)) || mapped == nullptr) {
      LOG_WARNING << "Rtsr - could not read back GPU timestamps, this frame's timings are dropped";
      return {};
    }
    const size_t offset = ticks.size();
    ticks.resize(offset + heap.Used);
    std::memcpy(ticks.data() + offset, mapped, heap.Used * sizeof(Tick));
    heap.Readback->Unmap(0, nullptr);
  }

  return ticks;
}

void GpuTimer::Reset() {
  for (auto& heap : m_Heaps) {
    heap.Used = 0;
  }
  m_Count = 0;
}

bool GpuTimer::Grow() {
  if (m_Device == nullptr || m_Heaps.size() >= MAX_HEAPS) {
    return false;
  }

  Heap heap;
  heap.Capacity = FIRST_HEAP_CAPACITY << m_Heaps.size();

  D3D12_QUERY_HEAP_DESC queryDesc{};
  queryDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
  queryDesc.Count = heap.Capacity;
  queryDesc.NodeMask = 0;
  if (FAILED(m_Device->CreateQueryHeap(&queryDesc, IID_PPV_ARGS(&heap.Queries)))) {
    LOG_WARNING << "Rtsr - could not create a timestamp query heap, timings will be incomplete";
    return false;
  }

  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
  heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask = 1;
  heapProperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = static_cast<UINT64>(heap.Capacity) * sizeof(Tick);
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  if (FAILED(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&heap.Readback)))) {
    LOG_WARNING
        << "Rtsr - could not create a timestamp readback buffer, timings will be incomplete";
    return false;
  }

  heap.Queries->SetName(L"RTSR timestamp queries");
  heap.Readback->SetName(L"RTSR timestamp readback");

  m_Heaps.push_back(std::move(heap));
  return true;
}

} // namespace DirectX
} // namespace gits
