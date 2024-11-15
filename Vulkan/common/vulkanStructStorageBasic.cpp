// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

// ------------------------------------------------------------------------------------------------

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

// ------------------------------------------------------------------------------------------------

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

// ------------------------------------------------------------------------------------------------

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

// ------------------------------------------------------------------------------------------------

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

// ------------------------------------------------------------------------------------------------

void gits::Vulkan::COnQueueSubmitEndDataStorage::SetDataSize(VkDeviceSize dataSize) {
  _dataSize = dataSize;
  if (_dataSize) {
    _data.resize(_dataSize);
  }
}

void gits::Vulkan::COnQueueSubmitEndDataStorage::GatherDataOnQueueSubmitEnd(
    CCommandBufferState* commandBufferState, VkDeviceMemory memory) {
  // When all: _device, _dataSize and _dataMemory members are set,
  // this class instance will add itself to queueSubmitEndMessageReceivers
  // and OnQueueSubmitEnd() will perform memory mapping and data copy.

  if (commandBufferState) {
    _device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;
    commandBufferState->queueSubmitEndMessageReceivers.push_back(this);
  }
  _dataMemory = memory;
}

void gits::Vulkan::COnQueueSubmitEndDataStorage::OnQueueSubmitEnd() {
  if (_device && _dataSize && _dataMemory) {
    mapMemoryAndCopyData(_data.data(), _device, _dataMemory, 0, _dataSize);
  }
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::CVkDeviceOrHostAddressConstKHRData(
    const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
    uint32_t offset,
    uint64_t stride,
    uint32_t count,
    const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _controlData(controlData), _bufferDeviceAddress(), _DeviceOrHostAddressConst(nullptr) {
  if ((deviceorhostaddress.deviceAddress == 0) || (count == 0) || (stride == 0)) {
    return;
  }

  Initialize(deviceorhostaddress, offset, stride, count);
}

void gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::Initialize(
    const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
    uint32_t offset,
    uint64_t stride,
    uint32_t count) {
  SetDataSize(count * stride);
  _offset = offset;

  switch (_controlData.executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    // Store original device address
    _bufferDeviceAddress =
        CBufferDeviceAddressObjectData(deviceorhostaddress.deviceAddress, offset);

    // For substream recording, input data needs to be acquired and stored for further use in
    // a state restoration phase. In case of non-indexed geometry, data acquisition is very
    // simple as it requires just a copy operation.
    if (isSubcaptureBeforeRestorationPhase() && _bufferDeviceAddress._buffer) {
      auto commandBuffer = _controlData.commandBuffer;
      auto commandBufferState = SD()._commandbufferstates[commandBuffer];
      auto device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;
      auto memoryBufferPair =
          createTemporaryBuffer(device, _dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      auto srcBuffer = _bufferDeviceAddress._buffer;
      auto srcOffset = _bufferDeviceAddress._offset + offset;

      injectCopyCommand(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _dataSize, srcBuffer, srcOffset,
                        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
                        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
                        memoryBufferPair.second->bufferHandle, 0, 0, VK_ACCESS_HOST_READ_BIT);

      GatherDataOnQueueSubmitEnd(commandBufferState.get(),
                                 memoryBufferPair.first->deviceMemoryHandle);
    }
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    memcpy(_data.data(), ((uint8_t*)deviceorhostaddress.hostAddress) + offset, _dataSize);
    break;
  }
}

// ------------------------------------------------------------------------------------------------

VkDeviceOrHostAddressConstKHR* gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::Value() {
  if (!_DeviceOrHostAddressConst) {
    _DeviceOrHostAddressConst = std::make_unique<VkDeviceOrHostAddressConstKHR>();
    _DeviceOrHostAddressConst->deviceAddress = 0;

    switch (_controlData.executionSide) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _DeviceOrHostAddressConst->deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      _DeviceOrHostAddressConst->hostAddress = _data.data() + _offset;
      break;
    }
  }
  return _DeviceOrHostAddressConst.get();
}

std::set<uint64_t> gits::Vulkan::CVkDeviceOrHostAddressConstKHRData::GetMappedPointers() {
  return _bufferDeviceAddress.GetMappedPointers();
}

// ------------------------------------------------------------------------------------------------

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
    : CVkDeviceOrHostAddressConstKHRData(), _indexType(indexType) {
  _controlData = controlData;
  _offset = 0;

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
  // (maxVertex - 1) is a workaround for an incorrect behavior of some applications
  _bufferDeviceAddress = CBufferDeviceAddressObjectData(vertexData.deviceAddress,
                                                        std::max(0u, maxVertex - 1) * stride);

  if (_bufferDeviceAddress._offset < 0) {
    CALL_ONCE[] {
      Log(INFO) << "Application uses negative offsets for buffer device addresses!";
    };
  }

  if (isSubcaptureBeforeRestorationPhase()) {
    // The whole vertex data needs to be copied for state restoration purposes. This is done with
    // a compute shader because the source data is provided via a device address and is
    // additionally offset by indices (which are unknown to us). The compute shader performs more
    // or less the same operations as the PrepareIndexedVertexDataOnHost() function.

    // Since we don't know yet how much data would be copied (because we don't know the actual
    // values of indices and a spread between max and min index), we need to prepare storage for
    // maximal potential amount of data. But later on we will know the exact amount (as it is
    // going to be stored by the compute shader).

    // Data copying is performed with an indirect dispatch divided into multiple workgroups, each
    // consisting of multiple local invocations. This done to speed up copying process as a single
    // compute shader may take too much time to copy all the data and GPU may treat it as a hang.
    // So all in all, the first compute shader gathers the information about the data size and
    // offset and also prepares the number of workgroup count (indirect parameters). The second
    // (indirect) compute shader performs the actual copy operation.

    //              DataSize           Offset         Indirect count x/y/z      Vertices
    SetDataSize(sizeof(uint32_t) + sizeof(uint32_t) + 3 * sizeof(uint32_t) +
                std::max(count, maxVertex) * stride);

    auto commandBuffer = _controlData.commandBuffer;
    auto commandBufferState = SD()._commandbufferstates[commandBuffer];
    auto device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;
    auto memoryBufferPair = createTemporaryBuffer(
        device, _dataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    auto dstBuffer = memoryBufferPair.second->bufferHandle;
    auto dstMemory = memoryBufferPair.first->deviceMemoryHandle;
    auto descriptorSet = getTemporaryDescriptorSet(device, *commandBufferState);
    auto pipelineLayout = SD().internalResources.internalPipelines[device].getLayout();

    {
      VkDescriptorBufferInfo bufferInfo = {
          dstBuffer,    // VkBuffer        buffer;
          0,            // VkDeviceSize    offset;
          VK_WHOLE_SIZE // VkDeviceSize    range;
      };

      VkWriteDescriptorSet writeSet = {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // VkStructureType                sType
          nullptr,                                // const void                   * pNext
          descriptorSet,                          // VkDescriptorSet                dstSet
          0,                                      // uint32_t                       dstBinding
          0,                                      // uint32_t                       dstArrayElement
          1,                                      // uint32_t                       descriptorCount
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,      // VkDescriptorType               descriptorType
          nullptr,                                // const VkDescriptorImageInfo  * pImageInfo
          &bufferInfo,                            // const VkDescriptorBufferInfo * pBufferInfo
          nullptr                                 // const VkBufferView           * pTexelBufferView
      };
      drvVk.vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    }

    // Prepare memory barriers, dispatch the compute shaders
    {
      VkMemoryBarrier barrier = {
          VK_STRUCTURE_TYPE_MEMORY_BARRIER,                       // VkStructureType   sType
          nullptr,                                                // const void      * pNext
          VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, // VkAccessFlags     srcAccessMask
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT  // VkAccessFlags     dstAccessMask
      };
      drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr,
                                 0, nullptr);
    }

    {
      VkPipeline prepareIndirectCopyPipeline = VK_NULL_HANDLE;

      switch (indexType) {
      case VK_INDEX_TYPE_UINT16:
        prepareIndirectCopyPipeline = SD().internalResources.internalPipelines[device]
                                          .getPrepareIndirectCopyFor16BitIndexedVerticesPipeline();
        break;
      case VK_INDEX_TYPE_UINT32:
        prepareIndirectCopyPipeline = SD().internalResources.internalPipelines[device]
                                          .getPrepareIndirectCopyFor32BitIndexedVerticesPipeline();
        break;
      default:
        throw std::runtime_error("Invalid index type specified for triangles data!");
      };

      // Look at getPrepareIndirectCopyFor32BitIndexedVerticesShaderModuleSource()
      struct ControlData {
        VkDeviceAddress AddressOfIndices;
        uint32_t Stride;
        uint32_t Count;
        uint32_t FirstVertex;
      } controlData = {indexData.deviceAddress +
                           offset, // For indexed vertices offset is applied to indices
                       (uint32_t)stride, count, firstVertex};

      drvVk.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                              prepareIndirectCopyPipeline);
      drvVk.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
                                    0, 1, &descriptorSet, 0, nullptr);
      drvVk.vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                               sizeof(controlData), &controlData);
      drvVk.vkCmdDispatch(commandBuffer, 1, 1, 1);
    }

    {
      VkBufferMemoryBarrier barrier = {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType sType
          nullptr,                                 // const void    * pNext
          VK_ACCESS_SHADER_WRITE_BIT,              // VkAccessFlags   srcAccessMask
          VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
              VK_ACCESS_SHADER_WRITE_BIT, // VkAccessFlags   dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,        // uint32_t        srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,        // uint32_t        dstQueueFamilyIndex
          dstBuffer,                      // VkBuffer        buffer
          0,                              // VkDeviceSize    offset
          5 * sizeof(uint32_t)            // VkDeviceSize    size
      };
      drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    {
      // Look at getPerformIndirectCopyShaderModuleSource()
      struct ControlData {
        VkDeviceAddress VertexData;
      } controlData = {vertexData.deviceAddress};
      drvVk.vkCmdBindPipeline(
          commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
          SD().internalResources.internalPipelines[device].getPerformIndirectCopyPipeline());
      drvVk.vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                               sizeof(controlData), &controlData);
      drvVk.vkCmdDispatchIndirect(commandBuffer, dstBuffer, 2 * sizeof(uint32_t));
    }

    {
      VkMemoryBarrier barrier = {
          VK_STRUCTURE_TYPE_MEMORY_BARRIER,                       // VkStructureType   sType
          nullptr,                                                // const void      * pNext
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // VkAccessFlags     srcAccessMask
          VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT |
              VK_ACCESS_HOST_READ_BIT // VkAccessFlags     dstAccessMask
      };
      drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr,
                                 0, nullptr);
    }

    if (commandBufferState->currentPipeline != VK_NULL_HANDLE) {
      drvVk.vkCmdBindPipeline(commandBuffer, commandBufferState->currentPipelineBindPoint,
                              commandBufferState->currentPipeline);
    }

    GatherDataOnQueueSubmitEnd(commandBufferState.get(), dstMemory);
  }
}

