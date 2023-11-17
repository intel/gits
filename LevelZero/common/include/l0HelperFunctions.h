// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0Functions.h"
#include "l0ArgumentsAuto.h"
#include "l0StateDynamic.h"
#include "l0Arguments.h"

namespace gits {
namespace l0 {
class CGitsL0MemoryUpdate : public CFunction {
  static constexpr unsigned ARG_NUM = 2U;

  void* _usmPtr = nullptr;
  CBinaryResource _resource;

  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) override;

public:
  CGitsL0MemoryUpdate() {}
  CGitsL0MemoryUpdate(const void* usmPtr);

  virtual unsigned Id() const override {
    return ID_GITS_L0_MEMORY_UPDATE;
  }
  virtual const char* Name() const override {
    return "CGitsL0MemoryUpdate";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual void Run();
};

class CGitsL0MemoryRestore : public CFunction {
  static constexpr unsigned ARG_NUM = 2U;
  void* _usmPtr = nullptr;
  CBinaryResource _resource;
  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) override;

public:
  CGitsL0MemoryRestore() {}
  CGitsL0MemoryRestore(const void* usmPtr, const size_t& size);
  CGitsL0MemoryRestore(const void* usmPtr, const void* resourcePtr, const size_t& size);
  CGitsL0MemoryRestore(const void* globalPointer, const std::vector<char>& globalPtrAllocation);
  virtual unsigned Id() const override {
    return ID_GITS_L0_MEMORY_RESTORE;
  }
  virtual const char* Name() const override {
    return "CGitsL0MemoryRestore";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual void Run();
};

class CGitsL0TokenMakeCurrentThread : public CFunction {
  static constexpr uint32_t _argNum = 1U;

protected:
  int _threadId = 0;

public:
  CGitsL0TokenMakeCurrentThread() = default;
  explicit CGitsL0TokenMakeCurrentThread(const int& threadid);
  virtual uint32_t ArgumentCount() const override {
    return _argNum;
  }
  virtual CArgument& Argument(uint32_t idx) override;
  virtual uint32_t Id() const {
    return ID_GITS_L0_MAKE_CURRENT_THREAD;
  }
  virtual const char* Name() const {
    return "CGitsL0TokenMakeCurrentThread";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write([[maybe_unused]] CCodeOStream& stream) const {}
  virtual void Run();
};

class CGitsL0OriginalQueueFamilyInfo : public CFunction {
private:
  static constexpr uint32_t _argNum = 3U;
  Cze_device_handle_t _hDevice;
  Cze_structure_type_t _stype;
  Cze_command_queue_group_properties_t::CSArray _cqGroupProperties;

public:
  CGitsL0OriginalQueueFamilyInfo() = default;
  CGitsL0OriginalQueueFamilyInfo(ze_device_handle_t hDevice,
                                 const std::vector<ze_command_queue_group_properties_t>& props);
  virtual uint32_t ArgumentCount() const override {
    return _argNum;
  }
  virtual CArgument& Argument(uint32_t idx) override;
  virtual uint32_t Id() const {
    return ID_GITS_ORIGINAL_QUEUE_FAMILY_INFO;
  }
  virtual const char* Name() const {
    return "CGitsL0OriginalQueueFamilyInfo";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write([[maybe_unused]] CCodeOStream& stream) const {}
  virtual void Run();
};

} // namespace l0
} // namespace gits