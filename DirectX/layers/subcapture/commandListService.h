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
#include "commandWriters.h"
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
  unsigned commandListKey{};
  std::vector<std::unique_ptr<D3D12RenderTargetViewState>> renderTargetViews;
  std::unique_ptr<D3D12DepthStencilViewState> depthStencilView;
  bool rtsSingleHandleToDescriptorRange{};
};

struct CommandListClearRenderTargetView : public CommandListCommand {
  CommandListClearRenderTargetView(unsigned key)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARRENDERTARGETVIEW, key) {}
  unsigned commandListKey{};
  std::unique_ptr<D3D12RenderTargetViewState> renderTargetView;
  FLOAT colorRGBA[4]{};
  std::vector<D3D12_RECT> rects{};
};

struct CommandListClearDepthStencilView : public CommandListCommand {
  CommandListClearDepthStencilView(unsigned key)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARDEPTHSTENCILVIEW, key) {}
  unsigned commandListKey{};
  std::unique_ptr<D3D12DepthStencilViewState> depthStencilView;
  FLOAT depth{};
  UINT8 stencil{};
  std::vector<D3D12_RECT> rects{};
};

struct CommandListClearUnorderedAccessViewUint : public CommandListCommand {
  CommandListClearUnorderedAccessViewUint(unsigned key)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWUINT,
                           key) {}
  unsigned commandListKey{};
  std::unique_ptr<D3D12UnorderedAccessViewState> viewGPUHandleInCurrentHeap;
  std::unique_ptr<D3D12UnorderedAccessViewState> viewCPUHandle;
  unsigned resourceKey{};
  UINT values[4]{};
  std::vector<D3D12_RECT> rects{};
};

struct CommandListClearUnorderedAccessViewFloat : public CommandListCommand {
  CommandListClearUnorderedAccessViewFloat(unsigned key)
      : CommandListCommand(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_CLEARUNORDEREDACCESSVIEWFLOAT,
                           key) {}
  unsigned commandListKey{};
  std::unique_ptr<D3D12UnorderedAccessViewState> viewGPUHandleInCurrentHeap;
  std::unique_ptr<D3D12UnorderedAccessViewState> viewCPUHandle;
  unsigned resourceKey{};
  FLOAT values[4]{};
  std::vector<D3D12_RECT> rects{};
};

struct CommandListState : public ObjectState, gits::noncopyable {
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
  std::vector<unsigned> descriptorHeapKeys{};
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
  void restoreCommandState(CommandListClearDepthStencilView* command);
  template <typename CommandListClearUnorderedAccessView>
  void restoreCommandState(CommandListClearUnorderedAccessView* command);
  void initAuxiliaryRtvHeap(unsigned deviceKey);
  void initAuxiliaryDsvHeap(unsigned deviceKey);
  void initAuxiliaryUavGpuHeap(unsigned deviceKey);
  void initAuxiliaryUavCpuHeap(unsigned deviceKey);
  void createAuxiliaryRtv(D3D12RenderTargetViewState* view);
  void createAuxiliaryDsv(D3D12DepthStencilViewState* view);
  void createAuxiliaryUavGpu(D3D12UnorderedAccessViewState* view);
  void createAuxiliaryUavCpu(D3D12UnorderedAccessViewState* view);
  bool equalRtv(D3D12RenderTargetViewState* view, DescriptorState* descriptor);
  bool equalDsv(D3D12DepthStencilViewState* view, DescriptorState* descriptor);
  bool equalUav(D3D12UnorderedAccessViewState* view, DescriptorState* descriptor);

private:
  StateTrackingService& stateService_;
  bool restoreCommandLists_{false};
  std::unordered_map<unsigned, CommandListState*> commandListsByKey_;

  unsigned auxiliaryRtvDescriptorHeapKey_{};
  unsigned auxiliaryRtvDescriptorHeapIndex_{};
  unsigned auxiliaryDsvDescriptorHeapKey_{};
  unsigned auxiliaryDsvDescriptorHeapIndex_{};
  unsigned auxiliaryUavGpuDescriptorHeapKey_{};
  unsigned auxiliaryUavGpuDescriptorHeapIndex_{};
  unsigned auxiliaryUavCpuDescriptorHeapKey_{};
  unsigned auxiliaryUavCpuDescriptorHeapIndex_{};
  const unsigned auxiliaryHeapSize_{96};
};

} // namespace DirectX
} // namespace gits
