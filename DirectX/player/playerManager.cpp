// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"
#include "layerAuto.h"
#include "pluginService.h"
#include "IPlugin.h"
#include "messageBus.h"
#include "log.h"
#include "exception.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

PlayerManager* PlayerManager::m_Instance = nullptr;

PlayerManager& PlayerManager::Get() {
  if (!m_Instance) {
    m_Instance = new PlayerManager();
    gits::MessageBus::get().subscribe(
        {PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
        [](Topic t, const MessagePtr& m) { PlayerManager::Destroy(); });
  }
  return *m_Instance;
}

PlayerManager::~PlayerManager() {
  try {
    LOG_INFO << "PlayerManager: Playback completed. Cleaning up...";
    m_MultithreadedObjectCreationService.Shutdown();
    m_PluginService.reset();
  } catch (...) {
    topmost_exception_handler("PlayerManager::~PlayerManager");
  }

  if (m_D3d12CoreDll) {
    FreeLibrary(m_D3d12CoreDll);
  }
  if (m_DmlDll) {
    FreeLibrary(m_DmlDll);
  }
  if (m_DmlDebugDll) {
    FreeLibrary(m_DmlDebugDll);
  }
  if (m_DStorageDll) {
    FreeLibrary(m_DStorageDll);
  }
  if (m_DStorageCoreDll) {
    FreeLibrary(m_DStorageCoreDll);
  }
}

PlayerManager::PlayerManager() {
  m_ExecuteCommands = Configurator::Get().directx.player.execute;

  // Load DirectX runtimes
  LoadDirectML();
  LoadDirectStorage();

  GetAdapterService().LoadAdapters();
  GetIntelExtensionsService().LoadIntelExtensions(GetAdapterService().GetAdapter());
  GetIntelExtensionsService().SetApplicationInfo();

  m_PluginService = std::make_unique<PluginService>();
  m_PluginService->loadPlugins();

  m_LayerManager.LoadLayers(*this, *m_PluginService.get());
}

void PlayerManager::AddObject(unsigned objectKey, IUnknown* object) {
  m_Objects[objectKey] = object;
}

void PlayerManager::RemoveObject(unsigned objectKey) {
  m_Objects.erase(objectKey);
}

IUnknown* PlayerManager::FindObject(unsigned objectKey) {
  auto it = m_Objects.find(objectKey);
  if (it == m_Objects.end()) {
    return nullptr;
  }
  return it->second;
}

bool PlayerManager::LoadAgilitySdk(const std::filesystem::path& path) {
  std::string dllPath = (path / "D3D12Core.dll").string();
  m_D3d12CoreDll = LoadLibrary(dllPath.c_str());
  if (!m_D3d12CoreDll) {
    LOG_ERROR << "Agility SDK - Failed to load (" << dllPath << ")";
    return false;
  }
  UINT sdkVersion = *reinterpret_cast<UINT*>(GetProcAddress(m_D3d12CoreDll, "D3D12SDKVersion"));

  Microsoft::WRL::ComPtr<ID3D12SDKConfiguration> sdkConfiguration;
  HRESULT hr = D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdkConfiguration));
  if (hr != S_OK) {
    LOG_ERROR << "Agility SDK - D3D12GetInterface(ID3D12SDKConfiguration) failed";
    return false;
  }

  hr = sdkConfiguration->SetSDKVersion(sdkVersion, path.string().c_str());
  if (hr != S_OK) {
    LOG_ERROR << "Agility SDK - SetSDKVersion call failed. This method can be used only in "
              << "Windows Developer Mode. Check settings !";
    return false;
  }

  LOG_INFO << "Agility SDK - Loaded SDK version (" << sdkVersion << ")";

  return true;
}

void PlayerManager::LoadDirectML() {
  m_DmlDll = LoadLibrary(".\\D3D12\\DirectML.dll");
  if (!m_DmlDll) {
    LOG_ERROR << "Failed to load DirectML runtime (D3D12\\DirectML.dll)";
    return;
  }
  LOG_INFO << "Loaded DirectML runtime (D3D12\\DirectML.dll)";

  if (Configurator::Get().directx.player.debugLayer) {
    m_DmlDebugDll = LoadLibrary(".\\D3D12\\DirectML.Debug.dll");
    if (!m_DmlDebugDll) {
      LOG_ERROR << "Failed to load DirectML debug runtime (D3D12\\DirectML.Debug.dll)";
      return;
    }
    LOG_INFO << "Loaded DirectML debug runtime (D3D12\\DirectML.Debug.dll)";
  }
}

void PlayerManager::LoadDirectStorage() {
  m_DStorageDll = LoadLibrary(".\\D3D12\\dstorage.dll");
  if (!m_DStorageDll) {
    LOG_ERROR << "DirectStorage - Failed to load (D3D12\\dstorage.dll)";
    return;
  }
  m_DStorageCoreDll = LoadLibrary(".\\D3D12\\dstoragecore.dll");
  if (!m_DStorageCoreDll) {
    LOG_ERROR << "DirectStorage - Failed to load (D3D12\\dstoragecore.dll)";
    return;
  }
  LOG_INFO << "Loaded DirectStorage runtime (D3D12\\dstorage.dll and D3D12\\dstoragecore.dll)";
}

} // namespace DirectX
} // namespace gits