void gits::Vulkan::CDeviceOrHostAddressAccelerationStructureVertexDataGITSData::OnQueueSubmitEnd() {
  COnQueueSubmitEndDataStorage::OnQueueSubmitEnd();

  if ((_indexType != VK_INDEX_TYPE_NONE_KHR) && _device && _dataSize && _dataMemory) {
    // When vertex data is indexed, acquisition can only be performed via a compute shader. This
    // is caused by the fact that indices can point to arbitrarily large/small offsets within
    // a buffer with vertices. So a compute shader needs to check the minimal and maximal index
    // value and copy data from the found range.
    // Additionally, for the GITS to properly handle the data, the compute shader needs to store
    // actual amount of data that was copied and the calculated offset (minimal index * stride).
    // This is the data that the compute shader stores:
    // - uint dataSize;
    // - uint offset;
    // - uint8_t vertices[];
    auto* basePtr = (uint32_t*)_data.data();

    _dataSize = basePtr[0]; // Data size = stride * ((maxIndex + 1) - minIndex)
    _offset = basePtr[1];   // Offset    = minIndex * stride

    // Remove additional control information (a header) from the acquired data and leave only the
    // actual vertex data. (This is done for cohesion with other cases/non-index data). This
    // additional data is used to control the copy operation and consists of:
    // - data size,
    // - offset,
    // - indirect count x/y/z
    // followed immediately by a vertex data
    std::vector<uint8_t> data(_dataSize);
    memcpy(data.data(), _data.data() + 5 * sizeof(uint32_t), _dataSize);
    _data.swap(data);
  }
}

