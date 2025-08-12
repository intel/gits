// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingShaderPatchService.h"
#include "gits.h"
#include "log2.h"
#include "config.h"

#include <string>
#include <vector>
#include <dxcapi.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

RaytracingShaderPatchService::RaytracingShaderPatchService() {
  dxilDll_ = LoadLibrary(".\\D3D12\\dxil.dll");
  GITS_ASSERT(dxilDll_);
  dxcDll_ = LoadLibrary(".\\D3D12\\dxcompiler.dll");
  GITS_ASSERT(dxcDll_);
}

RaytracingShaderPatchService::~RaytracingShaderPatchService() {
  FreeLibrary(dxcDll_);
  FreeLibrary(dxilDll_);
}

void RaytracingShaderPatchService::patchInstances(ID3D12GraphicsCommandList* commandList,
                                                  D3D12_GPU_VIRTUAL_ADDRESS instancesBuffer,
                                                  unsigned instancesCount,
                                                  D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
                                                  D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer) {
  if (!instancesPipelineState_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);

    initializeInstances(device.Get());
  }

  commandList->SetComputeRootSignature(instancesRootSignature_);
  commandList->SetPipelineState(instancesPipelineState_);

  commandList->SetComputeRootUnorderedAccessView(0, instancesBuffer);
  commandList->SetComputeRootShaderResourceView(1, gpuAddressBuffer);
  commandList->SetComputeRootConstantBufferView(2, mappingCountBuffer);

  commandList->Dispatch(instancesCount, 1, 1);
}

void RaytracingShaderPatchService::initializeInstances(ID3D12Device* device) {
  {
    D3D12_ROOT_SIGNATURE_DESC desc{};
    D3D12_ROOT_PARAMETER parameters[3]{};
    desc.NumParameters = 3;
    desc.pParameters = parameters;
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    parameters[0].Descriptor.ShaderRegister = 0;
    parameters[0].Descriptor.RegisterSpace = 0;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[1].Descriptor.ShaderRegister = 0;
    parameters[1].Descriptor.RegisterSpace = 0;
    parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[2].Descriptor.ShaderRegister = 0;
    parameters[2].Descriptor.RegisterSpace = 0;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    HRESULT hr =
        D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    GITS_ASSERT(hr == S_OK);
    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                     IID_PPV_ARGS(&instancesRootSignature_));
    GITS_ASSERT(hr == S_OK);
  }

  std::string cs =
      R"(struct InstanceDesc
{
  uint64_t padding[7];
  uint64_t blas;
};
RWStructuredBuffer<InstanceDesc> instances : register(u0);

struct GpuAddressMapping
{
  uint64_t captureStart;
  uint64_t playerStart;
  uint64_t size;
};
StructuredBuffer<GpuAddressMapping> mappings : register(t0);

cbuffer MappingCount : register(b0)
{
  uint gpuAddressCount;
  uint shaderIdentiferCount;
  uint viewDescriptorCount;
  uint sampleDescriptorCount;
};

[numthreads(1, 1, 1)]
void gits_patch(uint3 gId : SV_GroupID, uint3 dtId : SV_DispatchThreadID, 
                uint3 gtId : SV_GroupThreadID, uint gi : SV_GroupIndex)
{
  uint instancesIndex = dtId.x;
  uint64_t captureAddress = instances[instancesIndex].blas;
  int first = 0;
  int last = gpuAddressCount - 1;
  while (first <= last) {
    int mid = first + (last - first) / 2;
    GpuAddressMapping mapping = mappings[mid];
    if (captureAddress >= mapping.captureStart && captureAddress <
        mapping.captureStart + mapping.size) {
      uint64_t offset = captureAddress - mapping.captureStart;
      uint64_t playbackAddress = mapping.playerStart + offset;
      instances[dtId.x].blas = playbackAddress;
      break;
    } else if (captureAddress >= mapping.captureStart + mapping.size) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
})";

  initializePipelineState(cs, device, instancesRootSignature_, &instancesPipelineState_);
  instancesPipelineState_->SetName(L"GitsPatchInstances_CS");
}

void RaytracingShaderPatchService::patchInstancesOffset(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS instancesBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS instancesOffsetsBuffer,
    unsigned instancesCount,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer) {

  if (!instancesOffsetPipelineState_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);

    initializeInstancesOffset(device.Get());
  }

  commandList->SetComputeRootSignature(instancesOffsetRootSignature_);
  commandList->SetPipelineState(instancesOffsetPipelineState_);

  commandList->SetComputeRootUnorderedAccessView(0, instancesBuffer);
  commandList->SetComputeRootShaderResourceView(1, instancesOffsetsBuffer);
  commandList->SetComputeRootShaderResourceView(2, gpuAddressBuffer);
  commandList->SetComputeRootConstantBufferView(3, mappingCountBuffer);

  commandList->Dispatch(instancesCount, 1, 1);
}

