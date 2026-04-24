// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerService.h"
#include "analyzerResults.h"
#include "keyUtils.h"
#include "log.h"
#include "exception.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

AnalyzerService::AnalyzerService(SubcaptureRange& subcaptureRange,
                                 AnalyzerCommandListService& commandListService,
                                 AnalyzerRaytracingService& raytracingService,
                                 AnalyzerExecuteIndirectService& executeIndirectService)
    : m_SubcaptureRange(subcaptureRange),
      m_CommandListService(commandListService),
      m_RaytracingService(raytracingService),
      m_ExecuteIndirectService(executeIndirectService) {
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
}

AnalyzerService::~AnalyzerService() {
  try {
    if (m_InRange) {
      LOG_ERROR << "Subcapture analysis terminated prematurely";
      DumpAnalysisFile();
    }
  } catch (...) {
    topmost_exception_handler("AnalyzerService::~AnalyzerService");
  }
}

void AnalyzerService::NotifyObject(unsigned objectKey) {
  if (m_Optimize && m_InRange) {
    if (objectKey) {
      m_ObjectsForRestore.insert(objectKey);
    }
  }
}

void AnalyzerService::NotifyObjects(const std::vector<unsigned>& objectKeys) {
  if (m_Optimize && m_InRange) {
    for (unsigned key : objectKeys) {
      if (key) {
        m_ObjectsForRestore.insert(key);
      }
    }
  }
}

void AnalyzerService::CommandListCommand(unsigned commandListKey) {
  if (m_SubcaptureRange.CommandListSubcapture()) {
    return;
  }
  if (m_InRange) {
    auto it = m_CommandListsReset.find(commandListKey);
    if (it == m_CommandListsReset.end()) {
      m_CommandListsForRestore.insert(commandListKey);
    }
  }
}

void AnalyzerService::Present(unsigned callKey, unsigned swapChainKey) {
  if (m_SubcaptureRange.CommandListSubcapture()) {
    m_ObjectsForRestore.insert(swapChainKey);
    m_SubcaptureRange.FrameEnd(IsStateRestoreKey(callKey));
    return;
  }

  if (m_SubcaptureRange.IsFrameRangeStart(IsStateRestoreKey(callKey))) {
    m_InRange = true;
    m_BeforeRange = false;
    ClearReadyExecutables();
    auto& queueEvents = m_GpuExecutionTracker.GetQueueEvents();
    for (auto& commandQueue : queueEvents) {
      for (GpuExecutionTracker::QueueEvent* event : commandQueue.second) {
        std::vector<unsigned>& objectKeys = m_CommandQueueCommandsForRestore[event->CallKey];
        if (event->CommandQueueKey) {
          objectKeys.push_back(event->CommandQueueKey);
        }
        switch (event->Kind) {
        case GpuExecutionTracker::QueueEventKind::Execute: {
          auto* execute = static_cast<ExecuteCommandListCommand*>(event);
          for (unsigned commandListKey : execute->CommandListKeys) {
            m_CommandListsForRestore.insert(commandListKey);
          }
        } break;
        case GpuExecutionTracker::QueueEventKind::Signal: {
          auto* signal = static_cast<GpuExecutionTracker::SignalEvent*>(event);
          objectKeys.push_back(signal->Fence.Key);
        } break;
        case GpuExecutionTracker::QueueEventKind::Wait: {
          auto* wait = static_cast<GpuExecutionTracker::WaitEvent*>(event);
          objectKeys.push_back(wait->Fence.Key);
        } break;
        }
        delete event;
      }
    }
    queueEvents.clear();
    m_SubcaptureRange.FrameEnd(IsStateRestoreKey(callKey));
    return;
  }

  m_SubcaptureRange.FrameEnd(IsStateRestoreKey(callKey));
  if (!m_SubcaptureRange.InRange() && m_InRange) {
    m_CommandListService.CommandListsRestore(m_CommandListsForRestore);
    m_InRange = false;
    DumpAnalysisFile();
  }
}

