// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "argument.h"

#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0Log.h"
#include "l0Tools.h"

namespace gits {
namespace l0 {
class CProgramSourceArray;

template <typename T, typename T_WRAP>
class CArg : public CArgument {
public:
  typedef T L0Type;
  typedef T_WRAP GITSType;

private:
  L0Type _value;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<L0Type, GITSType> CSArray;
  CArg() : _value(){};
  CArg(L0Type& value) : _value(value){};
  L0Type& operator*() {
    return _value;
  }
  L0Type& Original() {
    return _value;
  }
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _value);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _value);
  }
  virtual std::string ToString() const {
    return ToStringHelper(Value());
  }

  L0Type& Value() {
    return _value;
  }
  const L0Type& Value() const {
    return _value;
  }
};

template <class T, class T_WRAP>
const char* CArg<T, T_WRAP>::NAME = T_WRAP::NAME;

template <class T, class T_WRAP>
class CArgHandle : public CArgument {
  T key_;

public:
  static const char* NAME;
  typedef T L0Type;
  typedef CArgumentMappedSizedArray<T, T_WRAP, gits::ADD_MAPPING> CSMapArray;
  typedef CArgumentMappedSizedArray<T, T_WRAP, gits::NO_ACTION> CSArray;

  CArgHandle() : key_(nullptr) {}
  CArgHandle(T arg) : key_(arg) {}
  CArgHandle(T* arg) : key_(*arg) {}

  static void AddMapping(T key, T value) {
    get_map()[key] = value;
    T_WRAP::AddMutualMapping(key, value);
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
    if (CheckMapping(key)) {
      get_map().erase(key);
    }
    T_WRAP::RemoveMutualMapping(key);
  }

  static void RemoveMapping(const T* keys, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      RemoveMapping(keys[i]);
    }
  }

  static T GetMapping(T key) {
    if (key == nullptr) {
      return nullptr;
    }

    auto& the_map = get_map();
    auto iter = the_map.find(key);
    if (iter == the_map.end()) {
      LOG_ERROR << "Couldn't map L0 handle name " << key;
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    return iter->second;
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
    auto& the_map = get_map();
    return the_map.find(key) != the_map.end();
  }

  bool CheckMapping() {
    return CheckMapping(key_);
  }

  static T GetOriginal(T value) {
    const auto& map = get_map();
    for (auto it = map.begin(); it != map.end(); ++it) {
      if (it->second == value) {
        return it->first;
      }
    }
    LOG_ERROR << "Couldn't find the original Level Zero object " << value;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  virtual const char* Name() const {
    return NAME;
  }
  static const char* TypeNameStr() {
    return NAME;
  }
  static const char* WrapTypeNameStr() {
    return "CArgHandle";
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

  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, key_);
  }

  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, key_);
  }

  void Assign(T other) {
    AddMapping(other);
  }

private:
  typedef std::unordered_map<T, T> name_map_t;
  static name_map_t& get_map() {
    INIT_NEW_STATIC_OBJ(objects_map, name_map_t)
    static bool initialized = false;
    if (!initialized) {
      objects_map[0] = 0;
      initialized = true;
    }
    return objects_map;
  }
};

template <class T, class T_WRAP>
const char* gits::l0::CArgHandle<T, T_WRAP>::NAME = T_WRAP::NAME;

class COutArgument : public CArgument {
public:
  static const size_t space_provided = 64 * 1024;

  COutArgument() {}
  template <typename... T>
  COutArgument(T...) {}

  class ToAnyPtr {
    void* ptr_ = nullptr;

  public:
    explicit ToAnyPtr(void* ptr) : ptr_(ptr) {}
    template <class T>
    operator T*() {
      return (T*)ptr_;
    }
  };

  ToAnyPtr operator*() {
    static std::vector<char> space(space_provided);
    return ToAnyPtr(&space[0]);
  }

  virtual bool Array() const {
    return false;
  }
  virtual const char* Name() const {
    return "<out_arg>";
  }
  virtual void Write([[maybe_unused]] CBinOStream& stream) const { /* Do nothing */
  }
  virtual void Read([[maybe_unused]] CBinIStream& stream) { /* Do nothing */
  }
};