void RaytracingShaderPatchService::initializeInstancesOffset(ID3D12Device* device) {
  D3D12_ROOT_SIGNATURE_DESC desc{};
  D3D12_ROOT_PARAMETER parameters[4]{};
  desc.NumParameters = 4;
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
  parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[3].Descriptor.ShaderRegister = 0;
  parameters[3].Descriptor.RegisterSpace = 0;

  Microsoft::WRL::ComPtr<ID3DBlob> signature;
  Microsoft::WRL::ComPtr<ID3DBlob> error;
  HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  GITS_ASSERT(hr == S_OK);
  hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                   IID_PPV_ARGS(&instancesOffsetRootSignature_));
  GITS_ASSERT(hr == S_OK);

  std::string cs =
      R"(struct InstanceDesc
{
  uint64_t padding[7];
  uint64_t blas;
};
RWStructuredBuffer<InstanceDesc> instances : register(u0);

StructuredBuffer<uint> instancesOffsets : register(t0);

struct GpuAddressMapping
{
  uint64_t captureStart;
  uint64_t playerStart;
  uint64_t size;
};
StructuredBuffer<GpuAddressMapping> mappings : register(t1);

cbuffer MappingCount : register(b0)
{
  uint gpuAddressCount;
  uint shaderIdentiferCount;
  uint viewDescriptorCount;
  uint sampleDescriptorCount;
};

[numthreads(1, 1, 1)]
void gits_patch(uint3 gId : SV_GroupID, uint3 dtId : SV_DispatchThreadID, 
                uint3 gtId : SV_GroupThreadID, uint gi : SV_GroupIndex)
{
  uint instancesIndex = instancesOffsets[dtId.x];
  uint64_t captureAddress = instances[instancesIndex].blas;
  int first = 0;
  int last = gpuAddressCount - 1;
  while (first <= last) {
    int mid = first + (last - first) / 2;
    GpuAddressMapping mapping = mappings[mid];
    if (captureAddress >= mapping.captureStart && captureAddress <
        mapping.captureStart + mapping.size) {
      uint64_t offset = captureAddress - mapping.captureStart;
      uint64_t playbackAddress = mapping.playerStart + offset;
      instances[instancesIndex].blas = playbackAddress;
      break;
    } else if (captureAddress >= mapping.captureStart + mapping.size) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
})";

  initializePipelineState(cs, device, instancesOffsetRootSignature_,
                          &instancesOffsetPipelineState_);
  instancesOffsetPipelineState_->SetName(L"GitsPatchInstancesOffset_CS");
}

void RaytracingShaderPatchService::patchBindingTable(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS bindingTableBuffer,
    unsigned recordCount,
    unsigned recordSize,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddressBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS shaderIdentiferBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS viewDescriptorBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS sampleDescriptorBuffer,
    D3D12_GPU_VIRTUAL_ADDRESS mappingCountBuffer) {
  if (!bindingTablePipelineState_) {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT hr = commandList->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(hr == S_OK);

    initializeBindingTable(device.Get());
  }

  commandList->SetComputeRootSignature(bindingTableRootSignature_);
  commandList->SetPipelineState(bindingTablePipelineState_);

  commandList->SetComputeRootUnorderedAccessView(0, bindingTableBuffer);
  commandList->SetComputeRootShaderResourceView(1, gpuAddressBuffer);
  commandList->SetComputeRootShaderResourceView(2, shaderIdentiferBuffer);
  commandList->SetComputeRootShaderResourceView(3, viewDescriptorBuffer);
  commandList->SetComputeRootShaderResourceView(4, sampleDescriptorBuffer);
  commandList->SetComputeRootConstantBufferView(5, mappingCountBuffer);
  unsigned size = recordSize / sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
  commandList->SetComputeRoot32BitConstant(6, size, 0);

  commandList->Dispatch(recordCount, 1, 1);
}

void RaytracingShaderPatchService::initializeBindingTable(ID3D12Device* device) {
  {
    D3D12_ROOT_SIGNATURE_DESC desc{};
    D3D12_ROOT_PARAMETER parameters[7]{};
    desc.NumParameters = 7;
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
    parameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    parameters[4].Descriptor.ShaderRegister = 3;
    parameters[4].Descriptor.RegisterSpace = 0;
    parameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[5].Descriptor.ShaderRegister = 0;
    parameters[5].Descriptor.RegisterSpace = 0;
    parameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[6].Constants.Num32BitValues = 1;
    parameters[6].Constants.ShaderRegister = 1;
    parameters[6].Constants.RegisterSpace = 0;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    HRESULT hr =
        D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    GITS_ASSERT(hr == S_OK);
    hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                     IID_PPV_ARGS(&bindingTableRootSignature_));
    GITS_ASSERT(hr == S_OK);
  }

  std::string cs =
      R"(RWStructuredBuffer<uint64_t> bindingTable : register(u0);

struct Mapping
{
  uint64_t captureStart;
  uint64_t playerStart;
  uint64_t size;
};
StructuredBuffer<Mapping> gpuAddressMappings : register(t0);

struct ShaderIdentifierMapping
{
  uint64_t4 captureIdentifier;
  uint64_t4 playerIdentifier;
};
StructuredBuffer<ShaderIdentifierMapping> shaderIdentifierMappings : register(t1);