void AnalyzerService::ExecuteCommandLists(unsigned callKey,
                                          unsigned commandQueueKey,
                                          std::vector<unsigned>& commandListKeys) {
  if (m_SubcaptureRange.CommandListSubcapture()) {
    return;
  }
  if (m_BeforeRange) {
    ClearReadyExecutables();
    if (m_GpuExecutionTracker.IsCommandQueueWaiting(commandQueueKey)) {
      ExecuteCommandListCommand* executable = new ExecuteCommandListCommand{};
      executable->CommandListKeys = commandListKeys;
      m_GpuExecutionTracker.Execute(callKey, commandQueueKey, executable);
    }
  } else if (m_InRange) {
    for (unsigned commandListKey : commandListKeys) {
      auto it = m_CommandListsResetBeforeExecution.find(commandListKey);
      if (it == m_CommandListsResetBeforeExecution.end()) {
        m_CommandListsForRestore.insert(commandListKey);
      }
      m_CommandListsExecuted.insert(commandListKey);
    }
  }
}

void AnalyzerService::CommandListReset(unsigned commandListKey,
                                       unsigned allocatorKey,
                                       unsigned initialStateKey) {
  if (m_SubcaptureRange.CommandListSubcapture()) {
    if (m_InRange) {
      m_ObjectsForRestore.insert(commandListKey);
      m_ObjectsForRestore.insert(allocatorKey);
      m_ObjectsForRestore.insert(initialStateKey);
    }
    return;
  }
  if (m_InRange) {
    auto it = m_CommandListsExecuted.find(commandListKey);
    if (it == m_CommandListsExecuted.end()) {
      m_CommandListsResetBeforeExecution.insert(commandListKey);
    }
    m_CommandListsReset.insert(commandListKey);
  }
}

void AnalyzerService::ExecutionStart() {
  if (m_SubcaptureRange.IsExecutionRangeStart()) {
    m_InRange = true;
    m_BeforeRange = false;
  }
}

void AnalyzerService::ExecutionEnd() {
  if (m_SubcaptureRange.CommandListSubcapture() && !m_SubcaptureRange.InRange() && m_InRange) {
    m_InRange = false;
    DumpAnalysisFile();
  }
}

void AnalyzerService::CommandQueueWait(unsigned callKey,
                                       unsigned commandQueueKey,
                                       unsigned fenceKey,
                                       UINT64 fenceValue) {
  if (m_BeforeRange) {
    m_GpuExecutionTracker.CommandQueueWait(callKey, commandQueueKey, fenceKey, fenceValue);
  }
}

void AnalyzerService::CommandQueueSignal(unsigned callKey,
                                         unsigned commandQueueKey,
                                         unsigned fenceKey,
                                         UINT64 fenceValue) {
  if (m_BeforeRange) {
    m_GpuExecutionTracker.CommandQueueSignal(callKey, commandQueueKey, fenceKey, fenceValue);
  }
}

void AnalyzerService::FenceSignal(unsigned callKey, unsigned fenceKey, UINT64 fenceValue) {
  if (m_BeforeRange) {
    m_GpuExecutionTracker.FenceSignal(callKey, fenceKey, fenceValue);
  }
}

void AnalyzerService::MappedDataMeta(unsigned ResourceKey) {
  if (m_InRange) {
    m_ObjectsForRestore.insert(ResourceKey);
  }
}

void AnalyzerService::CreateXessContext(xessD3D12CreateContextCommand& c) {
  m_ObjectsForRestore.insert(c.m_phContext.Key);
}

void AnalyzerService::CreateXellContext(xellD3D12CreateContextCommand& c) {
  m_ObjectsForRestore.insert(c.m_out_context.Key);
}

void AnalyzerService::CreateXefgContext(xefgSwapChainD3D12CreateContextCommand& c) {
  m_ObjectsForRestore.insert(c.m_phSwapChain.Key);
}

void AnalyzerService::ForceApplicationSwapChainRestore(unsigned key) {
  m_ObjectsForRestore.insert(key);
}

