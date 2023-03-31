// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Tools.h"
#include "l0Arguments.h"

#include "gits.h"
#include "exception.h"
#include "l0Drivers.h"
#include "l0Header.h"
#include "l0Log.h"
#include "pragmas.h"
#include "recorder.h"
#include "l0StateDynamic.h"
#include "l0StateTracking.h"
#include <unordered_map>
#include <algorithm>

DISABLE_WARNINGS
#include <boost/filesystem.hpp>
ENABLE_WARNINGS

namespace gits {
namespace l0 {
namespace {
std::vector<uint32_t> FilterQueueFamily(
    const std::vector<ze_command_queue_group_properties_t>& currentProps,
    const ze_command_queue_group_property_flags_t filteredFlags,
    const std::vector<uint32_t>& blockedOrdinals) {
  std::vector<uint32_t> matchingOrdinals;
  for (auto i = 0U; i < currentProps.size(); i++) {
    const auto flagsMatching = (currentProps.at(i).flags & filteredFlags) != 0U;
    const auto isOrdinalBlocked =
        !blockedOrdinals.empty() &&
        std::find(blockedOrdinals.begin(), blockedOrdinals.end(), i) != blockedOrdinals.end();
    if (flagsMatching && !isOrdinalBlocked) {
      matchingOrdinals.push_back(i);
    }
  }

  return matchingOrdinals;
}
ze_command_queue_group_property_flags_t FilterFlag(
    const ze_command_queue_group_property_flags_t& flags,
    const ze_command_queue_group_property_flags_t& flagToFilter) {
  return static_cast<ze_command_queue_group_property_flags_t>(static_cast<uint32_t>(flags) &
                                                              ~static_cast<uint32_t>(flagToFilter));
}
} // namespace
size_t CalculateImageSize(ze_image_desc_t desc) {
  return desc.depth * static_cast<size_t>(desc.width) * desc.height *
         BitsPerPixel(desc.format.layout) / 8;
}

void SaveBuffer(const bfs::path& dir, const std::string name, const std::vector<char>& data) {
  std::string filename = name + ".dat";
  bfs::path path = dir / filename;
  bfs::create_directory(path.parent_path());
  bfs::ofstream binStream(path, bfs::ofstream::binary);
  binStream.write(data.data(), data.size());
  binStream.close();
}

size_t BitsPerPixel(ze_image_format_layout_t imageFormat) {
  size_t cBytes = 0;
  size_t bBytes = 0;
  switch (imageFormat) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
  case ZE_IMAGE_FORMAT_LAYOUT_Y8:
    cBytes = 1;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16:
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
  case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
  case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
  case ZE_IMAGE_FORMAT_LAYOUT_YUYV:
  case ZE_IMAGE_FORMAT_LAYOUT_VYUY:
  case ZE_IMAGE_FORMAT_LAYOUT_YVYU:
  case ZE_IMAGE_FORMAT_LAYOUT_UYVY:
    cBytes = 2;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32:
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
  case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
  case ZE_IMAGE_FORMAT_LAYOUT_AYUV:
    cBytes = 4;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
    cBytes = 8;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
    cBytes = 16;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_Y16:
    bBytes = 16;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_NV12:
    bBytes = 12;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_P010:
  case ZE_IMAGE_FORMAT_LAYOUT_P012:
  case ZE_IMAGE_FORMAT_LAYOUT_P016:
    bBytes = 24;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_Y410:
  case ZE_IMAGE_FORMAT_LAYOUT_Y216:
  case ZE_IMAGE_FORMAT_LAYOUT_P216:
    bBytes = 32;
    break;
  default:
    throw ENotSupported(EXCEPTION_MESSAGE);
  }
  return cBytes ? cBytes * 8 : bBytes;
}

std::array<texel_type, 5> GetTexelTypeArrayFromLayout(ze_image_format_layout_t layout) {
  const texel_type SURFACE_FORMAT_UNDEFINED = static_cast<texel_type>(-1);
  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
    return {texel_type::R8ui, texel_type::R8i, texel_type::R8, texel_type::R8snorm,
            SURFACE_FORMAT_UNDEFINED};
  case ZE_IMAGE_FORMAT_LAYOUT_16:
    return {texel_type::R16ui, texel_type::R16i, texel_type::R16, texel_type::R16snorm,
            texel_type::R16f};
  case ZE_IMAGE_FORMAT_LAYOUT_32:
    return {texel_type::R32ui, texel_type::R32i, SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED,
            texel_type::R32f};
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
    return {texel_type::RG8ui, texel_type::RG8i, texel_type::RG8, texel_type::RG8snorm,
            SURFACE_FORMAT_UNDEFINED};
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
    return {texel_type::RGBA8ui, texel_type::RGBA8i, texel_type::RGBA8, texel_type::RGBA8snorm,
            SURFACE_FORMAT_UNDEFINED};
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
    return {texel_type::RG16ui, texel_type::RG16i, texel_type::RG16, texel_type::RG16snorm,
            texel_type::RG16f};
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
    return {texel_type::RGBA16ui, texel_type::RGBA16i, texel_type::RGBA16, texel_type::RGBA16snorm,
            texel_type::RGBA16f};
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
    return {texel_type::RG32ui, texel_type::RG32i, SURFACE_FORMAT_UNDEFINED,
            SURFACE_FORMAT_UNDEFINED, texel_type::RG32f};
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
    return {texel_type::RGBA32ui, texel_type::RGBA32i, SURFACE_FORMAT_UNDEFINED,
            SURFACE_FORMAT_UNDEFINED, texel_type::RGBA32f};
  case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
    return {texel_type::RGB10A2ui, SURFACE_FORMAT_UNDEFINED, texel_type::RGB10A2,
            SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED};
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
    return {SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED,
            SURFACE_FORMAT_UNDEFINED, texel_type::RG11B10f};
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
    return {SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED,
            SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED};
  case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
    return {SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED,
            SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED};
  case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
    return {SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED,
            SURFACE_FORMAT_UNDEFINED, SURFACE_FORMAT_UNDEFINED};
  default:
    Log(WARN) << "The " << layout << " format cannot be converted to GITS internal format.";
    throw ENotSupported(EXCEPTION_MESSAGE);
  }
}

