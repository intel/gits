// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helper.h"

#ifdef GITS_API_OCL
#include "openclDrivers.h"
#endif
#include <algorithm>
#include "mapping.h"
#include "bit_range.h"
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

using namespace gits::OpenCL;
using gits::OpenCL::COclDriver;
using gits::OpenCL::drvOcl;

class CCLArgObj : public CArgument {
  intptr_t key_;

public:
  CCLArgObj() {}
  CCLArgObj(intptr_t arg) : key_(arg) {}
  CCLArgObj(intptr_t* arg) : key_(*arg) {}

  static void AddMapping(intptr_t key, intptr_t value) {
    get_map()[key] = value;
  }

  template <class T, class U>
  static void AddMapping(T key, U value) {
    AddMapping((intptr_t)key, (intptr_t)value);
  }

  void AddMapping(intptr_t value) {
    AddMapping(key_, value);
  }

  static void AddMapping(const intptr_t* keys, const intptr_t* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      AddMapping(keys[i], values[i]);
    }
  }

  template <class T, class U>
  static void AddMapping(const T* keys, const U* values, size_t num) {
    AddMapping((const intptr_t*)keys, (const intptr_t*)values, num);
  }

  void RemoveMapping() {
    RemoveMapping(key_);
  }

  static void RemoveMapping(intptr_t key) {
    if (CheckMapping(key)) {
      get_map().erase(key);
    }
  }

  static void RemoveMapping(const intptr_t* keys, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      RemoveMapping(keys[i]);
    }
  }

  static intptr_t GetMapping(intptr_t key) {
    auto& the_map = get_map();
    auto iter = the_map.find(key);
    if (iter == the_map.end()) {
      Log(ERR) << "Couldn't map OpenCL object name " << key;
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    return iter->second;
  }

  static void GetMapping(const intptr_t* keys, intptr_t* values, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      values[i] = GetMapping(keys[i]);
    }
  }

  static std::vector<intptr_t> GetMapping(const intptr_t* keys, size_t num) {
    std::vector<intptr_t> v;
    v.reserve(num);
    for (size_t i = 0; i < num; ++i) {
      v.push_back(GetMapping(keys[i]));
    }
    return v;
  }

  static bool CheckMapping(intptr_t key) {
    auto& the_map = get_map();
    return the_map.find(key) != the_map.end();
  }

  bool CheckMapping() {
    return CheckMapping(key_);
  }

private:
  typedef std::unordered_map<intptr_t, intptr_t> name_map_t;
  static name_map_t& get_map() {
    INIT_NEW_STATIC_OBJ(objects_map, name_map_t)
    static bool initialized = false;
    if (!initialized) {
      objects_map[0] = 0;
    }
    return objects_map;
  }
};

namespace api {
#define DECLARE_VARIABLE_OCL_FUNCTION(a, b, c, e, f) extern a(STDCALL*& b) c;
OCL_FUNCTIONS(DECLARE_VARIABLE_)
} // namespace api
