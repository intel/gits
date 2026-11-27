// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "vulkanHeader2.h"

#include <algorithm>
#include <locale>
#include <string_view>
#include <type_traits>

namespace gits {
namespace vulkan {

inline bool EqualPhysicalDeviceFeatureNames(std::string_view left, std::string_view right) {
  const auto& locale = std::locale::classic();
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin(), [&locale](char lhs, char rhs) {
           return std::tolower(lhs, locale) == std::tolower(rhs, locale);
         });
}

inline std::string_view GetPhysicalDeviceFeatureIdentity(std::string_view structureName,
                                                         std::string_view memberName) {
% for structure in [core_features] + chain_features:
<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif
  if (EqualPhysicalDeviceFeatureNames(structureName, "${structure.name}")\
% for alias in structure.aliases:
 || EqualPhysicalDeviceFeatureNames(structureName, "${alias}")\
% endfor
      ) {
% for member in feature_members(structure):
    if (EqualPhysicalDeviceFeatureNames(memberName, "${member.name}")) {
      return "${feature_identities[(structure.name, member.name)]}";
    }
% endfor
    return {};
  }
% if define:
#endif
% endif
% endfor
  return {};
}

inline bool IsKnownPhysicalDeviceFeature(std::string_view selector) {
  const auto separator = selector.find('.');
  if (separator != std::string_view::npos) {
    return !GetPhysicalDeviceFeatureIdentity(selector.substr(0, separator),
                                             selector.substr(separator + 1)).empty();
  }
% for structure in [core_features] + chain_features:
<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif
% for member in feature_members(structure):
  if (EqualPhysicalDeviceFeatureNames(selector, "${member.name}")) {
    return true;
  }
% endfor
% if define:
#endif
% endif
% endfor
  return false;
}

template <typename Features, typename Visitor>
void VisitPhysicalDeviceFeatures(Features& features, Visitor&& visitor) {
% for member in feature_members(core_features):
  visitor("${core_features.name}", "${member.name}", features.${member.name});
% endfor
}

template <typename Base, typename Visitor>
void VisitPhysicalDeviceFeatureChainImpl(Base* node, Visitor&& visitor) {
  for (; node; node = node->pNext) {
    switch (node->sType) {
    case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2: {
      using Features = std::conditional_t<std::is_const_v<Base>, const VkPhysicalDeviceFeatures2,
                                          VkPhysicalDeviceFeatures2>;
      auto& features = *reinterpret_cast<Features*>(node);
      VisitPhysicalDeviceFeatures(features.features, visitor);
      break;
    }
% for structure in chain_features:
<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif
    case ${structure.stype_value}: {
      using Features = std::conditional_t<std::is_const_v<Base>, const ${structure.name}, ${structure.name}>;
      auto& features = *reinterpret_cast<Features*>(node);
% for member in feature_members(structure):
      visitor("${structure.name}", "${member.name}", features.${member.name});
% endfor
      break;
    }
% if define:
#endif
% endif
% endfor
    default:
      break;
    }
  }
}

template <typename Visitor>
void VisitPhysicalDeviceFeatureChain(void* pNext, Visitor&& visitor) {
  VisitPhysicalDeviceFeatureChainImpl(static_cast<VkBaseOutStructure*>(pNext), visitor);
}

template <typename Visitor>
void VisitPhysicalDeviceFeatureChain(const void* pNext, Visitor&& visitor) {
  VisitPhysicalDeviceFeatureChainImpl(static_cast<const VkBaseInStructure*>(pNext), visitor);
}

} // namespace vulkan
} // namespace gits