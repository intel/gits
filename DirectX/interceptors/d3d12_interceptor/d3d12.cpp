// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "d3d12RecorderInterface.h"
#include "gitsLoader.h"

#include <string>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <detours.h>

gits::DirectX::D3D12DispatchTable g_dispatchTableSystem;
gits::DirectX::D3D12DispatchTable g_dispatchTableWrapper;

class Layer;
using D3D12CoreRegisterLayersPtr = HRESULT(WINAPI*)(Layer* layers, UINT numLayers);
using D3D12CoreGetLayeredDeviceSizePtr = HRESULT(WINAPI*)(Layer* layers, UINT numLayers);
using D3D12CoreCreateLayeredDevicePtr =
    HRESULT(WINAPI*)(const void* p1, DWORD p2, const void* p3, REFIID riid, void** device);
using GetBehaviorValuePtr = HRESULT(WINAPI*)();
using SetAppCompatStringPointerPtr = HRESULT(WINAPI*)();
using D3D12DeviceRemovedExtendedDataPtr = HRESULT(WINAPI*)();
using D3D12PIXEventsReplaceBlockPtr = HRESULT(WINAPI*)();
using D3D12PIXGetThreadInfoPtr = HRESULT(WINAPI*)();
using D3D12PIXNotifyWakeFromFenceSignalPtr = HRESULT(WINAPI*)();
using D3D12PIXReportCounterPtr = HRESULT(WINAPI*)();

struct D3D12DispatchTableForwarded {
  D3D12CoreRegisterLayersPtr D3D12CoreRegisterLayers{};
  D3D12CoreGetLayeredDeviceSizePtr D3D12CoreGetLayeredDeviceSize{};
  D3D12CoreCreateLayeredDevicePtr D3D12CoreCreateLayeredDevice{};
  GetBehaviorValuePtr GetBehaviorValue{};
  SetAppCompatStringPointerPtr SetAppCompatStringPointer{};
  D3D12DeviceRemovedExtendedDataPtr D3D12DeviceRemovedExtendedData{};
  D3D12PIXEventsReplaceBlockPtr D3D12PIXEventsReplaceBlock{};
  D3D12PIXGetThreadInfoPtr D3D12PIXGetThreadInfo{};
  D3D12PIXNotifyWakeFromFenceSignalPtr D3D12PIXNotifyWakeFromFenceSignal{};
  D3D12PIXReportCounterPtr D3D12PIXReportCounter{};
};
D3D12DispatchTableForwarded g_dispatchTableForwarded;

HMODULE g_systemDll{0};
std::unique_ptr<gits::CGitsLoader> g_gitsLoader;

void initializeDispatchTable() {
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  g_systemDll = LoadLibrary("C:\\Windows\\System32\\d3d12.dll");
  assert(g_systemDll);

  g_dispatchTableSystem.D3D12CreateDevice = reinterpret_cast<decltype(D3D12CreateDevice)*>(
      GetProcAddress(g_systemDll, "D3D12CreateDevice"));

  g_dispatchTableSystem.D3D12GetDebugInterface =
      reinterpret_cast<decltype(D3D12GetDebugInterface)*>(
          GetProcAddress(g_systemDll, "D3D12GetDebugInterface"));

  g_dispatchTableSystem.D3D12CreateRootSignatureDeserializer =
      reinterpret_cast<decltype(D3D12CreateRootSignatureDeserializer)*>(
          GetProcAddress(g_systemDll, "D3D12CreateRootSignatureDeserializer"));

  g_dispatchTableSystem.D3D12CreateVersionedRootSignatureDeserializer =
      reinterpret_cast<decltype(D3D12CreateVersionedRootSignatureDeserializer)*>(
          GetProcAddress(g_systemDll, "D3D12CreateVersionedRootSignatureDeserializer"));

  g_dispatchTableSystem.D3D12EnableExperimentalFeatures =
      reinterpret_cast<decltype(D3D12EnableExperimentalFeatures)*>(
          GetProcAddress(g_systemDll, "D3D12EnableExperimentalFeatures"));

  g_dispatchTableSystem.D3D12GetInterface = reinterpret_cast<decltype(D3D12GetInterface)*>(
      GetProcAddress(g_systemDll, "D3D12GetInterface"));

  g_dispatchTableSystem.D3D12SerializeRootSignature =
      reinterpret_cast<decltype(D3D12SerializeRootSignature)*>(
          GetProcAddress(g_systemDll, "D3D12SerializeRootSignature"));

  g_dispatchTableSystem.D3D12SerializeVersionedRootSignature =
      reinterpret_cast<decltype(D3D12SerializeVersionedRootSignature)*>(
          GetProcAddress(g_systemDll, "D3D12SerializeVersionedRootSignature"));
}

