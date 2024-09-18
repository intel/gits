#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_vulkan import *
from datetime import datetime
import operator
import re

import sys
import os
import shutil

vulkan_flags = [
  'VkFlags',
  'VkInstanceCreateFlags',
  'VkFormatFeatureFlags',
  'VkImageUsageFlags',
  'VkImageCreateFlags',
  'VkSampleCountFlags',
  'VkQueueFlags',
  'VkMemoryPropertyFlags',
  'VkMemoryHeapFlags',
  'VkDeviceCreateFlags',
  'VkDeviceQueueCreateFlags',
  'VkPipelineStageFlags',
  'VkMemoryMapFlags',
  'VkImageAspectFlags',
  'VkSparseImageFormatFlags',
  'VkSparseMemoryBindFlags',
  'VkFenceCreateFlags',
  'VkSemaphoreCreateFlags',
  'VkEventCreateFlags',
  'VkQueryPoolCreateFlags',
  'VkQueryPipelineStatisticFlags',
  'VkQueryResultFlags',
  'VkBufferCreateFlags',
  'VkBufferUsageFlags',
  'VkBufferViewCreateFlags',
  'VkImageViewCreateFlags',
  'VkShaderModuleCreateFlags',
  'VkPipelineCacheCreateFlags',
  'VkPipelineCreateFlags',
  'VkPipelineShaderStageCreateFlags',
  'VkPipelineVertexInputStateCreateFlags',
  'VkPipelineInputAssemblyStateCreateFlags',
  'VkPipelineTessellationStateCreateFlags',
  'VkPipelineViewportStateCreateFlags',
  'VkPipelineRasterizationStateCreateFlags',
  'VkPipelineRasterizationStateStreamCreateFlagsEXT',
  'VkCullModeFlags',
  'VkPipelineMultisampleStateCreateFlags',
  'VkPipelineDepthStencilStateCreateFlags',
  'VkPipelineColorBlendStateCreateFlags',
  'VkColorComponentFlags',
  'VkPipelineDynamicStateCreateFlags',
  'VkPipelineLayoutCreateFlags',
  'VkShaderStageFlags',
  'VkSamplerCreateFlags',
  'VkDescriptorSetLayoutCreateFlags',
  'VkDescriptorPoolCreateFlags',
  'VkDescriptorPoolResetFlags',
  'VkFramebufferCreateFlags',
  'VkRenderPassCreateFlags',
  'VkAttachmentDescriptionFlags',
  'VkSubpassDescriptionFlags',
  'VkAccessFlags',
  'VkDependencyFlags',
  'VkCommandPoolCreateFlags',
  'VkCommandPoolResetFlags',
  'VkCommandBufferUsageFlags',
  'VkQueryControlFlags',
  'VkCommandBufferResetFlags',
  'VkStencilFaceFlags',
  'VkSurfaceTransformFlagsKHR',
  'VkCompositeAlphaFlagsKHR',
  'VkSwapchainCreateFlagsKHR',
  'VkDisplayModeCreateFlagsKHR',
  'VkDisplayPlaneAlphaFlagsKHR',
  'VkDisplaySurfaceCreateFlagsKHR',
  'VkXlibSurfaceCreateFlagsKHR',
  'VkXcbSurfaceCreateFlagsKHR',
  'VkWaylandSurfaceCreateFlagsKHR',
  'VkMirSurfaceCreateFlagsKHR',
  'VkAndroidSurfaceCreateFlagsKHR',
  'VkWin32SurfaceCreateFlagsKHR',
  'VkCommandPoolTrimFlagsKHR',
  'VkDebugReportFlagsEXT',
  'VkExternalMemoryHandleTypeFlagsNV',
  'VkExternalMemoryFeatureFlagsNV',
  'VkPeerMemoryFeatureFlagsKHX',
  'VkMemoryAllocateFlagsKHX',
  'VkDeviceGroupPresentModeFlagsKHX',
  'VkViSurfaceCreateFlagsNN',
  'VkExternalMemoryHandleTypeFlagsKHX',
  'VkExternalMemoryFeatureFlagsKHX',
  'VkExternalSemaphoreHandleTypeFlagsKHX',
  'VkExternalSemaphoreFeatureFlagsKHX',
  'VkIndirectCommandsLayoutUsageFlagsNVX',
  'VkObjectEntryUsageFlagsNVX',
  'VkSurfaceCounterFlagsEXT',
  'VkDescriptorUpdateTemplateCreateFlags',
  'VkDeviceGroupPresentModeFlagsKHR',
  'VkMemoryAllocateFlags',
  'VkPipelineRasterizationDepthClipStateCreateFlagsEXT',
  'VkSemaphoreWaitFlags',
  'VkConditionalRenderingFlagsEXT',
  'VkPipelineCreationFeedbackFlags',
  'VkPipelineCreationFeedbackFlagsEXT',
  'VkDescriptorBindingFlags',
  'VkDeviceMemoryReportFlagsEXT',
  'VkSubmitFlagsKHR',
  'VkVideoSessionCreateFlagsKHR',
  'VkVideoBeginCodingFlagsKHR',
  'VkVideoEndCodingFlagsKHR',
  'VkVideoCodingQualityPresetFlagsKHR',
  'VkVideoCapabilitiesFlagsKHR',
  'VkVideoCodingControlFlagsKHR',
  'VkVideoDecodeH264CreateFlagsEXT',
  'VkVideoDecodeH265CreateFlagsEXT',
  'VkVideoEncodeH264CapabilitiesFlagsEXT',
  'VkVideoEncodeH264InputModeFlagsEXT',
  'VkVideoEncodeH264OutputModeFlagsEXT',
  'VkVideoEncodeH264CreateFlagsEXT',
  'VkVideoEncodeRateControlFlagsKHR',
  'VkVideoChromaSubsamplingFlagsKHR',
  'VkVideoCodecOperationFlagsKHR',
  'VkVideoDecodeFlagsKHR',
  'VkVideoEncodeFlagsKHR',
  'VkSubmitFlags',
  'VkRenderingFlags',
  'VkGraphicsPipelineLibraryFlagsEXT',
  'VkAccelerationStructureCreateFlagsKHR',
  'VkBuildAccelerationStructureFlagsKHR',
  'VkGeometryFlagsKHR',
  'VkGeometryInstanceFlagsKHR',
  'VkExternalMemoryHandleTypeFlags'
]

vulkan_uint32 = vulkan_flags + [
  'uint32_t',
  'bool32_t',
  'VkBool32',
]
vulkan_uint64 = [
  'VkDeviceSize',
  'VkDeviceAddress',
  'VkAccessFlags2',
  'VkAccessFlags2KHR',
  'VkPipelineStageFlags2',
  'VkPipelineStageFlags2KHR',
]

vulkan_union = [
  "VkClearColorValue",
  "VkClearValue"
]

vulkan_other_primitives = [
  "bool",
  "int32_t",
  "int64_t",
  "uint8_t",
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "size_t",
  "float",
  "double",
  "void*",
  "void**",
  "nullptr"
]

vulkan_enums = []
for enum in get_enums():
  vulkan_enums.append(enum.name)

vulkan_structs = []
for struct in get_structs():
  vulkan_structs.append(struct.name.rstrip('_'))

primitive_types = vulkan_enums + vulkan_uint32 + vulkan_uint64 + vulkan_union + vulkan_other_primitives

opaque_dispatchable_handles = [
  "VkInstance",
  "VkPhysicalDevice",
  "VkDevice",
  "VkQueue",
  "VkCommandBuffer",
]

opaque_nondispatchable_handles = [
  'VkSemaphore',
  'VkFence',
  'VkDeviceMemory',
  'VkBuffer',
  'VkImage',
  'VkEvent',
  'VkQueryPool',
  'VkBufferView',
  'VkImageView',
  'VkShaderModule',
  'VkPipelineCache',
  'VkPipelineLayout',
  'VkRenderPass',
  'VkPipeline',
  'VkDescriptorSetLayout',
  'VkSampler',
  'VkDescriptorPool',
  'VkDescriptorSet',
  'VkFramebuffer',
  'VkCommandPool',
  'VkSamplerYcbcrConversion',
  'VkDescriptorUpdateTemplate',
  'VkSurfaceKHR',
  'VkSwapchainKHR',
  'VkDisplayKHR',
  'VkDisplayModeKHR',
  'VkDebugReportCallbackEXT',
  'VkObjectTableNVX',
  'VkIndirectCommandsLayoutNVX',
  'VkDebugUtilsMessengerEXT',
  'VkValidationCacheEXT',
  'VkPerformanceConfigurationINTEL',
  'VkVideoSessionKHR',
  'VkVideoSessionParametersKHR',
  'VkAccelerationStructureKHR',
  'VkDeferredOperationKHR'
]

other_opaque_handles = [
  "HWND",
  "HINSTANCE",
]

opaque_handles = opaque_dispatchable_handles + opaque_nondispatchable_handles + other_opaque_handles

# Data for CCode
types_needing_name_registration = [
  "StringArray",
  "ByteStringArray",
]

types_not_needing_declaration = vulkan_enums + vulkan_uint32 + vulkan_uint64 + vulkan_other_primitives + opaque_handles + [
  "NullWrapper",
  "VoidPtr",
]

vulkan_mapped_types = opaque_dispatchable_handles
vulkan_mapped_types_nondisp = opaque_nondispatchable_handles

AUTO_GENERATED_HEADER = f"""
//
// FILE AUTO-GENERATED BY GITS CODE GENERATOR. DO NOT MODIFY DIRECTLY.
// GENERATED ON: {datetime.now()}
//
""".strip('\n')

copyright_header = f"""// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

{AUTO_GENERATED_HEADER}

"""
# Header for .def files requires a different comment style.
copyright_header_def = copyright_header.replace('//', ';')

structs_names = []


def arg_decl(fdata, prototypes=False, vkDrivers=False):
  content = ''
  if (fdata['type'] != 'void') and (vkDrivers is False):
    content += fdata['type'] + ' return_value, '
  for arg in fdata['args']:
    arg_type = arg['type']
    arg_array = ''
    re_array = re.search(r'\[([0-9_]+)\]', arg_type)
    if re_array:
      arg_type = arg_type.replace(re_array.group(0), '')
      arg_array = re_array.group(0)
    content += arg_type + ' ' + arg['name'] + arg_array + ','
    if prototypes is True:
      content += '\n'
    else:
      content += ' '
  content = content.rstrip(', \n')
  return content


def add_version(name, version):
  name = name.strip('_')
  if (version is not None and version > 0):
    return name + '_V' + str(version)
  else:
    return name


def make_id(name, version):
  id_ = re.sub('([a-z])([A-Z])', r'\g<1>_\g<2>', name)
  id_ = re.sub('([0-9])D', r'_\g<1>D_', id_)

  id_final = 'ID_' + id_.upper().strip('_')
  id_final = add_version(id_final, version)
  return id_final

def make_type(fdata):
  type = ""
  if fdata['functionType'] & FuncType.PARAM:
    type += "GITS_VULKAN_PARAM_APITYPE | "
  if fdata['functionType'] & FuncType.QUEUE_SUBMIT:
    type += "GITS_VULKAN_QUEUE_SUBMIT_APITYPE | "
  if fdata['functionType'] & FuncType.CREATE_IMAGE:
    type += "GITS_VULKAN_CREATE_IMAGE_APITYPE | "
  if fdata['functionType'] & FuncType.CREATE_BUFFER:
    type += "GITS_VULKAN_CREATE_BUFFER_APITYPE | "
  if fdata['functionType'] & FuncType.COMMAND_BUFFER_SET:
    type += "GITS_VULKAN_COMMAND_BUFFER_SET_APITYPE | "
  if fdata['functionType'] & FuncType.COMMAND_BUFFER_BIND:
    type += "GITS_VULKAN_COMMAND_BUFFER_BIND_APITYPE | "
  if fdata['functionType'] & FuncType.COMMAND_BUFFER_PUSH:
    type += "GITS_VULKAN_COMMAND_BUFFER_PUSH_APITYPE | "
  if fdata['functionType'] & FuncType.BEGIN_RENDER_PASS:
    type += "GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE | "
  if fdata['functionType'] & FuncType.END_RENDER_PASS:
    type += "GITS_VULKAN_END_RENDER_PASS_APITYPE | "
  if fdata['functionType'] & FuncType.DRAW:
    type += "GITS_VULKAN_DRAW_APITYPE | "
  if fdata['functionType'] & FuncType.BLIT:
    type += "GITS_VULKAN_BLIT_APITYPE | "
  if fdata['functionType'] & FuncType.DISPATCH:
    type += "GITS_VULKAN_DISPATCH_APITYPE | "
  if fdata['functionType'] & FuncType.NEXT_SUBPASS:
    type += "GITS_VULKAN_NEXT_SUBPASS_APITYPE | "
  type = type.strip(" | ")
  return type

def arg_call(fdata, prePostExec=False, vkDrivers=False):
  content = ""
  if (fdata['type'] != 'void') and (vkDrivers is False):
    content += 'return_value, '
  if prePostExec is True:
    content += "drvVk, "
  for arg in fdata['args']:
    content += arg['name'] + ", "
  content = content.rstrip(", ")
  return content


