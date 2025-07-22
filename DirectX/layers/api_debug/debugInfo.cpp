// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "debugInfo.h"
#include "log.h"
#include "config.h"
#include "configurationLib.h"
#include "enumToStrAuto.h"
#include "toStr.h"
#include "gits.h"

#include <sstream>
#include <memory>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

DebugInfo::DebugInfo() {}

void DebugInfo::createDXGIFactory(CreateDXGIFactoryCommand& command) {

  HRESULT hr =
      CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, command.riid_.value, command.ppFactory_.value);
  if (hr == S_OK) {
    command.result_.value = hr;
    command.skip = true;
  } else {
    Log(ERR) << "CreateDXGIFactory2 with DXGI_CREATE_FACTORY_DEBUG failed.";
  }
}

void DebugInfo::createDXGIFactory1(CreateDXGIFactory1Command& command) {

  HRESULT hr =
      CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, command.riid_.value, command.ppFactory_.value);
  if (hr == S_OK) {
    command.result_.value = hr;
    command.skip = true;
  } else {
    Log(ERR) << "CreateDXGIFactory2 with DXGI_CREATE_FACTORY_DEBUG failed.";
  }
}

void DebugInfo::createDXGIFactory2(CreateDXGIFactory2Command& command) {
  command.Flags_.value |= DXGI_CREATE_FACTORY_DEBUG;
}

void DebugInfo::createD3D12DevicePre(D3D12CreateDeviceCommand& command) {

  // Enable D3D12 debug layer
  Microsoft::WRL::ComPtr<ID3D12Debug> debug;
  HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
  if (hr != S_OK) {
    Log(ERR) << "D3D12GetDebugInterface (ID3D12Debug) failed.";
    return;
  }
  debug->EnableDebugLayer();

  // Enable DRED
  Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
  hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
  if (hr != S_OK) {
    Log(ERR) << "D3D12GetDebugInterface (ID3D12DeviceRemovedExtendedDataSettings) failed.";
    return;
  }
  dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
}

void DebugInfo::createD3D12DevicePost(D3D12CreateDeviceCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue;
  ID3D12Device* device = static_cast<ID3D12Device*>(*command.ppDevice_.value);
  HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
  if (hr != S_OK) {
    Log(INFOV) << "QueryInterface to ID3D12InfoQueue1 failed.";
    return;
  }
  DWORD callbackCookie{};
  hr = infoQueue->RegisterMessageCallback(debugMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                                          this, &callbackCookie);
  if (hr != S_OK) {
    Log(ERR) << "ID3D12InfoQueue1::RegisterMessageCallback failed.";
  }

  d3d12CallbackRegistered_ = true;

  initNvAPIValidation(device);
}

void DebugInfo::createDMLDevicePre(DMLCreateDeviceCommand& command) {
  command.flags_.value |= DML_CREATE_DEVICE_FLAG_DEBUG;
}

void DebugInfo::createDMLDevice1Pre(DMLCreateDevice1Command& command) {
  command.flags_.value |= DML_CREATE_DEVICE_FLAG_DEBUG;
}

void DebugInfo::checkDXGIDebugInfo(Command& command, IUnknown* object) {
  Microsoft::WRL::ComPtr<IDXGIInfoQueue> infoQueue;
  HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&infoQueue));
  if (hr != S_OK) {
    Microsoft::WRL::ComPtr<IDXGIDeviceSubObject> deviceSubObject;
    hr = object->QueryInterface(IID_PPV_ARGS(&deviceSubObject));
    if (hr == S_OK) {
      Microsoft::WRL::ComPtr<IDXGIDevice> device;
      hr = deviceSubObject->QueryInterface(IID_PPV_ARGS(&device));
      if (hr == S_OK) {
        hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
      }
    }
  }

  if (!infoQueue.Get()) {
    return;
  }

  SIZE_T size = 0;
  hr = infoQueue->GetMessage(DXGI_DEBUG_ALL, 0, nullptr, &size);
  if (hr == S_FALSE) {
    std::unique_ptr<char[]> messageBytes(new char[size]);
    DXGI_INFO_QUEUE_MESSAGE* message =
        reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(messageBytes.get());
    hr = infoQueue->GetMessage(DXGI_DEBUG_ALL, 0, message, &size);
    if (hr == S_OK) {
      traceMessage(static_cast<D3D12_MESSAGE_SEVERITY>(message->Severity), message->pDescription);
    }
  }
}