class CvoidPtr : public CArg<void*, CvoidPtr> {
public:
  typedef CArgumentSizedArray<void*, CvoidPtr> CSArray;
  static const char* NAME;
  CvoidPtr() : CArg() {}
  CvoidPtr(const void* value) : CArg() {
    Value() = const_cast<void*>(value);
  }
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write([[maybe_unused]] CBinOStream& stream) const { /* Do nothing */
  }
  virtual void Read([[maybe_unused]] CBinIStream& stream) { /* Do nothing */
  }

  struct PtrConverter {
  public:
    explicit PtrConverter([[maybe_unused]] void* ptr) {}
    operator void**() {
      return nullptr;
    }
    operator void*() const {
      return nullptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(nullptr);
  }
};

class Cze_host_pfn_t : public CArg<void(__zecall*)(void*), Cze_host_pfn_t> {
public:
  static const char* NAME;
  static void __zecall Callback(void*) {}
  Cze_host_pfn_t() : CArg() {}
  Cze_host_pfn_t(L0Type value) : CArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Read(CBinIStream& stream);
};

class CKernelArgValue : public CArgument {
  bool _localMemoryScanned = false;

protected:
  size_t _size = 0U;
  void* _ptr = nullptr;
  const void* _obj = nullptr;
  std::vector<char> _buffer;

public:
  CKernelArgValue() = default;
  CKernelArgValue(const size_t len, const void* buffer);

  virtual const char* Name() const {
    return "void*";
  }
  virtual size_t Length() const {
    return _size;
  }

  const void* Value() const {
    return !_buffer.empty() ? _buffer.data() : _ptr;
  }

  const void* operator*();
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
};

/** @class CMappedPtr
     *  @brief Thin pointer wrapper.
     *
     *  Provides mapping, should be used in places where only device allocation
     *  can appear.
     */
class CMappedPtr : public CArgHandle<void*, CMappedPtr> {
public:
  static const char* NAME;
  CMappedPtr() : CArgHandle() {}
  CMappedPtr(void* arg) : CArgHandle(arg) {}
  CMappedPtr(const void* arg) : CArgHandle(const_cast<void*>(arg)) {}
  static void AddMutualMapping([[maybe_unused]] void* key, [[maybe_unused]] void* value){};
  static void RemoveMutualMapping([[maybe_unused]] void* key){};
  static bool InitializedWithOriginal() {
    return true;
  }
};

/** @class CUSMPtr
     *  @brief Hybrid pointer. Can act as mapped pointer or buffer.
     *
     *  During recording detects whether passed pointer is host or device
     *  allocation. If host - saves the buffer. If device - acts like mapped
     *  pointer.
     *  Should be used where any kind of allocation can appear.
     */
class CUSMPtr : public CArgument {
protected:
  size_t _size = 0U;
  void* _ptr = nullptr;
  CBinaryResource _resource;
  std::vector<char> _temp_buffer;

public:
  bool IsMappedPointer() const {
    return _resource.GetResourceHash() == CResourceManager::EmptyHash;
  }

  // PointerProxy allows us to differentiate between srcptr and dstptr
  class PointerProxy {
    CUSMPtr* _ptr;

