// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Log.h"

namespace gits {
namespace l0 {

void LogKernelExecution(const uint32_t& kernelNumber,
                        const char* kernelName,
                        const uint32_t& cmdQueueNumber,
                        const uint32_t& cmdListNumber) {
  std::stringstream ss;
  ss << "--- Queue #" << cmdQueueNumber << " / CommandList #" << cmdListNumber << " / Kernel #"
     << kernelNumber << ", \"" << kernelName << "\" ---";
  LOG_TRACE << ss.str();
}

void LogAppendKernel(const uint32_t& kernelNumber, const char* pKernelName) {
  std::stringstream ss;
  ss << "--- Kernel append call #" << kernelNumber << ", \"" << pKernelName << "\" ---";
  LOG_TRACE << ss.str();
}
} // namespace l0
} // namespace gits
