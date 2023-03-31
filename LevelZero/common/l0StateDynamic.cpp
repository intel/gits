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
      memType(UnifiedMemoryType::shared) {}

CAllocState::CAllocState(ze_context_handle_t hContext,
                         ze_device_mem_alloc_desc_t device_desc,
                         size_t size,
                         size_t alignment,
                         ze_device_handle_t hDevice)
    : hContext(hContext),
      device_desc(device_desc),
      size(size),
      alignment(alignment),
      hDevice(hDevice) {}

CAllocState::CAllocState(ze_module_handle_t hModule,
                         const char* ptrName,
                         size_t size,
                         AllocStateType allocType)
    : hModule(hModule),
      name(std::string(ptrName)),
      size(size),
      memType(UnifiedMemoryType::device),
      allocType(allocType) {}

CAllocState::CAllocState(ze_context_handle_t hContext,
                         ze_host_mem_alloc_desc_t host_desc,
                         size_t size,
                         size_t alignment)
    : hContext(hContext),
      host_desc(host_desc),
      size(size),
      alignment(alignment),
      memType(UnifiedMemoryType::host) {}

CKernelState::CKernelState(ze_module_handle_t hModule, const ze_kernel_desc_t* kernelDesc)
    : hModule(hModule) {
  desc.stype = kernelDesc->stype;
  desc.pNext = kernelDesc->pNext;
  desc.flags = kernelDesc->flags;
  desc.pKernelName = new char[std::strlen(kernelDesc->pKernelName) + 1];
  std::strcpy(const_cast<char*>(desc.pKernelName), kernelDesc->pKernelName);
}

void CKernelState::CKernelExecutionInfo::SetArgument(uint32_t index,
                                                     size_t typeSize,
                                                     const void* value) {
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

const std::map<uint32_t, CKernelState::ArgInfo>& CKernelState::CKernelExecutionInfo::GetArguments()
    const {
  return args;
}

const CKernelState::ArgInfo& CKernelState::CKernelExecutionInfo::GetArgument(
    const uint32_t& index) const {
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
    : hContext(hContext), hDevice(hDevice), desc(desc) {}

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
                           ze_module_desc_t desc,
                           ze_module_build_log_handle_t hBuildLog)
    : hContext(hContext), hDevice(hDevice), desc(desc), hBuildLog(hBuildLog) {
#ifdef WITH_OCLOC
  const uint64_t hash = ComputeHash(desc.pInputModule, desc.inputSize, THashType::XX);
  if (!ocloc::SD().oclocStates.empty() &&
      ocloc::SD().oclocStates.find(hash) != ocloc::SD().oclocStates.end()) {
    oclocState = ocloc::SD().oclocStates.at(hash);
  }
#endif
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
    : argType(KernelArgType::buffer), buffer(allocSize, '\0'), h_buf(ptr) {}

CKernelArgument::CKernelArgument(size_t allocSize, ze_image_handle_t ptr)
    : argType(KernelArgType::image), buffer(allocSize, '\0'), h_img(ptr) {}

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

LayoutBuilder::LayoutBuilder() : layout(), latestFileName("") {}

void LayoutBuilder::UpdateLayout(const CKernelState& ks,
                                 const CKernelState::CKernelExecutionInfo& kernelInfo,
                                 const uint32_t& queueSubmitNum,
                                 const uint32_t& cmdListNum,
                                 const uint32_t& argIndex) {
  const auto& arg = kernelInfo.GetArgument(argIndex);
  if (queueSubmitNumber != queueSubmitNum || cmdListNumber != cmdListNum ||
      appendKernelNumber != kernelInfo.kernelNumber) {
#ifdef WITH_OCLOC
    AddBuildOptions(ks.desc.pKernelName, ks.hModule);
#endif
    queueSubmitNumber = queueSubmitNum;
    cmdListNumber = cmdListNum;
    appendKernelNumber = kernelInfo.kernelNumber;
  }
  if (arg.type == KernelArgType::image) {
    const auto& imgState = SD().Get<CImageState>(
        reinterpret_cast<ze_image_handle_t>(const_cast<void*>(arg.argValue)), EXCEPTION_MESSAGE);
    AddImage(argIndex, imgState.desc, ks.desc.pKernelName);
  } else {
    AddBuffer(argIndex, ks.desc.pKernelName);
  }
}

std::string LayoutBuilder::GetFileName() {
  return latestFileName;
}

void LayoutBuilder::SaveLayoutToJsonFile() {
  if (layout.empty()) {
    return;
  }
  const auto& cfg = Config::Get();
  const bfs::path path =
      (cfg.player.outputDir.empty() ? cfg.common.streamDir / "dump" : cfg.player.outputDir) /
      "layout.json";
  SaveJsonFile(layout, path);
}

std::string LayoutBuilder::GetKeyName(const uint32_t& argNumber, const char* pKernelName) {
  std::stringstream ss;
  ss << pKernelName << ".ZeKernelLaunch." << queueSubmitNumber << "_" << cmdListNumber << "_"
     << appendKernelNumber << ".arg_" << argNumber;
  return ss.str();
}

void LayoutBuilder::AddBuildOptions(const char* pKernelName, const ze_module_handle_t& hModule) {
  std::string key(std::string(pKernelName) + ".ocloc_arguments");
  std::string pathToBuildOptions(layout.get<std::string>(key, ""));
  if (pathToBuildOptions.empty()) {
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
      layout.add_child(key, children);
    }
  }
}

std::string LayoutBuilder::BuildFileName(const uint32_t& argNumber, bool isBuffer) {
  static int fileCounter = 1;
  std::stringstream fileName;
  fileName << (isBuffer ? "NDRangeBuffer_" : "NDRangeImage_") << queueSubmitNumber << "_"
           << cmdListNumber << "_" << appendKernelNumber << "_arg_" << argNumber << "_"
           << fileCounter++;
  latestFileName = fileName.str();
  return latestFileName;
}

void LayoutBuilder::AddBuffer(const uint32_t& argNumber, const char* pKernelName) {
  layout.add(GetKeyName(argNumber, pKernelName), BuildFileName(argNumber));
}

void LayoutBuilder::AddImage(const uint32_t& argNumber,
                             const ze_image_desc_t& imageDesc,
                             const char* pKernelName) {
  boost::property_tree::ptree ptTemp, imageDescription;
  std::string image_type = get_texel_format_string(
      GetTexelTypeArrayFromLayout(imageDesc.format.layout)[imageDesc.format.type]);
  imageDescription.add("image_type", image_type);
  imageDescription.add("image_width", imageDesc.width);
  imageDescription.add("image_height", imageDesc.height);
  imageDescription.add("image_depth", imageDesc.depth);
  ptTemp.add_child(BuildFileName(argNumber, false), imageDescription);
  layout.add_child(GetKeyName(argNumber, pKernelName), ptTemp);
}
} // namespace l0
} // namespace gits
