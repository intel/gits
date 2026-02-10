// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "exception.h"
#include "l0Header.h"
#include "l0Log.h"
#include "l0Tools.h"
#include "MemorySniffer.h"
#include <utility>
#ifdef WITH_OCLOC
#include "oclocStateDynamic.h"
#endif

#include <map>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "nlohmann/json.hpp"

namespace gits {
namespace l0 {
class CUSMPtr;

struct CState {
  CState() = default;
  virtual ~CState() = default;

  CState(CState const&) = delete;
  CState& operator=(CState const&) = delete;
  CState& operator=(CState&&) = delete;
  CState(CState&&) = delete;

  void RestoreReset() {
    _restored = false;
  }
  void RestoreFinished() {
    _restored = true;
  }
  bool Restored() const {
    return _restored;
  }

protected:
  bool _restored = false;
};

/** @brief
     * state responsible for saving pre-executed kernel arguments
     */
struct CKernelArgument {
  using type = ze_kernel_handle_t;
  using states_type = std::unordered_map<type, std::vector<CKernelArgument>>;
  KernelArgType argType = KernelArgType::pointer;
  std::vector<char> buffer;
  ze_image_handle_t h_img = nullptr;
  void* h_buf = nullptr;
  void AllocateBuffer(const size_t& allocSize);

public:
  CKernelArgument() = default;
  CKernelArgument(size_t allocSize, void* ptr);
  CKernelArgument(size_t allocSize, ze_image_handle_t ptr);
};
/** @brief
     * state responsible for kernel argument dumps
     */
struct CKernelArgumentDump : public CKernelArgument {
  using type = ze_command_list_handle_t;
  using states_type = std::unordered_map<type, std::vector<CKernelArgumentDump>>;
  ze_image_desc_t imageDesc = {};
  uint32_t kernelNumber = 0U;
  uint64_t kernelArgIndex = 0U;
  bool isIndirectDump = false;
  bool isInputArg = false;

public:
  bool injected = false;
  CKernelArgumentDump() = default;
  CKernelArgumentDump(size_t allocSize, void* ptr, uint32_t kernelNumber, uint64_t kernelArgIndex);
  CKernelArgumentDump(ze_image_desc_t imageDesc,
                      size_t allocSize,
                      ze_image_handle_t ptr,
                      uint32_t kernelNumber,
                      uint64_t kernelArgIndex);
  CKernelArgumentDump(uint32_t kernelNumber, uint64_t kernelArgIndex);
  void UpdateIndexes(uint32_t kernelNum, uint32_t argIndex);
};

struct CAllocState : public CState {
  using type = void*;
  using states_type = std::unordered_map<type, std::unique_ptr<CAllocState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_mem_alloc_desc_t device_desc = {};
  ze_host_mem_alloc_desc_t host_desc = {};
  ze_module_handle_t hModule = nullptr;
  std::string name;
  size_t size = 0U;
  size_t alignment = 0U;
  ze_device_handle_t hDevice = nullptr;
  PagedMemoryRegionHandle sniffedRegionHandle = nullptr;
  UnifiedMemoryType memType = UnifiedMemoryType::device;
  std::map<size_t, bool> indirectPointersOffsets;
  AllocStateType allocType = AllocStateType::pointer;
  std::vector<char> globalPtrAllocation;
  std::vector<char> originalGlobalPtrAllocation;
  bool savedForStateRestore = false;
  bool beingExecuted = false;

  bool modified = false;
  uint32_t scannedTimes = 0U;
  struct ResidencyInfo {
    ResidencyInfo() = default;
    ResidencyInfo(ze_context_handle_t hContext,
                  ze_device_handle_t hDevice,
                  size_t size,
                  size_t offset)
        : hContext(hContext), hDevice(hDevice), size(size), offset(offset) {}
    ze_context_handle_t hContext = nullptr;
    ze_device_handle_t hDevice = nullptr;
    size_t size = 0U;
    size_t offset = 0U;
  };
  std::unique_ptr<ResidencyInfo> residencyInfo;

  void* virtualMappingAddress = nullptr;