void SaveImage(const bfs::path& dir,
               const char* image,
               const ze_image_desc_t& desc,
               const std::string& name) {
  const unsigned rgba8TexelSize = 4;
  bfs::create_directory(dir);
  const size_t imagesMemorySize = CalculateImageSize(desc);
  try {
    auto texelType = GetTexelTypeArrayFromLayout(desc.format.layout)[desc.format.type];
    if (texelType == static_cast<texel_type>(-1)) {
      throw ENotSupported(EXCEPTION_MESSAGE);
    }
    const size_t imageSize = imagesMemorySize / desc.depth;
    for (unsigned i = 0; i < desc.depth; i++) {
      std::vector<uint8_t> frame(image + imageSize * i, image + imageSize * (i + 1));
      if (desc.format.type == ZE_IMAGE_FORMAT_TYPE_FLOAT) {
        normalize_texture_data(texelType, frame, static_cast<int>(desc.width),
                               static_cast<int>(desc.height));
      }
      std::vector<uint8_t> convertedData(static_cast<size_t>(desc.width) * desc.height *
                                         rgba8TexelSize);
      convert_texture_data(texelType, frame, texel_type::BGRA8, convertedData,
                           static_cast<int>(desc.width), static_cast<int>(desc.height));
      std::string fileName = (desc.depth == 1 ? name : name + "-" + std::to_string(i)) + ".png";
      bfs::path path = dir / fileName;
      CGits::Instance().WriteImage(path.string(), static_cast<size_t>(desc.width), desc.height,
                                   true, frame, false, true);
    }
  } catch (const ENotImplemented& ex) {
    Log(ERR) << ex.what();
  }
}

std::string BuildFileName(KernelArgType type,
                          uint32_t queueSubmitNumber,
                          uint32_t cmdListNumber,
                          uint32_t kernelNumber,
                          uint32_t kernelArgNumber) {
  static unsigned fileCounter = 1;
  std::stringstream fileName;
  fileName << "NDRange" << (type == KernelArgType::buffer ? "Buffer_" : "Image_")
           << queueSubmitNumber << "_" << cmdListNumber << "_" << kernelNumber << "_arg_"
           << kernelArgNumber << "_" << fileCounter++;
  return fileName.str();
}

