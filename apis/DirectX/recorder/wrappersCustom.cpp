// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "wrappersAuto.h"

#include "captureManager.h"
#include "interfaceArgumentUpdaters.h"
#include "commandsAuto.h"
#include "layerAuto.h"

#include <processthreadsapi.h>

namespace gits {
namespace DirectX {

D3D12_HEAP_DESC ID3D12HeapWrapper::GetDesc() {
  D3D12_HEAP_DESC result{};

  auto& manager = CaptureManager::Get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    ID3D12HeapGetDescCommand command(GetCurrentThreadId(), reinterpret_cast<ID3D12Heap*>(this));

    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.CreateCommandKey();
    if (!command.Skip) {
      result = command.m_Object.Value->GetDesc();
    }

    command.m_Result.Value = result;

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }

    // Post layers may restore the heap properties requested by the application
    result = command.m_Result.Value;
  } else {
    result = GetWrappedObject<ID3D12Heap>()->GetDesc();
  }

  return result;
}

} // namespace DirectX
} // namespace gits
