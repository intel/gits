// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0Drivers.h"
#include "l0Header.h"
#include "config.h"
#include "texture_converter.h"

#include <memory>
#include <string>
#include <array>
#include <map>
#include <unordered_map>

namespace gits {
namespace l0 {
struct CKernelState;
struct CKernelArgumentDump;
class CStateDynamic;
void SaveBuffer(const bfs::path& dir, const std::string name, const std::vector<char>& data);
size_t BitsPerPixel(ze_image_format_layout_t imageFormat);
std::array<texel_type, 5> GetTexelTypeArrayFromLayout(ze_image_format_layout_t layout);
void SaveImage(const bfs::path& dir,
               const char* image,
               const ze_image_desc_t& desc,
               const std::string& name);
size_t CalculateImageSize(ze_image_desc_t desc);
enum class KernelArgType { pointer = 1, buffer, image };
std::string BuildFileName(KernelArgType type,
                          uint32_t queueSubmitNumber,
                          uint32_t cmdListNumber,
                          uint32_t kernelNumber,
                          uint32_t kernelArgNumber);
void PrepareArguments(const CKernelState& kernelState,
                      uint32_t kernelIndex,
                      std::vector<CKernelArgumentDump>& argDumpStates,
                      bool dumpUnique = false);
bool CheckWhetherSync(bool isImmediate,
                      bool isSync,
                      const ze_event_handle_t& eventSignal,
                      bool callOnce = false);
void DumpReadyArguments(std::vector<CKernelArgumentDump>& readyArgVector,
                        uint32_t cmdQueueNumber,
                        uint32_t cmdListNumber,
                        const Config& cfg,
                        CStateDynamic& sd);
const bfs::path& GetDumpPath(const Config& cfg);
bool CaptureKernels(const Config& cfg);
bool CaptureImages(const Config& cfg);
enum class UnifiedMemoryType : unsigned { host = 1 << 0, device = 1 << 1, shared = 1 << 2 };
bool CheckCfgZeroInitialization(const Config& cfg);
bool ZeroInitializeUsm(CDriver& driver,
                       const ze_command_list_handle_t& commandList,
                       void** pptr,
                       const size_t& size,
                       const UnifiedMemoryType& type);
bool ZeroInitializeImage(CDriver& driver,
                         const ze_command_list_handle_t& commandList,
                         const ze_image_handle_t* phImage,
                         const ze_image_desc_t* desc);
std::vector<ze_device_handle_t> GetDevices(const CDriver& drv, const ze_driver_handle_t& driver);
std::vector<ze_driver_handle_t> GetDrivers(const CDriver& drv);
ze_device_handle_t GetGPUDevice(CStateDynamic& sd, const CDriver& drv);
ze_command_list_handle_t GetCommandListImmediate(CStateDynamic& sd,
                                                 const CDriver& driver,
                                                 const ze_context_handle_t& context,
                                                 ze_result_t* err = nullptr);
bool IsCommandListImmediate(const ze_command_list_handle_t& handle, CStateDynamic& sd);
std::pair<void*, uintptr_t> GetAllocFromRegion(void* pAlloc, CStateDynamic& sd);
void* GetOffsetPointer(void* ptr, const uintptr_t& offset);
std::pair<void*, uintptr_t> GetAllocFromOriginalPtr(void* originalPtr, CStateDynamic& sd);
size_t GetSizeFromCopyRegion(const ze_copy_region_t* region);
bool IsNullIndirectPointersInBufferEnabled(const Config& cfg);
enum class AllocStateType : unsigned { pointer, global_pointer, function_pointer };
bool IsControlledSubmission(const ze_command_queue_desc_t* desc);
bool IsControlledSubmission(const ze_command_list_desc_t* desc);
bool ShouldDumpSpv(bool dumpSpv, const ze_module_desc_t* desc);
uint32_t GetMostCommonOrdinal(const ze_command_queue_group_property_flags_t& originalFlags,
                              const std::vector<ze_command_queue_group_properties_t>& currentProps,
                              const std::vector<uint32_t>& blockedOrdinals);

template <typename K, typename V>
std::vector<V> GetMapValues(const std::unordered_map<K, V>& map) {
  std::vector<V> vec;
  vec.reserve(map.size());
  for (const auto& i : map) {
    vec.push_back(i.second);
  }
  return vec;
}
bool IsPointerInsideAllocation(const void* pointer, const std::vector<char>& allocation);
void* GetPointerFromOriginalGlobalAllocation(const void* originalPtr,
                                             const std::vector<char>& originalAllocation,
                                             std::vector<char>& currentAllocation);
void* GetMappedGlobalPtrFromOriginalAllocation(CStateDynamic& sd, void* originalPtr);
} // namespace l0
} // namespace gits