void DebugInfo::checkD3D12DebugInfo(Command& command, IUnknown* object) {
  flushNvAPIValidation(object);

  if (d3d12CallbackRegistered_) {
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
  HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&infoQueue));
  if (hr != S_OK) {
    Microsoft::WRL::ComPtr<ID3D12DeviceChild> deviceChild;
    hr = object->QueryInterface(IID_PPV_ARGS(&deviceChild));
    if (hr == S_OK) {
      Microsoft::WRL::ComPtr<ID3D12Device> device;
      hr = deviceChild->QueryInterface(IID_PPV_ARGS(&device));
      if (hr == S_OK) {
        hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
      }
    }
  }
  if (!infoQueue.Get()) {
    return;
  }

  UINT64 numStoredMessages = infoQueue->GetNumStoredMessages();
  if (numStoredMessages != UINT64_MAX) {
    for (UINT64 i = 0; i < numStoredMessages; ++i) {
      SIZE_T size = 0;
      HRESULT hr = infoQueue->GetMessage(i, nullptr, &size);
      if (hr == S_FALSE) {
        std::unique_ptr<char[]> messageBytes(new char[size]);
        D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(messageBytes.get());
        hr = infoQueue->GetMessage(i, message, &size);
        if (hr == S_OK) {
          traceMessage(message->Severity, message->pDescription);
        }
      }
    }
    infoQueue->ClearStoredMessages();
  }
}

void __stdcall DebugInfo::debugMessageCallback(D3D12_MESSAGE_CATEGORY category,
                                               D3D12_MESSAGE_SEVERITY severity,
                                               D3D12_MESSAGE_ID id,
                                               LPCSTR description,
                                               void* context) {
  DebugInfo* debugInfo = static_cast<DebugInfo*>(context);
  debugInfo->traceMessage(severity, description);
}

void __stdcall DebugInfo::NvAPIdebugMessageCallback(
    void* context,
    NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY severity,
    const char* messageCode,
    const char* message,
    const char* messageDetails) {
  std::string description;
  description = "NvAPI code: ";
  description += messageCode;
  description += ", message: ";
  description += message;
  description += ", details: ";
  description += messageDetails;

  D3D12_MESSAGE_SEVERITY d3d12Severity{};
  if (severity == NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_ERROR) {
    d3d12Severity = D3D12_MESSAGE_SEVERITY_ERROR;
  } else if (severity == NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_WARNING) {
    d3d12Severity = D3D12_MESSAGE_SEVERITY_WARNING;
  } else {
    Log(ERR) << "Unknown NvAPI message severity: " << severity;
  }

  DebugInfo* debugInfo = static_cast<DebugInfo*>(context);
  debugInfo->traceMessage(d3d12Severity, description.c_str());
}

void DebugInfo::traceMessage(D3D12_MESSAGE_SEVERITY severity, const char* message) {
  auto& cfg = Configurator::Get();

  // Check if the message should be added to the trace file
  if (!cfg.directx.features.trace.enabled) {
    return;
  }
  if (!cfg.directx.features.trace.print.debugLayerWarnings &&
      severity >= D3D12_MESSAGE_SEVERITY_WARNING) {
    return;
  }

  std::string severityStr;
  switch (severity) {
  case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    severityStr = "[CORRUPTION] ";
    break;
  case D3D12_MESSAGE_SEVERITY_ERROR:
    severityStr = "[ERROR] ";
    break;
  case D3D12_MESSAGE_SEVERITY_WARNING:
    severityStr = "[WARNING] ";
    break;
  case D3D12_MESSAGE_SEVERITY_INFO:
    severityStr = "[INFO] ";
    break;
  case D3D12_MESSAGE_SEVERITY_MESSAGE:
    severityStr = "[MESSAGE] ";
    break;
  }

  // The message to be logged in the trace files if DirectX.Features.Trace is enabled
  static auto publisherId = Configurator::IsPlayer() ? PUBLISHER_PLAYER : PUBLISHER_RECORDER;
  CGits::Instance().GetMessageBus().publish(
      {publisherId, TOPIC_LOG},
      std::make_shared<LogMessage>(LogLevel::TRACE, severityStr, message));
}

