// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Functions.h"
#include "l0Drivers.h"
#include "l0StateTracking.h"
#include "l0PlayerRunWrap.h"

#include "exception.h"
#include "log.h"

namespace gits {
namespace l0 {
%for name, func in functions.items():
  %if func['enabled']:
    %if (func.get('type') != 'void' or len(func['args']) > 0):
C${name}::C${name}(
      %if func.get('type') != 'void':
  ze_result_t return_value${',' if len(func['args']) > 0 else ''}
      %endif
      %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
      %endfor
) :
      %if func.get('type') != 'void':
  _return_value(return_value)${',' if len(func['args']) > 0 else ''}
      %endif
      %for arg in func['args']:
        %if arg.get('wrapParams'):
  _${arg['name']}(${arg['wrapParams'].replace('{name}', arg['name'])})\
        %elif arg.get('range'):
  _${arg['name']}(${arg['range'].split(',')[1]}, ${arg['name']})\
        %else:
  _${arg['name']}(${arg['name']})\
        %endif
${'' if loop.last else ','}
      %endfor
{
}
    %endif
gits::CArgument& C${name}::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx${', ' if len(func['args']) > 0 else ''}${make_params(func, prefix='_')});
}
    %if func.get('type') != 'void':
gits::CArgument& C${name}::Result(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _return_value);
}
    %endif

void C${name}::Run() {
    %if func.get('runWrap'):
  ${func.get('runWrapName')}(${'this, ' if func.get('passToken') else ''}${make_params(func, prefix='_', with_retval=True)});
    %else:
      %if name == 'zeInit':
  drv.Initialize();
      %endif
      %if func.get('skipRun'):
  Log(TRACE) << "Function ${func.get('name')} skipped";
      %else:
  ${'_return_value.Value() = ' if func.get('type') != 'void' else ''}drv.${func.get('name')}(${make_params(func, prefix='*_')});
        %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${'this, ' if func.get('passToken') else ''}${make_params(func, prefix='*_', with_retval=True)});
        %endif
        %for arg in func['args']:
          %if arg.get('release', False):
  _${arg['name']}.RemoveMapping();
          %endif
        %endfor
      %endif
    %endif
}

  %endif
%endfor
} // namespace l0
} // namespace gits