def generate_vulkan_drivers(functions):
  vk_drivers = open('vulkanDriversAuto.inl', 'w')
  vk_drivers.write(copyright_header)
  global_level_functions = ""
  instance_level_functions = ""
  device_level_functions = ""
  prototype_level_functions = ""

  for name in sorted(functions.keys()):
    function = functions[name][0]  # 0 because we only want the base version.
    type = function['type']
    if type == 'void':
      type = 'void_t'
    args_decl = arg_decl(function, False, True)
    args_call = arg_call(function, False, True)

    content = "_LEVEL_FUNCTION(%(type)s, %(name)s, (%(args_decl)s), (%(args_call)s)" % locals()
    definition = "VK_"

    if function['customDriver'] is True:
      definition += "CUSTOM_"

    if function['level'] == FuncLevel.GLOBAL:
      global_level_functions += definition + "GLOBAL" + content + ")\n"
    elif function['level'] == FuncLevel.INSTANCE:
      instance_level_functions += definition + "INSTANCE" + content + ", " + function['args'][0]['name'] + ")\n"
    elif function['level'] == FuncLevel.PROTOTYPE:
      prototype_level_functions += definition + "PROTOTYPE" + content + ")\n"
    else:
      device_level_functions += definition + "DEVICE" + content + ", " + function['args'][0]['name'] + ")\n"

  content = """
#ifndef VK_GLOBAL_LEVEL_FUNCTION
#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)
#endif

#ifndef VK_CUSTOM_GLOBAL_LEVEL_FUNCTION
#define VK_CUSTOM_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call) VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)
#endif

%(global_level_functions)s
#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_CUSTOM_GLOBAL_LEVEL_FUNCTION


#ifndef VK_INSTANCE_LEVEL_FUNCTION
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

#ifndef VK_CUSTOM_INSTANCE_LEVEL_FUNCTION
#define VK_CUSTOM_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name) VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

%(instance_level_functions)s
#undef VK_INSTANCE_LEVEL_FUNCTION
#undef VK_CUSTOM_INSTANCE_LEVEL_FUNCTION


#ifndef VK_DEVICE_LEVEL_FUNCTION
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

#ifndef VK_CUSTOM_DEVICE_LEVEL_FUNCTION
#define VK_CUSTOM_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name) VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, first_argument_name)
#endif

%(device_level_functions)s
#undef VK_DEVICE_LEVEL_FUNCTION
#undef VK_CUSTOM_DEVICE_LEVEL_FUNCTION


#ifndef VK_PROTOTYPE_LEVEL_FUNCTION
#define VK_PROTOTYPE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)
#endif

%(prototype_level_functions)s
#undef VK_PROTOTYPE_LEVEL_FUNCTION
""" % {'global_level_functions': global_level_functions, 'instance_level_functions': instance_level_functions, 'device_level_functions': device_level_functions, 'prototype_level_functions': prototype_level_functions}

  vk_drivers.write(content)


def generate_vulkan_log(structs, enums):
  vk_log_auto_cpp = open('vulkanLogAuto.cpp', 'w')
  vk_log_auto_inl = open('vulkanLogAuto.inl', 'w')

  vk_log_auto_cpp.write(copyright_header)
  vk_log_auto_inl.write(copyright_header)

  includes = "#include \"config.h\"\n"     \
  "#include \"vulkanLog.h\"\n"             \
  "#include \"vulkanTools_lite.h\"\n\n"
  vk_log_auto_cpp.write(includes)
  namespaces = "namespace gits {\n"  \
  "namespace Vulkan {\n\n"
  vk_log_auto_cpp.write(namespaces)

  content = ""
  content_inl = ""
  declared_structs = []
  while len(declared_structs) < len(structs):
    for key in sorted(structs.keys()):
      if key not in declared_structs:
        to_declare = []
        struct = structs[key][0]  # 0 because we only need the base version.
        for var in struct['vars']:
          elem_type = var['type'] + '_'
          if (elem_type not in declared_structs) and (elem_type in structs.keys()):
            to_declare.append(elem_type)
        if len(to_declare) == 0:
          versioned_name = add_version(key, struct.get('version'))
          content_inl += "CVkLog & operator<<(const " + versioned_name + "& c);\n"
          content += "CVkLog & CVkLog::operator<<(const " + versioned_name + "& c) {\n"
          content += "  *this << \"{\" << "

          for var in struct['vars']:
            content += "\" " + var['name'] + ": \""

            type_cast = var['type'].replace("Flags", "FlagBits")
            if (var['type'].find("Flags") != -1) and (type_cast in enums):
              type_cast = "(" + type_cast + ")"
            else:
              type_cast = ""

            if (var['name'] == "pNext"):
              content += " << (PNextPointerTypeTag)c.pNext << \", \" << "
            elif ('count' in var):
              content += ";\n"

              content += "  if ((isTraceDataOptPresent(TraceData::VK_STRUCTS))"
              # Avoid nullptr checks for arrays on the stack.
              if '[' not in var['type']:
                content+= " && (c." + var['name'] + " != nullptr)"
              if ('logCondition' in var):
                content += " && (" + var['logCondition'] + ")"
              content += ") {\n"

              content += "    *this << \"{\";\n"
              content += "    for (uint32_t i = 0; i < (uint32_t)c." + var['count'] + "; ++i) {\n"
              content += "      *this << \" [\" << i << \"]:\" << " + type_cast + "c." + var['name'] + "[i];\n"
              content += "    }\n"
              content += "    *this << \" }\";\n"
              content += "  } else {\n"
              content += "    *this << (void*)c." + var['name'] + ";\n"
              content += "  }\n"
              content += "  *this << \", \" << "
            else:
              content += " << " + type_cast + "c." + var['name'] + " << \", \" << "

          content = content.rstrip("<< \", \" << ")
          content += " << \" }\";\n"
          content += "  return *this;\n"
          content += "}\n\n"

          content_inl += "CVkLog & operator<<(const " + versioned_name + "* c);\n"
          content += "CVkLog & CVkLog::operator<<(const " + versioned_name + "* c) {\n"
          content += "  if (c != nullptr) {\n"
          content += "    if (isTraceDataOptPresent(TraceData::VK_STRUCTS))\n"
          content += "      *this << *c;\n"
          content += "    else\n"
          content += "      _buffer << \"{ \" << (void*)c << \" }\";\n"
          content += "  } else {\n"
          content += "    _buffer << \"{ 0 }\";\n"
          content += "  }\n"
          content += "  return *this;\n"
          content += "}\n\n"

          declared_structs.append(key)
  content = content.replace("\", \" << \" ", "\", ")
  content = content.replace("\"{\" << \" ", "\"{ ")
  content = content.replace("c.8", "8")
  content = content.replace("c.6", "6")
  content = content.replace("c.4", "4")
  content = content.replace("c.3", "3")
  content = content.replace("c.2", "2")
  content = content.replace("c.1", "1")
  vk_log_auto_cpp.write(content)
  vk_log_auto_inl.write(content_inl)

  content_enums = ""
  content_enums_inl = ""
  for key in sorted(enums.keys()):
    for enum in enums[key]:
      content_enums_inl += "CVkLog & operator<<(const " + key + "& c);\n"
      content_enums += "CVkLog & CVkLog::operator<<(const " + key + "& c) {\n"

      if key.find("Bits") != -1:
        content_enums += "  std::underlying_type_t<" + key + "> e = c;\n"
        content_enums += "  std::ostringstream os;\n"

        enum['vars'].sort(key=lambda x: int(x['value']), reverse=True)
        if enum['vars'][-1]['value'] == '0':
          content_enums += "  if (e == 0) {\n"
          content_enums += "    os << \"" + enum['vars'][-1]['name'] + " | \";\n"
          content_enums += "  }\n"
        for var in enum['vars']:
          if var['value'] != '0':
            content_enums += "  if (isBitSet(e, " + var['name'] + ")) {\n"
            content_enums += "    os << \"" + var['name'] + " | \";\n"
            content_enums += "    e &= ~" + var['name'] + ";\n"
            content_enums += "  }\n"

        content_enums += "  for (decltype(e) i = 1; i <= e; i <<= 1) {\n"
        content_enums += "    if (i & e) {\n"

        content_enums += "      os << i << \" | \";\n"
        content_enums += "      Log(WARN) << \"Unknown enum number: \" << i << \" for " + key + "\";\n"
        content_enums += "      break;\n"
        content_enums += "    }\n"
        content_enums += "  }\n"
        content_enums += "  std::string str = os.str();\n"
        content_enums += "  if (str.size() > 3) {\n"
        content_enums += "    str = str.substr(0, str.size() - 3);\n"
        content_enums += "  } else {\n"
        content_enums += "    str = \"0\";\n"
        content_enums += "  }\n"
        content_enums += "  _buffer << \"{ \" << str << \" }\";\n"

      else:
        content_enums += "  switch (c) {\n"
        for var in enum['vars']:
          content_enums += "    case " + var['value'] + ":\n"
          content_enums += "      _buffer << \"" + var['name'] + "\";\n"
          content_enums += "      break;\n"

        content_enums += "    default:\n"
        content_enums += "      _buffer << c;\n"
        content_enums += "      Log(WARN) << \"Unknown enum number: \" << c << \" for " + key + "\";\n"
        content_enums += "      break;\n"
        content_enums += "  }\n"

      content_enums += "  return *this;\n"
      content_enums += "}\n\n"
  vk_log_auto_cpp.write(content_enums)
  vk_log_auto_inl.write(content_enums_inl)

  content_nondisp = "#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)\n"
  content_nondisp_inl = content_nondisp
  for key in vulkan_mapped_types_nondisp:
    content_nondisp_inl += "CVkLog & operator<<(const " + key + "& c);\n"
    content_nondisp += "CVkLog & CVkLog::operator<<(const " + key + "& c) {\n"
    content_nondisp += "  *this << \"{ \" << (void*)c << \" }\";\n"
    content_nondisp += "  return *this;\n"
    content_nondisp += "}\n\n"

    content_nondisp_inl += "CVkLog & operator<<(const " + key + "* c);\n"
    content_nondisp += "CVkLog & CVkLog::operator<<(const " + key + "* c) {\n"
    content_nondisp += "  if (c != nullptr) {\n"
    content_nondisp += "    if (isTraceDataOptPresent(TraceData::VK_STRUCTS))\n"
    content_nondisp += "      *this << *c;\n"
    content_nondisp += "    else\n"
    content_nondisp += "      _buffer << \"{ \" << (void*)c << \" }\";\n"
    content_nondisp += "  } else {\n"
    content_nondisp += "    _buffer << \"{ 0 }\";\n"
    content_nondisp += "  }\n"
    content_nondisp += "  return *this;\n"
    content_nondisp += "}\n\n"

  content_nondisp += "#endif\n"
  content_nondisp_inl += "#endif\n"
  vk_log_auto_cpp.write(content_nondisp)
  vk_log_auto_inl.write(content_nondisp_inl)

  content_disp = ""
  content_disp_inl = ""
  for key in vulkan_mapped_types:
    content_disp_inl += "CVkLog & operator<<(const " + key + "& c);\n"
    content_disp += "CVkLog & CVkLog::operator<<(const " + key + "& c) {\n"
    content_disp += "  *this << \"{ \" << (void *)c << \" }\";\n"
    content_disp += "  return *this;\n"
    content_disp += "}\n\n"

    content_disp_inl += "CVkLog & operator<<(const " + key + "* c);\n"
    content_disp += "CVkLog & CVkLog::operator<<(const " + key + "* c) {\n"
    content_disp += "  if (c != nullptr) {\n"
    content_disp += "    if (isTraceDataOptPresent(TraceData::VK_STRUCTS))\n"
    content_disp += "      *this << *c;\n"
    content_disp += "    else\n"
    content_disp += "      _buffer << \"{ \" << (void*)c << \" }\";\n"
    content_disp += "  } else {\n"
    content_disp += "    _buffer << \"{ 0 }\";\n"
    content_disp += "  }\n"
    content_disp += "  return *this;\n"
    content_disp += "}\n\n"
  vk_log_auto_cpp.write(content_disp)
  vk_log_auto_inl.write(content_disp_inl)

  vk_log_auto_cpp.write("""
} // namespace Vulkan
} // namespace gits
""")

  return content


