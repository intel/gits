// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directx/utils.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>
#include <plog/Log.h>

#include <cassert>

namespace directx {

void LoadIntelExtensions() {
  ComPtr<IDXGIFactory6> factory;
  HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
  assert(hr == S_OK);

  ComPtr<IDXGIAdapter1> adapter;
  hr = factory->EnumAdapters1(0, &adapter);
  assert(hr == S_OK);

  DXGI_ADAPTER_DESC1 desc{};
  hr = adapter->GetDesc1(&desc);
  assert(hr == S_OK);

  // If the extensions cannot be loaded (e.g. not an Intel GPU) just print a warning
  hr = INTC_LoadExtensionsLibrary(false, desc.VendorId, desc.DeviceId);
  if (SUCCEEDED(hr)) {
    LOG_INFO << "CCode - Loaded Intel Extensions (" << desc.Description << ")";
  } else {
    LOG_WARNING << "CCode - Failed to load Intel Extensions (" << desc.Description << ")";
  }
}

HMODULE LoadAgilitySdk(const std::filesystem::path& path) {
  std::string dllPath = (path / "D3D12Core.dll").string();
  auto hModule = LoadLibrary(dllPath.c_str());
  if (!hModule) {
    LOG_ERROR << "CCode - Failed to load Agility SDK (" << dllPath << ")";
    return hModule;
  }
  UINT sdkVersion = *reinterpret_cast<UINT*>(GetProcAddress(hModule, "D3D12SDKVersion"));

  Microsoft::WRL::ComPtr<ID3D12SDKConfiguration> sdkConfiguration;
  HRESULT hr = D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdkConfiguration));
  if (hr != S_OK) {
    LOG_ERROR << "CCode - Agility SDK: D3D12GetInterface(ID3D12SDKConfiguration) failed";
    return hModule;
  }

  hr = sdkConfiguration->SetSDKVersion(sdkVersion, path.string().c_str());
  if (hr != S_OK) {
    LOG_ERROR << "CCode - Agility SDK: SetSDKVersion call failed. This method can be used only in "
              << "Windows Developer Mode. Check settings !";
    return hModule;
  }

  LOG_INFO << "CCode - Loaded Agility SDK version (" << sdkVersion << ")";

  return hModule;
}

void PatchPipelineState(D3D12_PIPELINE_STATE_STREAM_DESC& desc,
                        ID3D12RootSignature* pRootSignature,
                        void* subobjectsData,
                        size_t subobjectsDataSize) {
  if (!pRootSignature) {
    return;
  }

  size_t stateOffset = 0;
  char* data = static_cast<char*>(subobjectsData);
  size_t dataOffset = 0;
  while (stateOffset < desc.SizeInBytes) {
    void* subobjectData = static_cast<char*>(desc.pPipelineStateSubobjectStream) + stateOffset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE: {
      auto* subobject =
          reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE*>(subobjectData);
      *subobject = pRootSignature;
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = data + dataOffset;
        dataOffset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(subobjectData);
      if (subobject->pSODeclaration) {
        subobject->pSODeclaration =
            reinterpret_cast<D3D12_SO_DECLARATION_ENTRY*>(data + dataOffset);
        dataOffset += subobject->NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

        auto* entries = const_cast<D3D12_SO_DECLARATION_ENTRY*>(subobject->pSODeclaration);
        for (unsigned i = 0; i < subobject->NumEntries; ++i) {
          entries[i].SemanticName = reinterpret_cast<LPCSTR>(data + dataOffset);
          dataOffset += strlen(entries[i].SemanticName) + 1;
        }
      }
      if (subobject->pBufferStrides) {
        subobject->pBufferStrides = reinterpret_cast<UINT*>(data + dataOffset);
        dataOffset += subobject->NumStrides * sizeof(UINT);
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      D3D12_INPUT_LAYOUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(subobjectData);
      if (subobject->pInputElementDescs) {
        subobject->pInputElementDescs =
            reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(data + dataOffset);
        dataOffset += subobject->NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC);

        auto* elements = const_cast<D3D12_INPUT_ELEMENT_DESC*>(subobject->pInputElementDescs);
        for (unsigned i = 0; i < subobject->NumElements; ++i) {
          elements[i].SemanticName = reinterpret_cast<LPCSTR>(data + dataOffset);
          dataOffset += strlen(elements[i].SemanticName) + 1;
        }
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      D3D12_CACHED_PIPELINE_STATE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(subobjectData);
      subobject->pCachedBlob = nullptr;
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      D3D12_VIEW_INSTANCING_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(subobjectData);
      if (subobject->pViewInstanceLocations) {
        subobject->pViewInstanceLocations =
            reinterpret_cast<D3D12_VIEW_INSTANCE_LOCATION*>(data + dataOffset);
        dataOffset += subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION);
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    default:
      assert(0 && "Unexpected subobject type");
      break;
    }
  }
}

void WaitForFence(ID3D12Fence* fence, uint64_t fenceValue) {
  UINT64 value = fence->GetCompletedValue();
  if (value >= fenceValue) {
    return;
  }

  HRESULT hr = fence->SetEventOnCompletion(fenceValue, nullptr);
  assert(hr == S_OK);
}

template <typename Desc, typename PipelineLibrary>
static HRESULT PreloadPipelineImpl(PipelineLibrary* pPipelineLibrary, LPCWSTR pName, Desc* pDesc) {
  ID3D12PipelineState* pPipelineState{};
  Microsoft::WRL::ComPtr<ID3D12Device2> pDevice;
  pPipelineLibrary->GetDevice(IID_PPV_ARGS(&pDevice));
  assert(pDevice);

  HRESULT hr = E_FAIL;
  if constexpr (std::is_same_v<Desc, const D3D12_GRAPHICS_PIPELINE_STATE_DESC>) {
    hr = pDevice->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(&pPipelineState));
  } else if constexpr (std::is_same_v<Desc, const D3D12_COMPUTE_PIPELINE_STATE_DESC>) {
    hr = pDevice->CreateComputePipelineState(pDesc, IID_PPV_ARGS(&pPipelineState));
  } else if constexpr (std::is_same_v<Desc, const D3D12_PIPELINE_STATE_STREAM_DESC>) {
    hr = pDevice->CreatePipelineState(pDesc, IID_PPV_ARGS(&pPipelineState));
  }
  assert(hr == S_OK);

  hr = pPipelineLibrary->StorePipeline(pName, pPipelineState);
  assert(hr == S_OK);

  return hr;
};

void PreloadComputePipeline(ID3D12PipelineLibrary* pPipelineLibrary,
                            LPCWSTR pName,
                            const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc) {
  PreloadPipelineImpl(pPipelineLibrary, pName, pDesc);
}

void PreloadGraphicsPipeline(ID3D12PipelineLibrary* pPipelineLibrary,
                             LPCWSTR pName,
                             const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc) {
  PreloadPipelineImpl(pPipelineLibrary, pName, pDesc);
}

void PreloadPipeline(ID3D12PipelineLibrary1* pPipelineLibrary,
                     LPCWSTR pName,
                     const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc) {
  PreloadPipelineImpl(pPipelineLibrary, pName, pDesc);
}

} // namespace directx
