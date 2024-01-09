// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclLibrary.cpp
*
* @brief Definition of OpenCL common part library implementation.
*
*/

#include "openclLibrary.h"

#include "gits.h"

namespace gits {
namespace OpenCL {

/**
    * @brief Constructor
    *
    * CLibrary class constructor.
    */
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc)
    : gits::CLibrary(ID_OPENCL, std::move(stc)) {}

CLibrary::~CLibrary() {}

/**
    * @brief Creates OpenCL method call wrapper
    *
    * Method creates OpenCL method call wrappers based on unique
    * identifier.
    *
    * @param id Unique OpenCL method identifier.
    *
    * @exception EOperationFailed Unknown OpenCL method identifier
    *
    * @return OpenCL method call wrapper.
    */
OpenCL::CFunction* OpenCL::CLibrary::FunctionCreate(unsigned id) const {
  return OpenCL::CFunction::Create(id);
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_OPENCL));
}
} //namespace OpenCL
} //namespace gits
