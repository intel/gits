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
                                 uint64_t heapOffset,
                                 const char* flagsExpr = "pDesc.Flags") {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreatePlacedResource(" << resourceKey << ", "
     << ccode::objKeyToPtrStr(resourceKey) << ", " << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ", " << heapOffset << ");" << std::endl;
  ss << "directx::RecordCreatePlacedResource(" << heapKey << ", " << resourceKey << ", "
     << flagsExpr << ");" << std::endl;
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

CCodeLayer::CCodeLayer() : Layer("CCode") {
  Configurator::GetMutable().directx.player.plugins.push_back("CpuPatch");
}

void CCodeLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  m_ExportGpuVirtualAddressCapture = c.m_Result.Value;
}

void CCodeLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  m_ExportGpuDescriptorHandleCapture = c.m_Result.Value;
}

void CCodeLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  memcpy(m_ExportShaderIdentifierCapture.data(), c.m_Result.Value,
         m_ExportShaderIdentifierCapture.size());
}

void CCodeLayer::Pre(StateRestoreBeginCommand& c) {
  m_IsStateRestore = true;
}

void CCodeLayer::Pre(StateRestoreEndCommand& c) {
  m_IsStateRestore = false;
  nextCommand();
}

void CCodeLayer::Pre(IDXGISwapChainPresentCommand& c) {
  if (m_IsStateRestore || (c.m_Flags.Value & DXGI_PRESENT_TEST)) {
    return;
  }
  m_CurrentFrame++;
}

