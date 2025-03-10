// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "l0Arguments.h"
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

void GlobalSubmissionTracker::Add(ze_command_list_handle_t hCommandList,
                                  std::shared_ptr<CKernelExecutionInfo> currentKernelInfo) {
  queueSubmissionTracker[submissionId++] =
      std::make_unique<QueueSubmissionTracker>(hCommandList, currentKernelInfo);
}

void GlobalSubmissionTracker::SyncCheck() {
  std::vector<uint32_t> submissionsToRemove;
  for (auto& queueSubmissionInfo : queueSubmissionTracker) {
    if (queueSubmissionInfo.second->executionInfo->hSignalEvent == nullptr) {
      Log(WARN) << "Signal event is nullptr for kernel: "
                << queueSubmissionInfo.second->executionInfo->handle
                << ", cannot track kernel completion status";
      submissionsToRemove.push_back(queueSubmissionInfo.first);
    } else {
      if (drv.inject.zeEventQueryStatus(queueSubmissionInfo.second->executionInfo->hSignalEvent) ==
          ZE_RESULT_SUCCESS) {
        submissionsToRemove.push_back(queueSubmissionInfo.first);
      }
    }
  }
  for (const auto& submissionIdKey : submissionsToRemove) {
    queueSubmissionTracker.erase(submissionIdKey);
  }
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
  insertionOrderedId = ++CAllocState::insertionOrderedCounter;
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
  insertionOrderedId = ++CAllocState::insertionOrderedCounter;
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
  insertionOrderedId = ++CAllocState::insertionOrderedCounter;
}

CAllocState::CAllocState(ze_context_handle_t hContext,
                         ze_host_mem_alloc_desc_t host_desc,
                         size_t size,
                         size_t alignment)
    : hContext(hContext),
      host_desc(host_desc),
      size(size),
      alignment(alignment),
      memType(UnifiedMemoryType::host) {
  insertionOrderedId = ++CAllocState::insertionOrderedCounter;
}

CAllocState::CAllocState(ze_context_handle_t hContext, size_t size, const void* pStart)
    : hContext(hContext),
      size(size),
      memType(UnifiedMemoryType::device),
      allocType(AllocStateType::virtual_pointer),
      pointerHint(pStart) {
  CGits::Instance().AddLocalMemoryUsage(size);
  insertionOrderedId = ++CAllocState::insertionOrderedCounter;
}

CAllocState::~CAllocState() {
  try {
    if (memType == UnifiedMemoryType::device || memType == UnifiedMemoryType::shared) {
      CGits::Instance().SubtractLocalMemoryUsage(size);
    }
  } catch (...) {
    topmost_exception_handler("CAllocState::~CAllocState");
  }
}

uint32_t CAllocState::insertionOrderedCounter = 0;

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
  if (args[index].type == KernelArgType::pointer) {
    args[index].valueString = ToStringHelperHexMemoryView(
        reinterpret_cast<const uint8_t*>(args[index].originalValue), typeSize);
  } else {
    args[index].valueString = ToStringHelper(reinterpret_cast<void*>(*pointers.back()));
  }
  Log(TRACE) << "Argument[" << index << "] = " << args[index].valueString;
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

#ifdef WITH_OCLOC
ProgramInfo::ProgramInfo(const char* buildOptions,
                         const std::string filename,
                         std::shared_ptr<ocloc::COclocState>& oclocState)
    : moduleFileName(std::move(filename)), oclocState(oclocState) {
  if (buildOptions != nullptr) {
    this->buildOptions = std::string(buildOptions);
  }
}
#endif
ProgramInfo::ProgramInfo(const char* buildOptions, const std::string filename)
    : moduleFileName(std::move(filename)) {
  if (buildOptions != nullptr) {
    this->buildOptions = std::string(buildOptions);
  }
}

