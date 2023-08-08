// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
  layoutBuilder.SaveLayoutToJsonFile();
}

template <>
typename CDriverState::states_type& CStateDynamic::Map<CDriverState>() {
  return driverStates_;
}
template <>
typename CDeviceState::states_type& CStateDynamic::Map<CDeviceState>() {
  return deviceStates_;
}
template <>
typename CKernelState::states_type& CStateDynamic::Map<CKernelState>() {
  return kernelStates_;
}
template <>
typename CModuleState::states_type& CStateDynamic::Map<CModuleState>() {
  return moduleStates_;
}
template <>
typename CAllocState::states_type& CStateDynamic::Map<CAllocState>() {
  return allocStates_;
}
template <>
typename CKernelArgumentDump::states_type& CStateDynamic::Map<CKernelArgumentDump>() {
  return kernelArgumentDumps_;
}
template <>
typename CKernelArgument::states_type& CStateDynamic::Map<CKernelArgument>() {
  return kernelArguments_;
}
template <>
typename CEventPoolState::states_type& CStateDynamic::Map<CEventPoolState>() {
  return eventPoolStates_;
}
template <>
typename CFenceState::states_type& CStateDynamic::Map<CFenceState>() {
  return fenceStates_;
}
template <>
typename CEventState::states_type& CStateDynamic::Map<CEventState>() {
  return eventStates_;
}
template <>
typename CContextState::states_type& CStateDynamic::Map<CContextState>() {
  return contextStates_;
}
template <>
typename CImageState::states_type& CStateDynamic::Map<CImageState>() {
  return imageStates_;
}
template <>
typename CCommandListState::states_type& CStateDynamic::Map<CCommandListState>() {
  return commandListStates_;
}
template <>
typename CCommandQueueState::states_type& CStateDynamic::Map<CCommandQueueState>() {
  return commandQueueStates_;
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

CAllocState::~CAllocState() {
  if (memType == UnifiedMemoryType::device || memType == UnifiedMemoryType::shared) {
    CGits::Instance().SubtractLocalMemoryUsage(size);
  }
}

CKernelState::CKernelState(ze_module_handle_t hModule, const ze_kernel_desc_t* kernelDesc)
    : hModule(hModule) {
  desc.stype = kernelDesc->stype;
  desc.pNext = kernelDesc->pNext;
  desc.flags = kernelDesc->flags;
  desc.pKernelName = new char[std::strlen(kernelDesc->pKernelName) + 1];
  std::strcpy(const_cast<char*>(desc.pKernelName), kernelDesc->pKernelName);
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
  CGits::Instance().SubtractLocalMemoryUsage(CalculateImageSize(desc));
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
                           ze_module_desc_t descriptor,
                           ze_module_build_log_handle_t hBuildLog)
    : hContext(hContext), hDevice(hDevice), desc(descriptor), hBuildLog(hBuildLog) {
  desc.pInputModule = new uint8_t[desc.inputSize + 1];
  std::memcpy((void*)desc.pInputModule, descriptor.pInputModule, desc.inputSize);
  if (descriptor.pBuildFlags != nullptr) {
    const auto buildFlagsSize = std::strlen(descriptor.pBuildFlags) + 1;
    desc.pBuildFlags = new char[buildFlagsSize];
    std::memcpy((void*)desc.pBuildFlags, descriptor.pBuildFlags, buildFlagsSize);
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
                                         uint32_t kernelArgIndex)
    : CKernelArgument(allocSize, ptr), kernelNumber(kernelNumber), kernelArgIndex(kernelArgIndex) {}

CKernelArgumentDump::CKernelArgumentDump(ze_image_desc_t imageDesc,
                                         size_t allocSize,
                                         ze_image_handle_t ptr,
                                         uint32_t kernelNumber,
                                         uint32_t kernelArgIndex)
    : CKernelArgument(allocSize, ptr),
      imageDesc(imageDesc),
      kernelNumber(kernelNumber),
      kernelArgIndex(kernelArgIndex) {}

void CKernelArgumentDump::UpdateIndexes(uint32_t kernelNum, uint32_t argIndex) {
  this->kernelNumber = kernelNum;
  this->kernelArgIndex = argIndex;
}

LayoutBuilder::LayoutBuilder() : layout(), latestFileName("") {
  layout.add("version", "1.0");
}

boost::property_tree::ptree LayoutBuilder::GetModuleLinkInfoPtree(
    CStateDynamic& sd, const std::unordered_set<ze_module_handle_t>& moduleLinks) const {
  boost::property_tree::ptree modulesInfo;
  for (const auto& module : moduleLinks) {
    boost::property_tree::ptree moduleInfo;
    const auto& moduleState = sd.Get<CModuleState>(module, EXCEPTION_MESSAGE);
    moduleInfo.add("module_file", moduleState.moduleFileName);
    moduleInfo.add("build_options", moduleState.desc.pBuildFlags);
    modulesInfo.push_back(std::make_pair("", moduleInfo));
  }
  return modulesInfo;
}

void LayoutBuilder::UpdateLayout(const char* pKernelName,
                                 const ze_module_handle_t& hModule,
                                 const CKernelExecutionInfo& kernelInfo,
                                 const uint32_t& queueSubmitNum,
                                 const uint32_t& cmdListNum,
                                 const uint32_t& argIndex) {
  UpdateExecutionKeyId(queueSubmitNum, cmdListNum, kernelInfo.kernelNumber);
  const auto executionKey = GetExecutionKeyId();
  if (zeKernels.find(executionKey) == zeKernels.not_found()) {
    Add("kernel_name", pKernelName);
    auto& sd = SD();
    const auto& moduleState = sd.Get<CModuleState>(hModule, EXCEPTION_MESSAGE);
    if (moduleState.desc.pBuildFlags != nullptr) {
      Add("build_options", moduleState.desc.pBuildFlags);
    }
    Add("module_file", moduleState.moduleFileName);
    if (moduleState.IsModuleLinkUsed()) {
      AddChild("module_link", GetModuleLinkInfoPtree(sd, moduleState.moduleLinks));
    }

#ifdef WITH_OCLOC
    AddOclocInfo(hModule);
#endif
  }
  const auto& arg = kernelInfo.GetArgument(argIndex);
  const auto argKey = "args." + std::to_string(argIndex);
  if (arg.type == KernelArgType::image) {
    boost::property_tree::ptree imageArgument;
    const auto& imgState = SD().Get<CImageState>(
        reinterpret_cast<ze_image_handle_t>(const_cast<void*>(arg.argValue)), EXCEPTION_MESSAGE);
    const auto imageDescription = GetImageDescription(imgState.desc);
    imageArgument.add_child(BuildFileName(argIndex, false), imageDescription);
    AddChild(argKey, imageArgument);
  } else {
    Add(argKey, BuildFileName(argIndex));
  }
}

std::string LayoutBuilder::GetFileName() {
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
  layout.add_child("ze_kernels", zeKernels);

  const auto& cfg = Config::Get();
  const bfs::path path =
      (cfg.player.outputDir.empty() ? cfg.common.streamDir / "dump" : cfg.player.outputDir) /
      "layout.json";
  SaveJsonFile(layout, path);
}

std::string LayoutBuilder::GetExecutionKeyId() {
  std::stringstream ss;
  ss << queueSubmitNumber << "_" << cmdListNumber << "_" << appendKernelNumber;
  return ss.str();
}

void LayoutBuilder::AddOclocInfo(const ze_module_handle_t& hModule) {
  const auto& oclocState = SD().Get<CModuleState>(hModule, EXCEPTION_MESSAGE).oclocState;
  if (oclocState.get() != nullptr && !oclocState->args.empty()) {
    auto children = boost::property_tree::ptree();
    const auto size = oclocState->args.size();
    for (auto i = 0U; i < size; i++) {
      auto child = boost::property_tree::ptree();
      child.put("", oclocState->args[i]);
      children.push_back(std::make_pair("", child));
      if (Config::Get().IsRecorder() && oclocState->args[i] == "-options") {
        auto nextChild = boost::property_tree::ptree();
        std::string options = oclocState->args[++i];
        options += " -I \"" + Config::Get().common.streamDir.string() + "\"";
        options += " -I \"" + (Config::Get().common.streamDir / "gitsFiles").string() + "\"";
        nextChild.put("", options);
        children.push_back(std::make_pair("", nextChild));
      }
    }
    AddArray("ocloc.sources", oclocState->savedFileNames);
    AddChild("ocloc.args", children);
  }
}

std::string LayoutBuilder::BuildFileName(const uint32_t& argNumber, bool isBuffer) {
  static int fileCounter = 1;
  std::stringstream fileName;
  fileName << (isBuffer ? "NDRangeBuffer_" : "NDRangeImage_");
  fileName << queueSubmitNumber << "_" << cmdListNumber << "_";
  fileName << appendKernelNumber << "_arg_" << argNumber << "_" << fileCounter++;
  latestFileName = fileName.str();
  return latestFileName;
}

boost::property_tree::ptree LayoutBuilder::GetImageDescription(const ze_image_desc_t& imageDesc) {
  boost::property_tree::ptree imageDescription;
  std::string image_type = get_texel_format_string(
      GetTexelTypeArrayFromLayout(imageDesc.format.layout)[imageDesc.format.type]);
  imageDescription.add("image_type", image_type);
  imageDescription.add("image_width", imageDesc.width);
  imageDescription.add("image_height", imageDesc.height);
  imageDescription.add("image_depth", imageDesc.depth);
  return imageDescription;
}
} // namespace l0
} // namespace gits
