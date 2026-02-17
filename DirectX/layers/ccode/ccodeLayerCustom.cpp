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

static void createResource(ccode::CommandPrinter& p, unsigned resourceKey) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreateResource(" << resourceKey << ", "
     << ccode::objKeyToPtrStr(resourceKey) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

static void createPlacedResource(ccode::CommandPrinter& p,
                                 unsigned resourceKey,
                                 unsigned heapKey,
                                 uint64_t heapOffset) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreatePlacedResource(" << resourceKey << ", "
     << ccode::objKeyToPtrStr(resourceKey) << ", " << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ", " << heapOffset << ");" << std::endl;
  p.setPostCommand(ss.str());
}

static void createHeap(ccode::CommandPrinter& p, unsigned heapKey) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreateHeap(" << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

CCodeLayer::CCodeLayer() : Layer("CCode") {}

void CCodeLayer::pre(StateRestoreBeginCommand& c) {
  isStateRestore_ = true;
}

void CCodeLayer::pre(StateRestoreEndCommand& c) {
  isStateRestore_ = false;
  nextCommand();
}

void CCodeLayer::pre(IDXGISwapChainPresentCommand& c) {
  if (isStateRestore_ || (c.Flags_.value & DXGI_PRESENT_TEST)) {
    return;
  }
  currentFrame_++;
}

void CCodeLayer::pre(IDXGISwapChain1Present1Command& c) {
  if (isStateRestore_ || (c.PresentFlags_.value & DXGI_PRESENT_TEST)) {
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

void CCodeLayer::post(CreateWindowMetaCommand& c) {
  auto ptrValue = reinterpret_cast<std::uintptr_t>(c.hWnd_.value);
  auto& stream = ccode::CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "// Command #" << keyToStr(c.key) << " CreateWindowMetaCommand" << std::endl;
  ss << "WindowService::Get().Create(" << ptrValue << ", " << c.width_.value << ", "
     << c.height_.value << ");" << std::endl;
}

void CCodeLayer::post(WaitForFenceSignaledCommand& c) {
  using namespace ccode;
  auto& stream = CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "directx::WaitForFence(" << objKeyToPtrStr(c.fence_.key) << ", " << c.value_.value << ");"
     << std::endl;
}

void CCodeLayer::post(MappedDataMetaCommand& c) {
  using namespace ccode;
  auto& stream = CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "// Command #" << keyToStr(c.key) << " MappedDataMetaCommand" << std::endl;
  ss << "{" << std::endl;

  CppParameterInfo dataInfo("void", "data");
  dataInfo.size = c.data_.size;
  CppParameterOutput dataOut;
  toCpp(c.data_.value, dataInfo, dataOut);
  ss << dataOut.initialization;

  ss << "void* mappedAddress = directx::MapTrackingService::Get().GetCurrentAddress("
     << toHex(c.mappedAddress_.value) << ");" << std::endl;
  ss << "std::memcpy(static_cast<char*>(mappedAddress) + " << c.offset_.value << ", "
     << dataOut.value << ", " << c.data_.size << ");" << std::endl;
  ss << "}" << std::endl;
}

void CCodeLayer::post(CreateHeapAllocationMetaCommand& c) {
  using namespace ccode;
  auto& stream = CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  ss << "// Command #" << keyToStr(c.key) << " CreateHeapAllocationMetaCommand" << std::endl;
  ss << "directx::HeapAllocationService::Get().CreateHeapAllocation(" << objKeyToStr(c.heap_.key)
     << ", " << c.data_.size << ");" << std::endl;
}

void CCodeLayer::post(INTC_D3D12_GetSupportedVersionsCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pDeviceInfo("ID3D12Device", "pDevice");
  CppParameterInfo pSupportedExtVersionsInfo("INTCExtensionVersion", "pSupportedExtVersions");
  pSupportedExtVersionsInfo.isPtr = true;
  pSupportedExtVersionsInfo.size = 0;
  if (c.pSupportedExtVersionsCount_.value) {
    pSupportedExtVersionsInfo.size = *c.pSupportedExtVersionsCount_.value;
  }

  // Print command
  CommandPrinter p(c, "INTC_D3D12_GetSupportedVersions");
  preProcess(p, c);
  p.addArgument(c.pDevice_, pDeviceInfo);
  p.addArgument(c.pSupportedExtVersions_, pSupportedExtVersionsInfo);
  p.addArgumentValue(c.pSupportedExtVersionsCount_.value);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
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
  p.addArgument(c.pDevice_, pDeviceInfo);
  p.addArgument(c.ppExtensionContext_, ppExtensionContextInfo);
  p.addArgument(c.pExtensionInfo_, pExtensionInfoInfo);
  p.addArgument(c.pExtensionAppInfo_, pExtensionAppInfoInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
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
  p.addArgument(c.pDevice_, pDeviceInfo);
  p.addArgument(c.ppExtensionContext_, ppExtensionContextInfo);
  p.addArgument(c.pExtensionInfo_, pExtensionInfoInfo);
  p.addArgument(c.pExtensionAppInfo_, pExtensionAppInfoInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_DestroyDeviceExtensionContextCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo ppExtensionContextInfo("INTCExtensionContext", "ppExtensionContext");

  // Print command
  CommandPrinter p(c, "INTC_DestroyDeviceExtensionContext");
  preProcess(p, c);
  p.addArgument(c.ppExtensionContext_, ppExtensionContextInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_SetFeatureSupportCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pFeatureInfo("INTC_D3D12_FEATURE", "pFeature");
  pFeatureInfo.isPtr = true;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_SetFeatureSupport");
  preProcess(p, c);
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pFeature_, pFeatureInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.ppvResource_.key, c.riid_.value);

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
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pHeap_, pHeapInfo);
  p.addArgumentValue(c.HeapOffset_.value);
  p.addArgument(c.pDesc_, pDescInfo);
  p.addArgumentValue(c.InitialState_.value);
  p.addArgument(c.pOptimizedClearValue_, pOptimizedClearValueInfo);
  p.addArgumentValue(c.riid_.value);
  p.addArgument(c.ppvResource_, ppvResourceInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.ppvResource_.key, c.riidResource_.value);

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
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pHeapProperties_, pHeapPropertiesInfo);
  p.addArgumentValue(c.HeapFlags_.value);
  p.addArgument(c.pDesc_, pDescInfo);
  p.addArgumentValue(c.InitialResourceState_.value);
  p.addArgument(c.pOptimizedClearValue_, pOptimizedClearValueInfo);
  p.addArgumentValue(c.riidResource_.value);
  p.addArgument(c.ppvResource_, ppvResourceInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateReservedResourceCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.ppvResource_.key, c.riid_.value);

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
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pDesc_, pDescInfo);
  p.addArgumentValue(c.InitialState_.value);
  p.addArgument(c.pOptimizedClearValue_, pOptimizedClearValueInfo);
  p.addArgumentValue(c.riid_.value);
  p.addArgument(c.ppvResource_, ppvResourceInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateHeapCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.ppvHeap_.key, c.riid_.value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_HEAP_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo ppvHeapInfo("void", "ppvHeap");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateHeap");
  preProcess(p, c);
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pDesc_, pDescInfo);
  p.addArgumentValue(c.riid_.value);
  p.addArgument(c.ppvHeap_, ppvHeapInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateCommandQueueCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.ppCommandQueue_.key, c.riid_.value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_COMMAND_QUEUE_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo ppCommandQueueInfo("void", "ppCommandQueue");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateCommandQueue");
  preProcess(p, c);
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pDesc_, pDescInfo);
  p.addArgumentValue(c.riid_.value);
  p.addArgument(c.ppCommandQueue_, ppCommandQueueInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_SetApplicationInfoCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionAppInfoInfo("INTCExtensionAppInfo1", "pExtensionAppInfo");
  pExtensionAppInfoInfo.isPtr = true;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_SetApplicationInfo");
  preProcess(p, c);
  p.addArgument(c.pExtensionAppInfo_, pExtensionAppInfoInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CheckFeatureSupportCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pFeatureSupportDataInfo("void", "pFeatureSupportData");
  pFeatureSupportDataInfo.isPtr = true;
  pFeatureSupportDataInfo.size = c.FeatureSupportDataSize_.value;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CheckFeatureSupport");
  preProcess(p, c);
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgumentValue(c.Feature_.value);
  p.addArgument(c.pFeatureSupportData_, pFeatureSupportDataInfo);
  p.addArgumentValue(c.FeatureSupportDataSize_.value);
  postProcess(p, c);
  p.print();

  nextCommand();
}
void CCodeLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& c) {
  using namespace ccode;

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pResourceDescsInfo("INTC_D3D12_RESOURCE_DESC_0001", "pResourceDescs");
  pResourceDescsInfo.isPtr = true;
  pResourceDescsInfo.size = c.numResourceDescs_.value;

  // Print command
  CommandPrinter p(c, "INTC_D3D12_GetResourceAllocationInfo");
  preProcess(p, c);
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgumentValue(c.visibleMask_.value);
  p.addArgumentValue(c.numResourceDescs_.value);
  p.addArgument(c.pResourceDescs_, pResourceDescsInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  using namespace ccode;

  // Declare new object
  auto& stream = CCodeStream::getInstance();
  stream.addInterface(c.ppPipelineState_.key, c.riid_.value);

  // Parameter data
  CppParameterInfo pExtensionContextInfo("INTCExtensionContext", "pExtensionContext");
  pExtensionContextInfo.isPtr = true;
  CppParameterInfo pDescInfo("INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC", "pDesc");
  pDescInfo.isPtr = true;
  CppParameterInfo ppPipelineStateInfo("void", "ppPipelineState");

  // Print command
  CommandPrinter p(c, "INTC_D3D12_CreateComputePipelineState");
  preProcess(p, c);
  p.addArgument(c.pExtensionContext_, pExtensionContextInfo);
  p.addArgument(c.pDesc_, pDescInfo);
  p.addArgumentValue(c.riid_.value);
  p.addArgument(c.ppPipelineState_, ppPipelineStateInfo);
  postProcess(p, c);
  p.print();

  nextCommand();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, ID3D12FenceSetEventOnCompletionCommand& c) {
  // Skip all the non-null SetEventOnCompletion since the events cannot be recorded
  if (c.hEvent_.value != 0) {
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

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12FenceGetCompletedValueCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::WaitForFence(" << objKeyToPtrStr(c.object_.key) << ", " << c.result_.value << ");"
     << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateDescriptorHeapCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::DescriptorHeapService::Get().Create(" << objKeyToStr(c.ppvHeap_.key) << ", "
     << objKeyToPtrStr(c.ppvHeap_.key) << ", &pDescriptorHeapDesc);" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12ResourceMapCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::MapTrackingService::Get().MapResource(" << objKeyToStr(c.object_.key) << ", "
     << c.Subresource_.value << ", " << toHex(c.ppData_.captureValue) << ", &ppData);" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12DeviceCreateCommittedResourceCommand& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device4CreateCommittedResource1Command& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device8CreateCommittedResource2Command& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device10CreateCommittedResource3Command& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12DeviceCreateReservedResourceCommand& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device4CreateReservedResource1Command& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device10CreateReservedResource2Command& c) {
  createResource(p, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreatePlacedResourceCommand& c) {
  createPlacedResource(p, c.ppvResource_.key, c.pHeap_.key, c.HeapOffset_.value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device8CreatePlacedResource1Command& c) {
  createPlacedResource(p, c.ppvResource_.key, c.pHeap_.key, c.HeapOffset_.value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device10CreatePlacedResource2Command& c) {
  createPlacedResource(p, c.ppvResource_.key, c.pHeap_.key, c.HeapOffset_.value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12DeviceCreateHeapCommand& c) {
  createHeap(p, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12Device4CreateHeap1Command& c) {
  createHeap(p, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, INTC_D3D12_CreateHeapCommand& c) {
  createHeap(p, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "void* pAddress = directx::HeapAllocationService::Get().GetHeapAllocation("
     << objKeyToStr(c.ppvHeap_.key) << ");" << std::endl;
  p.setPreCommand(ss.str());
  p.getArgumentValue(0) = "pAddress";
  createHeap(p, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "void* pAddress = directx::HeapAllocationService::Get().GetHeapAllocation("
     << objKeyToStr(c.ppvHeap_.key) << ");" << std::endl;
  p.setPreCommand(ss.str());
  p.getArgumentValue(0) = "pAddress";
  createHeap(p, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadGraphicsPipeline(" << objKeyToPtrStr(c.object_.key) << ", "
     << p.getArgumentValue(0) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadComputePipeline(" << objKeyToPtrStr(c.object_.key) << ", "
     << p.getArgumentValue(0) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadPipeline(" << objKeyToPtrStr(c.object_.key) << ", "
     << p.getArgumentValue(0) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

} // namespace DirectX
} // namespace gits