  const void* pointerHint = nullptr;
  struct VirtualMemMapInfo {
    size_t virtualMemorySizeFromOffset = 0U;
    ze_physical_mem_handle_t hPhysicalMemory = nullptr;
    size_t physicalMemoryOffset = 0U;
    ze_memory_access_attribute_t access = ZE_MEMORY_ACCESS_ATTRIBUTE_NONE;
    VirtualMemMapInfo(size_t virtualMemorySizeFromOffset,
                      ze_physical_mem_handle_t hPhysicalMemory,
                      size_t physicalMemoryOffset,
                      ze_memory_access_attribute_t access)
        : virtualMemorySizeFromOffset(virtualMemorySizeFromOffset),
          hPhysicalMemory(hPhysicalMemory),
          physicalMemoryOffset(physicalMemoryOffset),
          access(access) {}
  };
  // virtualMemoryOffset -> MemMapInfo
  std::map<size_t, std::shared_ptr<VirtualMemMapInfo>> memMaps;

  static uint32_t insertionOrderedCounter;
  uint32_t insertionOrderedId = 0U;

public:
  CAllocState() = default;
  CAllocState(ze_context_handle_t hContext,
              ze_device_mem_alloc_desc_t device_desc,
              ze_host_mem_alloc_desc_t host_desc,
              size_t size,
              size_t alignment,
              ze_device_handle_t hDevice);
  CAllocState(ze_context_handle_t hContext,
              ze_device_mem_alloc_desc_t device_desc,
              size_t size,
              size_t alignment,
              ze_device_handle_t hDevice);
  CAllocState(ze_context_handle_t hContext,
              ze_host_mem_alloc_desc_t host_desc,
              size_t size,
              size_t alignment);
  CAllocState(ze_module_handle_t hModule,
              const char* ptrName,
              size_t size,
              AllocStateType allocType);
  CAllocState(ze_context_handle_t hContext, size_t size, const void* pStart);
  ~CAllocState();
};

struct CPhysicalMemState : public CState {
  using type = ze_physical_mem_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CPhysicalMemState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_physical_mem_desc_t desc = {};
  CPhysicalMemState() = default;
  CPhysicalMemState(ze_context_handle_t hContext,
                    ze_device_handle_t hDevice,
                    ze_physical_mem_desc_t desc)
      : hContext(hContext), hDevice(hDevice), desc(desc) {}
};

struct ArgInfo {
  KernelArgType type = KernelArgType::pointer;
  size_t argSize = 0U;
  size_t typeSize = 0U;
  const void* argValue = nullptr;
  const void* originalValue = nullptr;
  ze_image_desc_t desc = {};
  std::string valueString;
  ArgInfo() = default;
};

struct CKernelExecutionInfo {
  ze_kernel_handle_t handle = nullptr;
  ze_group_count_t launchFuncArgs = {};
  ze_scheduling_hint_exp_flags_t schedulingHintFlags =
      static_cast<ze_scheduling_hint_exp_flags_t>(0U);
  unsigned indirectUsmTypes = 0U;
  uint32_t kernelNumber = 0U;
  uint32_t groupSizeX = 0U;
  uint32_t groupSizeY = 0U;
  uint32_t groupSizeZ = 0U;
  uint32_t offsetX = 0U;
  uint32_t offsetY = 0U;
  uint32_t offsetZ = 0U;
  bool isGroupSizeSet = false;
  bool isOffsetSet = false;
  ze_module_handle_t hModule = nullptr;
  std::string pKernelName;
  ze_event_handle_t hSignalEvent = nullptr;
  std::vector<std::unique_ptr<CKernelArgument>> stateRestoreBuffers;

private:
  std::map<uint32_t, ArgInfo> args;
  std::vector<std::shared_ptr<intptr_t>> pointers;

public:
  const std::map<uint32_t, ArgInfo>& GetArguments() const;
  const ArgInfo& GetArgument(const uint32_t& index) const;
  void SetArgument(uint32_t index, size_t typeSize, const void* value);
  CKernelExecutionInfo() = default;
  CKernelExecutionInfo(const std::unique_ptr<CKernelExecutionInfo>& ptr) {
    handle = ptr->handle;
    launchFuncArgs = ptr->launchFuncArgs;
    schedulingHintFlags = ptr->schedulingHintFlags;
    indirectUsmTypes = ptr->indirectUsmTypes;
    kernelNumber = ptr->kernelNumber;
    groupSizeX = ptr->groupSizeX;
    groupSizeY = ptr->groupSizeY;
    groupSizeZ = ptr->groupSizeZ;
    offsetX = ptr->offsetX;
    offsetY = ptr->offsetY;
    offsetZ = ptr->offsetZ;
    isGroupSizeSet = ptr->isGroupSizeSet;
    isOffsetSet = ptr->isOffsetSet;
    hModule = ptr->hModule;
    pKernelName = ptr->pKernelName;
    hSignalEvent = ptr->hSignalEvent;
    args = ptr->args;
    pointers = ptr->pointers;
  };
};

struct CKernelState : public CState {
  using type = ze_kernel_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CKernelState>>;
  ze_module_handle_t hModule = nullptr;
  ze_kernel_desc_t desc = {};

