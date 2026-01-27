// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace gits {
namespace DirectX {
namespace ccode {

struct CppParameterInfo {
  std::string type;
  std::string name;
  bool isPtr{false};
  std::optional<unsigned> size;

  // Indices for multi-dimensional arrays
  // E.g. if a struct array contains nested structs
  std::vector<unsigned> parentIndices{};
  // Index in the array
  bool isArrayElement{false};
  unsigned index{0};

  CppParameterInfo() = default;
  CppParameterInfo(std::string type, std::string name)
      : type(std::move(type)), name(std::move(name)) {}
  CppParameterInfo(std::string type, std::string name, const CppParameterInfo& parentInfo)
      : CppParameterInfo(type, name) {
    setParentIndices(parentInfo);
  }

  bool isSizeZero() const {
    return size.has_value() && size.value() == 0;
  }

  void setParentIndices(const CppParameterInfo& parentInfo) {
    parentIndices = parentInfo.parentIndices;
    if (parentInfo.isArrayElement) {
      parentIndices.push_back(parentInfo.index);
    }
  }

  std::string getIndexedName() const {
    if (!isArrayElement && parentIndices.empty()) {
      return name;
    }

    std::string indexedName = name;
    // Change the name based on parent indices
    for (size_t i = 0; i < parentIndices.size(); ++i) {
      indexedName += "_" + std::to_string(parentIndices[i]);
    }
    // Change name based on own index
    if (isArrayElement) {
      indexedName += "[" + std::to_string(index) + "]";
    }
    return indexedName;
  }
};

struct CppParameterOutput {
  std::string initialization;
  std::string value;
  std::string decorator;
};

} // namespace ccode
} // namespace DirectX
} // namespace gits
