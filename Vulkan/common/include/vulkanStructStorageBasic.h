// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "vulkanHeader.h"
#include "resource_manager.h"
#include <vector>
#include <memory>
#include <set>
#include "exception.h"

namespace gits {
namespace Vulkan {
template <class T1, class WRAP_T1>
class CDataArray {
public:
  typedef T1 TKey;
  typedef WRAP_T1 TKeyArg;

private:
  std::vector<std::shared_ptr<TKeyArg>> _cargsDict;
  std::vector<TKey> _data;

public:
  CDataArray() {}
  CDataArray(size_t size, const TKey* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    for (size_t i = 0; i < size; i++) {
      auto obj = std::make_shared<TKeyArg>(&dictionary[i]);
      _cargsDict.push_back(obj);
    }
  }
  void AddElem(const TKey* elem) {
    if (elem == NULL) {
      return;
    }

    auto obj = std::make_shared<TKeyArg>(elem);
    _cargsDict.push_back(obj);
  }
  template <class WRAP_T2>
  CDataArray(size_t size, const TKey* dictionary, const WRAP_T2 arg3) {
    if (dictionary == NULL) {
      return;
    }

    for (size_t i = 0; i < size; i++) {
      auto obj = std::make_shared<TKeyArg>(&dictionary[i], arg3);
      _cargsDict.push_back(obj);
    }
  }
  CDataArray(size_t size, TKey* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    for (size_t i = 0; i < size; i++) {
      auto obj = std::make_shared<TKeyArg>(&dictionary[i]);
      _cargsDict.push_back(obj);
    }
  }
  std::vector<std::shared_ptr<TKeyArg>>& Vector() {
    return _cargsDict;
  }
  const std::vector<std::shared_ptr<TKeyArg>>& Vector() const {
    return _cargsDict;
  }

  TKey* Value() {
    if (_cargsDict.size() == 0) {
      return 0;
    }
    _data.clear();
    for (unsigned i = 0; i < _cargsDict.size(); i++) {
      _data.push_back(**_cargsDict[i]);
    }
    return &_data[0];
  }
  uint64_t size() {
    return _cargsDict.size();
  }
  TKey* operator*() {
    return Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;

    for (unsigned i = 0; i < _cargsDict.size(); i++) {
      for (uint64_t elem : _cargsDict[i]->GetMappedPointers()) {
        pointers.insert(elem);
      }
    }
    return pointers;
  }
};

class CNullWrapperData {
  const void* _ptr;

public:
  CNullWrapperData(const void* srcpNext) {
    if (srcpNext == NULL) {
      _ptr = NULL;
    }
  }
  const void** Value() {
    return &_ptr;
  }
  struct PtrConverter {
  private:
    void* _ptr;

  public:
    explicit PtrConverter(void* ptr) : _ptr(ptr) {}
    operator void*() const {
      return _ptr;
    }
    operator const void*() const {
      return _ptr;
    }
    operator const VkAllocationCallbacks*() const {
      return (VkAllocationCallbacks*)_ptr;
    }
    operator const VkSpecializationInfo*() const {
      return (VkSpecializationInfo*)_ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter((void*)_ptr);
  }
};

class CBinaryResourceData {
protected:
  std::vector<uint8_t> _data;

public:
  CBinaryResourceData(TResourceType _, const void* data, size_t size);
  struct PointerProxy {
    PointerProxy() : _ptr(0) {}
    explicit PointerProxy(const void* ptr, size_t size) : _ptr(ptr), _size(size) {}
    template <class T>
    operator const T*() const {
      return (const T*)_ptr;
    }
    operator const void*() const {
      return _ptr;
    }
    size_t Size() const {
      return _size;
    }

