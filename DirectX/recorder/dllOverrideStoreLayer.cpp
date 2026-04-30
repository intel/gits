// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "DllOverrideStoreLayer.h"
#include "CaptureManager.h"
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
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12GetDebugInterfaceCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12CreateRootSignatureDeserializerCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12EnableExperimentalFeaturesCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12GetInterfaceCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12SerializeRootSignatureCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Post(D3D12SerializeVersionedRootSignatureCommand& c) {
  CaptureAgilitySDKDll(c);
}

void DllOverrideStoreLayer::Pre(xessGetVersionCommand& c) {
  CaptureXessDll(c);
}

void DllOverrideStoreLayer::Pre(xessD3D12CreateContextCommand& c) {
  CaptureXessDll(c);
}

void DllOverrideStoreLayer::Pre(xellGetVersionCommand& c) {
  CaptureXellDll(c);
}

void DllOverrideStoreLayer::Pre(xellD3D12CreateContextCommand& c) {
  CaptureXellDll(c);
}

void DllOverrideStoreLayer::Pre(xefgSwapChainGetVersionCommand& c) {
  CaptureXefgDll(c);
}

void DllOverrideStoreLayer::Pre(xefgSwapChainD3D12CreateContextCommand& c) {
  CaptureXefgDll(c);
}

void DllOverrideStoreLayer::CaptureAgilitySDKDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_AgilitySdkDllMutex);
  if (m_AgilitySdkDllChecked) {
    return;
  }

  if (CaptureDll(L"D3D12Core.dll", c.ThreadId)) {
    m_Manager.updateCommandKey(c);
  }

  m_AgilitySdkDllChecked = true;
}

void DllOverrideStoreLayer::CaptureXessDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_XessDllMutex);
  if (m_XessDllChecked) {
    return;
  }

  CaptureDll(L"libxess.dll", c.ThreadId);
  m_XessDllChecked = true;
}

void DllOverrideStoreLayer::CaptureXellDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_XellDllMutex);
  if (m_XellDllChecked) {
    return;
  }
  CaptureDll(L"libxell.dll", c.ThreadId);
  m_XellDllChecked = true;
}

void DllOverrideStoreLayer::CaptureXefgDll(Command& c) {
  std::lock_guard<std::mutex> lock(m_XefgDllMutex);
  if (m_XefgDllChecked) {
    return;
  }
  CaptureDll(L"libxess_fg.dll", c.ThreadId);
  m_XefgDllChecked = true;
}

bool DllOverrideStoreLayer::CaptureDll(const std::wstring& dllName, unsigned threadId) {
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
