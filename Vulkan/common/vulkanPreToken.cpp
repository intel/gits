// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanPreToken.h"
#include "vulkanFunctions.h"
#include "vulkanStateTracking.h"
#include "vulkanCCodeWriteWrap.h"

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkCreateNativeWindow::ARG_NUM>
    gits::Vulkan::CGitsVkCreateNativeWindow::argumentInfos_ = {{
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // bool
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // HWND
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // HINSTANCE
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkCreateNativeWindow::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkCreateXlibWindow::ARG_NUM>
    gits::Vulkan::CGitsVkCreateXlibWindow::argumentInfos_ = {{
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // bool
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // HWND
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // HINSTANCE
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkCreateXlibWindow::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkUpdateNativeWindow::ARG_NUM>
    gits::Vulkan::CGitsVkUpdateNativeWindow::argumentInfos_ = {{
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // int
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // bool
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // HWND
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkUpdateNativeWindow::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkMemoryUpdate::ARG_NUM>
    gits::Vulkan::CGitsVkMemoryUpdate::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDevice
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDeviceMemory
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t
        {gits::Vulkan::ArgType::OTHER, 1, false},          // void* (CDeclaredBinaryResource)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkMemoryUpdate::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkMemoryUpdate::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, *_device, *_mem, *_offset, *_length, *_resource);
}

gits::Vulkan::CGitsVkMemoryUpdate::CGitsVkMemoryUpdate()
    : _device(std::make_unique<CVkDevice>()),
      _mem(std::make_unique<CVkDeviceMemory>()),
      _offset(std::make_unique<Cuint64_t>()),
      _length(std::make_unique<Cuint64_t>()),
      _resource(std::make_unique<CDeclaredBinaryResource>()) {}

void gits::Vulkan::CGitsVkMemoryUpdate::GetDiffSubRange(const std::vector<char>& oldData,
                                                        const std::vector<char>& newRangeData,
                                                        std::uint64_t& length,
                                                        std::uint64_t& offset) {
  const uint8_t* minOldPtr = (const uint8_t*)(&oldData[0] + offset); //+ offset
  const uint8_t* maxOldPtr = (const uint8_t*)(minOldPtr + length);
  const uint8_t* minNewPtr = (const uint8_t*)(&newRangeData[0]);
  const uint8_t* maxNewPtr = (const uint8_t*)(minNewPtr + length);

  while (minOldPtr < maxOldPtr && *minOldPtr == *minNewPtr) {
    minNewPtr++;
    minOldPtr++;
  }
  while (minOldPtr < maxOldPtr) {
    maxNewPtr--;
    maxOldPtr--;
    if (*maxOldPtr != *maxNewPtr) {
      maxNewPtr++;
      maxOldPtr++;
      break;
    }
  }

  offset = minNewPtr - (const uint8_t*)&newRangeData[0];
  length = maxOldPtr - minOldPtr;
}

gits::Vulkan::CGitsVkMemoryUpdate::CGitsVkMemoryUpdate(VkDevice device,
                                                       VkDeviceMemory mem,
                                                       bool unmap)
    : _device(std::make_unique<CVkDevice>(device)), _mem(std::make_unique<CVkDeviceMemory>(mem)) {

  auto& memoryState = SD()._devicememorystates[mem];
  auto& mapping = memoryState->mapping;

  if (device == memoryState->deviceStateStore->deviceHandle) {
    uint64_t unmapSize = mapping->size;
    char* pointer = mapping->pData;

    uint64_t offset = 0;

    if (Configurator::Get().vulkan.recorder.memoryAccessDetection) {
      std::pair<const void*, size_t> baseRange;
      baseRange.first = pointer;
      baseRange.second = unmapSize;
      auto subRange = GetSubrangeOverlappingMemoryPages(
          baseRange, (**mapping->sniffedRegionHandle).GetTouchedPagesAndReset());

      if (!unmap) {
        if (!MemorySniffer::Get().Protect(mapping->sniffedRegionHandle)) {
          Log(WARN) << "Protecting memory region: "
                    << (**mapping->sniffedRegionHandle).BeginAddress() << " - "
                    << (**mapping->sniffedRegionHandle).EndAddress() << " FAILED!.";
        }
      }

      offset = (uint64_t)subRange.first - (uint64_t)pointer;
      unmapSize = subRange.second;

      if (unmapSize == 0) {
        _offset = std::make_unique<Cuint64_t>(0);
        _length = std::make_unique<Cuint64_t>(0);
        _resource = std::make_unique<CDeclaredBinaryResource>();
        return;
      }
    }
    if (Configurator::Get().vulkan.recorder.memorySegmentSize) {
      std::vector<char> mappedMemCopy;
      mappedMemCopy.resize(unmapSize);
      char* pointerToData = pointer + offset;
      memcpy(mappedMemCopy.data(), pointerToData, unmapSize);
      uint64_t lengthNew = unmapSize;
      uint64_t offsetNew = offset;
      GetDiffSubRange(mapping->compareData, mappedMemCopy, lengthNew, offsetNew);
      _offset = std::make_unique<Cuint64_t>(offset + offsetNew);
      _length = std::make_unique<Cuint64_t>(lengthNew);

      if (lengthNew > 0) {
        memcpy(&mapping->compareData[**_offset], &mappedMemCopy[offsetNew], (size_t)lengthNew);
        _resource = std::make_unique<CDeclaredBinaryResource>(RESOURCE_DATA_RAW,
                                                              &mappedMemCopy[offsetNew], **_length);
      } else {
        _resource = std::make_unique<CDeclaredBinaryResource>();
      }

    } else {
      _offset = std::make_unique<Cuint64_t>(offset);
      _length = std::make_unique<Cuint64_t>(unmapSize);
      if (**_length != 0) {
        _resource = std::make_unique<CDeclaredBinaryResource>(RESOURCE_DATA_RAW,
                                                              pointer + **_offset, **_length);
      } else {
        _resource = std::make_unique<CDeclaredBinaryResource>();
      }
    }
    if (Configurator::Get().vulkan.recorder.shadowMemory) {
      memoryState->shadowMemory->Flush(**_offset, **_length);
    }
  } else {
    throw std::runtime_error("device from StateDynamic doesn't match device from vkMemoryUpdate");
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate::Run() {
  if (**_resource) {
    void* pointer = SD()._devicememorystates[**_mem]->mapping->pData;
    char* pointerToData = (char*)pointer + **_offset;
    std::memcpy(pointerToData, **_resource, (size_t) * *_length);
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate::Write(CBinOStream& stream) const {
  _device->Write(stream);
  _mem->Write(stream);
  _offset->Write(stream);
  _length->Write(stream);
  if (**_length != 0) {
    _resource->Write(stream);
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate::Read(CBinIStream& stream) {
  _device->Read(stream);
  _mem->Read(stream);
  _offset->Read(stream);
  _length->Read(stream);
  if (**_length != 0) {
    _resource->Read(stream);
  }
}

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkMemoryUpdate2::ARG_NUM>
    gits::Vulkan::CGitsVkMemoryUpdate2::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDeviceMemory
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 1, false}, // uint64_t* (array of uint64_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 1, false}, // uint64_t* (array of uint64_t)
        {gits::Vulkan::ArgType::OTHER, 2,
         false}, // void** (An array of pointers to data from CDeclaredBinaryResources)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkMemoryUpdate2::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkMemoryUpdate2::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, *_mem, *_size, _offset, _length, _resource);
}

gits::Vulkan::CGitsVkMemoryUpdate2::CGitsVkMemoryUpdate2()
    : _mem(std::make_unique<CVkDeviceMemory>()), _size(std::make_unique<Cuint64_t>()) {}

gits::Vulkan::CGitsVkMemoryUpdate2::CGitsVkMemoryUpdate2(VkDeviceMemory memory,
                                                         uint32_t regionCount,
                                                         const VkBufferCopy* pRegions)
    : _mem(std::make_unique<CVkDeviceMemory>(memory)),
      _size(std::make_unique<Cuint64_t>(regionCount)) {

  auto& memoryState = SD()._devicememorystates[memory];
  char* pointer = (char*)memoryState->mapping->pData;

  for (uint32_t i = 0; i < regionCount; ++i) {
    size_t offset = (size_t)pRegions[i].dstOffset;
    size_t length = (size_t)pRegions[i].size;
    char* pointerToData = pointer + offset;
    std::vector<char> mappedMemCopy;

    _offset.push_back(std::make_shared<Cuint64_t>(offset));
    _length.push_back(std::make_shared<Cuint64_t>(length));
    if (!isUseExternalMemoryExtensionUsed() && !Configurator::Get().vulkan.recorder.shadowMemory) {
      // Operations on non-shadow memory are slow, so we operate on a copy.
      mappedMemCopy.resize(length);
      memcpy(mappedMemCopy.data(), pointerToData, length);
      pointerToData = mappedMemCopy.data();
    }
    _resource.push_back(
        std::make_shared<CDeclaredBinaryResource>(RESOURCE_DATA_RAW, pointerToData, length));

    if (Configurator::Get().vulkan.recorder.shadowMemory) {
      size_t offset_flush = offset;
      if (CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
        offset_flush += memoryState->mapping->offset;
      }
      memoryState->shadowMemory->Flush(offset_flush, length);
    }
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate2::Run() {
  if (**_size > 0) {
    std::vector<VkBufferCopy> updatedRegions;
    std::vector<VkMappedMemoryRange> regionsToFlush;
    updatedRegions.reserve(**_size);
    regionsToFlush.reserve(**_size);

    VkDeviceMemory memory = **_mem;
    auto& memoryState = SD()._devicememorystates[memory];
    VkDevice device = memoryState->deviceStateStore->deviceHandle;

    for (unsigned int i = 0; i < **_size; ++i) {
      const auto resourcePtr = **_resource[i];
      if (resourcePtr) {
        updatedRegions.push_back({
            **_offset[i], // VkDeviceSize srcOffset;
            **_offset[i], // VkDeviceSize dstOffset;
            **_length[i]  // VkDeviceSize size;
        });
        regionsToFlush.push_back({
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType sType;
            nullptr,                               // const void* pNext;
            memory,                                // VkDeviceMemory memory;
            **_offset[i],                          // VkDeviceSize offset;
            **_length[i]                           // VkDeviceSize size;
        });

        char* pointer = memoryState->mapping->pData;
        std::memcpy((char*)pointer + updatedRegions[i].dstOffset, resourcePtr,
                    (size_t)updatedRegions[i].size);
      }
    }

    if (updatedRegions.size() > 0) {
      drvVk.vkFlushMappedMemoryRanges(device, regionsToFlush.size(), regionsToFlush.data());
      drvVk.vkTagMemoryContentsUpdateGITS(device, memory, updatedRegions.size(),
                                          updatedRegions.data());
    }
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate2::Write(CBinOStream& stream) const {
  _mem->Write(stream);
  _size->Write(stream);
  for (auto& off : _offset) {
    off->Write(stream);
  }
  for (auto& len : _length) {
    len->Write(stream);
  }
  for (auto& res : _resource) {
    res->Write(stream);
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate2::Read(CBinIStream& stream) {
  _mem->Read(stream);
  _size->Read(stream);
  for (uint64_t i = 0; i < **_size; i++) {
    auto offsetPtr(std::make_shared<Cuint64_t>());
    offsetPtr->Read(stream);
    _offset.push_back(std::move(offsetPtr));
  }
  for (uint64_t i = 0; i < **_size; i++) {
    auto lengthPtr(std::make_shared<Cuint64_t>());
    lengthPtr->Read(stream);
    _length.push_back(std::move(lengthPtr));
  }

  for (uint64_t i = 0; i < **_size; i++) {
    auto binaryPtr(std::make_shared<CDeclaredBinaryResource>());
    binaryPtr->Read(stream);
    _resource.push_back(std::move(std::move(binaryPtr)));
  }
}

void gits::Vulkan::CGitsVkMemoryUpdate2::Write(CCodeOStream& stream) const {
  if (**_size == 0) {
    return; // To avoid littering CCode with empty updates.
  }

  // Captured function calls go to state restore file if is present or to frames file.
  // When using execCmdBuffsBeforeQueueSubmit while dumping CCode this token is sometimes first, so switching to proper file is needed.
  stream.select(stream.selectCCodeFile());
  gits::Vulkan::CVectorPrintHelper<Cuint64_t> offsets(_offset);
  gits::Vulkan::CVectorPrintHelper<Cuint64_t> lengths(_length);
  gits::Vulkan::CVectorPrintHelper<CDeclaredBinaryResource> resources(_resource);

  stream.Indent() << "{\n";
  stream.ScopeBegin();

  offsets.Declare(stream);
  lengths.Declare(stream);
  resources.Declare(stream);

  stream.Indent() << "CGitsVkMemoryUpdate2(" << *_mem << ", " << *_size << ", " << offsets << ", "
                  << lengths << ", " << resources << ");\n";

  stream.ScopeEnd();
  stream.Indent() << "}\n";
}

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkMemoryRestore::ARG_NUM>
    gits::Vulkan::CGitsVkMemoryRestore::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDevice
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDeviceMemory
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t
        {gits::Vulkan::ArgType::OTHER, 1, false},          // void* (CDeclaredBinaryResource)
    }};

void gits::Vulkan::CGitsVkMemoryRestore::GetDiffFromZero(const std::vector<char>& oldData,
                                                         std::uint64_t& length,
                                                         std::uint64_t& offset) {
  const uint8_t* minOldPtr = (const uint8_t*)(&oldData[0]);
  const uint8_t* maxOldPtr = (const uint8_t*)(minOldPtr + length);

  while (minOldPtr < maxOldPtr && *minOldPtr == 0) {
    minOldPtr++;
  }
  while (minOldPtr < maxOldPtr) {
    maxOldPtr--;
    if (*maxOldPtr != 0) {
      maxOldPtr++;
      break;
    }
  }

  offset = minOldPtr - (const uint8_t*)&oldData[0];
  length = maxOldPtr - minOldPtr;
}

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkMemoryRestore::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkMemoryRestore::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, *_device, *_mem, *_length, *_offset, *_resource);
}

gits::Vulkan::CGitsVkMemoryRestore::CGitsVkMemoryRestore()
    : _device(std::make_unique<CVkDevice>()),
      _mem(std::make_unique<CVkDeviceMemory>()),
      _length(std::make_unique<Cuint64_t>()),
      _offset(std::make_unique<Cuint64_t>()),
      _resource(std::make_unique<CDeclaredBinaryResource>()) {}

gits::Vulkan::CGitsVkMemoryRestore::CGitsVkMemoryRestore(VkDevice device,
                                                         VkDeviceMemory mem,
                                                         std::uint64_t size)
    : _device(std::make_unique<CVkDevice>(device)), _mem(std::make_unique<CVkDeviceMemory>(mem)) {
  assert((size > 0) && "Restoring empty memory!\n");

  auto& memoryState = SD()._devicememorystates[mem];
  auto& mapping = memoryState->mapping;
  void* pointer = nullptr;

  if (memoryState->IsMapped()) {
    pointer = mapping->pData - mapping->offset;
  } else {
    VkResult mapMemoryResult = drvVk.vkMapMemory(device, mem, 0, size, 0, &pointer);
    if (mapMemoryResult != VK_SUCCESS) {
      pointer = nullptr;
      Log(ERR) << "vkMapMemory returned error: " << mapMemoryResult
               << ". It can cause a corruption in a subcaptured stream.";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }

  if (Configurator::Get().vulkan.recorder.memoryAccessDetection && memoryState->IsMapped()) {
    auto& dereferencedRegionHandle = **mapping->sniffedRegionHandle;
    dereferencedRegionHandle.Reset();
    if (!MemorySniffer::Get().Protect(mapping->sniffedRegionHandle)) {
      Log(WARN) << "Protecting memory region: " << dereferencedRegionHandle.BeginAddress() << " - "
                << dereferencedRegionHandle.EndAddress() << " FAILED!.";
    }
  }

  std::vector<char> rangeCopy(size);
  memcpy(rangeCopy.data(), (char*)pointer, (size_t)size);
  uint64_t offsetNew = 0;
  uint64_t lengthNew = size;
  GetDiffFromZero(rangeCopy, lengthNew, offsetNew);
  _offset = std::make_unique<Cuint64_t>(offsetNew);
  _length = std::make_unique<Cuint64_t>(lengthNew);
  if (lengthNew > 0) {
    if (Configurator::Get().vulkan.recorder.memorySegmentSize && memoryState->IsMapped()) {
      memcpy(mapping->compareData.data(), mapping->pData, mapping->size);
    }
    _resource = std::make_unique<CDeclaredBinaryResource>(RESOURCE_DATA_RAW, &rangeCopy[offsetNew],
                                                          lengthNew);
  } else {
    _resource = std::make_unique<CDeclaredBinaryResource>();
  }

  if (Configurator::Get().vulkan.recorder.shadowMemory && memoryState->IsMapped()) {
    memoryState->shadowMemory->Flush(**_offset, **_length);
  }

  if (!memoryState->IsMapped()) {
    drvVk.vkUnmapMemory(device, mem);
  }
}

gits::Vulkan::CGitsVkMemoryRestore::CGitsVkMemoryRestore(VkDevice device,
                                                         VkDeviceMemory mem,
                                                         std::uint64_t size,
                                                         const void* mappedPtr)
    : _device(std::make_unique<CVkDevice>(device)), _mem(std::make_unique<CVkDeviceMemory>(mem)) {
  assert((mappedPtr != nullptr) && "Trying to copy memory contents from nullptr!\n");

  std::vector<char> rangeCopy(size);
  memcpy(rangeCopy.data(), mappedPtr, size);
  uint64_t offsetNew = 0;
  uint64_t lengthNew = size;
  GetDiffFromZero(rangeCopy, lengthNew, offsetNew);
  _offset = std::make_unique<Cuint64_t>(offsetNew);
  _length = std::make_unique<Cuint64_t>(lengthNew);
  if (lengthNew > 0) {
    _resource = std::make_unique<CDeclaredBinaryResource>(RESOURCE_DATA_RAW, &rangeCopy[offsetNew],
                                                          lengthNew);
  } else {
    _resource = std::make_unique<CDeclaredBinaryResource>();
  }
}

void gits::Vulkan::CGitsVkMemoryRestore::Run() {
  if (**_resource) {
    VkDevice device = **_device;
    VkDeviceMemory memory = **_mem;

    auto& memoryState = SD()._devicememorystates[memory];
    if (memoryState && memoryState->IsMapped()) {
      auto& mapping = memoryState->mapping;
      VkBufferCopy updatedRegion = {
          **_offset, // VkDeviceSize srcOffset
          **_offset, // VkDeviceSize dstOffset
          **_length  // VkDeviceSize size
      };
      std::memcpy(mapping->pData + updatedRegion.dstOffset, **_resource, updatedRegion.size);

      VkMappedMemoryRange range = {
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,     // VkStructureType   sType
          nullptr,                                   // const void      * pNext
          memory,                                    // VkDeviceMemory    memory
          mapping->offset + updatedRegion.dstOffset, // VkDeviceSize      offset
          updatedRegion.size                         // VkDeviceSize      size
      };
      drvVk.vkFlushMappedMemoryRanges(device, 1, &range);
      drvVk.vkTagMemoryContentsUpdateGITS(device, memory, 1, &updatedRegion);
    } else {
      checkMemoryMappingFeasibility(device, memory);

      void* pointer = nullptr;
      drvVk.vkMapMemory(device, memory, **_offset, **_length, 0, &pointer);

      VkBufferCopy updatedRegion = {
          0,        // VkDeviceSize srcOffset
          0,        // VkDeviceSize dstOffset
          **_length // VkDeviceSize size
      };
      std::memcpy(pointer, **_resource, (size_t)updatedRegion.size);

      VkMappedMemoryRange range = {
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType  sType
          nullptr,                               // const void     * pNext
          memory,                                // VkDeviceMemory   memory
          **_offset,                             // VkDeviceSize     offset
          updatedRegion.size                     // VkDeviceSize     size
      };
      drvVk.vkFlushMappedMemoryRanges(device, 1, &range);
      drvVk.vkTagMemoryContentsUpdateGITS(device, memory, 1, &updatedRegion);

      drvVk.vkUnmapMemory(device, memory);
    }
  }
}

void gits::Vulkan::CGitsVkMemoryRestore::Write(CBinOStream& stream) const {
  _device->Write(stream);
  _mem->Write(stream);
  _length->Write(stream);
  _offset->Write(stream);
  if (**_length != 0) {
    _resource->Write(stream);
  }
}

void gits::Vulkan::CGitsVkMemoryRestore::Read(CBinIStream& stream) {
  _device->Read(stream);
  _mem->Read(stream);
  _length->Read(stream);
  _offset->Read(stream);
  if (**_length != 0) {
    _resource->Read(stream);
  }
}

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkMemoryReset::ARG_NUM>
    gits::Vulkan::CGitsVkMemoryReset::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDevice
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkDeviceMemory
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkMemoryReset::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkMemoryReset::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, *_device, *_mem, *_length);
}

gits::Vulkan::CGitsVkMemoryReset::CGitsVkMemoryReset()
    : _device(std::make_unique<CVkDevice>()),
      _mem(std::make_unique<CVkDeviceMemory>()),
      _length(std::make_unique<Cuint64_t>()) {}

gits::Vulkan::CGitsVkMemoryReset::CGitsVkMemoryReset(VkDevice device,
                                                     VkDeviceMemory memory,
                                                     std::uint64_t size,
                                                     void* mappedPtr)
    : _device(std::make_unique<CVkDevice>(device)),
      _mem(std::make_unique<CVkDeviceMemory>(memory)),
      _length(std::make_unique<Cuint64_t>(size)) {
  VkBufferCopy updatedRegion = {
      0,   // VkDeviceSize srcOffset;
      0,   // VkDeviceSize dstOffset;
      size // VkDeviceSize size;
  };

  memset(mappedPtr, 0, (size_t)updatedRegion.size);
  drvVk.vkTagMemoryContentsUpdateGITS(device, memory, 1, &updatedRegion);
}

void gits::Vulkan::CGitsVkMemoryReset::Run() {
  VkDevice device = **_device;
  VkDeviceMemory memory = **_mem;

  VkBufferCopy updatedRegion = {
      0,        // VkDeviceSize srcOffset;
      0,        // VkDeviceSize dstOffset;
      **_length // VkDeviceSize size;
  };

  auto& memoryState = SD()._devicememorystates[memory];
  if (memoryState && memoryState->IsMapped()) {
    memset(memoryState->mapping->pData, 0, updatedRegion.size);
    drvVk.vkTagMemoryContentsUpdateGITS(device, memory, 1, &updatedRegion);
  } else {
    checkMemoryMappingFeasibility(device, memory);

    void* pointer = nullptr;
    drvVk.vkMapMemory(device, memory, updatedRegion.dstOffset, updatedRegion.size, 0, &pointer);

    memset(pointer, 0, updatedRegion.size);
    drvVk.vkTagMemoryContentsUpdateGITS(device, memory, 1, &updatedRegion);

    drvVk.vkUnmapMemory(device, memory);
  }
}

void gits::Vulkan::CGitsVkMemoryReset::Write(CBinOStream& stream) const {
  _device->Write(stream);
  _mem->Write(stream);
  _length->Write(stream);
}

void gits::Vulkan::CGitsVkMemoryReset::Read(CBinIStream& stream) {
  _device->Read(stream);
  _mem->Read(stream);
  _length->Read(stream);
}

void gits::Vulkan::CGitsVkMemoryReset::Write(CCodeOStream& stream) const {
  if (**_length == 0) {
    // Avoid littering CCode with empty updates.
    return;
  }

  stream.Indent() << "{\n";
  stream.ScopeBegin();

  stream.Indent() << "CGitsVkMemoryReset(" << *_device << ", " << *_mem << ", " << *_length
                  << ");\n";

  stream.ScopeEnd();
  stream.Indent() << "}\n";
}

// For building top-level acceleration structures, application needs to pass data about all the bottom-level
// acceleration structures (BLASs in short) which will be built in the top level one. BLASs are referenced
// via their device addresses. The list of BLASs/device addresses is stored in a buffer which is also provided
// via a device address. Unfortunately, device addresses may change from run to run. Each time stream is
// being replayed, driver may assign different device addresses. That's why, during replay, we need to update
// those addresses with values returned by a driver.
//
// We can do this by capturing and storing additional metadata during stream recording - a list of all
// acceleration structure handles built into the TLASes (which are retrieved from device addresses).
// This way, during replay, we can take this list and convert handles to proper device addresses. Then,
// these device addresses are place in the original location provided to build command. This is done with
// a compute shader just before the build command.

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::ARG_NUM>
    gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::argumentInfos_ = {{
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint64_t (_count)
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkCommandBuffer (_commandBuffer)
        {gits::Vulkan::ArgType::OTHER, 1, false},          // void* (CDeclaredBinaryResource)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::ArgumentInfo(
    unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, *_count, *_commandBuffer, *_resource);
}

gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::CGitsVkCmdPatchDeviceAddresses()
    : _count(std::make_unique<Cuint32_t>()),
      _commandBuffer(std::make_unique<CVkCommandBuffer>()),
      _resource(std::make_unique<CDeclaredBinaryResource>()) {}

gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::CGitsVkCmdPatchDeviceAddresses(
    VkCommandBuffer commandBuffer, CDeviceAddressPatcher& patcher, uint64_t commandId)
    : _count(std::make_unique<Cuint32_t>(patcher.Count())),
      _commandBuffer(std::make_unique<CVkCommandBuffer>(commandBuffer)) {
  // Any data uniquely identifying this very acceleration structure build command.
  // It is used only to generate a hash key.
  // This key is used later in a post-vkQueueSubmit() operation to store data.

  struct {
    uint64_t buildCommandID;
    VkCommandBuffer commandBuffer;
    TId tokenID;
  } hashGenerator = {commandId, commandBuffer, ID_GITS_VK_CMD_PATCH_DEVICE_ADDRESSES};

  hash_t hash = CGits::Instance().ResourceManager2().getHash(RESOURCE_DATA_RAW, &hashGenerator,
                                                             sizeof(hashGenerator));

  _resource = std::make_unique<CDeclaredBinaryResource>(hash);
  patcher.PrepareData(commandBuffer, hash);
}

// Elements in some memory locations may appear in a random order.
// That's why patching is not done by placing a new values at same
// place as it was located during recording. Patching is performed
// using a simple map - compute shader reads an old value in a given
// memory location, finds it in a map, takes a new value from the
// map and stores it in the same memory location.
void gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::Run() {
  auto count = **_count;
  if (count == 0) {
    return;
  }

  struct DeviceAddressPatchGITS {
    VkDeviceAddress originalValue;
    VkDeviceAddress newValue;
  };

  std::vector<VkDeviceAddress> locations;
  std::vector<DeviceAddressPatchGITS> patchesMap;
  std::vector<VkBufferMemoryBarrier> preMemoryBarriers;
  std::vector<VkBufferMemoryBarrier> postMemoryBarriers;

  locations.resize(count);
  [&](const void* source) {

#define ALL_VULKAN_BUFFER_ACCESS_BITS VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT
    auto* data = (const VkBufferDeviceAddressPatchGITS*)source;
    std::unordered_map<VkDeviceAddress, VkDeviceAddress> tmpMap;

    auto getAddress = [](const VkBufferDeviceAddressGITS& patch,
                         std::vector<VkBufferMemoryBarrier>* preBarriers = nullptr,
                         std::vector<VkBufferMemoryBarrier>* postBarriers = nullptr) {
      VkBuffer buffer = CVkBuffer::GetMapping(patch.buffer);

      if (preBarriers) {
        preBarriers->push_back({
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType   sType;
            nullptr,                                 // const void      * pNext;
            ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags     srcAccessMask;
            VK_ACCESS_SHADER_WRITE_BIT,              // VkAccessFlags     dstAccessMask;
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t          srcQueueFamilyIndex;
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t          dstQueueFamilyIndex;
            buffer,                                  // VkBuffer          buffer;
            0,                                       // VkDeviceSize      offset;
            VK_WHOLE_SIZE                            // VkDeviceSize      size;
        });
      }

      if (postBarriers) {
        postBarriers->push_back({
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType   sType;
            nullptr,                                 // const void      * pNext;
            VK_ACCESS_SHADER_WRITE_BIT,              // VkAccessFlags     srcAccessMask;
            ALL_VULKAN_BUFFER_ACCESS_BITS,           // VkAccessFlags     dstAccessMask;
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t          srcQueueFamilyIndex;
            VK_QUEUE_FAMILY_IGNORED,                 // uint32_t          dstQueueFamilyIndex;
            buffer,                                  // VkBuffer          buffer;
            0,                                       // VkDeviceSize      offset;
            VK_WHOLE_SIZE                            // VkDeviceSize      size;
        });
      }

      return SD()._bufferstates[buffer]->deviceAddress + patch.offset;
    };

    // Prepare a patches map and memory locations to update
    for (uint32_t i = 0; i < count; ++i) {
      auto location = getAddress(data[i].location, &preMemoryBarriers, &postMemoryBarriers);
      auto originalValue = data[i].patchedValue.originalDeviceAddress;
      auto patchedValue = getAddress(data[i].patchedValue);

      tmpMap[originalValue] = patchedValue;
      locations[i] = location;
    }

    // Convert map to a vector for use in the compute shader
    patchesMap.reserve(tmpMap.size());
    for (auto& element : tmpMap) {
      patchesMap.push_back({element.first, element.second});
    }
  }(**_resource);

  VkCommandBuffer commandBuffer = **_commandBuffer;
  auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
  VkDevice device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;

  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  VkDeviceAddress locationsDataAddress = 0;
  {
    uint64_t locationsDataSize = locations.size() * sizeof(locations[0]);
    auto locationsMemoryBufferPair =
        createTemporaryBuffer(device, locationsDataSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    mapMemoryAndCopyData(device, locationsMemoryBufferPair.first->deviceMemoryHandle, 0,
                         locations.data(), locationsDataSize);
    locationsDataAddress =
        getBufferDeviceAddress(device, locationsMemoryBufferPair.second->bufferHandle);

    preMemoryBarriers.push_back({
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,        // VkStructureType   sType;
        nullptr,                                        // const void      * pNext;
        VK_ACCESS_HOST_WRITE_BIT,                       // VkAccessFlags     srcAccessMask;
        VK_ACCESS_SHADER_READ_BIT,                      // VkAccessFlags     dstAccessMask;
        VK_QUEUE_FAMILY_IGNORED,                        // uint32_t          srcQueueFamilyIndex;
        VK_QUEUE_FAMILY_IGNORED,                        // uint32_t          dstQueueFamilyIndex;
        locationsMemoryBufferPair.second->bufferHandle, // VkBuffer          buffer;
        0,                                              // VkDeviceSize      offset;
        VK_WHOLE_SIZE                                   // VkDeviceSize      size;
    });
  }

  VkDeviceAddress patchesMapDataAddress = 0;
  {
    uint64_t patchesMapDataSize = patchesMap.size() * sizeof(patchesMap[0]);
    auto patchesMapMemoryBufferPair =
        createTemporaryBuffer(device, patchesMapDataSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    mapMemoryAndCopyData(device, patchesMapMemoryBufferPair.first->deviceMemoryHandle, 0,
                         patchesMap.data(), patchesMapDataSize);
    patchesMapDataAddress =
        getBufferDeviceAddress(device, patchesMapMemoryBufferPair.second->bufferHandle);

    preMemoryBarriers.push_back({
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,         // VkStructureType   sType;
        nullptr,                                         // const void      * pNext;
        VK_ACCESS_HOST_WRITE_BIT,                        // VkAccessFlags     srcAccessMask;
        VK_ACCESS_SHADER_READ_BIT,                       // VkAccessFlags     dstAccessMask;
        VK_QUEUE_FAMILY_IGNORED,                         // uint32_t          srcQueueFamilyIndex;
        VK_QUEUE_FAMILY_IGNORED,                         // uint32_t          dstQueueFamilyIndex;
        patchesMapMemoryBufferPair.second->bufferHandle, // VkBuffer          buffer;
        0,                                               // VkDeviceSize      offset;
        VK_WHOLE_SIZE                                    // VkDeviceSize      size;
    });
  }

  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                             static_cast<uint32_t>(preMemoryBarriers.size()),
                             preMemoryBarriers.data(), 0, nullptr);

  struct PushConstants {
    VkDeviceAddress AddressOfLocations;
    VkDeviceAddress AddressOfPatchesMap;
    uint32_t numMapElements;
  } pushConstants = {locationsDataAddress, patchesMapDataAddress, (uint32_t)patchesMap.size()};

  drvVk.vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      SD().internalResources.internalPipelines[device].getPatchDeviceAddressesPipeline());

  drvVk.vkCmdPushConstants(commandBuffer,
                           SD().internalResources.internalPipelines[device].getLayout(),
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);

  drvVk.vkCmdDispatch(commandBuffer, count, 1, 1);

  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr,
                             static_cast<uint32_t>(postMemoryBarriers.size()),
                             postMemoryBarriers.data(), 0, nullptr);

  if (commandBufferState->currentPipeline != VK_NULL_HANDLE) {
    drvVk.vkCmdBindPipeline(commandBuffer, commandBufferState->currentPipelineBindPoint,
                            commandBufferState->currentPipeline);
  }
}

void gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::Write(CBinOStream& stream) const {
  _count->Write(stream);
  if (**_count > 0) {
    _commandBuffer->Write(stream);
    _resource->Write(stream);
  }
}

void gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::Read(CBinIStream& stream) {
  _count->Read(stream);
  if (**_count > 0) {
    _commandBuffer->Read(stream);
    _resource->Read(stream);
  }
}

void gits::Vulkan::CGitsVkCmdPatchDeviceAddresses::Write(CCodeOStream& stream) const {
  throw std::runtime_error("Ray tracing in CCode is not yet supported.");
}

const std::array<gits::Vulkan::ArgInfo, 1>
    gits::Vulkan::CDestroyVulkanDescriptorSets::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 1, false}, // VkDescriptorSet*
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CDestroyVulkanDescriptorSets::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CDestroyVulkanDescriptorSets::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _descSetsArray);
}

gits::Vulkan::CDestroyVulkanDescriptorSets::CDestroyVulkanDescriptorSets() {}

gits::Vulkan::CDestroyVulkanDescriptorSets::CDestroyVulkanDescriptorSets(
    size_t count, VkDescriptorSet* descSetsArray)
    : _descSetsArray(count, descSetsArray) {}

void gits::Vulkan::CDestroyVulkanDescriptorSets::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _descSetsArray.VariableNameRegister(stream, false);
  _descSetsArray.Declare(stream);
  stream.Indent() << Name() << '(' << gits::Csize_t(_descSetsArray.Size()) << ", " << _descSetsArray
                  << ");\n";
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

void gits::Vulkan::CDestroyVulkanDescriptorSets::Run() {
  _descSetsArray.RemoveMapping();
}

const std::array<gits::Vulkan::ArgInfo, 1>
    gits::Vulkan::CDestroyVulkanCommandBuffers::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 1, false}, // VkCommandBuffer*
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CDestroyVulkanCommandBuffers::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CDestroyVulkanCommandBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _cmdBufsArray);
}

gits::Vulkan::CDestroyVulkanCommandBuffers::CDestroyVulkanCommandBuffers() {}

gits::Vulkan::CDestroyVulkanCommandBuffers::CDestroyVulkanCommandBuffers(
    size_t count, VkCommandBuffer* cmdBufsArray)
    : _cmdBufsArray(count, cmdBufsArray) {}

void gits::Vulkan::CDestroyVulkanCommandBuffers::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _cmdBufsArray.VariableNameRegister(stream, false);
  _cmdBufsArray.Declare(stream);
  stream.Indent() << Name() << '(' << gits::Csize_t(_cmdBufsArray.Size()) << ", " << _cmdBufsArray
                  << ");\n";
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

void gits::Vulkan::CDestroyVulkanCommandBuffers::Run() {
  _cmdBufsArray.RemoveMapping();
}

// CGitsVkEnumerateDisplayMonitors

#if defined(GITS_PLATFORM_WINDOWS)

namespace {
BOOL CALLBACK Monitorenumproc(HMONITOR Arg1, HDC Arg2, LPRECT Arg3, LPARAM Arg4) {
  std::vector<HMONITOR>* monitors = (std::vector<HMONITOR>*)Arg4;

  monitors->emplace_back(Arg1);
  return TRUE;
}
} // namespace

#endif

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkEnumerateDisplayMonitors::ARG_NUM>
    gits::Vulkan::CGitsVkEnumerateDisplayMonitors::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 1, false} // _monitors
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkEnumerateDisplayMonitors::ArgumentInfo(
    unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkEnumerateDisplayMonitors::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _monitors);
}

gits::Vulkan::CGitsVkEnumerateDisplayMonitors::CGitsVkEnumerateDisplayMonitors() : _monitors() {}

gits::Vulkan::CGitsVkEnumerateDisplayMonitors::CGitsVkEnumerateDisplayMonitors(bool) {
#if defined(GITS_PLATFORM_WINDOWS)
  std::vector<HMONITOR> monitors;
  EnumDisplayMonitors(NULL, NULL, Monitorenumproc, (LPARAM)&monitors);

  _monitors = CVkHMONITOR::CSMapArray(monitors.size(), monitors.data());
#endif
}

void gits::Vulkan::CGitsVkEnumerateDisplayMonitors::Run() {
#if defined(GITS_PLATFORM_WINDOWS)
  std::vector<HMONITOR> monitors;
  EnumDisplayMonitors(NULL, NULL, Monitorenumproc, (LPARAM)&monitors);

  for (size_t i = 0; (i < monitors.size()) && (i < _monitors.Size()); ++i) {
    (*_monitors)[i] = monitors[i];
  }
#endif
}

void gits::Vulkan::CGitsVkEnumerateDisplayMonitors::Write(CBinOStream& stream) const {
  _monitors.Write(stream);
}

void gits::Vulkan::CGitsVkEnumerateDisplayMonitors::Read(CBinIStream& stream) {
  _monitors.Read(stream);
}

//
// CGitsInitializeImage
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsInitializeImage::ARG_NUM>
    gits::Vulkan::CGitsInitializeImage::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false}, // VkCommandBuffer (CVkCommandBuffer)

        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkDependencyFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1, false}, // const VkMemoryBarrier* (CVkMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkBufferMemoryBarrier* (CVkBufferMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkImageMemoryBarrier* (CVkImageMemoryBarrierArray)

        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkBuffer (CVkBuffer)
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkImage (CVkImage)
        {gits::Vulkan::ArgType::ENUM, 0, false},           // VkImageLayout (CVkImageLayout)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkBufferImageCopy* (CVkBufferImageCopyArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkInitializeImageGITS* (CVkInitializeImageGITSArray)

        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkDependencyFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1, false}, // const VkMemoryBarrier* (CVkMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkBufferMemoryBarrier* (CVkBufferMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkImageMemoryBarrier* (CVkImageMemoryBarrierArray)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsInitializeImage::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsInitializeImage::Argument(unsigned idx) {
  return get_cargument(
      __FUNCTION__, idx, _commandBuffer, _preSrcStageMask, _preDstStageMask, _preDependencyFlags,
      _preMemoryBarrierCount, _prePMemoryBarriers, _preBufferMemoryBarrierCount,
      _prePBufferMemoryBarriers, _preImageMemoryBarrierCount, _prePImageMemoryBarriers,
      _copySrcBuffer, _copyDstImage, _copyDstImageLayout, _copyRegionCount, _copyPRegions,
      _initializeRegionCount, _initializePRegions, _postSrcStageMask, _postDstStageMask,
      _postDependencyFlags, _postMemoryBarrierCount, _postPMemoryBarriers,
      _postBufferMemoryBarrierCount, _postPBufferMemoryBarriers, _postImageMemoryBarrierCount,
      _postPImageMemoryBarriers);
}

