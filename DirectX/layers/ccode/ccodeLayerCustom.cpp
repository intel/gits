// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeLayerAuto.h"
#include "ccodeTypes.h"
#include "ccodeParameters.h"
#include "ccodeCommandPrinter.h"
#include "ccodeStream.h"
#include "configurator.h"

namespace gits {
namespace DirectX {

static void createResource(ccode::CommandPrinter& p, unsigned ResourceKey) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreateResource(" << ResourceKey << ", "
     << ccode::objKeyToPtrStr(ResourceKey) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

static void createPlacedResource(ccode::CommandPrinter& p,
                                 unsigned ResourceKey,
                                 unsigned heapKey,
                                 uint64_t heapOffset) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreatePlacedResource(" << ResourceKey << ", "
     << ccode::objKeyToPtrStr(ResourceKey) << ", " << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ", " << heapOffset << ");" << std::endl;
  p.setPostCommand(ss.str());
}

static void createHeap(ccode::CommandPrinter& p, unsigned heapKey) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreateHeap(" << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

static void createSwapChain(ccode::CommandPrinter& p, unsigned swapChainKey, unsigned cmdQueueKey) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::ScreenshotService::Get().RegisterSwapChain(" << swapChainKey << ", "
     << objKeyToPtrStr(swapChainKey) << ", " << objKeyToPtrStr(cmdQueueKey) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

static void present(ccode::CommandPrinter& p, unsigned swapChainKey) {
  std::ostringstream ss;
  ss << "directx::ScreenshotService::Get().CaptureFrame(" << swapChainKey << ");" << std::endl;
  p.setPreCommand(ss.str());
}

CCodeLayer::CCodeLayer() : Layer("CCode") {}

void CCodeLayer::Pre(StateRestoreBeginCommand& c) {
  isStateRestore_ = true;
}

void CCodeLayer::Pre(StateRestoreEndCommand& c) {
  isStateRestore_ = false;
  nextCommand();
}

void CCodeLayer::Pre(IDXGISwapChainPresentCommand& c) {
  if (isStateRestore_ || (c.m_Flags.Value & DXGI_PRESENT_TEST)) {
    return;
  }
  currentFrame_++;
}

void CCodeLayer::Pre(IDXGISwapChain1Present1Command& c) {
  if (isStateRestore_ || (c.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    return;
  }
  currentFrame_++;
}

void CCodeLayer::nextCommand() {
  using namespace ccode;

  static unsigned s_cmdCount = 1;
  static unsigned s_currentFrame = 0;
  static bool s_stateRestore = isStateRestore_;

  static unsigned s_commandsPerBlock = 0;
  if (s_commandsPerBlock == 0) {
    s_commandsPerBlock = Configurator::Get().directx.player.cCode.commandsPerBlock;
    GITS_ASSERT(s_commandsPerBlock > 0,
                "CCode - commandsPerBlock must be greater than 0 to enable block splitting");
  }

  bool shouldStartNewBlock = (isStateRestore_ != s_stateRestore) ||
                             (currentFrame_ != s_currentFrame) ||
                             (s_cmdCount == s_commandsPerBlock);
  if (shouldStartNewBlock) {
    CCodeStream::BlockInfo blockInfo = {};
    blockInfo.isStateRestore = s_stateRestore;
    blockInfo.frame = s_currentFrame;
    blockInfo.commands = s_cmdCount;

    auto& stream = CCodeStream::getInstance();
    stream.writeBlock(blockInfo);

    s_cmdCount = 0;
  }
  s_stateRestore = isStateRestore_;
  s_currentFrame = currentFrame_;
  s_cmdCount++;
}

void CCodeLayer::Post(CreateWindowMetaCommand& c) {
  auto ptrValue = reinterpret_cast<std::uintptr_t>(c.m_hWnd.Value);
  auto& stream = ccode::CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "// Command #" << keyToStr(c.Key) << " CreateWindowMetaCommand" << std::endl;
  ss << "WindowService::Get().Create(" << ptrValue << ", " << c.m_width.Value << ", "
     << c.m_height.Value << ");" << std::endl;
}

void CCodeLayer::Post(WaitForFenceSignaledCommand& c) {
  using namespace ccode;
  auto& stream = CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "directx::WaitForFence(" << objKeyToPtrStr(c.m_fence.Key) << ", " << c.m_Value.Value << ");"
     << std::endl;
}

void CCodeLayer::Post(MappedDataMetaCommand& c) {
  using namespace ccode;
  auto& stream = CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "// Command #" << keyToStr(c.Key) << " MappedDataMetaCommand" << std::endl;
  ss << "{" << std::endl;

  CppParameterInfo dataInfo("void", "data");
  dataInfo.size = c.m_data.Size;
  CppParameterOutput dataOut;
  toCpp(c.m_data.Value, dataInfo, dataOut);
  ss << dataOut.initialization;

  ss << "void* mappedAddress = directx::MapTrackingService::Get().GetCurrentAddress("
     << toHex(c.m_mappedAddress.Value) << ");" << std::endl;
  ss << "std::memcpy(static_cast<char*>(mappedAddress) + " << c.m_offset.Value << ", "
     << dataOut.value << ", " << c.m_data.Size << ");" << std::endl;
  ss << "}" << std::endl;
}

void CCodeLayer::Post(CreateHeapAllocationMetaCommand& c) {
  using namespace ccode;
  auto& stream = CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "// Command #" << keyToStr(c.Key) << " CreateHeapAllocationMetaCommand" << std::endl;
  ss << "directx::HeapAllocationService::Get().CreateHeapAllocation(" << objKeyToStr(c.m_heap.Key)
     << ", " << c.m_data.Size << ");" << std::endl;
}

void CCodeLayer::Post(IUnknownQueryInterfaceCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo ppvObjectInfo("void", "ppvObject");

  // Print command
  CommandPrinter p(c, "QueryInterface", c.m_Object.Key);
  preProcess(p, c);
  p.addArgumentValue(c.m_riid.Value);
  p.addArgument(c.m_ppvObject, ppvObjectInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(IUnknownAddRefCommand& c) {
  using namespace ccode;

  // Print command
  CommandPrinter p(c, "AddRef", c.m_Object.Key);
  preProcess(p, c);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(IUnknownReleaseCommand& c) {
  using namespace ccode;

  // Print command
  CommandPrinter p(c, "Release", c.m_Object.Key);
  preProcess(p, c);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pDeviceInfo("ID3D12Device", "pDevice");
  CppParameterInfo pSupportedExtVersionsInfo("INTCExtensionVersion", "pSupportedExtVersions");
  pSupportedExtVersionsInfo.isPtr = true;
  pSupportedExtVersionsInfo.size = 0;
  if (c.m_pSupportedExtVersionsCount.Value) {
    pSupportedExtVersionsInfo.size = *c.m_pSupportedExtVersionsCount.Value;
  }

  // Print command
  CommandPrinter p(c, "INTC_D3D12_GetSupportedVersions");
  preProcess(p, c);
  p.addArgument(c.m_pDevice, pDeviceInfo);
  p.addArgument(c.m_pSupportedExtVersions, pSupportedExtVersionsInfo);
  p.addArgumentValue(c.m_pSupportedExtVersionsCount.Value);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pDeviceInfo("ID3D12Device", "pDevice");
  pDeviceInfo.isPtr = true;
  CppParameterInfo ppExtensionContextInfo("INTCExtensionContext", "ppExtensionContext");
  CppParameterInfo pExtensionInfoInfo("INTCExtensionInfo", "pExtensionInfo");
  pExtensionInfoInfo.isPtr = true;
  CppParameterInfo pExtensionAppInfoInfo("INTCExtensionAppInfo", "pExtensionAppInfo");
  pExtensionAppInfoInfo.isPtr = true;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateDeviceExtensionContext");
  preProcess(p, c);
  p.addArgument(c.m_pDevice, pDeviceInfo);
  p.addArgument(c.m_ppExtensionContext, ppExtensionContextInfo);
  p.addArgument(c.m_pExtensionInfo, pExtensionInfoInfo);
  p.addArgument(c.m_pExtensionAppInfo, pExtensionAppInfoInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pDeviceInfo("ID3D12Device", "pDevice");
  pDeviceInfo.isPtr = true;
  CppParameterInfo ppExtensionContextInfo("INTCExtensionContext", "ppExtensionContext");
  CppParameterInfo pExtensionInfoInfo("INTCExtensionInfo", "pExtensionInfo");
  pExtensionInfoInfo.isPtr = true;
  CppParameterInfo pExtensionAppInfoInfo("INTCExtensionAppInfo1", "pExtensionAppInfo");
  pExtensionAppInfoInfo.isPtr = true;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateDeviceExtensionContext1");
  preProcess(p, c);
  p.addArgument(c.m_pDevice, pDeviceInfo);
  p.addArgument(c.m_ppExtensionContext, ppExtensionContextInfo);
  p.addArgument(c.m_pExtensionInfo, pExtensionInfoInfo);
  p.addArgument(c.m_pExtensionAppInfo, pExtensionAppInfoInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_DestroyDeviceExtensionContextCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo ppExtensionContextInfo("INTCExtensionContext", "ppExtensionContext");

  // Print command
  CommandPrinter p(c, "INTC_DestroyDeviceExtensionContext");
  preProcess(p, c);
  p.addArgument(c.m_ppExtensionContext, ppExtensionContextInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_SetFeatureSupportCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pFeatureInfo("INTC_D3D12_FEATURE", "pFeature");
  pFeatureInfo.isPtr = true;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_SetFeatureSupport");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pFeature, pFeatureInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.m_ppvResource.Key, c.m_riid.Value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pHeapInfo("ID3D12Heap", "pHeap");
  pHeapInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_RESOURCE_DESC_0001", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo pOptimizedClearValueInfo("D3D12_CLEAR_VALUE", "pOptimizedClearValue");
  pOptimizedClearValueInfo.isPtr = true;
  CppParameterInfo ppvResourceInfo("void", "ppvResource");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreatePlacedResource");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pHeap, pHeapInfo);
  p.addArgumentValue(c.m_HeapOffset.Value);
  p.addArgument(c.m_pDesc, pDescInfo);
  p.addArgumentValue(c.m_InitialState.Value);
  p.addArgument(c.m_pOptimizedClearValue, pOptimizedClearValueInfo);
  p.addArgumentValue(c.m_riid.Value);
  p.addArgument(c.m_ppvResource, ppvResourceInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.m_ppvResource.Key, c.m_riidResource.Value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pHeapPropertiesInfo("D3D12_HEAP_PROPERTIES", "pHeapProperties");
  pHeapPropertiesInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_RESOURCE_DESC_0001", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo pOptimizedClearValueInfo("D3D12_CLEAR_VALUE", "pOptimizedClearValue");
  pOptimizedClearValueInfo.isPtr = true;
  CppParameterInfo ppvResourceInfo("void", "ppvResource");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateCommittedResource");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pHeapProperties, pHeapPropertiesInfo);
  p.addArgumentValue(c.m_HeapFlags.Value);
  p.addArgument(c.m_pDesc, pDescInfo);
  p.addArgumentValue(c.m_InitialResourceState.Value);
  p.addArgument(c.m_pOptimizedClearValue, pOptimizedClearValueInfo);
  p.addArgumentValue(c.m_riidResource.Value);
  p.addArgument(c.m_ppvResource, ppvResourceInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateReservedResourceCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.m_ppvResource.Key, c.m_riid.Value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_RESOURCE_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo pOptimizedClearValueInfo("D3D12_CLEAR_VALUE", "pOptimizedClearValue");
  pOptimizedClearValueInfo.isPtr = true;
  CppParameterInfo ppvResourceInfo("void", "ppvResource");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateReservedResource");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pDesc, pDescInfo);
  p.addArgumentValue(c.m_InitialState.Value);
  p.addArgument(c.m_pOptimizedClearValue, pOptimizedClearValueInfo);
  p.addArgumentValue(c.m_riid.Value);
  p.addArgument(c.m_ppvResource, ppvResourceInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateHeapCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.m_ppvHeap.Key, c.m_riid.Value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_HEAP_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo ppvHeapInfo("void", "ppvHeap");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateHeap");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pDesc, pDescInfo);
  p.addArgumentValue(c.m_riid.Value);
  p.addArgument(c.m_ppvHeap, ppvHeapInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateCommandQueueCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.m_ppCommandQueue.Key, c.m_riid.Value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_COMMAND_QUEUE_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo ppCommandQueueInfo("void", "ppCommandQueue");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateCommandQueue");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pDesc, pDescInfo);
  p.addArgumentValue(c.m_riid.Value);
  p.addArgument(c.m_ppCommandQueue, ppCommandQueueInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_SetApplicationInfoCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionAppInfoInfo("INTCExtensionAppInfo1", "pExtensionAppInfo");
  pExtensionAppInfoInfo.isPtr = true;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_SetApplicationInfo");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionAppInfo, pExtensionAppInfoInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pFeatureSupportDataInfo("void", "pFeatureSupportData");
  pFeatureSupportDataInfo.isPtr = true;
  pFeatureSupportDataInfo.size = c.m_FeatureSupportDataSize.Value;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CheckFeatureSupport");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgumentValue(c.m_Feature.Value);
  p.addArgument(c.m_pFeatureSupportData, pFeatureSupportDataInfo);
  p.addArgumentValue(c.m_FeatureSupportDataSize.Value);
  postProcess(p, c);
  p.print();

  nextCommand();
}
void CCodeLayer::Post(INTC_D3D12_GetResourceAllocationInfoCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pResourceDescsInfo("INTC_D3D12_RESOURCE_DESC_0001", "pResourceDescs");
  pResourceDescsInfo.isPtr = true;
  pResourceDescsInfo.size = c.m_numResourceDescs.Value;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_GetResourceAllocationInfo");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgumentValue(c.m_visibleMask.Value);
  p.addArgumentValue(c.m_numResourceDescs.Value);
  p.addArgument(c.m_pResourceDescs, pResourceDescsInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.m_ppPipelineState.Key, c.m_riid.Value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo ppPipelineStateInfo("void", "ppPipelineState");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateComputePipelineState");
  preProcess(p, c);
  p.addArgument(c.m_pExtensionContext, pExtensionContextInfo);
  p.addArgument(c.m_pDesc, pDescInfo);
  p.addArgumentValue(c.m_riid.Value);
  p.addArgument(c.m_ppPipelineState, ppPipelineStateInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, ID3D12FenceSetEventOnCompletionCommand& c) {
  // Skip all the non-null SetEventOnCompletion since the events cannot be recorded
  if (c.m_hEvent.Value != 0) {
    p.skip();
  }
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, ID3D12ObjectGetPrivateDataCommand& c) {
  // Data is not used and may trigger Debug Layer errors
  p.skip();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, ID3D12ObjectSetPrivateDataCommand& c) {
  // Data is not used and may trigger Debug Layer errors
  p.skip();
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDXGIFactoryCreateSwapChainCommand& c) {
  createSwapChain(p, c.m_ppSwapChain.Key, c.m_pDevice.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  createSwapChain(p, c.m_ppSwapChain.Key, c.m_pDevice.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  createSwapChain(p, c.m_ppSwapChain.Key, c.m_pDevice.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             IDXGIFactory2CreateSwapChainForCompositionCommand& c) {
  createSwapChain(p, c.m_ppSwapChain.Key, c.m_pDevice.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDXGISwapChainPresentCommand& c) {
  present(p, c.m_Object.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDXGISwapChain1Present1Command& c) {
  present(p, c.m_Object.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12FenceGetCompletedValueCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::WaitForFence(" << objKeyToPtrStr(c.m_Object.Key) << ", " << c.m_Result.Value
     << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateDescriptorHeapCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::DescriptorHeapService::Get().Create(" << objKeyToStr(c.m_ppvHeap.Key) << ", "
     << objKeyToPtrStr(c.m_ppvHeap.Key) << ", &pDescriptorHeapDesc);" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12ResourceMapCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::MapTrackingService::Get().MapResource(" << objKeyToStr(c.m_Object.Key) << ", "
     << c.m_Subresource.Value << ", " << toHex(c.m_ppData.CaptureValue) << ", &ppData);"
     << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12DeviceCreateCommittedResourceCommand& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device4CreateCommittedResource1Command& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device8CreateCommittedResource2Command& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device10CreateCommittedResource3Command& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12DeviceCreateReservedResourceCommand& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device4CreateReservedResource1Command& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device10CreateReservedResource2Command& c) {
  createResource(p, c.m_ppvResource.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreatePlacedResourceCommand& c) {
  createPlacedResource(p, c.m_ppvResource.Key, c.m_pHeap.Key, c.m_HeapOffset.Value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device8CreatePlacedResource1Command& c) {
  createPlacedResource(p, c.m_ppvResource.Key, c.m_pHeap.Key, c.m_HeapOffset.Value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device10CreatePlacedResource2Command& c) {
  createPlacedResource(p, c.m_ppvResource.Key, c.m_pHeap.Key, c.m_HeapOffset.Value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateHeapCommand& c) {
  createHeap(p, c.m_ppvHeap.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12Device4CreateHeap1Command& c) {
  createHeap(p, c.m_ppvHeap.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, INTC_D3D12_CreateHeapCommand& c) {
  createHeap(p, c.m_ppvHeap.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "void* pAddress = directx::HeapAllocationService::Get().GetHeapAllocation("
     << objKeyToStr(c.m_ppvHeap.Key) << ");" << std::endl;
  p.setPreCommand(ss.str());
  p.getArgumentValue(0) = "pAddress";
  createHeap(p, c.m_ppvHeap.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "void* pAddress = directx::HeapAllocationService::Get().GetHeapAllocation("
     << objKeyToStr(c.m_ppvHeap.Key) << ");" << std::endl;
  p.setPreCommand(ss.str());
  p.getArgumentValue(0) = "pAddress";
  createHeap(p, c.m_ppvHeap.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadGraphicsPipeline(" << objKeyToPtrStr(c.m_Object.Key) << ", "
     << p.getArgumentValue(0) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadComputePipeline(" << objKeyToPtrStr(c.m_Object.Key) << ", "
     << p.getArgumentValue(0) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadPipeline(" << objKeyToPtrStr(c.m_Object.Key) << ", "
     << p.getArgumentValue(0) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

} // namespace DirectX
} // namespace gits