void hookD3d12() {
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  initializeDispatchTable();

  LONG ret = DetourTransactionBegin();
  assert(ret == NO_ERROR);

  ret = DetourUpdateThread(GetCurrentThread());
  assert(ret == NO_ERROR);

  ret = DetourAttach(&g_dispatchTableSystem.D3D12CreateDevice, D3D12CreateDevice);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.D3D12GetDebugInterface, D3D12GetDebugInterface);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.D3D12CreateRootSignatureDeserializer,
                     D3D12CreateRootSignatureDeserializer);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.D3D12CreateVersionedRootSignatureDeserializer,
                     D3D12CreateVersionedRootSignatureDeserializer);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.D3D12EnableExperimentalFeatures,
                     D3D12EnableExperimentalFeatures);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.D3D12GetInterface, D3D12GetInterface);
  assert(ret == NO_ERROR);
  ret =
      DetourAttach(&g_dispatchTableSystem.D3D12SerializeRootSignature, D3D12SerializeRootSignature);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.D3D12SerializeVersionedRootSignature,
                     D3D12SerializeVersionedRootSignature);
  assert(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  assert(ret == NO_ERROR);
}

void initialize() {
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  g_gitsLoader = std::make_unique<gits::CGitsLoader>("GITSRecorderD3D12");

  initializeDispatchTable();

  g_dispatchTableForwarded.D3D12CoreRegisterLayers = reinterpret_cast<D3D12CoreRegisterLayersPtr>(
      GetProcAddress(g_systemDll, "D3D12CoreRegisterLayers"));
  g_dispatchTableForwarded.D3D12CoreGetLayeredDeviceSize =
      reinterpret_cast<D3D12CoreGetLayeredDeviceSizePtr>(
          GetProcAddress(g_systemDll, "D3D12CoreGetLayeredDeviceSize"));
  g_dispatchTableForwarded.D3D12CoreCreateLayeredDevice =
      reinterpret_cast<D3D12CoreCreateLayeredDevicePtr>(
          GetProcAddress(g_systemDll, "D3D12CoreCreateLayeredDevice"));
  g_dispatchTableForwarded.GetBehaviorValue =
      reinterpret_cast<GetBehaviorValuePtr>(GetProcAddress(g_systemDll, "GetBehaviorValue"));
  g_dispatchTableForwarded.SetAppCompatStringPointer =
      reinterpret_cast<SetAppCompatStringPointerPtr>(
          GetProcAddress(g_systemDll, "SetAppCompatStringPointer"));
  g_dispatchTableForwarded.D3D12DeviceRemovedExtendedData =
      reinterpret_cast<D3D12DeviceRemovedExtendedDataPtr>(
          GetProcAddress(g_systemDll, "D3D12DeviceRemovedExtendedData"));
  g_dispatchTableForwarded.D3D12PIXEventsReplaceBlock =
      reinterpret_cast<D3D12PIXEventsReplaceBlockPtr>(
          GetProcAddress(g_systemDll, "D3D12PIXEventsReplaceBlock"));
  g_dispatchTableForwarded.D3D12PIXGetThreadInfo = reinterpret_cast<D3D12PIXGetThreadInfoPtr>(
      GetProcAddress(g_systemDll, "D3D12PIXGetThreadInfo"));
  g_dispatchTableForwarded.D3D12PIXNotifyWakeFromFenceSignal =
      reinterpret_cast<D3D12PIXNotifyWakeFromFenceSignalPtr>(
          GetProcAddress(g_systemDll, "D3D12PIXNotifyWakeFromFenceSignal"));
  g_dispatchTableForwarded.D3D12PIXReportCounter = reinterpret_cast<D3D12PIXReportCounterPtr>(
      GetProcAddress(g_systemDll, "D3D12PIXReportCounter"));

  auto* recorderWrapper =
      reinterpret_cast<gits::DirectX::IRecorderWrapper*>(g_gitsLoader->GetRecorderWrapperPtr());
  recorderWrapper->ExchangeD3D12DispatchTables(g_dispatchTableSystem, g_dispatchTableWrapper);
}

HRESULT WINAPI D3D12CreateDevice(IUnknown* pAdapter,
                                 D3D_FEATURE_LEVEL MinimumFeatureLevel,
                                 REFIID riid,
                                 void** ppDevice) {
  initialize();
  return g_dispatchTableWrapper.D3D12CreateDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);
}

HRESULT WINAPI D3D12GetDebugInterface(REFIID riid, void** ppvDebug) {
  initialize();
  return g_dispatchTableWrapper.D3D12GetDebugInterface(riid, ppvDebug);
}