  std::unique_ptr<CKernelExecutionInfo> currentKernelInfo;

public:
  CKernelState() = default;
  ~CKernelState();
  CKernelState(ze_module_handle_t hModule, const ze_kernel_desc_t* kernelDesc);
};

class QueueSubmissionSnapshot {
public:
  std::vector<std::shared_ptr<CKernelExecutionInfo>> kernelsExecutionInfo;
  uint32_t cmdListNumber = 0U;
  uint32_t cmdQueueNumber = 0U;
  ze_context_handle_t hContext = nullptr;
  ze_command_list_handle_t hCommandList = nullptr;
  std::vector<CKernelArgumentDump>* readyArgVector = nullptr;
  QueueSubmissionSnapshot(const ze_command_list_handle_t& cmdListHandle,
                          const bool& isImmediate,
                          const std::vector<std::shared_ptr<CKernelExecutionInfo>>& appendedKernels,
                          const uint32_t& cmdListNum,
                          const ze_context_handle_t& cmdListContext,
                          const uint32_t& cmdQueueNum,
                          std::vector<CKernelArgumentDump>* argumentsVector);
};

struct CCommandListState : public CState {
  using type = ze_command_list_handle_t;
  using states_type = std::map<type, std::unique_ptr<CCommandListState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_command_list_desc_t listDesc = {};
  ze_command_queue_desc_t queueDesc = {};
  bool isImmediate = false;
  bool isSync = false;
  bool isClosed = false;
  std::vector<std::shared_ptr<CKernelExecutionInfo>> appendedKernels;
  std::vector<std::unique_ptr<QueueSubmissionSnapshot>> immediateCmdListSubmissionsDumpState;
  std::vector<std::unique_ptr<QueueSubmissionSnapshot>> immediateCmdListNotSyncedSubmissions;
  uint32_t cmdListNumber = 0U;
  uint32_t cmdQueueNumber = 0U;
  struct Action {
    enum Type {
      Normal,
      Reset,
      Signal
    };
    Type type = Type::Normal;
    ze_event_handle_t signalEvent = nullptr;
    std::vector<ze_event_handle_t> waitEvents;
  };
  std::vector<Action> mockList;
  std::set<const void*> ptrsToUpdate;

public:
  CCommandListState() = default;
  CCommandListState(ze_context_handle_t hContext,
                    ze_device_handle_t hDevice,
                    ze_command_list_desc_t desc);
  CCommandListState(ze_context_handle_t hContext,
                    ze_device_handle_t hDevice,
                    ze_command_queue_desc_t altdesc);
  void AddAction(const ze_event_handle_t& signalEvent,
                 const uint32_t& numWaitEvents,
                 ze_event_handle_t* phWaitEvents,
                 Action::Type type = Action::Type::Normal) {
    Action action = {};
    action.signalEvent = signalEvent;
    action.type = type;
    for (auto i = 0U; i < numWaitEvents; i++) {
      action.waitEvents.push_back(phWaitEvents[i]);
    }
    mockList.push_back(std::move(action));
  }
};

struct CImageState : public CState {
  using type = ze_image_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CImageState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_image_desc_t desc = {};
  ze_image_handle_t imageView = nullptr;
  bool savedForStateRestore = false;