def generate_vulkan_tracer(functions, enums):
  vk_tracer_h = open('vulkanTracerAuto.h', 'w')
  vk_tracer_h.write(copyright_header)
  vk_tracer_cpp = open('vulkanTracerAuto.cpp', 'w')
  vk_tracer_cpp.write(copyright_header)

  content_h = """#pragma once

#include "vulkanLog.h"
#include "vulkanDrivers.h"

namespace gits {

  template<class T>
  void trace_return_value(T r) {
    VkLog(TRACE, NO_PREFIX) << " = " << r;
  }

  template<>
  void trace_return_value<void_t>(void_t) {
    VkLog(TRACE, NO_PREFIX) << "";
  }

"""

  includes = "#include \"config.h\"\n"           \
  "#include \"vulkanLog.h\"\n\n"
  namespaces = "namespace gits {\n\n"
  content_cpp = includes + namespaces

  for name in sorted(functions.keys()):
    function = functions[name][0]  # 0 because we only need the base version.

    content = "  void " + name + "_trace("
    content += arg_decl(function, False, True)
    content += ")"

    content_h += content + ";\n"
    content_cpp += content + " {\n"
    content_cpp += "    VkLog(TRACE, RAW) << \"(\""

    for arg in function['args']:
      if (arg['type'] != 'void'):
        content_cpp += " << \" " + arg['type'] + " " + arg['name'] + "=\""

        type_cast = arg['type'].replace("Flags", "FlagBits")
        if (arg['type'].find("Flags") != -1) and (type_cast in enums):
          type_cast = "(" + type_cast + ")"
        else:
          type_cast = ""

        if ('count' in arg):
          content_cpp += ";\n"
          is_pointer = False
          dereference_pointer = ""

          for argument in function['args']:
            if (argument['name'].find(arg['count']) != -1):
              if (argument['type'].find("*") != -1):
                is_pointer = True
                dereference_pointer = "*"

          content_cpp += "    if ((isTraceDataOptPresent(TraceData::VK_STRUCTS)) && (" + arg['name'] + " != nullptr)"
          if is_pointer:
            content_cpp += " && (" + arg['count'] + " != nullptr)"
          content_cpp += ") {\n"

          content_cpp += "      VkLog(TRACE, RAW) << \"{\";\n"
          content_cpp += "      for (uint32_t i = 0; i < (uint32_t)" + dereference_pointer + arg['count'] + "; ++i) {\n"
          content_cpp += "        VkLog(TRACE, RAW) << \" [\" << i << \"]:\" << " + type_cast + arg['name'] + "[i];\n"
          content_cpp += "      }\n"
          content_cpp += "      VkLog(TRACE, RAW) << \" }\";\n"
          content_cpp += "    } else {\n"
          content_cpp += "      VkLog(TRACE, RAW) << " + arg['name'] + ";\n"
          content_cpp += "    }\n"
          content_cpp += "    VkLog(TRACE, RAW) << \",\""
        else:
          content_cpp += " << " + type_cast + arg['name'] + " << \",\""

    if content_cpp.rfind("<< \",\"", len(content_cpp)-7) > 0:
      content_cpp = content_cpp[:-7]

    content_cpp += " << \" )\";\n"
    content_cpp += "  };\n"
    content_cpp += "\n"

  content_h +="} // namespace gits\n"
  content_cpp += "} // namespace gits\n"

  content_cpp = content_cpp.replace("\",\" << \" ", "\", ")
  content_cpp = content_cpp.replace("\"(\" << \" ", "\"( ")

  vk_tracer_h.write(content_h)
  vk_tracer_cpp.write(content_cpp)


def generate_vulkan_struct_storage(structs, enums):
  content = ""
  content_cpp = ""
  declared_structs = []
  vk_struct_storage_h = open('vulkanStructStorageAuto.h', 'w')
  vk_struct_storage_h.write(copyright_header)
  vk_struct_storage_cpp = open('vulkanStructStorageAuto.cpp', 'w')
  vk_struct_storage_cpp.write(copyright_header)

  begin_cpp = """#include "vulkanTools.h"
#include "vulkanTools_lite.h"
#include "vulkanStructStorageAuto.h"

"""
  begin_h = """#pragma once

#include "vulkanStructStorageBasic.h"

namespace gits{
namespace Vulkan {
"""

  vk_struct_storage_h.write(begin_h)
  vk_struct_storage_cpp.write(begin_cpp)

  content_nondisp = ""
  for key in vulkan_mapped_types_nondisp:
    content_nondisp += f"    typedef CSimpleMappedData<{key}> C{key}Data;\n"
    content_nondisp += f"    typedef CDataArray<{key}, C{key}Data> C{key}DataArray;\n"
  vk_struct_storage_h.write(content_nondisp)
  content_disp = ""
  for key in vulkan_mapped_types:
    content_disp += f"    typedef CSimpleMappedData<{key}> C{key}Data;\n"
    content_disp += f"    typedef CDataArray<{key}, C{key}Data> C{key}DataArray;\n"
  vk_struct_storage_h.write(content_disp)

  content_enums = ""
  for key in sorted(enums.keys()):
    for elem in enums[key]:
      content_enums += f"    typedef CSimpleData<{key}> C{key}Data;\n"
      content_enums += f"    typedef CDataArray<{key}, C{key}Data> C{key}DataArray;\n"

  vk_struct_storage_h.write(content_enums)
  while len(declared_structs) < len(structs):

    for key in sorted(structs.keys()):
      if key not in declared_structs:
        to_declare = []
        for elem in structs[key]:
          for var in elem['vars']:
            elem_type = var['type'].rstrip('**').rstrip('*').replace('const ', '') + '_'
            if (elem_type not in declared_structs) and (elem_type in structs.keys()):
              to_declare.append(elem_type)
          if len(to_declare) == 0:
            if elem.get('custom') is not True:
              Ctypes = []
              Cnames = []
              Cwraps = []
              Csize = []
              types = []
              key_name = key.strip('_')
              versioned_name = add_version(key, elem.get('version'))
             # content += "inline void copyVkStruct(" + key.rstrip('_') + "& dst, const " + key.rstrip('_') + " & src)\n{\n"

              for var in elem['vars']:
                Cnames.append('_' + var['name'])
                typename = re.sub(r'\:[0-9_]+', '', var['type'])    # get the base type without array brackets
                if var.get('wrapType'):
                  wrapType = ""
                  if '::CSArray' in var['wrapType'] or '::CSMapArray' in var['wrapType']:
                    wrapType = var['wrapType'].replace('::CSArray', 'DataArray').replace('::CSMapArray', 'DataArray')
                  elif 'ArrayOfArrays' in var['wrapType']:
                    wrapType = var['wrapType'].replace('ArrayOfArrays', 'DataArrayOfArrays')
                  elif 'Array' in var['wrapType']:
                    wrapType = var['wrapType'].replace('Array', 'DataArray')
                  else:
                    wrapType = var['wrapType'] + 'Data'
                  Ctypes.append(wrapType)
                elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types_nondisp and '*' in typename and 'const' not in typename:
                  typename_mod = typename.replace('const ', '').replace('*', '').replace(' ', '')
                  Ctypes.append('C' + typename_mod + 'DataArray')
                elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types and '*' in typename and 'const' not in typename:
                  typename_mod = typename.replace('const ', '').replace('*', '').replace(' ', '')
                  Ctypes.append('C' + typename_mod + 'DataArray')
                elif typename.replace('const ', '').strip(' *') in vulkan_uint32:
                  Ctypes.append('Cuint32_tData')
                elif typename.replace('const ', '').strip(' *') in vulkan_uint64:
                  Ctypes.append('Cuint64_tData')
                elif '*' in typename and typename.replace('const ', '').strip(' *') == 'void' and var['name'] == 'pNext':
                  Ctypes.append('CpNextWrapperData')
                elif '*' in typename and typename.replace('const ', '').strip(' *') == 'void':
                  Ctypes.append('CvoidPtrData')
                elif re.search(r'\[([0-9]+)\]', typename.replace('const ', '')):
                  Ctypes.append('C' + re.sub(r'\[([0-9_]+)\]', '', typename.replace('const ', '')) + 'DataArray')
                else:
                  typename_mod = typename.replace('const ', '').replace(' *', '').replace(' ', '')
                  Ctypes.append('C' + typename_mod + 'Data')
                types.append(typename.replace('const ', '').replace(' *', '').replace(' ', ''))

                if var.get('wrapParams'):
                  Cwraps.append(var['wrapParams'])
                else:
                  Cwraps.append('')
                if re.search(r'\[([0-9]+)\]', typename.replace('const ', '')):
                  size = re.findall(r'\[([0-9_]+)\]', typename)
                  Csize.append(size[0])
                else:
                  Csize.append('1')

              key_variable = key_name.replace('Vk', '').lower()
              key_decl = '_' + key_name.replace('Vk', '')
              argd = 'const ' + key_name + '* ' + key_variable
              if elem.get('constructorArgs') is not None:
                argd = elem.get('constructorArgs')
              argd_decl = 'std::unique_ptr<' + key_name + '> ' + key_decl

              argsDecl = ""
              for n, t in zip(Cnames, Ctypes):
                argsDecl += '      std::unique_ptr<' + t + '> ' + n + ';\n'
              if elem.get('passStructStorage') is True:
                argsDecl += '      VkStructStoragePointerGITS _baseIn;\n'

              counter = 0

              function_operator = ''
              init = ''
              mapped_pointers = '  std::set<uint64_t> returnMap;\n  if (!*_isNullPtr) {\n  '
              if len(elem['vars']) > 0:
                init = '  if (!*_isNullPtr)  {\n'
                init_to_nullptr = '  } else {\n'
                init_default = ''
                function_operator = 'if (' + key_decl + ' == nullptr) {\n'

              if elem.get('passStructStorage') is True:
                function_operator += """
    // Pass this structure through pNext
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
        **_pNext,                                      // const void* pNext;
        this                                           // const void* pStructStorage;
    };\n\n"""

              function_operator += '    ' + key_decl + ' = std::make_unique<' + key_name + '>();\n'
              for n, w, t, s, ot in zip(Cnames, Cwraps, Ctypes, Csize, types):
                if t != 'COutArgument':
                  counter += 1

                  if (ot not in primitive_types):
                    mapped_pointers += '  for (auto obj : ' + n + '->GetMappedPointers())\n      returnMap.insert((uint64_t)obj);\n  '

                  if w == '':
                    struct_type = t[1:].replace("Data", "") + '_'
                    if (struct_type) in structs.keys():
                      init += '    ' + n + ' = std::make_unique<' + t + '>(&' + key_variable + '->' + n.strip('_') + ');\n'
                    elif t[1:].strip(' *') in vulkan_mapped_types_nondisp:
                      init += '    ' + n + ' = std::make_unique<' + t + '>(' + key_variable + '->' + n.strip('_') + ');\n'
                    elif t[1:].strip(' *') in vulkan_mapped_types:
                      init += '    ' + n + ' = std::make_unique<' + t + '>(' + key_variable + '->' + n.strip('_') + ');\n'  # uint64_t cast
                    elif s != '1':
                      init += '    ' + n + ' = std::make_unique<' + t + '>(' + s + ', ' + key_variable + '->' + n.strip('_') + ');\n'
                    else:
                      init += '    ' + n + ' = std::make_unique<' + t + '>(' + key_variable + '->' + n.strip('_') + ');\n'
                  else:
                    init += '    ' + n + ' = std::make_unique<' + t + '>(' + w + ');\n'

                  init_to_nullptr += '    ' + n + ' = nullptr;\n'
                  init_default += n + '(std::make_unique<' + t + '>()), '
                  if s != '1':
                    function_operator += '    auto ' + n.strip('_') + 'Values = **' + n + ';\n'
                    function_operator += '    if (' + n.strip('_') + 'Values != nullptr) {\n'
                    function_operator += '      for (int i = 0; i < ' + s + '; i++)\n'
                    function_operator += '        ' + key_decl + '->' + n.strip('_') + '[i] = ' + n.strip('_') + 'Values[i];\n'
                    function_operator += '    } else {\n'
                    function_operator += '      throw std::runtime_error(EXCEPTION_MESSAGE);\n'
                    function_operator += '    }\n'
                  else:
                    member_value = '    ' + key_decl + '->' + n.strip('_') + ' = **' + n + ';\n'
                    if elem.get('passStructStorage') is True:
                      member_value = member_value.replace('**_pNext', '&_baseIn')
                    function_operator += member_value

              mapped_pointers += '}\n  return returnMap;'
              if elem.get('passStructStorage') is True:
                init += """
    // Pass data to vulkan arguments class
    _baseIn = {
        VK_STRUCTURE_TYPE_STRUCT_STORAGE_POINTER_GITS, // VkStructureType sType;
        %(key_variable)s->pNext,                       // const void* pNext;
        this                                           // const void* pStructStorage;
    };
    const_cast<%(unversioned_name)s*>(%(key_variable)s)->pNext = &_baseIn;\n""" % {'unversioned_name': key_name, 'key_variable': key_variable}
              if len(elem['vars']) > 0:
                init += init_to_nullptr
                init += '  }'
                function_operator += '  }'
              if counter > 0:
                init_default = init_default.strip(', ')
              else:
                init = ''
              function_operator += '\n  return ' + key_decl + '.get();'
              key_name_data = versioned_name + "Data"
              if len(elem['vars']) > 0:
                content += """    struct C%(name_data)s : public CBaseDataStruct, gits::noncopyable {
%(argsDecl)s
      %(argd_decl)s;
      CboolData _isNullPtr;

      C%(name_data)s(%(argd)s);
      %(unversioned_name)s* Value();

      PtrConverter<%(unversioned_name)s> operator*() {
        return PtrConverter<%(unversioned_name)s>(Value());
      }
      void * GetPtrType() override { return (void *)Value(); }
      std::set<uint64_t> GetMappedPointers();
    };
""" % {'unversioned_name': key_name, 'name_data': key_name_data, 'argd': argd, 'argsDecl': argsDecl, 'argd_decl': argd_decl}
                content += "    typedef CDataArray<" + key_name + ", C" + key_name_data + "> C" + key_name_data + "Array;\n"
                content += "    typedef CDataArrayOfArrays<" + key_name + ", C" + key_name_data + "> C" + key_name_data + "ArrayOfArrays;\n\n\n"

                constructor = """gits::Vulkan::C%(name_data)s::C%(name_data)s(%(argd)s) : %(key_decl)s(nullptr), _isNullPtr(%(key_variable)s == nullptr) {
%(init)s
}""" % {'name_data' : key_name_data, 'argd': argd, 'key_decl': key_decl, 'key_variable': key_variable, 'init': init}

                if elem.get('constructorWrap') is True:
                  constructor = '// for constructor see vulkanStructStorageWrap.cpp'

                content_cpp += """%(constructor)s

%(unversioned_name)s* gits::Vulkan::C%(name_data)s::Value() {
  if (*_isNullPtr)
    return nullptr;
  %(function_operator)s
}

std::set<uint64_t> gits::Vulkan::C%(name_data)s::GetMappedPointers() {
%(mapped_pointers)s
}

""" % {'constructor': constructor, 'unversioned_name': key_name, 'name_data': key_name_data, 'function_operator': function_operator, 'mapped_pointers': mapped_pointers}

            declared_structs.append(key)
  vk_struct_storage_cpp.write(content_cpp)
  end_h = """
  } // namespace Vulkan
} // namespace gits
"""
  vk_struct_storage_h.write(content)
  vk_struct_storage_h.write(end_h)