gits::Vulkan::CGitsInitializeImage::CGitsInitializeImage() {}

void gits::Vulkan::CGitsInitializeImage::Run() {
  auto& state = SD()._imagestates[*_copyDstImage];
  if (state->imageCreateInfoData.Value() &&
      state->imageCreateInfoData.Value()->samples != VK_SAMPLE_COUNT_1_BIT) {
    CALL_ONCE[] {
      Log(WARN) << "Restoring multisample image. Calling vkCmdCopyBufferToImage with number of "
                   "samples greater than one is against specification.";
    };
  }
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
  }
}

void gits::Vulkan::CGitsInitializeImage::Exec() {
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, *_preSrcStageMask, *_preDstStageMask,
                             *_preDependencyFlags, *_preMemoryBarrierCount, *_prePMemoryBarriers,
                             *_preBufferMemoryBarrierCount, *_prePBufferMemoryBarriers,
                             *_preImageMemoryBarrierCount, *_prePImageMemoryBarriers);
  drvVk.vkCmdCopyBufferToImage(*_commandBuffer, *_copySrcBuffer, *_copyDstImage,
                               *_copyDstImageLayout, *_copyRegionCount, *_copyPRegions);
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, *_postSrcStageMask, *_postDstStageMask,
                             *_postDependencyFlags, *_postMemoryBarrierCount, *_postPMemoryBarriers,
                             *_postBufferMemoryBarrierCount, *_postPBufferMemoryBarriers,
                             *_postImageMemoryBarrierCount, *_postPImageMemoryBarriers);
}

