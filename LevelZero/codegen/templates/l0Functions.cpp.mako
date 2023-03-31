// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
C${name}::C${name}(
    %if func.get('type') != 'void':
  ze_result_t return_value,
    %endif
        %for arg in func['args']:
  ${arg['type']} ${arg['name']}${'' if loop.last else ','}
        %endfor
) :
    %if func.get('type') != 'void':
  _return_value(return_value),
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

gits::CArgument& C${name}::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, ${make_params(func['args'], '_')});
}
    %if func.get('type') != 'void':
gits::CArgument& C${name}::Result(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _return_value);
}
    %endif

void C${name}::Run() {
    %if func.get('runWrap'):
  ${func.get('runWrapName')}(${'' if func.get('type') == 'void' else '_return_value, '}${make_params(func['args'], '_')});
    %else:
      %if name == 'zeInit':
  drv.Initialize();
      %endif
      %if func.get('skipRun'):
  Log(TRACE) << "Function ${func.get('name')} skipped";
      %else:
  ${'_return_value.Value() = ' if func.get('type') != 'void' else ''}drv.${func.get('name')}(${make_params(func['args'], '*_')});
        %if func.get('stateTrack'):
  ${func.get('stateTrackName')}(${'*_return_value, 'if func.get('type') != 'void' else ''}${make_params(func['args'], '*_')});
        %endif
      %endif
    %endif
}

  %endif
%endfor
} // namespace l0
} // namespace gits