def generate_vulkan_header(enums, structs, functions):
  vk_header = open('vulkanHeader.h', 'w')
  vk_header.write(copyright_header)
  output = ""

  vk_header_include = """#pragma once

#include "vulkan_basic.h"
#ifdef GITS_PLATFORM_WINDOWS
#ifdef BUILD_FOR_CCODE
// If we include windows.h, it includes further headers. One of them defines
// WGL functions which we have already defined ourselves. This breaks OGL and
// OCL CCode. So instead of including windows.h, we just declare the types we
// need from it.
struct _SECURITY_ATTRIBUTES;
typedef _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
#else
#include <windows.h>
#endif  // BUILD_FOR_CCODE
#endif  // GITS_PLATFORM_WINDOWS
"""
  vk_header.write(vk_header_include)
  for key in sorted(enums.keys()):
    for enum in enums[key]:

      if enum['size'] == 64:
        output += "typedef enum " + key + "_ : VkFlags64 {\n"
      else:
        output += "typedef enum " + key + "_ {\n"

      for var in enum['vars']:
        output += "  " + var['name']
        ending = ",\n"

        if int(var['value']) >= 0:
          output += " = " + hex(int(var['value'])).rstrip('L') + ending
        else:
          output += " = -(" + hex(abs(int(var['value']))).rstrip('L') + ")" + ending

      output += "} " + key + ";\n\n"

  for key in sorted(structs.keys()):
    struct = structs[key][0]  # 0 because we only need the base version.
    decl_name = "struct"
    if struct.get('type'):
      decl_name = struct.get('type')
    output += decl_name + " " + key + ";\n"
    output += "typedef " + key + " " + key.rstrip('_') + ";\n\n"
  for key in sorted(functions.keys()):
    function = functions[key][0]  # 0 because we only need the base version.
    output += "typedef " + function['type'] + " (VKAPI_PTR *PFN_" + key + ") ("
    output += arg_decl(function, False, True)
    output += ");\n"
  output += "\n#ifdef VK_PROTOTYPES\n"
  for key in sorted(functions.keys()):
    function = functions[key][0]  # 0 because we only need the base version.
    output += "VKAPI_ATTR " + function['type'] + " VKAPI_CALL " + key + "(\n"
    output += arg_decl(function, True, True)
    output += ");\n\n"
  output += "#endif\n"
  declared_structs = []
  while len(declared_structs) < len(structs):
    for key in sorted(structs.keys()):
      if key not in declared_structs:
        to_declare = []
        struct = structs[key][0]  # 0 because we only need the base version.
        for var in struct['vars']:
          elem_type = var['type'] + '_'
          if (elem_type not in declared_structs) and (elem_type in structs.keys()):
            to_declare.append(elem_type)
        if len(to_declare) == 0:
          if struct.get('type'):
            output += struct.get('type') + " " + key + " {\n"
          else:
            output += "struct " + key + " {\n"
          import re

          for var in struct['vars']:
            var_type = var['type']
            var_name = var['name']
            if re.search(r':([A-Za-z0-9_]+)', var_type):
              bit_field_found = re.search(r':([A-Za-z0-9_]+)', var_type)
              var_type = re.sub(r':([A-Za-z0-9_]+)', '', var_type)
              var_name += bit_field_found.group(0)

            regex_string = r'(\[([A-Za-z0-9_]+)\])+'
            regex_result = re.search(regex_string, var_type)
            if regex_result:
              output += "  " + re.sub(regex_string, '', var_type) + " " + var_name + regex_result.group(0) + ";\n"
            else:
              output += "  " + var_type + " " + var_name + ";\n"
          output += "}" + ";\n\n"
          declared_structs.append(key)

  vk_header.write(output)
  vk_header.close()


def generate_vulkan_def(functions, def_filename, library_name):
  plugin_def = open(def_filename, 'w')
  header_def = copyright_header_def + f"""
  LIBRARY {library_name}
  EXPORTS
""".lstrip('\n')
  plugin_def.write(header_def)
  for key in sorted(functions.keys()):
    if functions[key][0]['level'] == FuncLevel.PROTOTYPE:
      continue
    plugin_def.write("    " + key + "\n")

  plugin_def.close()


def generate_layer_json():
  layer_json = open('VkLayer_vulkan_GITS_recorder.json', 'w')

  vulkan_layer_bin = ".\\\\VkLayer_vulkan_GITS_recorder.dll" #Windows
  if sys.platform.lower().startswith('linux'):
    vulkan_layer_bin = "./libVkLayer_vulkan_GITS_recorder.so" #Linux
  contents = """{
    "file_format_version" : "1.1.0",
    "layer" : {
        "name": "VK_LAYER_INTEL_vulkan_GITS_recorder",
        "type": "GLOBAL",
        "library_path": "%(vulkan_layer_bin)s",
        "api_version": "1.3.248",
        "implementation_version": "1",
        "description": "Vulkan layer used to record GITS Vulkan streams"
    }
}
""" % {'vulkan_layer_bin': vulkan_layer_bin}

  layer_json.write(contents)
  layer_json.close()


def generate_prepost(functions):
  prepost_c = open('vulkanPrePostAuto.cpp', 'w')
  prepost_c.write(copyright_header)
  prepost_c_include = "\n#include \"vulkanExecWrap.h\"\n"
  prepost_c.write(prepost_c_include)
  prepost_c_static = """
namespace {
  std::recursive_mutex globalMutex;
} // namespace

using namespace gits::Vulkan;

void thread_tracker() {
  static std::map<std::thread::id, int> threadIdMap;
  static int currentThreadId = 0;
  static int generatedThreadId = 0;

  //Add thread to map if not mapped
  if (threadIdMap.find(std::this_thread::get_id()) == threadIdMap.end()) {
    threadIdMap[std::this_thread::get_id()] = generatedThreadId;
    generatedThreadId++;
  }

  //If thread changed schedule thread change
  if (currentThreadId != threadIdMap[std::this_thread::get_id()]) {
    currentThreadId = threadIdMap[std::this_thread::get_id()];
    Log(TRACE) << "ThreadID: " << currentThreadId;
  }
}

namespace {
  // Avoid recording API - recursive functions.
  std::atomic<uint32_t> recursionDepth = 0;
  const std::atomic<uint32_t> disableDepth = 1000;
} // namespace

void PrePostDisableVulkan() {
  recursionDepth.store(disableDepth);
  CGitsPluginVulkan::_recorderFinished = true;
}

void EndFramePost() {
  CGitsPluginVulkan::RecorderWrapper().EndFramePost();
}

void CloseRecorderIfRequired() {
  CGitsPluginVulkan::RecorderWrapper().CloseRecorderIfRequired();
}

#define VKATTRIB

#define GITS_WRAPPER_PRE                                            \\
  --recursionDepth;                                                 \\
  if (CGitsPluginVulkan::Configuration().common.recorder.enabled) { \\
    if (recursionDepth == 0) {                                      \\
      try {

#define GITS_WRAPPER_POST                                           \\
        CloseRecorderIfRequired();                                  \\
      }                                                             \\
      catch (...) {                                                 \\
        topmost_exception_handler(__FUNCTION__);                    \\
      }                                                             \\
    }                                                               \\
  }

#define GITS_ENTRY                                                  \\
  ++recursionDepth;                                                 \\
  thread_tracker();                                                 \\
  IRecorderWrapper& wrapper = CGitsPluginVulkan::RecorderWrapper();

#define GITS_MUTEX std::unique_lock<std::recursive_mutex> lock(globalMutex);
#define GITS_ENTRY_VK GITS_MUTEX GITS_ENTRY
"""
  extern_str = "\nextern \"C\" {\n"
  prepost_c_static += extern_str
  prepost_c.write(prepost_c_static)

  interceptorExportedFunctions = ""

  for function_name in sorted(functions.keys()):
    function = functions[function_name][0]  # 0 because we only need the base version.
    if function['level'] == FuncLevel.PROTOTYPE:
      continue

    interceptorExportedFunctions += "  {\"" + function_name + "\", (PFN_vkVoidFunction)" + function_name + "},\n"

    return_type = function['type']
    function_arguments = arg_decl(function, False, True)
    stored_value = "auto return_value = "
    success_value = "return_value, "
    function_call = "wrapper.Drivers()."
    function_arguments_call = arg_call(function, False, True)
    returned_value = "return return_value;"

    if (function.get('recExecWrap') is True):
      function_call = "recExecWrap_"
    if (return_type == "void"):
      stored_value = ""
      success_value = ""
      returned_value = ""

    function_pre_call = "\n  " + stored_value + function_call + function_name + "(" + function_arguments_call + ");"
    function_post_call = ""

    if (function.get('execPostRecWrap') is True):
      function_post_call = function_pre_call
      function_pre_call = ""
      if (return_type != "void"):
        success_value = "VK_SUCCESS, "
    if (function.get('endFrameTag') is True):
      function_post_call += "\n  EndFramePost();"

    gits_plugin_initialization = ""
    if function['level'] == FuncLevel.GLOBAL:
      gits_plugin_initialization = "CGitsPluginVulkan::Initialize();"
    func_def = f"\nVKATTRIB VISIBLE {return_type} VKAPI_CALL {function_name}({function_arguments}) {{\n"
    func_body = (
      f"  GITS_ENTRY_VK{function_pre_call}\n"
      f"  GITS_WRAPPER_PRE\n"
      f"  wrapper.{function_name}({success_value}{function_arguments_call});\n"
      f"  GITS_WRAPPER_POST{function_post_call}"
      )
    if gits_plugin_initialization:
      func_body = f"  {gits_plugin_initialization}\n{func_body}"
    if returned_value:
      func_body = f"{func_body}\n  {returned_value}"
    content = f"{func_def}{func_body}\n}}\n"
    prepost_c.write(content)

  content = """}

const std::unordered_map<std::string, PFN_vkVoidFunction> interceptorExportedFunctions = {
%(interceptorExportedFunctions)s};
""" % {'interceptorExportedFunctions': interceptorExportedFunctions}
  prepost_c.write(content)

  prepost_c.close()