void gits::Vulkan::CDeviceOrHostAddressAccelerationStructureVertexDataGITSData::
    InitializeIndexedVertexDataOnHost(
        VkDeviceOrHostAddressConstKHR vertexData,
        uint32_t offset, // For indexed vertices offset is applied to indices
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

    SetDataSize(stride * ((maxIndex + 1) - minIndex));
    _offset = stride * minIndex;
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

    SetDataSize(stride * ((maxIndex + 1) - minIndex));
    _offset = stride * minIndex;
    break;
  }
  default:
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  uint8_t* finalVertexPtr = ((uint8_t*)vertexData.hostAddress) + _offset;
  memcpy(_data.data(), finalVertexPtr, _dataSize);
}

// ------------------------------------------------------------------------------------------------
// No const, mainly scratch data

gits::Vulkan::CVkDeviceOrHostAddressKHRData::CVkDeviceOrHostAddressKHRData(
    const VkDeviceOrHostAddressKHR deviceorhostaddress,
    const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _controlData(controlData), _bufferDeviceAddress(), _DeviceOrHostAddress(nullptr) {
  if (deviceorhostaddress.deviceAddress == 0) {
    return;
  }

  switch (_controlData.executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    // Store original device address
    _bufferDeviceAddress = CBufferDeviceAddressObjectData(deviceorhostaddress.deviceAddress);
    break;
  case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
    throw std::runtime_error("Host operations not supported yet!");
    break;
  }
}

VkDeviceOrHostAddressKHR* gits::Vulkan::CVkDeviceOrHostAddressKHRData::Value() {
  if (!_DeviceOrHostAddress) {
    _DeviceOrHostAddress = std::make_unique<VkDeviceOrHostAddressKHR>();
    _DeviceOrHostAddress->deviceAddress = 0;

    switch (_controlData.executionSide) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      _DeviceOrHostAddress->deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      throw std::runtime_error("Host operations not supported yet!");
      break;
    }
  }
  return _DeviceOrHostAddress.get();
}

