// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "DllOverrideStoreLayer.h"
#include "captureManager.h"
#include "orderingRecorder.h"
#include "commandSerializersCustom.h"
#include "log.h"

#include <fstream>
#include <filesystem>
#include <windows.h>

namespace gits {
namespace DirectX {
DllOverrideStoreLayer::DllOverrideStoreLayer(CaptureManager& manager,
                                             stream::OrderingRecorder& recorder)
    : Layer("DllOverrideStore"), m_Manager(manager), m_Recorder(recorder) {}

void DllOverrideStoreLayer::Post(D3D12CreateDeviceCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12GetDebugInterfaceCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12CreateRootSignatureDeserializerCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12EnableExperimentalFeaturesCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12GetInterfaceCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12SerializeRootSignatureCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12SerializeVersionedRootSignatureCommand& c) {
  captureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Pre(xessGetVersionCommand& c) {
  captureXessDll(c);
}

void DllOverrideStoreLayer::Pre(xessD3D12CreateContextCommand& c) {
  captureXessDll(c);
}

void DllOverrideStoreLayer::Pre(xellGetVersionCommand& c) {
  captureXellDll(c);
}

void DllOverrideStoreLayer::Pre(xellD3D12CreateContextCommand& c) {
  captureXellDll(c);
}

void DllOverrideStoreLayer::Pre(xefgSwapChainGetVersionCommand& c) {
  captureXefgDll(c);
}

void DllOverrideStoreLayer::Pre(xefgSwapChainD3D12CreateContextCommand& c) {
  captureXefgDll(c);
}

void DllOverrideStoreLayer::captureAgilitySDKDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_AgilitySdkDllMutex);
  if (m_AgilitySdkDllChecked) {
    return;
  }

  if (captureDll(L"D3D12Core.dll", c.ThreadId)) {
    m_Manager.updateCommandKey(c);
  }

  m_AgilitySdkDllChecked = true;
}

void DllOverrideStoreLayer::captureXessDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_XessDllMutex);
  if (m_XessDllChecked) {
    return;
  }

  captureDll(L"libxess.dll", c.ThreadId);
  m_XessDllChecked = true;
}

void DllOverrideStoreLayer::captureXellDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_XellDllMutex);
  if (m_XellDllChecked) {
    return;
  }
  captureDll(L"libxell.dll", c.ThreadId);
  m_XellDllChecked = true;
}

void DllOverrideStoreLayer::captureXefgDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_XefgDllMutex);
  if (m_XefgDllChecked) {
    return;
  }
  captureDll(L"libxess_fg.dll", c.ThreadId);
  m_XefgDllChecked = true;
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
  command.Key = m_Manager.createCommandKey();
  command.m_dllName.Value = const_cast<wchar_t*>(dllName.c_str());
  command.m_dllData.Size = size;
  command.m_dllData.Value = content.data();
  m_Recorder.Record(command.Key, new DllContainerMetaSerializer(command));

  return true;
}

} // namespace DirectX
} // namespace gits
