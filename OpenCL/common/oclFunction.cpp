// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclFunction.h"

#include "openclFunctionsAuto.h"
#include "openclHelperFunctions.h"

#include "exception.h"

namespace gits {
namespace OpenCL {

/**
    * @brief Creates OpenCL function call warapper
    *
    * Method creates OpenCL function call wrappers based on unique
    * identifier.
    *
    * @param id Unique OpenCL function identifer.
    *
    * @exception EOperationFailed Unknown OpenCL function identifier
    *
    * @return OpenCL function call wrapper.
    */
CFunction* CFunction::Create(unsigned id) {
  if (id < CFunction::ID_OPENCL || id >= CFunction::ID_FUNCTION_END) {
    return nullptr;
  }
  switch (id) {
  case ID_GITS_CL_MEMORY_UPDATE:
    return new CGitsClMemoryUpdate;
  case ID_GITS_CL_MEMORY_RESTORE:
    return new CGitsClMemoryRestore;
  case ID_GITS_CL_MEMORY_REGION_RESTORE:
    return new CGitsClMemoryRegionRestore;
  case ID_GITS_CL_MAKE_CURRENT_THREAD:
    return new CGitsClTokenMakeCurrentThread;
#include "openclIDswitch.h"
  default:;
  }
  Log(ERR) << "Unknown OpenCL function with ID: " << id;
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
CArgument& CFunction::Result(unsigned idx) {
  Log(ERR) << "Results not supported in OpenCL!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}
} // namespace OpenCL
} // namespace gits