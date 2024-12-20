// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "argument.h"
#include "vulkanHeader.h"
#include "vectorMapper.h"

namespace gits {
namespace Vulkan {

//************************** CVulkanObj ***********************************
template <typename T, typename TTypeTag>
class CVulkanObj : public CArgument {
protected:
  typedef std::unordered_map<T, T> name_map_t;
  typedef VectorMapper<T, 100000> name_vec_map_t;
  T key_;
  typedef typename std::vector<T>::size_type size_type;

public:
  static const char* NAME;
  typedef CArgumentMappedSizedArray<T, CVulkanObj, gits::ADD_MAPPING> CSMapArray;
  typedef CArgumentMappedSizedArray<T, CVulkanObj, gits::NO_ACTION> CSArray;

  CVulkanObj() : key_(VK_NULL_HANDLE) {}
  CVulkanObj(T arg) : key_(arg) {}
  CVulkanObj(T* arg) : key_(*arg) {}

  static void AddMapping(T key, T value) {
    if (useVectorMapper()) {
      get_vector_mapper()[(size_type)key] = value;
    } else {
      get_map()[key] = value;
    }
  }

  void AddMapping(T value) {
    AddMapping(key_, value);
  }

  static void AddMapping(const T* keys, const T* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      AddMapping(keys[i], values[i]);
    }
  }

  void RemoveMapping() {
    RemoveMapping(key_);
  }

  static void RemoveMapping(T key) {
    if (useVectorMapper()) {
      get_vector_mapper().Unmap((size_type)key);
    } else {
      get_map().erase(key);
    }
  }

  static void RemoveMapping(const T* keys, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      RemoveMapping(keys[i]);
    }
  }

  static T GetMapping(T key) {
    if (VK_NULL_HANDLE == key) {
      return VK_NULL_HANDLE;
    }
    if (useVectorMapper()) {
      T val = get_vector_mapper()[(size_type)key];
      if (val == get_vector_mapper().NotMappedVal()) {
        Log(ERR) << "Couldn't map Vulkan object name " << key;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      return val;
    } else {
      auto iter = get_map().find(key);
      if (iter == get_map().end()) {
        if (Config::Get().IsPlayer()) {
          Log(ERR) << "Couldn't map Vulkan object name " << key;
          throw std::runtime_error(EXCEPTION_MESSAGE);
        } else {
          return key;
        }
      }
      return iter->second;
    }
  }

  static void GetMapping(const T* keys, T* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      values[i] = GetMapping(keys[i]);
    }
  }

  static std::vector<T> GetMapping(const T* keys, size_t num) {
    std::vector<T> v;
    v.reserve(num);
    for (size_t i = 0; i < num; ++i) {
      v.push_back(GetMapping(keys[i]));
    }
    return v;
  }

  static bool CheckMapping(T key) {
    if (useVectorMapper()) {
      auto& the_vec_map = get_vector_mapper();
      if (the_vec_map[(size_type)key] == the_vec_map.NotMappedVal()) {
        return false;
      } else {
        return true;
      }
    } else {
      auto& the_map = get_map();
      return the_map.find(key) != the_map.end();
    }
  }

  bool CheckMapping() {
    return CheckMapping(key_);
  }

  void* GetPtrType() {
    if (useVectorMapper()) {
      auto& vector_mapper = get_vector_mapper();
      if (vector_mapper[(size_type)key_] == vector_mapper.NotMappedVal()) {
        Log(ERR) << "Couldn't map Vulkan object name " << key_;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      return &vector_mapper[(size_type)key_];
    } else {
      auto& map = get_map();
      if (map.find(key_) == map.end()) {
        Log(ERR) << "Couldn't map Vulkan object name " << key_;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      return &map[key_];
    }
  }

  virtual const char* Name() const override {
    return NAME;
  }
  static const char* TypeNameStr() {
    return NAME;
  }
  static const char* WrapTypeNameStr() {
    static std::string cname_ = std::string("C") + NAME;
    return cname_.c_str();
  }

  void Reset(T value) {
    key_ = value;
  }
  T Original() const {
    return key_;
  }
  T Value() const {
    return GetMapping(key_);
  }
  T operator*() const {
    return Value();
  }

  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream,
                         CGits::Instance().GetOrderedIdFromPtr(reinterpret_cast<void*>(key_)));
  }

  virtual void Read(CBinIStream& stream) override {
    read_name_from_stream(stream, key_);
  }

  virtual void Write(CCodeOStream& stream) const override {
    stream << "GetMapping((" << TypeNameStr() << ")" << hex(key_) << ")";
  }

  void Assign(T other) {
    AddMapping(other);
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> objects;
    objects.insert((uint64_t)GetMapping(key_));
    return objects;
  }

private:
  static name_map_t& get_map() {
    INIT_NEW_STATIC_OBJ(objects_map, name_map_t)
    static bool initialized = false;
    if (!initialized) {
      objects_map[0] = 0;
    }
    return objects_map;
  }
  static name_vec_map_t& get_vector_mapper() {
    INIT_NEW_STATIC_OBJ(objects_map, name_vec_map_t)
    static bool initialized = false;
    if (!initialized) {
      objects_map[(size_type)0] = (T)0;
      initialized = true;
    }
    return objects_map;
  }
  static bool useVectorMapper() {
    static bool useVectorMap =
        !stream_older_than(GITS_REC_PLAY_PTR_VECTOR_MAPS) && Config::IsPlayer();
    return useVectorMap;
  }
};

