// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   oclocLibrary.cpp
*
* @brief Definition of LevelZero common part library implementation.
*
*/

#include "oclocLibrary.h"

#include "gits.h"

#include "oclocFunction.h"

namespace gits {
namespace ocloc {

/**
    * @brief Constructor
    *
    * CLibrary class constructor.
    */
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc)
    : gits::CLibrary(ID_LEVELZERO, std::move(stc)) {}

CLibrary::~CLibrary() {}

/**
    * @brief Creates LevelZero method call wrapper
    *
    * Method creates LevelZero method call wrappers based on unique
    * identifier.
    *
    * @param id Unique LevelZero method identifier.
    *
    * @exception EOperationFailed Unknown LevelZero method identifier
    *
    * @return LevelZero method call wrapper.
    */
gits::CFunction* ocloc::CLibrary::FunctionCreate(unsigned id) const {
  return static_cast<ocloc::CFunction*>(ocloc::CFunction::Create(id));
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_LEVELZERO));
}
} // namespace ocloc
} // namespace gits
