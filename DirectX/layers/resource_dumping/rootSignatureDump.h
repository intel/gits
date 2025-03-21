// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools_lite.h"
#include <d3d12.h>

#include <fstream>

namespace gits {
namespace DirectX {

std::ofstream& operator<<(std::ofstream& ofs, const D3D12_DESCRIPTOR_RANGE& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_DESCRIPTOR_RANGE1& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR_TABLE& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR_TABLE1& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_CONSTANTS& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_DESCRIPTOR1& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_PARAMETER& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_PARAMETER1& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_STATIC_SAMPLER_DESC& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_STATIC_SAMPLER_DESC1& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_SIGNATURE_DESC& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_SIGNATURE_DESC1& arg);
std::ofstream& operator<<(std::ofstream& ofs, const D3D12_ROOT_SIGNATURE_DESC2& arg);

class RootSignatureDump : public gits::noncopyable {
public:
  void DeserializeRootSignature(const void* pBlobWithRootSignature,
                                size_t blobLengthInBytes,
                                const std::wstring& dumpName);

private:
  void SaveRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pDesc,
                         const std::wstring& dumpName);
};

} // namespace DirectX
} // namespace gits
