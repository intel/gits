// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclHeader.h"
#include "openclTools.h"

#include "argument.h"

#include <string>
#include <vector>

namespace gits {
namespace OpenCL {
template <typename T, typename T_WRAP>
class CCLArg : public CArgument {
public:
  typedef T CLType;
  typedef T_WRAP GITSType;

private:
  CLType _value;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<CLType, GITSType> CSArray;
  CCLArg() : _value(){};
  CCLArg(CLType& value) : _value(value){};
  CLType& operator*() {
    return _value;
  }
  CLType& Original() {
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

  CLType& Value() {
    return _value;
  }
  const CLType& Value() const {
    return _value;
  }
};

template <class T, class T_WRAP>
const char* gits::OpenCL::CCLArg<T, T_WRAP>::NAME = T_WRAP::NAME;

template <class T, class T_WRAP>
class CCLArgObj : public CArgument {
  T key_;

public:
  static const char* NAME;
  typedef T CLType;
  typedef CArgumentMappedSizedArray<T, T_WRAP, gits::ADD_MAPPING> CSMapArray;
  typedef CArgumentMappedSizedArray<T, T_WRAP, gits::NO_ACTION> CSArray;

  CCLArgObj() = default;
  CCLArgObj(T arg) : key_(arg) {}
  CCLArgObj(T* arg) : key_(*arg) {}

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
      LOG_ERROR << "Couldn't map OpenCL object name " << key;
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

  static T GetOriginal(T value) {
    const auto& map = get_map();
    for (auto it = map.begin(); it != map.end(); ++it) {
      if (it->second == value) {
        return it->first;
      }
    }
    LOG_ERROR << "Couldn't find the original OpenCL object " << value;
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  bool CheckMapping() {
    return CheckMapping(key_);
  }

  virtual const char* Name() const {
    return NAME;
  }
  static const char* TypeNameStr() {
    return NAME;
  }
  static const char* WrapTypeNameStr() {
    return "CCLArgObj";
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
const char* gits::OpenCL::CCLArgObj<T, T_WRAP>::NAME = T_WRAP::NAME;
} // namespace OpenCL
} // namespace gits
