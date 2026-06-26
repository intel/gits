// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <dstorage.h>

#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <wrl/event.h>
#include <windows.h>

namespace directx {

class DirectStorageService {
public:
  static DirectStorageService& Get();

  LPCWSTR ResourcesFilePath();

  void PrepareEnqueueRequest(unsigned queueKey, DSTORAGE_REQUEST* request, uint64_t newOffset);
  void OnSubmit(unsigned queueKey, IDStorageQueue1* queue);
  void CompleteAllBatches();

private:
  using Buffer = std::vector<std::byte>;

  struct Batch {
    Microsoft::WRL::Wrappers::Event CompletionEvent;
    std::list<Buffer> Buffers;
  };

  void ClearCompletedBatches(unsigned queueKey);

private:
  std::wstring m_ResourcesFilePath{};
  std::unordered_map<unsigned, std::list<Buffer>> m_Buffers;
  std::unordered_map<unsigned, std::deque<std::unique_ptr<Batch>>> m_InflightBatches;
};

} // namespace directx