HRESULT WINAPI D3D12CreateRootSignatureDeserializer(LPCVOID pSrcData,
                                                    SIZE_T SrcDataSizeInBytes,
                                                    REFIID pRootSignatureDeserializerInterface,
                                                    void** ppRootSignatureDeserializer) {
  initialize();
  return g_dispatchTableWrapper.D3D12CreateRootSignatureDeserializer(
      pSrcData, SrcDataSizeInBytes, pRootSignatureDeserializerInterface,
      ppRootSignatureDeserializer);
}

HRESULT WINAPI
D3D12CreateVersionedRootSignatureDeserializer(LPCVOID pSrcData,
                                              SIZE_T SrcDataSizeInBytes,
                                              REFIID pRootSignatureDeserializerInterface,
                                              void** ppRootSignatureDeserializer) {
  initialize();
  return g_dispatchTableWrapper.D3D12CreateVersionedRootSignatureDeserializer(
      pSrcData, SrcDataSizeInBytes, pRootSignatureDeserializerInterface,
      ppRootSignatureDeserializer);
}

HRESULT WINAPI D3D12EnableExperimentalFeatures(UINT NumFeatures,
                                               const IID* pIIDs,
                                               void* pConfigurationStructs,
                                               UINT* pConfigurationStructSizes) {
  initialize();
  return g_dispatchTableWrapper.D3D12EnableExperimentalFeatures(
      NumFeatures, pIIDs, pConfigurationStructs, pConfigurationStructSizes);
}

HRESULT WINAPI D3D12GetInterface(REFCLSID rclsid, REFIID riid, void** ppvDebug) {
  initialize();
  return g_dispatchTableWrapper.D3D12GetInterface(rclsid, riid, ppvDebug);
}

HRESULT WINAPI D3D12SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
                                           D3D_ROOT_SIGNATURE_VERSION Version,
                                           ID3DBlob** ppBlob,
                                           ID3DBlob** ppErrorBlob) {
  initialize();
  return g_dispatchTableWrapper.D3D12SerializeRootSignature(pRootSignature, Version, ppBlob,
                                                            ppErrorBlob);
}

HRESULT WINAPI
D3D12SerializeVersionedRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
                                     ID3DBlob** ppBlob,
                                     ID3DBlob** ppErrorBlob) {
  initialize();
  return g_dispatchTableWrapper.D3D12SerializeVersionedRootSignature(pRootSignature, ppBlob,
                                                                     ppErrorBlob);
}

HRESULT WINAPI D3D12CoreRegisterLayers(Layer* layers, UINT numLayers) {
  initialize();
  return g_dispatchTableForwarded.D3D12CoreRegisterLayers(layers, numLayers);
}

SIZE_T WINAPI D3D12CoreGetLayeredDeviceSize(Layer* layers, UINT numLayers) {
  initialize();
  return g_dispatchTableForwarded.D3D12CoreGetLayeredDeviceSize(layers, numLayers);
}

HRESULT WINAPI
D3D12CoreCreateLayeredDevice(const void* p1, DWORD p2, const void* p3, REFIID riid, void** device) {
  initialize();
  return g_dispatchTableForwarded.D3D12CoreCreateLayeredDevice(p1, p2, p3, riid, device);
}

HRESULT WINAPI GetBehaviorValue() {
  initialize();
  return g_dispatchTableForwarded.GetBehaviorValue();
}

HRESULT WINAPI SetAppCompatStringPointer() {
  initialize();
  return g_dispatchTableForwarded.SetAppCompatStringPointer();
}

HRESULT WINAPI D3D12DeviceRemovedExtendedData() {
  initialize();
  return g_dispatchTableForwarded.D3D12DeviceRemovedExtendedData();
}

HRESULT WINAPI D3D12PIXEventsReplaceBlock() {
  initialize();
  return g_dispatchTableForwarded.D3D12PIXEventsReplaceBlock();
}

HRESULT WINAPI D3D12PIXGetThreadInfo() {
  initialize();
  return g_dispatchTableForwarded.D3D12PIXGetThreadInfo();
}

HRESULT WINAPI D3D12PIXNotifyWakeFromFenceSignal() {
  initialize();
  return g_dispatchTableForwarded.D3D12PIXNotifyWakeFromFenceSignal();
}

HRESULT WINAPI D3D12PIXReportCounter() {
  initialize();
  return g_dispatchTableForwarded.D3D12PIXReportCounter();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    hookD3d12();
    break;
  case DLL_PROCESS_DETACH:
    if (!lpvReserved) {
      if (g_systemDll) {
        FreeLibrary(g_systemDll);
      }
      g_gitsLoader.reset();
    }
    break;
  }
  return TRUE;
}