  public:
    PointerProxy(CUSMPtr* ptr) : _ptr(ptr) {}
    char* GetResourceData(void* originalSrcPtr) {
      auto& sd = SD();
      CBinaryResource::PointerProxy data(_ptr->_resource.Data());
      _ptr->_temp_buffer.assign((const char*)data, (const char*)data + data.Size());
      const auto allocInfo = GetAllocFromOriginalPtr(originalSrcPtr, sd);
      if (allocInfo.first != nullptr) {
        auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
        TranslatePointerOffsets(sd, _ptr->_temp_buffer.data(), allocState.indirectPointersOffsets,
                                true);
      }
      return _ptr->_temp_buffer.data();
    }
    // srcptr - no need to copy from resource to temporary buffer
    operator const void*() {
      if (_ptr->IsMappedPointer()) {
        auto& sd = SD();
        const auto allocInfo = GetAllocFromOriginalPtr(_ptr->_ptr, sd);
        if (allocInfo.first == nullptr && !sd.scanningGlobalPointersMode.empty()) {
          return GetMappedGlobalPtrFromOriginalAllocation(sd, _ptr->_ptr);
        }
        return GetOffsetPointer(allocInfo.first, allocInfo.second);
      }
      return reinterpret_cast<const void*>(GetResourceData(_ptr->_ptr));
    }
    // dstptr - CBinaryResource is read-only so we need a temporary buffer
    operator void*() {
      if (!_ptr->IsMappedPointer()) {
        return reinterpret_cast<void*>(GetResourceData(_ptr->_ptr));
      }
      auto& sd = SD();
      const auto allocInfo = GetAllocFromOriginalPtr(_ptr->_ptr, sd);
      if (allocInfo.first == nullptr && !sd.scanningGlobalPointersMode.empty()) {
        return GetMappedGlobalPtrFromOriginalAllocation(sd, _ptr->_ptr);
      }
      return GetOffsetPointer(allocInfo.first, allocInfo.second);
    }
    operator uint64_t*() {
      if (!_ptr->IsMappedPointer()) {
        return reinterpret_cast<uint64_t*>(GetResourceData(_ptr->_ptr));
      }
      auto& sd = SD();
      const auto allocInfo = GetAllocFromOriginalPtr(_ptr->_ptr, sd);
      if (allocInfo.first == nullptr && !sd.scanningGlobalPointersMode.empty()) {
        return reinterpret_cast<uint64_t*>(
            GetMappedGlobalPtrFromOriginalAllocation(sd, _ptr->_ptr));
      }
      return reinterpret_cast<uint64_t*>(GetOffsetPointer(allocInfo.first, allocInfo.second));
    }
  };

  CUSMPtr() : _resource(){};
  CUSMPtr(void* mappedPtr) : _ptr(mappedPtr), _resource() {}
  CUSMPtr(const void* mappedPtr) : _ptr(const_cast<void*>(mappedPtr)), _resource() {}
  CUSMPtr(const size_t len, const void* buffer) : _size(len), _resource() {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(buffer), sd);
    if (_size != 0U && buffer != nullptr && allocInfo.first == nullptr) {
      if (!sd.scanningGlobalPointersMode.empty()) {
        for (const auto& allocState : sd.Map<CAllocState>()) {
          const auto foundPointer =
              IsPointerInsideAllocation(buffer, allocState.second->globalPtrAllocation);
          if (foundPointer) {
            LOG_TRACEV << "Found device pointer address inside global allocation: " << buffer;
            _ptr = const_cast<void*>(buffer);
            return;
          }
        }
      }
      _resource.reset(RESOURCE_BUFFER, buffer, _size);
    } else {
      _ptr = GetOffsetPointer(allocInfo.first, allocInfo.second);
    }
  }
  CUSMPtr(uint64_t* pointerValue) : _size(sizeof(uint64_t)), _resource() {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(static_cast<void*>(pointerValue), sd);
    if (_size != 0U && pointerValue != nullptr && allocInfo.first == nullptr) {
      _resource.reset(RESOURCE_BUFFER, &pointerValue, _size);
    } else {
      _ptr = GetOffsetPointer(allocInfo.first, allocInfo.second);
    }
  }
  CUSMPtr(void* ptr, ze_image_handle_t image) : _ptr(ptr) {
    auto desc = SD().Get<CImageState>(image, EXCEPTION_MESSAGE).desc;
    _size = CalculateImageSize(desc);
    _resource.reset(RESOURCE_DATA_RAW, _ptr, _size);
  }
  CUSMPtr(const void* ptr, ze_image_handle_t image) : CUSMPtr(const_cast<void*>(ptr), image) {}
  CUSMPtr(const size_t len, const void* buffer, void* destPtr) : CUSMPtr(len, buffer) {
    if (_ptr == nullptr) {
      _ptr = destPtr;
    }
  }
  virtual const char* Name() const {
    return "void*";
  }
  virtual size_t Length() const {
    return _size;
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  PointerProxy Value() {
    return PointerProxy(this);
  }
  PointerProxy operator*() {
    return Value();
  }
  void FreeHostMemory();
};

template <class T, class WRAP_T>
class CL0StructArray : public CStructArray<T, WRAP_T> {
public:
  using CStructArray<T, WRAP_T>::CStructArray;
  virtual void Write(CBinOStream& stream) const override {
    CStructArray<T, WRAP_T>::Write(stream);
  }
};

struct CExtensionStructBase : public CArgument {
  static const char* NAME;
  using CArgument::CArgument;
  virtual ~CExtensionStructBase() = default;
};

class SourceFileInfo {
  static uint32_t _programSourceIdx;
  static uint32_t _binarySourceIdx;
  static std::string CreateFileName(ze_module_format_t format);

public:
  std::vector<uint8_t> data;
  std::string filename;

