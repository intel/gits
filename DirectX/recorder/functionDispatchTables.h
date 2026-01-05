// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

namespace gits {
namespace DirectX {

struct DXGIDispatchTable {
  decltype(CreateDXGIFactory)* CreateDXGIFactory;
  decltype(CreateDXGIFactory1)* CreateDXGIFactory1;
  decltype(CreateDXGIFactory2)* CreateDXGIFactory2;
  decltype(DXGIDeclareAdapterRemovalSupport)* DXGIDeclareAdapterRemovalSupport;
  decltype(DXGIGetDebugInterface1)* DXGIGetDebugInterface1;
};

struct D3D12DispatchTable {
  decltype(D3D12CreateDevice)* D3D12CreateDevice;
  decltype(D3D12GetDebugInterface)* D3D12GetDebugInterface;
  decltype(D3D12CreateRootSignatureDeserializer)* D3D12CreateRootSignatureDeserializer;
  decltype(D3D12CreateVersionedRootSignatureDeserializer)*
      D3D12CreateVersionedRootSignatureDeserializer;
  decltype(D3D12EnableExperimentalFeatures)* D3D12EnableExperimentalFeatures;
  decltype(D3D12GetInterface)* D3D12GetInterface;
  decltype(D3D12SerializeRootSignature)* D3D12SerializeRootSignature;
  decltype(D3D12SerializeVersionedRootSignature)* D3D12SerializeVersionedRootSignature;
};

struct DMLDispatchTable {
  decltype(DMLCreateDevice)* DMLCreateDevice;
  decltype(DMLCreateDevice1)* DMLCreateDevice1;
};

struct DStorageDispatchTable {
  decltype(DStorageSetConfiguration)* DStorageSetConfiguration;
  decltype(DStorageSetConfiguration1)* DStorageSetConfiguration1;
  decltype(DStorageGetFactory)* DStorageGetFactory;
  decltype(DStorageCreateCompressionCodec)* DStorageCreateCompressionCodec;
};

struct Kernel32DispatchTable {
  decltype(WaitForSingleObject)* WaitForSingleObject;
  decltype(WaitForSingleObjectEx)* WaitForSingleObjectEx;
  decltype(WaitForMultipleObjects)* WaitForMultipleObjects;
  decltype(WaitForMultipleObjectsEx)* WaitForMultipleObjectsEx;
  decltype(SignalObjectAndWait)* SignalObjectAndWait;
  decltype(LoadLibraryA)* LoadLibraryA;
  decltype(LoadLibraryW)* LoadLibraryW;
  decltype(LoadLibraryExA)* LoadLibraryExA;
  decltype(LoadLibraryExW)* LoadLibraryExW;
};

} // namespace DirectX
} // namespace gits
