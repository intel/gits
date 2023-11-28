// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLog.h"
#include "vulkanStateTracking.h"

gits::Vulkan::CBinaryResourceData::CBinaryResourceData(TResourceType _,
                                                       const void* data,
                                                       size_t size)
    : _data((uint8_t*)data, (uint8_t*)data + size) {}

gits::Vulkan::CBinaryResourceData::PointerProxy gits::Vulkan::CBinaryResourceData::Data() const {
  return PointerProxy(_data.data(), _data.size());
}

gits::Vulkan::CVkGenericArgumentData::CVkGenericArgumentData(const void* vkgenericargumentdata)
    : _argument(nullptr) {
  vkgenericargumentdata = ignoreLoaderSpecificStructureTypes(vkgenericargumentdata);

  if (vkgenericargumentdata) {
    switch (*(VkStructureType*)vkgenericargumentdata) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    _argument = std::make_unique<C##structure##Data>((structure*)vkgenericargumentdata);           \
    break;

#include "vulkanPNextWrappers.inl"

    default:
      VkLog(ERR) << "Unknown enum value: " << *(VkStructureType*)vkgenericargumentdata
                 << " for CVkGenericArgumentData";
      const void* pNext = ((const VkBaseInStructure*)vkgenericargumentdata)->pNext;
      while (pNext != nullptr) {
        VkLog(WARN) << "Additional structure pointed to by pNext: " << *(VkStructureType*)pNext;
        pNext = ((const VkBaseInStructure*)pNext)->pNext;
      }
      break;
    }
  }
}

const void* gits::Vulkan::CVkGenericArgumentData::Value() {
  if (!_argument) {
    return nullptr;
  } else {
    return _argument->GetPtrType();
  }
}

gits::Vulkan::CVkClearColorValueData::CVkClearColorValueData(
    const VkClearColorValue* clearcolorvalue)
    : _ClearColorValue(nullptr), _isNullPtr(clearcolorvalue == nullptr) {
  if (!*_isNullPtr) {
    _uint32 = std::make_unique<Cuint32_tDataArray>(4, clearcolorvalue->uint32);
  } else {
    _uint32 = nullptr;
  }
}

VkClearColorValue* gits::Vulkan::CVkClearColorValueData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearColorValue == nullptr) {
    _ClearColorValue = std::make_unique<VkClearColorValue>();
    auto uint32Values = **_uint32;
    if (uint32Values != nullptr) {
      for (int i = 0; i < 4; i++) {
        _ClearColorValue->uint32[i] = uint32Values[i];
      }
    } else {
      Log(ERR) << "The pointer to array **_uint32 is null.";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  return _ClearColorValue.get();
}

gits::Vulkan::CVkClearValueData::CVkClearValueData(const VkClearValue* clearvalue)
    : _ClearValue(nullptr), _isNullPtr(clearvalue == nullptr) {
  if (!*_isNullPtr) {
    _color = std::make_unique<CVkClearColorValueData>(&clearvalue->color);
  } else {
    _color = nullptr;
  }
}

VkClearValue* gits::Vulkan::CVkClearValueData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_ClearValue == nullptr) {
    _ClearValue = std::make_unique<VkClearValue>();
    _ClearValue->color = **_color;
  }
  return _ClearValue.get();
}

gits::Vulkan::CBufferDeviceAddressObjectData::CBufferDeviceAddressObjectData(
    VkDeviceAddress originalDeviceAddress, int64_t additionalOffset)
    : _buffer(VK_NULL_HANDLE), _offset(0) {
  _originalDeviceAddress = originalDeviceAddress;

  if ((originalDeviceAddress != 0) &&
      (!useCaptureReplayFeaturesForBuffersAndAccelerationStructures() ||
       isSubcaptureBeforeRestorationPhase())) {
    _buffer = findBufferFromDeviceAddress(originalDeviceAddress + additionalOffset);
    if (_buffer) {
      _offset = originalDeviceAddress - SD()._bufferstates[_buffer]->deviceAddress;
    }
  }
}