std::set<uint64_t> gits::Vulkan::CVkDeviceOrHostAddressKHRData::GetMappedPointers() {
  return _bufferDeviceAddress.GetMappedPointers();
}

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkAccelerationStructureGeometryDataKHRData::
    CVkAccelerationStructureGeometryDataKHRData(
        VkGeometryTypeKHR geometryType,
        const VkAccelerationStructureGeometryDataKHR* accelerationstructuregeometrydatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _AccelerationStructureGeometryDataKHR(nullptr),
      _isNullPtr(accelerationstructuregeometrydatakhr == nullptr) {
  if (*_isNullPtr) {
    return;
  }

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

// ------------------------------------------------------------------------------------------------

gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHRData::
    CVkAccelerationStructureGeometryInstancesDataKHRData(
        const VkAccelerationStructureGeometryInstancesDataKHR*
            accelerationstructuregeometryinstancesdatakhr,
        const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
        const VkAccelerationStructureBuildControlDataGITS& controlData)
    : _sType(VK_STRUCTURE_TYPE_MAX_ENUM),
      _pNext(),
      _arrayOfPointers(false),
      _bufferDeviceAddress(),
      _AccelerationStructureGeometryInstancesDataKHR(),
      _controlData(controlData),
      _pointers(),
      _baseIn(),
      _isNullPtr(accelerationstructuregeometryinstancesdatakhr == nullptr) {
  if (*_isNullPtr || (buildRangeInfo.primitiveCount == 0)) {
    return;
  }

  SetDataSize(buildRangeInfo.primitiveCount * sizeof(VkAccelerationStructureInstanceKHR));

  _sType = accelerationstructuregeometryinstancesdatakhr->sType;
  _pNext =
      std::make_unique<CpNextWrapperData>(accelerationstructuregeometryinstancesdatakhr->pNext);
  _arrayOfPointers = accelerationstructuregeometryinstancesdatakhr->arrayOfPointers;

  switch (_controlData.executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS: {
    _bufferDeviceAddress = CBufferDeviceAddressObjectData(
        accelerationstructuregeometryinstancesdatakhr->data.deviceAddress,
        buildRangeInfo.primitiveOffset);

    auto baseAddress = accelerationstructuregeometryinstancesdatakhr->data.deviceAddress +
                       buildRangeInfo.primitiveOffset;
    auto& commandBufferState = SD()._commandbufferstates[controlData.commandBuffer];

    if (!useCaptureReplayFeaturesForBuffersAndAccelerationStructures()) {
      // Prepare data for device address patching
      // Multiple AS builds can occur in a single command buffer and many of them may need
      // address patching. But, additionally, a single AS build will be performed also in
      // a state restore phase and it also requires device patching but only for a dedicated
      // set of device addresses.

      auto& sharedPatcher = commandBufferState->addressPatchers[controlData.buildCommandIndex];
      uint32_t addressOffset =
          offsetof(VkAccelerationStructureInstanceKHR, accelerationStructureReference);

      uint32_t structureSize = sizeof(VkAccelerationStructureInstanceKHR);
      if (_arrayOfPointers) {
        // Device address points to an array of device addresses referencing individual VkAccelerationStructureInstanceKHR structures

        // Update acceleration structure references (device addresses)
        for (uint32_t i = 0; i < buildRangeInfo.primitiveCount; ++i) {
          sharedPatcher.AddIndirectAddress(baseAddress + i * structureSize, addressOffset);
          individualPatcher.AddIndirectAddress(baseAddress + i * structureSize, addressOffset);
        }

        // Update device addresses pointing to VkAccelerationStructureInstanceKHR structures
        for (uint32_t i = 0; i < buildRangeInfo.primitiveCount; ++i) {
          sharedPatcher.AddDirectAddress(baseAddress + i * structureSize);
          individualPatcher.AddDirectAddress(baseAddress + i * structureSize);
        }
      } else {
        // Device address points to an array of VkAccelerationStructureInstanceKHR structures

        // Update acceleration structure references (device addresses)
        for (uint32_t i = 0; i < buildRangeInfo.primitiveCount; ++i) {
          sharedPatcher.AddDirectAddress(baseAddress + i * structureSize + addressOffset);
          individualPatcher.AddDirectAddress(baseAddress + i * structureSize + addressOffset);
        }
      }
    }

    if (isSubcaptureBeforeRestorationPhase() && _bufferDeviceAddress._buffer) {
      // Acquire instance data
      auto commandBuffer = controlData.commandBuffer;
      auto commandBufferState = SD()._commandbufferstates[commandBuffer];
      auto device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;
      auto memoryBufferPair =
          createTemporaryBuffer(device, _dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      auto srcBuffer = _bufferDeviceAddress._buffer;
      auto srcOffset = _bufferDeviceAddress._offset;

      if (_arrayOfPointers) {
        // Submit a compute shader which copies structures
        throw std::runtime_error("Array of pointers not implemented yet!");
      } else if (_bufferDeviceAddress._buffer) {
        // Copy data with a just a usual copy command
        injectCopyCommand(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _dataSize, srcBuffer, srcOffset,
                          VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
                          VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
                          memoryBufferPair.second->bufferHandle, 0, 0, VK_ACCESS_HOST_READ_BIT);
      }

      GatherDataOnQueueSubmitEnd(commandBufferState.get(),
                                 memoryBufferPair.first->deviceMemoryHandle);
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

  // Pass data to "vulkan arguments" class (see CVkAccelerationStructureGeometryInstancesDataKHR() constructor)
  if (!isSubcaptureBeforeRestorationPhase()) {
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS,        // VkStructureType sType;
        accelerationstructuregeometryinstancesdatakhr->pNext, // const void* pNext;
        this                                                  // const void* pStructStorage;
    };
    const_cast<VkAccelerationStructureGeometryInstancesDataKHR*>(
        accelerationstructuregeometryinstancesdatakhr)
        ->pNext = &_baseIn;
  }
}

VkAccelerationStructureGeometryInstancesDataKHR* gits::Vulkan::
    CVkAccelerationStructureGeometryInstancesDataKHRData::Value() {
  if (*_isNullPtr) {
    return nullptr;
  }
  if (_AccelerationStructureGeometryInstancesDataKHR == nullptr) {
    VkDeviceOrHostAddressConstKHR address = {0};

    switch (_controlData.executionSide) {
    case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
      address.deviceAddress = _bufferDeviceAddress._originalDeviceAddress;
      break;
    case VK_COMMAND_EXECUTION_SIDE_HOST_GITS:
      if (_arrayOfPointers) {
        _pointers.resize(_dataSize / sizeof(VkAccelerationStructureInstanceKHR));
        for (uint32_t i = 0; i < _dataSize; i += sizeof(VkAccelerationStructureInstanceKHR)) {
          _pointers[i] = &_data[i];
        }
        address.hostAddress = _pointers.data();
      } else {
        address.hostAddress = _data.data();
      }
      break;
    }

    // Pass this structure through pNext
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
        **_pNext,                                      // const void* pNext;
        this                                           // const void* pStructStorage;
    };

    _AccelerationStructureGeometryInstancesDataKHR =
        std::make_unique<VkAccelerationStructureGeometryInstancesDataKHR>();
    _AccelerationStructureGeometryInstancesDataKHR->sType = _sType;
    _AccelerationStructureGeometryInstancesDataKHR->pNext = &_baseIn;
    _AccelerationStructureGeometryInstancesDataKHR->arrayOfPointers = _arrayOfPointers;
    _AccelerationStructureGeometryInstancesDataKHR->data = address;
  }
  return _AccelerationStructureGeometryInstancesDataKHR.get();
}

std::set<uint64_t> gits::Vulkan::CVkAccelerationStructureGeometryInstancesDataKHRData::
    GetMappedPointers() {
  switch (_controlData.executionSide) {
  case VK_COMMAND_EXECUTION_SIDE_DEVICE_GITS:
    return _bufferDeviceAddress.GetMappedPointers();
  default:
    return {};
  }
}
