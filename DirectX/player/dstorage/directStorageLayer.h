// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <wrl.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class PlayerManager;

class DirectStorageLayer : public Layer {
public:
  DirectStorageLayer();
  ~DirectStorageLayer();

  void Pre(IDStorageFactoryOpenFileCommand& c) override;
  void Post(IDStorageFactoryCreateQueueCommand& c) override;
  void Pre(IDStorageQueueEnqueueRequestCommand& c) override;
  void Pre(IDStorageQueueSubmitCommand& c) override;
  void Post(IDStorageQueueSubmitCommand& c) override;

  // Skip
  void Pre(IDStorageQueue1EnqueueSetEventCommand& c) override;
  void Pre(IDStorageCustomDecompressionQueueSetRequestResultsCommand& c) override;

private:
  using Buffer = std::vector<std::byte>;

  std::wstring m_ResourcesFilePath;
  std::unordered_map<unsigned, std::list<Buffer>> m_Buffers;
  std::unordered_map<unsigned, Microsoft::WRL::Wrappers::Event> m_Events;
};

} // namespace DirectX
} // namespace gits
