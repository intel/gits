// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "function.h"
#include "platform.h"

#define GITS_BIT_AT_IDX(x) (1u << x)

namespace gits {
namespace Vulkan {
enum class ArgType {
  PRIMITIVE_TYPE,
  ENUM,
  STRUCT,
  OPAQUE_HANDLE,
  // Add variants above.
  OTHER,
};
struct ArgInfo {
  ArgType type;
  uint32_t numPtr;           // 0 for T, 1 for T*, 2 for T**, etc.
  bool typeMayNeedAmpersand; // Whether it possibly needs to be prefixed
                             // with '&' in CCode. Note: this depends on
                             // the value of the argument. This field only
                             // indicates a possibility.
};

/**
    * @brief Vulkan library specific function wrapper
    *
    * gits::Vulkan::CFunction is an Vulkan library specific function
    * call wrapper.
    */
class CFunction : public gits::CFunction {
  virtual ArgInfo ArgumentInfo(unsigned idx) const = 0;

public:
  enum TApiType {
    GITS_VULKAN_PARAM_APITYPE = GITS_BIT_AT_IDX(0),
    GITS_VULKAN_QUEUE_SUBMIT_APITYPE = GITS_BIT_AT_IDX(1),
    GITS_VULKAN_CREATE_IMAGE_APITYPE = GITS_BIT_AT_IDX(2),
    GITS_VULKAN_CREATE_BUFFER_APITYPE = GITS_BIT_AT_IDX(3),
    GITS_VULKAN_CMDBUFFER_SET_APITYPE = GITS_BIT_AT_IDX(4),
    GITS_VULKAN_CMDBUFFER_BIND_APITYPE = GITS_BIT_AT_IDX(5),
    GITS_VULKAN_CMDBUFFER_PUSH_APITYPE = GITS_BIT_AT_IDX(6),
    GITS_VULKAN_BEGIN_RENDERPASS_APITYPE = GITS_BIT_AT_IDX(7),
    GITS_VULKAN_END_RENDERPASS_APITYPE = GITS_BIT_AT_IDX(8)
  };

  enum TId {
    BEGIN_VULKAN = CToken::ID_VULKAN,
    ID_GITS_VK_WINDOW_CREATOR,
    ID_GITS_VK_MEMORY_UPDATE,
    ID_GITS_VK_MEMORY_RESTORE,
    ID_GITS_VK_WINDOW_UPDATE,
    ID_GITS_VK_MEMORY_UPDATE2,
    ID_VK_DESTROY_VULKAN_DESCRIPTOR_SETS,
    ID_VK_DESTROY_VULKAN_COMMAND_BUFFERS,
    ID_GITS_VK_MEMORY_RESET,
    ID_GITS_VK_ENUMERATE_DISPLAY_MONITORS,
    ID_GITS_VK_CMD_INITIALIZE_IMAGE_INTEL,
    ID_GITS_VK_CMD_INITIALIZE_BUFFER_INTEL,
    ID_GITS_VK_STATE_RESTORE_INFO,
    ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS,
    ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_IMAGES_INTEL,
    ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_BUFFERS_INTEL,
    ID_GITS_VK_XLIB_WINDOW_CREATOR,
    ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS_2,
    BEGIN_AUTOGENERATED_IDs = BEGIN_VULKAN + 1000,
#include "vulkanIDs.h"
    ID_FUNCTION_END
  };
  static CFunction* Create(unsigned id);
  virtual MaybeConstCArgRef Return() const override {
    return MaybeConstCArgRef();
  }
  virtual unsigned ResultCount() const override {
    return 0;
  }
  virtual CArgument& Result(unsigned idx) override;

  virtual CLibrary::TId LibraryId() const override {
    return CLibrary::ID_VULKAN;
  }

  virtual void Write(CCodeOStream& stream) const override;
  virtual std::set<uint64_t> GetMappedPointers() = 0;
};
class CQueueSubmitFunction : public CFunction {
public:
  CQueueSubmitFunction();
  // Common 'Run' for all drawcalls. Do not override.
  virtual void Run() final;
  virtual void RunImpl() = 0;
  void CountUp();
  virtual void Trace();
};
class CImageFunction : public CFunction {
public:
  CImageFunction();
  // Common 'Run' for all drawcalls. Do not override.
  virtual void Run() final;
  virtual void RunImpl() = 0;
  void CountUp();
  virtual void Trace();
};
class CBufferFunction : public CFunction {
public:
  CBufferFunction();
  // Common 'Run' for all drawcalls. Do not override.
  virtual void Run() final;
  virtual void RunImpl() = 0;
  void CountUp();
  virtual void Trace();
};
} // namespace Vulkan
} // namespace gits