void PrepareArguments(const CKernelState& kernelState,
                      uint32_t kernelIndex,
                      std::vector<CKernelArgumentDump>& argDumpStates,
                      bool dumpUnique) {
  const auto& kernelInfo = kernelState.executedKernels.at(kernelIndex);
  for (const auto& arg : kernelInfo.GetArguments()) {
    if (arg.second.type == KernelArgType::buffer) {
      auto ptr = const_cast<void*>(arg.second.argValue);
      if (dumpUnique) {
        auto it = std::find_if(argDumpStates.begin(), argDumpStates.end(),
                               [ptr](const CKernelArgumentDump& p) { return p.h_buf == ptr; });
        if (it != argDumpStates.end()) {
          it->UpdateIndexes(kernelInfo.kernelNumber, arg.first);
          continue;
        }
      }
      auto argDump = std::make_shared<CKernelArgumentDump>(arg.second.argSize, ptr,
                                                           kernelInfo.kernelNumber, arg.first);
      argDumpStates.push_back(*argDump);
    } else if (arg.second.type == KernelArgType::image) {
      auto ptr = reinterpret_cast<ze_image_handle_t>(const_cast<void*>(arg.second.argValue));
      if (dumpUnique) {
        auto it = std::find_if(argDumpStates.begin(), argDumpStates.end(),
                               [ptr](const CKernelArgumentDump& p) { return p.h_img == ptr; });
        if (it != argDumpStates.end()) {
          it->UpdateIndexes(kernelInfo.kernelNumber, arg.first);
          continue;
        }
      }
      auto argDump = std::make_shared<CKernelArgumentDump>(arg.second.desc,
                                                           CalculateImageSize(arg.second.desc), ptr,
                                                           kernelInfo.kernelNumber, arg.first);
      argDumpStates.push_back(*argDump);
    }
  }
}

bool CheckWhetherSync(bool isImmediate,
                      bool isSync,
                      const ze_event_handle_t& eventSignal,
                      bool callOnce) {
  if (isImmediate && callOnce) {
    if (!isSync) {
      if (eventSignal) {
        return true;
      } else {
        throw ENotSupported(
            "Dumping from async immediate command list without a signal event is not supported");
      }
    }
  }
  return false;
}

void DumpReadyArguments(std::vector<CKernelArgumentDump>& readyArgVector,
                        uint32_t cmdQueueNumber,
                        uint32_t cmdListNumber,
                        const Config& cfg,
                        CStateDynamic& sd) {
  const auto path = GetDumpPath(cfg);
  const auto captureImages = CaptureImages(cfg);
  const auto nullIndirectBuffers = IsNullIndirectPointersInBufferEnabled(cfg);
  for (auto& argState : readyArgVector) {
    std::string name = BuildFileName(argState.argType, cmdQueueNumber, cmdListNumber,
                                     argState.kernelNumber, argState.kernelArgIndex);
    if (nullIndirectBuffers && argState.argType == KernelArgType::buffer) {
      auto allocInfo = GetAllocFromRegion(argState.h_buf, sd);
      const auto& indirectList =
          sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE).indirectPointersOffsets;
      for (const auto& pair : indirectList) {
        if (pair.first >= allocInfo.second && pair.second) {
          auto it = std::next(argState.buffer.begin(), pair.first - allocInfo.second);
          auto itEnd =
              std::next(argState.buffer.begin(), pair.first - allocInfo.second + sizeof(void*));
          std::fill(it, itEnd, '\0');
        }
      }
    }
    SaveBuffer(path, name, argState.buffer);
    if (argState.argType == KernelArgType::image && captureImages) {
      SaveImage(path, argState.buffer.data(), argState.imageDesc, name);
    }
  }
  readyArgVector.clear();
}

const bfs::path& GetDumpPath(const Config& cfg) {
  static const bfs::path path =
      cfg.player.outputDir.empty() ? cfg.common.streamDir / "dump" : cfg.player.outputDir;
  return path;
}

bool CaptureKernels(const Config& cfg) {
  auto captureKernels = cfg.IsPlayer() ? cfg.player.l0CaptureKernels
                                       : cfg.recorder.levelZero.utilities.captureKernels;
  return !captureKernels.empty();
}

