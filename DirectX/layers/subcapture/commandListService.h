// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"
#include "commandIdsAuto.h"
#include "commandWriter.h"
#include "descriptorService.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

struct CommandListCommand {
  CommandListCommand(CommandId id_, unsigned key) : id(id_), commandKey(key) {}
  CommandId id{};
  unsigned commandKey{};
  std::unique_ptr<CommandWriter> commandWriter;
};

struct CommandListOMSetRenderTargets : public CommandListCommand {
  CommandListOMSetRenderTargets(unsigned key)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS, key) {}
  std::vector<std::unique_ptr<D3D12RenderTargetViewState>> renderTargetViews;
  std::unique_ptr<D3D12DepthStencilViewState> depthStencilView;
  unsigned commandListKey{};
  bool rtsSingleHandleToDescriptorRange{};
};

struct CommandListClearRenderTargetView : public CommandListCommand {
  CommandListClearRenderTargetView(unsigned key)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW, key) {}
  std::unique_ptr<D3D12RenderTargetViewState> renderTargetView;
  unsigned commandListKey{};
  FLOAT colorRGBA[4];
  std::vector<D3D12_RECT> rects{};
};

struct CommandListState : public ObjectState, gits::noncopyable {
  CommandListState(StateId id) : ObjectState(id) {}
  ~CommandListState() {
    clearCommands();
  }
  void clearCommands() {
    for (CommandListCommand* command : commands) {
      delete command;
    }
    commands.clear();
  }
  unsigned deviceKey{};
  UINT nodeMask{};
  D3D12_COMMAND_LIST_TYPE type{};
  IID iid{};
  std::vector<CommandListCommand*> commands;
};

struct D3D12CommandListState : public CommandListState {
  D3D12CommandListState() : CommandListState(D3D12_COMMANDLIST) {}
  unsigned allocatorKey{};
  unsigned initialStateKey{};
};

struct D3D12CommandList1State : public CommandListState {
  D3D12CommandList1State() : CommandListState(D3D12_COMMANDLIST1) {}
  D3D12_COMMAND_LIST_FLAGS flags{};
};

class StateTrackingService;

class CommandListService {
public:
  CommandListService(StateTrackingService& stateService);
  void addCommandList(CommandListState* state);
  void removeCommandList(unsigned key);
  void restoreCommandLists();

private:
  void restoreCommandState(CommandListOMSetRenderTargets* command);
  void restoreCommandState(CommandListClearRenderTargetView* command);
  void initAuxiliaryRtvHeap(unsigned deviceKey);
  void initAuxiliaryDsvHeap(unsigned deviceKey);
  void createAuxiliaryRtv(D3D12RenderTargetViewState* view);
  void createAuxiliaryDsv(D3D12DepthStencilViewState* view);
  bool equalRtv(D3D12RenderTargetViewState* view, DescriptorState* descriptor);
  bool equalDsv(D3D12DepthStencilViewState* view, DescriptorState* descriptor);

private:
  StateTrackingService& stateService_;
  bool restoreCommandLists_{false};
  std::unordered_map<unsigned, CommandListState*> commandListsByKey_;
  std::unordered_set<unsigned> commandListsWithoutReset_;

  unsigned auxiliaryRtvDescriptorHeapKey_{};
  unsigned auxiliaryRtvDescriptorHeapIndex_{};
  unsigned auxiliaryDsvDescriptorHeapKey_{};
  unsigned auxiliaryDsvDescriptorHeapIndex_{};
  const unsigned auxiliaryHeapSize_{64};
};

} // namespace DirectX
} // namespace gits
