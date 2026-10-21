// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineTracker.h"
#include "shaderInfo.h"

#include "rtsr/shared/serialization/rtsrStateSerialization.h"
#include "log.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>
#include <windows.h>

#include <utility>

namespace gits {
namespace DirectX {

namespace {

std::string ToNarrow(const wchar_t* text) {
  if (text == nullptr) {
    return {};
  }
  const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
  if (required <= 1) {
    return {};
  }
  std::string result(static_cast<size_t>(required) - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
  return result;
}

void SetObjectName(ID3D12Object* object, const std::string& name) {
  if (object == nullptr || name.empty()) {
    return;
  }
  const int required = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
  if (required <= 1) {
    return;
  }
  std::wstring wide(static_cast<size_t>(required) - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wide.data(), required);
  object->SetName(wide.c_str());
}

ID3D12Device* DeviceFromChild(ID3D12DeviceChild* child) {
  if (child == nullptr) {
    return nullptr;
  }
  ID3D12Device* device = nullptr;
  if (FAILED(child->GetDevice(IID_PPV_ARGS(&device)))) {
    return nullptr;
  }
  // GetDevice adds a reference. D3D12 keeps the parent available to its device
  // children, so keep a borrowed pointer rather than changing the
  // application's observable refcount.
  device->Release();
  return device;
}

D3D12_SHADER_BYTECODE ToBytecode(const rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO& stage) {
  // Points straight at the request's buffer. The pipeline state is created
  // before the request goes out of scope, and D3D12 copies the bytecode, so no
  // intermediate copy is needed.
  return D3D12_SHADER_BYTECODE{stage.Bytecode.data(), stage.Bytecode.size()};
}

// The round trip preserves the original cached blob's length together with a
// null pointer, which CreateGraphicsPipelineState rejects. RTSR never clears
// this and relies on the applications it has seen not using PSO caching.
template <typename DescT>
void ClearCachedPipelineState(DescT& desc) {
  desc.CachedPSO.pCachedBlob = nullptr;
  desc.CachedPSO.CachedBlobSizeInBytes = 0;
}

// Overwrites the bytecode of every stage the server sent for this pipeline.
// Only one enumerator per distinct value is listed: RTSR_SHADER_STAGE aliases
// several names onto the same value, so naming both would be a duplicate label.
void PatchGraphicsStages(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
                         const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request,
                         const rtsr::RTSR_COMMUNICATION_PIPELINE_UPDATE_INFO& update) {
  for (const uint32_t reference : update.ShaderReferences) {
    if (reference >= request.ShaderStages.size()) {
      continue;
    }
    const auto& stage = request.ShaderStages[reference];
    switch (stage.ShaderStage) {
    case rtsr::RTSR_SHADER_STAGE_VERTEX:
      desc.VS = ToBytecode(stage);
      break;
    case rtsr::RTSR_SHADER_STAGE_PIXEL:
      desc.PS = ToBytecode(stage);
      break;
    case rtsr::RTSR_SHADER_STAGE_GEOMETRY:
      desc.GS = ToBytecode(stage);
      break;
    case rtsr::RTSR_SHADER_STAGE_TESSELLATION_HULL:
      desc.HS = ToBytecode(stage);
      break;
    case rtsr::RTSR_SHADER_STAGE_TESSELLATION_DOMAIN:
      desc.DS = ToBytecode(stage);
      break;
    default:
      break;
    }
  }
}

void PatchComputeStages(D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
                        const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request,
                        const rtsr::RTSR_COMMUNICATION_PIPELINE_UPDATE_INFO& update) {
  for (const uint32_t reference : update.ShaderReferences) {
    if (reference >= request.ShaderStages.size()) {
      continue;
    }
    const auto& stage = request.ShaderStages[reference];
    if (stage.ShaderStage == rtsr::RTSR_SHADER_STAGE_COMPUTE) {
      desc.CS = ToBytecode(stage);
    }
  }
}

} // namespace

PipelineTracker::PipelineTracker(rtsr::RtsrClient& client) : m_Client(client) {}

// **********************************************************************************************
// * Root signatures                                                                            *
// **********************************************************************************************

void PipelineTracker::OnCreateRootSignature(GITSKey key,
                                            const void* blob,
                                            SIZE_T blobSize,
                                            ID3D12RootSignature* rootSignature) {
  if (blob == nullptr || blobSize == 0) {
    return;
  }

  // Safe to call here: the global D3D12 entry points are intercepted on the
  // capture path, but this runs inside a hook, where GITS's own recursion guard
  // already dispatches straight to the system DLL.
  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
  HRESULT hr =
      D3D12CreateVersionedRootSignatureDeserializer(blob, blobSize, IID_PPV_ARGS(&deserializer));
  if (FAILED(hr) || !deserializer) {
    LOG_WARNING << "Rtsr - could not deserialize a root signature, pipelines using it will not "
                   "be harvested";
    return;
  }

  // The unconverted description preserves the original 1.0/1.1/1.2 version,
  // which is what the editor is shown and what round-trips back.
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc = deserializer->GetUnconvertedRootSignatureDesc();
  if (desc == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  auto& tracked = m_RootSignatures[key];
  tracked.Deserializer = std::move(deserializer);
  tracked.Desc = desc;
  tracked.Live = rootSignature;
}

PipelineTracker::TrackedRootSignature PipelineTracker::FindRootSignature(GITSKey key) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = m_RootSignatures.find(key);
  return it == m_RootSignatures.end() ? TrackedRootSignature{} : it->second;
}

// **********************************************************************************************
// * Harvesting                                                                                 *
// **********************************************************************************************

void PipelineTracker::Publish(GITSKey key,
                              GITSKey rootSignatureKey,
                              rtsr::RTSR_PIPELINE_TYPE type,
                              const rtsr::PipelineStateInfo& info,
                              std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages,
                              ID3D12PipelineState* pipelineState,
                              ID3D12Device* device) {
  auto serialized = rtsr::SerializePipelineState(info);
  if (!serialized) {
    LOG_WARNING << "Rtsr - pipeline " << key << " could not be serialized: " << serialized.Error;
    return;
  }

  bool replacedExisting = false;
  rtsr::RTSR_PIPELINE_INCARNATION incarnation{};
  std::string name;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Re-harvesting the same key means the object previously at that key is
    // gone, so its bookkeeping has to be undone before the new one is recorded.
    const auto existing = m_Pipelines.find(key);
    if (existing != m_Pipelines.end()) {
      replacedExisting = true;
      if (existing->second.Live == nullptr) {
        m_UnboundPipelines.fetch_sub(1, std::memory_order_relaxed);
      }
      if (existing->second.Replacement) {
        m_Retired.push_back(std::move(existing->second.Replacement));
        m_ActiveReplacements.fetch_sub(1, std::memory_order_relaxed);
      }
    }

    const auto pending = m_PendingNames.find(key);
    if (pending != m_PendingNames.end()) {
      name = std::move(pending->second);
      m_PendingNames.erase(pending);
    }

    TrackedPipeline tracked;
    tracked.Type = type;
    tracked.Live = pipelineState;
    tracked.Device = device;
    tracked.RootSignatureKey = rootSignatureKey;
    tracked.StateHash = serialized.Value.Hash;
    tracked.Incarnation = m_NextIncarnation++;
    incarnation = tracked.Incarnation;
    tracked.Name = name;
    m_Pipelines[key] = std::move(tracked);

    if (pipelineState == nullptr) {
      m_UnboundPipelines.fetch_add(1, std::memory_order_relaxed);
    }
  }

