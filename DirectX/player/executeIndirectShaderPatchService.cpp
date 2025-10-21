// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executeIndirectShaderPatchService.h"
#include "gits.h"
#include "log.h"

#include <wrl/client.h>
#include <dxcapi.h>

namespace gits {
namespace DirectX {

ExecuteIndirectShaderPatchService::ExecuteIndirectShaderPatchService() {
  dxilDll_ = LoadLibrary(".\\D3D12\\dxil.dll");
  GITS_ASSERT(dxilDll_);
  dxcDll_ = LoadLibrary(".\\D3D12\\dxcompiler.dll");
  GITS_ASSERT(dxcDll_);
}

ExecuteIndirectShaderPatchService ::~ExecuteIndirectShaderPatchService() {
  FreeLibrary(dxcDll_);
  FreeLibrary(dxilDll_);
}

void ExecuteIndirectShaderPatchService::patchArgumentBuffer(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS argumentBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS countBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS patchOffsetsBuffer,
    unsigned patchOffsetsCount,
    D3D12_GPU_VIRTUAL_ADDRESS raytracingPatchBuffer,
    unsigned raytracingPatchCount,
    unsigned maxArgumentCount,
    unsigned commandStride,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer) {

  if (!pipelineState_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);

    initialize(device.Get());
  }

  commandList->SetComputeRootSignature(rootSignature_);
  commandList->SetPipelineState(pipelineState_);

  commandList->SetComputeRootUnorderedAccessView(0, argumentBuffer);
  commandList->SetComputeRootShaderResourceView(1, patchOffsetsBuffer);
  commandList->SetComputeRootShaderResourceView(2, raytracingPatchBuffer);
  commandList->SetComputeRootShaderResourceView(3, gpuAddressBuffer);
  commandList->SetComputeRootConstantBufferView(4, mappingCountBuffer);
  commandList->SetComputeRootConstantBufferView(5, countBuffer);
  commandList->SetComputeRoot32BitConstant(6, maxArgumentCount, 0);
  commandList->SetComputeRoot32BitConstant(7, commandStride, 0);
  commandList->SetComputeRoot32BitConstant(8, patchOffsetsCount, 0);
  commandList->SetComputeRoot32BitConstant(9, raytracingPatchCount, 0);

  commandList->Dispatch(1, 1, 1);
}

void ExecuteIndirectShaderPatchService::initialize(ID3D12Device* device) {
  {
    D3D12_ROOT_SIGNATURE_DESC desc{};
    D3D12_ROOT_PARAMETER parameters[10]{};
    desc.NumParameters = 10;
    desc.pParameters = parameters;
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    parameters[0].Descriptor.ShaderRegister = 0;
    parameters[0].Descriptor.RegisterSpace = 0;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[1].Descriptor.ShaderRegister = 0;
    parameters[1].Descriptor.RegisterSpace = 0;
    parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[2].Descriptor.ShaderRegister = 1;
    parameters[2].Descriptor.RegisterSpace = 0;
    parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[3].Descriptor.ShaderRegister = 2;
    parameters[3].Descriptor.RegisterSpace = 0;
    parameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[4].Descriptor.ShaderRegister = 0;
    parameters[4].Descriptor.RegisterSpace = 0;
    parameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[5].Descriptor.ShaderRegister = 1;
    parameters[5].Descriptor.RegisterSpace = 0;
    parameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[6].Constants.Num32BitValues = 1;
    parameters[6].Constants.ShaderRegister = 2;
    parameters[6].Constants.RegisterSpace = 0;
    parameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[7].Constants.Num32BitValues = 1;
    parameters[7].Constants.ShaderRegister = 3;
    parameters[7].Constants.RegisterSpace = 0;
    parameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[8].Constants.Num32BitValues = 1;
    parameters[8].Constants.ShaderRegister = 4;
    parameters[8].Constants.RegisterSpace = 0;
    parameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[9].Constants.Num32BitValues = 1;
    parameters[9].Constants.ShaderRegister = 5;
    parameters[9].Constants.RegisterSpace = 0;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    HRESULT hr =
        D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    GITS_ASSERT(hr == S_OK);
    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                     IID_PPV_ARGS(&rootSignature_));
    GITS_ASSERT(hr == S_OK);
  }

