// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "windowService.h"
#include "replayDescriptorHandleService.h"
#include "mapTrackingService.h"
#include "gpuAddressService.h"
#include "heapAllocationService.h"
#include "pluginService.h"
#include "objectInfo.h"
#include "traceFactory.h"
#include "adapterService.h"
#include "intelExtensionsService.h"
#include "xessService.h"
#include "subcaptureFactory.h"
#include "analyzerLayerAuto.h"
#include "resourceDumpingFactory.h"
#include "skipCallsFactory.h"
#include "multithreadedObjectCreationService.h"
#include "pipelineLibraryService.h"
#include "contextMapService.h"

#include <vector>
#include <memory>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ObjectInfoPlayer : public ObjectInfos {
public:
  IUnknown* object{};

public:
  void addObjectInfo(Layer* layer, ObjectInfo* objectInfo) override {
    infos_[layer].reset(objectInfo);
  }

  virtual ObjectInfo* getObjectInfo(Layer* layer) override {
    auto it = infos_.find(layer);
    if (it == infos_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

private:
  std::map<Layer*, std::unique_ptr<ObjectInfo>> infos_;
};

class PlayerManager : public gits::noncopyable {
public:
  static PlayerManager& get();
  static void destroy() {
    delete instance_;
    instance_ = nullptr;
  }

  ~PlayerManager();

  std::vector<Layer*>& getPreLayers() {
    return preLayers_;
  }
  std::vector<Layer*>& getPostLayers() {
    return postLayers_;
  }

  bool executeCommands() {
    return executeCommands_;
  }

  bool multithreadedShaderCompilation() {
    return multithreadedShaderCompilation_;
  }

  WindowService& getWindowService() {
    return windowService_;
  }
  ReplayDescriptorHandleService& getDescriptorHandleService() {
    return descriptorHandleService_;
  }
  MapTrackingService& getMapTrackingService() {
    return mapTrackingService_;
  }
  GpuAddressService& getGpuAddressService() {
    return gpuAddressService_;
  }
  HeapAllocationService& getHeapAllocationService() {
    return heapAllocationService_;
  }
  AdapterService& getAdapterService() {
    return adapterService_;
  }
  IntelExtensionsService& getIntelExtensionsService() {
    return intelExtensionsService_;
  }
  XessService& getXessService() {
    return xessService_;
  }
  MultithreadedObjectCreationService& getMultithreadedObjectCreationService() {
    return multithreadedObjectCreationService_;
  }
  PipelineLibraryService& getPipelineLibraryService() {
    return pipelineLibraryService_;
  }

  void addObject(unsigned objectKey, ObjectInfoPlayer* objectInfo);
  void removeObject(unsigned objectKey);
  ObjectInfoPlayer* findObject(unsigned objectKey);

  ContextMapService& getIntelExtensionsContextMap() {
    return intelExtensionsContextMap_;
  }
  ContextMapService& getXessContextMap() {
    return xessContextMap_;
  }

private:
  PlayerManager();

  void loadAgilitySdk();
  void loadDirectML();
  void loadDirectStorage();

private:
  static PlayerManager* instance_;

  // These hold pointers to Layers stored in layersOwner_. Layer order is important.
  std::vector<Layer*> preLayers_;
  std::vector<Layer*> postLayers_;
  // Holds ownership of Layers.
  std::vector<std::unique_ptr<Layer>> layersOwner_;

  // Factory classes encapsulate layer creation logic.
  TraceFactory traceFactory_;
  SubcaptureFactory subcaptureFactory_;
  ResourceDumpingFactory resourceDumpingFactory_;
  SkipCallsFactory skipCallsFactory_;

  bool executeCommands_{true};
  bool multithreadedShaderCompilation_{true};

  WindowService windowService_;
  ReplayDescriptorHandleService descriptorHandleService_;
  MapTrackingService mapTrackingService_;
  GpuAddressService gpuAddressService_;
  HeapAllocationService heapAllocationService_;
  AdapterService adapterService_;
  IntelExtensionsService intelExtensionsService_;
  std::unique_ptr<PluginService> pluginService_;
  XessService xessService_;
  MultithreadedObjectCreationService multithreadedObjectCreationService_;
  PipelineLibraryService pipelineLibraryService_;

  HMODULE d3d12CoreDll_{};
  HMODULE dmlDll_{};
  HMODULE dmlDebugDll_{};
  HMODULE dStorageDll_{};
  HMODULE dStorageCoreDll_{};

  std::unordered_map<unsigned, std::unique_ptr<ObjectInfoPlayer>> objects_;
  ContextMapService intelExtensionsContextMap_;
  ContextMapService xessContextMap_;
};

} // namespace DirectX
} // namespace gits
