// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   l0StateDynamic.cpp
*
* @brief Definition of OpenCL common part library implementation.
*
*/

#include "l0StateDynamic.h"
#include "l0Header.h"
#include "l0Tools.h"
#include <string>

#ifdef WITH_OCLOC
#include "oclocFunctions.h"
#include "oclocStateDynamic.h"
#include "tools.h"
#endif

namespace gits {
namespace l0 {
CStateDynamic* CStateDynamic::instance;

CStateDynamic& CStateDynamic::Instance() {
  if (instance == nullptr) {
    instance = new CStateDynamic;
  }
  return *instance;
}

CStateDynamic* CStateDynamic::InstancePtr() {
  return instance;
}

CStateDynamic::~CStateDynamic() {
  try {
    layoutBuilder.SaveLayoutToJsonFile();
  } catch (...) {
    topmost_exception_handler("CStateDynamic::~CStateDynamic");
  }
}

template <>
const typename CDriverState::states_type& CStateDynamic::Map<CDriverState>() const {
  return driverStates_;
}
template <>
typename CDriverState::states_type& CStateDynamic::Map<CDriverState>() {
  return driverStates_;
}
template <>
const typename CDeviceState::states_type& CStateDynamic::Map<CDeviceState>() const {
  return deviceStates_;
}
template <>
typename CDeviceState::states_type& CStateDynamic::Map<CDeviceState>() {
  return deviceStates_;
}
template <>
const typename CKernelState::states_type& CStateDynamic::Map<CKernelState>() const {
  return kernelStates_;
}
template <>
typename CKernelState::states_type& CStateDynamic::Map<CKernelState>() {
  return kernelStates_;
}
template <>
const typename CModuleState::states_type& CStateDynamic::Map<CModuleState>() const {
  return moduleStates_;
}
template <>
typename CModuleState::states_type& CStateDynamic::Map<CModuleState>() {
  return moduleStates_;
}
template <>
const typename CAllocState::states_type& CStateDynamic::Map<CAllocState>() const {
  return allocStates_;
}
template <>
typename CAllocState::states_type& CStateDynamic::Map<CAllocState>() {
  return allocStates_;
}
template <>
const typename CKernelArgumentDump::states_type& CStateDynamic::Map<CKernelArgumentDump>() const {
  return kernelArgumentDumps_;
}
template <>
typename CKernelArgumentDump::states_type& CStateDynamic::Map<CKernelArgumentDump>() {
  return kernelArgumentDumps_;
}
template <>
const typename CEventPoolState::states_type& CStateDynamic::Map<CEventPoolState>() const {
  return eventPoolStates_;
}
template <>
typename CEventPoolState::states_type& CStateDynamic::Map<CEventPoolState>() {
  return eventPoolStates_;
}
template <>
const typename CFenceState::states_type& CStateDynamic::Map<CFenceState>() const {
  return fenceStates_;
}
template <>
typename CFenceState::states_type& CStateDynamic::Map<CFenceState>() {
  return fenceStates_;
}
template <>
const typename CEventState::states_type& CStateDynamic::Map<CEventState>() const {
  return eventStates_;
}
template <>
typename CEventState::states_type& CStateDynamic::Map<CEventState>() {
  return eventStates_;
}
template <>
const typename CContextState::states_type& CStateDynamic::Map<CContextState>() const {
  return contextStates_;
}
template <>
typename CContextState::states_type& CStateDynamic::Map<CContextState>() {
  return contextStates_;
}
template <>
const typename CImageState::states_type& CStateDynamic::Map<CImageState>() const {
  return imageStates_;
}
template <>
typename CImageState::states_type& CStateDynamic::Map<CImageState>() {
  return imageStates_;
}
template <>
const typename CCommandListState::states_type& CStateDynamic::Map<CCommandListState>() const {
  return commandListStates_;
}
template <>
typename CCommandListState::states_type& CStateDynamic::Map<CCommandListState>() {
  return commandListStates_;
}
template <>
const typename CCommandQueueState::states_type& CStateDynamic::Map<CCommandQueueState>() const {
  return commandQueueStates_;
}
template <>
typename CCommandQueueState::states_type& CStateDynamic::Map<CCommandQueueState>() {
  return commandQueueStates_;
}
template <>
typename CPhysicalMemState::states_type& CStateDynamic::Map<CPhysicalMemState>() {
  return physicalMemStates_;
}
template <>
const typename CPhysicalMemState::states_type& CStateDynamic::Map<CPhysicalMemState>() const {
  return physicalMemStates_;
}
CAllocState::CAllocState(ze_context_handle_t hContext,
                         ze_device_mem_alloc_desc_t device_desc,
                         ze_host_mem_alloc_desc_t host_desc,
                         size_t size,
                         size_t alignment,
                         ze_device_handle_t hDevice)
    : hContext(hContext),
      device_desc(device_desc),
      host_desc(host_desc),
      size(size),
      alignment(alignment),
      hDevice(hDevice),
      memType(UnifiedMemoryType::shared) {
  CGits::Instance().AddLocalMemoryUsage(size);
}

CAllocState::CAllocState(ze_context_handle_t hContext,
                         ze_device_mem_alloc_desc_t device_desc,
                         size_t size,
                         size_t alignment,
                         ze_device_handle_t hDevice)
    : hContext(hContext),
      device_desc(device_desc),
      size(size),
      alignment(alignment),
      hDevice(hDevice) {
  CGits::Instance().AddLocalMemoryUsage(size);
}

CAllocState::CAllocState(ze_module_handle_t hModule,
                         const char* ptrName,
                         size_t size,
                         AllocStateType allocType)
    : hModule(hModule),
      name(std::string(ptrName)),
      size(size),
      memType(UnifiedMemoryType::device),
      allocType(allocType) {
  CGits::Instance().AddLocalMemoryUsage(size);
}

CAllocState::CAllocState(ze_context_handle_t hContext,
                         ze_host_mem_alloc_desc_t host_desc,
                         size_t size,
                         size_t alignment)
    : hContext(hContext),
      host_desc(host_desc),
      size(size),
      alignment(alignment),
      memType(UnifiedMemoryType::host) {}

CAllocState::CAllocState(ze_context_handle_t hContext, size_t size, const void* pStart)
    : hContext(hContext),
      size(size),
      memType(UnifiedMemoryType::device),
      allocType(AllocStateType::virtual_pointer),
      pointerHint(pStart) {}

CAllocState::~CAllocState() {
  try {
    if (memType == UnifiedMemoryType::device || memType == UnifiedMemoryType::shared) {
      CGits::Instance().SubtractLocalMemoryUsage(size);
    }
  } catch (...) {
    topmost_exception_handler("CAllocState::~CAllocState");
  }
}

CKernelState::CKernelState(ze_module_handle_t hModule, const ze_kernel_desc_t* kernelDesc)
    : hModule(hModule) {
  desc.stype = kernelDesc->stype;
  desc.pNext = kernelDesc->pNext;
  desc.flags = kernelDesc->flags;
  desc.pKernelName = new char[std::strlen(kernelDesc->pKernelName) + 1];
  std::strcpy(const_cast<char*>(desc.pKernelName), kernelDesc->pKernelName);
  currentKernelInfo = std::make_unique<CKernelExecutionInfo>();
}

CKernelState::~CKernelState() {
  delete[] desc.pKernelName;
}

void CKernelExecutionInfo::SetArgument(uint32_t index, size_t typeSize, const void* value) {
  args[index].originalValue = value;
  args[index].type = KernelArgType::pointer;
  if (value) {
    pointers.push_back(std::make_shared<intptr_t>());
    std::memcpy(pointers.back().get(), value, sizeof(intptr_t));
    value = reinterpret_cast<const void*>(*pointers.back());
    auto h_buf = reinterpret_cast<void*>(*pointers.back());
    const auto allocPair = GetAllocFromRegion(h_buf, SD());
    if (SD().Exists<CAllocState>(allocPair.first)) {
      args[index].type = KernelArgType::buffer;
      args[index].argSize =
          SD().Get<CAllocState>(allocPair.first, EXCEPTION_MESSAGE).size - allocPair.second;
    }
    auto h_img = reinterpret_cast<ze_image_handle_t>(*pointers.back());
    if (SD().Exists<CImageState>(h_img)) {
      args[index].type = KernelArgType::image;
      args[index].desc = SD().Get<CImageState>(h_img, EXCEPTION_MESSAGE).desc;
    }
  }
  args[index].argValue = value;
  args[index].typeSize = typeSize;
}

const std::map<uint32_t, ArgInfo>& CKernelExecutionInfo::GetArguments() const {
  return args;
}

const ArgInfo& CKernelExecutionInfo::GetArgument(const uint32_t& index) const {
  return args.at(index);
}

CCommandListState::CCommandListState(ze_context_handle_t hContext,
                                     ze_device_handle_t hDevice,
                                     ze_command_list_desc_t desc)
    : hContext(hContext), hDevice(hDevice), listDesc(desc) {}

CCommandListState::CCommandListState(ze_context_handle_t hContext,
                                     ze_device_handle_t hDevice,
                                     ze_command_queue_desc_t altdesc)
    : hContext(hContext),
      hDevice(hDevice),
      queueDesc(altdesc),
      isImmediate(true),
      isSync(altdesc.mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {}

CImageState::CImageState(ze_context_handle_t hContext,
                         ze_device_handle_t hDevice,
                         ze_image_desc_t desc)
    : hContext(hContext), hDevice(hDevice), desc(desc) {
  CGits::Instance().AddLocalMemoryUsage(CalculateImageSize(desc));
}

CImageState::~CImageState() {
  try {
    CGits::Instance().SubtractLocalMemoryUsage(CalculateImageSize(desc));
  } catch (...) {
    topmost_exception_handler("CImageState::~CImageState");
  }
}

CCommandQueueState::CCommandQueueState(ze_context_handle_t hContext,
                                       ze_device_handle_t hDevice,
                                       ze_command_queue_desc_t desc)
    : hContext(hContext),
      hDevice(hDevice),
      desc(desc),
      isSync(desc.mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {}

CDeviceState::CDeviceState(ze_driver_handle_t hDriver) : hDriver(hDriver) {}

CDeviceState::CDeviceState(ze_device_handle_t hDevice) : hDevice(hDevice) {}

CModuleState::CModuleState(ze_context_handle_t hContext,
                           ze_device_handle_t hDevice,
                           const ze_module_desc_t* descriptor,
                           ze_module_build_log_handle_t hBuildLog)
    : hContext(hContext), hDevice(hDevice), desc(*descriptor), hBuildLog(hBuildLog) {
  desc.pInputModule = new uint8_t[desc.inputSize + 1];
  std::memcpy((void*)desc.pInputModule, descriptor->pInputModule, desc.inputSize);
  if (descriptor->pBuildFlags != nullptr) {
    const auto buildFlagsSize = std::strlen(descriptor->pBuildFlags) + 1;
    desc.pBuildFlags = new char[buildFlagsSize];
    std::memcpy((void*)desc.pBuildFlags, descriptor->pBuildFlags, buildFlagsSize);
  }
  if (descriptor->pConstants != nullptr && descriptor->pConstants->numConstants == 0) {
    desc.pConstants = nullptr;
  }
#ifdef WITH_OCLOC
  const uint64_t hash = ComputeHash(desc.pInputModule, desc.inputSize, THashType::XX);
  if (!ocloc::SD().oclocStates.empty() &&
      ocloc::SD().oclocStates.find(hash) != ocloc::SD().oclocStates.end()) {
    oclocState = ocloc::SD().oclocStates.at(hash);
  }
#endif
}

CModuleState::~CModuleState() {
  delete[] desc.pInputModule;
  if (desc.pBuildFlags != nullptr) {
    delete[] desc.pBuildFlags;
  }
}

void CModuleState::AddModuleLinks(const uint32_t& numModules, const ze_module_handle_t* phModules) {
  for (auto i = 0U; i < numModules; i++) {
    moduleLinks.insert(phModules[i]);
  }
}

bool CModuleState::IsModuleLinkUsed() const {
  return !moduleLinks.empty();
}

CContextState::CContextState(ze_driver_handle_t hDriver, ze_context_desc_t desc)
    : hDriver(hDriver), desc(desc) {}

CContextState::CContextState(ze_driver_handle_t hDriver,
                             ze_context_desc_t desc,
                             uint32_t numDevices,
                             ze_device_handle_t* phDevices)
    : hDriver(hDriver), desc(desc) {
  hDevices.assign(phDevices, phDevices + numDevices);
}

CEventPoolState::CEventPoolState(ze_context_handle_t hContext, ze_event_pool_desc_t desc)
    : hContext(hContext), desc(desc) {}

CEventState::CEventState(ze_event_pool_handle_t hEventPool, ze_event_desc_t desc)
    : hEventPool(hEventPool), desc(desc) {}

CFenceState::CFenceState(ze_command_queue_handle_t hCommandQueue, ze_fence_desc_t desc)
    : hCommandQueue(hCommandQueue), desc(desc) {}

CKernelArgument::CKernelArgument(size_t allocSize, void* ptr)
    : argType(KernelArgType::buffer), h_buf(ptr) {
  AllocateBuffer(allocSize);
}

void CKernelArgument::AllocateBuffer(const size_t& allocSize) {
  if (!IsDumpOnlyLayoutEnabled(Config::Get())) {
    buffer = std::vector<char>(allocSize, '\0');
  }
}

CKernelArgument::CKernelArgument(size_t allocSize, ze_image_handle_t ptr)
    : argType(KernelArgType::image), h_img(ptr) {
  AllocateBuffer(allocSize);
}

CKernelArgumentDump::CKernelArgumentDump(size_t allocSize,
                                         void* ptr,
                                         uint32_t kernelNumber,
                                         uint64_t kernelArgIndex)
    : CKernelArgument(allocSize, ptr), kernelNumber(kernelNumber), kernelArgIndex(kernelArgIndex) {}

CKernelArgumentDump::CKernelArgumentDump(ze_image_desc_t imageDesc,
                                         size_t allocSize,
                                         ze_image_handle_t ptr,
                                         uint32_t kernelNumber,
                                         uint64_t kernelArgIndex)
    : CKernelArgument(allocSize, ptr),
      imageDesc(imageDesc),
      kernelNumber(kernelNumber),
      kernelArgIndex(kernelArgIndex) {}

void CKernelArgumentDump::UpdateIndexes(uint32_t kernelNum, uint32_t argIndex) {
  this->kernelNumber = kernelNum;
  this->kernelArgIndex = argIndex;
}

LayoutBuilder::LayoutBuilder() : layout(), latestFileName("") {
  layout["version"] = "1.0";
}

nlohmann::ordered_json LayoutBuilder::GetModuleLinkInfo(
    CStateDynamic& sd, const std::unordered_set<ze_module_handle_t>& moduleLinks) const {
  nlohmann::ordered_json modulesInfo = nlohmann::ordered_json::array();
  for (const auto& module : moduleLinks) {
    nlohmann::ordered_json moduleInfo;
    const auto& moduleState = sd.Get<CModuleState>(module, EXCEPTION_MESSAGE);
    moduleInfo["module_file"] = moduleState.moduleFileName;
    moduleInfo["build_options"] = moduleState.desc.pBuildFlags;
    modulesInfo.push_back(moduleInfo);
  }
  return modulesInfo;
}

void LayoutBuilder::UpdateLayout(const CKernelExecutionInfo* kernelInfo,
                                 const uint32_t& queueSubmitNum,
                                 const uint32_t& cmdListNum,
                                 const uint64_t& argIndex,
                                 bool isInputMode,
                                 bool isIndirectDump) {
  UpdateExecutionKeyId(queueSubmitNum, cmdListNum, kernelInfo->kernelNumber);
  const auto executionKey = GetExecutionKeyId();
  if (zeKernels.find(executionKey) == zeKernels.end()) {
    Add("kernel_name", kernelInfo->pKernelName);
    auto& sd = SD();
    const auto& moduleState = sd.Get<CModuleState>(kernelInfo->hModule, EXCEPTION_MESSAGE);
    if (moduleState.desc.pBuildFlags != nullptr) {
      Add("build_options", moduleState.desc.pBuildFlags);
    }
    Add("module_file", moduleState.moduleFileName);
    if (moduleState.IsModuleLinkUsed()) {
      Add("module_link", GetModuleLinkInfo(sd, moduleState.moduleLinks));
    }

#ifdef WITH_OCLOC
    AddOclocInfo(kernelInfo->hModule);
#endif
  }
  if (!isIndirectDump) {
    const auto& arg = kernelInfo->GetArgument(argIndex);
    if (arg.type == KernelArgType::image) {
      nlohmann::ordered_json imageArgument;
      const auto& imgState = SD().Get<CImageState>(
          reinterpret_cast<ze_image_handle_t>(const_cast<void*>(arg.argValue)), EXCEPTION_MESSAGE);
      const auto imageDescription = GetImageDescription(imgState.desc);
      imageArgument[BuildFileName(argIndex, false, isIndirectDump, isInputMode)] = imageDescription;
      if (!isInputMode) {
        Add("args", std::to_string(argIndex), imageArgument);
      }
    } else {
      const auto fileName = BuildFileName(argIndex, true, isIndirectDump, isInputMode);
      if (!isInputMode) {
        Add("args", std::to_string(argIndex), fileName);
      }
    }
  } else {
    std::stringstream keyIndex;
    keyIndex << std::hex << argIndex;
    const auto fileName = BuildFileName(argIndex, true, isIndirectDump, isInputMode);
    if (!isInputMode) {
      Add("indirect_args", keyIndex.str(), fileName);
    }
  }
}

bool LayoutBuilder::Exists(const uint32_t& queueSubmitNum,
                           const uint32_t& cmdListNum,
                           const uint32_t& kernelNumber,
                           const uint64_t& kernelArgIndex) const {
  std::stringstream ss;
  ss << queueSubmitNum << "_" << cmdListNum << "_" << kernelNumber;
  const auto key = ss.str();
  if (zeKernels.find(key) == zeKernels.end()) {
    return false;
  }
  const auto pickedZeKernel = zeKernels[key];
  if (pickedZeKernel.find("args") == pickedZeKernel.end()) {
    return false;
  }
  for (const auto& arg : pickedZeKernel["args"].items()) {
    if (arg.key() == std::to_string(kernelArgIndex)) {
      return true;
    }
  }
  return false;
}

std::string LayoutBuilder::GetFileName() const {
  return latestFileName;
}

void LayoutBuilder::UpdateExecutionKeyId(const uint32_t& queueSubmitNum,
                                         const uint32_t& cmdListNum,
                                         const uint32_t& kernelNumber) {
  queueSubmitNumber = queueSubmitNum;
  cmdListNumber = cmdListNum;
  appendKernelNumber = kernelNumber;
}

void LayoutBuilder::SaveLayoutToJsonFile() {
  if (zeKernels.empty()) {
    return;
  }
  layout["ze_kernels"] = zeKernels;

  const auto& cfg = Config::Get();
  const std::filesystem::path path =
      (cfg.player.outputDir.empty() ? cfg.common.streamDir / "dump" : cfg.player.outputDir) /
      "layout.json";
  SaveJsonFile(layout, path);
}

std::string LayoutBuilder::GetExecutionKeyId() const {
  std::stringstream ss;
  ss << queueSubmitNumber << "_" << cmdListNumber << "_" << appendKernelNumber;
  return ss.str();
}

void LayoutBuilder::AddOclocInfo(const ze_module_handle_t& hModule) {
  const auto& oclocState = SD().Get<CModuleState>(hModule, EXCEPTION_MESSAGE).oclocState;
  if (oclocState.get() != nullptr && !oclocState->args.empty()) {
    nlohmann::ordered_json children = nlohmann::ordered_json::array();
    const auto size = oclocState->args.size();
    for (auto i = 0U; i < size; i++) {
      children.push_back(oclocState->args[i]);
      if (Config::Get().IsRecorder() && oclocState->args[i] == "-options") {
        std::string options = oclocState->args[++i];
        options += " -I \"" + Config::Get().common.streamDir.string() + "\"";
        options += " -I \"" + (Config::Get().common.streamDir / "gitsFiles").string() + "\"";
        children.push_back(options);
      }
    }
    Add("ocloc", "sources", oclocState->savedFileNames);
    Add("ocloc", "args", children);
  }
}

std::string LayoutBuilder::BuildFileName(const uint64_t& argNumber,
                                         bool isBuffer,
                                         bool isIndirectDump,
                                         bool isInputMode) {
  static int fileCounter = 1;
  std::stringstream fileName;
  fileName << (isBuffer ? "NDRangeBuffer_" : "NDRangeImage_");
  if (isInputMode) {
    fileName << "input_";
  }
  fileName << queueSubmitNumber << "_" << cmdListNumber << "_";
  fileName << appendKernelNumber << "_arg_";
  if (isIndirectDump) {
    fileName << std::hex << argNumber << std::dec;
  } else {
    fileName << argNumber;
  }
  if (!isIndirectDump) {
    fileName << "_" << fileCounter;
    if (!isInputMode) {
      fileCounter++;
    }
  }
  latestFileName = fileName.str();
  return latestFileName;
}

nlohmann::ordered_json LayoutBuilder::GetImageDescription(const ze_image_desc_t& imageDesc) const {
  nlohmann::ordered_json imageDescription;
  const std::string image_type = get_texel_format_string(
      GetTexelTypeArrayFromLayout(imageDesc.format.layout)[imageDesc.format.type]);
  imageDescription["image_type"] = image_type;
  imageDescription["image_width"] = imageDesc.width;
  imageDescription["image_height"] = imageDesc.height;
  imageDescription["image_depth"] = imageDesc.depth;
  return imageDescription;
}

QueueSubmissionSnapshot::QueueSubmissionSnapshot(
    const ze_command_list_handle_t& cmdListHandle,
    const bool& isImmediate,
    const std::vector<std::shared_ptr<CKernelExecutionInfo>>& appendedKernels,
    const uint32_t& cmdListNum,
    const ze_context_handle_t& cmdListContext,
    const uint32_t& submissionNum,
    std::vector<CKernelArgumentDump>* argumentsVector) {
  if (isImmediate) {
    throw EOperationFailed(
        "Application used illegal operation by submitting immediate command list");
  }
  hCommandList = cmdListHandle;
  cmdQueueNumber = submissionNum;
  kernelsExecutionInfo = appendedKernels;
  cmdListNumber = cmdListNum;
  hContext = cmdListContext;
  readyArgVector = argumentsVector;
}
} // namespace l0
} // namespace gits