def generate_vulkan_tokens(functions):
  tokens_h = open('vulkanFunctions.h', 'w')
  tokens_c = open('vulkanFunctions.cpp', 'w')
  tokens_h.write(copyright_header)
  tokens_c.write(copyright_header)

  tokens_c_include = """#include "vulkanFunctions.h"
#include "vulkanPlayerRunWrap.h"
#include "vulkanStateTracking.h"
"""
  tokens_h_include = """#pragma once

#include <array>

#include "vkFunction.h"
#include "vulkanArgumentsAuto.h"

namespace gits {
  namespace Vulkan{
    using std::uint64_t;
"""
  tokens_c.write(tokens_c_include)
  tokens_h.write(tokens_h_include)
  for key in sorted(functions.keys()):
    for elem in functions[key]:
      Ctypes = []
      Cnames = []
      Cwraps = []
      types = []
      counter = 0
      versioned_name = add_version(key, elem.get('version'))
      if (elem['type'] != 'void'):
        Cnames.append('_return_value')
        typename = elem['type']
        if elem.get('retVwrapType'):
          Ctypes.append(elem.get('retVwrapType'))
        elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types_nondisp and '*' in typename and 'const' not in typename:
          typename_mod = typename.replace('*', '').replace('const ', '').replace(' ', '')
          Ctypes.append('C'+typename_mod+'::CSArray')
        elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types and '*' in typename and 'const' not in typename:
          typename_mod = typename.replace('*', '').replace('const ', '').replace(' ', '')
          Ctypes.append('C'+typename_mod+'::CSArray')
        elif typename.replace('const ', '') in vulkan_uint32:
          Ctypes.append('Cuint32_t')
        elif typename.replace('const ', '') in vulkan_uint64:
          Ctypes.append('Cuint64_t')
        elif '*' in typename and typename.replace('const ', '').strip(' *') == 'void' and arg['name'] == 'pNext':
          Ctypes.append('CpNextWrapper')
        else:
          Ctypes.append('C' + typename.replace('const ', ''))
        types.append(typename.replace('*', '').replace('const ', '').replace(' ', ''))
        if (elem.get('retVwrapParams')):
          Cwraps.append(elem.get('retVwrapParams'))
        else:
          Cwraps.append('')

      remove_mapping = ""
      for arg in elem['args']:
        Cnames.append('_' + arg['name'])
        typename = arg['type']#.strip(' *')
        if arg.get('removeMapping') is True:
          remove_mapping += "\n  %(name)s.RemoveMapping();" % {'name': '_' + arg['name']}
        if arg.get('wrapType'):
          Ctypes.append(arg['wrapType'])
        elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types_nondisp and '*' in typename and 'const' not in typename:
          typename_mod = typename.replace('*', '').replace('const ', '').replace(' ', '')
          Ctypes.append('C'+typename_mod+'::CSArray')
        elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types and '*' in typename and 'const' not in typename:
          typename_mod = typename.replace('*', '').replace('const ', '').replace(' ', '')
          Ctypes.append('C'+typename_mod+'::CSArray')
        elif typename.replace('const ', '') in vulkan_uint32:
          Ctypes.append('Cuint32_t')
        elif typename.replace('const ', '') in vulkan_uint64:
          Ctypes.append('Cuint64_t')
        elif '*' in typename and typename.replace('const ', '').strip(' *') == 'void' and arg['name'] == 'pNext':
          Ctypes.append('CpNextWrapper')
        elif '*' in typename and typename.replace('const ', '').strip(' *') not in structs_names:
          typename_mod = typename.replace('const ', '').strip(' *')
          Ctypes.append('C' + typename_mod + '::CSArray')
        else:
          typename_mod = typename.replace('const ', '').strip(' *')
          Ctypes.append('C' + typename_mod)
        types.append(typename.replace('*', '').replace('const ', '').replace(' ', ''))
        if arg.get('wrapParams'):
          Cwraps.append(arg['wrapParams'])
        elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types_nondisp and '*' in typename and 'const' not in typename:
          Cwraps.append('1, ' + arg['name'])
        elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types and '*' in typename and 'const' not in typename:
          Cwraps.append('1, ' + arg['name'])  # uint64_t cast
        elif typename.replace('const ', '').strip(' ') in vulkan_mapped_types_nondisp and '*' not in typename:
          Cwraps.append(arg['name'])
        elif typename.replace('const ', '').strip(' ') in vulkan_mapped_types and '*' not in typename:
          Cwraps.append(arg['name'])  # uint64_t cast
        else:
          Cwraps.append('')
        counter += 1
      func = elem
      argd = arg_decl(elem)

      argsDecl = ""
      for n, t in zip(Cnames, Ctypes):
        argsDecl += '      ' + t + ' ' + n + ';\n'

      def undecorate(type):
        # type: (str) -> str
        """Strip the type to its core, e.g. const float* to just float."""
        return type.replace('const ', '', 1).strip('* ')

      argInfos = ""
      wrapTypes = list(Ctypes)
      # Remove retval if present so the arguments match their wraps.
      try:
        wrapTypes.remove('CVkResult')
      except ValueError:
        pass
      for arg, wraptype in zip(elem['args'], wrapTypes):
        typename = arg['type']
        numPtr = typename.count('*')
        needs_ampersand = 'false'

        argInfos += '{ gits::Vulkan::ArgType::'
        if undecorate(typename) in opaque_handles:
          argInfos += 'OPAQUE_HANDLE'
        elif undecorate(typename) in vulkan_enums: # TODO: We are looking for a str in list[str], so it works, right?
          argInfos += 'ENUM'
        elif undecorate(typename) in primitive_types:
          argInfos += 'PRIMITIVE_TYPE'
          if (undecorate(typename) in vulkan_union) and (numPtr == 1):
            needs_ampersand = 'true'
        elif undecorate(typename) in vulkan_structs:
          argInfos += 'STRUCT'
          if not wraptype.endswith('Array'):
            needs_ampersand = 'true'
        elif 'void*' in typename:
          argInfos += 'OTHER'  # void* or void**
        else:
          argInfos += 'OTHER'
          print("Warning: type \"" + typename + "\" is of unknown category.")

        argInfos += ', ' + str(numPtr) + ', ' + needs_ampersand + ' }, // ' + typename + ' (' + wraptype + ')\n  '
      argInfos = argInfos.strip('\n ')

      argsCall = ""
      argsCallOrig = ""
      wrapCall = ""
      stateTrackCall = ""
      for n in Cnames:
        if n != '_return_value':
          argsCall += '*' + n + ', '
        argsCallOrig += n + '.Original(), '
        wrapCall += n + ', '
        stateTrackCall += '*' + n + ', '
      argsCall = argsCall.strip(', ')
      argsCallOrig = argsCallOrig.strip(', ')
      wrapCall = wrapCall.strip(', ')
      stateTrackCall = stateTrackCall.strip(', ')

      c = ""
      inherit_type = 'CFunction'
      run_name = 'Run'
      if func.get('functionType') & FuncType.QUEUE_SUBMIT:
        inherit_type = 'CQueueSubmitFunction'
        run_name = 'RunImpl'
      elif func.get('functionType') & FuncType.CREATE_IMAGE:
        inherit_type = 'CImageFunction'
        run_name = 'RunImpl'
      if func.get('functionType') & FuncType.CREATE_BUFFER:
        inherit_type = 'CBufferFunction'
        run_name = 'RunImpl'
      return_override = ""
      if func['type'] != 'void':
        return_override = "\n      virtual const CArgument* Return() const { return (stream_older_than(GITS_VULKAN_RETURN_VALUE_FIX) && Config::IsPlayer()) ? CFunction::Return() : &_return_value; }"

      ccodePostActionNeeded = ""
      if func.get('ccodePostActionNeeded') is False:
        ccodePostActionNeeded = "\n      virtual bool CCodePostActionNeeded() const override { return false; }"

      suffix = ""
      if func.get('ccodeWrap') is True:
        suffix = "\n      virtual const char* Suffix() const { return \"_CCODEWRAP\"; }"

      write_wrap_decl = ""
      write_wrap_def = ""
      if func.get('ccodeWriteWrap') is True:
        write_wrap_decl += '\n      virtual void Write(CCodeOStream& stream) const override;'
        write_wrap_decl += '\n      friend void C' + versioned_name + '_CCODEWRITEWRAP(CCodeOStream& stream, const C' + versioned_name + '& function);'
        write_wrap_def += '\n\nvoid gits::Vulkan::C' + versioned_name + '::Write(CCodeOStream& stream) const {'
        write_wrap_def += '\n  stream.select(stream.selectCCodeFile());'
        write_wrap_def += '\n  C' + versioned_name + '_CCODEWRITEWRAP(stream, *this);'
        write_wrap_def += '\n}'
        if func.get('ccodeWrap') is True:
          raise RuntimeError("If ccodeWriteWrap is enabled, ccodeWrap does "
                             "not do anything. Having both enabled "
                             "indicates a logic error.")
      command_buffer_decl = ""
      command_buffer_def = ""
      if func.get('tokenCache') is not None:
        command_buffer_decl = '\n      virtual VkCommandBuffer CommandBuffer();'
        command_buffer_def = '\n\nVkCommandBuffer gits::Vulkan::C' + versioned_name + '::CommandBuffer() {'
        command_buffer_def += '\n  return _commandBuffer.Original();'
        command_buffer_def += '\n}'

      if len(func['args']) > 0 or func['type'] != 'void':
        c = """
    class C%(versioned_name)s : public %(inherit_type)s, gits::noncopyable {
%(argsDecl)s
      virtual CArgument &Argument(unsigned idx) override;
      static const std::array<ArgInfo, %(argc)s> argumentInfos_;
      virtual ArgInfo ArgumentInfo(unsigned idx) const override;
      virtual unsigned ArgumentCount() const override { return %(argc)s; }%(return_override)s

    public:
      C%(versioned_name)s();
      C%(versioned_name)s(%(argd)s);
      virtual unsigned Id() const override { return %(id)s; }
      virtual unsigned Type() const { return %(type)s;}
      virtual const char *Name() const override { return "%(unversioned_name)s"; }%(suffix)s%(ccodePostActionNeeded)s
      virtual void %(run_name)s() override;%(write_wrap_decl)s%(command_buffer_decl)s
      virtual void Exec();
      virtual void StateTrack();
      virtual void RemoveMapping();
      virtual std::set<uint64_t> GetMappedPointers();
      virtual void TokenBuffersUpdate();
    };""" % {'unversioned_name': key, 'versioned_name': versioned_name, 'return_override': return_override, 'id': make_id(key, func['version']), 'argc': len(func['args']), 'argd': argd, 'argsDecl': argsDecl, 'inherit_type': inherit_type, 'run_name': run_name, 'write_wrap_decl': write_wrap_decl, 'command_buffer_decl': command_buffer_decl, 'suffix': suffix, 'ccodePostActionNeeded': ccodePostActionNeeded, 'type': make_type(func)}
      else:
        c = """
    class C%(versioned_name)s : public %(inherit_type)s, gits::noncopyable {
%(argsDecl)s
      virtual CArgument &Argument(unsigned idx) override;
      static const std::array<ArgInfo, %(argc)s> argumentInfos_;
      virtual ArgInfo ArgumentInfo(unsigned idx) const override;
      virtual unsigned ArgumentCount() const override { return %(argc)s; }

    public:
      C%(versioned_name)s();
      virtual unsigned Id() const override { return %(id)s; }
      virtual unsigned Type() const { return %(type)s;}
      virtual const char *Name() const override { return "%(unversioned_name)s"; }%(suffix)s
      virtual void %(run_name)s() override;%(write_wrap_decl)s%(command_buffer_decl)s
      virtual void Exec();
      virtual void StateTrack();
      virtual void RemoveMapping();
      virtual std::set<uint64_t> GetMappedPointers();
      virtual void TokenBuffersUpdate();
    };""" % {'unversioned_name': key, 'versioned_name': versioned_name, 'id': make_id(key, func['version']), 'argc': len(func['args']), 'argsDecl': argsDecl, 'inherit_type': inherit_type, 'run_name': run_name, 'write_wrap_decl': write_wrap_decl, 'command_buffer_decl': command_buffer_decl, 'suffix': suffix, 'type': make_type(func)}
      tokens_h.write(c + '\n')

      cargument = 'return get_cargument(__FUNCTION__, idx, '
      count = 0
      for arg in Cnames:
        if (arg != '_return_value'):
          cargument += arg + ', '
          count = count + 1
      cargument = cargument.strip(', ')
      cargument += ');'
      if (count == 0):
        cargument = 'report_cargument_error(__FUNCTION__, idx);'
      init = ''
      if len(func['args']) > 0 or func['type'] != 'void':
        init = ':\n  '
      mapped_pointers = '\n  std::set<uint64_t> returnMap;\n  '
      counter = 0
      for n, w, t, o in zip(Cnames, Cwraps, Ctypes, types):
        if t != 'COutArgument':
          counter += 1
          if (o not in primitive_types):
            mapped_pointers += 'for (auto obj : ' + n + '.GetMappedPointers())\n    returnMap.insert((uint64_t)obj);\n  '
          if w == '':
            init += n + '(' + n.strip('_') + '), '
          else:
            init += n + '(' + w + '), '
      if counter > 0:
        init = init.strip(', ')
      else:
        init = ''
      mapped_pointers += 'return returnMap;'

      run_cmd = """Exec();
  StateTrack();
  RemoveMapping();"""
      if func.get('runWrap') is True:
        run_cmd = "%(name)s_WRAPRUN(%(wrapCall)s);" % {'name': func.get('runWrapName'), 'wrapCall': wrapCall}
      exec_cmd = "drvVk.%(name)s(%(argsCall)s)" % {'name': key, 'argsCall': argsCall}
      state_track = ""
      if func.get('stateTrack') is True:
        state_track = "\n  %(name)s_SD(%(argsCall)s);" % {'name': func.get('stateTrackName'), 'argsCall': stateTrackCall}
      return_value = ""
      return_value_end = ";"
      if func.get('type') not in ('void', 'VkDeviceAddress'):
        return_value = "_return_value.Assign("
        return_value_end = ");"
      token_buff_update = ""
      if func.get('tokenCache') is not None:
        token_buff_update = "\n  SD()._commandbufferstates[*_commandBuffer]->tokensBuffer.Add(new C%(name)s(%(argsCallOrig)s));" % {'name': versioned_name, 'argsCallOrig': argsCallOrig}
        if func.get('runWrap') is not True:
          run_cmd = """if (Config::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    TokenBuffersUpdate();
  } else {
    Exec();
    StateTrack();
    RemoveMapping();
  }"""
      c = ""
      if len(func['args']) > 0 or func['type'] != 'void':
        c = """
/* ***************************** %(id)s *************************** */

const std::array<gits::Vulkan::ArgInfo, %(argc)s> gits::Vulkan::C%(versioned_name)s::argumentInfos_ = {{
  %(argInfos)s
}};

gits::Vulkan::C%(versioned_name)s::C%(versioned_name)s()
{
}


gits::Vulkan::C%(versioned_name)s::C%(versioned_name)s(%(argd)s)%(init)s
{
}

gits::CArgument &gits::Vulkan::C%(versioned_name)s::Argument(unsigned idx)
{
  %(cargument)s
}

gits::Vulkan::ArgInfo gits::Vulkan::C%(versioned_name)s::ArgumentInfo(unsigned idx) const
{
  return argumentInfos_[idx];
}

std::set<uint64_t> gits::Vulkan::C%(versioned_name)s::GetMappedPointers()
{%(mapped_pointers)s
}

void gits::Vulkan::C%(versioned_name)s::%(run_name)s()
{
  %(run_cmd)s
}

void gits::Vulkan::C%(versioned_name)s::Exec()
{
  %(return_value)s%(exec_cmd)s%(return_value_end)s
}

void gits::Vulkan::C%(versioned_name)s::StateTrack()
{%(state_track)s
}

void gits::Vulkan::C%(versioned_name)s::TokenBuffersUpdate()
{%(token_buff_update)s
}

void gits::Vulkan::C%(versioned_name)s::RemoveMapping()
{%(remove_mapping)s
}%(write_wrap_def)s%(command_buffer_def)s""" % {'id': make_id(key, func['version']), 'versioned_name': versioned_name, 'cargument': cargument, 'argc': len(func['args']), 'argd': argd, 'argInfos': argInfos, 'init': init, 'run_name': run_name, 'write_wrap_def': write_wrap_def, 'command_buffer_def': command_buffer_def, 'mapped_pointers': mapped_pointers, 'exec_cmd': exec_cmd, 'state_track': state_track, 'return_value': return_value, 'return_value_end': return_value_end, 'remove_mapping': remove_mapping, 'run_cmd': run_cmd, 'token_buff_update': token_buff_update}
      else:
        c = """
/* ***************************** %(id)s *************************** */

const std::array<gits::Vulkan::ArgInfo, %(argc)s> gits::Vulkan::%(versioned_name)s::argumentInfos_ = {{
  %(argInfos)s
}};

gits::Vulkan::C%(versioned_name)s::C%(versioned_name)s()
{
}

gits::CArgument &gits::Vulkan::C%(versioned_name)s::Argument(unsigned idx)
{
  %(cargument)s
}

gits::Vulkan::ArgInfo gits::Vulkan::C%(versioned_name)s::ArgumentInfo(unsigned idx) const
{
  return argumentInfos_[idx];
}

std::set<uint64_t> gits::Vulkan::C%(versioned_name)s::GetMappedPointers()
{%(mapped_pointers)s
}

void gits::Vulkan::C%(versioned_name)s::%(run_name)s()
{
  %(run)s
}%(write_wrap_def)s%(command_buffer_def)s""" % {'id': make_id(key, func['version']), 'versioned_name': versioned_name, 'cargument': cargument, 'argc': len(func['args']), 'argInfos': argInfos, 'run': run, 'run_name': run_name, 'write_wrap_def': write_wrap_def, 'command_buffer_def': command_buffer_def, 'mapped_pointers': mapped_pointers}
      tokens_c.write(c + '\n')
  tokens_h_end = """  } // namespace Vulkan
} // namespace gits
"""
  tokens_h.write(tokens_h_end)
  tokens_h.close()
  tokens_c.close()


