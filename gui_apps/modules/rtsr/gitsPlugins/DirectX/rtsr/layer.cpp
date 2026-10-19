// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"

#include "keyUtils.h"
#include "log.h"

#include <windows.h>

#include <filesystem>
#include <vector>

namespace gits {
namespace DirectX {

namespace {

// Identifies this session to the server. RTSR reports the host executable,
// which under GITS would always be gitsPlayer or the recorded game's launcher
// and therefore says nothing useful. The module path still goes in the path
// field, but the name is the one a user recognises.
rtsr::ClientInfo MakeClientInfo() {
  std::vector<char> module(MAX_PATH + 1, '\0');
  GetModuleFileNameA(nullptr, module.data(), static_cast<DWORD>(module.size()));
  const std::filesystem::path modulePath(module.data());

  rtsr::ClientInfo info;
  info.ApplicationPath = modulePath.string();
  info.ApplicationName = modulePath.stem().string();
  info.UsedApi = rtsr::RTSR_API_DX12;
  return info;
}

ID3D12Device* DeviceFromChild(ID3D12DeviceChild* child) {
  // Pipeline-library load commands carry the library rather than the device.
  // Use that always-live input instead of their output PSO, which asynchronous
  // creation deliberately leaves null until the first use.
  if (child == nullptr) {
    return nullptr;
  }
  ID3D12Device* device = nullptr;
  if (FAILED(child->GetDevice(IID_PPV_ARGS(&device)))) {
    return nullptr;
  }
  // GetDevice adds a reference. The device outlives every pipeline created from
  // it, so the raw pointer is released again immediately rather than held.
  device->Release();
  return device;
}

ID3D12PipelineState* AsPipelineState(void** object) {
  if (object == nullptr) {
    return nullptr;
  }
  return static_cast<ID3D12PipelineState*>(*object);
}

// Every D3D12 interface is singly inherited, so the pointer an application asked
// for addresses the same object whichever command list interface it named.
ID3D12GraphicsCommandList* AsCommandList(void** object) {
  if (object == nullptr) {
    return nullptr;
  }
  return static_cast<ID3D12GraphicsCommandList*>(*object);
}

} // namespace

RtsrLayer::RtsrLayer(const rtsr::PluginConfig& config, CGits* gits, bool playback) : Layer("Rtsr") {
  auto logSink = [](std::string_view message) { LOG_INFO << message; };

  m_Client = std::make_unique<rtsr::RtsrClient>(config.Endpoints, logSink);

  // Both have to exist before the connection, because the server can send a
  // request the moment the handshake completes.
  m_Tracker = std::make_unique<PipelineTracker>(*m_Client);
  m_Performance = std::make_unique<PerformanceTracker>(*m_Client, *m_Tracker, gits, playback);

  // All of these run on the transport's receive thread while the server waits
  // for the answer, so the result it gets reflects what actually happened. They
  // touch no command state, only the trackers' own maps and the D3D12 device,
  // whose methods are free threaded.
  rtsr::ClientCallbacks callbacks;
  callbacks.UpdatePipelines = [this](const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request) {
    return m_Tracker->ApplyUpdates(request);
  };
  callbacks.ResetPipelines = [this](const rtsr::RTSR_RESET_PIPELINES_REQUEST& request) {
    return m_Tracker->ResetPipelines(request);
  };
  callbacks.EnablePerformanceMeasurements = [this](bool enable) {
    return m_Performance->SetEnabled(enable);
  };

  m_Enabled = m_Client->Connect(MakeClientInfo(), std::move(callbacks),
                                std::chrono::milliseconds(config.ConnectTimeoutMs));
  if (!m_Enabled) {
    LOG_WARNING << "Rtsr - no RTSR server answered on " << config.Endpoints.Address
                << "; the plugin is disabled and will not affect the stream";
    m_Performance.reset();
    m_Tracker.reset();
    m_Client.reset();
    return;
  }

  LOG_INFO << "Rtsr - connected to the RTSR server, harvesting DirectX 12 pipelines";
}

RtsrLayer::~RtsrLayer() {
  // Order matters. The performance tracker goes first because its worker thread
  // reports through the client, so it has to be joined while the client is still
  // connected. Disconnecting next joins the receive thread, which is what
  // invokes the callbacks that reach into the tracker, so nothing can be running
  // in the tracker by the time it is destroyed. The client is torn down last
  // because the trackers hold references to it, and its own shutdown round-trips
  // a request, which has to happen while the process and winsock are still
  // alive.
  m_Performance.reset();
  if (m_Client) {
    m_Client->Disconnect();
  }
  m_Tracker.reset();
  m_Client.reset();
}

// **********************************************************************************************
// * Root signatures                                                                            *
// **********************************************************************************************

void RtsrLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (!m_Enabled || c.m_Result.Value != S_OK || c.m_ppvRootSignature.Value == nullptr ||
      *c.m_ppvRootSignature.Value == nullptr) {
    return;
  }
  // The object is captured alongside the blob so a replacement pipeline can be
  // built against the original signature instead of a rebuilt copy.
  m_Tracker->OnCreateRootSignature(c.m_ppvRootSignature.Key, c.m_pBlobWithRootSignature.Value,
                                   c.m_blobLengthInBytes.Value,
                                   static_cast<ID3D12RootSignature*>(*c.m_ppvRootSignature.Value));
}

// **********************************************************************************************
// * Harvesting                                                                                 *
// **********************************************************************************************

// None of the harvesting hooks test c.Skip. A player layer may replace a
// creation command with its own implementation and mark it skipped while still
// producing the object, and MultithreadedObjectCreationLayer moves creation to a
// worker thread entirely, leaving the output pointer null. What matters is that a
// description exists; the live object is bound later, on first use.

void RtsrLayer::Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (!m_Enabled || c.m_pDesc.Value == nullptr) {
    return;
  }
  m_Tracker->HarvestGraphics(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey, *c.m_pDesc.Value,
                             AsPipelineState(c.m_ppPipelineState.Value), c.m_Object.Value);
}