  SourceFileInfo() = default;
  SourceFileInfo(const size_t _size, const uint8_t* _data, ze_module_format_t format);
  SourceFileInfo(const size_t _size, const uint8_t* _data, std::string _filename);
};

class CProgramSource : public CArgumentFileText {
  const char* emptyFileName = "GITSL0EmptyFile";

  const char* textCstr = nullptr;
  size_t textLength = 0U;

  SourceFileInfo sourceFile;

public:
  typedef CProgramSourceArray CSArray;
  static constexpr const char* NAME = "const char*";

  CProgramSource() = default;
  CProgramSource(const uint8_t* text, size_t size, ze_module_format_t format);
  CProgramSource(SourceFileInfo sourceFile);

  size_t* Length();
  const char** Value();
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

  SourceFileInfo Original();

  struct PtrConverter {
  private:
    const char** _ptr;

  public:
    explicit PtrConverter(const char** ptr) : _ptr(ptr) {}
    operator const char*() const {
      if (_ptr == nullptr) {
        return nullptr;
      }
      return *_ptr;
    }
    operator const unsigned char*() const {
      if (_ptr == nullptr) {
        return nullptr;
      }
      return reinterpret_cast<const unsigned char*>(*_ptr);
    }
    operator const char**() const {
      return _ptr;
    }
    operator const void*() const {
      if (_ptr == nullptr) {
        return nullptr;
      }
      return *_ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }

private:
  static std::string GetProgramBinary(const unsigned char* binary, const size_t length);
};

class CProgramSourceArray : public CArgumentSizedArray<SourceFileInfo, CProgramSource> {
  std::vector<uint8_t*> dataVector;

  std::vector<SourceFileInfo> ConvertToArray(const size_t count,
                                             const uint8_t** data,
                                             const size_t* sizes,
                                             const ze_module_format_t format);

public:
  CProgramSourceArray() = default;
  CProgramSourceArray(const size_t count,
                      const uint8_t** data,
                      const size_t* sizes,
                      const ze_module_format_t format = ZE_MODULE_FORMAT_IL_SPIRV);
  const uint8_t** Value();
  const uint8_t** operator*() {
    return Value();
  }
};

class Cuintptr_t : public CArg<uintptr_t, Cuintptr_t> {
public:
  typedef CArgumentSizedArray<uintptr_t, Cuintptr_t> CSArray;
  static constexpr const char* NAME = "uintptr_t";
  Cuintptr_t() = default;
  Cuintptr_t(const void* value) {
    Value() = reinterpret_cast<uintptr_t>(value);
  }
  virtual const char* Name() const {
    return NAME;
  }
  operator void*() const {
    return reinterpret_cast<void*>(Value());
  }
  operator const void*() const {
    return reinterpret_cast<const void*>(Value());
  }
  struct PtrConverter {
    L0Type ptrValue = 0U;

  public:
    explicit PtrConverter(L0Type ptrValue) : ptrValue(ptrValue) {}
    operator const void*() const {
      return reinterpret_cast<void*>(ptrValue);
    }
    operator void*() const {
      return reinterpret_cast<void*>(ptrValue);
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
};

class CBinaryData : public CArgument {
private:
  mutable CBinaryResource _resource;

protected:
  size_t _size = 0U;
  void* _ptr = nullptr;
  std::vector<char> _buffer;

public:
  CBinaryData() : _size(0), _ptr(0) {}
  CBinaryData(const size_t size, const void* buffer);
  ~CBinaryData() = default;
  virtual const char* Name() const {
    return "void*";
  }
  virtual size_t Length() const {
    return _size;
  }

  void* Value() {
    return !_buffer.empty() ? _buffer.data() : _ptr;
  }
  const void* Value() const {
    return !_buffer.empty() ? _buffer.data() : _ptr;
  }
  void* operator*() {
    return Value();
  }
  const void* operator*() const {
    return Value();
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual std::string ToString() const override {
    return ToStringHelper(_ptr);
  }
};
} // namespace l0
} // namespace gits
