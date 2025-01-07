// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"
#include "openclHeader.h"
#include "gits.h"
#include "oclFunction.h"
#include "openclArguments.h"
#include "openclLibrary.h"
#include "pragmas.h"

namespace gits {
namespace OpenCL {

%for name, func in only_enabled(functions).items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
class C${name} : public CFunction {
  static const unsigned ARG_NUM = ${len(func['args'])};
  %if func.get('type') != 'void':
  static const unsigned RESULT_NUM = 1;

  ${get_wrap_type(func['retV'])} _return_value;\
  %else:
  static const unsigned RESULT_NUM = 0;
  %endif

  %for arg in func['args']:
  ${get_wrap_type(arg)} _${arg['name']};
  %endfor

  virtual unsigned ArgumentCount() const { return ARG_NUM; }
  virtual CArgument& Argument(unsigned idx);
  virtual const CArgument* Return() const { return ${'nullptr' if func['type'] == 'void' else '&_return_value'}; }
  virtual unsigned ResultCount() const { return RESULT_NUM; }
  virtual CArgument& Result(unsigned idx);

public:
  C${name}() = default;
  %if func.get('type') != 'void' or len(func['args']) > 0:
  C${name}(${make_params(func, with_types=True, with_retval=True, tabs_num=2)});
  %endif

  virtual unsigned int Id() const { return ${func.get('id')}; }
  virtual const char* Name() const { return "${func.get('name')}"; }
  virtual unsigned Version() const { return VERSION_${func.get('availableFrom')}; }
  virtual void Run();
  %if func.get('ccodeWrap'):
  virtual void Write(CCodeOStream &stream) const;
  %elif (func.get('functionType') == Creator and func['type'] != 'cl_int') or func['name'] == 'clLinkProgram' or 'EnqueueMap' in func['name']:
  virtual void WritePostCall(CCodeOStream &stream) const;
  %endif
};
  %if 'platform' in func:
#endif
  %endif

%endfor
} // namespace OpenCL
} // namespace gits
