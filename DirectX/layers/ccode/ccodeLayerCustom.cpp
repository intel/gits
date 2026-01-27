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

static void createResource(ccode::CommandPrinter& cmdPrinter, unsigned resourceKey) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreateResource(" << resourceKey << ", "
     << ccode::objKeyToPtrStr(resourceKey) << ");" << std::endl;
  cmdPrinter.setPostCommand(ss.str());
}

static void createPlacedResource(ccode::CommandPrinter& cmdPrinter,
                                 unsigned resourceKey,
                                 unsigned heapKey,
                                 uint64_t heapOffset) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreatePlacedResource(" << resourceKey << ", "
     << ccode::objKeyToPtrStr(resourceKey) << ", " << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ", " << heapOffset << ");" << std::endl;
  cmdPrinter.setPostCommand(ss.str());
}

static void createHeap(ccode::CommandPrinter& cmdPrinter, unsigned heapKey) {
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().CreateHeap(" << heapKey << ", "
     << ccode::objKeyToPtrStr(heapKey) << ");" << std::endl;
  cmdPrinter.setPostCommand(ss.str());
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

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12FenceGetCompletedValueCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::WaitForFence(" << objKeyToPtrStr(c.object_.key) << ", " << c.result_.value << ");"
     << std::endl;
  cmdPrinter.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreateDescriptorHeapCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::DescriptorHeapService::Get().Create(" << objKeyToStr(c.ppvHeap_.key) << ", "
     << objKeyToPtrStr(c.ppvHeap_.key) << ", &pDescriptorHeapDesc);" << std::endl;
  cmdPrinter.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter, ID3D12ResourceMapCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::MapTrackingService::Get().MapResource(" << objKeyToStr(c.object_.key) << ", "
     << c.Subresource_.value << ", " << toHex(c.ppData_.captureValue) << ", &ppData);" << std::endl;
  cmdPrinter.setPostCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreateCommittedResourceCommand& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device4CreateCommittedResource1Command& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device8CreateCommittedResource2Command& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device10CreateCommittedResource3Command& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreateReservedResourceCommand& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device4CreateReservedResource1Command& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device10CreateReservedResource2Command& c) {
  createResource(cmdPrinter, c.ppvResource_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12DeviceCreatePlacedResourceCommand& c) {
  createPlacedResource(cmdPrinter, c.ppvResource_.key, c.pHeap_.key, c.HeapOffset_.value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device8CreatePlacedResource1Command& c) {
  createPlacedResource(cmdPrinter, c.ppvResource_.key, c.pHeap_.key, c.HeapOffset_.value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device10CreatePlacedResource2Command& c) {
  createPlacedResource(cmdPrinter, c.ppvResource_.key, c.pHeap_.key, c.HeapOffset_.value);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter, ID3D12DeviceCreateHeapCommand& c) {
  createHeap(cmdPrinter, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device4CreateHeap1Command& c) {
  createHeap(cmdPrinter, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter, INTC_D3D12_CreateHeapCommand& c) {
  createHeap(cmdPrinter, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "void* pAddress = directx::HeapAllocationService::Get().GetHeapAllocation("
     << objKeyToStr(c.ppvHeap_.key) << ");" << std::endl;
  cmdPrinter.setPreCommand(ss.str());
  cmdPrinter.getArgumentValue(0) = "pAddress";
  createHeap(cmdPrinter, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "void* pAddress = directx::HeapAllocationService::Get().GetHeapAllocation("
     << objKeyToStr(c.ppvHeap_.key) << ");" << std::endl;
  cmdPrinter.setPreCommand(ss.str());
  cmdPrinter.getArgumentValue(0) = "pAddress";
  createHeap(cmdPrinter, c.ppvHeap_.key);
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadGraphicsPipeline(" << objKeyToPtrStr(c.object_.key) << ", "
     << cmdPrinter.getArgumentValue(0) << ", &pDesc);" << std::endl;
  cmdPrinter.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadComputePipeline(" << objKeyToPtrStr(c.object_.key) << ", "
     << cmdPrinter.getArgumentValue(0) << ", &pDesc);" << std::endl;
  cmdPrinter.setPreCommand(ss.str());
}

void CCodeLayer::postProcess(ccode::CommandPrinter& cmdPrinter,
                             ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  using namespace ccode;
  std::ostringstream ss;
  ss << "directx::PreloadPipeline(" << objKeyToPtrStr(c.object_.key) << ", "
     << cmdPrinter.getArgumentValue(0) << ", &pDesc);" << std::endl;
  cmdPrinter.setPreCommand(ss.str());
}

} // namespace DirectX
} // namespace gits
