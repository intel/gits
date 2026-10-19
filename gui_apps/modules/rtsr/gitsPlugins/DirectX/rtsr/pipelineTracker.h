// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "rtsr/shared/client/rtsrClient.h"
#include "rtsr/shared/protocol/rtsrWireTypes.h"
#include "rtsr/shared/serialization/rtsrSerdeTypes.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace gits {
namespace DirectX {

// Tracks the root signatures and pipeline states of the running stream, reports
// them to the RTSR server, and applies the replacement pipelines the server
// sends back.
//
// Everything is keyed on the GITS object key rather than on the object pointer,
// which RTSR had to use because its interceptor exposed nothing else. Keys are
// stable, cannot be recycled by the allocator, survive a player layer moving
// pipeline creation onto a worker thread, and double as the pipeline id on the
// wire, so every entry in the editor is traceable back to the stream.
//
// Threading: harvesting runs on whichever threads drive the graphics API, which
// during capture is the application's threads, and replacement runs on the
// RTSR receive thread. All public methods are therefore safe to call
// concurrently.
class PipelineTracker {
public:
  explicit PipelineTracker(rtsr::RtsrClient& client);

  // The blob is all a plugin sees of a root signature's contents, so it is
  // deserialized once here and the deserializer is kept alive because the
  // description points into it. The object itself is borrowed, not retained:
  // holding a reference would keep its refcount above zero, so GITS would never
  // report the release that prunes this entry.
  void OnCreateRootSignature(GITSKey key,
                             const void* blob,
                             SIZE_T blobSize,
                             ID3D12RootSignature* rootSignature);

  // Harvest entry points. The live pipeline may still be null when a player
  // layer took creation off the layer chain; it is bound on first use instead.
  void HarvestGraphics(GITSKey key,
                       GITSKey rootSignatureKey,
                       const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
                       ID3D12PipelineState* pipelineState,
                       ID3D12Device* device);
  void HarvestCompute(GITSKey key,
                      GITSKey rootSignatureKey,
                      const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
                      ID3D12PipelineState* pipelineState,
                      ID3D12Device* device);
  void HarvestStream(GITSKey key,
                     GITSKey rootSignatureKey,
                     const D3D12_PIPELINE_STATE_STREAM_DESC* desc,
                     ID3D12PipelineState* pipelineState,
                     ID3D12Device* device);

  // True while either a harvested pipeline is still waiting for its live object
  // or at least one replacement is installed. When false the bind path has
  // nothing to do and can skip the lock entirely, which matters because
  // SetPipelineState is one of the most frequent commands in a frame.
  bool HasBindWork() const {
    return m_UnboundPipelines.load(std::memory_order_relaxed) != 0 ||
           m_ActiveReplacements.load(std::memory_order_relaxed) != 0;
  }

  // Associates the live object with an already harvested pipeline and returns
  // the pipeline that should actually be bound: the replacement when one is
  // installed, otherwise the argument unchanged. One lock covers both, since the
  // caller needs both on the same command.
  //
  // The returned pointer stays valid without holding the lock because a
  // replacement is never released while the session lives, only retired.
  ID3D12PipelineState* OnBindPipeline(GITSKey key, ID3D12PipelineState* pipelineState);

  // Whether this key belongs to a pipeline the server was told about. Asked once
  // per pipeline binding while performance is being measured, because a pipeline
  // the editor never heard of has neither a name nor a shader to show for its
  // cost.
  bool IsTracked(GITSKey key) const;

  // An application supplied debug name replaces the generated one.
  void OnSetName(GITSKey key, const wchar_t* name);

  // Reports the pipeline as gone. Harmless for keys that are not pipelines.
  void OnRelease(GITSKey key);

  // Builds and installs the replacement pipelines the server asks for. Runs on
  // the RTSR receive thread while the server waits for the answer, so the result
  // reflects what actually happened: success only when every requested pipeline
  // was replaced.
  rtsr::RTSR_RESULT ApplyUpdates(const rtsr::RTSR_UPDATE_PIPELINES_REQUEST& request);

  // Drops the replacements for the listed pipelines, restoring the originals.
  rtsr::RTSR_RESULT ResetPipelines(const rtsr::RTSR_RESET_PIPELINES_REQUEST& request);

private:
  struct TrackedRootSignature {
    Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> Deserializer;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* Desc{};
    ID3D12RootSignature* Live{};
  };

  struct TrackedPipeline {
    rtsr::RTSR_PIPELINE_TYPE Type{rtsr::RTSR_PIPELINE_TYPE_UNKNOWN};
    ID3D12PipelineState* Live{};
    ID3D12Device* Device{};
    GITSKey RootSignatureKey{};
    rtsr::RTSR_HASH StateHash{};
    rtsr::RTSR_PIPELINE_INCARNATION Incarnation{};
    std::string Name;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> Replacement;
    // Set once the replacement has actually been bound, so the log can say the
    // pipeline is really being drawn with rather than merely replaced. Cleared
    // whenever a new replacement is installed.
    bool SubstitutionReported{false};
  };

  // What building a replacement needs, copied out under the lock so the pipeline
  // state can be created without holding it.
  struct ReplacementInputs {
    bool Found{false};
    rtsr::RTSR_PIPELINE_TYPE Type{rtsr::RTSR_PIPELINE_TYPE_UNKNOWN};
    ID3D12Device* Device{};
    ID3D12RootSignature* RootSignature{};
    rtsr::RTSR_HASH StateHash{};
    rtsr::RTSR_PIPELINE_INCARNATION Incarnation{};
    std::string Name;
  };

  // Returns the deserializer by value on purpose. The description points into
  // it, so releasing the reference before the description has been consumed
  // would leave the caller holding a dangling pointer if another thread
  // released the root signature meanwhile.
  TrackedRootSignature FindRootSignature(GITSKey key) const;

  ReplacementInputs SnapshotForReplacement(GITSKey key) const;

  // Installs a freshly built replacement, retiring whatever it supersedes.
  // Returns false when the pipeline disappeared or was recreated while it was
  // being built.
  bool InstallReplacement(GITSKey key,
                          rtsr::RTSR_HASH expectedStateHash,
                          rtsr::RTSR_PIPELINE_INCARNATION expectedIncarnation,
                          Microsoft::WRL::ComPtr<ID3D12PipelineState> replacement);

  // Serializes the state, reports it and names it.
  void Publish(GITSKey key,
               GITSKey rootSignatureKey,
               rtsr::RTSR_PIPELINE_TYPE type,
               const rtsr::PipelineStateInfo& info,
               std::vector<rtsr::RTSR_COMMUNICATION_SHADER_STAGE_INFO> stages,
               ID3D12PipelineState* pipelineState,
               ID3D12Device* device);

  rtsr::RtsrClient& m_Client;

  mutable std::mutex m_Mutex;
  std::unordered_map<GITSKey, TrackedRootSignature> m_RootSignatures;
  std::unordered_map<GITSKey, TrackedPipeline> m_Pipelines;

  // Superseded replacements. The GPU may still be executing command lists that
  // reference one, and a plugin cannot know when that work retires without
  // tracking submissions, so they are kept alive for the session instead of
  // being released the moment they are replaced, which is what RTSR does. Shader
  // edits are driven by hand, so this stays small.
  std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_Retired;
  rtsr::RTSR_PIPELINE_INCARNATION m_NextIncarnation{1};

  std::atomic<size_t> m_UnboundPipelines{0};
  std::atomic<size_t> m_ActiveReplacements{0};
};

} // namespace DirectX
} // namespace gits