  // Client calls stay outside the lock: they take the transport's queue mutex,
  // and nesting the two only invites a lock ordering problem later.
  if (replacedExisting) {
    m_Client.PipelineDeleted(key);
  }
  m_Client.PipelineCreated(key, type, serialized.Value.Json, serialized.Value.Hash, incarnation,
                           true, {}, std::move(stages));
  // Leave the live object's debug name alone. The editor shows an application
  // name when the stream recorded one, and "(unnamed)" otherwise.
  if (!name.empty()) {
    m_Client.SetPipelineName(key, name);
  }
}

void PipelineTracker::HarvestGraphics(GITSKey key,
                                      GITSKey rootSignatureKey,
                                      const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
                                      ID3D12PipelineState* pipelineState,
                                      ID3D12Device* device) {
  // Held for the whole call so the description stays alive.
  const TrackedRootSignature rootSignature = FindRootSignature(rootSignatureKey);
  if (rootSignature.Desc == nullptr) {
    LOG_DEBUG << "Rtsr - skipping graphics pipeline " << key
              << ": its root signature was never seen";
    return;
  }

  rtsr::PipelineStateInfo info{};
  info.pRootSignatureDesc = rootSignature.Desc;
  info.Type = rtsr::PipelineStateType::Graphics;
  info.pGraphicsPipelineStateDesc = &desc;

  std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages;
  AppendGraphicsShaderStages(stages, desc);

  Publish(key, rootSignatureKey, rtsr::RTSR_PIPELINE_TYPE_GRAPHICS, info, std::move(stages),
          pipelineState, device != nullptr ? device : DeviceFromChild(rootSignature.Live));
}

void PipelineTracker::HarvestCompute(GITSKey key,
                                     GITSKey rootSignatureKey,
                                     const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
                                     ID3D12PipelineState* pipelineState,
                                     ID3D12Device* device) {
  const TrackedRootSignature rootSignature = FindRootSignature(rootSignatureKey);
  if (rootSignature.Desc == nullptr) {
    LOG_DEBUG << "Rtsr - skipping compute pipeline " << key
              << ": its root signature was never seen";
    return;
  }

  rtsr::PipelineStateInfo info{};
  info.pRootSignatureDesc = rootSignature.Desc;
  info.Type = rtsr::PipelineStateType::Compute;
  info.pComputePipelineStateDesc = &desc;

  std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages;
  AppendComputeShaderStages(stages, desc);

  Publish(key, rootSignatureKey, rtsr::RTSR_PIPELINE_TYPE_COMPUTE, info, std::move(stages),
          pipelineState, device != nullptr ? device : DeviceFromChild(rootSignature.Live));
}

void PipelineTracker::HarvestStream(GITSKey key,
                                    GITSKey rootSignatureKey,
                                    const D3D12_PIPELINE_STATE_STREAM_DESC* desc,
                                    ID3D12PipelineState* pipelineState,
                                    ID3D12Device* device) {
  if (desc == nullptr) {
    return;
  }

  CD3DX12_PIPELINE_STATE_STREAM_PARSE_HELPER helper;
  if (FAILED(D3DX12ParsePipelineStream(*desc, &helper))) {
    LOG_DEBUG << "Rtsr - skipping pipeline " << key << ": its state stream could not be parsed";
    return;
  }

  // Discriminating on which shader is present matches RTSR. Mesh and
  // amplification pipelines fall through both branches: the parse helper drops
  // those subobjects and the wire format has no representation for them.
  if (D3D12_SHADER_BYTECODE(helper.PipelineStream.VS).pShaderBytecode != nullptr) {
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsDesc = helper.PipelineStream.GraphicsDescV0();
    HarvestGraphics(key, rootSignatureKey, graphicsDesc, pipelineState, device);
  } else if (D3D12_SHADER_BYTECODE(helper.PipelineStream.CS).pShaderBytecode != nullptr) {
    const D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = helper.PipelineStream.ComputeDescV0();
    HarvestCompute(key, rootSignatureKey, computeDesc, pipelineState, device);
  } else {
    LOG_DEBUG << "Rtsr - skipping pipeline " << key
              << ": neither a vertex nor a compute shader, mesh pipelines are not supported";
  }
}

// **********************************************************************************************
// * Binding                                                                                    *
// **********************************************************************************************

ID3D12PipelineState* PipelineTracker::OnBindPipeline(GITSKey key,
                                                     ID3D12PipelineState* pipelineState) {
  ID3D12PipelineState* toBind = pipelineState;
  bool reportSubstitution = false;
  std::string name;

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Pipelines.find(key);
    if (it == m_Pipelines.end()) {
      return pipelineState;
    }

    if (it->second.Live == nullptr && pipelineState != nullptr) {
      it->second.Live = pipelineState;
      m_UnboundPipelines.fetch_sub(1, std::memory_order_relaxed);
    }
    // Defensive fallback for any deferred creation path whose creation command
    // did not expose a device. GetDevice adds a temporary reference; the
    // pipeline itself guarantees the borrowed parent remains valid.
    if (it->second.Device == nullptr && pipelineState != nullptr) {
      ID3D12Device* device = nullptr;
      if (SUCCEEDED(pipelineState->GetDevice(IID_PPV_ARGS(&device)))) {
        it->second.Device = device;
        device->Release();
      }
    }

    if (it->second.Replacement) {
      toBind = it->second.Replacement.Get();
      if (!it->second.SubstitutionReported) {
        it->second.SubstitutionReported = true;
        reportSubstitution = true;
        name = it->second.Name;
      }
    }
  }

