// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "debugHelperLayer.h"
#include "configurationLib.h"

#include <sstream>

namespace gits {
namespace DirectX {

void DebugHelperLayer::setD3D12ObjectName(void* obj, unsigned key) {
  ID3D12Object* object = static_cast<ID3D12Object*>(obj);
  std::wstringstream s;
  s << "O" << key;
  object->SetName(s.str().c_str());
}

void DebugHelperLayer::setDXGIObjectName(void* obj, unsigned key) {
  IDXGIObject* object = static_cast<IDXGIObject*>(obj);
  std::stringstream ss;
  ss << "O" << key;
  std::string s = ss.str();
  object->SetPrivateData(WKPDID_D3DDebugObjectName, s.size(), s.c_str());
}

DebugHelperLayer::DebugHelperLayer()
    : Layer("DebugHelper"),
      multithreadedShaderCompilation_(
          Configurator::Get().directx.player.multithreadedShaderCompilation) {}

void DebugHelperLayer::pre(ID3D12ObjectSetNameCommand& c) {
  c.skip = true;
}

void DebugHelperLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}
void DebugHelperLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvResource_.value, c.ppvResource_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvHeap_.value, c.ppvHeap_.key);
}

void DebugHelperLayer::post(ID3D12Device4CreateHeap1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvHeap_.value, c.ppvHeap_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvHeap_.value, c.ppvHeap_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppCommandList_.value, c.ppCommandList_.key);
}

void DebugHelperLayer::post(ID3D12Device4CreateCommandList1Command& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppCommandList_.value, c.ppCommandList_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppvRootSignature_.value, c.ppvRootSignature_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppCommandAllocator_.value, c.ppCommandAllocator_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppCommandQueue_.value, c.ppCommandQueue_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppPipelineState_.value, c.ppPipelineState_.key);
}

void DebugHelperLayer::post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppPipelineState_.value, c.ppPipelineState_.key);
}

void DebugHelperLayer::post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppPipelineState_.value, c.ppPipelineState_.key);
}

void DebugHelperLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppStateObject_.value, c.ppStateObject_.key);
}

void DebugHelperLayer::post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppPipelineState_.value, c.ppPipelineState_.key);
}

void DebugHelperLayer::post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppPipelineState_.value, c.ppPipelineState_.key);
}

void DebugHelperLayer::post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (c.result_.value != S_OK || multithreadedShaderCompilation_) {
    return;
  }
  setD3D12ObjectName(*c.ppPipelineState_.value, c.ppPipelineState_.key);
}

void DebugHelperLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setDXGIObjectName(*c.ppSwapChain_.value, c.ppSwapChain_.key);
}

void DebugHelperLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setDXGIObjectName(*c.ppSwapChain_.value, c.ppSwapChain_.key);
}

void DebugHelperLayer::post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setDXGIObjectName(*c.ppSwapChain_.value, c.ppSwapChain_.key);
}

void DebugHelperLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  setD3D12ObjectName(*c.ppSurface_.value, c.ppSurface_.key);
}

} // namespace DirectX
} // namespace gits