// The purpose of the type traits pattern is to provide metadata for
// external types we can't modify.
template <class T>
class CVulkanEnumTypeTraits {
public:
  static const char* Name();
  static std::string GetVariantName(T variant);
};

template <class T>
class CVulkanEnum : public gits::Cint32_t {
public:
  static const char* NAME; // In case you need a static member, normally use Name().

  using gits::Cint32_t::Write; // Otherwise it's not possible to override only 1 overload of Write.
  typedef CArgumentSizedArray<T, CVulkanEnum> CSArray;

  CVulkanEnum() : Cint32_t(0) {}
  CVulkanEnum(int32_t value) : Cint32_t(value) {}
  T Original() const {
    return (T)_int;
  }
  T operator*() const {
    return (T)_int;
  }
  void Assign(int32_t value) {
    _int = value;
  }
  const char* Name() const override {
    return CVulkanEnumTypeTraits<T>::Name();
  }
  virtual void Write(CCodeOStream& stream) const override {
    stream << CVulkanEnumTypeTraits<T>::GetVariantName((T)_int);
  }
};

// Since this is a pointer, we aren't limited to string literals, we can assign any pointer.
template <class T>
const char* CVulkanEnum<T>::NAME = CVulkanEnumTypeTraits<T>::Name();

// It's just like CBinaryResource except in CCode it will declare the
// Resource as a variable instead of using it like a value. This extends
// Resource's lifetime until the end of the API call scope.
class CDeclaredBinaryResource : public CBinaryResource {
public:
  using CBinaryResource::CBinaryResource; // Inherit constructors.
  using CBinaryResource::Write;           // Inherit Write(CBinOStream).
  virtual bool DeclarationNeeded() const override;
  virtual void Declare(CCodeOStream& stream) const override;
  virtual void Write(CCodeOStream& stream) const override;
};

class CVkBool32 : public CArgument {
protected:
  uint32_t _uint32;

public:
  typedef CArgumentSizedArray<uint32_t, Cuint32_t> CSArray;
  CVkBool32() : _uint32(VK_FALSE) {}
  CVkBool32(uint32_t uint) : _uint32(uint) {}
  uint32_t operator*() {
    return _uint32;
  }
  uint32_t Original() {
    return _uint32;
  }
  virtual const char* Name() const override {
    return "VkBool32";
  }
  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream, _uint32);
  }
  virtual void Read(CBinIStream& stream) override {
    read_name_from_stream(stream, _uint32);
  }
  virtual void Write(CCodeOStream& stream) const override {
    if (_uint32 == VK_TRUE) {
      stream << "VK_TRUE";
    } else if (_uint32 == VK_FALSE) {
      stream << "VK_FALSE";
    } else {
      Log(ERR) << "Invalid VkBool32 value: " << _uint32;
      throw ENotSupported(EXCEPTION_MESSAGE);
    }
  }
};