ze_module_constants_t* CModuleState::CreateNewModuleConstants(
    const ze_module_constants_t* originalConstants) {
  auto* pConstantIds = new uint32_t[originalConstants->numConstants]();
  std::memcpy(pConstantIds, originalConstants->pConstantIds,
              originalConstants->numConstants * sizeof(uint32_t));
  auto** pConstantValues = new uint64_t*[originalConstants->numConstants]();
  for (size_t i = 0U; i < originalConstants->numConstants; i++) {
    pConstantValues[i] = new uint64_t();
    std::memcpy(pConstantValues[i], originalConstants->pConstantValues[i], sizeof(uint64_t));
  }
  auto* pConstants = new ze_module_constants_t();
  pConstants->numConstants = originalConstants->numConstants;
  pConstants->pConstantIds = pConstantIds;
  pConstants->pConstantValues = const_cast<const void**>(reinterpret_cast<void**>(pConstantValues));
  return pConstants;
}

void CModuleState::SetModuleDesc(const ze_module_desc_t* originalDescriptor) {
  desc.stype = originalDescriptor->stype;
  desc.pNext = originalDescriptor->pNext;
  desc.format = originalDescriptor->format;
  if (IsPNextOfType(originalDescriptor->pNext, ZE_STRUCTURE_TYPE_MODULE_PROGRAM_EXP_DESC)) {
    const auto originalModuleProgramDesc =
        *reinterpret_cast<const ze_module_program_exp_desc_t*>(originalDescriptor->pNext);
    auto newModuleProgramDesc = new ze_module_program_exp_desc_t(originalModuleProgramDesc);

    newModuleProgramDesc->inputSizes = new size_t[originalModuleProgramDesc.count]();
    memcpy((void*)newModuleProgramDesc->inputSizes, originalModuleProgramDesc.inputSizes,
           originalModuleProgramDesc.count * sizeof(originalModuleProgramDesc.inputSizes[0]));
    newModuleProgramDesc->pInputModules = new const uint8_t*[originalModuleProgramDesc.count]();
    if (originalModuleProgramDesc.pBuildFlags != nullptr) {
      newModuleProgramDesc->pBuildFlags = new const char*[originalModuleProgramDesc.count]();
    }
    if (originalModuleProgramDesc.pConstants != nullptr) {
      newModuleProgramDesc->pConstants =
          new const ze_module_constants_t*[originalModuleProgramDesc.count];
    }

    for (size_t i = 0; i < originalModuleProgramDesc.count; i++) {
      newModuleProgramDesc->pInputModules[i] =
          new uint8_t[originalModuleProgramDesc.inputSizes[i]]();
      memcpy((void*)newModuleProgramDesc->pInputModules[i],
             originalModuleProgramDesc.pInputModules[i], originalModuleProgramDesc.inputSizes[i]);
      if (originalModuleProgramDesc.pBuildFlags != nullptr &&
          originalModuleProgramDesc.pBuildFlags[i] != nullptr) {
        const auto buildFlagsLength = strlen(originalModuleProgramDesc.pBuildFlags[i]);
        newModuleProgramDesc->pBuildFlags[i] = new char[buildFlagsLength]();
        memcpy((void*)newModuleProgramDesc->pBuildFlags[i],
               originalModuleProgramDesc.pBuildFlags[i], buildFlagsLength);
      }
      if (originalModuleProgramDesc.pConstants != nullptr &&
          originalModuleProgramDesc.pConstants[i] != nullptr &&
          originalModuleProgramDesc.pConstants[i]->numConstants != 0) {
        newModuleProgramDesc->pConstants[i] =
            CreateNewModuleConstants(originalModuleProgramDesc.pConstants[i]);
      } else if (originalModuleProgramDesc.pConstants != nullptr) {
        newModuleProgramDesc->pConstants[i] = nullptr;
      }
    }
    desc.pNext = newModuleProgramDesc;
  }

  desc.inputSize = originalDescriptor->inputSize;
  if (originalDescriptor->pInputModule != nullptr) {
    desc.pInputModule = new uint8_t[originalDescriptor->inputSize + 1];
    std::memcpy((void*)desc.pInputModule, originalDescriptor->pInputModule,
                originalDescriptor->inputSize);
  }
  if (originalDescriptor->pBuildFlags != nullptr) {
    const auto buildFlagsSize = std::strlen(originalDescriptor->pBuildFlags) + 1;
    desc.pBuildFlags = new char[buildFlagsSize];
    std::memcpy((void*)desc.pBuildFlags, originalDescriptor->pBuildFlags, buildFlagsSize);
  }
  if (originalDescriptor->pConstants != nullptr &&
      originalDescriptor->pConstants->numConstants == 0) {
    desc.pConstants = nullptr;
  } else if (originalDescriptor->pConstants != nullptr) {
    desc.pConstants = CreateNewModuleConstants(originalDescriptor->pConstants);
  }
}

