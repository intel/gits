// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "functionDispatchTables.h"
#include "intelExtensions.h"
#include "xessDispatchTableAuto.h"
#include "xellDispatchTableAuto.h"
#include "xefgDispatchTableAuto.h"
#include "nvapiDispatchTable.h"
#include "d3d11on12DispatchTable.h"
#include "captureDescriptorHandleService.h"
#include "mapTrackingService.h"
#include "rootSignatureService.h"
#include "gpuAddressService.h"
#include "pluginService.h"
#include "fenceService.h"
#include "orderingRecorder.h"
#include "contextMapService.h"
#include "directx.h"
#include "captureLayerManager.h"

#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <utility>

namespace gits {
namespace DirectX {

class IUnknownWrapper;

class CaptureManager {
public:
  static CaptureManager& Get();

  void ExchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                  DXGIDispatchTable& wrapperTable);
  void ExchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                   D3D12DispatchTable& wrapperTable);

  DXGIDispatchTable& GetDXGIDispatchTable() {
    return m_DxgiDispatchTableSystem;
  }
  D3D12DispatchTable& GetD3D12DispatchTable() {
    return m_D3D12DispatchTableSystem;
  }
  DMLDispatchTable& GetDMLDispatchTable() {
    return m_DmlDispatchTable;
  }
  DStorageDispatchTable& GetDStorageDispatchTable() {
    return m_DstorageDispatchTable;
  }
  Kernel32DispatchTable& GetKernel32DispatchTable() {
    return m_Kernel32DispatchTableSystem;
  }
  XessDispatchTable& GetXessDispatchTable() {
    return m_XessDispatchTable;
  }
  NvAPIDispatchTable& GetNvAPIDispatchTable() {
    return m_NvapiDispatchTable;
  }
  D3D11On12DispatchTable& GetD3D11On12DispatchTable() {
    return m_D3D11On12DispatchTable;
  }
  XellDispatchTable& GetXellDispatchTable() {
    return m_XellDispatchTable;
  }
  XefgDispatchTable& GetXefgDispatchTable() {
    return m_XefgDispatchTable;
  }

  std::vector<Layer*>& GetPreLayers() {
    return m_LayerManager.GetPreLayers();
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_LayerManager.GetPostLayers();
  }

  unsigned IncrementGlobalStackDepth() {
    return ++m_GlobalStackDepth;
  }
  unsigned DecrementGlobalStackDepth() {
    return --m_GlobalStackDepth;
  }
  unsigned IncrementLocalStackDepth() {
    return ++m_LocalStackDepth;
  }
  unsigned DecrementLocalStackDepth() {
    return --m_LocalStackDepth;
  }

  unsigned CreateWrapperKey() {
    return m_WrapperUniqueKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }
  unsigned CreateCommandKey() {
    return m_CommandUniqueKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }
  std::pair<unsigned, unsigned> CreateCommandKeyRange(unsigned rangeSize);

  void UpdateCommandKey(Command& command) {
    m_Recorder->Skip(command.Key);
    command.Key = CreateCommandKey();
  }

  void AddWrapper(IUnknownWrapper* wrapper);
  void RemoveWrapper(IUnknownWrapper* wrapper);
  IUnknownWrapper* FindWrapper(IUnknown* object);

  CaptureDescriptorHandleService& GetDescriptorHandleService() {
    return m_DescriptorHandleService;
  }
  MapTrackingService& GetMapTrackingService() {
    return *m_MapTrackingService;
  }
  RootSignatureService& GetRootSignatureService() {
    return m_RootSignatureService;
  }
  GpuAddressService& GetGpuAddressService() {
    return m_GpuAddressService;
  }
  FenceService& GetFenceService() {
    return *m_FenceService;
  }
  ContextMapService& GetIntelExtensionsContextMap() {
    return m_IntelExtensionsContextMap;
  }
  ContextMapService& GetXessContextMap() {
    return m_XessContextMap;
  }
  ContextMapService& GetXellContextMap() {
    return m_XellContextMap;
  }
  ContextMapService& GetXefgContextMap() {
    return m_XefgContextMap;
  }
  std::unordered_map<std::string, unsigned int>& GetNvAPIFunctionIds() {
    return m_NvapiFunctionIds;
  }

  void InterceptXessFunctions();
  void InterceptXellFunctions();
  void InterceptXefgFunctions();
  void LoadIntelExtension(const uint32_t& vendorID, const uint32_t& deviceID);

private:
  CaptureManager();
  ~CaptureManager();
  CaptureManager(const CaptureManager&) = delete;
  CaptureManager& operator=(const CaptureManager&) = delete;

  void Close();
  void InterceptDirectMLFunctions();
  void InterceptDirectStorageFunctions();
  void InterceptKernelFunctions();
  void InterceptNvAPIFunctions();
  void InterceptD3D11On12Functions();

private:
  static CaptureManager* m_Instance;
  CaptureLayerManager m_LayerManager;

  DXGIDispatchTable m_DxgiDispatchTableSystem{};
  DXGIDispatchTable m_DxgiDispatchTableWrapper{};
  D3D12DispatchTable m_D3D12DispatchTableSystem{};
  D3D12DispatchTable m_D3D12DispatchTableWrapper{};
  DMLDispatchTable m_DmlDispatchTable{};
  DStorageDispatchTable m_DstorageDispatchTable{};
  Kernel32DispatchTable m_Kernel32DispatchTableSystem{};
  XessDispatchTable m_XessDispatchTable{};
  XellDispatchTable m_XellDispatchTable{};
  XefgDispatchTable m_XefgDispatchTable{};
  NvAPIDispatchTable m_NvapiDispatchTable{};
  D3D11On12DispatchTable m_D3D11On12DispatchTable{};

  std::atomic<unsigned> m_GlobalStackDepth{0};
  static thread_local unsigned m_LocalStackDepth;

  std::atomic<unsigned> m_WrapperUniqueKey{0};
  std::atomic<unsigned> m_CommandUniqueKey{0};

  std::unordered_map<IUnknown*, IUnknownWrapper*> m_Wrappers;
  std::mutex m_WrappersMutex;

  std::unique_ptr<stream::OrderingRecorder> m_Recorder;

  CaptureDescriptorHandleService m_DescriptorHandleService;
  std::unique_ptr<MapTrackingService> m_MapTrackingService;
  RootSignatureService m_RootSignatureService;
  GpuAddressService m_GpuAddressService;
  std::unique_ptr<FenceService> m_FenceService;
  PluginService m_PluginService;

  ContextMapService m_IntelExtensionsContextMap;
  ContextMapService m_XessContextMap;
  ContextMapService m_XellContextMap;
  ContextMapService m_XefgContextMap;

  HMODULE m_KernelDll{};
  HMODULE m_DmlDll{};
  HMODULE m_DstorageDll{};
  HMODULE m_XessDll{};
  HMODULE m_XellDll{};
  HMODULE m_XefgDll{};
  bool m_LoadingXessDll{};
  bool m_LoadingXellDll{};
  bool m_LoadingXefgDll{};
  bool m_IntelExtensionLoaded{};
  HMODULE m_NvapiDll{};
  HMODULE m_D3D11Dll{};
  std::unordered_map<std::string, unsigned int> m_NvapiFunctionIds;
};

} // namespace DirectX
} // namespace gits
