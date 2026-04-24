// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  CommandListCommand(CommandId id_, unsigned key, unsigned clKey)
      : m_Id(id_), m_CommandKey(key), m_CommandListKey(clKey) {}
  virtual ~CommandListCommand() = default;
  CommandId m_Id{};
  unsigned m_CommandKey{};
  unsigned m_CommandListKey{};
  std::unique_ptr<stream::CommandSerializer> m_CommandSerializer;
};

struct CommandListOMSetRenderTargets : public CommandListCommand {
  CommandListOMSetRenderTargets(unsigned key, unsigned commandListKey)
      : CommandListCommand(
            CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS, key, commandListKey) {}
  std::vector<std::unique_ptr<D3D12RenderTargetViewState>> m_RenderTargetViews;
  std::unique_ptr<D3D12DepthStencilViewState> m_DepthStencilView;
  bool m_RtsSingleHandleToDescriptorRange{};
};

struct CommandListClearRenderTargetView : public CommandListCommand {
  CommandListClearRenderTargetView(unsigned key, unsigned commandListKey)
      : CommandListCommand(
            CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW, key, commandListKey) {}
  std::unique_ptr<D3D12RenderTargetViewState> m_RenderTargetView;
  FLOAT m_ColorRGBA[4]{};
  std::vector<D3D12_RECT> m_Rects{};
};

struct CommandListClearDepthStencilView : public CommandListCommand {
  CommandListClearDepthStencilView(unsigned key, unsigned commandListKey)
      : CommandListCommand(
            CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW, key, commandListKey) {}
  std::unique_ptr<D3D12DepthStencilViewState> m_DepthStencilView;
  FLOAT m_Depth{};
  UINT8 m_Stencil{};
  std::vector<D3D12_RECT> m_Rects{};
};

struct CommandListClearUnorderedAccessViewUint : public CommandListCommand {
  CommandListClearUnorderedAccessViewUint(unsigned key, unsigned commandListKey)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT,
                           key,
                           commandListKey) {}
  std::unique_ptr<D3D12UnorderedAccessViewState> m_ViewGPUHandleInCurrentHeap;
  std::unique_ptr<D3D12UnorderedAccessViewState> m_ViewCPUHandle;
  unsigned m_ResourceKey{};
  UINT m_Values[4]{};
  std::vector<D3D12_RECT> m_Rects{};
};

struct CommandListClearUnorderedAccessViewFloat : public CommandListCommand {
  CommandListClearUnorderedAccessViewFloat(unsigned key, unsigned commandListKey)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT,
                           key,
                           commandListKey) {}
  std::unique_ptr<D3D12UnorderedAccessViewState> m_ViewGPUHandleInCurrentHeap;
  std::unique_ptr<D3D12UnorderedAccessViewState> m_ViewCPUHandle;
  unsigned m_ResourceKey{};
  FLOAT m_Values[4]{};
  std::vector<D3D12_RECT> m_Rects{};
};

struct CommandListState : public ObjectState {
  CommandListState() = default;
  ~CommandListState() {
    ClearCommands();
  }
  CommandListState(CommandListState&) = delete;
  CommandListState& operator=(const CommandListState&) = delete;

  void ClearCommands() {
    for (CommandListCommand* Command : m_Commands) {
      delete Command;
    }
    m_Commands.clear();
  }
  unsigned m_AllocatorKey{};
  UINT m_NodeMask{};
  D3D12_COMMAND_LIST_TYPE m_Type{};
  IID m_Iid{};
  std::vector<CommandListCommand*> m_Commands;
  std::vector<unsigned> m_DescriptorHeapKeys{};
  bool m_Closed{};
};

class StateTrackingService;

class CommandListService {
public:
  CommandListService(StateTrackingService& stateService);
  void AddCommandList(CommandListState* state);
  void RemoveCommandList(unsigned key);
  void RestoreCommandLists();

private:
  void RestoreCommandState(CommandListOMSetRenderTargets* Command);
  void RestoreCommandState(CommandListClearRenderTargetView* Command);
  void RestoreCommandState(CommandListClearDepthStencilView* Command);
  template <typename CommandListClearUnorderedAccessView>
  void RestoreCommandState(CommandListClearUnorderedAccessView* Command);
  void InitAuxiliaryRtvHeap(unsigned DeviceKey);
  void InitAuxiliaryDsvHeap(unsigned DeviceKey);
  void InitAuxiliaryUavGpuHeap(unsigned DeviceKey);
  void InitAuxiliaryUavCpuHeap(unsigned DeviceKey);
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
  std::unordered_map<unsigned, CommandListState*> m_CommandListsByKey;

  unsigned m_AuxiliaryRtvDescriptorHeapKey{};
  unsigned m_AuxiliaryRtvDescriptorHeapIndex{};
  unsigned m_AuxiliaryDsvDescriptorHeapKey{};
  unsigned m_AuxiliaryDsvDescriptorHeapIndex{};
  unsigned m_AuxiliaryUavGpuDescriptorHeapKey{};
  unsigned m_AuxiliaryUavGpuDescriptorHeapIndex{};
  unsigned m_AuxiliaryUavCpuDescriptorHeapKey{};
  unsigned m_AuxiliaryUavCpuDescriptorHeapIndex{};
  const unsigned m_AuxiliaryHeapSize{96};
};

} // namespace DirectX
} // namespace gits