void CModuleState::FillProgramInfo(std::vector<std::string> filenames) {
  if (IsPNextOfType(desc.pNext, ZE_STRUCTURE_TYPE_MODULE_PROGRAM_EXP_DESC)) {
    const auto moduleProgramDesc =
        *reinterpret_cast<const ze_module_program_exp_desc_t*>(desc.pNext);
    for (size_t i = 0; i < moduleProgramDesc.count; i++) {
      std::string filename = filenames.size() == moduleProgramDesc.count ? filenames[i] : "";
#ifdef WITH_OCLOC
      std::shared_ptr<ocloc::COclocState> oclocState;
      const uint64_t hash = ComputeHash(moduleProgramDesc.pInputModules[i],
                                        moduleProgramDesc.inputSizes[i], THashType::XX);
      if (!ocloc::SD().oclocStates.empty() &&
          ocloc::SD().oclocStates.find(hash) != ocloc::SD().oclocStates.end()) {
        oclocState = ocloc::SD().oclocStates.at(hash);
        programs.emplace_back(
            moduleProgramDesc.pBuildFlags != nullptr ? moduleProgramDesc.pBuildFlags[i] : nullptr,
            filename, oclocState);
      } else {
        programs.emplace_back(
            moduleProgramDesc.pBuildFlags != nullptr ? moduleProgramDesc.pBuildFlags[i] : nullptr,
            filename);
      }
#else
      programs.emplace_back(
          moduleProgramDesc.pBuildFlags != nullptr ? moduleProgramDesc.pBuildFlags[i] : nullptr,
          filename);
#endif
    }
  } else {
    std::string filename = !filenames.empty() ? filenames[0] : "";
#ifdef WITH_OCLOC
    std::shared_ptr<ocloc::COclocState> oclocState;
    const uint64_t hash = ComputeHash(desc.pInputModule, desc.inputSize, THashType::XX);
    if (!ocloc::SD().oclocStates.empty() &&
        ocloc::SD().oclocStates.find(hash) != ocloc::SD().oclocStates.end()) {
      oclocState = ocloc::SD().oclocStates.at(hash);
    }
    programs.emplace_back(desc.pBuildFlags, filename, oclocState);
#else
    programs.emplace_back(desc.pBuildFlags, filename);
#endif
  }
}

CModuleState::CModuleState(ze_context_handle_t hContext,
                           ze_device_handle_t hDevice,
                           const ze_module_desc_t* descriptor,
                           ze_module_build_log_handle_t hBuildLog,
                           std::vector<std::string> filenames)
    : hContext(hContext), hDevice(hDevice), hBuildLog(hBuildLog) {
  SetModuleDesc(descriptor);
  FillProgramInfo(filenames);
}

void CModuleState::DeleteModuleConstants(const ze_module_constants_t* pConstants) {
  if (pConstants != nullptr && pConstants->numConstants != 0) {
    delete[] pConstants->pConstantIds;
    for (size_t i = 0U; i < pConstants->numConstants; i++) {
      delete reinterpret_cast<const uint64_t*>(pConstants->pConstantValues[i]);
    }
    delete[] pConstants->pConstantValues;
    delete pConstants;
  }
}

