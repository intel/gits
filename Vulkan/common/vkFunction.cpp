// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vkFunction.h"
#include "vulkanFunctions.h"
#include "vulkanPreToken.h"

namespace gits {
namespace Vulkan {
/**
    * @brief Creates Vulkan function call warapper
    *
    * Method creates Vulkan function call wrappers based on unique
    * identifier.
    *
    * @param id Unique Vulkan function identifer.
    *
    * @exception EOperationFailed Unknown Vulkan function identifier
    *
    * @return Vulkan function call wrapper.
    */
CFunction* CFunction::Create(unsigned id) {
  if (id < CFunction::ID_VULKAN || id >= CFunction::ID_FUNCTION_END) {
    return nullptr;
  }
  switch (id) {
  case ID_GITS_VK_WINDOW_CREATOR:
    return new CGitsVkCreateNativeWindow;
  case ID_GITS_VK_MEMORY_UPDATE:
    return new CGitsVkMemoryUpdate;
  case ID_GITS_VK_MEMORY_RESTORE:
    return new CGitsVkMemoryRestore;
  case ID_GITS_VK_WINDOW_UPDATE:
    return new CGitsVkUpdateNativeWindow;
  case ID_GITS_VK_MEMORY_UPDATE2:
    return new CGitsVkMemoryUpdate2;
  case ID_VK_DESTROY_VULKAN_DESCRIPTOR_SETS:
    return new CDestroyVulkanDescriptorSets;
  case ID_VK_DESTROY_VULKAN_COMMAND_BUFFERS:
    return new CDestroyVulkanCommandBuffers;
  case ID_GITS_VK_MEMORY_RESET:
    return new CGitsVkMemoryReset;
  case ID_GITS_VK_ENUMERATE_DISPLAY_MONITORS:
    return new CGitsVkEnumerateDisplayMonitors;
  case ID_GITS_VK_CMD_INITIALIZE_IMAGE_INTEL:
    return new CGitsInitializeImage;
  case ID_GITS_VK_CMD_INITIALIZE_BUFFER_INTEL:
    return new CGitsInitializeBuffer;
  case ID_GITS_VK_STATE_RESTORE_INFO:
    return new CGitsVkStateRestoreInfo;
  case ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS:
    return new CGitsVkCmdInsertMemoryBarriers;
  case ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_IMAGES_INTEL:
    return new CGitsInitializeMultipleImages;
  case ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_BUFFERS_INTEL:
    return new CGitsInitializeMultipleBuffers;
  case ID_GITS_VK_XLIB_WINDOW_CREATOR:
    return new CGitsVkCreateXlibWindow;
  case ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS_2:
    return new CGitsVkCmdInsertMemoryBarriers2;
  case ID_GITS_VK_CMD_PATCH_DEVICE_ADDRESSES:
    return new CGitsVkCmdPatchDeviceAddresses;
#include "vulkanIDswitch.h"
  default:;
  }
  Log(ERR) << "Unknown Vulkan function with ID: " << id;
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

CArgument& CFunction::Result(unsigned idx) {
  Log(ERR) << "Results not supported in Vulkan!!!";
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

VkCommandBuffer CFunction::CommandBuffer() {
  return VK_NULL_HANDLE;
}

/**
     * @brief Saves function data to a C code file
     *
     * Method saves function data to a C code file.
     *
     * @param stream Output stream to use.
     */
void CFunction::Write(CCodeOStream& stream) const {
  // Captured function calls go to state restore file if is present or to frames file.
  stream.select(stream.selectCCodeFile());

  auto returnVal = Return();

  // Count local variables, register global ones.
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

  // Print arguments' declarations.
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
  }

  // Print the function call.
  stream.Indent();
  if (returnVal) {
    stream << returnVal->Name() << " " << stream.VariableName(returnVal->ScopeKey()) << " = ";
  }
  stream << Name() << Suffix() << "(";
  for (unsigned idx = 0; idx < ArgumentCount(); idx++) {
    const CArgument& arg = Argument(idx);
    if (ArgumentInfo(idx).typeMayNeedAmpersand && arg.AmpersandNeeded()) {
      // E.g. if function expects int* but the argument is int then add '&'.
      stream << '&';
    }
    stream << arg;
    if (idx < ArgumentCount() - 1) {
      stream << ", ";
    }
  }
  stream << ");" << std::endl;

  // Print post-apicall stuff.
  if (CCodePostActionNeeded()) {
    for (unsigned i = 0; i < ArgumentCount(); i++) {
      const CArgument& arg = Argument(i);
      if (arg.PostActionNeeded()) {
        if (returnVal) {
          stream.Indent() << "if (ret == VK_SUCCESS) {\n";
          stream.ScopeBegin();
        }
        arg.PostAction(stream);
        if (returnVal) {
          stream.ScopeEnd();
          stream.Indent() << "}\n";
        }
      }
    }
  }

  WritePostCall(stream);

  if (varNum || returnVal) {
    stream.ScopeEnd();
    stream.Indent() << "}" << std::endl;
  }
}

CQueueSubmitFunction::CQueueSubmitFunction() {}

void CQueueSubmitFunction::Trace() {
  Log(TRACE, NO_NEWLINE) << "QueueSubmit nr: "
                         << CGits::Instance().vkCounters.CurrentQueueSubmitCount() << " ";
}

void CQueueSubmitFunction::CountUp() {
  CGits::Instance().vkCounters.QueueSubmitCountUp();
}

void CQueueSubmitFunction::Run() {
  CountUp();
  this->Trace();
  this->RunImpl();
}

CImageFunction::CImageFunction() {}

void CImageFunction::Trace() {
  Log(TRACE, NO_NEWLINE) << "Image nr: " << CGits::Instance().vkCounters.CurrentImageCount() << " ";
}

void CImageFunction::CountUp() {
  CGits::Instance().vkCounters.ImageCountUp();
}

void CImageFunction::Run() {
  CountUp();
  this->Trace();
  this->RunImpl();
}

CBufferFunction::CBufferFunction() {}

void CBufferFunction::Trace() {
  Log(TRACE, NO_NEWLINE) << "Buffer nr: " << CGits::Instance().vkCounters.CurrentBufferCount()
                         << " ";
}

void CBufferFunction::CountUp() {
  CGits::Instance().vkCounters.BufferCountUp();
}

void CBufferFunction::Run() {
  CountUp();
  this->Trace();
  this->RunImpl();
}
} // namespace Vulkan
} // namespace gits
