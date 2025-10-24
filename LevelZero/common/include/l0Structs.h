// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0Header.h"
#include "l0Arguments.h"
#include "l0ArgumentsAuto.h"

#ifdef WITH_OCLOC
#include "tools.h"
#include "tools_lite.h"
#include "oclocFunctions.h"
#include "oclocStateDynamic.h"
#endif

namespace gits {
namespace l0 {
class Cze_module_constants_t : public CArgument {
private:
  typedef ze_module_constants_t L0Type;
  L0Type _struct = {};
  Cuint32_t _numConstants;
  Cuint32_t::CSArray _constantIds;
  CvoidPtr::CSArray _constantValues;

public:
  typedef CL0StructArray<L0Type, Cze_module_constants_t> CSArray;
  static const char* NAME;
  Cze_module_constants_t() = default;
  Cze_module_constants_t(const L0Type& value);
  Cze_module_constants_t(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  L0Type operator*();
  L0Type* Ptr();
  virtual std::set<uint64_t> GetMappedPointers();
};

class Cze_module_constants_t_V1 : public CArgument {
private:
  typedef ze_module_constants_t L0Type;
  L0Type _struct = {};
  Cuint8_t _version;
  Cuint32_t _numConstants;
  Cuint32_t::CSArray _constantIds;
  Cuint64_t::CSArray _constantValues;
  std::vector<std::unique_ptr<uintptr_t[]>> pPointers;

public:
  typedef CL0StructArray<L0Type, Cze_module_constants_t_V1> CSArray;
  static const char* NAME;
  Cze_module_constants_t_V1() = default;
  Cze_module_constants_t_V1(const L0Type& value);
  Cze_module_constants_t_V1(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

  L0Type operator*();
  L0Type* Ptr();
  virtual std::set<uint64_t> GetMappedPointers();
};

class Cze_module_constants_ptr : public CArgument {
private:
  typedef const ze_module_constants_t* L0Type;
  L0Type _pointer = nullptr;

  Cze_module_constants_t_V1::CSArray _pConstants;

public:
  typedef CL0StructArray<L0Type, Cze_module_constants_ptr> CSArray;
  static const char* NAME;
  Cze_module_constants_ptr() = default;
  Cze_module_constants_ptr(const L0Type& value);
  Cze_module_constants_ptr(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

  L0Type operator*();
  L0Type* Ptr();
  virtual std::set<uint64_t> GetMappedPointers();
};

class Cze_module_desc_t : public CArgument {
private:
  typedef ze_module_desc_t L0Type;
  L0Type _struct = {};
  Cze_structure_type_t _stype;
  CExtensionStructCore _pNext;
  Cze_module_format_t _format;
  Csize_t _inputSize;
  Cuint8_t::CSArray _pInputModule;
  Cchar::CSArray _pBuildFlags;
  Cze_module_constants_t::CSArray _pConstants;
  std::string _filename;

public:
  typedef CL0StructArray<L0Type, Cze_module_desc_t> CSArray;
  static const char* NAME;
  Cze_module_desc_t() = default;
  Cze_module_desc_t(const L0Type& value);
  Cze_module_desc_t(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  L0Type operator*();
  L0Type* Ptr();
  virtual std::set<uint64_t> GetMappedPointers();
};

class Cze_module_desc_t_V1 : public CArgument {
private:
  typedef ze_module_desc_t L0Type;
  L0Type _struct = {};
  Cze_structure_type_t _stype;
  CExtensionStructCore _pNext;
  Cze_module_format_t _format;
  Csize_t _inputSize;
  CProgramSource _pInputModule;
  Cchar::CSArray _pBuildFlags;
  Cze_module_constants_t_V1::CSArray _pConstants;

public:
  typedef CL0StructArray<L0Type, Cze_module_desc_t_V1> CSArray;
  static const char* NAME;
  Cze_module_desc_t_V1() = default;
  Cze_module_desc_t_V1(const L0Type& value);
  Cze_module_desc_t_V1(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  L0Type operator*();
  L0Type* Ptr();
  virtual std::set<uint64_t> GetMappedPointers();
  virtual std::vector<std::string> GetProgramSourcesNames();
};

class Cze_module_program_exp_desc_t : public CArgument {
private:
  typedef ze_module_program_exp_desc_t L0Type;
  L0Type _struct = {};
  Cze_structure_type_t _stype;
  CExtensionStructCore _pNext;
  Cuint32_t _count;
  Csize_t::CSArray _inputSizes;
  CProgramSource::CSArray _pInputModules;
  CStringArray _pBuildFlags;
  Cze_module_constants_ptr::CSArray _pConstants;

  std::vector<size_t> newInputSizes;
  std::vector<uint8_t*> newInputModules;

public:
  typedef CL0StructArray<L0Type, Cze_module_program_exp_desc_t> CSArray;
  static const char* NAME;
  Cze_module_program_exp_desc_t() = default;
  Cze_module_program_exp_desc_t(const L0Type& value);
  Cze_module_program_exp_desc_t(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  L0Type operator*();
  L0Type* Ptr();
  virtual std::set<uint64_t> GetMappedPointers();
  virtual std::vector<std::string> GetProgramSourcesNames();
  void* GetPtrType() override;
};
} // namespace l0
} // namespace gits