def generate_vulkan_recorder_wrapper(functions):
  wrap_h = open('vulkanRecorderWrapperAuto.h', 'w')
  wrap_c = open('vulkanRecorderWrapperAuto.cpp', 'w')
  wrap_i = open('vulkanRecorderWrapperIfaceAuto.h', 'w')
  wrap_h.write(copyright_header)
  wrap_c.write(copyright_header)
  wrap_i.write(copyright_header)

  wrap_c_include = """#include "vulkanRecorderWrapper.h"
#include "vulkanRecorderSubwrappers.h"

namespace gits {
namespace Vulkan {
"""
  wrap_c.write(wrap_c_include)
  for key in sorted(functions.keys()):
    value = functions[key][-1]
    if (value.get('enabled') is True) or (value.get('recWrap') is True):
      wrapper_pre_post = ""
      if value.get('functionType') & FuncType.QUEUE_SUBMIT:
        wrapper_pre_post = "QUEUE_SUBMIT_WRAPPER_PRE_POST\n  "
      elif value.get('functionType') & FuncType.CREATE_IMAGE:
        wrapper_pre_post = "CREATE_IMAGE_WRAPPER_PRE_POST\n  "
      elif value.get('functionType') & FuncType.CREATE_BUFFER:
        wrapper_pre_post = "CREATE_BUFFER_WRAPPER_PRE_POST\n  "
      pre_token = ""
      if value.get('preToken') is not None and (value.get('recWrap') is not True) and value.get('preToken') is not False:
        pre_token = "_recorder.Schedule(new %(name)s);\n    " % {'name': value.get('preToken')}
      post_token = ""
      if value.get('postToken') is not None and (value.get('recWrap') is not True):
        post_token = "\n    _recorder.Schedule(new %(name)s);\n" % {'name': value.get('postToken')}
      rec_cond = ""
      if (value.get('tokenCache') is not None) and (value.get('recWrap') is not True):
        rec_cond = "if (_recorder.Running() && !Config::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {"
      elif value.get('recWrap') is not True:
        rec_cond = "if (_recorder.Running()) {"
      state_track = ""
      rec_cond_end = ""
      new_line = ""
      if value.get('stateTrack') is True and (value.get('recWrap') is not True):
        state_track = "\n  %(name)s_SD(%(argsCall)s);" % {'name': value.get('stateTrackName'), 'argsCall': arg_call(value)}
      rec_wrap = ""
      if (value.get('recWrap') is True):
        rec_wrap = "%(name)s_RECWRAP(%(argsCall)s, _recorder);" % {'name': value.get('recWrapName'), 'argsCall': arg_call(value)}
      else:
        rec_cond_end = "\n  }"
        new_line = "\n    "
      schedule = ""
      versioned_name = add_version(key, value.get('version'))

      if (value.get('recWrap') is not True):
        schedule = "_recorder.Schedule(new C%(name)s(%(arg_call)s));" % {'name': versioned_name, 'arg_call': arg_call(value)}
      tokenCache = ""
      if (value.get('tokenCache') is not None) and (value.get('recWrap') is not True):
        tokenCache = " else {\n    " + value.get('tokenCache')
        tokenCache += ".Add(new C%(name)s(%(arg_call)s));" % {'name': versioned_name, 'arg_call': arg_call(value)}
        tokenCache += "\n  }"
      wc = """
void CRecorderWrapper::%(name)s(%(arg_decl)s) const
{
  %(wrapper_pre_post)s%(rec_cond)s%(rec_wrap)s%(new_line)s%(pre_token)s%(schedule)s%(post_token)s%(rec_cond_end)s%(tokenCache)s%(state_track)s
}""" % {'name': key, 'arg_decl': arg_decl(value), 'pre_token': pre_token, 'post_token': post_token, 'rec_cond': rec_cond, 'state_track': state_track, 'rec_wrap': rec_wrap, 'schedule': schedule, 'tokenCache': tokenCache, 'rec_cond_end': rec_cond_end, 'wrapper_pre_post': wrapper_pre_post, 'new_line': new_line}
      wrap_c.write(wc + '\n')
      wrap_h.write('void ' + key + '(' + arg_decl(value) + ') const override;\n')
      wrap_i.write('virtual void ' + key + '(' + arg_decl(value) + ') const = 0;\n')
    else:
      wc = """
void CRecorderWrapper::%(name)s(%(arg_decl)s) const
{
  CALL_ONCE [] { Log(ERR) << "function %(name)s not implemented"; };
}""" % {'name': key, 'arg_decl': arg_decl(value)}
      wrap_c.write(wc + '\n')
      wrap_h.write('void ' + key + '(' + arg_decl(value) + ') const override;\n')
      wrap_i.write('virtual void ' + key + '(' + arg_decl(value) + ') const = 0;\n')
  wrap_c.write('} // namespace Vulkan\n} // namespace gits\n')