  // Once per installed replacement, so it stays readable even though this runs
  // per draw. Creating a replacement only proves the server was answered; this
  // is the line that proves the stream is actually drawing with it.
  if (reportSubstitution) {
    LOG_INFO << "Rtsr - pipeline " << key << " (" << name
             << ") is now being drawn with its replacement";
  }

  return toBind;
}

// **********************************************************************************************
// * Metadata and lifetime                                                                      *
// **********************************************************************************************

bool PipelineTracker::IsTracked(GITSKey key) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Pipelines.find(key) != m_Pipelines.end();
}

void PipelineTracker::OnSetName(GITSKey key, const wchar_t* name) {
  std::string narrow = ToNarrow(name);
  if (narrow.empty()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Pipelines.find(key);
    if (it == m_Pipelines.end()) {
      m_PendingNames[key] = std::move(narrow);
      return;
    }
    it->second.Name = narrow;
  }

  m_Client.SetPipelineName(key, narrow);
}

void PipelineTracker::OnRelease(GITSKey key) {
  bool wasPipeline = false;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);

    const auto it = m_Pipelines.find(key);
    if (it != m_Pipelines.end()) {
      wasPipeline = true;
      if (it->second.Live == nullptr) {
        m_UnboundPipelines.fetch_sub(1, std::memory_order_relaxed);
      }
      if (it->second.Replacement) {
        m_Retired.push_back(std::move(it->second.Replacement));
        m_ActiveReplacements.fetch_sub(1, std::memory_order_relaxed);
      }
      m_Pipelines.erase(it);
    }

    // Root signatures are dropped here too. RTSR never removed them, so its map
    // grew for the lifetime of the process and its raw pointer keys could be
    // reused by a later allocation.
    m_RootSignatures.erase(key);
    m_PendingNames.erase(key);
  }

  if (wasPipeline) {
    m_Client.PipelineDeleted(key);
  }
}