void gits::Vulkan::CGitsInitializeImage::StateTrack() {
  vkCmdPipelineBarrier_SD(*_commandBuffer, *_preSrcStageMask, *_preDstStageMask,
                          *_preDependencyFlags, *_preMemoryBarrierCount, *_prePMemoryBarriers,
                          *_preBufferMemoryBarrierCount, *_prePBufferMemoryBarriers,
                          *_preImageMemoryBarrierCount, *_prePImageMemoryBarriers);
  vkCmdCopyBufferToImage_SD(*_commandBuffer, *_copySrcBuffer, *_copyDstImage, *_copyDstImageLayout,
                            *_copyRegionCount, *_copyPRegions);
  vkCmdPipelineBarrier_SD(*_commandBuffer, *_postSrcStageMask, *_postDstStageMask,
                          *_postDependencyFlags, *_postMemoryBarrierCount, *_postPMemoryBarriers,
                          *_postBufferMemoryBarrierCount, *_postPBufferMemoryBarriers,
                          *_postImageMemoryBarrierCount, *_postPImageMemoryBarriers);
}

void gits::Vulkan::CGitsInitializeImage::TokenBuffersUpdate() {
  gits::Vulkan::CLibrary::CVulkanCommandBufferTokensBuffer& tokensBuffer =
      SD()._commandbufferstates[*_commandBuffer]->tokensBuffer;
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), _preSrcStageMask.Original(), _preDstStageMask.Original(),
      _preDependencyFlags.Original(), _preMemoryBarrierCount.Original(),
      _prePMemoryBarriers.Original(), _preBufferMemoryBarrierCount.Original(),
      _prePBufferMemoryBarriers.Original(), _preImageMemoryBarrierCount.Original(),
      _prePImageMemoryBarriers.Original()));
  tokensBuffer.Add(new CvkCmdCopyBufferToImage(
      _commandBuffer.Original(), _copySrcBuffer.Original(), _copyDstImage.Original(),
      _copyDstImageLayout.Original(), _copyRegionCount.Original(), _copyPRegions.Original()));
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), _postSrcStageMask.Original(), _postDstStageMask.Original(),
      _postDependencyFlags.Original(), _postMemoryBarrierCount.Original(),
      _postPMemoryBarriers.Original(), _postBufferMemoryBarrierCount.Original(),
      _postPBufferMemoryBarriers.Original(), _postImageMemoryBarrierCount.Original(),
      _postPImageMemoryBarriers.Original()));
}

