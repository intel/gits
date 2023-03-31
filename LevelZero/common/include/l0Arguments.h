// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "argument.h"

#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0Log.h"
#include "l0StateTracking.h"
#include "l0Tools.h"

namespace gits {
namespace l0 {

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
  virtual void Write(CCodeOStream& stream) const {
    stream << ToString();
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
      if (GetRefCount(GetMapping(key)) == 0) {
        get_map().erase(key);
      }
    }
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
      Log(ERR) << "Couldn't map L0 handle name " << key;
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
    Log(ERR) << "Couldn't find the original Level Zero object " << value;
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

  virtual void Declare(CCodeOStream& stream) const {
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = (" << TypeNameStr()
                    << ") 0x" << ToStringHelper((void*)key_) << ";" << std::endl;
  }

  virtual void Write(CCodeOStream& stream) const {
    stream << "(" << TypeNameStr() << ") " << WrapTypeNameStr() << "::GetMapping("
           << ToStringHelper((void*)key_) << ")";
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
  virtual void Write(CBinOStream& /*stream*/) const { /* Do nothing */
  }
  virtual void Read(CBinIStream& /*stream*/) { /* Do nothing */
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << "outArg()";
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
  virtual void Write(CBinOStream& /*stream*/) const { /* Do nothing */
  }
  virtual void Read(CBinIStream& /*stream*/) { /* Do nothing */
  }
  virtual void Write(CCodeOStream& stream) const;

  struct PtrConverter {
  public:
    explicit PtrConverter(void* /*ptr*/) {}
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
  virtual void Write(CCodeOStream& stream) const;
};

class CKernelArgValue : public CArgument {
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
  virtual void Write(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const;
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

private:
  bool IsMappedPointer() const {
    return _resource.GetResourceHash() == CResourceManager::EmptyHash;
  }

public:
  // PointerProxy allows us to differentiate between srcptr and dstptr
  class PointerProxy {
    CUSMPtr* _ptr;

  public:
    PointerProxy(CUSMPtr* ptr) : _ptr(ptr) {}
    // srcptr - no need to copy from resource to temporary buffer
    operator const void*() const {
      if (_ptr->IsMappedPointer()) {
        auto& sd = SD();
        const auto allocInfo = GetAllocFromOriginalPtr(_ptr->_ptr, sd);
        if (allocInfo.first == nullptr && !sd.scanningGlobalPointersMode.empty()) {
          return GetMappedGlobalPtrFromOriginalAllocation(sd, _ptr->_ptr);
        }
        return GetOffsetPointer(allocInfo.first, allocInfo.second);
      }
      return _ptr->_resource.Data();
    }
    // dstptr - CBinaryResource is read-only so we need a temporary buffer
    operator void*() {
      CBinaryResource::PointerProxy data(_ptr->_resource.Data());
      if (!_ptr->IsMappedPointer()) {
        _ptr->_temp_buffer.assign((const char*)data, (const char*)data + data.Size());
        return _ptr->_temp_buffer.data();
      }
      auto& sd = SD();
      const auto allocInfo = GetAllocFromOriginalPtr(_ptr->_ptr, sd);
      if (allocInfo.first == nullptr && !sd.scanningGlobalPointersMode.empty()) {
        return GetMappedGlobalPtrFromOriginalAllocation(sd, _ptr->_ptr);
      }
      return GetOffsetPointer(allocInfo.first, allocInfo.second);
    }
  };

  CUSMPtr() : _resource(){};
  CUSMPtr(void* mappedPtr) : _ptr(mappedPtr), _resource() {}
  CUSMPtr(const size_t len, const void* buffer) : _size(len), _resource() {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(buffer), sd);
    if (_size != 0U && buffer != nullptr && allocInfo.first == nullptr) {
      if (!sd.scanningGlobalPointersMode.empty()) {
        for (const auto& allocState : sd.Map<CAllocState>()) {
          const auto foundPointer =
              IsPointerInsideAllocation(buffer, allocState.second->globalPtrAllocation);
          if (foundPointer) {
            Log(TRACEV) << "Found device pointer address inside global allocation: " << buffer;
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
  CUSMPtr(void* ptr, ze_image_handle_t image) : _ptr(ptr) {
    auto desc = SD().Get<CImageState>(image, EXCEPTION_MESSAGE).desc;
    _size = CalculateImageSize(desc);
    _resource.reset(RESOURCE_DATA_RAW, _ptr, _size);
  }
  CUSMPtr(const void* ptr, ze_image_handle_t image) : CUSMPtr(const_cast<void*>(ptr), image) {}

  virtual const char* Name() const {
    return "void*";
  }
  virtual size_t Length() const {
    return _size;
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
  PointerProxy Value() {
    return PointerProxy(this);
  }
  PointerProxy operator*() {
    return Value();
  }
};

template <class T, class WRAP_T>
class CL0StructArray : public CStructArray<T, WRAP_T> {
public:
  using CStructArray<T, WRAP_T>::CStructArray;
  virtual void Declare(CCodeOStream& stream) const override {
    for (const auto& ptr : CStructArray<T, WRAP_T>::Vector()) {
      if (ptr->DeclarationNeeded()) {
        ptr->Declare(stream);
      }
      ptr->Write(stream);
    }
  }
  virtual void Write(CBinOStream& stream) const override {
    CStructArray<T, WRAP_T>::Write(stream);
  }
  virtual void Write(CCodeOStream& stream) const override {
    for (const auto& ptr : CStructArray<T, WRAP_T>::Vector()) {
      stream << stream.VariableName(ptr->ScopeKey());
    }
    if (CStructArray<T, WRAP_T>::Vector().empty()) {
      stream << "nullptr";
    }
  }
  virtual bool AmpersandNeeded() const {
    return !CStructArray<T, WRAP_T>::Vector().empty();
  }
};

struct CExtensionStructBase : public CArgument {
  static const char* NAME;
  using CArgument::CArgument;
  virtual ~CExtensionStructBase() = default;
};

} // namespace l0
} // namespace gits