// **********************************************************************************************
// * Replacement                                                                                *
// **********************************************************************************************

PipelineTracker::ReplacementInputs PipelineTracker::SnapshotForReplacement(GITSKey key) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  ReplacementInputs inputs;
  const auto pipeline = m_Pipelines.find(key);
  if (pipeline == m_Pipelines.end()) {
    return inputs;
  }

  inputs.Found = true;
  inputs.Type = pipeline->second.Type;
  inputs.Device = pipeline->second.Device;
  inputs.StateHash = pipeline->second.StateHash;
  inputs.Incarnation = pipeline->second.Incarnation;
  inputs.Name = pipeline->second.Name;

  const auto rootSignature = m_RootSignatures.find(pipeline->second.RootSignatureKey);
  if (rootSignature != m_RootSignatures.end()) {
    inputs.RootSignature = rootSignature->second.Live;
  }
  return inputs;
}

bool PipelineTracker::InstallReplacement(GITSKey key,
                                         rtsr::RTSR_HASH expectedStateHash,
                                         rtsr::RTSR_PIPELINE_INCARNATION expectedIncarnation,
                                         Microsoft::WRL::ComPtr<ID3D12PipelineState> replacement) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  const auto it = m_Pipelines.find(key);
  if (it == m_Pipelines.end() || it->second.StateHash != expectedStateHash ||
      it->second.Incarnation != expectedIncarnation) {
    // Released while the replacement was being built. It was never bound, so
    // letting it go here is safe.
    return false;
  }

  if (it->second.Replacement) {
    m_Retired.push_back(std::move(it->second.Replacement));
  } else {
    m_ActiveReplacements.fetch_add(1, std::memory_order_relaxed);
  }
  it->second.Replacement = std::move(replacement);
  it->second.SubstitutionReported = false;
  return true;
}