void gits::Vulkan::CGitsInitializeImage::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _prePMemoryBarriers.VariableNameRegister(stream, false);
  _prePMemoryBarriers.Declare(stream);
  _prePBufferMemoryBarriers.VariableNameRegister(stream, false);
  _prePBufferMemoryBarriers.Declare(stream);
  _prePImageMemoryBarriers.VariableNameRegister(stream, false);
  _prePImageMemoryBarriers.Declare(stream);
  stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer << ", " << _preSrcStageMask << ", "
                  << _preDstStageMask << ", " << _preDependencyFlags << ", "
                  << _preMemoryBarrierCount << ", " << _prePMemoryBarriers << ", "
                  << _preBufferMemoryBarrierCount << ", " << _prePBufferMemoryBarriers << ", "
                  << _preImageMemoryBarrierCount << ", " << _prePImageMemoryBarriers << ");"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;

  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _copyPRegions.VariableNameRegister(stream, false);
  _copyPRegions.Declare(stream);
  stream.Indent() << "vkCmdCopyBufferToImage( " << _commandBuffer << ", " << _copySrcBuffer << ", "
                  << _copyDstImage << ", " << _copyDstImageLayout << ", " << _copyRegionCount
                  << ", " << _copyPRegions << ");" << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;

  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _postPMemoryBarriers.VariableNameRegister(stream, false);
  _postPMemoryBarriers.Declare(stream);
  _postPBufferMemoryBarriers.VariableNameRegister(stream, false);
  _postPBufferMemoryBarriers.Declare(stream);
  _postPImageMemoryBarriers.VariableNameRegister(stream, false);
  _postPImageMemoryBarriers.Declare(stream);
  stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer << ", " << _postSrcStageMask << ", "
                  << _postDstStageMask << ", " << _postDependencyFlags << ", "
                  << _postMemoryBarrierCount << ", " << _postPMemoryBarriers << ", "
                  << _postBufferMemoryBarrierCount << ", " << _postPBufferMemoryBarriers << ", "
                  << _postImageMemoryBarrierCount << ", " << _postPImageMemoryBarriers << ");"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

