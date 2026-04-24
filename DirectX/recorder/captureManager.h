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
  static CaptureManager& get();

  void exchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                  DXGIDispatchTable& wrapperTable);
  void exchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                   D3D12DispatchTable& wrapperTable);

  DXGIDispatchTable& getDXGIDispatchTable() {
    return m_DxgiDispatchTableSystem;
  }
  D3D12DispatchTable& getD3D12DispatchTable() {
    return m_D3D12DispatchTableSystem;
  }
  DMLDispatchTable& getDMLDispatchTable() {
    return m_DmlDispatchTable;
  }
  DStorageDispatchTable& getDStorageDispatchTable() {
    return m_DstorageDispatchTable;
  }
  Kernel32DispatchTable& getKernel32DispatchTable() {
    return m_Kernel32DispatchTableSystem;
  }
  XessDispatchTable& getXessDispatchTable() {
    return m_XessDispatchTable;
  }
  NvAPIDispatchTable& getNvAPIDispatchTable() {
    return m_NvapiDispatchTable;
  }
  D3D11On12DispatchTable& getd3d11on12DispatchTable() {
    return m_D3D11On12DispatchTable;
  }
  XellDispatchTable& getXellDispatchTable() {
    return m_XellDispatchTable;
  }
  XefgDispatchTable& getXefgDispatchTable() {
    return m_XefgDispatchTable;
  }

  std::vector<Layer*>& GetPreLayers() {
    return m_LayerManager.GetPreLayers();
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_LayerManager.GetPostLayers();
  }

  unsigned incrementGlobalStackDepth() {
    return ++m_GlobalStackDepth;
  }
  unsigned decrementGlobalStackDepth() {
    return --m_GlobalStackDepth;
  }
  unsigned incrementLocalStackDepth() {
    return ++m_LocalStackDepth;
  }
  unsigned decrementLocalStackDepth() {
    return --m_LocalStackDepth;
  }

  unsigned createWrapperKey() {
    return m_WrapperUniqueKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }
  unsigned createCommandKey() {
    return m_CommandUniqueKey.fetch_add(1, std::memory_order_relaxed) + 1;
  }
  std::pair<unsigned, unsigned> createCommandKeyRange(unsigned rangeSize);

  void updateCommandKey(Command& command) {
    m_Recorder->Skip(command.Key);
    command.Key = createCommandKey();
  }

  void addWrapper(IUnknownWrapper* wrapper);
  void removeWrapper(IUnknownWrapper* wrapper);
  IUnknownWrapper* findWrapper(IUnknown* object);

  CaptureDescriptorHandleService& getDescriptorHandleService() {
    return m_DescriptorHandleService;
  }
  MapTrackingService& getMapTrackingService() {
    return *m_MapTrackingService;
  }
  RootSignatureService& getRootSignatureService() {
    return m_RootSignatureService;
  }
  GpuAddressService& getGpuAddressService() {
    return m_GpuAddressService;
  }
  FenceService& getFenceService() {
    return *m_FenceService;
  }
  ContextMapService& getIntelExtensionsContextMap() {
    return m_IntelExtensionsContextMap;
  }
  ContextMapService& getXessContextMap() {
    return m_XessContextMap;
  }
  ContextMapService& getXellContextMap() {
    return m_XellContextMap;
  }
  ContextMapService& getXefgContextMap() {
    return m_XefgContextMap;
  }
  std::unordered_map<std::string, unsigned int>& getNvAPIFunctionIds() {
    return m_NvapiFunctionIds;
  }

  void interceptXessFunctions();
  void interceptXellFunctions();
  void interceptXefgFunctions();
  void loadIntelExtension(const uint32_t& vendorID, const uint32_t& deviceID);

private:
  CaptureManager();
  ~CaptureManager();
  CaptureManager(const CaptureManager&) = delete;
  CaptureManager& operator=(const CaptureManager&) = delete;

  void close();
  void interceptDirectMLFunctions();
  void interceptDirectStorageFunctions();
  void interceptKernelFunctions();
  void interceptNvAPIFunctions();
  void interceptD3D11On12Functions();

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