CModuleState::~CModuleState() {
  if (IsPNextOfType(desc.pNext, ZE_STRUCTURE_TYPE_MODULE_PROGRAM_EXP_DESC)) {
    const auto& moduleProgramDesc =
        *reinterpret_cast<const ze_module_program_exp_desc_t*>(desc.pNext);
    for (size_t i = 0; i < moduleProgramDesc.count; i++) {
      delete[] moduleProgramDesc.pInputModules[i];
      if (moduleProgramDesc.pBuildFlags != nullptr && moduleProgramDesc.pBuildFlags[i] != nullptr) {
        delete[] moduleProgramDesc.pBuildFlags[i];
      }
      if (moduleProgramDesc.pConstants != nullptr) {
        DeleteModuleConstants(moduleProgramDesc.pConstants[i]);
      }
    }
    delete[] moduleProgramDesc.inputSizes;
    delete[] moduleProgramDesc.pInputModules;
    if (moduleProgramDesc.pBuildFlags != nullptr) {
      delete[] moduleProgramDesc.pBuildFlags;
    }
    if (moduleProgramDesc.pConstants != nullptr) {
      delete[] moduleProgramDesc.pConstants;
    }
  }
  if (desc.pInputModule != nullptr) {
    delete[] desc.pInputModule;
  }
  if (desc.pBuildFlags != nullptr) {
    delete[] desc.pBuildFlags;
  }
  DeleteModuleConstants(desc.pConstants);
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

CKernelArgumentDump::CKernelArgumentDump(uint32_t kernelNumber, uint64_t kernelArgIndex)
    : kernelNumber(kernelNumber), kernelArgIndex(kernelArgIndex) {}
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
  layout["version"] = "1.1";
}

nlohmann::ordered_json LayoutBuilder::GetModuleLinkInfo(
    CStateDynamic& sd, const std::unordered_set<ze_module_handle_t>& moduleLinks) const {
  nlohmann::ordered_json modulesInfo = nlohmann::ordered_json::array();
  for (const auto& module : moduleLinks) {
    nlohmann::ordered_json moduleInfo;
    const auto& moduleState = sd.Get<CModuleState>(module, EXCEPTION_MESSAGE);
    if (moduleState.programs.size() == 1) {
      moduleInfo["module_file"] = moduleState.programs[0].moduleFileName;
      moduleInfo["build_options"] = moduleState.programs[0].buildOptions;
    } else {
      nlohmann::ordered_json programsInfo;
      for (auto& program : moduleState.programs) {
        nlohmann::ordered_json programInfo;
        programInfo["module_file"] = program.moduleFileName;
        programInfo["build_options"] = program.buildOptions;
        programsInfo.push_back(moduleInfo);
      }
      moduleInfo["programs"] = programsInfo;
    }
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
    if (moduleState.programs.size() == 1) {
      if (moduleState.desc.pBuildFlags != nullptr) {
        Add("build_options", moduleState.programs[0].buildOptions);
      }
      Add("module_file", moduleState.programs[0].moduleFileName);
#ifdef WITH_OCLOC
      auto oclocInfo = GetOclocInfo(moduleState.programs[0].oclocState);
      if (oclocInfo.size() != 0) {
        Add("ocloc", oclocInfo);
      }
#endif
    } else {
      nlohmann::ordered_json modulesInfo = nlohmann::ordered_json::array();
      for (const auto& program : moduleState.programs) {
        nlohmann::ordered_json moduleInfo;
        moduleInfo["module_file"] = program.moduleFileName;
        moduleInfo["build_options"] = program.buildOptions;
#ifdef WITH_OCLOC
        auto oclocInfo = GetOclocInfo(program.oclocState);
        if (oclocInfo.size() != 0) {
          moduleInfo["ocloc"] = oclocInfo;
        }
#endif
        modulesInfo.push_back(moduleInfo);
      }
      Add("programs", modulesInfo);
    }
    if (moduleState.IsModuleLinkUsed()) {
      Add("module_link", GetModuleLinkInfo(sd, moduleState.moduleLinks));
    }
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
    } else if (arg.type == KernelArgType::buffer) {
      const auto fileName = BuildFileName(argIndex, true, isIndirectDump, isInputMode);
      if (!isInputMode) {
        Add("args", std::to_string(argIndex), fileName);
      }
    }
    Add("args_info", std::to_string(argIndex), "value", arg.valueString);
    Add("args_info", std::to_string(argIndex), "typeSize", arg.typeSize);
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
  const auto path = GetDumpPath(cfg) / "layout.json";
  SaveJsonFile(layout, path);
}

