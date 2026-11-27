//
${header}

#pragma once

#include "vulkanHeader2.h"

#include <type_traits>

namespace gits {
namespace vulkan {

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