// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   function.cpp
 *
 * @brief Definition of library function call wrapper.
 *
 */

#include "function.h"
#include "argument.h"
#include "tools.h"
#include "streams.h"

/* ******************************* F U N C T I O N ***************************** */

/**
 * @brief Destructor
 *
 * Destructor of gits::CFunction class.
 */
gits::CFunction::~CFunction() {}

/**
 * @brief Saves function data to a binary file
 *
 * Method saves function data to a binary file.
 *
 * @param stream Output stream to use.
 */
void gits::CFunction::Write(CBinOStream& stream) const {
  // save function arguments
  unsigned size = ArgumentCount();
  for (unsigned idx = 0; idx < size; idx++) {
    stream << Argument(idx);
  }
  auto ret = Return();
  if (ret) {
    stream << *ret;
  }
  if (Configurator::Get().common.recorder.highIntegrity) {
    stream.flush();
  }
}

/**
 * @brief Loads function data from a binary file
 *
 * Method loads function data from a binary file.
 *
 * @param stream Input stream to use.
 */
void gits::CFunction::Read(CBinIStream& stream) {
  // read token arguments
  unsigned size = ArgumentCount();

  for (unsigned idx = 0; idx < size; idx++) {
    stream >> Argument(idx);
  }

  auto ret = Return();
  if (ret) {
    stream >> *ret;
  }
}

/**
 * @brief Saves function data to a C code file
 *
 * Method saves function data to a C code file.
 *
 * @param stream Output stream to use.
 */
void gits::CFunction::Write(CCodeOStream& stream) const {
  // implementation - captured function calls go to this file:
  stream.select(stream.selectCCodeFile());

  auto returnVal = Return();

  unsigned varNum = 0;
  for (unsigned i = 0; i < ArgumentCount(); i++) {
    const CArgument& arg = Argument(i);
    if (arg.DeclarationNeeded()) {
      if (arg.GlobalScopeVariable()) {
        arg.VariableNameRegister(stream, false);
      } else {
        ++varNum;
      }
    }
  }
  if (returnVal && returnVal->GlobalScopeVariable()) {
    returnVal->VariableNameRegister(stream, true);
  }

  // print arguments declarations
  if (varNum || returnVal) {
    stream.Indent() << "{" << std::endl;
    stream.ScopeBegin();
  }
  for (unsigned i = 0; i < ArgumentCount(); i++) {
    const CArgument& arg = Argument(i);
    if (arg.DeclarationNeeded()) {
      if (!arg.GlobalScopeVariable()) {
        arg.VariableNameRegister(stream, false);
      }
      arg.Declare(stream);
    }
  }
  if (returnVal) {
    if (!returnVal->GlobalScopeVariable()) {
      returnVal->VariableNameRegister(stream, true);
    }
    returnVal->Declare(stream);
  }

  // print function
  stream.Indent();
  if (returnVal) {
    stream << stream.VariableName(returnVal->ScopeKey()) << " = ";
  }
  stream << Name() << Suffix() << "(";
  for (unsigned idx = 0; idx < ArgumentCount(); idx++) {
    stream << Argument(idx);
    if (idx < ArgumentCount() - 1) {
      stream << ", ";
    }
  }
  if (strcmp(Suffix(), "") != 0 && Return()) {
    stream << ", " << *Return();
  }
  stream << ");" << std::endl;

  for (unsigned i = 0; i < ArgumentCount(); i++) {
    const CArgument& arg = Argument(i);
    if (arg.PostActionNeeded()) {
      arg.PostAction(stream);
    }
  }

  WritePostCall(stream);

  if (varNum || returnVal) {
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
  }
}

/**
 * @brief Returns function argument
 *
 * Method returns function argument.
 *
 * @param idx Index of an argument to be returned.
 */
const gits::CArgument& gits::CFunction::Argument(unsigned idx) const {
  CFunction& nonConst = const_cast<CFunction&>(*this);
  return nonConst.Argument(idx);
}

const gits::CArgument& gits::CFunction::Result(unsigned idx) const {
  CFunction& nonConst = const_cast<CFunction&>(*this);
  return nonConst.Result(idx);
}

gits::CArgument* gits::CFunction::Return() {
  const CFunction& func = const_cast<const CFunction&>(*this);
  if (func.Return()) {
    return const_cast<gits::CArgument*>(func.Return());
  } else {
    return nullptr;
  }
}

NORETURN void gits::report_cargument_error(const char* func, unsigned idx) {
  LOG_ERROR << "Invalid argument number: " << func << "( " << idx << ")";
  throw std::runtime_error("invalid CArgument index requested");
}
