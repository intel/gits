// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "exception.h"
#include "l0Header.h"
#include "l0Tools.h"
#include "MemorySniffer.h"
#ifdef WITH_OCLOC
#include "oclocStateDynamic.h"
#endif

#include <map>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace l0 {
struct CState {
  CState() = default;
  virtual ~CState() = default;

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
};

struct ArgInfo {
  KernelArgType type = KernelArgType::pointer;
  size_t argSize = 0U;
  size_t typeSize = 0U;
  const void* argValue = nullptr;
  const void* originalValue = nullptr;
  ze_image_desc_t desc = {};
  ArgInfo() = default;
};

struct CKernelExecutionInfo {
  ze_group_count_t launchFuncArgs = {};
  ze_scheduling_hint_exp_flags_t schedulingHintflags =
      static_cast<ze_scheduling_hint_exp_flags_t>(0U);
  unsigned indirectUsmTypes = 0U;
  uint32_t kernelNumber = 0U;
  uint32_t groupSizeX = 0U;
  uint32_t groupSizeY = 0U;
  uint32_t groupSizeZ = 0U;
  uint32_t offsetX = 0U;
  uint32_t offsetY = 0U;
  uint32_t offsetZ = 0U;

private:
  std::map<uint32_t, ArgInfo> args;
  std::vector<std::shared_ptr<intptr_t>> pointers;

public:
  const std::map<uint32_t, ArgInfo>& GetArguments() const;
  const ArgInfo& GetArgument(const uint32_t& index) const;
  void SetArgument(uint32_t index, size_t typeSize, const void* value);
  CKernelExecutionInfo() = default;
};

struct CKernelState : public CState {
  using type = ze_kernel_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CKernelState>>;
  ze_module_handle_t hModule = nullptr;
  ze_kernel_desc_t desc = {};
  bool isGroupSizeSet = false;
  bool isOffsetSet = false;

  CKernelExecutionInfo currentKernelInfo;
  std::unordered_map<uint32_t, CKernelExecutionInfo> executedKernels;

public:
  CKernelState() = default;
  CKernelState(ze_module_handle_t hModule, const ze_kernel_desc_t* kernelDesc);
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
  std::unordered_map<uint32_t, ze_kernel_handle_t> appendedKernelsMap;
  uint32_t cmdListNumber = 0U;
  uint32_t cmdQueueNumber = 0U;

public:
  CCommandListState() = default;
  CCommandListState(ze_context_handle_t hContext,
                    ze_device_handle_t hDevice,
                    ze_command_list_desc_t desc);
  CCommandListState(ze_context_handle_t hContext,
                    ze_device_handle_t hDevice,
                    ze_command_queue_desc_t altdesc);
};

struct CImageState : public CState {
  using type = ze_image_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CImageState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_image_desc_t desc = {};
  ze_image_handle_t imageView = nullptr;

public:
  CImageState() = default;
  CImageState(ze_context_handle_t hContext, ze_device_handle_t hDevice, ze_image_desc_t desc);
};

