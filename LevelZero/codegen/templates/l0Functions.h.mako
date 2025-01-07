// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"
#include "l0Header.h"
#include "gits.h"
#include "l0Function.h"
#include "l0ArgumentsAuto.h"
#include "l0Library.h"
#include "l0Structs.h"
#include "pragmas.h"

namespace gits {
  namespace l0 {
%for name, func in functions.items():
  %if func['enabled']:
    class C${name} : public CFunction {
      static const unsigned ARG_NUM = ${len(func['args'])};
    %if func.get('type') != 'void':
      static const unsigned RESULT_NUM = 1;

      Cze_result_t _return_value;
    %else:
      static const unsigned RESULT_NUM = 0;

    %endif
    %for arg in func['args']:
      ${get_wrap_type(arg)} _${arg['name']};
    %endfor

      virtual unsigned ArgumentCount() const { return ARG_NUM; }
      virtual CArgument& Argument(unsigned idx);
    %if func.get('type') != 'void':
      virtual const CArgument* Return() const { return &_return_value; }
      virtual CArgument& Result(unsigned idx);
    %endif
      virtual unsigned ResultCount() const { return RESULT_NUM; }

    public:
      C${name}() = default;
    %if func.get('type') != 'void' or len(func['args']) > 0:
      C${name}(
      %if func.get('type') != 'void':
        ${func.get('type')} return_value${', ' if len(func['args']) > 0 else ''}
      %endif
      %for arg in func['args']:
        ${arg['type']} ${arg['name']}${'' if loop.last else ','}
      %endfor
      );
    %endif
      virtual unsigned int Id() const { return ${func.get('id')}; }
      virtual const char* Name() const { return "${func.get('name')}"; }
      virtual unsigned Version() const { return ZE_API_VERSION_1_0; }
      virtual void Run();
    };
  %endif
%endfor
  } // namespace l0
} //namespace gits