std::set<uint64_t> gits::Vulkan::CGitsInitializeImage::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto obj : _commandBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _prePMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _prePBufferMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _prePImageMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _copySrcBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _copyDstImage.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _copyPRegions.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _initializePRegions.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _postPMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _postPBufferMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _postPImageMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }

  return returnMap;
}

//
// CGitsVkCmdInsertMemoryBarriers
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::ARG_NUM>
    gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkCommandBuffer (CVkCommandBuffer)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkDependencyFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1, false}, // const VkMemoryBarrier* (CVkMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkBufferMemoryBarrier* (CVkBufferMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkImageMemoryBarrier* (CVkImageMemoryBarrierArray)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::ArgumentInfo(
    unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _commandBuffer, _SrcStageMask, _DstStageMask,
                       _DependencyFlags, _MemoryBarrierCount, _PMemoryBarriers,
                       _BufferMemoryBarrierCount, _PBufferMemoryBarriers, _ImageMemoryBarrierCount,
                       _PImageMemoryBarriers);
}

gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::CGitsVkCmdInsertMemoryBarriers() {}

gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::CGitsVkCmdInsertMemoryBarriers(
    VkCommandBuffer commandBuffer,
    VkPipelineStageFlags SrcStageMask,
    VkPipelineStageFlags DstStageMask,
    VkDependencyFlags DependencyFlags,
    uint32_t MemoryBarrierCount,
    const VkMemoryBarrier* PMemoryBarriers,
    uint32_t BufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* PBufferMemoryBarriers,
    uint32_t ImageMemoryBarrierCount,
    const VkImageMemoryBarrier* PImageMemoryBarriers)
    : _commandBuffer(commandBuffer),
      _SrcStageMask(SrcStageMask),
      _DstStageMask(DstStageMask),
      _DependencyFlags(DependencyFlags),
      _MemoryBarrierCount(MemoryBarrierCount),
      _PMemoryBarriers(MemoryBarrierCount, PMemoryBarriers),
      _BufferMemoryBarrierCount(BufferMemoryBarrierCount),
      _PBufferMemoryBarriers(BufferMemoryBarrierCount, PBufferMemoryBarriers),
      _ImageMemoryBarrierCount(ImageMemoryBarrierCount),
      _PImageMemoryBarriers(ImageMemoryBarrierCount, PImageMemoryBarriers) {}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::Run() {
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
  }
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::Exec() {
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, *_SrcStageMask, *_DstStageMask, *_DependencyFlags,
                             *_MemoryBarrierCount, *_PMemoryBarriers, *_BufferMemoryBarrierCount,
                             *_PBufferMemoryBarriers, *_ImageMemoryBarrierCount,
                             *_PImageMemoryBarriers);
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::StateTrack() {
  vkCmdPipelineBarrier_SD(*_commandBuffer, *_SrcStageMask, *_DstStageMask, *_DependencyFlags,
                          *_MemoryBarrierCount, *_PMemoryBarriers, *_BufferMemoryBarrierCount,
                          *_PBufferMemoryBarriers, *_ImageMemoryBarrierCount,
                          *_PImageMemoryBarriers);
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::TokenBuffersUpdate() {
  SD()._commandbufferstates[*_commandBuffer]->tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), _SrcStageMask.Original(), _DstStageMask.Original(),
      _DependencyFlags.Original(), _MemoryBarrierCount.Original(), _PMemoryBarriers.Original(),
      _BufferMemoryBarrierCount.Original(), _PBufferMemoryBarriers.Original(),
      _ImageMemoryBarrierCount.Original(), _PImageMemoryBarriers.Original()));
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::Write(CCodeOStream& stream) const {
  size_t chunkSize = Configurator::Get().vulkan.recorder.maxArraySizeForCCode;
  size_t itMemoryBarriers = 0, itBufferMemoryBarriers = 0, itImageMemoryBarriers = 0;

  while (itMemoryBarriers < *_MemoryBarrierCount ||
         itBufferMemoryBarriers < *_BufferMemoryBarrierCount ||
         itImageMemoryBarriers < *_ImageMemoryBarrierCount) {
    size_t sizeMemoryBarrier =
        CalculateChunkSize(*_MemoryBarrierCount, chunkSize, itMemoryBarriers);
    size_t sizeBufferMemoryBarrier =
        CalculateChunkSize(*_BufferMemoryBarrierCount, chunkSize, itBufferMemoryBarriers);
    size_t sizeImageMemoryBarrier =
        CalculateChunkSize(*_ImageMemoryBarrierCount, chunkSize, itImageMemoryBarriers);

    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
    _PMemoryBarriers.VariableNameRegister(stream, false);
    _PMemoryBarriers.Declare(stream, itMemoryBarriers, itMemoryBarriers + sizeMemoryBarrier);
    _PBufferMemoryBarriers.VariableNameRegister(stream, false);
    _PBufferMemoryBarriers.Declare(stream, itBufferMemoryBarriers,
                                   itBufferMemoryBarriers + sizeBufferMemoryBarrier);
    _PImageMemoryBarriers.VariableNameRegister(stream, false);
    _PImageMemoryBarriers.Declare(stream, itImageMemoryBarriers,
                                  itImageMemoryBarriers + sizeImageMemoryBarrier);
    stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer << ", " << _SrcStageMask << ", "
                    << _DstStageMask << ", " << _DependencyFlags << ", " << sizeMemoryBarrier
                    << ", " << _PMemoryBarriers << ", " << sizeBufferMemoryBarrier << ", "
                    << _PBufferMemoryBarriers << ", " << sizeImageMemoryBarrier << ", "
                    << _PImageMemoryBarriers << ");" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;

    itMemoryBarriers += chunkSize;
    itBufferMemoryBarriers += chunkSize;
    itImageMemoryBarriers += chunkSize;
  }
}

