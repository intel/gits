// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"
#include "gits.h"
#include "IPlugin.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

PlayerManager* PlayerManager::instance_ = nullptr;

PlayerManager& PlayerManager::get() {
  if (!instance_) {
    instance_ = new PlayerManager();
    CGits::Instance().GetMessageBus().subscribe(
        {PUBLISHER_PLAYER, TOPIC_END}, [](Topic t, const MessagePtr& m) {
          auto msg = std::dynamic_pointer_cast<PlaybackEndMessage>(m);
          if (msg) {
            PlayerManager::destroy();
          }
        });
  }
  return *instance_;
}

PlayerManager::~PlayerManager() {
  try {
    LOG_INFO << "PlayerManager: Playback completed. Cleaning up...";
    multithreadedObjectCreationService_.shutdown();
    pluginService_.reset();
  } catch (...) {
    topmost_exception_handler("PlayerManager::~PlayerManager");
  }

  if (d3d12CoreDll_) {
    FreeLibrary(d3d12CoreDll_);
  }
  if (dmlDll_) {
    FreeLibrary(dmlDll_);
  }
  if (dmlDebugDll_) {
    FreeLibrary(dmlDebugDll_);
  }
  if (dStorageDll_) {
    FreeLibrary(dStorageDll_);
  }
  if (dStorageCoreDll_) {
    FreeLibrary(dStorageCoreDll_);
  }
}

PlayerManager::PlayerManager() {
  auto& cfg = Configurator::Get();
  executeCommands_ = cfg.directx.player.execute;
  multithreadedShaderCompilation_ =
      cfg.directx.player.multithreadedShaderCompilation && !cfg.directx.features.subcapture.enabled;

  // Load DirectX runtimes
  loadDirectML();
  loadDirectStorage();

  getAdapterService().loadAdapters();
  getIntelExtensionsService().loadIntelExtensions(getAdapterService().getAdapter());
  getIntelExtensionsService().setApplicationInfo();

  pluginService_ = std::make_unique<PluginService>();
  pluginService_->loadPlugins();

  layerManager_.loadLayers(*this, *pluginService_.get());
}

void PlayerManager::flushMultithreadedShaderCompilation() {
  const auto& results = multithreadedObjectCreationService_.completeAll();

  for (const auto& [key, output] : results) {
    if (output.result != S_OK) {
      continue;
    }

    addObject(key, static_cast<IUnknown*>(output.object));
  }
}

void PlayerManager::addObject(unsigned objectKey, IUnknown* object) {
  objects_[objectKey] = object;
}

void PlayerManager::removeObject(unsigned objectKey) {
  objects_.erase(objectKey);
}

IUnknown* PlayerManager::findObject(unsigned objectKey) {
  auto it = objects_.find(objectKey);
  if (it == objects_.end()) {
    return nullptr;
  }
  return it->second;
}

bool PlayerManager::loadAgilitySdk(const std::filesystem::path& path) {
  std::string dllPath = (path / "D3D12Core.dll").string();
  d3d12CoreDll_ = LoadLibrary(dllPath.c_str());
  if (!d3d12CoreDll_) {
    LOG_ERROR << "Agility SDK - Failed to load (" << dllPath << ")";
    return false;
  }
  UINT sdkVersion = *reinterpret_cast<UINT*>(GetProcAddress(d3d12CoreDll_, "D3D12SDKVersion"));

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

void PlayerManager::loadDirectML() {
  dmlDll_ = LoadLibrary(".\\D3D12\\DirectML.dll");
  if (!dmlDll_) {
    LOG_ERROR << "Failed to load DirectML runtime (D3D12\\DirectML.dll)";
    return;
  }
  LOG_INFO << "Loaded DirectML runtime (D3D12\\DirectML.dll)";

  if (Configurator::Get().directx.player.debugLayer) {
    dmlDebugDll_ = LoadLibrary(".\\D3D12\\DirectML.Debug.dll");
    if (!dmlDebugDll_) {
      LOG_ERROR << "Failed to load DirectML debug runtime (D3D12\\DirectML.Debug.dll)";
      return;
    }
    LOG_INFO << "Loaded DirectML debug runtime (D3D12\\DirectML.Debug.dll)";
  }
}

void PlayerManager::loadDirectStorage() {
  dStorageDll_ = LoadLibrary(".\\D3D12\\dstorage.dll");
  if (!dStorageDll_) {
    LOG_ERROR << "DirectStorage - Failed to load (D3D12\\dstorage.dll)";
    return;
  }
  dStorageCoreDll_ = LoadLibrary(".\\D3D12\\dstoragecore.dll");
  if (!dStorageCoreDll_) {
    LOG_ERROR << "DirectStorage - Failed to load (D3D12\\dstoragecore.dll)";
    return;
  }
  LOG_INFO << "Loaded DirectStorage runtime (D3D12\\dstorage.dll and D3D12\\dstoragecore.dll)";
}

} // namespace DirectX
} // namespace gits