class CNullWrapper : public CArgument {
  std::uint64_t _ptr;

public:
  CNullWrapper() : _ptr(0) {}
  CNullWrapper(const void* ptr) : _ptr((std::uint64_t)ptr) {
    if (ptr != NULL) {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
  }
  CNullWrapper(void* ptr) : _ptr((std::uint64_t)ptr) {
    if (ptr != NULL) {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
  }
  CNullWrapper(const VkAllocationCallbacks* ptr) : _ptr((std::uint64_t)ptr) {
    if (ptr != NULL) {
      CALL_ONCE[] {
        Log(WARN) << "Non-null allocation callbacks detected\n";
      };
    }
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
    return PtrConverter(NULL);
  }
  PtrConverter Original() {
    return PtrConverter(NULL);
  }
  void Assign(void* ptr) {
    _ptr = (std::uint64_t)ptr;
  }
  virtual const char* Name() const override {
    return "NullWrapper";
  }
  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream, _ptr);
  }
  virtual void Read(CBinIStream& stream) override {
    read_name_from_stream(stream, _ptr);
  }
  virtual void Write(CCodeOStream& stream) const override {
    if (_ptr) {
      Log(WARN) << "Pointer is " << _ptr << "while it should be null.";
      stream << hex(_ptr) << " /* nullptr was expected here */";
    } else {
      stream << "nullptr";
    }
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

class CVoidPtr : public CArgument {
  std::uint64_t _ptr;
  int _type; // 1 = pointer, 2 = pointer to pointer, etc.
public:
  CVoidPtr() : _ptr(0), _type(1) {}
  CVoidPtr(const void* ptr) : _ptr((std::uint64_t)ptr), _type(1) {}
  CVoidPtr(void* ptr) : _ptr((std::uint64_t)ptr), _type(1) {}
  CVoidPtr(const void** ptr) : _ptr((std::uint64_t)ptr), _type(2) {}
  CVoidPtr(void** ptr) : _ptr((std::uint64_t)ptr), _type(2) {}
  struct PtrConverter {
  private:
    void* _ptr;

  public:
    explicit PtrConverter(void** ptr) : _ptr(*ptr) {}
    explicit PtrConverter(void* ptr) : _ptr(ptr) {}
    operator void*() const {
      return _ptr;
    }
    operator void**() const {
      static void*
          tempPtr; // Static to ensure the lifetime, but this introduces thread-safety issues.
      tempPtr = _ptr;
      return &tempPtr;
    }
  };

  PtrConverter operator*() {
    if (_type == 2) {
      return PtrConverter((void**)_ptr);
    } else {
      return PtrConverter((void*)_ptr);
    }
  }
  PtrConverter Original() {
    if (_type == 2) {
      return PtrConverter((void**)_ptr);
    } else {
      return PtrConverter((void*)_ptr);
    }
  }
  void Assign(void* ptr) {
    _ptr = (std::uint64_t)ptr;
  }
  virtual const char* Name() const override {
    return "VoidPtr";
  }
  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream, _ptr);
  }
  virtual void Read(CBinIStream& stream) override {
    read_name_from_stream(stream, _ptr);
  }
  virtual void Write(CCodeOStream& stream) const override {
    if (_type == 2) {
      if ((void**)_ptr == nullptr) {
        stream << "nullptr";
      } else {
        stream << "(void**)" << hex((void**)_ptr);
      }
    } else {
      if (*(void**)_ptr == nullptr) {
        stream << "nullptr";
      } else {
        stream << "(void*)" << hex(*(void**)_ptr);
      }
    }
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

class CVulkanShader : public CArgument {
  CDeclaredBinaryResource _data;
  CDeclaredBinaryResource::PointerProxy _pCodeProxy;

public:
  CVulkanShader() {}
  CVulkanShader(size_t codeSize, const uint32_t* pCode)
      : _data(RESOURCE_DATA_RAW, (void*)pCode, codeSize) {}

  const uint32_t* Value() {
    _pCodeProxy = *_data;
    return _pCodeProxy;
  }
  const uint32_t* operator*() {
    return Value();
  }
  const uint32_t* Original() {
    return Value();
  }
  virtual const char* Name() const override {
    return "CVulkanShader";
  }
  virtual void Write(CBinOStream& stream) const override {
    _data.Write(stream);
  }
  virtual void Read(CBinIStream& stream) override {
    _data.Read(stream);
  }
  virtual void Write(CCodeOStream& stream) const override {
    stream << _data;
  }
  virtual bool DeclarationNeeded() const override {
    return _data.DeclarationNeeded();
  }
  virtual void Declare(CCodeOStream& stream) const override {
    _data.Declare(stream);
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

// On 32-bit all nondispatchable handle types are typedef'd to uint64_t.
// This means compiler sees e.g. CVulkanObj<VkBuffer> as identical to
// CVulkanObj<VkEvent>. On the other hand type tags below are seen by the
// compiler as distinct types. We use them to instantiate the template for
// each handle type.
typedef struct HWND_T* HWNDTypeTag;
typedef CVulkanObj<HWND, HWNDTypeTag> CVkHWND;
typedef struct HINSTANCE_T* HINSTANCETypeTag;
typedef CVulkanObj<HINSTANCE, HINSTANCETypeTag> CVkHINSTANCE;
typedef struct HMONITOR_T* HMONITORTypeTag;
typedef CVulkanObj<HMONITOR, HMONITORTypeTag> CVkHMONITOR;
typedef struct D3DKMT_HANDLE_T* D3DKMT_HANDLETypeTag;
typedef CVulkanObj<D3DKMT_HANDLE, D3DKMT_HANDLETypeTag> CD3DKMT_HANDLE;
typedef void* VkWindow_TypeTag;
typedef struct xcb_connection_t* xcb_connection_t_TypeTag;
typedef CVulkanObj<xcb_connection_t*, xcb_connection_t_TypeTag> Cxcb_connection_t;
typedef struct Display_T* VkDisplay_TypeTag;
typedef CVulkanObj<Display*, VkDisplay_TypeTag> CVkDisplay;

typedef CVulkanObj<void*, VkWindow_TypeTag> _CVkWindow;
class CVkWindow : public _CVkWindow {
public:
  CVkWindow() : _CVkWindow() {}
  CVkWindow(uint32_t v) : _CVkWindow(reinterpret_cast<void*>(v)) {}

  void AddMapping(uint32_t v) {
    _CVkWindow::AddMapping(reinterpret_cast<void*>(v));
  }

  uint32_t Original() const {
    return reinterpret_cast<uintptr_t>(_CVkWindow::Original());
  }
  uint32_t operator*() const {
    return reinterpret_cast<uintptr_t>(Value());
  }
};

class CVkClearColorValue : public CArgument, gits::noncopyable {
  std::unique_ptr<Cuint32_t::CSArray> _uint32;

  std::unique_ptr<VkClearColorValue> _ClearColorValue;
  std::unique_ptr<VkClearColorValue> _ClearColorValueOriginal;
  Cbool _isNullPtr;

public:
  CVkClearColorValue();
  CVkClearColorValue(const VkClearColorValue* clearcolorvalue);
  virtual const char* Name() const override {
    return "VkClearColorValue";
  }
  VkClearColorValue* Value();

  PtrConverter<VkClearColorValue> operator*() {
    return PtrConverter<VkClearColorValue>(Value());
  }
  PtrConverter<VkClearColorValue> Original();
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual bool AmpersandNeeded() const override;
  virtual bool DeclarationNeeded() const override;
  virtual void Declare(CCodeOStream& stream) const override;
};

class CVkClearValue : public CArgument, gits::noncopyable {
  std::unique_ptr<CVkClearColorValue> _color;

  std::unique_ptr<VkClearValue> _ClearValue;
  std::unique_ptr<VkClearValue> _ClearValueOriginal;
  Cbool _isNullPtr;

public:
  CVkClearValue();
  CVkClearValue(const VkClearValue* clearvalue);
  static const char* NAME;
  virtual const char* Name() const override {
    return NAME;
  }
  VkClearValue* Value();

  PtrConverter<VkClearValue> operator*() {
    return PtrConverter<VkClearValue>(Value());
  }
  PtrConverter<VkClearValue> Original();
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual void Declare(CCodeOStream& stream) const override;
  virtual bool DeclarationNeeded() const override;

  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

typedef CVulkanEnum<VkStructureType> CVkStructureType;
class CVkGenericArgument : public CArgument, gits::noncopyable {
  std::unique_ptr<CVkStructureType> _sType;
  std::unique_ptr<CArgument> _argument;
  std::unique_ptr<Cbool> _skipped;
  Cbool _isNullPtr;

public:
  CVkGenericArgument();
  CVkGenericArgument(const void* vkgenericargument);
  void InitArgument(uint32_t type);
  void CreateArgument(uint32_t type, const void* vkgenericargument);
  virtual const char* Name() const override;
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
    operator VkMemoryDedicatedRequirements*() const {
      return (VkMemoryDedicatedRequirements*)_ptr;
    }
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
    // operator VkWaylandSurfaceCreateInfoKHR*() const {
    //   return (VkWaylandSurfaceCreateInfoKHR*)_ptr;
    // }
    operator VkWin32SurfaceCreateInfoKHR*() const {
      return (VkWin32SurfaceCreateInfoKHR*)_ptr;
    }
    operator VkWriteDescriptorSet*() const {
      return (VkWriteDescriptorSet*)_ptr;
    }
    operator VkXcbSurfaceCreateInfoKHR*() const {
      return (VkXcbSurfaceCreateInfoKHR*)_ptr;
    }
    // operator VkXlibSurfaceCreateInfoKHR*() const {
    //   return (VkXlibSurfaceCreateInfoKHR*)_ptr;
    // }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
  PtrConverter Original() {
    return PtrConverter(Value());
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual bool AmpersandNeeded() const override;
  virtual bool DeclarationNeeded() const override;
  virtual void Declare(CCodeOStream& stream) const override;
};

class CVkGenericArgumentArray : public CArgument, gits::noncopyable {
private:
  std::vector<std::shared_ptr<CVkGenericArgument>> _cgenericargsDict;
  std::vector<const void*> _data;

public:
  CVkGenericArgumentArray() {}
  CVkGenericArgumentArray(int size, const void* const* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    for (int i = 0; i < size; i++) {
      auto obj = std::make_shared<CVkGenericArgument>(dictionary[i]);
      _cgenericargsDict.push_back(std::move(obj));
    }
  }

  const void** Value() {
    if (_cgenericargsDict.size() == 0) {
      return 0;
    }
    _data.clear();
    for (unsigned i = 0; i < _cgenericargsDict.size(); i++) {
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
  const char* Name() const override {
    return "CVkGenericArgumentArray";
  }
  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream, (unsigned)_cgenericargsDict.size());
    for (auto& arg : _cgenericargsDict) {
      arg->Write(stream);
    }
  }
  virtual void Read(CBinIStream& stream) override {
    unsigned dictSize;
    read_name_from_stream(stream, dictSize);
    if (dictSize <= _cgenericargsDict.max_size()) {
      for (unsigned i = 0; i < dictSize; i++) {
        std::shared_ptr<CVkGenericArgument> keyArgPtr(new CVkGenericArgument());
        keyArgPtr->Read(stream);
        _cgenericargsDict.push_back(std::move(keyArgPtr));
      }
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  virtual void Write(CCodeOStream& stream) const override {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
};

class CpNextWrapper : public CArgument, gits::noncopyable {
  std::uint64_t _ptr;
  std::unique_ptr<CVkGenericArgument> _data;

public:
  CpNextWrapper() : _ptr(0), _data(nullptr) {}
  CpNextWrapper(const void* ptr);

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

  const void* Value() {
    if (!_ptr) {
      return nullptr;
    }

    return _data->Value();
  }

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
  PtrConverter Original() {
    return PtrConverter(Value());
  }
  const char* Name() const override {
    return "pNextWrapper";
  }
  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream, _ptr);
    if (_ptr) {
      _data->Write(stream);
    }
  }
  virtual void Read(CBinIStream& stream) override {
    read_name_from_stream(stream, _ptr);
    if (_ptr) {
      _data = std::make_unique<CVkGenericArgument>();
      _data->Read(stream);
    }
  }
  virtual void Write(CCodeOStream& stream) const override {
    if ((void*)_ptr == nullptr) {
      stream << "nullptr";
    } else {
      _data->Write(stream);
    }
  }
  virtual void Declare(CCodeOStream& stream) const override {
    if ((void*)_ptr != nullptr) {
      _data->Declare(stream);
    }
  }
};

typedef CVulkanEnum<VkDescriptorType> CVkDescriptorType;

class CDescriptorUpdateTemplateObject : public CArgument, gits::noncopyable {
private:
  std::unique_ptr<CArgument> _argument;
  std::unique_ptr<CVkDescriptorType> _descType;
  std::unique_ptr<Cuint64_t> _offset;
  std::unique_ptr<Cuint64_t> _size;
  std::vector<char> _data;

public:
  CDescriptorUpdateTemplateObject()
      : _descType(std::make_unique<CVkDescriptorType>()),
        _offset(std::make_unique<Cuint64_t>()),
        _size(std::make_unique<Cuint64_t>()) {}
  CDescriptorUpdateTemplateObject(VkDescriptorType descType,
                                  const void* pData,
                                  std::uint64_t offset);
  const void* Value() {
    _data.clear();
    _data.resize((size_t) * *_size);
    memcpy(_data.data(), _argument->GetPtrType(), (size_t) * *_size);
    return (void*)_data.data();
  }
  std::uint64_t GetOffset() {
    return **_offset;
  }
  std::uint64_t GetSize() {
    return **_size;
  }
  virtual void Write(CBinOStream& stream) const override {
    _descType->Write(stream);
    _offset->Write(stream);
    _size->Write(stream);
    _argument->Write(stream);
  }
  virtual void Read(CBinIStream& stream) override;
  const char* Name() const override {
    return "CDescriptorUpdateTemplateObject";
  }
  virtual void Write(CCodeOStream& stream) const override {
    stream << stream.VariableName(_argument->ScopeKey());
  }
  virtual void Declare(CCodeOStream& stream) const override {
    if (**_descType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
        **_descType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) { // VkBufferView
      _argument->VariableNameRegister(
          stream, false); // CVulkanObj didn't declare variable, so we are declaring it here
    }
    _argument->Declare(stream);
  }
};

class CUpdateDescriptorSetWithTemplateArray : public CArgument, gits::noncopyable {
private:
  std::vector<std::shared_ptr<CDescriptorUpdateTemplateObject>> _cgenericargsDict;
  std::vector<char> _data;
  std::unique_ptr<Cuint64_t> _size;

public:
  CUpdateDescriptorSetWithTemplateArray() : _size(std::make_unique<Cuint64_t>()) {}
  CUpdateDescriptorSetWithTemplateArray(VkDescriptorUpdateTemplate descriptorUpdateTemplateKHR,
                                        const void* pData);

  const void* Value();
  const void* operator*() {
    return Value();
  }
  const void* Original() {
    return Value();
  }
  const char* Name() const override {
    return "CUpdateDescriptorSetWithTemplateArray";
  }
  virtual void Write(CBinOStream& stream) const override {
    write_name_to_stream(stream, (unsigned)_cgenericargsDict.size());
    _size->Write(stream);
    for (auto& arg : _cgenericargsDict) {
      arg->Write(stream);
    }
  }
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override {
    stream << "&" << getVarName("vec_", this) << "[0]";
  }
  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override {
    std::string vecName = getVarName("vec_", this);
    stream.Indent() << "std::vector<char> " << vecName << "(" << (size_t) * *_size << ");\n";

    for (auto& arg : _cgenericargsDict) {
      arg.get()->Declare(stream);
      stream.Indent() << "memcpy(&" << vecName << "[0] + " << arg->GetOffset() << ", &";
      arg.get()->Write(stream);
      stream << ", " << (size_t)arg->GetSize() << ");\n";
    }
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

class CVkPipelineCacheCreateInfo_V1 : public CArgument, gits::noncopyable {
  std::unique_ptr<CVkStructureType> _sType;
  std::unique_ptr<CpNextWrapper> _pNext;
  std::unique_ptr<Cuint32_t> _flags;
  std::unique_ptr<Csize_t> _initialDataSize;
  std::unique_ptr<CDeclaredBinaryResource> _pInitialData;
  std::vector<uint8_t> _initialData;

  std::unique_ptr<VkPipelineCacheCreateInfo> _PipelineCacheCreateInfo;
  std::unique_ptr<VkPipelineCacheCreateInfo> _PipelineCacheCreateInfoOriginal;
  Cbool _isNullPtr;

public:
  CVkPipelineCacheCreateInfo_V1();
  CVkPipelineCacheCreateInfo_V1(const VkPipelineCacheCreateInfo* pipelinecachecreateinfo);
  static const char* NAME;
  virtual const char* Name() const override {
    return NAME;
  }
  VkPipelineCacheCreateInfo* Value();

  PtrConverter<VkPipelineCacheCreateInfo> operator*() {
    return PtrConverter<VkPipelineCacheCreateInfo>(Value());
  }
  PtrConverter<VkPipelineCacheCreateInfo> Original();
  void* GetPtrType() override {
    return (void*)Value();
  }
  virtual std::set<uint64_t> GetMappedPointers();
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual bool AmpersandNeeded() const override;
  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override;
};

typedef struct VkBuffer_T* VkBufferTypeTag;
typedef CVulkanObj<VkBuffer, VkBufferTypeTag> CVkBuffer;
class CBufferDeviceAddressObjectData;

class CBufferDeviceAddressObject : public CArgument, gits::noncopyable {
  std::unique_ptr<Cuint64_t> _originalDeviceAddress;
  std::unique_ptr<CVkBuffer> _buffer;
  std::unique_ptr<Cint64_t> _offset;
  VkDeviceAddress _deviceAddress;

public:
  CBufferDeviceAddressObject()
      : _originalDeviceAddress(std::make_unique<Cuint64_t>()),
        _buffer(std::make_unique<CVkBuffer>()),
        _offset(std::make_unique<Cint64_t>()),
        _deviceAddress(0) {}

  CBufferDeviceAddressObject(VkDeviceAddress deviceAddress);
  CBufferDeviceAddressObject(const CBufferDeviceAddressObjectData& bufferDeviceAddressObject);

  VkDeviceAddress Value();

  // Manual definition due to different cast operators
  struct PtrConverter {
  private:
    VkDeviceAddress _deviceAddress;

  public:
    explicit PtrConverter(VkDeviceAddress deviceAddress) : _deviceAddress(deviceAddress) {}
    operator VkDeviceAddress() const {
      return _deviceAddress;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
  PtrConverter Original() {
    return PtrConverter(**_originalDeviceAddress);
  }

  virtual std::set<uint64_t> GetMappedPointers();

  virtual void Write(CBinOStream& stream) const override {
    _originalDeviceAddress->Write(stream);
    _buffer->Write(stream);
    _offset->Write(stream);
  }

  virtual void Read(CBinIStream& stream) override {
    _originalDeviceAddress->Read(stream);
    _buffer->Read(stream);
    _offset->Read(stream);
  }

  const char* Name() const override {
    return "CBufferDeviceAddressObject";
  }

  virtual void Write(CCodeOStream& stream) const override {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
};

typedef CVulkanEnum<VkCommandExecutionSideGITS> CVkCommandExecutionSideGITS;
struct CVkDeviceOrHostAddressConstKHRData;

class CVkDeviceOrHostAddressConstKHR : public CArgument {
  std::unique_ptr<CVkCommandExecutionSideGITS> _commandExecutionSideGITS;
  std::unique_ptr<CBufferDeviceAddressObject> _bufferDeviceAddress;

  std::unique_ptr<Csize_t> _dataSize;
  std::unique_ptr<CDeclaredBinaryResource> _resource;
  std::vector<uint8_t> _data;

  std::unique_ptr<VkDeviceOrHostAddressConstKHR> _DeviceOrHostAddress;
  std::unique_ptr<VkDeviceOrHostAddressConstKHR> _DeviceOrHostAddressOriginal;

public:
  CVkDeviceOrHostAddressConstKHR()
      : _commandExecutionSideGITS(std::make_unique<CVkCommandExecutionSideGITS>()),
        _bufferDeviceAddress(std::make_unique<CBufferDeviceAddressObject>()),
        _dataSize(std::make_unique<Csize_t>()),
        _resource(std::make_unique<CDeclaredBinaryResource>()),
        _data(),
        _DeviceOrHostAddress(nullptr),
        _DeviceOrHostAddressOriginal(nullptr) {}

  CVkDeviceOrHostAddressConstKHR(const VkDeviceOrHostAddressConstKHR deviceorhostaddress,
                                 const CVkDeviceOrHostAddressConstKHRData& deviceOrHostAddressData);

  virtual ~CVkDeviceOrHostAddressConstKHR() {}

  virtual const char* Name() const override {
    return "VkDeviceOrHostAddressConstKHR";
  }

  VkDeviceOrHostAddressConstKHR* Value();

  PtrConverter<VkDeviceOrHostAddressConstKHR> operator*() {
    return PtrConverter<VkDeviceOrHostAddressConstKHR>(Value());
  }

  PtrConverter<VkDeviceOrHostAddressConstKHR> Original();

  void* GetPtrType() override {
    return (void*)Value();
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return _bufferDeviceAddress->GetMappedPointers();
  }

  virtual void Write(CBinOStream& stream) const override;

  virtual void Read(CBinIStream& stream) override;

  virtual void Write(CCodeOStream& stream) const override {
    TODO("Implement proper CCode support.")
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }

  virtual bool DeclarationNeeded() const override {
    return true;
  }

  virtual void Declare(CCodeOStream& stream) const override {
    TODO("Implement proper CCode support.")
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
};

class CDeviceOrHostAddressAccelerationStructureVertexDataGITS
    : public CVkDeviceOrHostAddressConstKHR {

public:
  CDeviceOrHostAddressAccelerationStructureVertexDataGITS() : CVkDeviceOrHostAddressConstKHR() {}

  CDeviceOrHostAddressAccelerationStructureVertexDataGITS(
      VkDeviceOrHostAddressConstKHR vertexData,
      const CVkDeviceOrHostAddressConstKHRData& deviceOrHostAddressData)
      : CVkDeviceOrHostAddressConstKHR(vertexData, deviceOrHostAddressData) {
    // Implementation of this class is exactly the same as CVkDeviceOrHostAddressConstKHR.
    // The difference is in the struct storage counterpart.
  }

  virtual const char* Name() const override {
    return "CDeviceOrHostAddressAccelerationStructureVertexDataGITS";
  }
};

struct CVkDeviceOrHostAddressKHRData;

class CVkDeviceOrHostAddressKHR : public CArgument {
  std::unique_ptr<CVkCommandExecutionSideGITS> _commandExecutionSideGITS;
  std::unique_ptr<CBufferDeviceAddressObject> _bufferDeviceAddress;

  std::unique_ptr<VkDeviceOrHostAddressKHR> _DeviceOrHostAddress;
  std::unique_ptr<VkDeviceOrHostAddressKHR> _DeviceOrHostAddressOriginal;

public:
  CVkDeviceOrHostAddressKHR()
      : _commandExecutionSideGITS(std::make_unique<CVkCommandExecutionSideGITS>()),
        _bufferDeviceAddress(std::make_unique<CBufferDeviceAddressObject>()),
        _DeviceOrHostAddress(nullptr),
        _DeviceOrHostAddressOriginal(nullptr) {}

  CVkDeviceOrHostAddressKHR(const VkDeviceOrHostAddressKHR deviceorhostaddress,
                            const CVkDeviceOrHostAddressKHRData& deviceOrHostAddressData);

  virtual ~CVkDeviceOrHostAddressKHR() {}

  virtual const char* Name() const override {
    return "VkDeviceOrHostAddressKHR";
  }

  VkDeviceOrHostAddressKHR* Value();

  PtrConverter<VkDeviceOrHostAddressKHR> operator*() {
    return PtrConverter<VkDeviceOrHostAddressKHR>(Value());
  }

  PtrConverter<VkDeviceOrHostAddressKHR> Original();

  void* GetPtrType() override {
    return (void*)Value();
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return _bufferDeviceAddress->GetMappedPointers();
  }

  virtual void Write(CBinOStream& stream) const override;

  virtual void Read(CBinIStream& stream) override;

  virtual void Write(CCodeOStream& stream) const override {
    TODO("Implement proper CCode support.")
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }

  virtual bool DeclarationNeeded() const override {
    return true;
  }

  virtual void Declare(CCodeOStream& stream) const override {
    TODO("Implement proper CCode support.")
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
};

class CVkAccelerationStructureGeometryInstancesDataKHR : public CArgument {
  std::unique_ptr<CVkStructureType> _sType;
  std::unique_ptr<CpNextWrapper> _pNext;
  std::unique_ptr<CVkCommandExecutionSideGITS> _commandExecutionSideGITS;
  std::unique_ptr<Cuint32_t> _arrayOfPointers;
  std::unique_ptr<CBufferDeviceAddressObject> _bufferDeviceAddress;

  std::unique_ptr<Cuint32_t> _count;
  std::unique_ptr<CDeclaredBinaryResource> _resource;
  std::vector<VkAccelerationStructureInstanceKHR> _inputData;

  std::unique_ptr<VkAccelerationStructureGeometryInstancesDataKHR>
      _AccelerationStructureGeometryInstancesDataKHR;
  std::unique_ptr<VkAccelerationStructureGeometryInstancesDataKHR>
      _AccelerationStructureGeometryInstancesDataKHROriginal;
  Cbool _isNullPtr;

public:
  CVkAccelerationStructureGeometryInstancesDataKHR()
      : _sType(std::make_unique<CVkStructureType>()),
        _pNext(std::make_unique<CpNextWrapper>()),
        _commandExecutionSideGITS(std::make_unique<CVkCommandExecutionSideGITS>()),
        _arrayOfPointers(std::make_unique<Cuint32_t>()),
        _bufferDeviceAddress(std::make_unique<CBufferDeviceAddressObject>()),
        _count(std::make_unique<Cuint32_t>()),
        _inputData(),
        _AccelerationStructureGeometryInstancesDataKHR(nullptr),
        _AccelerationStructureGeometryInstancesDataKHROriginal(nullptr),
        _isNullPtr(false) {}

  CVkAccelerationStructureGeometryInstancesDataKHR(
      const VkAccelerationStructureGeometryInstancesDataKHR*
          accelerationstructuregeometryinstancesdatakhr,
      const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
      const VkAccelerationStructureBuildControlDataGITS& controlData);

  virtual const char* Name() const override {
    return "VkAccelerationStructureGeometryInstancesDataKHR";
  }

  VkAccelerationStructureGeometryInstancesDataKHR* Value();

  PtrConverter<VkAccelerationStructureGeometryInstancesDataKHR> operator*() {
    return PtrConverter<VkAccelerationStructureGeometryInstancesDataKHR>(Value());
  }

  PtrConverter<VkAccelerationStructureGeometryInstancesDataKHR> Original();

  void* GetPtrType() override {
    return (void*)Value();
  }

  virtual std::set<uint64_t> GetMappedPointers();
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual bool AmpersandNeeded() const override;
  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override;
};

typedef CVulkanEnum<VkGeometryTypeKHR> CVkGeometryTypeKHR;
class CVkAccelerationStructureGeometryTrianglesDataKHR;
class CVkAccelerationStructureGeometryAabbsDataKHR;
class CVkAccelerationStructureGeometryInstancesDataKHR;

class CVkAccelerationStructureGeometryDataKHR : public CArgument {
  std::unique_ptr<CVkGeometryTypeKHR> _geometryType;
  std::unique_ptr<CVkAccelerationStructureGeometryTrianglesDataKHR> _triangles;
  std::unique_ptr<CVkAccelerationStructureGeometryAabbsDataKHR> _aabbs;
  std::unique_ptr<CVkAccelerationStructureGeometryInstancesDataKHR> _instances;

  std::unique_ptr<VkAccelerationStructureGeometryDataKHR> _AccelerationStructureGeometryDataKHR;
  std::unique_ptr<VkAccelerationStructureGeometryDataKHR>
      _AccelerationStructureGeometryDataKHROriginal;
  Cbool _isNullPtr;

public:
  CVkAccelerationStructureGeometryDataKHR()
      : _geometryType(std::make_unique<CVkGeometryTypeKHR>()),
        _triangles(std::make_unique<CVkAccelerationStructureGeometryTrianglesDataKHR>()),
        _aabbs(std::make_unique<CVkAccelerationStructureGeometryAabbsDataKHR>()),
        _instances(std::make_unique<CVkAccelerationStructureGeometryInstancesDataKHR>()),
        _AccelerationStructureGeometryDataKHR(nullptr),
        _AccelerationStructureGeometryDataKHROriginal(nullptr),
        _isNullPtr(false) {}

  CVkAccelerationStructureGeometryDataKHR(
      VkGeometryTypeKHR geometryType,
      const VkAccelerationStructureGeometryDataKHR* accelerationstructuregeometrydatakhr,
      const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo,
      VkAccelerationStructureBuildControlDataGITS controlData);

  virtual const char* Name() const override {
    return "VkAccelerationStructureGeometryDataKHR";
  }
  VkAccelerationStructureGeometryDataKHR* Value();

  PtrConverter<VkAccelerationStructureGeometryDataKHR> operator*() {
    return PtrConverter<VkAccelerationStructureGeometryDataKHR>(Value());
  }

  PtrConverter<VkAccelerationStructureGeometryDataKHR> Original();

  void* GetPtrType() override {
    return (void*)Value();
  }

  virtual std::set<uint64_t> GetMappedPointers();
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual bool AmpersandNeeded() const override;
  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override;
};

class CVkMemoryBarrier2;
typedef CStructArray<VkMemoryBarrier2, CVkMemoryBarrier2> CVkMemoryBarrier2Array;
class CVkBufferMemoryBarrier2;
typedef CStructArray<VkBufferMemoryBarrier2, CVkBufferMemoryBarrier2> CVkBufferMemoryBarrier2Array;
class CVkImageMemoryBarrier2;
typedef CStructArray<VkImageMemoryBarrier2, CVkImageMemoryBarrier2> CVkImageMemoryBarrier2Array;

class CVkDependencyInfo : public CArgument, gits::noncopyable {
  std::unique_ptr<CVkStructureType> _sType;
  std::unique_ptr<CpNextWrapper> _pNext;
  std::unique_ptr<Cuint32_t> _dependencyFlags;
  std::unique_ptr<Cuint32_t> _memoryBarrierCount;
  std::unique_ptr<CVkMemoryBarrier2Array> _pMemoryBarriers;
  std::unique_ptr<Cuint32_t> _bufferMemoryBarrierCount;
  std::unique_ptr<CVkBufferMemoryBarrier2Array> _pBufferMemoryBarriers;
  std::unique_ptr<Cuint32_t> _imageMemoryBarrierCount;
  std::unique_ptr<CVkImageMemoryBarrier2Array> _pImageMemoryBarriers;

  std::unique_ptr<VkDependencyInfo> _DependencyInfo;
  std::unique_ptr<VkDependencyInfo> _DependencyInfoOriginal;
  Cbool _isNullPtr;

public:
  CVkDependencyInfo();
  CVkDependencyInfo(const VkDependencyInfo* dependencyinfo);
  static const char* NAME;
  virtual const char* Name() const override {
    return NAME;
  }
  VkDependencyInfo* Value();

  PtrConverter<VkDependencyInfo> operator*() {
    return PtrConverter<VkDependencyInfo>(Value());
  }
  PtrConverter<VkDependencyInfo> Original();
  void* GetPtrType() override {
    return (void*)Value();
  }
  virtual std::set<uint64_t> GetMappedPointers();
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual bool AmpersandNeeded() const override;
  virtual bool DeclarationNeeded() const override {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const override;
  void Declare(CCodeOStream& stream,
               size_t memoryBarrierStart,
               size_t memoryBarrierEnd,
               size_t bufferMemoryBarrierStart,
               size_t bufferMemoryBarrierEnd,
               size_t imageMemoryBarrierStart,
               size_t imageMemoryBarrierEnd) const;
  uint32_t GetMemoryBarrierCount() const;
  uint32_t GetBufferMemoryBarrierCount() const;
  uint32_t GetImageMemoryBarrierCount() const;
};
} // namespace Vulkan
} // namespace gits
