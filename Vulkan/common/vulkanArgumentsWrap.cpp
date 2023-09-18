//====================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//====================== end_copyright_notice ==============================

#include "vulkanArgumentsAuto.h"

bool gits::Vulkan::CVkDescriptorImageInfo::DeclarationNeeded() const {
  // Because if it was a nullptr, we'll just print "nullptr", no need for a variable.
  return !*_isNullPtr;
}