  private:
    const void* _ptr;
    size_t _size;
  };
  PointerProxy Data() const;
  PointerProxy Value() {
    return Data();
  }
  PointerProxy operator*() {
    return Data();
  }
};

class CDeclaredBinaryResourceData : public CBinaryResourceData {
  using CBinaryResourceData::CBinaryResourceData; // Inherit the constructor.
};

template <class T>
class CSimpleData {
private:
  T data;

public:
  CSimpleData() {}
  CSimpleData(const T srcData) {
    data = srcData;
  }
  CSimpleData(const T* srcData) {
    data = *srcData;
  }
  T Value() {
    return data;
  }

  T operator*() {
    return Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

template <class T>
class CSimpleMappedData {
private:
  T data;

public:
  CSimpleMappedData() {}
  CSimpleMappedData(const T srcData) {
    data = srcData;
  }
  CSimpleMappedData(const T* srcData) {
    data = *srcData;
  }
  T Value() {
    return data;
  }
  T operator*() {
    return Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    pointers.insert((uint64_t)data);
    return pointers;
  }
};
/*  template<class T>
    class CSimpleNonDispData
    {
    private:
      uint64_t handle;
      T data;
    public:
      CSimpleNonDispData() {}
      CSimpleNonDispData(const T srcData)
      {
        handle = srcData.handle;
      }
      CSimpleNonDispData(const T* srcData)
      {
        handle = srcData->handle;
      }
      T* Value()
      {
        data = T(handle);
        return &data;
      }

      PtrConverter<T> operator*() {
        return PtrConverter<T>(Value());
      }
    };*/

typedef CSimpleData<bool> CboolData;
typedef CSimpleData<std::uint8_t> Cuint8_tData;
typedef CSimpleData<std::uint16_t> Cuint16_tData;
typedef CSimpleData<std::uint32_t> Cuint32_tData;
typedef CSimpleData<std::uint64_t> Cuint64_tData;
typedef CSimpleData<std::int32_t> Cint32_tData;
typedef CSimpleData<std::int64_t> Cint64_tData;
typedef CSimpleData<float> CfloatData;
typedef CSimpleData<size_t> Csize_tData;
typedef CSimpleData<PFN_vkAllocationFunction> CPFN_vkAllocationFunctionData;
typedef CSimpleData<PFN_vkFreeFunction> CPFN_vkFreeFunctionData;
typedef CSimpleData<void*> CvoidPtrData;
//typedef CSimpleData<const void*> CVkGenericArgumentData; //TODO

class CcharDataArray : gits::noncopyable {
  std::string* stringData;
  const char* outData;
  CboolData _isNullPtr;

public:
  CcharDataArray(const char* srcData) : _isNullPtr(srcData == 0) {
    if (!*_isNullPtr) {
      stringData = new std::string(srcData);
    } else {
      stringData = nullptr;
    }
  }
  CcharDataArray(const char** srcData) : _isNullPtr(srcData == 0) {
    if (!*_isNullPtr) {
      stringData = new std::string(*srcData);
    } else {
      stringData = nullptr;
    }
  }
  CcharDataArray(const char* srcData, char terminator, int term_pos) : _isNullPtr(srcData == 0) {
    if (!*_isNullPtr) {
      stringData = new std::string(srcData);
    } else {
      stringData = nullptr;
    }
  }
  CcharDataArray(size_t num, const char* srcData) : _isNullPtr(srcData == 0) {
    if (!*_isNullPtr) {
      stringData = new std::string(srcData);
    } else {
      stringData = nullptr;
    }
  }
  ~CcharDataArray();
  const char* Value() {
    if (*_isNullPtr) {
      return 0;
    }
    outData = stringData->c_str();
    return outData;
  }
  const char* operator*() {
    return Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

typedef CDataArray<const char*, CcharDataArray> CStringDataArray;
typedef CDataArray<std::uint8_t, Cuint8_tData> Cuint8_tDataArray;
typedef CDataArray<std::uint32_t, Cuint32_tData> Cuint32_tDataArray;
typedef CDataArray<std::uint64_t, Cuint64_tData> Cuint64_tDataArray;
typedef CDataArray<std::int32_t, Cint32_tData> Cint32_tDataArray;
typedef CDataArray<std::int64_t, Cint64_tData> Cint64_tDataArray;
typedef CDataArray<float, CfloatData> CfloatDataArray;
typedef CDataArray<size_t, Csize_tData> Csize_tDataArray;
typedef CSimpleData<HWND> CVkHWNDData;
typedef CSimpleData<HINSTANCE> CVkHINSTANCEData;
typedef CSimpleData<HMONITOR> CVkHMONITORData;
typedef CSimpleData<xcb_connection_t*> Cxcb_connection_tData;
typedef CSimpleData<xcb_window_t> Cxcb_window_tData;
typedef CSimpleData<Display*> CVkDisplayData;
typedef CSimpleData<Window> CVkWindowData;
// typedef CDataArray<uint32_t, CcharData> CVulkanShaderData;
class CVulkanShaderData {
private:
  std::vector<std::shared_ptr<Cuint32_tData>> _cargsDict;
  std::vector<uint32_t> _data;

public:
  CVulkanShaderData() {}
  CVulkanShaderData(size_t size, const uint32_t* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    for (size_t i = 0; i < size / 4; i++) {
      auto obj = std::make_shared<Cuint32_tData>(&dictionary[i]);
      _cargsDict.push_back(obj);
    }
  }
  CVulkanShaderData(size_t size, uint32_t* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    for (size_t i = 0; i < size / 4; i++) {
      auto obj = std::make_shared<Cuint32_tData>(&dictionary[i]);
      _cargsDict.push_back(obj);
    }
  }
  std::vector<std::shared_ptr<Cuint32_tData>>& Vector() {
    return _cargsDict;
  }
  const std::vector<std::shared_ptr<Cuint32_tData>>& Vector() const {
    return _cargsDict;
  }

  uint32_t* Value() {
    if (_cargsDict.size() == 0) {
      return 0;
    }
    _data.clear();
    for (unsigned i = 0; i < _cargsDict.size(); i++) {
      _data.push_back(**_cargsDict[i]);
    }
    return &_data[0];
  }
  uint32_t* operator*() {
    return Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};
class CBaseDataStruct {
public:
  CBaseDataStruct() {}
  virtual ~CBaseDataStruct() = default;
  virtual void* GetPtrType() {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
};

typedef CSimpleData<VkStructureType> CVkStructureTypeData;
class CVkGenericArgumentData : gits::noncopyable {
  CBaseDataStruct* _argument;

public:
  ~CVkGenericArgumentData();
  CVkGenericArgumentData(const void* vkgenericargumentdata);

  const void* Value();
  struct PtrConverter {
  private:
    const void* _ptr;

  public:
    explicit PtrConverter(const void* ptr) : _ptr(ptr) {}
    operator const void*() const {
      return _ptr;
    }
    operator void*() const {
      return (void*)_ptr;
    }
    //operator VkAndroidSurfaceCreateInfoKHR*() const { return (VkAndroidSurfaceCreateInfoKHR*)_ptr; }
    operator VkApplicationInfo*() const {
      return (VkApplicationInfo*)_ptr;
    }
    operator VkBindSparseInfo*() const {
      return (VkBindSparseInfo*)_ptr;
    }
    operator VkBufferCreateInfo*() const {
      return (VkBufferCreateInfo*)_ptr;
    }
    operator VkBufferMemoryBarrier*() const {
      return (VkBufferMemoryBarrier*)_ptr;
    }
    operator VkBufferViewCreateInfo*() const {
      return (VkBufferViewCreateInfo*)_ptr;
    }
    operator VkCommandBufferAllocateInfo*() const {
      return (VkCommandBufferAllocateInfo*)_ptr;
    }
    operator VkCommandBufferBeginInfo*() const {
      return (VkCommandBufferBeginInfo*)_ptr;
    }
    operator VkCommandBufferInheritanceInfo*() const {
      return (VkCommandBufferInheritanceInfo*)_ptr;
    }
    operator VkCommandPoolCreateInfo*() const {
      return (VkCommandPoolCreateInfo*)_ptr;
    }
    operator VkComputePipelineCreateInfo*() const {
      return (VkComputePipelineCreateInfo*)_ptr;
    }
    operator VkCopyDescriptorSet*() const {
      return (VkCopyDescriptorSet*)_ptr;
    }
    // operator VkDebugReportCreateInfoExt*() const { return (VkDebugReportCreateInfoExt*)_ptr; }
    operator VkDescriptorPoolCreateInfo*() const {
      return (VkDescriptorPoolCreateInfo*)_ptr;
    }
    operator VkDescriptorSetAllocateInfo*() const {
      return (VkDescriptorSetAllocateInfo*)_ptr;
    }
    operator VkDescriptorSetLayoutCreateInfo*() const {
      return (VkDescriptorSetLayoutCreateInfo*)_ptr;
    }
    operator VkDeviceCreateInfo*() const {
      return (VkDeviceCreateInfo*)_ptr;
    }
    operator VkDeviceQueueCreateInfo*() const {
      return (VkDeviceQueueCreateInfo*)_ptr;
    }
    operator VkDisplayModeCreateInfoKHR*() const {
      return (VkDisplayModeCreateInfoKHR*)_ptr;
    }
    operator VkDisplayPresentInfoKHR*() const {
      return (VkDisplayPresentInfoKHR*)_ptr;
    }
    operator VkDisplaySurfaceCreateInfoKHR*() const {
      return (VkDisplaySurfaceCreateInfoKHR*)_ptr;
    }
    operator VkEventCreateInfo*() const {
      return (VkEventCreateInfo*)_ptr;
    }
    operator VkFenceCreateInfo*() const {
      return (VkFenceCreateInfo*)_ptr;
    }
    operator VkFramebufferCreateInfo*() const {
      return (VkFramebufferCreateInfo*)_ptr;
    }
    operator VkGraphicsPipelineCreateInfo*() const {
      return (VkGraphicsPipelineCreateInfo*)_ptr;
    }
    operator VkImageCreateInfo*() const {
      return (VkImageCreateInfo*)_ptr;
    }
    operator VkImageMemoryBarrier*() const {
      return (VkImageMemoryBarrier*)_ptr;
    }
    operator VkImageViewCreateInfo*() const {
      return (VkImageViewCreateInfo*)_ptr;
    }
    operator VkInstanceCreateInfo*() const {
      return (VkInstanceCreateInfo*)_ptr;
    }
    // operator VkLoaderDeviceCreateInfo*() const { return (VkLoaderDeviceCreateInfo*)_ptr; }
    // operator VkLoaderInstanceCreateInfo*() const { return (VkLoaderInstanceCreateInfo*)_ptr; }
    operator VkMappedMemoryRange*() const {
      return (VkMappedMemoryRange*)_ptr;
    }
    // operator VkMaxEnum*() const { return (VkMaxEnum*)_ptr; }
    operator VkMemoryAllocateInfo*() const {
      return (VkMemoryAllocateInfo*)_ptr;
    }
    operator VkMemoryBarrier*() const {
      return (VkMemoryBarrier*)_ptr;
    }
    // operator VkMirSurfaceCreateInfoKHR*() const { return (VkMirSurfaceCreateInfoKHR*)_ptr; }
    operator VkPipelineCacheCreateInfo*() const {
      return (VkPipelineCacheCreateInfo*)_ptr;
    }
    operator VkPipelineColorBlendStateCreateInfo*() const {
      return (VkPipelineColorBlendStateCreateInfo*)_ptr;
    }
    operator VkPipelineDepthStencilStateCreateInfo*() const {
      return (VkPipelineDepthStencilStateCreateInfo*)_ptr;
    }
    operator VkPipelineDynamicStateCreateInfo*() const {
      return (VkPipelineDynamicStateCreateInfo*)_ptr;
    }
    operator VkPipelineInputAssemblyStateCreateInfo*() const {
      return (VkPipelineInputAssemblyStateCreateInfo*)_ptr;
    }
    operator VkPipelineLayoutCreateInfo*() const {
      return (VkPipelineLayoutCreateInfo*)_ptr;
    }
    operator VkPipelineMultisampleStateCreateInfo*() const {
      return (VkPipelineMultisampleStateCreateInfo*)_ptr;
    }
    operator VkPipelineRasterizationStateCreateInfo*() const {
      return (VkPipelineRasterizationStateCreateInfo*)_ptr;
    }
    operator VkPipelineShaderStageCreateInfo*() const {
      return (VkPipelineShaderStageCreateInfo*)_ptr;
    }
    operator VkPipelineTessellationStateCreateInfo*() const {
      return (VkPipelineTessellationStateCreateInfo*)_ptr;
    }
    operator VkPipelineVertexInputStateCreateInfo*() const {
      return (VkPipelineVertexInputStateCreateInfo*)_ptr;
    }
    operator VkPipelineViewportStateCreateInfo*() const {
      return (VkPipelineViewportStateCreateInfo*)_ptr;
    }
    operator VkPresentInfoKHR*() const {
      return (VkPresentInfoKHR*)_ptr;
    }
    operator VkQueryPoolCreateInfo*() const {
      return (VkQueryPoolCreateInfo*)_ptr;
    }
    // operator VkRangeSize*() const { return (VkRangeSize*)_ptr; }
    operator VkRenderPassBeginInfo*() const {
      return (VkRenderPassBeginInfo*)_ptr;
    }
    operator VkRenderPassCreateInfo*() const {
      return (VkRenderPassCreateInfo*)_ptr;
    }
    operator VkSamplerCreateInfo*() const {
      return (VkSamplerCreateInfo*)_ptr;
    }
    operator VkSemaphoreCreateInfo*() const {
      return (VkSemaphoreCreateInfo*)_ptr;
    }
    operator VkShaderModuleCreateInfo*() const {
      return (VkShaderModuleCreateInfo*)_ptr;
    }
    operator VkSubmitInfo*() const {
      return (VkSubmitInfo*)_ptr;
    }
    operator VkSwapchainCreateInfoKHR*() const {
      return (VkSwapchainCreateInfoKHR*)_ptr;
    }
    //operator VkWaylandSurfaceCreateInfoKHR*() const { return (VkWaylandSurfaceCreateInfoKHR*)_ptr; }
    operator VkWin32SurfaceCreateInfoKHR*() const {
      return (VkWin32SurfaceCreateInfoKHR*)_ptr;
    }
    operator VkWriteDescriptorSet*() const {
      return (VkWriteDescriptorSet*)_ptr;
    }
    operator VkXcbSurfaceCreateInfoKHR*() const {
      return (VkXcbSurfaceCreateInfoKHR*)_ptr;
    }
    // operator VkXlibSurfaceCreateInfoKHR*() const { return (VkXlibSurfaceCreateInfoKHR*)_ptr; }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
};

class CVkGenericArgumentDataArray {
private:
  std::vector<std::shared_ptr<CVkGenericArgumentData>> _cgenericargsDict;
  std::vector<const void*> _data;

public:
  CVkGenericArgumentDataArray() {}
  CVkGenericArgumentDataArray(uint32_t size, const void* const* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    for (uint32_t i = 0; i < size; i++) {
      auto obj = std::make_shared<CVkGenericArgumentData>(dictionary[i]);
      _cgenericargsDict.push_back(obj);
    }
  }
  const void** Value() {
    if (_cgenericargsDict.size() == 0) {
      return 0;
    }
    _data.clear();
    for (uint32_t i = 0; i < _cgenericargsDict.size(); i++) {
      _data.push_back(**_cgenericargsDict[i]);
    }
    return &_data[0];
  }
  const void** operator*() {
    return Value();
  }
  const void** Original() {
    return Value();
  }
};

class CpNextWrapperData {
  std::shared_ptr<CVkGenericArgumentData> _ptr;

public:
  CpNextWrapperData(const void* srcpNext)
      : _ptr(std::make_shared<CVkGenericArgumentData>(srcpNext)) {}
  const void* Value() {
    if (_ptr) {
      return _ptr->Value();
    } else {
      return nullptr;
    }
  }
  struct PtrConverter {
  private:
    const void* _ptr;

  public:
    explicit PtrConverter(const void* ptr) : _ptr(ptr) {}
    operator void*() const {
      return (void*)_ptr;
    }
    operator const void*() const {
      return _ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
};

class CVkClearColorValueData : public CBaseDataStruct, gits::noncopyable {
  Cuint32_tDataArray* _uint32;

  VkClearColorValue* _ClearColorValue;
  CboolData _isNullPtr;

public:
  CVkClearColorValueData(const VkClearColorValue* clearcolorvalue);
  ~CVkClearColorValueData();
  VkClearColorValue* Value();

  PtrConverter<VkClearColorValue> operator*() {
    return PtrConverter<VkClearColorValue>(Value());
  }
  void* GetPtrType() {
    return (void*)Value();
  }
};
typedef CDataArray<VkClearColorValue, CVkClearColorValueData> CVkClearColorValueDataArray;
class CVkClearValueData : public CBaseDataStruct, gits::noncopyable {
  CVkClearColorValueData* _color;

  VkClearValue* _ClearValue;
  CboolData _isNullPtr;

public:
  CVkClearValueData(const VkClearValue* clearvalue);
  ~CVkClearValueData();
  VkClearValue* Value();

  PtrConverter<VkClearValue> operator*() {
    return PtrConverter<VkClearValue>(Value());
  }
  void* GetPtrType() {
    return (void*)Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};
typedef CDataArray<VkClearValue, CVkClearValueData> CVkClearValueDataArray;
typedef CSimpleMappedData<VkSampler> CVkSamplerData;
typedef CSimpleMappedData<VkImageView> CVkImageViewData;
typedef CSimpleData<VkImageLayout> CVkImageLayoutData;

class CVkDescriptorImageInfoData : public CBaseDataStruct, gits::noncopyable {
  CVkSamplerData* _sampler;
  CVkImageViewData* _imageView;
  CVkImageLayoutData* _imageLayout;

  VkDescriptorImageInfo* _DescriptorImageInfo;
  CboolData _isNullPtr;

public:
  CVkDescriptorImageInfoData(const VkDescriptorImageInfo* descriptorimageinfo,
                             const VkDescriptorType descriptorType);
  ~CVkDescriptorImageInfoData();
  VkDescriptorImageInfo* Value();

  PtrConverter<VkDescriptorImageInfo> operator*() {
    return PtrConverter<VkDescriptorImageInfo>(Value());
  }
  void* GetPtrType() {
    return (void*)Value();
  }
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> pointers;
    for (auto obj : _sampler->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : _imageView->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    for (auto obj : _imageLayout->GetMappedPointers()) {
      pointers.insert((uint64_t)obj);
    }
    return pointers;
  }
};
typedef CDataArray<VkDescriptorImageInfo, CVkDescriptorImageInfoData>
    CVkDescriptorImageInfoDataArray;

class CBufferDeviceAddressObjectData : public CBaseDataStruct, gits::noncopyable {
private:
  Cuint64_tData* _deviceAddress;

public:
  CBufferDeviceAddressObjectData(VkDeviceAddress deviceAddress);
  ~CBufferDeviceAddressObjectData();

  VkDeviceAddress operator*() {
    return Value();
  }

  VkDeviceAddress Value();
};
} // namespace Vulkan
} // namespace gits
