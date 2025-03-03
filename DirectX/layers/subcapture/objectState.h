// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "intelExtensions.h"
#include "directx.h"

#include <vector>
#include <memory>
#include <string>
#include <map>

namespace gits {
namespace DirectX {

struct ObjectState {
  enum StateId {
    DXGI_FACTORY,
    DXGI_ADAPTER,
    DXGI_ADAPTERBYLUID,
    DXGI_SWAPCHAIN,
    DXGI_SWAPCHAINFORHWND,
    DXGI_SWAPCHAINBUFFER,
    DXGI_GETPARENT,
    D3D12_DEVICE,
    D3D12_COMMANDQUEUE,
    D3D12_COMMANDQUEUE1,
    D3D12_DESCRIPTORHEAP,
    D3D12_HEAP,
    D3D12_HEAP1,
    D3D12_HEAPFROMADDRESS,
    D3D12_COMMANDALLOCATOR,
    D3D12_ROOTSIGNATURE,
    D3D12_PIPELINELIBRARY,
    D3D12_LOADPIPELINESTATE,
    D3D12_LOADGRAPHICSPIPELINESTATE,
    D3D12_LOADCOMPUTEPIPELINESTATE,
    D3D12_COMMANDSIGNATURE,
    D3D12_GRAPHICSPIPELINESTATE,
    D3D12_COMPUTEPIPELINESTATE,
    D3D12_PIPELINESTATESTREAMSTATE,
    D3D12_STATEOBJECTSTATE,
    D3D12_ADDTOSTATEOBJECTSTATE,
    D3D12_STATEOBJECTPROPERTIESSTATE,
    D3D12_COMMANDLIST,
    D3D12_COMMANDLIST1,
    D3D12_COMMITTEDRESOURCE,
    D3D12_COMMITTEDRESOURCE1,
    D3D12_COMMITTEDRESOURCE2,
    D3D12_COMMITTEDRESOURCE3,
    D3D12_PLACEDRESOURCE,
    D3D12_PLACEDRESOURCE1,
    D3D12_PLACEDRESOURCE2,
    D3D12_RESERVEDRESOURCE,
    D3D12_RESERVEDRESOURCE1,
    D3D12_RESERVEDRESOURCE2,
    D3D12_FENCE,
    D3D12_QUERYHEAP,
    D3D12_INTC_DEVICEEXTENSIONCONTEXT,
    D3D12_INTC_DEVICEEXTENSIONCONTEXT1,
    D3D12_INTC_COMMITTEDRESOURCE,
    D3D12_INTC_PLACEDRESOURCE,
    D3D12_INTC_COMPUTEPIPELINESTATE,
    DSTORAGE_FACTORY,
    DSTORAGE_FILE,
    DSTORAGE_QUEUE,
    DSTORAGE_CUSTOMDECOMPRESSIONQUEUE,
    DSTORAGE_STATUSARRAY
  };
  ObjectState(StateId id_) : id(id_) {}
  virtual ~ObjectState() = default;
  StateId id;
  unsigned key{};
  unsigned parentKey{};
  unsigned childKey{};
  IUnknown* object{};
  std::wstring name;
  D3D12_RESIDENCY_PRIORITY residencyPriority{};
  bool restored{};
  int refCount{};
  bool keepDestroyed{};
  bool destroyed{};
};

struct ResourceState : public ObjectState {
  ResourceState(StateId stateId) : ObjectState(stateId) {}
  D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress{};
  bool isMappable{};
  bool isGenericRead{};
};

struct DXGIFactoryState : public ObjectState {
  DXGIFactoryState() : ObjectState(DXGI_FACTORY) {}
  UINT flags{};
  IID iid{};
};

struct DXGIAdapterState : public ObjectState {
  DXGIAdapterState() : ObjectState(DXGI_ADAPTER) {}
  UINT adapter{};
  DXGI_GPU_PREFERENCE gpuPreference{};
  IID iid{};
};

struct DXGIAdapterByLuidState : public ObjectState {
  DXGIAdapterByLuidState() : ObjectState(DXGI_ADAPTERBYLUID) {}
  LUID adapterLuid{};
  IID iid{};
};

struct DXGISwapChainState : public ObjectState {
  DXGISwapChainState() : ObjectState(DXGI_SWAPCHAIN) {}
  unsigned factoryKey{};
  unsigned deviceKey{};
  DXGI_SWAP_CHAIN_DESC desc{};
  IDXGISwapChain* swapChain{};
};

struct DXGISwapChainForHwndState : public ObjectState {
  DXGISwapChainForHwndState() : ObjectState(DXGI_SWAPCHAINFORHWND) {}
  unsigned factoryKey{};
  unsigned deviceKey{};
  HWND hWnd{};
  DXGI_SWAP_CHAIN_DESC1 desc{};
  bool isFullscreenDesc{};
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
  unsigned restrictToOutputKey{};
  IDXGISwapChain* swapChain{};
};

struct DXGISwapChainBufferState : public ObjectState {
  DXGISwapChainBufferState() : ObjectState(DXGI_SWAPCHAINBUFFER) {}
  unsigned swapChainKey{};
  UINT buffer{};
  IID iid{};
};

struct DXGIGetParentState : public ObjectState {
  DXGIGetParentState() : ObjectState(DXGI_GETPARENT) {}
  unsigned objectKey{};
  IID iid{};
};

struct D3D12DeviceState : public ObjectState {
  D3D12DeviceState() : ObjectState(D3D12_DEVICE) {}
  unsigned adapterKey{};
  D3D_FEATURE_LEVEL minimumFeatureLevel{};
  IID iid{};
};

struct D3D12CommandQueueState : public ObjectState {
  D3D12CommandQueueState() : ObjectState(D3D12_COMMANDQUEUE) {}
  unsigned deviceKey{};
  D3D12_COMMAND_QUEUE_DESC desc{};
  IID iid{};
  ID3D12CommandQueue* commandQueue{};
};

struct D3D12CommandQueue1State : public ObjectState {
  D3D12CommandQueue1State() : ObjectState(D3D12_COMMANDQUEUE1) {}
  unsigned deviceKey{};
  D3D12_COMMAND_QUEUE_DESC desc{};
  IID creatorId{};
  IID iid{};
  ID3D12CommandQueue* commandQueue{};
};

struct D3D12DescriptorHeapState : public ObjectState {
  D3D12DescriptorHeapState() : ObjectState(D3D12_DESCRIPTORHEAP) {}
  unsigned deviceKey{};
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  IID iid{};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};
};

struct D3D12HeapState : public ObjectState {
  D3D12HeapState() : ObjectState(D3D12_HEAP) {}
  unsigned deviceKey{};
  D3D12_HEAP_DESC desc{};
  IID iid{};
};

struct D3D12Heap1State : public ObjectState {
  D3D12Heap1State() : ObjectState(D3D12_HEAP1) {}
  unsigned deviceKey{};
  D3D12_HEAP_DESC desc{};
  unsigned protectedSessionKey{};
  IID iid{};
};

struct D3D12HeapFromAddressState : public ObjectState {
  D3D12HeapFromAddressState() : ObjectState(D3D12_HEAPFROMADDRESS) {}
  unsigned deviceKey{};
  void* address{};
  std::vector<char> buffer;
  IID iid{};
};

struct D3D12CommandAllocatorState : public ObjectState {
  D3D12CommandAllocatorState() : ObjectState(D3D12_COMMANDALLOCATOR) {}
  unsigned deviceKey{};
  D3D12_COMMAND_LIST_TYPE type{};
  IID iid{};
};

struct D3D12RootSignatureState : public ObjectState {
  D3D12RootSignatureState() : ObjectState(D3D12_ROOTSIGNATURE) {}
  unsigned deviceKey{};
  UINT nodeMask{};
  std::unique_ptr<char[]> blob;
  SIZE_T blobSize{};
  IID iid{};
};

struct D3D12PipelineLibraryState : public ObjectState {
  D3D12PipelineLibraryState() : ObjectState(D3D12_PIPELINELIBRARY) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> blob;
  SIZE_T blobSize{};
  IID iid{};
};

struct D3D12LoadPipelineState : public ObjectState {
  D3D12LoadPipelineState() : ObjectState(D3D12_LOADPIPELINESTATE) {}
  unsigned pipelineLibraryKey{};
  std::wstring name{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12LoadGraphicsPipelineState : public ObjectState {
  D3D12LoadGraphicsPipelineState() : ObjectState(D3D12_LOADGRAPHICSPIPELINESTATE) {}
  unsigned pipelineLibraryKey{};
  std::wstring name{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12LoadComputePipelineState : public ObjectState {
  D3D12LoadComputePipelineState() : ObjectState(D3D12_LOADCOMPUTEPIPELINESTATE) {}
  unsigned pipelineLibraryKey{};
  std::wstring name;
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12CommandSignatureState : public ObjectState {
  D3D12CommandSignatureState() : ObjectState(D3D12_COMMANDSIGNATURE) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> descEncoded;
  unsigned rootSignatureKey{};
  IID iid{};
};

struct D3D12GraphicsPipelineState : public ObjectState {
  D3D12GraphicsPipelineState() : ObjectState(D3D12_GRAPHICSPIPELINESTATE) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12ComputePipelineState : public ObjectState {
  D3D12ComputePipelineState() : ObjectState(D3D12_COMPUTEPIPELINESTATE) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12PipelineStateStreamState : public ObjectState {
  D3D12PipelineStateStreamState() : ObjectState(D3D12_PIPELINESTATESTREAMSTATE) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12StateObjectState : public ObjectState {
  D3D12StateObjectState() : ObjectState(D3D12_STATEOBJECTSTATE) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct D3D12AddToStateObjectState : public ObjectState {
  D3D12AddToStateObjectState() : ObjectState(D3D12_ADDTOSTATEOBJECTSTATE) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> descEncoded;
  unsigned stateObjectToGrowFromKey{};
  IID iid{};
};

struct D3D12StateObjectPropertiesState : public ObjectState {
  D3D12StateObjectPropertiesState() : ObjectState(D3D12_STATEOBJECTPROPERTIESSTATE) {}
  unsigned stateObjectKey{};
  std::map<std::wstring, std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>>
      shaderIdentifiers;
};

struct D3D12CommittedResourceState : public ResourceState {
  D3D12CommittedResourceState() : ResourceState(D3D12_COMMITTEDRESOURCE) {}
  unsigned deviceKey{};
  D3D12_HEAP_PROPERTIES heapProperties{};
  D3D12_HEAP_FLAGS heapFlags{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  IID iid{};
};

struct D3D12CommittedResource1State : public ResourceState {
  D3D12CommittedResource1State() : ResourceState(D3D12_COMMITTEDRESOURCE1) {}
  unsigned deviceKey{};
  D3D12_HEAP_PROPERTIES heapProperties{};
  D3D12_HEAP_FLAGS heapFlags{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  unsigned protectedSessionKey{};
  IID iid{};
};

struct D3D12CommittedResource2State : public ResourceState {
  D3D12CommittedResource2State() : ResourceState(D3D12_COMMITTEDRESOURCE2) {}
  unsigned deviceKey{};
  D3D12_HEAP_PROPERTIES heapProperties{};
  D3D12_HEAP_FLAGS heapFlags{};
  D3D12_RESOURCE_DESC1 desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  unsigned protectedSessionKey{};
  IID iid{};
};

struct D3D12CommittedResource3State : public ResourceState {
  D3D12CommittedResource3State() : ResourceState(D3D12_COMMITTEDRESOURCE3) {}
  unsigned deviceKey{};
  D3D12_HEAP_PROPERTIES heapProperties{};
  D3D12_HEAP_FLAGS heapFlags{};
  D3D12_RESOURCE_DESC1 desc{};
  D3D12_BARRIER_LAYOUT initialLayout{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  unsigned protectedSessionKey{};
  std::vector<DXGI_FORMAT> castableFormats;
  IID iid{};
};

struct D3D12PlacedResourceState : public ResourceState {
  D3D12PlacedResourceState() : ResourceState(D3D12_PLACEDRESOURCE) {}
  unsigned deviceKey{};
  unsigned heapKey{};
  UINT64 heapOffset{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  IID iid{};
};

struct D3D12PlacedResource1State : public ResourceState {
  D3D12PlacedResource1State() : ResourceState(D3D12_PLACEDRESOURCE1) {}
  unsigned deviceKey{};
  unsigned heapKey{};
  UINT64 heapOffset{};
  D3D12_RESOURCE_DESC1 desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  IID iid{};
};

struct D3D12PlacedResource2State : public ResourceState {
  D3D12PlacedResource2State() : ResourceState(D3D12_PLACEDRESOURCE2) {}
  unsigned deviceKey{};
  unsigned heapKey{};
  UINT64 heapOffset{};
  D3D12_RESOURCE_DESC1 desc{};
  D3D12_BARRIER_LAYOUT initialLayout{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  std::vector<DXGI_FORMAT> castableFormats;
  IID iid{};
};

struct D3D12ReservedResourceState : public ResourceState {
  D3D12ReservedResourceState() : ResourceState(D3D12_RESERVEDRESOURCE) {}
  unsigned deviceKey{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  IID iid{};
};

struct D3D12ReservedResource1State : public ResourceState {
  D3D12ReservedResource1State() : ResourceState(D3D12_RESERVEDRESOURCE1) {}
  unsigned deviceKey{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  unsigned protectedSessionKey{};
  IID iid{};
};

struct D3D12ReservedResource2State : public ResourceState {
  D3D12ReservedResource2State() : ResourceState(D3D12_RESERVEDRESOURCE2) {}
  unsigned deviceKey{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_BARRIER_LAYOUT initialLayout{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  unsigned protectedSessionKey{};
  std::vector<DXGI_FORMAT> castableFormats;
  IID iid{};
};

struct D3D12FenceState : public ObjectState {
  D3D12FenceState() : ObjectState(D3D12_FENCE) {}
  unsigned deviceKey{};
  D3D12_FENCE_FLAGS flags{};
  IID iid{};
};

struct D3D12QueryHeapState : public ObjectState {
  D3D12QueryHeapState() : ObjectState(D3D12_QUERYHEAP) {}
  unsigned deviceKey{};
  D3D12_QUERY_HEAP_DESC desc{};
  IID iid{};
};

struct D3D12INTCDeviceExtensionContextState : public ObjectState {
  D3D12INTCDeviceExtensionContextState() : ObjectState(D3D12_INTC_DEVICEEXTENSIONCONTEXT) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> extensionInfoEncoded;
  bool isExtensionAppInfo{};
  std::unique_ptr<char[]> extensionAppInfoEncoded;
};

struct D3D12INTCDeviceExtensionContext1State : public ObjectState {
  D3D12INTCDeviceExtensionContext1State() : ObjectState(D3D12_INTC_DEVICEEXTENSIONCONTEXT1) {}
  unsigned deviceKey{};
  std::unique_ptr<char[]> extensionInfoEncoded;
  bool isExtensionAppInfo{};
  std::unique_ptr<char[]> extensionAppInfoEncoded;
};

struct D3D12INTCCommittedResourceState : public ResourceState {
  D3D12INTCCommittedResourceState() : ResourceState(D3D12_INTC_COMMITTEDRESOURCE) {}
  unsigned deviceKey{};
  unsigned extensionContextKey{};
  D3D12_HEAP_PROPERTIES heapProperties{};
  D3D12_HEAP_FLAGS heapFlags{};
  INTC_D3D12_RESOURCE_DESC_0001 descIntc{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  IID iid{};
};

struct D3D12INTCPlacedResourceState : public ResourceState {
  D3D12INTCPlacedResourceState() : ResourceState(D3D12_INTC_PLACEDRESOURCE) {}
  unsigned deviceKey{};
  unsigned extensionContextKey{};
  unsigned heapKey{};
  UINT64 heapOffset{};
  INTC_D3D12_RESOURCE_DESC_0001 descIntc{};
  D3D12_RESOURCE_DESC desc{};
  D3D12_RESOURCE_STATES initialResourceState{};
  bool isClearValue{};
  D3D12_CLEAR_VALUE clearValue{};
  IID iid{};
};

struct D3D12INTCComputePipelineState : public ObjectState {
  D3D12INTCComputePipelineState() : ObjectState(D3D12_INTC_COMPUTEPIPELINESTATE) {}
  unsigned extensionContextKey{};
  std::unique_ptr<char[]> descEncoded;
  IID iid{};
};

struct DStorageFactoryState : public ObjectState {
  DStorageFactoryState() : ObjectState(DSTORAGE_FACTORY) {}
  IID iid{};
};

struct DStorageFileState : public ObjectState {
  DStorageFileState() : ObjectState(DSTORAGE_FILE) {}
  std::wstring path{};
  IID iid{};
};

struct DStorageQueueState : public ObjectState {
  DStorageQueueState() : ObjectState(DSTORAGE_QUEUE) {}
  DSTORAGE_QUEUE_DESC desc{};
  unsigned deviceKey{};
  std::string name{};
  IID iid{};
};

struct DStorageCustomDecompressionQueueState : public ObjectState {
  DStorageCustomDecompressionQueueState() : ObjectState(DSTORAGE_CUSTOMDECOMPRESSIONQUEUE) {}
  IID iid{};
};

struct DStorageStatusArrayState : public ObjectState {
  DStorageStatusArrayState() : ObjectState(DSTORAGE_STATUSARRAY) {}
  unsigned capacity{};
  std::string name{};
  IID iid{};
};

} // namespace DirectX
} // namespace gits
