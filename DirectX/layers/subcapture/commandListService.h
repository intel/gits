// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "objectState.h"
#include "commandIdsAuto.h"
#include "commandSerializer.h"
#include "descriptorService.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

struct CommandListCommand {
  CommandListCommand(CommandId id_, CommandKey key, ObjectKey commandListKey)
      : Id(id_), Key(key), CommandListKey(commandListKey) {}
  virtual ~CommandListCommand() = default;
  CommandId Id{};
  CommandKey Key{};
  ObjectKey CommandListKey{};
  std::unique_ptr<stream::CommandSerializer> CommandSerializer;
};

struct CommandListOMSetRenderTargets : public CommandListCommand {
  CommandListOMSetRenderTargets(CommandKey key, ObjectKey commandListKey)
      : CommandListCommand(
            CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS, key, commandListKey) {}
  std::vector<std::unique_ptr<D3D12RenderTargetViewState>> RenderTargetViews;
  std::unique_ptr<D3D12DepthStencilViewState> DepthStencilView;
  bool RtsSingleHandleToDescriptorRange{};
};

struct CommandListClearRenderTargetView : public CommandListCommand {
  CommandListClearRenderTargetView(CommandKey key, ObjectKey commandListKey)
      : CommandListCommand(
            CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW, key, commandListKey) {}
  std::unique_ptr<D3D12RenderTargetViewState> RenderTargetView;
  FLOAT ColorRGBA[4]{};
  std::vector<D3D12_RECT> Rects{};
};

struct CommandListClearDepthStencilView : public CommandListCommand {
  CommandListClearDepthStencilView(CommandKey key, ObjectKey commandListKey)
      : CommandListCommand(
            CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW, key, commandListKey) {}
  std::unique_ptr<D3D12DepthStencilViewState> m_DepthStencilView;
  FLOAT Depth{};
  UINT8 Stencil{};
  std::vector<D3D12_RECT> Rects{};
};

struct CommandListClearUnorderedAccessViewUint : public CommandListCommand {
  CommandListClearUnorderedAccessViewUint(CommandKey key, ObjectKey commandListKey)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT,
                           key,
                           commandListKey) {}
  std::unique_ptr<D3D12UnorderedAccessViewState> ViewGPUHandleInCurrentHeap;
  std::unique_ptr<D3D12UnorderedAccessViewState> ViewCPUHandle;
  ObjectKey ResourceKey{};
  UINT Values[4]{};
  std::vector<D3D12_RECT> Rects{};
};

struct CommandListClearUnorderedAccessViewFloat : public CommandListCommand {
  CommandListClearUnorderedAccessViewFloat(CommandKey key, ObjectKey commandListKey)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT,
                           key,
                           commandListKey) {}
  std::unique_ptr<D3D12UnorderedAccessViewState> ViewGPUHandleInCurrentHeap;
  std::unique_ptr<D3D12UnorderedAccessViewState> ViewCPUHandle;
  ObjectKey ResourceKey{};
  FLOAT Values[4]{};
  std::vector<D3D12_RECT> Rects{};
};

struct CommandListState : public ObjectState {
  CommandListState() = default;
  ~CommandListState() {
    ClearCommands();
  }
  CommandListState(CommandListState&) = delete;
  CommandListState& operator=(const CommandListState&) = delete;

  void ClearCommands() {
    for (CommandListCommand* Command : Commands) {
      delete Command;
    }
    Commands.clear();
  }
  ObjectKey AllocatorKey{};
  UINT NodeMask{};
  D3D12_COMMAND_LIST_TYPE Type{};
  IID Iid{};
  std::vector<CommandListCommand*> Commands;
  std::vector<ObjectKey> DescriptorHeapKeys{};
  bool Closed{};
};

class StateTrackingService;

class CommandListService {
public:
  CommandListService(StateTrackingService& stateService);
  void AddCommandList(CommandListState* state);
  void RemoveCommandList(ObjectKey key);
  void RestoreCommandLists();

private:
  void RestoreCommandState(CommandListOMSetRenderTargets* Command);
  void RestoreCommandState(CommandListClearRenderTargetView* Command);
  void RestoreCommandState(CommandListClearDepthStencilView* Command);
  template <typename CommandListClearUnorderedAccessView>
  void RestoreCommandState(CommandListClearUnorderedAccessView* Command);
  void InitAuxiliaryRtvHeap(ObjectKey deviceKey);
  void InitAuxiliaryDsvHeap(ObjectKey deviceKey);
  void InitAuxiliaryUavGpuHeap(ObjectKey deviceKey);
  void InitAuxiliaryUavCpuHeap(ObjectKey deviceKey);
  void CreateAuxiliaryRtv(D3D12RenderTargetViewState* view);
  void CreateAuxiliaryDsv(D3D12DepthStencilViewState* view);
  void CreateAuxiliaryUavGpu(D3D12UnorderedAccessViewState* view);
  void CreateAuxiliaryUavCpu(D3D12UnorderedAccessViewState* view);
  bool EqualRtv(D3D12RenderTargetViewState* view, DescriptorState* descriptor);
  bool EqualDsv(D3D12DepthStencilViewState* view, DescriptorState* descriptor);
  bool EqualUav(D3D12UnorderedAccessViewState* view, DescriptorState* descriptor);

private:
  StateTrackingService& m_StateService;
  bool m_RestoreCommandLists{false};
  std::unordered_map<ObjectKey, CommandListState*> m_CommandListsByKey;

  ObjectKey m_AuxiliaryRtvDescriptorHeapKey{};
  unsigned m_AuxiliaryRtvDescriptorHeapIndex{};
  ObjectKey m_AuxiliaryDsvDescriptorHeapKey{};
  unsigned m_AuxiliaryDsvDescriptorHeapIndex{};
  ObjectKey m_AuxiliaryUavGpuDescriptorHeapKey{};
  unsigned m_AuxiliaryUavGpuDescriptorHeapIndex{};
  ObjectKey m_AuxiliaryUavCpuDescriptorHeapKey{};
  unsigned m_AuxiliaryUavCpuDescriptorHeapIndex{};
  const unsigned m_AuxiliaryHeapSize{96};
};

} // namespace DirectX
} // namespace gits
