// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "debugHelperLayer.h"
#include "configurationLib.h"

#include <sstream>

namespace gits {
namespace DirectX {

void DebugHelperLayer::SetD3D12ObjectName(void* object, unsigned key) {
  ID3D12Object* d3d12Object = static_cast<ID3D12Object*>(object);
  std::wstringstream s;
  s << "O" << key;
  d3d12Object->SetName(s.str().c_str());
}

void DebugHelperLayer::SetDXGIObjectName(void* object, unsigned key) {
  IDXGIObject* dxgiObject = static_cast<IDXGIObject*>(object);
  std::stringstream ss;
  ss << "O" << key;
  std::string s = ss.str();
  dxgiObject->SetPrivateData(WKPDID_D3DDebugObjectName, s.size(), s.c_str());
}

DebugHelperLayer::DebugHelperLayer()
    : Layer("DebugHelper"),
      m_MultithreadedShaderCompilation(
          Configurator::Get().directx.player.multithreadedShaderCompilation) {}

void DebugHelperLayer::Pre(ID3D12ObjectSetNameCommand& c) {
  c.Skip = true;
}

void DebugHelperLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}
void DebugHelperLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvResource.Value, c.m_ppvResource.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvHeap.Value, c.m_ppvHeap.Key);
}

void DebugHelperLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvHeap.Value, c.m_ppvHeap.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvHeap.Value, c.m_ppvHeap.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateCommandListCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppCommandList.Value, c.m_ppCommandList.Key);
}

void DebugHelperLayer::Post(ID3D12Device4CreateCommandList1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppCommandList.Value, c.m_ppCommandList.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppvRootSignature.Value, c.m_ppvRootSignature.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppCommandAllocator.Value, c.m_ppCommandAllocator.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppCommandQueue.Value, c.m_ppCommandQueue.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppPipelineState.Value, c.m_ppPipelineState.Key);
}

void DebugHelperLayer::Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppPipelineState.Value, c.m_ppPipelineState.Key);
}

void DebugHelperLayer::Post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppPipelineState.Value, c.m_ppPipelineState.Key);
}

void DebugHelperLayer::Post(ID3D12Device5CreateStateObjectCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppStateObject.Value, c.m_ppStateObject.Key);
}

void DebugHelperLayer::Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppPipelineState.Value, c.m_ppPipelineState.Key);
}

void DebugHelperLayer::Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppPipelineState.Value, c.m_ppPipelineState.Key);
}

void DebugHelperLayer::Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || m_MultithreadedShaderCompilation) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppPipelineState.Value, c.m_ppPipelineState.Key);
}

void DebugHelperLayer::Post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetDXGIObjectName(*c.m_ppSwapChain.Value, c.m_ppSwapChain.Key);
}

void DebugHelperLayer::Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetDXGIObjectName(*c.m_ppSwapChain.Value, c.m_ppSwapChain.Key);
}

void DebugHelperLayer::Post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetDXGIObjectName(*c.m_ppSwapChain.Value, c.m_ppSwapChain.Key);
}

void DebugHelperLayer::Post(IDXGISwapChainGetBufferCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  SetD3D12ObjectName(*c.m_ppSurface.Value, c.m_ppSurface.Key);
}

} // namespace DirectX
} // namespace gits