  struct ResidencyInfo {
    ResidencyInfo(ze_context_handle_t hContext, ze_device_handle_t hDevice)
        : hContext(hContext), hDevice(hDevice) {}
    ze_context_handle_t hContext = nullptr;
    ze_device_handle_t hDevice = nullptr;
  };
  std::unique_ptr<ResidencyInfo> residencyInfo;

public:
  CImageState() = default;
  CImageState(ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_image_desc_t desc);
  ~CImageState();
};

struct CCommandQueueState : public CState {
  using type = ze_command_queue_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CCommandQueueState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_command_queue_desc_t desc = {};
  bool isSync = false;
  uint32_t cmdQueueNumber = 0U;
  std::vector<std::unique_ptr<QueueSubmissionSnapshot>> queueSubmissionDumpState;
  std::vector<std::unique_ptr<QueueSubmissionSnapshot>> notSyncedSubmissions;

public:
  CCommandQueueState() = default;
  CCommandQueueState(ze_context_handle_t hContext,
                     ze_device_handle_t hDevice,
                     ze_command_queue_desc_t desc);
};

struct CDriverState : public CState {
  using type = ze_driver_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CDriverState>>;

public:
  ze_init_driver_type_flags_t driverType;

  CDriverState() = default;
};

struct CDeviceState : public CState {
  using type = ze_device_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CDeviceState>>;
  ze_driver_handle_t hDriver = nullptr;
  ze_device_properties_t properties = {};
  ze_device_handle_t hDevice = nullptr;
  std::vector<ze_command_queue_group_properties_t> cqGroupProperties;
  std::vector<ze_command_queue_group_properties_t> originalQueueGroupProperties;
  std::unordered_map<uint32_t, uint32_t> multiContextEngineMap;

public:
  CDeviceState() = default;
  explicit CDeviceState(ze_driver_handle_t hDriver);
  explicit CDeviceState(ze_device_handle_t hDevice);
};

struct ProgramInfo {
  std::string moduleFileName;
  std::string buildOptions;
#ifdef WITH_OCLOC
  std::shared_ptr<ocloc::COclocState> oclocState = nullptr;
#endif

public:
#ifdef WITH_OCLOC
  ProgramInfo(const char* buildOptions,
              const std::string filename,
              std::shared_ptr<ocloc::COclocState>& oclocState);
#endif
  ProgramInfo(const char* buildOptions, const std::string filename);
};

struct CModuleState : public CState {
  using type = ze_module_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CModuleState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_module_desc_t desc = {};
  std::unordered_set<ze_module_handle_t> moduleLinks;
  ze_module_build_log_handle_t hBuildLog = nullptr;

  std::vector<ProgramInfo> programs;

private:
  ze_module_constants_t* CreateNewModuleConstants(const ze_module_constants_t* originalConstants);
  void DeleteModuleConstants(const ze_module_constants_t* originalConstants);

  void SetModuleDesc(const ze_module_desc_t* descriptor);
  void FillProgramInfo(std::vector<std::string> filenames);

public:
  CModuleState() = default;
  ~CModuleState();
  CModuleState(ze_context_handle_t hContext,
               ze_device_handle_t hDevice,
               const ze_module_desc_t* desc,
               ze_module_build_log_handle_t hBuildLog,
               std::vector<std::string> filenames);

  void AddModuleLinks(const uint32_t& numModules, const ze_module_handle_t* phModules);
  bool IsModuleLinkUsed() const;
};

struct CContextState : public CState {
  using type = ze_context_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CContextState>>;
  ze_driver_handle_t hDriver = nullptr;
  ze_context_desc_t desc = {};
  ze_command_list_handle_t gitsImmediateList = nullptr;
  std::vector<ze_device_handle_t> hDevices;
  void* virtualMemory = nullptr;
  size_t virtualMemorySize = 0U;

