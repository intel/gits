// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "DllOverrideStoreLayer.h"
#include "captureManager.h"
#include "gitsRecorder.h"
#include "commandWritersCustom.h"
#include "log.h"

#include <fstream>
#include <filesystem>
#include <windows.h>

namespace gits {
namespace DirectX {
DllOverrideStoreLayer::DllOverrideStoreLayer(CaptureManager& manager, GitsRecorder& recorder)
    : Layer("DllOverrideStore"), manager_(manager), recorder_(recorder) {}

void DllOverrideStoreLayer::post(D3D12CreateDeviceCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12GetDebugInterfaceCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12CreateRootSignatureDeserializerCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12EnableExperimentalFeaturesCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12GetInterfaceCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12SerializeRootSignatureCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::post(D3D12SerializeVersionedRootSignatureCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::pre(xessGetVersionCommand& c) {
  captureXessDll(c);
}

void DllOverrideStoreLayer::pre(xessD3D12CreateContextCommand& c) {
  captureXessDll(c);
}

void DllOverrideStoreLayer::captureAgilitySDKDll(Command& c) {
  std::lock_guard<std::mutex> lock(agilitySDKDllMutex_);
  if (agilitySDKDllchecked_) {
    return;
  }

  if (captureDll(L"D3D12Core.dll", c.threadId)) {
    manager_.updateCommandKey(c);
  }

  agilitySDKDllchecked_ = true;
}

void DllOverrideStoreLayer::captureXessDll(Command& c) {
  std::lock_guard<std::mutex> lock(xessDllMutex_);
  if (xessDllchecked_) {
    return;
  }

  captureDll(L"libxess.dll", c.threadId);
  xessDllchecked_ = true;
}

bool DllOverrideStoreLayer::captureDll(const std::wstring& dllName, unsigned threadId) {
  HMODULE dll = GetModuleHandleW(dllName.c_str());
  if (!dll) {
    return false;
  }

  char dllPath[MAX_PATH];
  DWORD result = GetModuleFileName(dll, dllPath, MAX_PATH);
  if (result == 0) {
    LOG_ERROR << "GetModuleFileName failed";
    return false;
  }

  if (!std::filesystem::exists(dllPath)) {
    return false;
  }

  std::ifstream file(dllPath, std::ios::binary);
  if (!file) {
    LOG_ERROR << "Failed to open dll file";
    return false;
  }

  file.seekg(0, std::ios::end);
  std::size_t size = file.tellg();
  file.seekg(0);

  std::vector<char> content(size, 0);
  if (!file.read(content.data(), size)) {
    LOG_ERROR << "Failed to read dll file";
    return false;
  }

  DllContainerMetaCommand command(threadId);
  command.key = manager_.createCommandKey();
  command.dllName_.value = const_cast<wchar_t*>(dllName.c_str());
  command.dllData_.size = size;
  command.dllData_.value = content.data();
  recorder_.record(command.key, new DllContainerMetaWriter(command));

  return true;
}

} // namespace DirectX
} // namespace gits