bool CaptureImages(const Config& cfg) {
  return cfg.IsPlayer() ? cfg.player.l0CaptureImages
                        : cfg.recorder.levelZero.utilities.captureImages;
}

bool CheckCfgZeroInitialization(const Config& cfg) {
  const auto zeroInit = cfg.IsPlayer() ? cfg.player.l0InjectBufferResetAfterCreate
                                       : cfg.recorder.levelZero.utilities.bufferResetAfterCreate;
  const auto dumpBuffers = cfg.IsPlayer()
                               ? !cfg.player.l0CaptureKernels.empty()
                               : !cfg.recorder.levelZero.utilities.captureKernels.empty();
  return zeroInit && dumpBuffers;
}

bool ZeroInitializeUsm(CDriver& driver,
                       const ze_command_list_handle_t& commandList,
                       void** pptr,
                       const size_t& size,
                       const UnifiedMemoryType& type) {
  if (pptr == nullptr) {
    return false;
  }
  auto* ptr = *pptr;
  if (ptr == nullptr) {
    return false;
  }
  const auto zeroBuffer = std::vector<char>(size, 0);
  if (type == UnifiedMemoryType::device) {
    driver.zeCommandListAppendMemoryCopy(commandList, ptr, zeroBuffer.data(), size, nullptr, 0,
                                         nullptr);
    Log(INFO) << "^------------------ injected zero-initialization write";
  } else {
    std::memcpy(ptr, zeroBuffer.data(), size);
  }
  return true;
}

bool ZeroInitializeImage(CDriver& driver,
                         const ze_command_list_handle_t& commandList,
                         const ze_image_handle_t* phImage,
                         const ze_image_desc_t* desc) {
  if (phImage == nullptr || desc == nullptr) {
    return false;
  }
  const auto size = CalculateImageSize(*desc);
  const auto zeroBuffer = std::vector<char>(size, 0);
  const ze_image_region_t region = {
      0, 0, 0, static_cast<uint32_t>(desc->width), desc->height, desc->depth};
  driver.zeCommandListAppendImageCopyFromMemory(commandList, *phImage, zeroBuffer.data(), &region,
                                                nullptr, 0, nullptr);
  Log(INFO) << "^------------------ injected zero-initialization write";
  return true;
}

std::vector<ze_driver_handle_t> GetDrivers(const CDriver& cDriver) {
  uint32_t drivCount = 0;
  cDriver.zeDriverGet(&drivCount, nullptr);
  std::vector<ze_driver_handle_t> drivers(drivCount);
  ze_result_t result = cDriver.zeDriverGet(&drivCount, drivers.data());
  zeDriverGet_SD(result, &drivCount, drivers.data());
  return result == ZE_RESULT_SUCCESS ? drivers : std::vector<ze_driver_handle_t>();
}

std::vector<ze_device_handle_t> GetDevices(const CDriver& cDriver,
                                           const ze_driver_handle_t& driver) {
  uint32_t devCount = 0;
  cDriver.zeDeviceGet(driver, &devCount, nullptr);
  std::vector<ze_device_handle_t> devices(devCount);
  ze_result_t result = cDriver.zeDeviceGet(driver, &devCount, devices.data());
  zeDeviceGet_SD(result, driver, &devCount, devices.data());
  return result == ZE_RESULT_SUCCESS ? devices : std::vector<ze_device_handle_t>();
}