  std::string cs =
      R"(RWByteAddressBuffer argumentBuffer : register(u0);

StructuredBuffer<uint> patchOffsets : register(t0);

struct RaytracingAddressMapping
{
  uint64_t captureAddress;
  uint64_t playerAddress;
};
StructuredBuffer<RaytracingAddressMapping> raytracingMappings : register(t1);

struct GpuAddressMapping
{
  uint64_t captureStart;
  uint64_t playerStart;
  uint64_t size;
};
StructuredBuffer<GpuAddressMapping> mappings : register(t2);

cbuffer MappingCount : register(b0)
{
  uint gpuAddressCount;
  uint shaderIdentiferCount;
  uint viewDescriptorCount;
  uint sampleDescriptorCount;
};

cbuffer ArgumentCount : register(b1)
{
  uint argumentCount;
};

cbuffer MaxArgumentCount : register(b2)
{
  uint maxArgumentCount;
};

cbuffer CommandStride : register(b3)
{
  uint commandStride;
};

cbuffer PatchOffsetsCount : register(b4)
{
  uint patchOffsetsCount;
};

cbuffer RaytracingPatchCount : register(b5)
{
  uint raytracingPatchCount;
};


[numthreads(1, 1, 1)]
void gits_patch(uint3 gId : SV_GroupID, uint3 dtId : SV_DispatchThreadID, 
                uint3 gtId : SV_GroupThreadID, uint gi : SV_GroupIndex)
{
  for (uint i = 0; i < patchOffsetsCount; ++i) {
    uint commandOffset = patchOffsets[i];
    uint count = argumentCount;
    if (count > maxArgumentCount) {
      count = maxArgumentCount;
    }
    for (uint j = 0; j < count; ++j) {
      uint bufferOffset = j * commandStride + commandOffset;
      uint2 lowHigh = argumentBuffer.Load2(bufferOffset);
      uint64_t captureAddress = lowHigh[1];
      captureAddress <<= 32;
      captureAddress += lowHigh[0];

      bool patched = false;
      for (uint k = 0; k < raytracingPatchCount; ++k) {
        RaytracingAddressMapping mapping = raytracingMappings[k];
        if (captureAddress == mapping.captureAddress) {
          uint2 lowHigh;
          lowHigh[0] = mapping.playerAddress & 0x00000000FFFFFFFF;
          lowHigh[1] = mapping.playerAddress >> 32;
          argumentBuffer.Store2(bufferOffset, lowHigh);
          patched = true;
          break;
        }
      }
      if (!patched) {
        int first = 0;
        int last = gpuAddressCount - 1;
        while (first <= last) {
          int mid = first + (last - first) / 2;
          GpuAddressMapping mapping = mappings[mid];
          if (captureAddress >= mapping.captureStart && captureAddress <
              mapping.captureStart + mapping.size) {
            uint64_t offset = captureAddress - mapping.captureStart;
            uint64_t playbackAddress = mapping.playerStart + offset;

            uint2 lowHigh;
            lowHigh[0] = playbackAddress & 0x00000000FFFFFFFF;
            lowHigh[1] = playbackAddress >> 32;
            argumentBuffer.Store2(bufferOffset, lowHigh);

            break;
          } else if (captureAddress >= mapping.captureStart + mapping.size) {
            first = mid + 1;
          } else {
            last = mid - 1;
          }
        }
      }
    }
  }
})";

  {
    auto dxcCreateInstanceFn = (DxcCreateInstanceProc)GetProcAddress(dxcDll_, "DxcCreateInstance");
    GITS_ASSERT(dxcCreateInstanceFn);

    Microsoft::WRL::ComPtr<IDxcUtils> utils;
    HRESULT hr = dxcCreateInstanceFn(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    GITS_ASSERT(hr == S_OK);
    Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
    hr = dxcCreateInstanceFn(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    GITS_ASSERT(hr == S_OK);

    std::vector<const WCHAR*> arguments;
    arguments.push_back(L"-E");
    arguments.push_back(L"gits_patch");
    arguments.push_back(L"-T");
    arguments.push_back(L"cs_6_0");
    if (Configurator::Get().directx.player.debugLayer) {
      arguments.push_back(L"-Zi");
      arguments.push_back(L"-Qembed_debug");
    }

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderBlob;
    hr = utils->CreateBlob(cs.c_str(), cs.length(), CP_ACP, &shaderBlob);
    GITS_ASSERT(hr == S_OK);

    DxcBuffer shaderBuffer{};
    shaderBuffer.Ptr = shaderBlob->GetBufferPointer();
    shaderBuffer.Size = shaderBlob->GetBufferSize();
    shaderBuffer.Encoding = 0;

    Microsoft::WRL::ComPtr<IDxcResult> compileResult;
    hr = compiler->Compile(&shaderBuffer, arguments.data(), arguments.size(), nullptr,
                           IID_PPV_ARGS(&compileResult));
    Microsoft::WRL::ComPtr<IDxcBlob> shader;
    hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr);
    GITS_ASSERT(hr == S_OK);

    if (!shader->GetBufferPointer()) {
      Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
      hr = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
      GITS_ASSERT(hr == S_OK);
      std::string error = static_cast<char*>(errors->GetBufferPointer());
      LOG_ERROR << "Shader compilation for SBT failed: " + error;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_;
    desc.CS = D3D12_SHADER_BYTECODE{shader->GetBufferPointer(), shader->GetBufferSize()};

    hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
    GITS_ASSERT(hr == S_OK);
  }
}

} // namespace DirectX
} // namespace gits
