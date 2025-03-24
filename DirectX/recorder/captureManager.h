// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "functionDispatchTables.h"
#include "intelExtensions.h"
#include "xessDispatchTableAuto.h"
#include "captureDescriptorHandleService.h"
#include "mapTrackingService.h"
#include "rootSignatureService.h"
#include "gpuAddressService.h"
#include "pluginService.h"
#include "fenceService.h"
#include "traceFactory.h"
#include "gitsRecorder.h"
#include "contextMapService.h"
#include "directx.h"
#include "resourceDumpingFactory.h"
#include "portabilityFactory.h"

#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>

namespace gits {
namespace DirectX {

class IUnknownWrapper;

class CaptureManager : public gits::noncopyable {
public:
  static CaptureManager& get();

  void exchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                  DXGIDispatchTable& wrapperTable);
  void exchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                   D3D12DispatchTable& wrapperTable);

  DXGIDispatchTable& getDXGIDispatchTable() {
    return dxgiDispatchTableSystem_;
  }
  D3D12DispatchTable& getD3D12DispatchTable() {
    return d3d12DispatchTableSystem_;
  }
  DMLDispatchTable& getDMLDispatchTable() {
    return dmlDispatchTable_;
  }
  DStorageDispatchTable& getDStorageDispatchTable() {
    return dstorageDispatchTable_;
  }
  Kernel32DispatchTable& getKernel32DispatchTable() {
    return kernel32DispatchTableSystem_;
  }
  XessDispatchTable& getXessDispatchTable() {
    return xessDispatchTable_;
  }

  std::vector<Layer*>& getPreLayers() {
    return preLayers_;
  }
  std::vector<Layer*>& getPostLayers() {
    return postLayers_;
  }

  unsigned incrementGlobalStackDepth() {
    return ++globalStackDepth_;
  }
  unsigned decrementGlobalStackDepth() {
    return --globalStackDepth_;
  }
  unsigned incrementLocalStackDepth() {
    return ++localStackDepth_;
  }
  unsigned decrementLocalStackDepth() {
    return --localStackDepth_;
  }

  unsigned createWrapperKey() {
    std::lock_guard<std::mutex> lock(uniqueKeyMutex_);
    return ++wrapperUniqueKey_;
  }
  unsigned createCommandKey() {
    std::lock_guard<std::mutex> lock(uniqueKeyMutex_);
    return ++commandUniqueKey_;
  }

  void updateCommandKey(Command& command) {
    recorder_->skip(command.key);
    command.key = createCommandKey();
  }

  void addWrapper(IUnknownWrapper* wrapper);
  void removeWrapper(IUnknownWrapper* wrapper);
  IUnknownWrapper* findWrapper(IUnknown* object);

  CaptureDescriptorHandleService& getDescriptorHandleService() {
    return descriptorHandleService_;
  }
  MapTrackingService& getMapTrackingService() {
    return *mapTrackingService_;
  }
  RootSignatureService& getRootSignatureService() {
    return rootSignatureService_;
  }
  GpuAddressService& getGpuAddressService() {
    return gpuAddressService_;
  }
  FenceService& getFenceService() {
    return *fenceService_;
  }
  ContextMapService& getIntelExtensionsContextMap() {
    return intelExtensionsContextMap_;
  }
  ContextMapService& getXessContextMap() {
    return xessContextMap_;
  }

  void interceptXessFunctions();
  void loadIntelExtension(const uint32_t& vendorID, const uint32_t& deviceID);

private:
  CaptureManager();
  ~CaptureManager();

  void createLayers();
  void interceptDirectMLFunctions();
  void interceptDirectStorageFunctions();
  void interceptKernelFunctions();

private:
  static CaptureManager* instance_;

  DXGIDispatchTable dxgiDispatchTableSystem_{};
  DXGIDispatchTable dxgiDispatchTableWrapper_{};
  D3D12DispatchTable d3d12DispatchTableSystem_{};
  D3D12DispatchTable d3d12DispatchTableWrapper_{};
  DMLDispatchTable dmlDispatchTable_{};
  DStorageDispatchTable dstorageDispatchTable_{};
  Kernel32DispatchTable kernel32DispatchTableSystem_{};
  XessDispatchTable xessDispatchTable_{};

  // These hold pointers to Layers stored in layersOwner_. Layer order is important.
  std::vector<Layer*> preLayers_;
  std::vector<Layer*> postLayers_;
  // Holds ownership of Layers.
  std::vector<std::unique_ptr<Layer>> layersOwner_;

  // Factory classes encapsulate layer creation logic.
  TraceFactory traceFactory_;
  ResourceDumpingFactory resourceDumpingFactory_;
  PortabilityFactory portabilityFactory_;

  std::atomic<unsigned> globalStackDepth_{0};
  static thread_local unsigned localStackDepth_;

  unsigned wrapperUniqueKey_{0};
  unsigned commandUniqueKey_{0};
  std::mutex uniqueKeyMutex_;

  std::unordered_map<IUnknown*, IUnknownWrapper*> wrappers_;
  std::mutex wrappersMutex_;

  std::unique_ptr<GitsRecorder> recorder_;

  CaptureDescriptorHandleService descriptorHandleService_;
  std::unique_ptr<MapTrackingService> mapTrackingService_;
  RootSignatureService rootSignatureService_;
  GpuAddressService gpuAddressService_;
  std::unique_ptr<FenceService> fenceService_;
  PluginService pluginService_;

  ContextMapService intelExtensionsContextMap_;
  ContextMapService xessContextMap_;

  HMODULE kernelDll_{};
  HMODULE dmlDll_{};
  HMODULE dStorageDll_{};
  HMODULE xessDll_{};
  bool loadingXessDll_{};
  bool intelExtensionLoaded_{};
};

} // namespace DirectX
} // namespace gits