ze_device_handle_t GetGPUDevice(CStateDynamic& sd, const CDriver& cDriver) {
  static ze_device_handle_t deviceHandle = nullptr;
  if (deviceHandle != nullptr) {
    return deviceHandle;
  }

  for (auto& device : sd.Map<CDeviceState>()) {
    if (device.second->properties.type == ZE_DEVICE_TYPE_GPU) {
      deviceHandle = device.first;
      return deviceHandle;
    }
  }

  for (auto& device : sd.Map<CDeviceState>()) {
    ze_device_properties_t deviceProperties = {};
    if (device.second->properties.deviceId == 0 &&
        drv.zeDeviceGetProperties(device.first, &deviceProperties) == ZE_RESULT_SUCCESS &&
        deviceProperties.type == ZE_DEVICE_TYPE_GPU) {
      sd.Get<CDeviceState>(device.first, EXCEPTION_MESSAGE).properties = deviceProperties;
      deviceHandle = device.first;
      return deviceHandle;
    }
  }

  std::vector<ze_driver_handle_t> drivers = GetDrivers(cDriver);
  for (auto& driver : drivers) {
    std::vector<ze_device_handle_t> devices = GetDevices(cDriver, driver);
    for (auto& device : devices) {
      ze_device_properties_t deviceProperties = {};
      if (drv.zeDeviceGetProperties(device, &deviceProperties) == ZE_RESULT_SUCCESS &&
          deviceProperties.type == ZE_DEVICE_TYPE_GPU) {
        sd.Get<CDeviceState>(device, EXCEPTION_MESSAGE).properties = deviceProperties;
        deviceHandle = device;
        return deviceHandle;
      }
    }
  }

  return nullptr;
}

ze_command_list_handle_t GetCommandListImmediate(CStateDynamic& sd,
                                                 const CDriver& driver,
                                                 const ze_context_handle_t& context,
                                                 ze_result_t* err) {
  auto& list = sd.Get<CContextState>(context, EXCEPTION_MESSAGE).gitsImmediateList;
  if (list != nullptr) {
    return list;
  }
  for (const auto& state : sd.Map<CCommandListState>()) {
    if (state.second->isImmediate) {
      return state.first;
    }
  }
  const auto device = GetGPUDevice(sd, driver);
  ze_command_list_handle_t handle;
  ze_command_queue_desc_t desc = {};
  desc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  const auto errCode = driver.zeCommandListCreateImmediate(context, device, &desc, &handle);
  if (err != nullptr) {
    *err = errCode;
  }
  if (errCode != ZE_RESULT_SUCCESS) {
    return nullptr;
  }
  list = handle;
  return handle;
}
bool IsCommandListImmediate(const ze_command_list_handle_t& handle, CStateDynamic& sd) {
  return sd.Get<CCommandListState>(handle, EXCEPTION_MESSAGE).isImmediate;
}
std::pair<void*, uintptr_t> GetAllocFromRegion(void* pAlloc, CStateDynamic& sd) {
  if (sd.Exists<CAllocState>(pAlloc)) {
    return std::make_pair(pAlloc, 0U);
  }
  for (const auto& state : sd.Map<CAllocState>()) {
    const auto ptrBegin = reinterpret_cast<uintptr_t>(state.first);
    const auto pAllocRegion = reinterpret_cast<uintptr_t>(pAlloc);
    if (ptrBegin < pAllocRegion &&
        pAllocRegion < ptrBegin + static_cast<uintptr_t>(state.second->size)) {
      const auto offset = pAllocRegion - ptrBegin;
      return std::make_pair(state.first, offset);
    }
  }
  return std::make_pair(nullptr, 0);
}
void* GetOffsetPointer(void* ptr, const uintptr_t& offset) {
  return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + offset);
}
std::pair<void*, uintptr_t> GetAllocFromOriginalPtr(void* originalPtr, CStateDynamic& sd) {
  if (CMappedPtr::CheckMapping(originalPtr)) {
    void* ptr = CMappedPtr::GetMapping(originalPtr);
    if (!sd.Exists<CAllocState>(ptr)) {
      return std::make_pair(nullptr, 0);
    }
    return std::make_pair(ptr, 0);
  }
  for (const auto& state : sd.Map<CAllocState>()) {
    void* usmOriginal = CMappedPtr::GetOriginal(state.first);
    const auto ptrBegin = reinterpret_cast<uintptr_t>(usmOriginal);
    const auto usmRegion = reinterpret_cast<uintptr_t>(originalPtr);
    if (ptrBegin < usmRegion && usmRegion < ptrBegin + static_cast<uintptr_t>(state.second->size)) {
      const auto offset = usmRegion - ptrBegin;
      return std::make_pair(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(state.first)),
                            offset);
    }
  }
  return std::make_pair(nullptr, 0);
}
size_t GetSizeFromCopyRegion(const ze_copy_region_t* region) {
  return region->depth != 0U ? region->width * region->height * region->depth
                             : region->width * region->height;
}
bool IsNullIndirectPointersInBufferEnabled(const Config& cfg) {
  return cfg.IsPlayer() ? !cfg.player.l0DisableNullIndirectPointersInBuffer
                        : cfg.recorder.levelZero.utilities.nullIndirectPointersInBuffer;
}
bool IsControlledSubmission(const ze_command_queue_desc_t* desc) {
  // Apps ignore spec recommendation about setting `flags` properly.
  return desc != nullptr && (desc->ordinal != 0U || desc->index != 0U);
}

