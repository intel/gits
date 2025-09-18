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
#include "config.h"
#include "log2.h"

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

void DllOverrideStoreLayer::post(ID3D12DeviceFactoryCreateDeviceCommand& c) {
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

  HMODULE dll = LoadLibrary("D3D12Core.dll");
  if (!dll) {
    return;
  }
  char agilitySDKDllPath[MAX_PATH];
  DWORD result = GetModuleFileName(dll, agilitySDKDllPath, MAX_PATH);
  FreeLibrary(dll);
  if (result == 0) {
    LOG_ERROR << "GetModuleFileName failed";
    return;
  }

  if (std::filesystem::exists(agilitySDKDllPath)) {
    std::ifstream file(agilitySDKDllPath, std::ios::binary);

    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0);

    std::vector<char> content(size, 0);
    file.read(content.data(), size);

    DllContainerMetaCommand command(c.threadId);
    command.key = manager_.createCommandKey();
    command.dllName_.value = L"D3D12Core.dll";
    command.dllData_.size = size;
    command.dllData_.value = content.data();

    recorder_.record(command.key, new DllContainerMetaWriter(command));
    manager_.updateCommandKey(c);
  }
  agilitySDKDllchecked_ = true;
}

void DllOverrideStoreLayer::captureXessDll(Command& c) {
  std::lock_guard<std::mutex> lock(xessDllMutex_);
  if (xessDllchecked_) {
    return;
  }

  HMODULE dll = LoadLibrary("libxess.dll");
  if (!dll) {
    return;
  }
  char xessDllPath[MAX_PATH];
  DWORD result = GetModuleFileName(dll, xessDllPath, MAX_PATH);
  FreeLibrary(dll);
  if (result == 0) {
    LOG_ERROR << "GetModuleFileName failed";
    return;
  }

  if (std::filesystem::exists(xessDllPath)) {
    std::ifstream file(xessDllPath, std::ios::binary);

    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0);

    std::vector<char> content(size, 0);
    file.read(content.data(), size);

    DllContainerMetaCommand command(c.threadId);
    command.key = manager_.createCommandKey();
    command.dllName_.value = L"libxess.dll";
    command.dllData_.size = size;
    command.dllData_.value = content.data();

    recorder_.record(command.key, new DllContainerMetaWriter(command));
  }
  xessDllchecked_ = true;
}

} // namespace DirectX
} // namespace gits