def generate_vulkan_arguments(structs, enums_dict):
  arguments_h = open('vulkanArgumentsAuto.h', 'w')
  arguments_cpp = open('vulkanArgumentsAuto.cpp', 'w')
  arguments_h.write(copyright_header)
  arguments_cpp.write(copyright_header)
  arguments_h_include = """#pragma once

#include "vulkanArgumentsBasic.h"

namespace gits {
namespace Vulkan {
"""
  arguments_h_end = """
} // namespace Vulkan
} // namespace gits
"""
  arguments_cpp_include = """#include "vulkanTools.h"

"""
  arguments_cpp.write(arguments_cpp_include)
  arguments_h.write(arguments_h_include)

  enums_decls = ''
  enums_defs = ''
  for name, enums in sorted(enums_dict.items()):
    for enum in enums:
      enums_decls += '    typedef CVulkanEnum<' + name + '> C' + name + ';\n'
      enums_decls += '    template<>\n'
      enums_decls += '    class CVulkanEnumTypeTraits<' + name + '> {\n'
      enums_decls += '    public:\n'
      enums_decls += '      static const char* Name();\n'
      enums_decls += '      static std::string GetVariantName(' + name + ' variant);\n'
      enums_decls += '    };\n\n'

      enums_defs += 'const char* gits::Vulkan::CVulkanEnumTypeTraits<' + name + '>::Name() {\n'
      enums_defs += '  return "' + name + '";\n'
      enums_defs += '}\n'
      enums_defs += 'std::string gits::Vulkan::CVulkanEnumTypeTraits<' + name + '>::GetVariantName(' + name + ' variant) {\n'
      enums_defs += '  switch (variant) {\n'

      for variant in enum['vars']:
        if enum['size'] == 64:
          enums_defs += '  case ' + name + "::" + variant['name'] + ':\n'
        else:
          enums_defs += '  case ' + variant['name'] + ':\n'
        enums_defs += '    return "' + variant['name'] + '";\n'

      enums_defs += '  default:\n'
      enums_defs += '    Log(WARN) << "Unknown enum variant: " << variant << " of ' + name + '";\n'
      enums_defs += '    return "(' + name + ')" + std::to_string((int)variant);\n'
      enums_defs += '  }\n'
      enums_defs += '}\n\n'

  arguments_h.write(enums_decls)
  arguments_cpp.write(enums_defs)

  mapped_decls = '''
    // On 32-bit, all nondispatchable handle types are typedef'd to uint64_t.
    // This means compiler sees e.g. CVulkanObj<VkBuffer> as identical to
    // CVulkanObj<VkEvent>. On the other hand type tags below are seen by the
    // compiler as distinct types. We use them to instantiate the template for
    // each handle type.\n'''
  mapped_defs = ''
  for name in sorted(vulkan_mapped_types) + sorted(vulkan_mapped_types_nondisp):
    mapped_decls += '    typedef struct ' + name + '_T* ' + name + 'TypeTag;\n'
    mapped_decls += '    typedef CVulkanObj<' + name + ', ' + name + 'TypeTag> C' + name + ';\n\n'

    mapped_defs += 'template<>\n'
    mapped_defs += 'const char* gits::Vulkan::CVulkanObj<' + name + ', gits::Vulkan::' + name + 'TypeTag>::NAME = "' + name + '";\n\n'
  arguments_h.write(mapped_decls)
  arguments_cpp.write(mapped_defs)

  for key in sorted(structs.keys()):
    for elem in structs[key]:
      key_name = key.strip('_')
      versioned_name = add_version(key, elem.get('version'))
      if elem.get('custom') is not True:
        class_def = "    class C" + versioned_name + ';\n'
        arguments_h.write(class_def)
      if elem.get('declareArray'):
        arguments_h.write('    typedef CStructArray<' + key_name + ', C' + versioned_name + '> C' + versioned_name + 'Array;\n')
      if elem.get('declareArrayOfArrays'):
        arguments_h.write('    typedef CStructArrayOfArrays<' + key_name + ', C' + versioned_name + '> C' + versioned_name + 'ArrayOfArrays;\n')

  for key in sorted(structs.keys()):
    for elem in structs[key]:
      Ctypes = []
      Cnames = []
      Cwraps = []
      Csize = []
      types = []
      counter = 0
      key_name = key.strip('_')
      versioned_name = add_version(key, elem.get('version'))

      if elem.get('custom') is not True:
        for arg in elem['vars']:
          Cnames.append('_' + arg['name'])
          typename = re.sub(r'\:[0-9_]+', '', arg['type'])
          if arg.get('wrapType'):
            Ctypes.append(arg['wrapType'])
          elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types_nondisp and '*' in typename and 'const' not in typename:
            typename_mod = typename.replace('const ', '').replace('*', '').replace(' ', '')
            Ctypes.append('C'+typename_mod+'::CSArray')
          elif typename.replace('const ', '').strip(' *') in vulkan_mapped_types and '*' in typename and 'const' not in typename:
            typename_mod = typename.replace('const ', '').replace('*', '').replace(' ', '')
            Ctypes.append('C'+typename_mod+'::CSArray')
          elif typename.replace('const ', '').strip(' *') in vulkan_uint32:
            Ctypes.append('Cuint32_t')
          elif typename.replace('const ', '').strip(' *') in vulkan_uint64:
            Ctypes.append('Cuint64_t')
          elif '*' in typename and typename.replace('const ', '').strip(' *') == 'void' and arg['name'] == 'pNext':
            Ctypes.append('CpNextWrapper')
          elif re.search(r'\[([0-9]+)\]', typename.replace('const ', '')):
            Ctypes.append('C' + re.sub(r'\[([0-9_]+)\]', '', typename.replace('const ', '')) + '::CSArray')
          else:
            typename_mod = typename.replace('const ', '').replace(' *', '').replace(' ', '')
            Ctypes.append('C' + typename_mod)
          types.append(typename.replace('const ', '').replace(' *', '').replace(' ', ''))
          if arg.get('wrapParams'):
            Cwraps.append(arg['wrapParams'])
          else:
            Cwraps.append('')
          if re.search(r'\[([0-9]+)\]', typename.replace('const ', '')):
            size = re.findall(r'\[([0-9_]+)\]', typename)
            Csize.append(size[0])
          else:
            Csize.append('1')
          counter += 1
        func = elem
        key_variable = key_name.replace('Vk', '').lower()
        key_decl = '_' + key_name.replace('Vk', '')
        key_decl_original = key_decl + 'Original'
        argd = 'const ' + key_name + '* ' + key_variable
        if elem.get('constructorArgs') is not None:
          argd = elem.get('constructorArgs')
        argd_decl = 'std::unique_ptr<' + key_name + '> ' + key_decl
        argd_decl_original = 'std::unique_ptr<' + key_name + '> ' + key_decl_original

        argsDecl = ""
        for n, t in zip(Cnames, Ctypes):
          argsDecl += '      std::unique_ptr<' + t + '> ' + n + ';\n'

        argsCall = ""
        counter = 0

        function_read = ''
        function_write = ''
        function_operator = ''
        function_original = ''
        init = ''
        mapped_pointers = '\n  std::set<uint64_t> returnMap;\n  if (!*_isNullPtr) {\n  '
        if len(func['vars']) > 0:
          init = '  if (!*_isNullPtr) {\n'
          init_to_nullptr = '  } else {\n'
          init_default = ''
          function_read = '  if (!*_isNullPtr) {\n'
          function_write = '  if (!*_isNullPtr) {\n'
          function_operator = '  if (' + key_decl + ' == nullptr) {\n'
          function_original = '  if (' + key_decl_original + ' == nullptr) {\n'
        function_operator += '    ' + key_decl + ' = std::make_unique<' + key_name + '>();\n'
        function_original += '    ' + key_decl_original + ' = std::make_unique<' + key_name + '>();\n'
        for n, w, t, s, ot in zip(Cnames, Cwraps, Ctypes, Csize, types):
          if t != 'COutArgument':
            counter += 1

            if (ot not in primitive_types):
              mapped_pointers += '  for (auto obj : ' + n + '->GetMappedPointers())\n      returnMap.insert((uint64_t)obj);\n  '

            if w == '':
              struct_type = t[1:] + '_'
              if (struct_type) in structs.keys():
                init += '    ' + n + ' = std::make_unique<' + t + '>(&' + key_variable + '->' + n.strip('_') + ');\n'
              elif s != '1':
                init += '    ' + n + ' = std::make_unique<' + t + '>(' + s + ', ' + key_variable + '->' + n.strip('_') + ');\n'
              else:
                init += '    ' + n + ' = std::make_unique<' + t + '>(' + key_variable + '->' + n.strip('_') + ');\n'
            else:
              init += '    ' + n + ' = std::make_unique<' + t + '>(' + w + ');\n'

            init_to_nullptr += '    ' + n + ' = nullptr;\n'
            init_default += n + '(std::make_unique<' + t + '>()), '
            if s != '1':
              decayed_type = re.sub(r'\[([0-9_]+)\]', '*', ot) # Turn arrays to pointers.
              function_operator += '    ' + decayed_type + ' ' + n.strip('_') + 'Values = **' + n + ';\n'
              function_operator += '    if (' + n.strip('_') + 'Values != nullptr) {\n'
              function_operator += '      for (int i = 0; i < ' + s + '; i++)\n'
              function_operator += '        ' + key_decl + '->' + n.strip('_') + '[i] = ' + n.strip('_') + 'Values[i];\n'
              function_operator += '    } else {\n'
              function_operator += '      throw std::runtime_error(EXCEPTION_MESSAGE);\n'
              function_operator += '    }\n'
              function_original += '    ' + decayed_type + ' ' + n.strip('_') + 'ValuesOriginal = ' + n + '->Original();\n'
              function_original += '    if (' + n.strip('_') + 'ValuesOriginal != nullptr) {\n'
              function_original += '      for (int i = 0; i < ' + s + '; i++)\n'
              function_original += '        ' + key_decl_original + '->' + n.strip('_') + '[i] = ' + n.strip('_') + 'ValuesOriginal[i];\n'
              function_original += '    } else {\n'
              function_original += '      throw std::runtime_error(EXCEPTION_MESSAGE);\n'
              function_original += '    }\n'
            else:
              function_operator += '    ' + key_decl + '->' + n.strip('_') + ' = **' + n + ';\n'
              function_original += '    ' + key_decl_original + '->' + n.strip('_') + ' = ' + n + '->Original();\n'

            function_read += '    ' + n + '->Read(stream);\n'
            function_write += '    ' + n + '->Write(stream);\n'

        if len(func['vars']) > 0:
          init += init_to_nullptr
          init += '  }'
          function_read += '  }'
          function_write += '  }'
          function_operator += '  }'
          function_original += '  }'
        if counter > 0:
          function_read = function_read.strip('\n')
          function_write = function_write.strip('\n')
          init_default = init_default.strip(', ')
        else:
          init = ''
        function_operator += '\n  return ' + key_decl + '.get();'
        function_original += '\n  return PtrConverter<' + key_name + '>(' + key_decl_original + '.get());'
        mapped_pointers += '}\n  return returnMap;'
        for n in Cnames:
          if n not in ('_return_value', '_self'):
            argsCall += re.sub('[_]', '', n) + ': *' + n + ' '
        argsCall = argsCall.strip(' ')

        # CCode - Write, Declare and DeclarationNeeded methods
        type_name = add_version(key_name, elem.get('version'))
        camel_case_type_name = key_name[0].lower() + key_name[1:]
        # Temporary defines:
        if type_name not in types_not_needing_declaration:
          ampersand_declare = '\n      virtual bool AmpersandNeeded() const override;'
          ampersand_define = 'bool gits::Vulkan::C' + type_name + '::AmpersandNeeded() const {\n'
          ampersand_define += '  return !*_isNullPtr;\n'
          ampersand_define += '}\n'

          declarationImplementation = ' { return true; }';
          if elem.get('declarationNeededWrap') is True:
            declarationImplementation = '; // see vulkanArgumentsWrap.cpp for definition'
          decl_declare = '\n      virtual bool DeclarationNeeded() const override' + declarationImplementation
          decl_declare += '\n      virtual void Declare(CCodeOStream &stream) const override;'

          decl_define = 'void gits::Vulkan::C' + type_name + '::Declare(CCodeOStream &stream) const {\n'
          decl_define += '  if (!*_isNullPtr) {\n'

          pre_declarations = ''
          declarations = '\n    stream.Register(ScopeKey(), "' + camel_case_type_name + '", true);\n'
          declarations += '    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = {\\n";\n'
          declarations += '    stream.ScopeBegin();\n'
          post_declarations = '\n'
          for cname, ctype, csize in zip(Cnames, Ctypes, Csize):
            if ctype[0] == 'C':
              t = ctype[1:]
            else:
              t = ctype
            # Remove argument version suffix. #TODO: is it needed in Vk?
            t = re.sub(r'_V\d+$', '', t)

            if cname[0] == '_':
              n = cname[1:]
            else:
              n = cname

            if t in types_not_needing_declaration:
              declarations += '    stream.Indent() << *(this->' + cname + ') << ", // ' + n + '\\n";\n'
            else:
              if t in types_needing_name_registration or True:  # TODO: finetune it.
                pre_declarations += '    ' + cname + '->VariableNameRegister(stream, false);\n'

              pre_declarations += '    ' + cname + '->Declare(stream);\n'

              if csize != '1':
                count = ""
                for var in elem['vars']:
                  if ('_' + var['name']) == cname:
                    if 'count' in var.keys():
                      if csize != var['count']:
                        count = " && i < \" << stream.VariableName(ScopeKey()) << \"." + var['count']

                post_declarations += '    stream.Indent() << "for (int i = 0; i < ' + csize + count + '; ++i)\\n";\n'
                post_declarations += '    stream.ScopeBegin(); // For indentation.\n'
                post_declarations += '    stream.Indent() << stream.VariableName(ScopeKey()) << ".' + n + '[i] = " << *' + cname + ' << "[i];\\n";\n'
                post_declarations += '    stream.ScopeEnd();\n'
                declarations += '    stream.Indent() << "{}, // ' + n + '\\n";\n'
              else:
                declarations += '    stream.Indent() << *' + cname + ' << ", // ' + n + '\\n";\n'

          declarations += '    stream.ScopeEnd();\n'
          declarations += '    stream.Indent() << "};\\n";\n'

          decl_define += pre_declarations
          decl_define += declarations
          decl_define += post_declarations
          decl_define += '  }\n'
          decl_define += '}\n'

          function_ccode_write = '  if (*_isNullPtr) {\n'
          function_ccode_write += '    stream << "nullptr";\n'
          function_ccode_write += '  } else {\n'
          function_ccode_write += '    stream << stream.VariableName(ScopeKey());\n'
          #function_ccode_write += '    intptr_t varNameKey = reinterpret_cast<intptr_t>(this);\n'
          #function_ccode_write += '    std::string varName = stream.VariableName(varNameKey);\n'
          #function_ccode_write += '    stream << varName;\n'
          function_ccode_write += '  }'
        else:
          ampersand_declare = ''
          ampersand_define = ''
          decl_declare = ''
          decl_define = ''
          function_ccode_write = '  throw ENotImplemented(EXCEPTION_MESSAGE);'

        c = ""
        inherit_type = 'CArgument'
        if len(func['vars']) > 0:
          c = """
    class C%(versioned_name)s : public %(inherit_type)s, gits::noncopyable {
%(argsDecl)s
      %(argd_decl)s;
      %(argd_decl_original)s;
      Cbool _isNullPtr;

    public:
      C%(versioned_name)s();
      C%(versioned_name)s(%(argd)s);
      static const char* NAME;
      virtual const char* Name() const override { return NAME; }
      %(unversioned_name)s* Value();

      PtrConverter<%(unversioned_name)s> operator*() {
        return PtrConverter<%(unversioned_name)s>(Value());
      }
      PtrConverter<%(unversioned_name)s> Original();
      void * GetPtrType() override { return (void *)Value(); }
      virtual std::set<uint64_t> GetMappedPointers();
      virtual void Write(CBinOStream& stream) const override;
      virtual void Read(CBinIStream& stream) override;
      virtual void Write(CCodeOStream& stream) const override;%(ampersand_declare)s%(decl_declare)s
    };""" % {'versioned_name': versioned_name, 'unversioned_name': key_name, 'argd': argd, 'argsDecl': argsDecl, 'inherit_type': inherit_type, 'argd_decl': argd_decl, 'argd_decl_original': argd_decl_original, 'ampersand_declare': ampersand_declare, 'decl_declare': decl_declare, }

          constructor = """gits::Vulkan::C%(versioned_name)s::C%(versioned_name)s(%(argd)s): %(key_decl)s(nullptr), %(key_decl_original)s(nullptr), _isNullPtr(%(key_variable)s == nullptr) {
%(init)s
}""" % {'versioned_name': versioned_name, 'argd': argd, 'key_decl': key_decl, 'key_decl_original': key_decl_original, 'key_variable': key_variable, 'init': init}
          if elem.get('constructorWrap') is True:
            constructor = '// for constructor see vulkanArgumentsWrap.cpp'

          d = """
gits::Vulkan::C%(versioned_name)s::C%(versioned_name)s(): %(init_default)s, %(key_decl)s(nullptr), %(key_decl_original)s(nullptr), _isNullPtr(false) {
}

%(constructor)s

const char* gits::Vulkan::C%(versioned_name)s::NAME = "%(unversioned_name)s";

%(unversioned_name)s* gits::Vulkan::C%(versioned_name)s::Value() {
  if (*_isNullPtr)
    return nullptr;
%(function_operator)s
}

gits::PtrConverter<%(unversioned_name)s> gits::Vulkan::C%(versioned_name)s::Original() {
  if (*_isNullPtr)
    return PtrConverter<%(unversioned_name)s>(nullptr);
%(function_original)s
}

std::set<uint64_t> gits::Vulkan::C%(versioned_name)s::GetMappedPointers() {
%(mapped_pointers)s
}

void gits::Vulkan::C%(versioned_name)s::Write(CBinOStream& stream) const {
  _isNullPtr.Write(stream);
%(function_write)s
}

void gits::Vulkan::C%(versioned_name)s::Read(CBinIStream& stream) {
  _isNullPtr.Read(stream);
%(function_read)s
}

void gits::Vulkan::C%(versioned_name)s::Write(CCodeOStream& stream) const {
%(function_ccode_write)s
}

%(ampersand_define)s
%(decl_define)s
""" % {'versioned_name': versioned_name, 'unversioned_name': key_name, 'constructor': constructor, 'function_operator': function_operator, 'function_read': function_read, 'function_write': function_write, 'function_ccode_write': function_ccode_write, 'ampersand_define': ampersand_define, 'decl_define': decl_define, 'key_decl': key_decl, 'init_default': init_default, 'function_original': function_original, 'key_decl_original': key_decl_original, 'mapped_pointers': mapped_pointers}

        arguments_h.write(c + '\n')
        arguments_cpp.write(d+'\n')

  arguments_h.write(arguments_h_end)

  arguments_h.close()
  arguments_cpp.close()


