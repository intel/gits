// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directx/debugLayerService.h"

#include <d3d12sdklayers.h>
#include <plog/Log.h>

#include "nvapi.h"

#include <sstream>
#include <string>

namespace directx {

DebugLayerService& DebugLayerService::Get() {
  static DebugLayerService instance;
  return instance;
}

void DebugLayerService::Enable() {
  if (m_Enabled) {
    return;
  }
  m_Enabled = true;

  m_Features.DxgiFactoryDebug = true;
  m_Features.D3D12DebugLayer = true;
  m_Features.GpuBasedValidation = false;
  m_Features.NvApiRaytracingValidation = false;
  m_Features.Dred = false; // not implemented

  const bool needD3D12Debug = m_Features.D3D12DebugLayer || m_Features.GpuBasedValidation;
  Microsoft::WRL::ComPtr<ID3D12Debug> debug;
  if (needD3D12Debug) {
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    if (hr != S_OK) {
      LOG_ERROR << "CCode - D3D12GetDebugInterface (ID3D12Debug) failed";
      return;
    }
    if (m_Features.D3D12DebugLayer) {
      debug->EnableDebugLayer();
      LOG_INFO << "CCode - D3D12 debug layer enabled";
    }
    if (m_Features.DxgiFactoryDebug) {
      LOG_INFO << "CCode - DXGI factory debug enabled";
    }
    if (m_Features.GpuBasedValidation) {
      Microsoft::WRL::ComPtr<ID3D12Debug1> debug1;
      if (SUCCEEDED(debug.As(&debug1))) {
        debug1->SetEnableGPUBasedValidation(TRUE);
        LOG_INFO << "CCode - GPU-based validation enabled";
      }
    }
  }
}

void DebugLayerService::OnDxgiFactoryCreated(REFIID riid,
                                             void** ppFactory,
                                             UINT createFactory2Flags) {
  if (!m_Enabled || !m_Features.DxgiFactoryDebug || !ppFactory || !*ppFactory) {
    return;
  }

  const UINT debugFlags = CombineDxgiFactoryFlags(createFactory2Flags);
  if ((createFactory2Flags & DXGI_CREATE_FACTORY_DEBUG) && debugFlags == createFactory2Flags) {
    return;
  }

  Microsoft::WRL::ComPtr<IUnknown> existing;
  existing.Attach(static_cast<IUnknown*>(*ppFactory));
  *ppFactory = nullptr;

  const HRESULT hr = CreateDXGIFactory2(debugFlags, riid, ppFactory);
  if (hr != S_OK) {
    LOG_ERROR << "CCode - CreateDXGIFactory2 with DXGI debug failed (HRESULT: " << hr
              << "). Ensure Graphics Tools is installed.";
    existing->QueryInterface(riid, ppFactory);
    return;
  }
}

void DebugLayerService::OnDeviceCreated(ID3D12Device* device) {
  if (!m_Enabled || !m_Features.NvApiRaytracingValidation) {
    return;
  }
  InitNvApiValidation(device);
}

void DebugLayerService::FlushRaytracingValidation(IUnknown* object) {
  if (!m_Enabled || !m_Features.NvApiRaytracingValidation || !m_NvApiCallbackRegistered ||
      !object) {
    return;
  }
  ComPtr<ID3D12Device> device;
  if (object->QueryInterface(IID_PPV_ARGS(&device)) != S_OK) {
    ComPtr<ID3D12DeviceChild> deviceChild;
    if (object->QueryInterface(IID_PPV_ARGS(&deviceChild)) != S_OK) {
      return;
    }
    deviceChild->GetDevice(IID_PPV_ARGS(&device));
    if (!device) {
      return;
    }
  }

  ComPtr<ID3D12Device5> device5;
  if (device->QueryInterface(IID_PPV_ARGS(&device5)) != S_OK || !device5) {
    return;
  }
  NvAPI_D3D12_FlushRaytracingValidationMessages(device5.Get());
}

UINT DebugLayerService::CombineDxgiFactoryFlags(UINT capturedFlags) const {
  if (!m_Enabled || !m_Features.DxgiFactoryDebug) {
    return capturedFlags;
  }
  return capturedFlags | DXGI_CREATE_FACTORY_DEBUG;
}

static void __stdcall NvApiRaytracingValidationCallback(
    void* context,
    NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY severity,
    const char* messageCode,
    const char* message,
    const char* messageDetails) {
  std::ostringstream ss;
  ss << "[NvAPI RT validation] ";
  switch (severity) {
  case NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_ERROR:
    ss << "[ERROR] ";
    break;
  case NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_WARNING:
    ss << "[WARNING] ";
    break;
  default:
    ss << "[?] ";
    break;
  }
  ss << "code: " << (messageCode ? messageCode : "") << ", message: " << (message ? message : "")
     << ", details: " << (messageDetails ? messageDetails : "");
  OutputDebugStringA(ss.str().c_str());
  OutputDebugStringA("\n");
}

void DebugLayerService::InitNvApiValidation(ID3D12Device* device) {
  if (!m_Enabled || !m_Features.NvApiRaytracingValidation || !device) {
    return;
  }
  auto status = NvAPI_Initialize();
  if (status == NVAPI_LIBRARY_NOT_FOUND || status == NVAPI_NVIDIA_DEVICE_NOT_FOUND) {
    return;
  } else if (status != NVAPI_OK) {
    LOG_ERROR << "CCode - NvAPI_Initialize failed! NvAPI Raytracing Validation not enabled";
    return;
  }

  ComPtr<ID3D12Device5> device5;
  if (device->QueryInterface(IID_PPV_ARGS(&device5)) != S_OK || !device5) {
    LOG_ERROR << "CCode - ID3D12Device5 not available; NvAPI raytracing validation not enabled.";
    return;
  }

  status = NvAPI_D3D12_EnableRaytracingValidation(device5.Get(),
                                                  NVAPI_D3D12_RAYTRACING_VALIDATION_FLAG_NONE);
  if (status == NVAPI_OK) {
    void* handle{};
    status = NvAPI_D3D12_RegisterRaytracingValidationMessageCallback(
        device5.Get(), NvApiRaytracingValidationCallback, this, &handle);
    if (status != NVAPI_OK) {
      LOG_ERROR << "CCode - NvAPI_D3D12_RegisterRaytracingValidationMessageCallback failed: "
                << status;
    } else {
      m_NvApiCallbackRegistered = true;
      LOG_INFO << "CCode - NvAPI raytracing validation enabled.";
    }
  } else if (status == NVAPI_ACCESS_DENIED) {
    LOG_ERROR << "CCode - NvAPI_D3D12_EnableRaytracingValidation failed! Environment variables: "
                 "NV_ALLOW_RAYTRACING_VALIDATION needs to be set to 1";
  } else {
    LOG_ERROR << "CCode - NvAPI_D3D12_EnableRaytracingValidation failed! Status: " << status;
  }
}

} // namespace directx
