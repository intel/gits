// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rootSignatureDump.h"
#include "gits.h"
#include "to_string/toStr.h"
#include <wrl.h>

namespace gits {
namespace DirectX {

void RootSignatureDump::DeserializeRootSignature(const void* pBlobWithRootSignature,
                                                 size_t blobLengthInBytes,
                                                 const std::wstring& dumpName) {
  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> pDeserializer;
  HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(
      pBlobWithRootSignature, blobLengthInBytes, IID_PPV_ARGS(&pDeserializer));
  GITS_ASSERT(hr == S_OK);
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pDesc =
      pDeserializer->GetUnconvertedRootSignatureDesc();

  SaveRootSignature(pDesc, dumpName);
}

void RootSignatureDump::SaveRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pDesc,
                                          const std::wstring& dumpName) {
  std::ofstream dumpFile(dumpName);
  if (pDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_0) {
    dumpFile << "RootSignatureVersion: 1.0\n";
    dumpFile << pDesc->Desc_1_0;
  } else if (pDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1) {
    dumpFile << "RootSignatureVersion: 1.1\n";
    dumpFile << pDesc->Desc_1_1;
  } else if (pDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_2) {
    dumpFile << "RootSignatureVersion: 1.2\n";
    dumpFile << pDesc->Desc_1_2;
  }
  dumpFile.close();
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_DESCRIPTOR_RANGE& arg) {
  ofs << "      RangeType: " << toStr(arg.RangeType) << "\n";
  ofs << "      NumDescriptors: "
      << ((arg.NumDescriptors == UINT_MAX) ? "UINT_MAX" : std::to_string(arg.NumDescriptors))
      << "\n";
  ofs << "      BaseShaderRegister: " << arg.BaseShaderRegister << "\n";
  ofs << "      RegisterSpace: " << arg.RegisterSpace << "\n";
  ofs << "      OffsetInDescriptorsFromTableStart: "
      << ((arg.OffsetInDescriptorsFromTableStart == D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
              ? "D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND"
              : std::to_string(arg.OffsetInDescriptorsFromTableStart))
      << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_DESCRIPTOR_RANGE1& arg) {
  ofs << "      RangeType: " << toStr(arg.RangeType) << "\n";
  ofs << "      NumDescriptors: "
      << ((arg.NumDescriptors == UINT_MAX) ? "UINT_MAX" : std::to_string(arg.NumDescriptors))
      << "\n";
  ofs << "      BaseShaderRegister: " << arg.BaseShaderRegister << "\n";
  ofs << "      RegisterSpace: " << arg.RegisterSpace << "\n";
  ofs << "      Flags: " << toStr(arg.Flags) << "\n";
  ofs << "      OffsetInDescriptorsFromTableStart: "
      << ((arg.OffsetInDescriptorsFromTableStart == D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
              ? "D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND"
              : std::to_string(arg.OffsetInDescriptorsFromTableStart))
      << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR_TABLE& arg) {
  ofs << "    NumDescriptorRanges: " << arg.NumDescriptorRanges << "\n";
  for (UINT i = 0; i < arg.NumDescriptorRanges; ++i) {
    ofs << "    DescriptorRange[" << i << "]:\n";
    ofs << arg.pDescriptorRanges[i];
  }
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR_TABLE1& arg) {
  ofs << "    NumDescriptorRanges: " << arg.NumDescriptorRanges << "\n";
  for (UINT i = 0; i < arg.NumDescriptorRanges; ++i) {
    ofs << "    DescriptorRange[" << i << "]:\n";
    ofs << arg.pDescriptorRanges[i];
  }
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_CONSTANTS& arg) {
  ofs << "    ShaderRegister: " << arg.ShaderRegister << "\n";
  ofs << "    RegisterSpace: " << arg.RegisterSpace << "\n";
  ofs << "    Num32BitValues: " << arg.Num32BitValues << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR& arg) {
  ofs << "    ShaderRegister: " << arg.ShaderRegister << "\n";
  ofs << "    RegisterSpace: " << arg.RegisterSpace << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR1& arg) {
  ofs << "    ShaderRegister: " << arg.ShaderRegister << "\n";
  ofs << "    RegisterSpace: " << arg.RegisterSpace << "\n";
  ofs << "    Flags: " << toStr(arg.Flags) << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_PARAMETER& arg) {
  ofs << "    ParameterType: " << toStr(arg.ParameterType) << "\n";
  ofs << "    ShaderVisibility: " << toStr(arg.ShaderVisibility) << "\n";
  switch (arg.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    ofs << arg.DescriptorTable;
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    ofs << arg.Constants;
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    ofs << arg.Descriptor;
    break;
  }
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_PARAMETER1& arg) {
  ofs << "    ParameterType: " << toStr(arg.ParameterType) << "\n";
  ofs << "    ShaderVisibility: " << toStr(arg.ShaderVisibility) << "\n";
  switch (arg.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    ofs << arg.DescriptorTable;
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    ofs << arg.Constants;
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    ofs << arg.Descriptor;
    break;
  }
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_STATIC_SAMPLER_DESC& arg) {
  ofs << "    Filter: " << toStr(arg.Filter) << "\n";
  ofs << "    AddressU: " << toStr(arg.AddressU) << "\n";
  ofs << "    AddressV: " << toStr(arg.AddressV) << "\n";
  ofs << "    AddressW: " << toStr(arg.AddressW) << "\n";
  ofs << "    MipLODBias: " << arg.MipLODBias << "\n";
  ofs << "    MaxAnisotropy: " << arg.MaxAnisotropy << "\n";
  ofs << "    ComparisonFunc: " << toStr(arg.ComparisonFunc) << "\n";
  ofs << "    BorderColor: " << toStr(arg.BorderColor) << "\n";
  ofs << "    MinLOD: " << arg.MinLOD << "\n";
  ofs << "    MaxLOD: " << arg.MaxLOD << "\n";
  ofs << "    ShaderRegister: " << arg.ShaderRegister << "\n";
  ofs << "    RegisterSpace: " << arg.RegisterSpace << "\n";
  ofs << "    ShaderVisibility: " << toStr(arg.ShaderVisibility) << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_STATIC_SAMPLER_DESC1& arg) {
  ofs << "    Filter: " << toStr(arg.Filter) << "\n";
  ofs << "    AddressU: " << toStr(arg.AddressU) << "\n";
  ofs << "    AddressV: " << toStr(arg.AddressV) << "\n";
  ofs << "    AddressW: " << toStr(arg.AddressW) << "\n";
  ofs << "    MipLODBias: " << arg.MipLODBias << "\n";
  ofs << "    MaxAnisotropy: " << arg.MaxAnisotropy << "\n";
  ofs << "    ComparisonFunc: " << toStr(arg.ComparisonFunc) << "\n";
  ofs << "    BorderColor: " << toStr(arg.BorderColor) << "\n";
  ofs << "    MinLOD: " << arg.MinLOD << "\n";
  ofs << "    MaxLOD: " << arg.MaxLOD << "\n";
  ofs << "    ShaderRegister: " << arg.ShaderRegister << "\n";
  ofs << "    RegisterSpace: " << arg.RegisterSpace << "\n";
  ofs << "    ShaderVisibility: " << toStr(arg.ShaderVisibility) << "\n";
  ofs << "    Flags: " << toStr(arg.Flags) << "\n";
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_SIGNATURE_DESC& arg) {
  ofs << "RootSignatureDesc:\n";
  ofs << "  Flags: " << toStr(arg.Flags) << "\n";
  ofs << "  NumParameters: " << arg.NumParameters << "\n";
  for (UINT i = 0; i < arg.NumParameters; ++i) {
    ofs << "  Parameter[" << i << "]:\n";
    ofs << arg.pParameters[i];
  }
  ofs << "  NumStaticSamplers: " << arg.NumStaticSamplers << "\n";
  for (UINT i = 0; i < arg.NumStaticSamplers; ++i) {
    ofs << "  StaticSampler[" << i << "]:\n";
    ofs << arg.pStaticSamplers[i];
  }
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_SIGNATURE_DESC1& arg) {
  ofs << "RootSignatureDesc:\n";
  ofs << "  Flags: " << toStr(arg.Flags) << "\n";
  ofs << "  NumParameters: " << arg.NumParameters << "\n";
  for (UINT i = 0; i < arg.NumParameters; ++i) {
    ofs << "  Parameter[" << i << "]:\n";
    ofs << arg.pParameters[i];
  }
  ofs << "  NumStaticSamplers: " << arg.NumStaticSamplers << "\n";
  for (UINT i = 0; i < arg.NumStaticSamplers; ++i) {
    ofs << "  StaticSampler[" << i << "]:\n";
    ofs << arg.pStaticSamplers[i];
  }
  return ofs;
}

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_SIGNATURE_DESC2& arg) {
  ofs << "RootSignatureDesc:\n";
  ofs << "  Flags: " << toStr(arg.Flags) << "\n";
  ofs << "  NumParameters: " << arg.NumParameters << "\n";
  for (UINT i = 0; i < arg.NumParameters; ++i) {
    ofs << "  Parameter[" << i << "]:\n";
    ofs << arg.pParameters[i];
  }
  ofs << "  NumStaticSamplers: " << arg.NumStaticSamplers << "\n";
  for (UINT i = 0; i < arg.NumStaticSamplers; ++i) {
    ofs << "  StaticSampler[" << i << "]:\n";
    ofs << arg.pStaticSamplers[i];
  }
  return ofs;
}

} // namespace DirectX
} // namespace gits