void RtsrLayer::Post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (!m_Enabled || c.m_pDesc.Value == nullptr) {
    return;
  }
  m_Tracker->HarvestCompute(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey, *c.m_pDesc.Value,
                            AsPipelineState(c.m_ppPipelineState.Value), c.m_Object.Value);
}

void RtsrLayer::Post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (!m_Enabled) {
    return;
  }
  m_Tracker->HarvestStream(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey, c.m_pDesc.Value,
                           AsPipelineState(c.m_ppPipelineState.Value), c.m_Object.Value);
}

void RtsrLayer::Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (!m_Enabled || c.m_pDesc.Value == nullptr) {
    return;
  }
  auto* pipelineState = AsPipelineState(c.m_ppPipelineState.Value);
  m_Tracker->HarvestGraphics(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey, *c.m_pDesc.Value,
                             pipelineState, DeviceFromChild(c.m_Object.Value));
}

void RtsrLayer::Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (!m_Enabled || c.m_pDesc.Value == nullptr) {
    return;
  }
  auto* pipelineState = AsPipelineState(c.m_ppPipelineState.Value);
  m_Tracker->HarvestCompute(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey, *c.m_pDesc.Value,
                            pipelineState, DeviceFromChild(c.m_Object.Value));
}

void RtsrLayer::Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (!m_Enabled) {
    return;
  }
  auto* pipelineState = AsPipelineState(c.m_ppPipelineState.Value);
  m_Tracker->HarvestStream(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey, c.m_pDesc.Value,
                           pipelineState, DeviceFromChild(c.m_Object.Value));
}

// **********************************************************************************************
// * Metadata and lifetime                                                                      *
// **********************************************************************************************

// Forwarded even when the call itself was skipped: player layers routinely drop
// SetName to avoid stalls, but the name is still what the user sees in the
// editor.
void RtsrLayer::Post(ID3D12ObjectSetNameCommand& c) {
  if (!m_Enabled) {
    return;
  }
  m_Tracker->OnSetName(c.m_Object.Key, c.m_Name.Value);
}

void RtsrLayer::Post(IUnknownReleaseCommand& c) {
  // A non-zero refcount means the object is still alive. Unlike RTSR, this needs
  // no pointer cast or QueryInterface: the key identifies the object, and the
  // tracker simply ignores keys it does not know.
  if (!m_Enabled || c.Skip || c.m_Result.Value != 0) {
    return;
  }
  m_Tracker->OnRelease(c.m_Object.Key);
  // Also drops the query timers of a released command list, and the fence of a
  // released command queue.
  m_Performance->OnRelease(c.m_Object.Key);
}

// **********************************************************************************************
// * Binding and substitution                                                                   *
// **********************************************************************************************

// Substitution rewrites the argument instead of skipping the command and
// re-issuing it, which is what RTSR does. Both the capture wrapper and the
// player runner pass command.m_pPipelineState.Value to the driver, and plugin
// layers run last in the Pre chain, so the rewrite is the last word. It also
// leaves the Post hooks running, which RTSR's approach suppresses, silently
// losing all timing data for exactly the pipelines under study.
//
// Only .Value is rewritten, never .Key, so a capture still records the original
// pipeline: substitution changes what executes, not what is recorded.