void DebugInfo::checkD3D12DeviceRemoval(Command& command, IUnknown* object) {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&device));
  if (hr != S_OK) {
    Microsoft::WRL::ComPtr<ID3D12DeviceChild> deviceChild;
    HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&deviceChild));
    if (hr == S_OK) {
      hr = deviceChild->GetDevice(IID_PPV_ARGS(&device));
    }
  }
  if (!device.Get()) {
    return;
  }

  HRESULT deviceRemovedReason = device->GetDeviceRemovedReason();
  if (deviceRemovedReason == S_OK) {
    return;
  }

  static bool dredPrinted = false;
  if (dredPrinted) {
    return;
  }
  dredPrinted = true;

  Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> dred0;
  Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData2> dred2;
  hr = device->QueryInterface(IID_PPV_ARGS(&dred2));
  if (hr != S_OK) {
    hr = device->QueryInterface(IID_PPV_ARGS(&dred0));
    if (hr != S_OK) {
      throw std::runtime_error("D3D12 Device Removed. Stopping.");
    }
  }

  initDredLog();

  ID3D12DeviceRemovedExtendedData* dred{};
  if (dred2.Get()) {
    D3D12_DRED_DEVICE_STATE deviceState = dred2->GetDeviceState();
    dredFile_ << "DRED Device State: " << deviceState << "\n\n";
    dred = dred2.Get();
  } else {
    dred = dred0.Get();
  }

  D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbsOutput;
  GITS_ASSERT(dred != nullptr);
  hr = dred->GetAutoBreadcrumbsOutput(&breadcrumbsOutput);
  if (hr == S_OK) {
    logDredBreadcrumbs(breadcrumbsOutput.pHeadAutoBreadcrumbNode);
  }

  dredFile_ << "\n";

  D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput;
  hr = dred->GetPageFaultAllocationOutput(&pageFaultOutput);
  if (hr == S_OK) {
    logDredPageFaults(pageFaultOutput);
  }
  dredFile_.flush();
}

void DebugInfo::initDredLog() {
  if (dredFile_.is_open()) {
    return;
  }

  std::filesystem::path outputTracePath = Configurator::Get().common.player.outputTracePath;
  outputTracePath.remove_filename();
  std::string streamDir = Configurator::Get().common.player.streamDir.filename().string();
  std::string filenameBase = outputTracePath.string() + streamDir + "_dred";
  std::string fileNum;
  std::string fileExt = ".txt";
  for (int i = 1;; ++i) {
    if (!std::filesystem::exists(filenameBase + fileNum + fileExt)) {
      break;
    }
    fileNum = std::to_string(i);
  }
  dredFile_.open(filenameBase + fileNum + fileExt);
}

void DebugInfo::logDredBreadcrumbs(const D3D12_AUTO_BREADCRUMB_NODE* headNode) {
  if (!headNode) {
    return;
  }

  dredFile_ << "DRED Breadcrumb Report\n";

  const D3D12_AUTO_BREADCRUMB_NODE* node = headNode;
  while (node) {
    unsigned nExecuted = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
    bool hasFinished = (nExecuted > 0) && (nExecuted == node->BreadcrumbCount);

    std::string cmdListName("O");
    if (node->pCommandListDebugNameA) {
      cmdListName = std::string(node->pCommandListDebugNameA);
    } else if (node->pCommandListDebugNameW) {
      std::wstring cmdListNameW = std::wstring(node->pCommandListDebugNameW);
      cmdListName = std::string(cmdListNameW.begin(), cmdListNameW.end());
    }
    std::string cmdQueueName;
    if (node->pCommandQueueDebugNameA) {
      cmdQueueName = std::string(node->pCommandQueueDebugNameA);
    } else if (node->pCommandQueueDebugNameW) {
      std::wstring cmdQueueNameW = std::wstring(node->pCommandQueueDebugNameW);
      cmdQueueName = std::string(cmdQueueNameW.begin(), cmdQueueNameW.end());
    }

    dredFile_ << "  Breadcrumb Node:\n";
    dredFile_ << "    Finished: " << (hasFinished ? "Yes" : "No") << "\n";
    if (!hasFinished && (nExecuted > 0)) {
      dredFile_ << "    Last Executed Operation: " << toStr(node->pCommandHistory[nExecuted - 1])
                << "\n";
    }
    dredFile_ << "    Command Queue: " << cmdQueueName << "\n";
    dredFile_ << "    Command List: " << cmdListName << "\n";
    dredFile_ << "    Operations Executed: " << nExecuted << " out of " << node->BreadcrumbCount
              << "\n";
    dredFile_ << "    Operations (" << node->BreadcrumbCount << "):\n";
    unsigned lastExecuted = UINT_MAX;
    if (nExecuted && nExecuted != node->BreadcrumbCount) {
      lastExecuted = nExecuted - 1;
    }
    for (unsigned i = 0; i < node->BreadcrumbCount; ++i) {
      if (node->pCommandHistory[i] != 0) {
        dredFile_ << "      " << i << ": " << toStr(node->pCommandHistory[i])
                  << (i == lastExecuted ? " LAST EXECUTED" : "") << "\n";
      }
    }

    node = node->pNext;
  }
  dredFile_.flush();
}