  ze_event_pool_handle_t gitsPoolEventHandle = nullptr;
  ze_event_handle_t gitsEventHandle = nullptr;

public:
  CContextState() = default;
  CContextState(ze_driver_handle_t hDriver, ze_context_desc_t desc);
  CContextState(ze_driver_handle_t hDriver,
                ze_context_desc_t desc,
                uint32_t numDevices,
                ze_device_handle_t* phDevices);
};

struct CEventPoolState : public CState {
  using type = ze_event_pool_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CEventPoolState>>;
  ze_context_handle_t hContext = nullptr;
  ze_event_pool_desc_t desc = {};

public:
  CEventPoolState() = default;
  CEventPoolState(ze_context_handle_t hContext, ze_event_pool_desc_t desc);
};

struct CEventState : public CState {
  using type = ze_event_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CEventState>>;
  ze_event_pool_handle_t hEventPool = nullptr;
  ze_event_desc_t desc = {};
  ze_command_list_handle_t immediateCmdListExecutingCmdLists = nullptr;
  bool canBeSynced = false;
  bool executionIsSynced = false;

public:
  CEventState() = default;
  CEventState(ze_event_pool_handle_t hEventPool, ze_event_desc_t desc);
};

struct CFenceState : public CState {
  using type = ze_fence_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CFenceState>>;
  ze_command_queue_handle_t hCommandQueue = nullptr;
  ze_fence_desc_t desc = {};
  bool canBeSynced = false;
  bool executionIsSynced = false;

public:
  CFenceState() = default;
  CFenceState(ze_command_queue_handle_t hCommandQueue, ze_fence_desc_t desc);
};

struct CSysDriverState : public CState {
  using type = zes_driver_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CSysDriverState>>;

public:
  ze_init_driver_type_flags_t driverType;

  CSysDriverState() = default;
};

struct CSysDeviceState : public CState {
  using type = zes_device_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CSysDeviceState>>;
  zes_driver_handle_t hDriver = nullptr;
  zes_device_properties_t properties = {};
  zes_device_handle_t hDevice = nullptr;

public:
  CSysDeviceState() = default;
  explicit CSysDeviceState(zes_driver_handle_t hDriver);
  explicit CSysDeviceState(zes_device_handle_t hDevice);
};

class LayoutBuilder {
private:
  nlohmann::ordered_json layout;
  nlohmann::ordered_json zeKernels;
  std::string latestFileName;
  uint32_t queueSubmitNumber = 0U;
  uint32_t cmdListNumber = 0U;
  uint32_t appendKernelNumber = 0U;
  std::string GetExecutionKeyId() const;
#ifdef WITH_OCLOC
  nlohmann::ordered_json GetOclocInfo(const std::shared_ptr<ocloc::COclocState>& oclocState);
#endif
  std::string BuildFileName(const uint64_t& argNumber,
                            bool isBuffer,
                            bool isIndirectMode,
                            bool isInputMode);
  nlohmann::ordered_json GetImageDescription(const ze_image_desc_t& imageDesc) const;
  void UpdateExecutionKeyId(const uint32_t& queueSubmitNum,
                            const uint32_t& cmdListNum,
                            const uint32_t& kernelNumber);

public:
  LayoutBuilder();
  void UpdateLayout(const CKernelExecutionInfo* kernelInfo,
                    const uint32_t& queueSubmitNum,
                    const uint32_t& cmdListNum,
                    const uint64_t& argIndex,
                    bool isInputMode,
                    bool isIndirectDump = false);
  bool Exists(const uint32_t& queueSubmitNumber,
              const uint32_t& cmdListNumber,
              const uint32_t& appendKernelNumber,
              const uint64_t& kernelArgIndex) const;
  std::string GetFileName() const;
  void SaveLayoutToJsonFile();
  nlohmann::ordered_json GetModuleLinkInfo(
      CStateDynamic& sd, const std::unordered_set<ze_module_handle_t>& moduleLinks) const;
  template <typename T, typename K>
  void Add(const T& key, const K& value) {
    zeKernels[GetExecutionKeyId()][key] = value;
  }
  template <typename T1, typename T2, typename K>
  void Add(const T1& key1, const T2& key2, const K& value) {
    zeKernels[GetExecutionKeyId()][key1][key2] = value;
  }
  template <typename T1, typename T2, typename T3, typename K>
  void Add(const T1& key1, const T2& key2, const T3& key3, const K& value) {
    zeKernels[GetExecutionKeyId()][key1][key2][key3] = value;
  }
};

struct GlobalSubmissionTracker {
  struct QueueSubmissionTracker {
    ze_command_list_handle_t hCommandList = nullptr;
    std::shared_ptr<CKernelExecutionInfo> executionInfo;
    QueueSubmissionTracker() = default;
    QueueSubmissionTracker(const ze_command_list_handle_t& cmdListHandle,
                           std::shared_ptr<CKernelExecutionInfo> kernelExecutionInfo)
        : hCommandList(cmdListHandle), executionInfo(std::move(kernelExecutionInfo)) {}
  };
  uint32_t submissionId = 0;
  std::map<uint32_t, std::unique_ptr<QueueSubmissionTracker>> queueSubmissionTracker;
  GlobalSubmissionTracker() = default;
  void Add(ze_command_list_handle_t hCommandList,
           std::shared_ptr<CKernelExecutionInfo> currentKernelInfo);
  void SyncCheck();
};

class DeallocationHandler {
  struct DeallocationInfo {
    CUSMPtr* usmPtrResource = nullptr;
    ze_event_handle_t hSignalEvent = nullptr;

