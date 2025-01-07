// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}


#ifndef VK_GLOBAL_LEVEL_FUNCTION
#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)
#endif

#ifndef VK_CUSTOM_GLOBAL_LEVEL_FUNCTION
#define VK_CUSTOM_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call) VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)
#endif

% for function in functions_by_level[FuncLevel.GLOBAL]:
${driver_definition(function)}
% endfor

#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_CUSTOM_GLOBAL_LEVEL_FUNCTION


#ifndef VK_INSTANCE_LEVEL_FUNCTION
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

#ifndef VK_CUSTOM_INSTANCE_LEVEL_FUNCTION
#define VK_CUSTOM_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name) VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

% for function in functions_by_level[FuncLevel.INSTANCE]:
${driver_definition(function)}
% endfor

#undef VK_INSTANCE_LEVEL_FUNCTION
#undef VK_CUSTOM_INSTANCE_LEVEL_FUNCTION


#ifndef VK_DEVICE_LEVEL_FUNCTION
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

#ifndef VK_CUSTOM_DEVICE_LEVEL_FUNCTION
#define VK_CUSTOM_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name) VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

% for function in functions_by_level[FuncLevel.DEVICE]:
${driver_definition(function)}
% endfor

#undef VK_DEVICE_LEVEL_FUNCTION
#undef VK_CUSTOM_DEVICE_LEVEL_FUNCTION


#ifndef VK_PROTOTYPE_LEVEL_FUNCTION
#define VK_PROTOTYPE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)
#endif

% for function in functions_by_level[FuncLevel.PROTOTYPE]:
${driver_definition(function)}
% endfor

#undef VK_PROTOTYPE_LEVEL_FUNCTION