void AnalyzerService::CreateDeviceExtensionContext(
    INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  m_ObjectsForRestore.insert(c.m_ppExtensionContext.Key);
}

void AnalyzerService::CreateDeviceExtensionContext(
    INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  m_ObjectsForRestore.insert(c.m_ppExtensionContext.Key);
}

void AnalyzerService::AddParent(unsigned key, unsigned parentKey) {
  if (key && parentKey) {
    m_ParentKeys[key].push_back(parentKey);
  }
}

void AnalyzerService::ClearReadyExecutables() {
  std::vector<GpuExecutionTracker::Executable*>& executables =
      m_GpuExecutionTracker.GetReadyExecutables();
  for (GpuExecutionTracker::Executable* executable : executables) {
    delete executable;
  }
  executables.clear();
}

void AnalyzerService::DumpAnalysisFile() {
  std::ofstream out(AnalyzerResults::GetAnalysisFileName());

  std::set<unsigned> objectKeys;

  out << "COMMAND_LIST_KEYS\n";
  for (unsigned key : m_CommandListsForRestore) {
    out << key << "\n";
    if (m_Optimize) {
      objectKeys.insert(key);
    }
  }

  out << "COMMAND_QUEUE_COMMANDS\n";
  for (auto& it : m_CommandQueueCommandsForRestore) {
    out << it.first << "\n";
    if (m_Optimize) {
      for (unsigned key : it.second) {
        objectKeys.insert(key);
      }
    }
  }

  m_RaytracingService.Flush();
  m_ExecuteIndirectService.Flush();

  out << "OBJECTS\n";
  for (unsigned key : m_ObjectsForRestore) {
    objectKeys.insert(key);
    FindParents(key, objectKeys);
  }
  for (unsigned key : m_CommandListService.GetObjectsForRestore()) {
    objectKeys.insert(key);
    FindParents(key, objectKeys);
  }
  for (unsigned key : m_RaytracingService.GetBindingTablesResources()) {
    objectKeys.insert(key);
    FindParents(key, objectKeys);
  }
  for (unsigned key : m_ExecuteIndirectService.GetArgumentBuffersResources()) {
    objectKeys.insert(key);
    FindParents(key, objectKeys);
  }
  for (unsigned key : objectKeys) {
    if (key) {
      out << key << "\n";
    }
  }

  out << "DESCRIPTORS\n";
  std::set<std::pair<unsigned, unsigned>> descriptors;
  for (auto& [heapKey, index] : m_CommandListService.GetDescriptors()) {
    descriptors.insert({heapKey, index});
  }
  for (auto& [heapKey, index] : m_RaytracingService.GetBindingTablesDescriptors()) {
    descriptors.insert({heapKey, index});
  }
  for (auto& [heapKey, index] : descriptors) {
    out << heapKey << " " << index << "\n";
  }

  out << "TLASES\n";
  for (unsigned buildKey : m_CommandListService.GetTlases()) {
    out << buildKey << "\n";
  }

  out << "BLASES\n";
  std::set<std::pair<unsigned, unsigned>> ases;
  for (unsigned buildKey : m_CommandListService.GetTlases()) {
    for (auto& blas : m_RaytracingService.GetBlases(buildKey)) {
      ases.insert(blas);
    }
  }
  for (auto& [ResourceKey, offset] : ases) {
    out << ResourceKey << " " << offset << "\n";
  }

  out << "AS_SOURCES\n";
  std::set<std::pair<unsigned, unsigned>>& sources = m_RaytracingService.GetSources();
  for (auto& [ResourceKey, offset] : sources) {
    out << ResourceKey << " " << offset << "\n";
  }
}

void AnalyzerService::FindParents(unsigned key, std::set<unsigned>& objectKeys) {
  auto it = m_ParentKeys.find(key);
  if (it != m_ParentKeys.end()) {
    for (unsigned parentKey : it->second) {
      if (objectKeys.find(parentKey) == objectKeys.end()) {
        objectKeys.insert(parentKey);
        FindParents(parentKey, objectKeys);
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