bool IsControlledSubmission(const ze_command_list_desc_t* desc) {
  return desc != nullptr && desc->commandQueueGroupOrdinal != 0U;
}

uint32_t GetMostCommonOrdinal(const ze_command_queue_group_property_flags_t& originalFlags,
                              const std::vector<ze_command_queue_group_properties_t>& currentProps,
                              const std::vector<uint32_t>& blockedOrdinals) {
  auto filteringFlags = originalFlags;
  auto ret = FilterQueueFamily(currentProps, filteringFlags, blockedOrdinals);
  if (!ret.empty()) {
    return ret.front();
  }

  if ((originalFlags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS) != 0U) {
    filteringFlags = FilterFlag(filteringFlags, ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS);
    ret = FilterQueueFamily(currentProps, filteringFlags, blockedOrdinals);
    if (!ret.empty()) {
      return ret.front();
    }
  }

  if ((originalFlags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS) != 0U) {
    filteringFlags =
        FilterFlag(filteringFlags, ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
    ret = FilterQueueFamily(currentProps, filteringFlags, blockedOrdinals);
    if (!ret.empty()) {
      return ret.front();
    }
  }

  ret = FilterQueueFamily(currentProps, ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE,
                          blockedOrdinals);
  if (!ret.empty()) {
    return ret.front();
  }
  Log(WARN) << "Couldn't find proper engine. Choosing first..";
  for (const auto& props : currentProps) {
    Log(TRACEV) << ToStringHelper(props.flags);
  }
  return 0U;
}

bool IsPointerInsideAllocation(const void* pointer, const std::vector<char>& allocation) {
  std::vector<char> charPointer(sizeof(pointer), '\0');
  *(reinterpret_cast<uintptr_t*>(charPointer.data())) = reinterpret_cast<uintptr_t>(pointer);
  const auto end = allocation.end();
  const auto it = std::search(allocation.begin(), end, charPointer.begin(), charPointer.end());
  return it != end;
}

void* GetPointerFromOriginalGlobalAllocation(const void* originalPtr,
                                             const std::vector<char>& originalAllocation,
                                             std::vector<char>& currentAllocation) {
  std::vector<char> charPointer(sizeof(originalPtr), '\0');
  *(reinterpret_cast<uintptr_t*>(charPointer.data())) = reinterpret_cast<uintptr_t>(originalPtr);
  const auto begin = originalAllocation.begin();
  const auto end = originalAllocation.end();
  const auto it = std::search(begin, end, charPointer.begin(), charPointer.end());
  const auto offset = std::distance(begin, it);
  if (offset < 0) {
    return nullptr;
  } else if (offset >= 0 &&
             static_cast<uint32_t>(offset) + sizeof(originalPtr) <= currentAllocation.size()) {
    charPointer.assign(currentAllocation.data() + offset,
                       currentAllocation.data() + offset + sizeof(originalPtr));
    return reinterpret_cast<void*>(*(reinterpret_cast<uintptr_t*>(charPointer.data())));
  }
  return nullptr;
}

void* GetMappedGlobalPtrFromOriginalAllocation(CStateDynamic& sd, void* originalPtr) {
  for (const auto& allocState : sd.Map<CAllocState>()) {
    if (allocState.second->allocType == AllocStateType::global_pointer &&
        !allocState.second->originalGlobalPtrAllocation.empty()) {
      void* ptr = GetPointerFromOriginalGlobalAllocation(
          originalPtr, allocState.second->originalGlobalPtrAllocation,
          allocState.second->globalPtrAllocation);
      if (ptr != nullptr) {
        return ptr;
      }
    }
  }
  return nullptr;
}
} // namespace l0
} // namespace gits