std::set<uint64_t> gits::Vulkan::CGitsVkCmdInsertMemoryBarriers::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto obj : _commandBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _PMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _PBufferMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _PImageMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }

  return returnMap;
}

//
// CGitsVkCmdInsertMemoryBarriers2
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::ARG_NUM>
    gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false}, // VkCommandBuffer (CVkCommandBuffer)
        {gits::Vulkan::ArgType::STRUCT, 1, true}, // const VkDependencyInfo* (CVkDependencyInfo)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::ArgumentInfo(
    unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _commandBuffer, _dependencyInfo);
}

gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::CGitsVkCmdInsertMemoryBarriers2() {}

gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::CGitsVkCmdInsertMemoryBarriers2(
    VkCommandBuffer commandBuffer, const VkDependencyInfo* pDependencyInfo)
    : _commandBuffer(commandBuffer), _dependencyInfo(pDependencyInfo) {}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::Run() {
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
  }
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::Exec() {
  drvVk.vkCmdPipelineBarrier2UnifiedGITS(*_commandBuffer, *_dependencyInfo);
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::StateTrack() {
  vkCmdPipelineBarrier2UnifiedGITS_SD(*_commandBuffer, *_dependencyInfo);
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::TokenBuffersUpdate() {
  SD()._commandbufferstates[*_commandBuffer]->tokensBuffer.Add(
      new CvkCmdPipelineBarrier2UnifiedGITS(_commandBuffer.Original(), _dependencyInfo.Original()));
}

void gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::Write(CCodeOStream& stream) const {
  CVkDependencyInfoCCodeWriter(stream, "vkCmdPipelineBarrier2UnifiedGITS", _commandBuffer,
                               _dependencyInfo);
}

std::set<uint64_t> gits::Vulkan::CGitsVkCmdInsertMemoryBarriers2::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto obj : _commandBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _dependencyInfo.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }

  return returnMap;
}

//
// CGitsInitializeMultipleImages
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsInitializeMultipleImages::ARG_NUM>
    gits::Vulkan::CGitsInitializeMultipleImages::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkCommandBuffer (CVkCommandBuffer)
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkBuffer (CVkBuffer)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkInitializeImageDataGITS* (CVkInitializeImageDataGITSArray)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsInitializeMultipleImages::ArgumentInfo(
    unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsInitializeMultipleImages::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _commandBuffer, _copySrcBuffer, _imagesCount,
                       _pInitializeImages);
}

gits::Vulkan::CGitsInitializeMultipleImages::CGitsInitializeMultipleImages() {}

gits::Vulkan::CGitsInitializeMultipleImages::CGitsInitializeMultipleImages(
    VkCommandBuffer commandBuffer,
    VkBuffer copySrcBuffer,
    std::vector<VkInitializeImageDataGITS> const& initializeImages)
    : _commandBuffer(commandBuffer),
      _copySrcBuffer(copySrcBuffer),
      _imagesCount(initializeImages.size()),
      _pInitializeImages(initializeImages.size(), initializeImages.data()) {}

void gits::Vulkan::CGitsInitializeMultipleImages::Run() {
  auto initializeImages = *_pInitializeImages;
  if (initializeImages == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  for (uint32_t i = 0; i < *_imagesCount; ++i) {
    auto& state = SD()._imagestates[initializeImages[i].image];
    if (state->imageCreateInfoData.Value() &&
        state->imageCreateInfoData.Value()->samples != VK_SAMPLE_COUNT_1_BIT) {
      CALL_ONCE[] {
        Log(WARN) << "Restoring multisample image. Calling vkCmdCopyBufferToImage with"
                     " number of samples greater than one is against specification.";
      };
      break;
    }
  }

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPre = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags srcAccessMask
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      *_copySrcBuffer,                         // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  _copyFromBufferMemoryBarrierPre =
      std::make_unique<CVkBufferMemoryBarrierData>(&copyFromBufferMemoryBarrierPre);

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPost = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      *_copySrcBuffer,                         // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  _copyFromBufferMemoryBarrierPost =
      std::make_unique<CVkBufferMemoryBarrierData>(&copyFromBufferMemoryBarrierPost);

  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
  }
}

void gits::Vulkan::CGitsInitializeMultipleImages::Exec() {
  auto initializeImages = *_pInitializeImages;
  if (initializeImages == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  drvVk.vkCmdPipelineBarrier(*_commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, 1, _copyFromBufferMemoryBarrierPre.get()->Value(), 0,
                             nullptr);
  for (uint32_t i = 0; i < *_imagesCount; ++i) {
    drvVk.vkCmdCopyBufferToImage(*_commandBuffer, *_copySrcBuffer, initializeImages[i].image,
                                 initializeImages[i].layout, initializeImages[i].copyRegionsCount,
                                 initializeImages[i].pCopyRegions);
  }
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                             _copyFromBufferMemoryBarrierPost.get()->Value(), 0, nullptr);
}

void gits::Vulkan::CGitsInitializeMultipleImages::StateTrack() {
  auto initializeImages = *_pInitializeImages;
  if (initializeImages == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkCmdPipelineBarrier_SD(*_commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
                          1, _copyFromBufferMemoryBarrierPre.get()->Value(), 0, nullptr);
  for (uint32_t i = 0; i < *_imagesCount; ++i) {
    vkCmdCopyBufferToImage_SD(*_commandBuffer, *_copySrcBuffer, initializeImages[i].image,
                              initializeImages[i].layout, initializeImages[i].copyRegionsCount,
                              initializeImages[i].pCopyRegions);
  }
  vkCmdPipelineBarrier_SD(*_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_HOST_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                          _copyFromBufferMemoryBarrierPost.get()->Value(), 0, nullptr);
}

void gits::Vulkan::CGitsInitializeMultipleImages::TokenBuffersUpdate() {
  auto initializeImages = _pInitializeImages.Original();
  if (initializeImages == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPre = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags srcAccessMask
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      _copySrcBuffer.Original(),               // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPost = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      _copySrcBuffer.Original(),               // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  gits::Vulkan::CLibrary::CVulkanCommandBufferTokensBuffer& tokensBuffer =
      SD()._commandbufferstates[*_commandBuffer]->tokensBuffer;
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyFromBufferMemoryBarrierPre, 0, nullptr));
  for (uint32_t i = 0; i < *_imagesCount; ++i) {
    tokensBuffer.Add(new CvkCmdCopyBufferToImage(
        _commandBuffer.Original(), _copySrcBuffer.Original(), initializeImages[i].image,
        initializeImages[i].layout, initializeImages[i].copyRegionsCount,
        initializeImages[i].pCopyRegions));
  }
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
      VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyFromBufferMemoryBarrierPost, 0, nullptr));
}