def generate_vulkan_create_switch(functions):
  vulkan_function_cpp = open('vulkanIDswitch.h', 'w')
  vulkan_function_cpp.write(copyright_header)
  output = ""
  for key in sorted(functions.keys()):
    for elem in functions[key]:
      output += 'case ' + make_id(key, elem['version']) + ':\n'
      versioned_name = add_version(key, elem['version'])
      output += '  return new C' + versioned_name + ';\n'
  vulkan_function_cpp.write(output)
  vulkan_function_cpp.close()


def generate_vulkan_function_ids_header(functions):
  vulkan_functions_generated = open('vulkanIDs.h', 'r')
  functions_ids = []
  try:
    for line in vulkan_functions_generated:
      elem = line.strip(",\n")
      if 'ID' in elem:
        functions_ids.append(elem)
  finally:
    vulkan_functions_generated.close()

  vulkanfunction_h = open('vulkanIDs.h', 'a')
  generated_id = ""
  for key in sorted(functions.keys()):
    for func in functions[key]:
      if make_id(key, func['version']) not in functions_ids:
        generated_id += make_id(key, func['version']) + ',\n'

  vulkanfunction_h.write(generated_id)
  vulkanfunction_h.close()


def generate_vulkan_lua_enums(enums):
  vulkan_enums_lua_generated = open('vulkanLuaEnums.h', 'w')
  vulkan_enums_lua_generated.write(copyright_header)
  output = ""
  for key in sorted(enums.keys()):
    for elem in enums[key]:
      output += "template<> inline " + key + " lua_to(lua_State* L, int pos) { return static_cast<" + key + ">(lua_tointeger(L, pos)); }\n"
  vulkan_enums_lua_generated.write(output)
  vulkan_enums_lua_generated.close()


def generate_vulkan_ccode_arguments():
  arguments_h = open('vulkanCCodeArgumentsAuto.h', 'w')
  arguments_h.write(copyright_header)
  arguments_h_include = """#pragma once

namespace gits {
namespace Vulkan {
"""
  arguments_h_end = """
} // namespace Vulkan
} // namespace gits
"""
  arguments_h.write(arguments_h_include)

  mapped_decls = ''
  for name in sorted(vulkan_mapped_types) + sorted(vulkan_mapped_types_nondisp):
    mapped_decls += '    typedef CVulkanObj<' + name + '> C' + name + ';\n'
  arguments_h.write(mapped_decls)
  arguments_h.write(arguments_h_end)


enums = get_enums()
functions = get_functions()
structs = get_structs()

enums_table = {}  # TODO: It's a dict, not a table; rename it.
structs_table = {}
structs_enabled_table = {}
functions_all_table = {}
functions_enabled_table = {}

for e in enums:
  enum = {}
  enum['size'] = e.size

  enum['vars'] = []
  for enumerator in e.enumerators:
    var = {}
    var['name'] = enumerator.name
    var['value'] = enumerator.value
    enum['vars'].append(var)
  if enums_table.get(e.name) is None:
    enums_table[e.name] = []
  enums_table[e.name].append(enum)

for s in structs:
  struct = {}
  struct['enabled'] = s.enabled
  if s.name.strip('_') not in structs_names:
    structs_names.append(s.name.strip('_'))
  if (s.type):
    struct['type'] = s.type
  if (s.custom):
    struct['custom'] = s.custom
  if (s.constructor_wrap):
    struct['constructorWrap'] = s.constructor_wrap
  if (s.pass_struct_storage):
    struct['passStructStorage'] = s.pass_struct_storage
  if (s.declaration_needed_wrap):
    struct['declarationNeededWrap'] = s.declaration_needed_wrap
  if (s.constructor_arguments):
    struct['constructorArgs'] = s.constructor_arguments
  if (s.declare_array):
    struct['declareArray'] = s.declare_array
  if (s.declare_array_of_arrays):
    struct['declareArrayOfArrays'] = s.declare_array_of_arrays
  if (s.version):
    struct['version'] = s.version
  else:
    struct['version'] = 0
  struct['vars'] = []
  for field in s.fields:
    var = {}
    var['name'] = field.name
    var['type'] = field.type
    if(field.wrap_type is not None):
      var['wrapType'] = field.wrap_type
    if(field.wrap_params is not None):
      var['wrapParams'] = field.wrap_params
    if(field.count is not None):
      var['count'] = field.count
    if(field.log_condition is not None):
      var['logCondition'] = field.log_condition
    struct['vars'].append(var)
  if structs_table.get(s.name) is None:
    structs_table[s.name] = []
  structs_table[s.name].append(struct)
  if (structs_enabled_table.get(s.name) is None) and (struct['enabled'] is True):
    structs_enabled_table[s.name] = []
  if struct['enabled'] is True:
    structs_enabled_table[s.name].append(struct)

for f in functions:
  function = {}
  function['args'] = []
  function['type'] = ''
  function['level'] = 'DeviceLevel'
  function['customDriver'] = False
  function['functionType'] = None
  function['enabled'] = f.enabled
  function['version'] = 0
  if retval := f.return_value:
    function['type'] = retval.type
    if retval.wrap_type:
      function['retVwrapType'] = retval.wrap_type
    if retval.wrap_params:
      function['retVwrapParams'] = retval.wrap_params
  if f.function_type:
    function['functionType'] = f.function_type
  if f.version:
    function['version'] = f.version
  if f.token_cache:
    function['tokenCache'] = f.token_cache
  i = 1

  if len(f.args) != len(function['args']) and len(function['args']) != 0:
    raise RuntimeError(f"Argcount mismatch: input has {len(f.args)}, while temp data dict has {len(function['args'])}.")
  for other_arg in f.args:
    if other_arg:
      arg = {}
      arg['type'] = other_arg.type
      arg['name'] = other_arg.name
      if other_arg.wrap_type:
        arg['wrapType'] = other_arg.wrap_type
      if other_arg.wrap_params:
        arg['wrapParams'] = other_arg.wrap_params
      if other_arg.count:
        arg['count'] = other_arg.count
      if other_arg.remove_mapping:
        arg['removeMapping'] = other_arg.remove_mapping
      if i <= len(function['args']):
        function['args'][i-1] = arg
      else:
        function['args'].append(arg)
    i += 1
  if f.pre_token is not None:
    function['preToken'] = f.pre_token
    function['preTokenName'] = f.name
  if f.post_token is not None:
    function['postToken'] = f.post_token
    function['postTokenName'] = f.name
  if f.state_track is not None:
    function['stateTrack'] = f.state_track
    function['stateTrackName'] = f.name
  if f.recorder_wrap is not None:
    function['recWrap'] = f.recorder_wrap
    function['recWrapName'] = f.name
  if f.run_wrap is not None:
    function['runWrap'] = f.run_wrap
    function['runWrapName'] = f.name
  if f.ccode_wrap is not None:
    function['ccodeWrap'] = f.ccode_wrap
    function['ccodeWrapName'] = f.name
  if f.ccode_write_wrap is not None:
    function['ccodeWriteWrap'] = f.ccode_write_wrap
    function['ccodeWriteWrapName'] = f.name
  if f.ccode_post_action_needed is not None:
    function['ccodePostActionNeeded'] = f.ccode_post_action_needed
  if f.recorder_exec_wrap is not None:
    function['recExecWrap'] = f.recorder_exec_wrap
    function['recExecWrapName'] = f.name
  if f.plugin_wrap is not None:
    function['pluginWrap'] = f.plugin_wrap
    function['pluginWrapName'] = f.name
  if f.exec_post_recorder_wrap is not None:
    function['execPostRecWrap'] = f.exec_post_recorder_wrap
  if f.end_frame_tag is not None:
    function['endFrameTag'] = f.end_frame_tag
  if f.level is not None:
    function['level'] = f.level
  if f.custom_driver is not None:
    function['customDriver'] = f.custom_driver
  if functions_all_table.get(f.name) is None:
    functions_all_table[f.name] = []
  if (functions_enabled_table.get(f.name) is None) and (function['enabled'] is True):
    functions_enabled_table[f.name] = []
  functions_all_table[f.name].append(function)
  functions_all_table[f.name].sort(key=operator.itemgetter('version'))
  if function['enabled'] is True:
    functions_enabled_table[f.name].append(function)
    functions_enabled_table[f.name].sort(key=operator.itemgetter('version'))

generate_vulkan_header(enums_table, structs_table, functions_all_table)
generate_prepost(functions_all_table)
generate_vulkan_def(functions_all_table, 'vkPlugin.def', 'vulkan-1.dll')
generate_vulkan_def(functions_all_table, 'vkLayer.def', 'VkLayer_vulkan_GITS_recorder.dll')
generate_layer_json()
generate_vulkan_drivers(functions_all_table)
generate_vulkan_log(structs_table, enums_table)
generate_vulkan_tracer(functions_all_table, enums_table)
generate_vulkan_tokens(functions_enabled_table)
generate_vulkan_recorder_wrapper(functions_all_table)
generate_vulkan_arguments(structs_enabled_table, enums_table)
generate_vulkan_function_ids_header(functions_enabled_table)
generate_vulkan_create_switch(functions_enabled_table)
generate_vulkan_struct_storage(structs_enabled_table, enums_table)
generate_vulkan_lua_enums(enums_table)
generate_vulkan_ccode_arguments()


def move_file(filename, subdir):
  path = os.path.join('../' + subdir, filename)
  print('Moving {} to {}...'.format(filename, path))
  shutil.move(filename, path)


def copy_file(filename, subdir):
  path = os.path.join('../' + subdir, filename)
  print('Copying {} to {}...'.format(filename, path))
  shutil.copy2(filename, path)


copy_file('vulkanIDs.h', 'common/include')
move_file('vulkanPrePostAuto.cpp', 'interceptor')
move_file('vulkanLogAuto.cpp', 'common')
move_file('vulkanLogAuto.inl', 'common/include')
move_file('vulkanTracerAuto.h', 'common/include')
move_file('vulkanTracerAuto.cpp', 'common')
move_file('vulkanHeader.h', 'common/include')
move_file('vulkanDriversAuto.inl', 'common/include')
move_file('vkPlugin.def', 'interceptor')
move_file('vkLayer.def', 'layer')
move_file('VkLayer_vulkan_GITS_recorder.json', 'layer')
move_file('vulkanArgumentsAuto.cpp', 'common')
move_file('vulkanArgumentsAuto.h', 'common/include')
move_file('vulkanStructStorageAuto.h', 'common/include')
move_file('vulkanStructStorageAuto.cpp', 'common')
move_file('vulkanFunctions.cpp', 'common')
move_file('vulkanFunctions.h', 'common/include')
move_file('vulkanIDswitch.h', 'common/include')
move_file('vulkanRecorderWrapperAuto.cpp', 'recorder')
move_file('vulkanRecorderWrapperAuto.h', 'recorder/include')
move_file('vulkanRecorderWrapperIfaceAuto.h', 'recorder/include')
move_file('vulkanLuaEnums.h', 'common/include')
move_file('vulkanCCodeArgumentsAuto.h', '../CCodeFiles/src/include')