    DeallocationInfo() = default;
    DeallocationInfo(CUSMPtr* usmPtr, ze_event_handle_t hSignalEvent)
        : usmPtrResource(usmPtr), hSignalEvent(hSignalEvent) {}

    void Deallocate();
  };

  std::unordered_map<ze_command_list_handle_t, std::vector<DeallocationInfo>>
      playbackResourcesInExecution;

public:
  void AddToResourcesInExecution(const ze_command_list_handle_t& hCommandList,
                                 CUSMPtr& ptr,
                                 const ze_event_handle_t& signalEvent);
  void DeallocateExecutedResources(const ze_command_list_handle_t& hCommandList);
  void DeallocateResourcesSynchedByEvent(const ze_event_handle_t& hEvent);
  void RemoveUseOfEvent(const ze_event_handle_t& hEvent);
};

class CStateDynamic {
private:
  CStateDynamic() = default;
  typename CAllocState::states_type allocStates_;
  typename CKernelState::states_type kernelStates_;
  typename CCommandListState::states_type commandListStates_;
  typename CImageState::states_type imageStates_;
  typename CCommandQueueState::states_type commandQueueStates_;
  typename CDriverState::states_type driverStates_;
  typename CDeviceState::states_type deviceStates_;
  typename CModuleState::states_type moduleStates_;
  typename CContextState::states_type contextStates_;
  typename CEventPoolState::states_type eventPoolStates_;
  typename CEventState::states_type eventStates_;
  typename CFenceState::states_type fenceStates_;
  typename CKernelArgumentDump::states_type kernelArgumentDumps_;
  typename CPhysicalMemState::states_type physicalMemStates_;
  typename CSysDriverState::states_type sysDriverStates_;
  typename CSysDeviceState::states_type sysDeviceStates_;
  static CStateDynamic* instance;

public:
  LayoutBuilder layoutBuilder;
  GlobalSubmissionTracker gst;
  DeallocationHandler deallocationHandler;
  std::unordered_set<ze_module_handle_t> scanningGlobalPointersMode;
  bool nomenclatureCounting = true;
  bool stateRestoreFinished = false;
  bool isProtectionWrapper = false;
  CStateDynamic(const CStateDynamic& other) = delete;
  CStateDynamic& operator=(const CStateDynamic& other) = delete;
  ~CStateDynamic();

  template <typename State>
  typename State::states_type& Map() {
    static typename State::states_type map;
    return map;
  }

  template <typename State>
  const typename State::states_type& Map() const {
    static typename State::states_type map;
    return map;
  }

