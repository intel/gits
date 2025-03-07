// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "debugInfo.h"
#include "log.h"
#include "printCustom.h"
#include "config.h"
#include "printEnumsAuto.h"
#include "gits.h"
#include "configKeySet.h"

#include <sstream>
#include <memory>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

DebugInfo::DebugInfo(FastOStream& traceFile, std::mutex& mutex, bool debugInfoWarning)
    : traceFile_(traceFile), mutex_(mutex), debugInfoWarning_(debugInfoWarning) {
  if (traceFile_.isOpen()) {
    print_ = true;
  }
}

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
  if (!print_) {
    return;
  }
  Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue;
  HRESULT hr = static_cast<ID3D12Device*>(*command.ppDevice_.value)
                   ->QueryInterface(IID_PPV_ARGS(&infoQueue));
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
}

void DebugInfo::createDMLDevicePre(DMLCreateDeviceCommand& command) {
  command.flags_.value |= DML_CREATE_DEVICE_FLAG_DEBUG;
}

void DebugInfo::createDMLDevice1Pre(DMLCreateDevice1Command& command) {
  command.flags_.value |= DML_CREATE_DEVICE_FLAG_DEBUG;
}

void DebugInfo::checkDXGIDebugInfo(Command& command, IUnknown* object) {

  if (!print_) {
    return;
  }

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
      printMessage(command.key, static_cast<D3D12_MESSAGE_SEVERITY>(message->Severity),
                   message->pDescription);
    }
  }
}

void DebugInfo::checkD3D12DebugInfo(Command& command, IUnknown* object) {

  if (d3d12CallbackRegistered_ || !print_) {
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
          printMessage(command.key, message->Severity, message->pDescription);
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
  if (!debugInfo->debugInfoWarning_ && severity == D3D12_MESSAGE_SEVERITY_WARNING) {
    return;
  }

  std::stringstream str;

  switch (severity) {
  case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    str << "[CORRUPTION] ";
    break;
  case D3D12_MESSAGE_SEVERITY_ERROR:
    str << "[ERROR] ";
    break;
  case D3D12_MESSAGE_SEVERITY_WARNING:
    str << "[WARNING] ";
    break;
  case D3D12_MESSAGE_SEVERITY_INFO:
    str << "[INFO] ";
    break;
  case D3D12_MESSAGE_SEVERITY_MESSAGE:
    str << "[MESSAGE] ";
    break;
  }

  str << description << "\n";

  debugInfo->traceFile_ << str.str();
  debugInfo->traceFile_.flush();
}

void DebugInfo::printMessage(unsigned commandKey,
                             D3D12_MESSAGE_SEVERITY severity,
                             const std::string& message) {

  if (!debugInfoWarning_ && severity == D3D12_MESSAGE_SEVERITY_WARNING) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  std::stringstream str;

  str << commandKey << " ";

  switch (severity) {
  case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    str << "[CORRUPTION] ";
    break;
  case D3D12_MESSAGE_SEVERITY_ERROR:
    str << "[ERROR] ";
    break;
  case D3D12_MESSAGE_SEVERITY_WARNING:
    str << "[WARNING] ";
    break;
  case D3D12_MESSAGE_SEVERITY_INFO:
    str << "[INFO] ";
    break;
  case D3D12_MESSAGE_SEVERITY_MESSAGE:
    str << "[MESSAGE] ";
    break;
  }

  str << message << "\n";

  traceFile_ << str.str();
  traceFile_.flush();
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
}

void DebugInfo::initDredLog() {

  if (dredFile_.isOpen()) {
    return;
  }

  std::filesystem::path outputTracePath = Config::Get().common.player.outputTracePath;
  outputTracePath.remove_filename();
  std::string streamDir = Config::Get().common.player.streamDir.filename().string();
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

    std::string cmdListName;
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
      dredFile_ << "    Last Executed Operation: " << node->pCommandHistory[nExecuted - 1] << "\n";
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
        dredFile_ << "      " << i << ": " << node->pCommandHistory[i]
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
  printHex(dredFile_, pageFaultOutput.PageFaultVA) << "\n";
  dredFile_ << "  Recent Freed Allocations:\n";
  printAllocationNodes(pageFaultOutput.pHeadRecentFreedAllocationNode);
  dredFile_ << "  Existing Allocations:\n";
  printAllocationNodes(pageFaultOutput.pHeadExistingAllocationNode);
  dredFile_.flush();
}

} // namespace DirectX
} // namespace gits