struct CCommandQueueState : public CState {
  using type = ze_command_queue_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CCommandQueueState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_command_queue_desc_t desc = {};
  bool isSync = false;
  uint32_t cmdQueueNumber = 0U;

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

struct CModuleState : public CState {
  using type = ze_module_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CModuleState>>;
  ze_context_handle_t hContext = nullptr;
  ze_device_handle_t hDevice = nullptr;
  ze_module_desc_t desc = {};
#ifdef WITH_OCLOC
  std::shared_ptr<ocloc::COclocState> oclocState = nullptr;
#endif
  ze_module_build_log_handle_t hBuildLog = nullptr;

public:
  CModuleState() = default;
  CModuleState(ze_context_handle_t hContext,
               ze_device_handle_t hDevice,
               ze_module_desc_t desc,
               ze_module_build_log_handle_t hBuildLog);
};

struct CContextState : public CState {
  using type = ze_context_handle_t;
  using states_type = std::unordered_map<type, std::unique_ptr<CContextState>>;
  ze_driver_handle_t hDriver = nullptr;
  ze_context_desc_t desc = {};
  ze_command_list_handle_t gitsImmediateList = nullptr;
  std::vector<ze_device_handle_t> hDevices;

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

public:
  CFenceState() = default;
  CFenceState(ze_command_queue_handle_t hCommandQueue, ze_fence_desc_t desc);
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
  uint32_t kernelArgIndex = 0U;

public:
  bool injected = false;
  CKernelArgumentDump() = default;
  CKernelArgumentDump(size_t allocSize, void* ptr, uint32_t kernelNumber, uint32_t kernelArgIndex);
  CKernelArgumentDump(ze_image_desc_t imageDesc,
                      size_t allocSize,
                      ze_image_handle_t ptr,
                      uint32_t kernelNumber,
                      uint32_t kernelArgIndex);
  void UpdateIndexes(uint32_t kernelNum, uint32_t argIndex);
};

class LayoutBuilder {
private:
  boost::property_tree::ptree layout;
  std::string latestFileName;
  uint32_t queueSubmitNumber = 0U;
  uint32_t cmdListNumber = 0U;
  uint32_t appendKernelNumber = 0U;
  std::string GetKeyName(const uint32_t& argNumber, const char* pKernelName);
  void AddBuildOptions(const char* pKernelName, const ze_module_handle_t& hModule);
  std::string BuildFileName(const uint32_t& argNumber, bool isBuffer = true);
  void AddBuffer(const uint32_t& argNumber, const char* pKernelName);
  void AddImage(const uint32_t& argNumber,
                const ze_image_desc_t& imageDesc,
                const char* pKernelName);

public:
  LayoutBuilder();
  void UpdateLayout(const char* pKernelName,
                    const ze_module_handle_t& hModule,
                    const CKernelExecutionInfo& kernelInfo,
                    const uint32_t& queueSubmitNum,
                    const uint32_t& cmdListNum,
                    const uint32_t& argIndex);
  std::string GetFileName();
  void SaveLayoutToJsonFile();
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
  typename CKernelArgument::states_type kernelArguments_;
  typename CKernelArgumentDump::states_type kernelArgumentDumps_;
  static CStateDynamic* instance;

public:
  LayoutBuilder layoutBuilder;
  std::unordered_set<ze_module_handle_t> scanningGlobalPointersMode;
  ~CStateDynamic();

  template <typename State>
  typename State::states_type& Map() {
    static typename State::states_type map;
    return map;
  }

  template <typename State>
  State& Get(const typename State::type key, CExceptionMessageInfo exception_message) {
    const auto& map = Map<State>();
    auto iter = map.find(key);
    if (iter == map.end()) {
      throw std::runtime_error(exception_message);
    }
    return *(iter->second);
  }

  template <typename State>
  bool Exists(const typename State::type key) {
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
typename CDeviceState::states_type& CStateDynamic::Map<CDeviceState>();
template <>
typename CKernelState::states_type& CStateDynamic::Map<CKernelState>();
template <>
typename CModuleState::states_type& CStateDynamic::Map<CModuleState>();
template <>
typename CAllocState::states_type& CStateDynamic::Map<CAllocState>();
template <>
typename CKernelArgumentDump::states_type& CStateDynamic::Map<CKernelArgumentDump>();
template <>
typename CKernelArgument::states_type& CStateDynamic::Map<CKernelArgument>();
template <>
typename CEventPoolState::states_type& CStateDynamic::Map<CEventPoolState>();
template <>
typename CFenceState::states_type& CStateDynamic::Map<CFenceState>();
template <>
typename CEventState::states_type& CStateDynamic::Map<CEventState>();
template <>
typename CContextState::states_type& CStateDynamic::Map<CContextState>();
template <>
typename CImageState::states_type& CStateDynamic::Map<CImageState>();
template <>
typename CCommandListState::states_type& CStateDynamic::Map<CCommandListState>();
template <>
typename CCommandQueueState::states_type& CStateDynamic::Map<CCommandQueueState>();
inline CStateDynamic& SD() {
  return CStateDynamic::Instance();
}
} // namespace l0
} // namespace gits