void gits::Vulkan::CGitsInitializeMultipleImages::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VkBufferMemoryBarrier copyFromBufferMemoryBarrierPre = {" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType sType"
                  << std::endl;
  stream.Indent() << "nullptr,                                    // const void* pNext"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_HOST_WRITE_BIT,                   // VkAccessFlags srcAccessMask"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_TRANSFER_READ_BIT,                // VkAccessFlags dstAccessMask"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t srcQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t dstQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << _copySrcBuffer << ",                             // VkBuffer buffer"
                  << std::endl;
  stream.Indent() << "0,                                          // VkDeviceSize offset"
                  << std::endl;
  stream.Indent() << "VK_WHOLE_SIZE                               // VkDeviceSize size;"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "};" << std::endl;
  stream.Indent()
      << "vkCmdPipelineBarrier(" << _commandBuffer
      << ", VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, "
         "VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyFromBufferMemoryBarrierPre, 0, nullptr);"
      << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;

  size_t chunkSize = Configurator::Get().vulkan.recorder.maxArraySizeForCCode;
  for (size_t i = 0; i < *_imagesCount; i += chunkSize) {
    size_t size = chunkSize;
    if (i + chunkSize > *_imagesCount) {
      size = *_imagesCount - i;
    }

    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
    _pInitializeImages.VariableNameRegister(stream, false);
    _pInitializeImages.Declare(stream, i, i + size);
    stream.Indent() << "for (uint32_t i = 0; i < " << size << "; ++i) {" << std::endl;
    stream.ScopeBegin();
    stream.Indent() << "vkCmdCopyBufferToImage( " << _commandBuffer << ", " << _copySrcBuffer
                    << ", " << _pInitializeImages << "[i].image, " << _pInitializeImages
                    << "[i].layout, " << _pInitializeImages << "[i].copyRegionsCount, "
                    << _pInitializeImages << "[i].pCopyRegions);" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
  }

  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VkBufferMemoryBarrier copyFromBufferMemoryBarrierPost = {" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType sType"
                  << std::endl;
  stream.Indent() << "nullptr,                                    // const void* pNext"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_TRANSFER_READ_BIT,                // VkAccessFlags srcAccessMask"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_HOST_WRITE_BIT,                   // VkAccessFlags dstAccessMask"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t srcQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t dstQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << _copySrcBuffer << ",                             // VkBuffer buffer"
                  << std::endl;
  stream.Indent() << "0,                                          // VkDeviceSize offset"
                  << std::endl;
  stream.Indent() << "VK_WHOLE_SIZE                               // VkDeviceSize size;"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "};" << std::endl;
  stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer
                  << ", VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, "
                     "VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, "
                     "&copyFromBufferMemoryBarrierPost, 0, nullptr);"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

std::set<uint64_t> gits::Vulkan::CGitsInitializeMultipleImages::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto obj : _commandBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _copySrcBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _pInitializeImages.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  return returnMap;
}

//
// CGitsInitializeBuffer
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsInitializeBuffer::ARG_NUM>
    gits::Vulkan::CGitsInitializeBuffer::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false}, // VkCommandBuffer (CVkCommandBuffer)

        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkDependencyFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1, false}, // const VkMemoryBarrier* (CVkMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkBufferMemoryBarrier* (CVkBufferMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkImageMemoryBarrier* (CVkImageMemoryBarrierArray)

        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkBuffer (CVkBuffer)
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkBuffer (CVkBuffer)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1, false}, // const VkBufferCopy* (CVkBufferCopyArray)

        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkPipelineStageFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // VkDependencyFlags (Cuint32_t)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1, false}, // const VkMemoryBarrier* (CVkMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkBufferMemoryBarrier* (CVkBufferMemoryBarrierArray)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkImageMemoryBarrier* (CVkImageMemoryBarrierArray)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsInitializeBuffer::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsInitializeBuffer::Argument(unsigned idx) {
  return get_cargument(
      __FUNCTION__, idx, _commandBuffer, _preSrcStageMask, _preDstStageMask, _preDependencyFlags,
      _preMemoryBarrierCount, _prePMemoryBarriers, _preBufferMemoryBarrierCount,
      _prePBufferMemoryBarriers, _preImageMemoryBarrierCount, _prePImageMemoryBarriers,
      _dataSrcBuffer, _dataDstBuffer, _dataRegionCount, _dataPRegions, _postSrcStageMask,
      _postDstStageMask, _postDependencyFlags, _postMemoryBarrierCount, _postPMemoryBarriers,
      _postBufferMemoryBarrierCount, _postPBufferMemoryBarriers, _postImageMemoryBarrierCount,
      _postPImageMemoryBarriers);
}

gits::Vulkan::CGitsInitializeBuffer::CGitsInitializeBuffer() {}

void gits::Vulkan::CGitsInitializeBuffer::Run() {
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
  }
}

void gits::Vulkan::CGitsInitializeBuffer::Exec() {
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, *_preSrcStageMask, *_preDstStageMask,
                             *_preDependencyFlags, *_preMemoryBarrierCount, *_prePMemoryBarriers,
                             *_preBufferMemoryBarrierCount, *_prePBufferMemoryBarriers,
                             *_preImageMemoryBarrierCount, *_prePImageMemoryBarriers);
  drvVk.vkCmdCopyBuffer(*_commandBuffer, *_dataSrcBuffer, *_dataDstBuffer, *_dataRegionCount,
                        *_dataPRegions);
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, *_postSrcStageMask, *_postDstStageMask,
                             *_postDependencyFlags, *_postMemoryBarrierCount, *_postPMemoryBarriers,
                             *_postBufferMemoryBarrierCount, *_postPBufferMemoryBarriers,
                             *_postImageMemoryBarrierCount, *_postPImageMemoryBarriers);
}

void gits::Vulkan::CGitsInitializeBuffer::StateTrack() {
  vkCmdPipelineBarrier_SD(*_commandBuffer, *_preSrcStageMask, *_preDstStageMask,
                          *_preDependencyFlags, *_preMemoryBarrierCount, *_prePMemoryBarriers,
                          *_preBufferMemoryBarrierCount, *_prePBufferMemoryBarriers,
                          *_preImageMemoryBarrierCount, *_prePImageMemoryBarriers);
  vkCmdCopyBuffer_SD(*_commandBuffer, *_dataSrcBuffer, *_dataDstBuffer, *_dataRegionCount,
                     *_dataPRegions);
  vkCmdPipelineBarrier_SD(*_commandBuffer, *_postSrcStageMask, *_postDstStageMask,
                          *_postDependencyFlags, *_postMemoryBarrierCount, *_postPMemoryBarriers,
                          *_postBufferMemoryBarrierCount, *_postPBufferMemoryBarriers,
                          *_postImageMemoryBarrierCount, *_postPImageMemoryBarriers);
}

void gits::Vulkan::CGitsInitializeBuffer::TokenBuffersUpdate() {
  gits::Vulkan::CLibrary::CVulkanCommandBufferTokensBuffer& tokensBuffer =
      SD()._commandbufferstates[*_commandBuffer]->tokensBuffer;
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), _preSrcStageMask.Original(), _preDstStageMask.Original(),
      _preDependencyFlags.Original(), _preMemoryBarrierCount.Original(),
      _prePMemoryBarriers.Original(), _preBufferMemoryBarrierCount.Original(),
      _prePBufferMemoryBarriers.Original(), _preImageMemoryBarrierCount.Original(),
      _prePImageMemoryBarriers.Original()));
  tokensBuffer.Add(new CvkCmdCopyBuffer(_commandBuffer.Original(), _dataSrcBuffer.Original(),
                                        _dataDstBuffer.Original(), _dataRegionCount.Original(),
                                        _dataPRegions.Original()));
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), _postSrcStageMask.Original(), _postDstStageMask.Original(),
      _postDependencyFlags.Original(), _postMemoryBarrierCount.Original(),
      _postPMemoryBarriers.Original(), _postBufferMemoryBarrierCount.Original(),
      _postPBufferMemoryBarriers.Original(), _postImageMemoryBarrierCount.Original(),
      _postPImageMemoryBarriers.Original()));
}

void gits::Vulkan::CGitsInitializeBuffer::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _prePMemoryBarriers.VariableNameRegister(stream, false);
  _prePMemoryBarriers.Declare(stream);
  _prePBufferMemoryBarriers.VariableNameRegister(stream, false);
  _prePBufferMemoryBarriers.Declare(stream);
  _prePImageMemoryBarriers.VariableNameRegister(stream, false);
  _prePImageMemoryBarriers.Declare(stream);
  stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer << ", " << _preSrcStageMask << ", "
                  << _preDstStageMask << ", " << _preDependencyFlags << ", "
                  << _preMemoryBarrierCount << ", " << _prePMemoryBarriers << ", "
                  << _preBufferMemoryBarrierCount << ", " << _prePBufferMemoryBarriers << ", "
                  << _preImageMemoryBarrierCount << ", " << _prePImageMemoryBarriers << ");"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;

  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _dataPRegions.VariableNameRegister(stream, false);
  _dataPRegions.Declare(stream);
  stream.Indent() << "vkCmdCopyBuffer( " << _commandBuffer << ", " << _dataSrcBuffer << ", "
                  << _dataDstBuffer << ", " << _dataRegionCount << ", " << _dataPRegions << ");"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;

  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  _postPMemoryBarriers.VariableNameRegister(stream, false);
  _postPMemoryBarriers.Declare(stream);
  _postPBufferMemoryBarriers.VariableNameRegister(stream, false);
  _postPBufferMemoryBarriers.Declare(stream);
  _postPImageMemoryBarriers.VariableNameRegister(stream, false);
  _postPImageMemoryBarriers.Declare(stream);
  stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer << ", " << _postSrcStageMask << ", "
                  << _postDstStageMask << ", " << _postDependencyFlags << ", "
                  << _postMemoryBarrierCount << ", " << _postPMemoryBarriers << ", "
                  << _postBufferMemoryBarrierCount << ", " << _postPBufferMemoryBarriers << ", "
                  << _postImageMemoryBarrierCount << ", " << _postPImageMemoryBarriers << ");"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

std::set<uint64_t> gits::Vulkan::CGitsInitializeBuffer::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto obj : _commandBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _prePMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _prePBufferMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _prePImageMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _dataSrcBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _dataDstBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _dataPRegions.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _postPMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _postPBufferMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _postPImageMemoryBarriers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }

  return returnMap;
}