void CCodeLayer::Pre(IDXGISwapChain1Present1Command& c) {
  if (m_IsStateRestore || (c.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    return;
  }
  m_CurrentFrame++;
}

void CCodeLayer::nextCommand() {
  using namespace ccode;

  static unsigned s_cmdCount = 1;
  static unsigned s_currentFrame = 0;
  static bool s_stateRestore = m_IsStateRestore;

  static unsigned s_commandsPerBlock = 0;
  if (s_commandsPerBlock == 0) {
    s_commandsPerBlock = Configurator::Get().directx.player.cCode.commandsPerBlock;
    GITS_ASSERT(s_commandsPerBlock > 0,
                "CCode - commandsPerBlock must be greater than 0 to enable block splitting");
  }

  bool shouldStartNewBlock = (m_IsStateRestore != s_stateRestore) ||
                             (m_CurrentFrame != s_currentFrame) ||
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
  s_stateRestore = m_IsStateRestore;
  s_currentFrame = m_CurrentFrame;
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

  if (c.m_ppvObject.Key) {
    // ID3D12StateObjectProperties
    CCodeStream::getInstance().addInterface(c.m_ppvObject.Key, c.m_riid.Value);
  }

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

void CCodeLayer::Post(NvAPI_InitializeCommand& c) {
  using namespace ccode;
  CommandPrinter p(c, "NvAPI_Initialize");
  preProcess(p, c);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_UnloadCommand& c) {
  using namespace ccode;
  CommandPrinter p(c, "NvAPI_Unload");
  preProcess(p, c);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  using namespace ccode;
  CppParameterInfo pDeviceInfo("ID3D12Device5", "pDevice");
  CppParameterInfo pStateInfo("NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS", "pState");
  pStateInfo.isPtr = true;
  CommandPrinter p(c, "NvAPI_D3D12_SetCreatePipelineStateOptions");
  preProcess(p, c);
  p.addArgument(c.m_pDevice, pDeviceInfo);
  p.addArgument(c.m_pState, pStateInfo);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  using namespace ccode;
  CppParameterInfo pDevInfo("IUnknown", "pDev");
  CommandPrinter p(c, "NvAPI_D3D12_SetNvShaderExtnSlotSpace");
  preProcess(p, c);
  p.addArgument(c.m_pDev, pDevInfo);
  p.addArgumentValue(c.m_uavSlot.Value);
  p.addArgumentValue(c.m_uavSpace.Value);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  using namespace ccode;
  CppParameterInfo pDevInfo("IUnknown", "pDev");
  CommandPrinter p(c, "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread");
  preProcess(p, c);
  p.addArgument(c.m_pDev, pDevInfo);
  p.addArgumentValue(c.m_uavSlot.Value);
  p.addArgumentValue(c.m_uavSpace.Value);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  using namespace ccode;
  CppParameterInfo pCommandListInfo("ID3D12GraphicsCommandList4", "pCommandList");
  CppParameterInfo pParamsInfo("NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS",
                               "pParams");
  pParamsInfo.isPtr = true;
  CommandPrinter p(c, "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx");
  preProcess(p, c);
  p.addArgument(c.m_pCommandList, pCommandListInfo);
  p.addArgument(c.m_pParams, pParamsInfo);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  using namespace ccode;
  CppParameterInfo pCommandListInfo("ID3D12GraphicsCommandList4", "pCommandList");
  CppParameterInfo pParamsInfo("NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS", "pParams");
  pParamsInfo.isPtr = true;
  CommandPrinter p(c, "NvAPI_D3D12_BuildRaytracingOpacityMicromapArray");
  preProcess(p, c);
  p.addArgument(c.m_pCommandList, pCommandListInfo);
  p.addArgument(c.m_pParams, pParamsInfo);
  postProcess(p, c);
  p.print();
  nextCommand();
}

void CCodeLayer::Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  using namespace ccode;
  CppParameterInfo pCommandListInfo("ID3D12GraphicsCommandList4", "pCommandList");
  CppParameterInfo pParamsInfo("NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS",
                               "pParams");
  pParamsInfo.isPtr = true;
  CommandPrinter p(c, "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation");
  preProcess(p, c);
  p.addArgument(c.m_pCommandList, pCommandListInfo);
  p.addArgument(c.m_pParams, pParamsInfo);
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

void CCodeLayer::preProcess(ccode::CommandPrinter& p, xessSetLoggingCallbackCommand& c) {
  p.skip();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, xellSetLoggingCallbackCommand& c) {
  p.skip();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, xefgSwapChainSetLoggingCallbackCommand& c) {
  p.skip();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, IDStorageFactoryOpenFileCommand& c) {
  static const wchar_t s_ResourcesFilePath[] = L"DirectStorageResources.bin";
  c.m_path.Value = const_cast<LPWSTR>(s_ResourcesFilePath);
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p, IDStorageQueue1EnqueueSetEventCommand& c) {
  p.skip();
}

void CCodeLayer::preProcess(ccode::CommandPrinter& p,
                            IDStorageCustomDecompressionQueueSetRequestResultsCommand& c) {
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

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             xefgSwapChainD3D12InitFromSwapChainCommand& c) {
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  m_XefgCmdQueueKeyByHandleKey[c.m_hSwapChain.Key] = c.m_pCmdQueue.Key;
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             xefgSwapChainD3D12InitFromSwapChainDescCommand& c) {
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  m_XefgCmdQueueKeyByHandleKey[c.m_hSwapChain.Key] = c.m_pCmdQueue.Key;
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             xefgSwapChainD3D12GetSwapChainPtrCommand& c) {
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  auto cmdQueueIt = m_XefgCmdQueueKeyByHandleKey.find(c.m_hSwapChain.Key);
  GITS_ASSERT(cmdQueueIt != m_XefgCmdQueueKeyByHandleKey.end());
  createSwapChain(p, c.m_ppSwapChain.Key, cmdQueueIt->second);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDXGISwapChainPresentCommand& c) {
  if (c.m_Flags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(c.Key)) {
    return;
  }
  present(p, c.m_Object.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDXGISwapChain1Present1Command& c) {
  if (c.m_PresentFlags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(c.Key)) {
    return;
  }
  present(p, c.m_Object.Key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, ID3D12FenceGetCompletedValueCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::WaitForFence(" << objKeyToPtrStr(c.m_Object.Key) << ", " << c.m_Result.Value
     << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDStorageFactoryOpenFileCommand& c) {
  p.getArgumentValue(0) = "directx::DirectStorageService::Get().ResourcesFilePath()";
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDStorageQueueEnqueueRequestCommand& c) {
  using namespace ccode;

  // Custom compression is not supported
  // Memory source is not supported
  if ((c.m_request.Value->Options.CompressionFormat & DSTORAGE_CUSTOM_COMPRESSION_0) ||
      (c.m_request.Value->Options.SourceType == DSTORAGE_REQUEST_SOURCE_MEMORY)) {
    p.skip();
    return;
  }

  std::ostringstream ss;
  // Argument order: DSTORAGE_REQUEST is the first (and only) non-trivial parameter.
  ss << "directx::DirectStorageService::Get().PrepareEnqueueRequest(" << c.m_Object.Key << ", "
     << p.getArgumentValue(0) << ", " << c.m_request.NewOffset << ");" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IDStorageQueueSubmitCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::DirectStorageService::Get().OnSubmit(" << c.m_Object.Key << ", "
     << "static_cast<IDStorageQueue1*>(" << objKeyToPtrStr(c.m_Object.Key) << "));" << std::endl;
  p.setPreCommand(ss.str());
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

void CCodeLayer::postProcess(ccode::CommandPrinter& p, INTC_D3D12_CreatePlacedResourceCommand& c) {
  createPlacedResource(p, c.m_ppvResource.Key, c.m_pHeap.Key, c.m_HeapOffset.Value,
                       "pD3D12Desc.Flags");
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, IUnknownReleaseCommand& c) {
  if (!c.m_Object.Key || !c.m_Object.Value) {
    p.skip();
    return;
  }
  std::ostringstream ss;
  const auto objectPtr = ccode::objKeyToPtrStr(c.m_Object.Key);
  ss << objectPtr << "->AddRef();" << std::endl;
  ss << "ULONG refCount = " << objectPtr << "->Release() - 1;" << std::endl;
  ss << "directx::RecordRelease(" << c.m_Object.Key << ", refCount);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (!c.m_Object.Key) {
    return;
  }
  std::ostringstream ss;
  const auto objectPtr = ccode::objKeyToPtrStr(c.m_Object.Key);
  ss << "directx::RecordGetGPUVirtualAddress(" << objectPtr << ", " << c.m_Object.Key << ", "
     << ccode::toHex(m_ExportGpuVirtualAddressCapture) << ");" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (!c.m_Object.Key) {
    return;
  }
  std::ostringstream ss;
  const auto objectPtr = ccode::objKeyToPtrStr(c.m_Object.Key);
  ss << "directx::RecordGetGPUDescriptorHandleForHeapStart(" << objectPtr << ", " << c.m_Object.Key
     << ", {" << ccode::toHex(m_ExportGpuDescriptorHandleCapture.ptr) << "});" << std::endl;
  p.setPreCommand(ss.str());
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

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (!c.m_Object.Key) {
    return;
  }
  std::ostringstream ss;
  const auto objectPtr = ccode::objKeyToPtrStr(c.m_Object.Key);
  const auto* bytes = static_cast<const uint8_t*>(m_ExportShaderIdentifierCapture.data());
  ss << "directx::RecordGetShaderIdentifier(" << objectPtr << ", " << p.getArgumentValue(0) << ", "
     << c.Key << ", ";
  ss << "std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>{";
  for (size_t i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    if (i > 0) {
      ss << ", ";
    }
    ss << static_cast<unsigned>(bytes[i]);
  }
  ss << "}.data());" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(
    ccode::CommandPrinter& p,
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (!c.m_Object.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::PatchService::Get().PreBuildRaytracingAccelerationStructure(" << c.m_Object.Key
     << ", " << c.Key << ", " << ccode::objKeyToPtrStr(c.m_Object.Key) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (!c.m_Object.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::PatchService::Get().PreDispatchRays(" << c.m_Object.Key << ", " << c.Key << ", "
     << ccode::objKeyToPtrStr(c.m_Object.Key) << ", &pDesc);" << std::endl;
  p.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (!c.m_Object.Key || !c.m_pCommandSignature.Key) {
    return;
  }
  const unsigned argumentBufferIndex =
      Configurator::Get().directx.player.cCode.wrapApiCalls ? 3u : 2u;
  const std::string originalArgumentBuffer = p.getArgumentValue(argumentBufferIndex);
  const std::string originalArgumentBufferOffset = p.getArgumentValue(argumentBufferIndex + 1);

  std::ostringstream pre;
  pre << "ID3D12Resource* patchExecuteIndirectArgumentBuffer = " << originalArgumentBuffer << ";"
      << std::endl;
  pre << "UINT64 patchExecuteIndirectArgumentBufferOffset = " << originalArgumentBufferOffset << ";"
      << std::endl;
  pre << "UINT64 captureExecuteIndirectArgumentBufferOffset = "
      << "patchExecuteIndirectArgumentBufferOffset;" << std::endl;
  pre << "directx::PatchService::Get().PreExecuteIndirect(" << c.m_Object.Key << ", " << c.Key
      << ", " << ccode::objKeyToPtrStr(c.m_Object.Key) << ", " << c.m_pCommandSignature.Key
      << ", &patchExecuteIndirectArgumentBuffer, &patchExecuteIndirectArgumentBufferOffset, "
      << c.m_MaxCommandCount.Value << ");" << std::endl;
  p.setPreCommand(pre.str());

  p.getArgumentValue(argumentBufferIndex) = "patchExecuteIndirectArgumentBuffer";
  p.getArgumentValue(argumentBufferIndex + 1) = "patchExecuteIndirectArgumentBufferOffset";

  std::ostringstream post;
  post << "directx::PatchService::Get().PostExecuteIndirect("
          "&patchExecuteIndirectArgumentBufferOffset, "
          "captureExecuteIndirectArgumentBufferOffset);"
       << std::endl;
  p.setPostCommand(post.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (!c.m_Object.Key || c.m_ppCommandLists.Keys.empty()) {
    return;
  }
  std::ostringstream pre;
  pre << "static const unsigned executeCommandListsKeys[] = {";
  for (size_t i = 0; i < c.m_ppCommandLists.Keys.size(); ++i) {
    if (i > 0) {
      pre << ", ";
    }
    pre << c.m_ppCommandLists.Keys[i];
  }
  pre << "};" << std::endl;
  pre << "directx::PatchService::Get().PreExecuteCommandLists(executeCommandListsKeys, "
      << c.m_ppCommandLists.Keys.size() << ");" << std::endl;
  p.setPreCommand(pre.str());

  std::ostringstream post;
  post << "directx::PatchService::Get().PostExecuteCommandLists("
       << ccode::objKeyToPtrStr(c.m_Object.Key) << ", executeCommandListsKeys, "
       << c.m_ppCommandLists.Keys.size() << ");" << std::endl;
  p.setPostCommand(post.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (!c.m_ppvCommandSignature.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::PatchService::Get().RegisterCommandSignature(" << c.m_ppvCommandSignature.Key
     << ", pDesc);" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, CreateDXGIFactoryCommand& c) {
  if (!c.m_ppFactory.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().OnDxgiFactoryCreated(" << p.getArgumentValue(0) << ", "
     << p.getArgumentValue(1) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, CreateDXGIFactory1Command& c) {
  if (!c.m_ppFactory.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().OnDxgiFactoryCreated(" << p.getArgumentValue(0) << ", "
     << p.getArgumentValue(1) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, CreateDXGIFactory2Command& c) {
  if (!c.m_ppFactory.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().OnDxgiFactoryCreated(" << p.getArgumentValue(1) << ", "
     << p.getArgumentValue(2) << ", " << c.m_Flags.Value << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p, D3D12CreateDeviceCommand& c) {
  if (!c.m_ppDevice.Key) {
    return;
  }
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().OnDeviceCreated("
     << ccode::objKeyToPtrStr(c.m_ppDevice.Key) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().FlushRaytracingValidation("
     << ccode::objKeyToPtrStr(c.m_pCommandList.Key) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().FlushRaytracingValidation("
     << ccode::objKeyToPtrStr(c.m_pCommandList.Key) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& p,
                             NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  std::ostringstream ss;
  ss << "directx::DebugLayerService::Get().FlushRaytracingValidation("
     << ccode::objKeyToPtrStr(c.m_pCommandList.Key) << ");" << std::endl;
  p.setPostCommand(ss.str());
}

} // namespace DirectX
} // namespace gits
