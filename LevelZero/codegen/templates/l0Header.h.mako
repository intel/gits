<%
COMPONENTS_TO_OMIT = ['ze_gits_extension','ze_deprecated', 'ze_dditable', 'zel_tracer']
components = {}
all_functions = {}
current_functions = {}
for name, func in functions.items():
    if not is_latest_version(functions, func):
      continue
    if func.get('component') not in COMPONENTS_TO_OMIT:
        namespace = func['component'].split('_')[0]
        component = func['component'] if func['component'] != namespace else namespace + '_global'
        if not all_functions.get(component):
            all_functions[component] = list()
        all_functions[component].append(func.get('name'))
        if not components.get(namespace):
            components[namespace] = set()
        components[namespace].add(component)
        current_functions[component] = []
%>\
// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <cstddef>

#ifdef WIN32
#define __zecall __cdecl
#else
#define __zecall
#endif

%for constant in constants:
%if 'condition' in constant:
#if ${constant['condition']}
#define ${constant['name']} ${constant['value']}
#endif
%else:
#define ${constant['name']} ${constant['value']}
%endif
%endfor

typedef struct _cl_mem* cl_mem;
typedef struct _cl_command_queue* cl_command_queue;
typedef struct _cl_context* cl_context;
typedef struct _cl_program* cl_program;

%for name, arg in arguments.items():
  %if not is_latest_version(arguments, arg):
<% continue %>
  %endif
  %if 'vars' in arg:
${'union' if arg.get('union') else 'struct'} ${arg.get('name')};
  %else:
typedef struct _${arg.get('name')}* ${arg.get('name')};
  %endif
%endfor

typedef uint8_t ze_bool_t;
typedef void(__zecall *ze_host_pfn_t)(
  void* pUserData
);

typedef void* zet_core_callbacks_t;
typedef void* zet_experimental_callbacks_t;
typedef void* zel_core_callbacks_t;
typedef uint8_t ze_rtas_builder_packed_input_data_format_exp_t;
typedef uint8_t ze_rtas_builder_packed_geometry_exp_flags_t;
typedef uint8_t ze_rtas_builder_packed_geometry_type_exp_t;
typedef uint8_t ze_rtas_builder_packed_instance_exp_flags_t;

%for name, enum in enums.items():
enum ${enum.get('name')} {
  %for var in enum['vars']:
    ${var['name']}${' = ' + var.get('value') if var.get('value') else ''}${'' if loop.last else ','}
  %endfor
};
%endfor

%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
<%
if func['component'] not in COMPONENTS_TO_OMIT:
  current_functions[func['component']].append(func.get('name'))
%>\
typedef ${func.get('type')} (__zecall *pfn_${func.get('name')})(
  %for arg in func['args']:
    ${arg['type']} ${arg['name']}${'' if loop.last else ','}
  %endfor
);
  %if func['component'] not in COMPONENTS_TO_OMIT and sorted(current_functions[func['component']]) == sorted(all_functions[func['component']]):
    %if func['component'] in components[get_namespace(func['component'])]:
<% current_functions.pop(func['component']) %>
struct ${func['component']}_dditable_t {
        %for function in sort_dditable(all_functions[func['component']], functions):
    pfn_${function} ${function};
        %endfor
};
    %endif
  %endif
%endfor

%for name, arg in arguments.items():
  %if not is_latest_version(arguments, arg):
<% continue %>
  %endif
  %if 'vars' in arg:
${'union' if arg.get('union') else 'struct'} ${arg.get('name')} {
    %for var in arg['vars']:
    ${var['type']} ${var['name']};
    %endfor
};
  %endif
%endfor

struct ze_dispatch_table_t
{
%for name, func in functions.items():
  %if not is_latest_version(functions, func):
<% continue %>
  %endif
    pfn_${func.get('name')} ${func.get('name')};
%endfor
};

struct zel_base_properties_t {
    zel_structure_type_t stype;
    void* pNext;
};