  template <typename State>
  State& Get(const typename State::type key, CExceptionMessageInfo exception_message) {
    auto& map = Map<State>();
    auto iter = map.find(key);
    if (iter == map.end()) {
      throw std::runtime_error(exception_message);
    }
    return *(iter->second);
  }
  template <typename State>
  const State& Get(const typename State::type key, CExceptionMessageInfo exception_message) const {
    const auto& map = Map<State>();
    auto iter = map.find(key);
    if (iter == map.end()) {
      throw std::runtime_error(exception_message);
    }
    return *(iter->second);
  }

  template <typename State>
  bool Exists(const typename State::type key) const {
    const auto& map = Map<State>();
    return map.find(key) != map.end();
  }

  template <typename State>
  void Release(const typename State::type key) {
    auto& map = Map<State>();
    map.erase(key);
  }
  static CStateDynamic* InstancePtr();
  static CStateDynamic& Instance();
};
template <>
typename CDriverState::states_type& CStateDynamic::Map<CDriverState>();
template <>
const typename CDriverState::states_type& CStateDynamic::Map<CDriverState>() const;
template <>
typename CDeviceState::states_type& CStateDynamic::Map<CDeviceState>();
template <>
const typename CDeviceState::states_type& CStateDynamic::Map<CDeviceState>() const;
template <>
typename CKernelState::states_type& CStateDynamic::Map<CKernelState>();
template <>
const typename CKernelState::states_type& CStateDynamic::Map<CKernelState>() const;
template <>
typename CModuleState::states_type& CStateDynamic::Map<CModuleState>();
template <>
const typename CModuleState::states_type& CStateDynamic::Map<CModuleState>() const;
template <>
typename CAllocState::states_type& CStateDynamic::Map<CAllocState>();
template <>
const typename CAllocState::states_type& CStateDynamic::Map<CAllocState>() const;
template <>
typename CKernelArgumentDump::states_type& CStateDynamic::Map<CKernelArgumentDump>();
template <>
const typename CKernelArgumentDump::states_type& CStateDynamic::Map<CKernelArgumentDump>() const;
template <>
typename CEventPoolState::states_type& CStateDynamic::Map<CEventPoolState>();
template <>
const typename CEventPoolState::states_type& CStateDynamic::Map<CEventPoolState>() const;
template <>
typename CFenceState::states_type& CStateDynamic::Map<CFenceState>();
template <>
const typename CFenceState::states_type& CStateDynamic::Map<CFenceState>() const;
template <>
typename CEventState::states_type& CStateDynamic::Map<CEventState>();
template <>
const typename CEventState::states_type& CStateDynamic::Map<CEventState>() const;
template <>
typename CContextState::states_type& CStateDynamic::Map<CContextState>();
template <>
const typename CContextState::states_type& CStateDynamic::Map<CContextState>() const;
template <>
typename CImageState::states_type& CStateDynamic::Map<CImageState>();
template <>
const typename CImageState::states_type& CStateDynamic::Map<CImageState>() const;
template <>
typename CCommandListState::states_type& CStateDynamic::Map<CCommandListState>();
template <>
const typename CCommandListState::states_type& CStateDynamic::Map<CCommandListState>() const;
template <>
typename CCommandQueueState::states_type& CStateDynamic::Map<CCommandQueueState>();
template <>
typename CPhysicalMemState::states_type& CStateDynamic::Map<CPhysicalMemState>();
template <>
const typename CPhysicalMemState::states_type& CStateDynamic::Map<CPhysicalMemState>() const;
template <>
const typename CCommandQueueState::states_type& CStateDynamic::Map<CCommandQueueState>() const;
template <>
typename CSysDriverState::states_type& CStateDynamic::Map<CSysDriverState>();
template <>
const typename CSysDriverState::states_type& CStateDynamic::Map<CSysDriverState>() const;
template <>
typename CSysDeviceState::states_type& CStateDynamic::Map<CSysDeviceState>();
template <>
const typename CSysDeviceState::states_type& CStateDynamic::Map<CSysDeviceState>() const;

inline CStateDynamic& SD() {
  return CStateDynamic::Instance();
}
} // namespace l0
} // namespace gits
