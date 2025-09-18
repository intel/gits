// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclFunctionsAuto.h"
#include "openclDrivers.h"
#include "openclPlayerRunWrap.h"
#include "openclStateTracking.h"
#include "exception.h"
#include "log2.h"

namespace gits {
namespace OpenCL {

%for name, func in only_enabled(functions).items():
  %if 'platform' in func:
#ifdef GITS_PLATFORM_${func.get('platform').upper()}
  %endif
C${name}::C${name}(${make_params(func, with_types=True, with_retval=True, tabs_num=2)}):
  %if func.get('type') != 'void':
  _return_value(return_value)${',' if len(func['args']) > 0 else ''}
  %endif
  %for arg in func['args']:
    %if arg.get('wrapParams'):
  _${arg['name']}(${arg['wrapParams'].replace('{name}', arg['name'])})\
    %elif arg.get('range'):
  _${arg['name']}(${arg['range'].split(',')[1]}, ${arg['name']})\
    %elif 'const char*' == arg['type']:
  _${arg['name']}(${arg['name']}, 0, 1)\
    %elif '*' in arg['type'] and not 'void' in arg['type'] and arg.get('wrapType') != 'CvoidPtr':
  _${arg['name']}(1, ${arg['name']})\
    %else:
  _${arg['name']}(${arg['name']})\
    %endif
${'' if loop.last else ','}
  %endfor
{
  %for arg in func['args']:
    %if arg.get('removeMapping'):
  _${arg['name']}.RemoveMapping();
    %endif
  %endfor
}

gits::CArgument& C${name}::Argument(unsigned idx) {
  %if len(func['args']) == 0:
  Log(ERR) << "Invalid Argument index: C${name}::Argument(" << idx << ")";
  throw ENotFound(EXCEPTION_MESSAGE);
  %else:
  return get_cargument(__FUNCTION__, idx${', ' if len(func['args']) > 0 else ''}${make_params(func, prefix='_', one_line=True)});
  %endif
}

gits::CArgument& C${name}::Result(unsigned idx) {
  %if func.get('type') == 'void' or name.startswith('clGetExtensionFunctionAddress'):
  Log(ERR) << "Invalid Result index: C${name}::Result(" << idx << ")";
  throw ENotFound(EXCEPTION_MESSAGE);
  %else:
  return get_cargument(__FUNCTION__, idx, _${'errcode_ret' if any('errcode_ret' == arg['name'] for arg in func['args']) else 'return_value'});
  %endif
}
  
void C${name}::Run() {
  %if 'D3D' in name or 'DX9' in name:
  D3DWarning();
  %endif
  %if func.get('runWrap'):
  ${func.get('runWrapName')}(${'this, ' if func.get('passToken') else ''}${make_params(func, prefix='_', with_retval=True, one_line=True)});
  %else:
    %if func.get('type') == 'void':
  drvOcl.${cut_version(name,func.get('version'))}(${make_params(func, prefix='*_',one_line=True)});
    %elif any('errcode_ret' == arg['name'] for arg in func['args']):
  _return_value.Assign(drvOcl.${cut_version(name,func.get('version'))}(${make_params(func, prefix='*_', one_line=True)}));
    %else:
  _return_value.Value() = drvOcl.${cut_version(name,func.get('version'))}(${make_params(func, prefix='*_', one_line=True)});
    %endif
    %if func.get('stateTrack'):
      %if func.get('passNullToken'):
  ${func.get('stateTrackName')}(nullptr, ${make_params(func, prefix='*_', with_retval=True, one_line=True)});
      %elif func.get('passToken'):
  ${func.get('stateTrackName')}(this, ${make_params(func, prefix='*_', with_retval=True, one_line=True)});
      %else:
  ${func.get('stateTrackName')}(${make_params(func, prefix='*_', with_retval=True, one_line=True)});
      %endif
    %endif
  %endif
  %for arg in func['args']:
    %if arg.get('removeMapping'):
  _${arg['name']}.RemoveMapping();
    %endif
  %endfor
}

  %if func.get('ccodeWrap'):
void C${name}::Write(CCodeOStream &stream) const {
  ${func.get('ccodeWrap')}
}
  %elif (func.get('functionType') == Creator and func['type'] != 'cl_int') or func['name'] == 'clLinkProgram' or 'EnqueueMap' in func['name']:
void C${name}::WritePostCall(CCodeOStream &stream) const {
  stream.Indent() << _return_value.WrapTypeNameStr() << "::AddMapping(0x" << (void*)_return_value.Original() << ", " << stream.VariableName(_return_value.ScopeKey()) << ");" << std::endl;
}
  %endif
  %if 'platform' in func:
#endif
  %endif

%endfor
} // namespace OpenCL
} // namespace gits