gits::Vulkan::CBufferDeviceAddressObjectData& gits::Vulkan::CBufferDeviceAddressObjectData::
operator=(CBufferDeviceAddressObjectData&& other) noexcept {
  if (this != &other) {
    _originalDeviceAddress = other._originalDeviceAddress;
    _buffer = other._buffer;
    _offset = other._offset;
  }

  return *this;
}

std::set<uint64_t> gits::Vulkan::CBufferDeviceAddressObjectData::GetMappedPointers() {
  std::set<uint64_t> returnMap;
  if (_buffer) {
    returnMap = SD()._bufferstates[_buffer]->GetMappedPointers();
  }
  return returnMap;
}

gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::CVkDeviceOrHostAddressConstKHRData(
    const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
    uint32_t offset,
    uint64_t stride,
    uint32_t count,
    const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _dataSize(count * stride),
      _controlData(controlData),
      _bufferDeviceAddress(),
      _hostOffset(0),
      _inputData(),
      _DeviceOrHostAddressConst(nullptr),
      _tmpBuffer(VK_NULL_HANDLE),
      _tmpMemory(VK_NULL_HANDLE) {
  if ((deviceorhostaddress.deviceAddress == 0) || (count == 0) || (stride == 0)) {
    return;
  }

  Initialize(deviceorhostaddress, offset, stride, count);

  auto hash = prepareStateTrackingHash(_controlData, deviceorhostaddress.deviceAddress, offset,
                                       stride, count);
  SD()._accelerationstructurekhrstates[_controlData.accelerationStructure]
      ->stateTrackingHashMap[hash] = this;
}

void gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::Initialize(
    const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
    uint32_t offset,
    uint64_t stride,
    uint32_t count) {
  _inputData.resize(_dataSize);

  switch (_controlData.executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    // Store original device address
    _bufferDeviceAddress =
        CBufferDeviceAddressObjectData(deviceorhostaddress.deviceAddress, offset);

    // In case of substream recording, input data needs to be acquired and stored
    // for further use during state restoration
    if (isSubcaptureBeforeRestorationPhase() && _bufferDeviceAddress._buffer) {
      auto commandBuffer = _controlData.commandBuffer;
      auto commandBufferState = SD()._commandbufferstates[commandBuffer];
      auto device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;
      auto memoryBufferPair =
          createTemporaryBuffer(device, _dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      auto srcBuffer = _bufferDeviceAddress._buffer;
      VkDeviceSize srcBufferOffset = _bufferDeviceAddress._offset + offset;
      _tmpMemory = memoryBufferPair.first->deviceMemoryHandle;
      _tmpBuffer = memoryBufferPair.second->bufferHandle;

      {
        std::vector<VkBufferMemoryBarrier> preBarriers = {
            VkBufferMemoryBarrier{
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                nullptr,                                 // const void* pNext;
                VK_ACCESS_MEMORY_WRITE_BIT,              // VkAccessFlags srcAccessMask;
                VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags dstAccessMask;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                srcBuffer,                               // VkBuffer buffer;
                srcBufferOffset,                         // VkDeviceSize offset;
                _dataSize                                // VkDeviceSize size;
            },
            VkBufferMemoryBarrier{
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                nullptr,                                 // const void* pNext;
                0,                                       // VkAccessFlags srcAccessMask;
                VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags dstAccessMask;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                _tmpBuffer,                              // VkBuffer buffer;
                0,                                       // VkDeviceSize offset;
                _dataSize                                // VkDeviceSize size;
            }};

        drvVk.vkCmdPipelineBarrier(_controlData.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                   static_cast<uint32_t>(preBarriers.size()), preBarriers.data(), 0,
                                   nullptr);
      }
      {
        VkBufferCopy region = {
            srcBufferOffset, // VkDeviceSize srcOffset;
            0,               // VkDeviceSize dstOffset;
            _dataSize        // VkDeviceSize size;
        };
        drvVk.vkCmdCopyBuffer(commandBuffer, srcBuffer, _tmpBuffer, 1, &region);
      }
      {
        std::vector<VkBufferMemoryBarrier> postBarriers = {
            VkBufferMemoryBarrier{
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                nullptr,                                 // const void* pNext;
                VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags srcAccessMask;
                VK_ACCESS_MEMORY_WRITE_BIT,              // VkAccessFlags dstAccessMask;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                srcBuffer,                               // VkBuffer buffer;
                srcBufferOffset,                         // VkDeviceSize offset;
                _dataSize                                // VkDeviceSize size;
            },
            VkBufferMemoryBarrier{
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType;
                nullptr,                                 // const void* pNext;
                VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags srcAccessMask;
                VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags dstAccessMask;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t srcQueueFamilyIndex;
                VK_QUEUE_FAMILY_IGNORED,                 // uint32_t dstQueueFamilyIndex;
                _tmpBuffer,                              // VkBuffer buffer;
                0,                                       // VkDeviceSize offset;
                _dataSize                                // VkDeviceSize size;
            }};

        drvVk.vkCmdPipelineBarrier(_controlData.commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                                   static_cast<uint32_t>(postBarriers.size()), postBarriers.data(),
                                   0, nullptr);
      }
    }
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    memcpy(_inputData.data(), ((uint8_t*)deviceorhostaddress.hostAddress) + offset, _dataSize);
    _hostOffset = offset;
    break;
  }
}

VkDeviceOrHostAddressConstKHR* gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::Value() {
  if (!_DeviceOrHostAddressConst) {
    _DeviceOrHostAddressConst = std::make_unique<VkDeviceOrHostAddressConstKHR>();
    _DeviceOrHostAddressConst->deviceAddress = 0;

    switch (_controlData.executionSide) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _DeviceOrHostAddressConst->deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      _DeviceOrHostAddressConst->hostAddress = _inputData.data() - _hostOffset;
      break;
    }
  }
  return _DeviceOrHostAddressConst.get();
}

std::set<uint64_t> gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::GetMappedPointers() {
  return _bufferDeviceAddress.GetMappedPointers();
}

void gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::OnQueueSubmitEnd() {
  if (isSubcaptureBeforeRestorationPhase() && _bufferDeviceAddress._buffer) {
    auto device = SD()._commandbufferstates[_controlData.commandBuffer]
                      ->commandPoolStateStore->deviceStateStore->deviceHandle;

    mapMemoryAndCopyData(_inputData.data(), device, _tmpMemory, 0, _dataSize);
  }
}

gits::Vulkan::CDeviceOrHostAddressAccelerationStructureVertexDataGITSData::
    CDeviceOrHostAddressAccelerationStructureVertexDataGITSData(
        VkDeviceOrHostAddressConstKHR vertexData,
        uint32_t offset,
        uint64_t stride,
        uint32_t count,
        uint32_t firstVertex,
        uint32_t maxVertex,
        VkDeviceOrHostAddressConstKHR indexData,
        VkIndexType indexType,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : CVkDeviceOrHostAddressConstKHRData() {
  _dataSize = count * stride;
  _controlData = controlData;

  if ((vertexData.deviceAddress == 0) || (count == 0) || (stride == 0)) {
    return;
  }

  if (indexType == VK_INDEX_TYPE_NONE_KHR) {
    // Non-indexed geometry, buffer with vertex data can be retrieved directly from a device address
    Initialize(vertexData, offset + firstVertex * stride, stride, count);
  } else {
    // Indexed geometry, indices alter/offset device address before fetching vertices,
    // so indices need to be read first in order to find a source buffer with vertex data
    if (_controlData.executionSide == VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS) {
      InitializeIndexedVertexDataOnDevice(vertexData, offset, stride, count, firstVertex, maxVertex,
                                          indexData, indexType);
    } else {
      InitializeIndexedVertexDataOnHost(vertexData, offset, stride, count, firstVertex, indexData,
                                        indexType);
    }
  }

  SD()._commandbufferstates[_controlData.commandBuffer]->queueSubmitEndMessageReceivers.push_back(
      this);

  auto hash =
      prepareStateTrackingHash(_controlData, vertexData.deviceAddress, offset, stride, count);
  SD()._accelerationstructurekhrstates[_controlData.accelerationStructure]
      ->stateTrackingHashMap[hash] = this;
}

void gits::Vulkan::CDeviceOrHostAddressAccelerationStructureVertexDataGITSData::
    InitializeIndexedVertexDataOnDevice(VkDeviceOrHostAddressConstKHR vertexData,
                                        uint32_t offset,
                                        uint64_t stride,
                                        uint32_t count,
                                        uint32_t firstVertex,
                                        uint32_t maxVertex,
                                        VkDeviceOrHostAddressConstKHR indexData,
                                        VkIndexType indexType) {
  _bufferDeviceAddress =
      CBufferDeviceAddressObjectData(vertexData.deviceAddress, maxVertex * stride);

  if (_bufferDeviceAddress._offset < 0) {
    CALL_ONCE[] {
      Log(INFO) << "Application uses negative offsets for buffer device addresses!";
    };
  }

  if (isSubcaptureBeforeRestorationPhase()) {
    // The whole vertex data needs to be copied for state restoration purposes.
    // This is done with a compute shader because the source data is provided
    // via a device address. The compute shader performs more or less the same
    // operations as the PrepareIndexedVertexDataOnHost() function.

    TODO("Implement state restoration for input vertex data during AS building.")
  }
}

void gits::Vulkan::CDeviceOrHostAddressAccelerationStructureVertexDataGITSData::
    InitializeIndexedVertexDataOnHost(VkDeviceOrHostAddressConstKHR vertexData,
                                      uint32_t offset,
                                      uint64_t stride,
                                      uint32_t count,
                                      uint32_t firstVertex,
                                      VkDeviceOrHostAddressConstKHR indexData,
                                      VkIndexType indexType) {

  switch (indexType) {
  case VK_INDEX_TYPE_UINT16: {
    uint16_t* baseIndex = (uint16_t*)(((uint8_t*)indexData.hostAddress) + offset);
    uint16_t maxIndex = baseIndex[0] + firstVertex;
    uint16_t minIndex = baseIndex[0] + firstVertex;

    for (uint32_t i = 0; i < count; ++i) {
      uint16_t index = baseIndex[i] + firstVertex;
      maxIndex = index > maxIndex ? index : maxIndex;
      minIndex = index < minIndex ? index : minIndex;
    }

    _dataSize = stride * (maxIndex - minIndex);
    _hostOffset = stride * minIndex;
    break;
  }
  case VK_INDEX_TYPE_UINT32: {
    uint32_t* baseIndex = (uint32_t*)(((uint8_t*)indexData.hostAddress) + offset);
    uint32_t maxIndex = baseIndex[0] + firstVertex;
    uint32_t minIndex = baseIndex[0] + firstVertex;

    for (uint32_t i = 0; i < count; ++i) {
      uint16_t index = baseIndex[i] + firstVertex;
      maxIndex = index > maxIndex ? index : maxIndex;
      minIndex = index < minIndex ? index : minIndex;
    }

    _dataSize = stride * (maxIndex - minIndex);
    _hostOffset = stride * minIndex;
    break;
  }
  default:
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  uint8_t* finalVertexPtr = ((uint8_t*)vertexData.hostAddress) + _hostOffset;
  _inputData.resize(_dataSize);
  memcpy(_inputData.data(), finalVertexPtr, _dataSize);
}

void gits::Vulkan::CDeviceOrHostAddressAccelerationStructureVertexDataGITSData::OnQueueSubmitEnd() {
  if (isSubcaptureBeforeRestorationPhase()) {

    TODO("Implement data gathering for the state restoration.")
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::
    CVkAccelerationStructureGeometryDataKHRData(
        VkGeometryTypeKHR geometryType,
        const VkAccelerationStructureGeometryDataKHR* accelerationstructuregeometrydatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _AccelerationStructureGeometryDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometrydatakhr == nullptr) {
  if (!*_isNullPtr) {
    _geometryType = std::make_unique<CVkGeometryTypeKHRData>(geometryType);
    switch (geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _triangles = std::make_unique<CVkAccelerationStructureGeometryTrianglesDataKHRData>(
          &accelerationstructuregeometrydatakhr->triangles, buildRangeInfo, controlData);
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _aabbs = std::make_unique<CVkAccelerationStructureGeometryAabbsDataKHRData>(
          &accelerationstructuregeometrydatakhr->aabbs, buildRangeInfo, controlData);
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _instances = std::make_unique<CVkAccelerationStructureGeometryInstancesDataKHRData>(
          &accelerationstructuregeometrydatakhr->instances, buildRangeInfo, controlData);
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
}

VkAccelerationStructureGeometryDataKHR* gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::
    Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_AccelerationStructureGeometryDataKHR == nullptr) {
    _AccelerationStructureGeometryDataKHR =
        std::make_unique<VkAccelerationStructureGeometryDataKHR>();
    switch (**_geometryType) {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
      _AccelerationStructureGeometryDataKHR->triangles = **_triangles;
      break;
    case VK_GEOMETRY_TYPE_AABBS_KHR:
      _AccelerationStructureGeometryDataKHR->aabbs = **_aabbs;
      break;
    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
      _AccelerationStructureGeometryDataKHR->instances = ***_instances;
      break;
    default:
      throw std::runtime_error("Unknown geometry type provided!");
      break;
    }
  }
  return _AccelerationStructureGeometryDataKHR.get();
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::GetMappedPointers() {
  switch (**_geometryType) {
  case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
    return _triangles->GetMappedPointers();
  case VK_GEOMETRY_TYPE_AABBS_KHR:
    return _aabbs->GetMappedPointers();
  case VK_GEOMETRY_TYPE_INSTANCES_KHR:
    return _instances->GetMappedPointers();
  default:
    throw std::runtime_error("Unknown geometry type provided!");
  }
}

gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHRData::
    CVkAccelerationStructureGeometryInstancesDataKHRData(
        const VkAccelerationStructureGeometryInstancesDataKHR*
            accelerationstructuregeometryinstancesdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _sType(VK_STRUCTURE_TYPE_MAX_ENUM),
      _pNext(),
      _executionSide(VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS),
      _arrayOfPointers(false),
      _bufferDeviceAddress(),
      _inputData(),
      _pointers(),
      _AccelerationStructureGeometryInstancesDataKHR(),
      _isNullPtr(accelerationstructuregeometryinstancesdatakhr == nullptr) {
  if (*_isNullPtr || (buildRangeInfo.primitiveCount == 0)) {
    return;
  }

  _sType = accelerationstructuregeometryinstancesdatakhr->sType;
  _pNext =
      std::make_unique<CpNextWrapperData>(accelerationstructuregeometryinstancesdatakhr->pNext);
  _executionSide = controlData.executionSide;
  _arrayOfPointers = accelerationstructuregeometryinstancesdatakhr->arrayOfPointers;

  switch (_executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress = CBufferDeviceAddressObjectData(
        accelerationstructuregeometryinstancesdatakhr->data.deviceAddress,
        buildRangeInfo.primitiveOffset);

    auto baseAddress = accelerationstructuregeometryinstancesdatakhr->data.deviceAddress +
                       buildRangeInfo.primitiveOffset;
    auto& commandBufferState = SD()._commandbufferstates[controlData.commandBuffer];

    if (!useCaptureReplayFeaturesForBuffersAndAccelerationStructures()) {
      // Prepare data for device address patching

      auto& addressPatcher = commandBufferState->addressPatchers[controlData.buildCommandIndex];
      uint32_t addressOffset =
          offsetof(VkAccelerationStructureInstanceKHR, accelerationStructureReference);

      uint32_t structureSize = sizeof(VkAccelerationStructureInstanceKHR);
      if (_arrayOfPointers) {
        // Device address points to an array of device addresses referencing individual VkAccelerationStructureInstanceKHR structures

        // Update acceleration structure references (device addresses)
        for (uint32_t i = 0; i < buildRangeInfo.primitiveCount; ++i) {
          addressPatcher.AddIndirectAddress(baseAddress + i * structureSize, addressOffset);
        }

        // Update device addresses pointing to VkAccelerationStructureInstanceKHR structures
        for (uint32_t i = 0; i < buildRangeInfo.primitiveCount; ++i) {
          addressPatcher.AddDirectAddress(baseAddress + i * structureSize);
        }
      } else {
        // Device address points to an array of VkAccelerationStructureInstanceKHR structures

        // Update acceleration structure references (device addresses)
        for (uint32_t i = 0; i < buildRangeInfo.primitiveCount; ++i) {
          addressPatcher.AddDirectAddress(baseAddress + i * structureSize + addressOffset);
        }
      }
    }

    if (isSubcaptureBeforeRestorationPhase()) {
      // Acquire instance data
      _inputData.resize(buildRangeInfo.primitiveCount);
      uint32_t dataSize = _inputData.size() * sizeof(VkAccelerationStructureInstanceKHR);

      auto memoryBufferPair = createTemporaryBuffer(
          commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle, dataSize,
          VK_BUFFER_USAGE_TRANSFER_DST_BIT, commandBufferState.get(),
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _inputData.data());

      if (_arrayOfPointers) {
        // Submit a compute shader which copies structures
        throw std::runtime_error("Array of pointers not implemented yet!");
      } else if (_bufferDeviceAddress._buffer) {
        // Simple copy operation
        VkBufferCopy region = {
            static_cast<VkDeviceSize>(_bufferDeviceAddress._offset), // VkDeviceSize srcOffset;
            0,                                                       // VkDeviceSize dstOffset;
            dataSize                                                 // VkDeviceSize size;
        };
        drvVk.vkCmdCopyBuffer(controlData.commandBuffer, _bufferDeviceAddress._buffer,
                              memoryBufferPair.second->bufferHandle, 1, &region);
      }
    }
    break;
  }
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS: {
    // Leave it for now, it's easy to implement but not used by existing apps

    throw std::runtime_error(
        "Building top-level acceleration structures on host is not supported yet.");
    break;
  }
  }

  auto hash = prepareStateTrackingHash(
      prepareAccelerationStructureControlData(controlData,
                                              accelerationstructuregeometryinstancesdatakhr->sType),
      accelerationstructuregeometryinstancesdatakhr->data.deviceAddress,
      buildRangeInfo.primitiveOffset, sizeof(VkAccelerationStructureInstanceKHR),
      buildRangeInfo.primitiveCount);
  SD()._accelerationstructurekhrstates[controlData.accelerationStructure]
      ->stateTrackingHashMap[hash] = this;
}

VkAccelerationStructureGeometryInstancesDataKHR* gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHRData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_AccelerationStructureGeometryInstancesDataKHR == nullptr) {
    VkDeviceOrHostAddressConstKHR address = {0};

    switch (_executionSide) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      address.deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      if (_arrayOfPointers) {
        _pointers.resize(_inputData.size());
        for (uint32_t i = 0; i < _inputData.size(); ++i) {
          _pointers[i] = &_inputData[i];
        }
        address.hostAddress = _pointers.data();
      } else {
        address.hostAddress = _inputData.data();
      }
      break;
    }

    _AccelerationStructureGeometryInstancesDataKHR =
        std::make_unique<VkAccelerationStructureGeometryInstancesDataKHR>();
    _AccelerationStructureGeometryInstancesDataKHR->sType = _sType;
    _AccelerationStructureGeometryInstancesDataKHR->pNext = **_pNext;
    _AccelerationStructureGeometryInstancesDataKHR->arrayOfPointers = _arrayOfPointers;
    _AccelerationStructureGeometryInstancesDataKHR->data = address;
  }
  return _AccelerationStructureGeometryInstancesDataKHR.get();
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHRData::
    GetMappedPointers() {
  switch (_executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    return _bufferDeviceAddress.GetMappedPointers();
  default:
    return {};
  }
}
