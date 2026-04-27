// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "windowService.h"
#include "replayDescriptorHandleService.h"
#include "mapTrackingService.h"
#include "playerGpuAddressService.h"
#include "heapAllocationService.h"
#include "adapterService.h"
#include "intelExtensionsService.h"
#include "xessService.h"
#include "multithreadedObjectCreationService.h"
#include "pipelineLibraryService.h"
#include "contextMapService.h"
#include "playerLayerManager.h"

#include "nvapi.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace gits {
namespace DirectX {

class PluginService;
class Layer;

class PlayerManager {
public:
  static PlayerManager& Get();
  static void Destroy() {
    delete m_Instance;
    m_Instance = nullptr;
  }

  ~PlayerManager();

  std::vector<Layer*>& GetPreLayers() {
    return m_LayerManager.GetPreLayers();
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_LayerManager.GetPostLayers();
  }

  bool ExecuteCommands() {
    return m_ExecuteCommands;
  }

  WindowService& GetWindowService() {
    return m_WindowService;
  }
  ReplayDescriptorHandleService& GetDescriptorHandleService() {
    return m_DescriptorHandleService;
  }
  MapTrackingService& GetMapTrackingService() {
    return m_MapTrackingService;
  }
  PlayerGpuAddressService& GetGpuAddressService() {
    return m_GpuAddressService;
  }
  HeapAllocationService& GetHeapAllocationService() {
    return m_HeapAllocationService;
  }
  AdapterService& GetAdapterService() {
    return m_AdapterService;
  }
  IntelExtensionsService& GetIntelExtensionsService() {
    return m_IntelExtensionsService;
  }
  XessService& GetXessService() {
    return m_XessService;
  }
  MultithreadedObjectCreationService& GetMultithreadedObjectCreationService() {
    return m_MultithreadedObjectCreationService;
  }
  PipelineLibraryService& GetPipelineLibraryService() {
    return m_PipelineLibraryService;
  }

  void AddObject(unsigned objectKey, IUnknown* object);
  void RemoveObject(unsigned objectKey);
  IUnknown* FindObject(unsigned objectKey);

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

  bool LoadAgilitySdk(const std::filesystem::path& path);

private:
  PlayerManager();
  PlayerManager(const PlayerManager&) = delete;
  PlayerManager& operator=(const PlayerManager&) = delete;

  void LoadDirectML();
  void LoadDirectStorage();

private:
  static PlayerManager* m_Instance;
  PlayerLayerManager m_LayerManager;

  bool m_ExecuteCommands{true};

  WindowService m_WindowService;
  ReplayDescriptorHandleService m_DescriptorHandleService;
  MapTrackingService m_MapTrackingService;
  PlayerGpuAddressService m_GpuAddressService;
  HeapAllocationService m_HeapAllocationService;
  AdapterService m_AdapterService;
  IntelExtensionsService m_IntelExtensionsService;
  std::unique_ptr<PluginService> m_PluginService;
  XessService m_XessService;
  MultithreadedObjectCreationService m_MultithreadedObjectCreationService;
  PipelineLibraryService m_PipelineLibraryService;

  HMODULE m_D3d12CoreDll{};
  HMODULE m_DmlDll{};
  HMODULE m_DmlDebugDll{};
  HMODULE m_DStorageDll{};
  HMODULE m_DStorageCoreDll{};

  std::unordered_map<unsigned, IUnknown*> m_Objects;
  ContextMapService m_IntelExtensionsContextMap;
  ContextMapService m_XessContextMap;
  ContextMapService m_XellContextMap;
  ContextMapService m_XefgContextMap;
};

} // namespace DirectX
} // namespace gits