rtsr::RTSR_RESULT PipelineTracker::ApplyUpdates(
    const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request) {
  if (request.UpdatedPipelines.size() != 1) {
    LOG_WARNING << "Rtsr - replacement requests must contain exactly one pipeline";
    return rtsr::RTSR_RESULT_FAIL;
  }
  bool allSucceeded = true;

  for (const auto& update : request.UpdatedPipelines) {
    const auto key = static_cast<GITSKey>(update.Id);
    const ReplacementInputs inputs = SnapshotForReplacement(key);
    if (!inputs.Found) {
      LOG_WARNING << "Rtsr - the server asked to update unknown pipeline " << update.Id;
      allSucceeded = false;
      continue;
    }
    if (inputs.Device == nullptr) {
      LOG_WARNING << "Rtsr - cannot replace pipeline " << update.Id << ": its device is not known";
      allSucceeded = false;
      continue;
    }
    if (inputs.StateHash != update.StateHash) {
      LOG_WARNING << "Rtsr - cannot replace pipeline " << update.Id
                  << ": the pipeline id now identifies different state";
      allSucceeded = false;
      continue;
    }
    if (inputs.Incarnation != update.Incarnation) {
      LOG_WARNING << "Rtsr - cannot replace pipeline " << update.Id
                  << ": the pipeline id now identifies a new object";
      allSucceeded = false;
      continue;
    }
    // The original object is reused rather than rebuilt from the round-tripped
    // description. The editor returns the state unchanged, so the description is
    // the one already harvested, and rebuilding it would mean calling
    // D3D12SerializeVersionedRootSignature, which GITS intercepts.
    if (inputs.RootSignature == nullptr) {
      LOG_WARNING << "Rtsr - cannot replace pipeline " << update.Id
                  << ": its root signature is no longer available";
      allSucceeded = false;
      continue;
    }

    auto deserialized = rtsr::DeserializePipelineState(update.State);
    if (!deserialized) {
      LOG_WARNING << "Rtsr - cannot replace pipeline " << update.Id
                  << ": its state could not be read back: " << deserialized.Error;
      allSucceeded = false;
      continue;
    }

    // Built without holding the lock: the driver compiles shaders here, and
    // stalling every SetPipelineState for that long would be worse than making
    // the server wait.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> replacement;
    HRESULT hr = E_FAIL;

    if (inputs.Type == rtsr::RTSR_PIPELINE_TYPE_GRAPHICS &&
        deserialized.Value->Type == rtsr::PipelineStateType::Graphics) {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = *deserialized.Value->pGraphicsPipelineStateDesc;
      PatchGraphicsStages(desc, request, update);
      desc.pRootSignature = inputs.RootSignature;
      ClearCachedPipelineState(desc);
      hr = inputs.Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&replacement));
    } else if (inputs.Type == rtsr::RTSR_PIPELINE_TYPE_COMPUTE &&
               deserialized.Value->Type == rtsr::PipelineStateType::Compute) {
      D3D12_COMPUTE_PIPELINE_STATE_DESC desc = *deserialized.Value->pComputePipelineStateDesc;
      PatchComputeStages(desc, request, update);
      desc.pRootSignature = inputs.RootSignature;
      ClearCachedPipelineState(desc);
      hr = inputs.Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&replacement));
    } else {
      LOG_WARNING << "Rtsr - cannot replace pipeline " << update.Id
                  << ": unsupported or mismatched pipeline type";
      allSucceeded = false;
      continue;
    }

    if (FAILED(hr) || !replacement) {
      LOG_WARNING << "Rtsr - creating the replacement for pipeline " << update.Id << " ("
                  << inputs.Name << ") failed with HRESULT 0x" << std::hex << hr;
      allSucceeded = false;
      continue;
    }

    if (!inputs.Name.empty()) {
      SetObjectName(replacement.Get(), inputs.Name + "-updated");
    }

    if (!InstallReplacement(key, update.StateHash, update.Incarnation, std::move(replacement))) {
      LOG_WARNING << "Rtsr - pipeline " << update.Id
                  << " was released or recreated while its replacement was being built";
      allSucceeded = false;
      continue;
    }

    LOG_INFO << "Rtsr - replaced pipeline " << update.Id << " (" << inputs.Name << ")";
  }

  return allSucceeded ? rtsr::RTSR_RESULT_SUCCESS : rtsr::RTSR_RESULT_FAIL;
}

rtsr::RTSR_RESULT PipelineTracker::ResetPipelines(
    const rtsr::RTSR_RESET_PIPELINES_REQUEST& request) {
  size_t resetCount = 0;
  bool allMatched = true;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& resetInfo : request.Pipelines) {
      const auto it = m_Pipelines.find(static_cast<GITSKey>(resetInfo.Id));
      if (it == m_Pipelines.end() || it->second.Incarnation != resetInfo.Incarnation) {
        allMatched = false;
        continue;
      }
      if (!it->second.Replacement) {
        continue;
      }
      m_Retired.push_back(std::move(it->second.Replacement));
      it->second.SubstitutionReported = false;
      m_ActiveReplacements.fetch_sub(1, std::memory_order_relaxed);
      ++resetCount;
    }
  }

  LOG_INFO << "Rtsr - restored " << resetCount << " pipeline(s) to their original state";
  return allMatched ? rtsr::RTSR_RESULT_SUCCESS : rtsr::RTSR_RESULT_FAIL;
}

} // namespace DirectX
} // namespace gits
