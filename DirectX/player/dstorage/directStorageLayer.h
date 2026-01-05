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

  void pre(IDStorageFactoryOpenFileCommand& c) override;
  void post(IDStorageFactoryCreateQueueCommand& c) override;
  void pre(IDStorageQueueEnqueueRequestCommand& c) override;
  void pre(IDStorageQueueSubmitCommand& c) override;
  void post(IDStorageQueueSubmitCommand& c) override;

  // Skip
  void pre(IDStorageQueue1EnqueueSetEventCommand& c) override;
  void pre(IDStorageCustomDecompressionQueueSetRequestResultsCommand& c) override;

private:
  using Buffer = std::vector<std::byte>;

  std::wstring resourcesFilePath_;
  std::unordered_map<unsigned, std::list<Buffer>> buffers_;
  std::unordered_map<unsigned, Microsoft::WRL::Wrappers::Event> events_;
};

} // namespace DirectX
} // namespace gits