//
// CGitsInitializeMultipleBuffers
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsInitializeMultipleBuffers::ARG_NUM>
    gits::Vulkan::CGitsInitializeMultipleBuffers::argumentInfos_ = {{
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkCommandBuffer (CVkCommandBuffer)
        {gits::Vulkan::ArgType::OPAQUE_HANDLE, 0, false},  // VkBuffer (CVkBuffer)
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // uint32_t (Cuint32_t)
        {gits::Vulkan::ArgType::STRUCT, 1,
         false}, // const VkInitializeBufferDataGITS* (CVkInitializeBufferDataGITSArray)
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsInitializeMultipleBuffers::ArgumentInfo(
    unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsInitializeMultipleBuffers::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _commandBuffer, _copySrcBuffer, _buffersCount,
                       _pInitializeBuffers);
}

gits::Vulkan::CGitsInitializeMultipleBuffers::CGitsInitializeMultipleBuffers() {}

gits::Vulkan::CGitsInitializeMultipleBuffers::CGitsInitializeMultipleBuffers(
    VkCommandBuffer commandBuffer,
    VkBuffer copySrcBuffer,
    std::vector<VkInitializeBufferDataGITS> const& initializeBuffers)
    : _commandBuffer(commandBuffer),
      _copySrcBuffer(copySrcBuffer),
      _buffersCount(initializeBuffers.size()),
      _pInitializeBuffers(initializeBuffers.size(), initializeBuffers.data()) {}

void gits::Vulkan::CGitsInitializeMultipleBuffers::Run() {
  auto initializeBuffers = *_pInitializeBuffers;
  if (initializeBuffers == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPre = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags srcAccessMask
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      *_copySrcBuffer,                         // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  _copyFromBufferMemoryBarrierPre =
      std::make_unique<CVkBufferMemoryBarrierData>(&copyFromBufferMemoryBarrierPre);

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPost = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      *_copySrcBuffer,                         // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  _copyFromBufferMemoryBarrierPost =
      std::make_unique<CVkBufferMemoryBarrierData>(&copyFromBufferMemoryBarrierPost);

  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
  }
}

void gits::Vulkan::CGitsInitializeMultipleBuffers::Exec() {
  auto initializeBuffers = *_pInitializeBuffers;
  if (initializeBuffers == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, 1, _copyFromBufferMemoryBarrierPre.get()->Value(), 0,
                             nullptr);
  for (uint32_t i = 0; i < *_buffersCount; ++i) {
    drvVk.vkCmdCopyBuffer(*_commandBuffer, *_copySrcBuffer, initializeBuffers[i].buffer, 1,
                          &initializeBuffers[i].bufferCopy);
  }
  drvVk.vkCmdPipelineBarrier(*_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                             _copyFromBufferMemoryBarrierPost.get()->Value(), 0, nullptr);
}
void gits::Vulkan::CGitsInitializeMultipleBuffers::StateTrack() {
  auto initializeBuffers = *_pInitializeBuffers;
  if (initializeBuffers == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  vkCmdPipelineBarrier_SD(*_commandBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr,
                          1, _copyFromBufferMemoryBarrierPre.get()->Value(), 0, nullptr);
  for (uint32_t i = 0; i < *_buffersCount; ++i) {
    vkCmdCopyBuffer_SD(*_commandBuffer, *_copySrcBuffer, initializeBuffers[i].buffer, 1,
                       &initializeBuffers[i].bufferCopy);
  }
  vkCmdPipelineBarrier_SD(*_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_HOST_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                          _copyFromBufferMemoryBarrierPost.get()->Value(), 0, nullptr);
}
void gits::Vulkan::CGitsInitializeMultipleBuffers::TokenBuffersUpdate() {
  auto initializeBuffers = _pInitializeBuffers.Original();
  if (initializeBuffers == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPre = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags srcAccessMask
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      _copySrcBuffer.Original(),               // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  VkBufferMemoryBarrier copyFromBufferMemoryBarrierPost = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
      nullptr,                                 // const void* pNext
      VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask
      VK_ACCESS_HOST_WRITE_BIT,                // VkAccessFlags dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex
      _copySrcBuffer.Original(),               // VkBuffer buffer
      0,                                       // VkDeviceSize offset
      VK_WHOLE_SIZE                            // VkDeviceSize size;
  };

  gits::Vulkan::CLibrary::CVulkanCommandBufferTokensBuffer& tokensBuffer =
      SD()._commandbufferstates[*_commandBuffer]->tokensBuffer;
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyFromBufferMemoryBarrierPre, 0, nullptr));
  for (uint32_t i = 0; i < *_buffersCount; ++i) {
    tokensBuffer.Add(new CvkCmdCopyBuffer(_commandBuffer.Original(), _copySrcBuffer.Original(),
                                          initializeBuffers[i].buffer, 1,
                                          &initializeBuffers[i].bufferCopy));
  }
  tokensBuffer.Add(new CvkCmdPipelineBarrier(
      _commandBuffer.Original(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
      VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyFromBufferMemoryBarrierPost, 0, nullptr));
}

void gits::Vulkan::CGitsInitializeMultipleBuffers::Write(CCodeOStream& stream) const {
  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VkBufferMemoryBarrier copyFromBufferMemoryBarrierPre = {" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType sType"
                  << std::endl;
  stream.Indent() << "nullptr,                                    // const void* pNext"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_HOST_WRITE_BIT,                   // VkAccessFlags srcAccessMask"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_TRANSFER_READ_BIT,                // VkAccessFlags dstAccessMask"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t srcQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t dstQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << _copySrcBuffer << ",                             // VkBuffer buffer"
                  << std::endl;
  stream.Indent() << "0,                                          // VkDeviceSize offset"
                  << std::endl;
  stream.Indent() << "VK_WHOLE_SIZE                               // VkDeviceSize size;"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "};" << std::endl;
  stream.Indent()
      << "vkCmdPipelineBarrier(" << _commandBuffer
      << ", VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, "
         "VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyFromBufferMemoryBarrierPre, 0, nullptr);"
      << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;

  size_t chunkSize = Configurator::Get().vulkan.recorder.maxArraySizeForCCode;
  for (size_t i = 0; i < *_buffersCount; i += chunkSize) {
    size_t size = chunkSize;
    if (i + chunkSize > *_buffersCount) {
      size = *_buffersCount - i;
    }

    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
    _pInitializeBuffers.VariableNameRegister(stream, false);
    _pInitializeBuffers.Declare(stream, i, i + size);
    stream.Indent() << "for (uint32_t i = 0; i < " << size << "; ++i) {" << std::endl;
    stream.ScopeBegin();
    stream.Indent() << "vkCmdCopyBuffer( " << _commandBuffer << ", " << _copySrcBuffer << ", "
                    << _pInitializeBuffers << "[i].buffer, "
                    << "1, "
                    << "&" << _pInitializeBuffers << "[i].bufferCopy);" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
  }

  stream.Indent() << "{" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VkBufferMemoryBarrier copyFromBufferMemoryBarrierPost = {" << std::endl;
  stream.ScopeBegin();
  stream.Indent() << "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType sType"
                  << std::endl;
  stream.Indent() << "nullptr,                                    // const void* pNext"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_TRANSFER_READ_BIT,                // VkAccessFlags srcAccessMask"
                  << std::endl;
  stream.Indent() << "VK_ACCESS_HOST_WRITE_BIT,                   // VkAccessFlags dstAccessMask"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t srcQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << "VK_QUEUE_FAMILY_IGNORED,                    // uint32_t dstQueueFamilyIndex"
                  << std::endl;
  stream.Indent() << _copySrcBuffer << ",                             // VkBuffer buffer"
                  << std::endl;
  stream.Indent() << "0,                                          // VkDeviceSize offset"
                  << std::endl;
  stream.Indent() << "VK_WHOLE_SIZE                               // VkDeviceSize size;"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "};" << std::endl;
  stream.Indent() << "vkCmdPipelineBarrier(" << _commandBuffer
                  << ", VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, "
                     "VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, "
                     "&copyFromBufferMemoryBarrierPost, 0, nullptr);"
                  << std::endl;
  stream.ScopeEnd();
  stream.Indent() << "}" << std::endl;
}

std::set<uint64_t> gits::Vulkan::CGitsInitializeMultipleBuffers::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto obj : _commandBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _copySrcBuffer.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  for (auto obj : _pInitializeBuffers.GetMappedPointers()) {
    returnMap.insert((uint64_t)obj);
  }
  return returnMap;
}

//
// CGitsVkStateRestoreInfo
//

const std::array<gits::Vulkan::ArgInfo, gits::Vulkan::CGitsVkStateRestoreInfo::ARG_NUM>
    gits::Vulkan::CGitsVkStateRestoreInfo::argumentInfos_ = {{
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // Cchar
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}, // Cint
        {gits::Vulkan::ArgType::PRIMITIVE_TYPE, 0, false}  // Cbool
    }};

gits::Vulkan::ArgInfo gits::Vulkan::CGitsVkStateRestoreInfo::ArgumentInfo(unsigned idx) const {
  return argumentInfos_[idx];
}

gits::CArgument& gits::Vulkan::CGitsVkStateRestoreInfo::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, *_phaseInfo, _timerIndex, _timerOn);
}

gits::Vulkan::CGitsVkStateRestoreInfo::CGitsVkStateRestoreInfo()
    : _phaseInfo(std::make_unique<Cchar::CSArray>()), _timerIndex(-1), _timerOn(false) {}

gits::Vulkan::CGitsVkStateRestoreInfo::CGitsVkStateRestoreInfo(const char* phaseInfo)
    : _phaseInfo(std::make_unique<Cchar::CSArray>(phaseInfo, '\0', 1)),
      _timerIndex(-1),
      _timerOn(false) {}

gits::Vulkan::CGitsVkStateRestoreInfo::CGitsVkStateRestoreInfo(const char* phaseInfo, int index)
    : _phaseInfo(std::make_unique<Cchar::CSArray>(phaseInfo, '\0', 1)),
      _timerIndex(index),
      _timerOn(true) {}

void gits::Vulkan::CGitsVkStateRestoreInfo::Run() {
  if (Configurator::Get().vulkan.player.printStateRestoreLogsVk) {
    auto& timersVec = CGits::Instance().Timers().stateRestoreTimers;
    std::string resultText = _phaseInfo->ToString();
    if (!_timerOn.Value()) {
      timersVec.push_back(std::make_unique<Timer>());
    } else {
      timersVec[_timerIndex.Value()]->Pause();
      int64_t timeElapsedNs = timersVec[_timerIndex.Value()]->Get();
      double timeElapsedMs = timeElapsedNs / 1e6;
      resultText += " in " + std::to_string(timeElapsedMs) + " ms";
    }
    Log(INFO) << resultText;
  }
}

void gits::Vulkan::CGitsVkStateRestoreInfo::Write(CBinOStream& stream) const {
  _phaseInfo->Write(stream);
  _timerIndex.Write(stream);
  _timerOn.Write(stream);
}

void gits::Vulkan::CGitsVkStateRestoreInfo::Read(CBinIStream& stream) {
  _phaseInfo->Read(stream);
  _timerIndex.Read(stream);
  _timerOn.Read(stream);
}
