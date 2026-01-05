// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "customTypes.h"

#include "stringFromType.h"
#include "stringToType.h"

namespace gits {
bool ObjectRange::empty() const {
  return range.empty();
}

bool VulkanObjectRange::operator[](uint64_t queueSubmitNumber) const {
  if (objMode == VulkanObjectMode::MODE_VK_QUEUE_SUBMIT) {
    return range[(size_t)queueSubmitNumber];
  } else if (objMode == VulkanObjectMode::MODE_VK_COMMAND_BUFFER ||
             objMode == VulkanObjectMode::MODE_VK_RENDER_PASS ||
             objMode == VulkanObjectMode::MODE_VK_DRAW ||
             objMode == VulkanObjectMode::MODE_VK_BLIT ||
             objMode == VulkanObjectMode::MODE_VK_DISPATCH) {
    return objVector[0] == queueSubmitNumber;
  } else {
    return false;
  }
}

void VulkanObjectRange::SetFromString(const std::string& str) {
  if (str.empty()) {
    return;
  }
  std::istringstream issVulkanObjects(str);
  std::vector<std::string> resourceTable;

  std::string strObj;
  while (std::getline(issVulkanObjects, strObj, '/')) {
    resourceTable.push_back(strObj);
  }
  range = BitRange(resourceTable.back());
  resourceTable.pop_back();
  for (const auto& obj : resourceTable) {
    objVector.push_back(std::stoul(obj, nullptr, 0));
  }
}
} // namespace gits
