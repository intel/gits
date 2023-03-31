// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"
#include "oclocHeader.h"
#include "gits.h"
#include "oclocFunction.h"
#include "oclocArguments.h"
#include "argument.h"
#include "oclocLibrary.h"
#include "pragmas.h"

DISABLE_WARNINGS
#include <boost/optional.hpp>
ENABLE_WARNINGS

namespace gits {
namespace ocloc {

class CoclocInvoke : public CFunction {
  static const unsigned ARG_NUM = 10;
  static const unsigned RESULT_NUM = 1;

  Cint _return_value;
  Cuint32_t _argc;
  CStringArray _argv;
  Cuint32_t _numSource;
  CProgramSources _sources;
  Cuint64_t::CSArray _sourceLens;
  CStringArray _sourcesNames;
  Cint _numInputHeader;
  CBufferArray _inputHeaders;
  Cuint64_t::CSArray _lenInputHeaders;
  CStringArray _headerIncludeNames;
  // output arguments are not needed for GITS purposes

  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx);
  virtual boost::optional<const CArgument&> Return() const {
    return _return_value;
  }
  virtual unsigned ResultCount() const {
    return RESULT_NUM;
  }
  virtual CArgument& Result(unsigned idx);

public:
  CoclocInvoke() {}
  CoclocInvoke(int return_value,
               unsigned int argc,
               const char** argv,
               const uint32_t numSources,
               const uint8_t** sources,
               const uint64_t* sourceLens,
               const char** sourcesNames,
               const uint32_t numInputHeaders,
               const uint8_t** dataInputHeaders,
               const uint64_t* lenInputHeaders,
               const char** nameInputHeaders,
               uint32_t* numOutputs,
               uint8_t*** dataOutputs,
               uint64_t** lenOutputs,
               char*** nameOutputs);
  virtual unsigned int Id() const {
    return ID_OCLOC_INVOKE;
  }
  virtual const char* Name() const {
    return "oclocInvoke";
  }
  virtual unsigned Version() const {
    return /*VERSION_1_0*/ 1;
  }
  virtual void Run();
};

class CoclocFreeOutput : public CFunction {
  static const unsigned ARG_NUM = 4;
  static const unsigned RESULT_NUM = 1;

  Cint _return_value;
  // output arguments are not needed for GITS purposes

  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx);
  virtual boost::optional<const CArgument&> Return() const {
    return _return_value;
  }
  virtual unsigned ResultCount() const {
    return RESULT_NUM;
  }
  virtual CArgument& Result(unsigned idx);

public:
  CoclocFreeOutput() {}
  CoclocFreeOutput(int return_value,
                   uint32_t* numOutputs,
                   uint8_t*** dataOutputs,
                   uint64_t** lenOutputs,
                   char*** nameOutputs);
  virtual unsigned int Id() const {
    return ID_OCLOC_FREE_OUTPUT;
  }
  virtual const char* Name() const {
    return "oclocFreeOutput";
  }
  virtual unsigned Version() const {
    return /*VERSION_1_0*/ 1;
  }
  virtual void Run();
};

} // namespace ocloc
} // namespace gits
