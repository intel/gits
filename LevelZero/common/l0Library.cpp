// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   l0Library.cpp
*
* @brief Definition of LevelZero common part library implementation.
*
*/

#include "l0Library.h"

#include "gits.h"

#include "l0Function.h"
#include "l0StateDynamic.h"
#include "l0Tools.h"

namespace gits {
namespace l0 {

/**
    * @brief Constructor
    *
    * CLibrary class constructor.
    */
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc) : gits::CLibrary(ID_LEVELZERO, stc) {}

CLibrary::~CLibrary() {
  delete CStateDynamic::InstancePtr();
}

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
gits::CFunction* l0::CLibrary::FunctionCreate(unsigned id) const {
  return static_cast<l0::CFunction*>(l0::CFunction::Create(id));
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_LEVELZERO));
}
} // namespace l0
} // namespace gits