void RtsrLayer::Pre(ID3D12DeviceCreateCommandListCommand& c) {
  if (!m_Enabled || c.Skip || !m_Tracker->HasBindWork()) {
    return;
  }
  c.m_pInitialState.Value =
      m_Tracker->OnBindPipeline(c.m_pInitialState.Key, c.m_pInitialState.Value);
}

void RtsrLayer::Pre(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  // The common case is no deferred pipelines and no active replacement, and this
  // is one of the most frequent commands in a frame, so bail out before taking
  // any lock.
  if (!m_Enabled || c.Skip || !m_Tracker->HasBindWork()) {
    return;
  }
  // The key and the live object arrive together here, which is how a pipeline
  // created on a worker thread gets its object without any extra hook.
  c.m_pPipelineState.Value =
      m_Tracker->OnBindPipeline(c.m_pPipelineState.Key, c.m_pPipelineState.Value);
}

void RtsrLayer::Pre(ID3D12GraphicsCommandListResetCommand& c) {
  if (!m_Enabled || c.Skip || !m_Tracker->HasBindWork()) {
    return;
  }
  // Reset takes an optional initial pipeline state, so a null argument here is
  // legitimate and passes straight through.
  c.m_pInitialState.Value =
      m_Tracker->OnBindPipeline(c.m_pInitialState.Key, c.m_pInitialState.Value);
}

// **********************************************************************************************
// * Performance measurement                                                                    *
// **********************************************************************************************

// Every hook here leads with the tracker's own predicate, which is two relaxed
// atomic loads, because these are among the most frequent commands in a frame
// and measurement is off for most of a session.
//
// Every one also insists on the command not being skipped. A skipped command did
// not reach the driver, so a timestamp recorded around it would measure a
// different piece of work than the one it is attributed to, and a resolve
// recorded into a command list that was never closed would be recorded twice.

void RtsrLayer::Post(ID3D12DeviceCreateCommandListCommand& c) {
  // A command list from CreateCommandList is already recording, so it never
  // reaches the reset hook before its first use. CreateCommandList1 needs
  // nothing: it produces a closed list, which has to be reset before use.
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  m_Performance->OnBeginRecording(c.m_ppCommandList.Key, c.m_pInitialState.Key,
                                  AsCommandList(c.m_ppCommandList.Value));
}

void RtsrLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip || c.m_Result.Value != S_OK) {
    return;
  }
  m_Performance->OnBeginRecording(c.m_Object.Key, c.m_pInitialState.Key, c.m_Object.Value);
}

void RtsrLayer::Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip) {
    return;
  }
  // The key of the pipeline the stream asked for, not of the replacement the Pre
  // hook may have bound in its place. An edited pipeline therefore keeps its
  // identity in the plot, which is what makes a before and after comparison
  // possible.
  m_Performance->OnSetPipelineState(c.m_Object.Key, c.m_pPipelineState.Key, c.m_Object.Value);
}

void RtsrLayer::Pre(ID3D12GraphicsCommandListCloseCommand& c) {
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip) {
    return;
  }
  // Pre, because the resolve has to be the last thing recorded into the command
  // list and nothing can be recorded once it is closed.
  m_Performance->OnCommandListClose(c.m_Object.Key, c.m_Object.Value);
}

void RtsrLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  // State restore submissions belong to a subcapture's setup rather than to the
  // frames the user is looking at.
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip || IsStateRestoreKey(c.Key)) {
    return;
  }
  m_Performance->OnExecuteCommandLists(c.m_Object.Value, c.m_Object.Key, c.m_ppCommandLists.Keys);
}

void RtsrLayer::Post(IDXGISwapChainPresentCommand& c) {
  // A test present neither displays nor ends a frame.
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip || c.m_Result.Value != S_OK ||
      (c.m_Flags.Value & DXGI_PRESENT_TEST) != 0 || IsStateRestoreKey(c.Key)) {
    return;
  }
  m_Performance->OnPresent();
}

void RtsrLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (!m_Enabled || !m_Performance->IsActive() || c.Skip || c.m_Result.Value != S_OK ||
      (c.m_PresentFlags.Value & DXGI_PRESENT_TEST) != 0 || IsStateRestoreKey(c.Key)) {
    return;
  }
  m_Performance->OnPresent();
}

} // namespace DirectX
} // namespace gits