StructuredBuffer<Mapping> viewDescriptorMappings : register(t2);
StructuredBuffer<Mapping> samplerDescriptorMappings : register(t3);

cbuffer MappingCount : register(b0)
{
  uint gpuAddressCount;
  uint shaderIdentiferCount;
  uint viewDescriptorCount;
  uint sampleDescriptorCount;
};

cbuffer ConstantBuffer : register(b1)
{
  uint recordSize;
};

[numthreads(1, 1, 1)]
void gits_patch(uint3 gId : SV_GroupID, uint3 dtId : SV_DispatchThreadID, 
                uint3 gtId : SV_GroupThreadID, uint gi : SV_GroupIndex)
{
  uint bindingTableOffset = dtId.x * recordSize;
  uint64_t4 captureIdentifier;
  captureIdentifier.x = bindingTable[bindingTableOffset];
  captureIdentifier.y = bindingTable[bindingTableOffset + 1];
  captureIdentifier.z = bindingTable[bindingTableOffset + 2];
  captureIdentifier.w = bindingTable[bindingTableOffset + 3];
  for (uint i = 0; i < shaderIdentiferCount; ++i) {
    if (shaderIdentifierMappings[i].captureIdentifier.x == captureIdentifier.x
        && shaderIdentifierMappings[i].captureIdentifier.y == captureIdentifier.y
        && shaderIdentifierMappings[i].captureIdentifier.z == captureIdentifier.z
        && shaderIdentifierMappings[i].captureIdentifier.w == captureIdentifier.w) {
      bindingTable[bindingTableOffset] = shaderIdentifierMappings[i].playerIdentifier.x;
      bindingTable[bindingTableOffset + 1] =
          shaderIdentifierMappings[i].playerIdentifier.y;
      bindingTable[bindingTableOffset + 2] =
          shaderIdentifierMappings[i].playerIdentifier.z;
      bindingTable[bindingTableOffset + 3] =
          shaderIdentifierMappings[i].playerIdentifier.w;
      break;
    }
  }
  for (uint i = 4; i < recordSize; ++i) {
    uint64_t captureAddress = bindingTable[bindingTableOffset + i];
    bool found = false;
    if (viewDescriptorCount) {
      int first = 0;
      int last = viewDescriptorCount - 1;
      while (first <= last) {
        int mid = first + (last - first) / 2;
        Mapping mapping = viewDescriptorMappings[mid];
        if (captureAddress >= mapping.captureStart && captureAddress <
            mapping.captureStart + mapping.size) {
          uint64_t offset = captureAddress - mapping.captureStart;
          uint64_t playbackAddress = mapping.playerStart + offset;
          bindingTable[bindingTableOffset + i] = playbackAddress;
          found = true;
          break;
        } else if (captureAddress >= mapping.captureStart + mapping.size) {
          first = mid + 1;
        } else {
          last = mid - 1;
        }
      }
    }
    if (!found && sampleDescriptorCount) {
      int first = 0;
      int last = sampleDescriptorCount - 1;
      while (first <= last) {
        int mid = first + (last - first) / 2;
        Mapping mapping = samplerDescriptorMappings[mid];
        if (captureAddress >= mapping.captureStart && captureAddress <
            mapping.captureStart + mapping.size) {
          uint64_t offset = captureAddress - mapping.captureStart;
          uint64_t playbackAddress = mapping.playerStart + offset;
          bindingTable[bindingTableOffset + i] = playbackAddress;
          found = true;
          break;
        } else if (captureAddress >= mapping.captureStart + mapping.size) {
          first = mid + 1;
        } else {
          last = mid - 1;
        }
      }
    }
    if (!found) {
      int first = 0;
      int last = gpuAddressCount - 1;
      while (first <= last) {
        int mid = first + (last - first) / 2;
        Mapping mapping = gpuAddressMappings[mid];
        if (captureAddress >= mapping.captureStart && captureAddress <
            mapping.captureStart + mapping.size) {
          uint64_t offset = captureAddress - mapping.captureStart;
          uint64_t playbackAddress = mapping.playerStart + offset;
          bindingTable[bindingTableOffset + i] = playbackAddress;
          found = true;
          break;
        } else if (captureAddress >= mapping.captureStart + mapping.size) {
          first = mid + 1;
        } else {
          last = mid - 1;
        }
      }
    }
  }
})";

  initializePipelineState(cs, device, bindingTableRootSignature_, &bindingTablePipelineState_);
  bindingTablePipelineState_->SetName(L"GitsPatchBindingTable_CS");
}

void RaytracingShaderPatchService::initializePipelineState(const std::string& shaderCode,
                                                           ID3D12Device* device,
                                                           ID3D12RootSignature* rootSignature,
                                                           ID3D12PipelineState** pipelineState) {
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
  hr = utils->CreateBlob(shaderCode.c_str(), shaderCode.length(), CP_ACP, &shaderBlob);
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
  desc.pRootSignature = rootSignature;
  desc.CS = D3D12_SHADER_BYTECODE{shader->GetBufferPointer(), shader->GetBufferSize()};

  hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(pipelineState));
  GITS_ASSERT(hr == S_OK);
}

} // namespace DirectX
} // namespace gits