std::string LayoutBuilder::GetExecutionKeyId() const {
  std::stringstream ss;
  ss << queueSubmitNumber << "_" << cmdListNumber << "_" << appendKernelNumber;
  return ss.str();
}

#ifdef WITH_OCLOC
nlohmann::ordered_json LayoutBuilder::GetOclocInfo(
    const std::shared_ptr<ocloc::COclocState>& oclocState) {
  nlohmann::ordered_json oclocInfo;
  if (oclocState.get() != nullptr && !oclocState->args.empty()) {
    nlohmann::ordered_json children = nlohmann::ordered_json::array();
    const auto size = oclocState->args.size();
    for (auto i = 0U; i < size; i++) {
      children.push_back(oclocState->args[i]);
      if (Config::Get().IsRecorder() && oclocState->args[i] == "-options") {
        std::string options = oclocState->args[++i];
        options += " -I \"" + Config::Get().common.recorder.dumpPath.string() + "\"";
        options +=
            " -I \"" + (Config::Get().common.recorder.dumpPath / "gitsFiles").string() + "\"";
        children.push_back(options);
      }
    }
    oclocInfo["sources"] = oclocState->savedFileNames;
    oclocInfo["args"] = children;
  }
  return oclocInfo;
}
#endif

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
    const uint32_t& cmdQueueNum,
    std::vector<CKernelArgumentDump>* argumentsVector) {
  if (isImmediate) {
    throw EOperationFailed(
        "Application used illegal operation by submitting immediate command list");
  }
  hCommandList = cmdListHandle;
  cmdQueueNumber = cmdQueueNum;
  kernelsExecutionInfo = appendedKernels;
  cmdListNumber = cmdListNum;
  hContext = cmdListContext;
  readyArgVector = argumentsVector;
}

void DeallocationHandler::AddToResourcesInExecution(const ze_command_list_handle_t& hCommandList,
                                                    CUSMPtr& ptr,
                                                    const ze_event_handle_t& signalEvent) {
  if (ptr.IsMappedPointer()) {
    return;
  }
  auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  if (cmdListState.isImmediate && cmdListState.isSync) {
    ptr.FreeHostMemory();
    return;
  }
  playbackResourcesInExecution[hCommandList].emplace_back(&ptr, signalEvent);
}

void DeallocationHandler::DeallocateExecutedResources(
    const ze_command_list_handle_t& hCommandList) {
  for (auto& resource : playbackResourcesInExecution[hCommandList]) {
    resource.Deallocate();
  }
  playbackResourcesInExecution[hCommandList].clear();
}

void DeallocationHandler::DeallocateResourcesSynchedByEvent(const ze_event_handle_t& hEvent) {
  for (auto& cmdListState : SD().Map<CCommandListState>()) {
    if (cmdListState.second->isImmediate) {
      std::vector<DeallocationInfo> notSynchedResources;
      for (auto& resource : playbackResourcesInExecution[cmdListState.first]) {
        if (resource.hSignalEvent == hEvent) {
          resource.Deallocate();
        } else {
          notSynchedResources.push_back(resource);
        }
      }
      playbackResourcesInExecution[cmdListState.first] = std::move(notSynchedResources);
    }
  }
}

void DeallocationHandler::RemoveUseOfEvent(const ze_event_handle_t& hEvent) {
  for (auto& cmdListResources : playbackResourcesInExecution) {
    for (auto& resource : cmdListResources.second) {
      if (resource.hSignalEvent == hEvent) {
        resource.hSignalEvent = nullptr;
      }
    }
  }
}

void DeallocationHandler::DeallocationInfo::Deallocate() {
  usmPtrResource->FreeHostMemory();
}
} // namespace l0
} // namespace gits
