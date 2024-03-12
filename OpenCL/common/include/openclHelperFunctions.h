// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "oclFunction.h"
#include "openclArgumentsAuto.h"
#include "openclStateDynamic.h"
#include "openclArguments.h"

namespace gits {
namespace OpenCL {
class CGitsClMemoryUpdate : public CFunction {
  static constexpr unsigned ARG_NUM = 2U;

  void* _ptr = nullptr;
  CBinaryResource _resource;

  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) override;

public:
  CGitsClMemoryUpdate() = default;
  CGitsClMemoryUpdate(void* ptr);

  virtual unsigned Id() const override {
    return ID_GITS_CL_MEMORY_UPDATE;
  }
  virtual const char* Name() const override {
    return "CGitsClMemoryUpdate";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write(CCodeOStream& stream) const override;
  virtual void Run();
};

class CGitsClMemoryRestore : public CFunction {
  static constexpr unsigned ARG_NUM = 3U;

  void* _ptr = nullptr;
  uint64_t _length = 0;
  CBinaryResource _resource;

  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) override;

public:
  CGitsClMemoryRestore() {}
  CGitsClMemoryRestore(void* ptr);
  CGitsClMemoryRestore(void* ptr, const size_t& size);

  virtual unsigned Id() const {
    return ID_GITS_CL_MEMORY_RESTORE;
  }
  virtual const char* Name() const {
    return "CGitsClMemoryRestore";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Run();
};

class CGitsClMemoryRegionRestore : public CFunction {
  static constexpr unsigned ARG_NUM = 4U;
  void* _ptr = nullptr;
  uint64_t _length = 0;
  uint64_t _offset = 0;
  CBinaryResource _resource;

  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) override;

public:
  CGitsClMemoryRegionRestore() {}
  CGitsClMemoryRegionRestore(void* ptr, const uint64_t& offset, const uint64_t& size);
  virtual unsigned Id() const {
    return ID_GITS_CL_MEMORY_REGION_RESTORE;
  }
  virtual const char* Name() const {
    return "CGitsClMemoryRegionRestore";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Run();
};

class CGitsClTokenMakeCurrentThread : public CFunction {
  static constexpr uint32_t _argNum = 1U;

protected:
  int _threadId = 0;

public:
  CGitsClTokenMakeCurrentThread() = default;
  explicit CGitsClTokenMakeCurrentThread(const int& threadid);
  virtual uint32_t ArgumentCount() const override {
    return _argNum;
  }
  virtual CArgument& Argument(uint32_t idx) override;
  virtual uint32_t Id() const {
    return ID_GITS_CL_MAKE_CURRENT_THREAD;
  }
  virtual const char* Name() const {
    return "CGitsClTokenMakeCurrentThread";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& /*stream*/) const {}
  virtual void Run();
};
} // namespace OpenCL
} // namespace gits