void DebugInfo::logDredPageFaults(const D3D12_DRED_PAGE_FAULT_OUTPUT& pageFaultOutput) {
  auto printAllocationNodes = [&](const D3D12_DRED_ALLOCATION_NODE* headNode) {
    if (!headNode) {
      return;
    }
    const D3D12_DRED_ALLOCATION_NODE* node = headNode;
    while (node != nullptr) {
      std::string objName;
      if (node->ObjectNameA) {
        objName = std::string(node->ObjectNameA);
      } else if (node->ObjectNameW) {
        std::wstring objNameW = std::wstring(node->ObjectNameW);
        objName = std::string(objNameW.begin(), objNameW.end());
      }
      dredFile_ << "      " << objName << "\n";
      dredFile_ << "        Type: " << node->AllocationType << "\n";

      node = node->pNext;
    }
  };

  dredFile_ << "DRED Page Fault Report:\n";
  dredFile_ << "  GPU Virtual Address: ";
  dredFile_ << std::hex << pageFaultOutput.PageFaultVA << std::dec << "\n";
  dredFile_ << "  Recent Freed Allocations:\n";
  printAllocationNodes(pageFaultOutput.pHeadRecentFreedAllocationNode);
  dredFile_ << "  Existing Allocations:\n";
  printAllocationNodes(pageFaultOutput.pHeadExistingAllocationNode);
}

void DebugInfo::initNvAPIValidation(ID3D12Device* device) {
  auto status = NvAPI_Initialize();
  if (status == NVAPI_LIBRARY_NOT_FOUND || status == NVAPI_NVIDIA_DEVICE_NOT_FOUND) {
    return;
  } else if (status != NVAPI_OK) {
    Log(ERR) << "NvAPI_Initialize failed! NvAPI Raytracing Validation not enabled";
    return;
  }

  Microsoft::WRL::ComPtr<ID3D12Device5> device5{};
  device->QueryInterface(IID_PPV_ARGS(&device5));
  if (!device5) {
    Log(ERR) << "ID3D12Device5 not available! NvAPI Raytracing Validation not enabled";
    return;
  }

  status = NvAPI_D3D12_EnableRaytracingValidation(device5.Get(),
                                                  NVAPI_D3D12_RAYTRACING_VALIDATION_FLAG_NONE);
  if (status == NVAPI_OK) {
    void* handle{};
    status = NvAPI_D3D12_RegisterRaytracingValidationMessageCallback(
        device5.Get(), NvAPIdebugMessageCallback, this, &handle);
    if (status != NVAPI_OK) {
      Log(ERR) << "NvAPI_D3D12_RegisterRaytracingValidationMessageCallback failed! Status: "
               << status;
    }
  } else if (status == NVAPI_ACCESS_DENIED) {
    Log(ERR) << "NvAPI_D3D12_EnableRaytracingValidation failed! Environment variables: "
                "NV_ALLOW_RAYTRACING_VALIDATION needs to be set to 1";
  } else {
    Log(ERR) << "NvAPI_D3D12_EnableRaytracingValidation failed! Status: " << status;
  }
}

void DebugInfo::flushNvAPIValidation(IUnknown* object) {
  Microsoft::WRL::ComPtr<ID3D12Device> device;
  object->QueryInterface(IID_PPV_ARGS(&device));
  if (!device) {
    Microsoft::WRL::ComPtr<ID3D12DeviceChild> deviceChild;
    object->QueryInterface(IID_PPV_ARGS(&deviceChild));
    if (!deviceChild) {
      return;
    }
    deviceChild->GetDevice(IID_PPV_ARGS(&device));
    if (!device) {
      return;
    }
  }

  Microsoft::WRL::ComPtr<ID3D12Device5> device5;
  device->QueryInterface(IID_PPV_ARGS(&device5));
  if (!device5) {
    return;
  }
  NvAPI_D3D12_FlushRaytracingValidationMessages(device5.Get());
}

} // namespace DirectX
} // namespace gits
