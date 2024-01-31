// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
  virtual void Declare(CCodeOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
  L0Type operator*();
  L0Type* Ptr();
  virtual bool DeclarationNeeded() const;
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
  std::vector<std::shared_ptr<uintptr_t>> pointers;

public:
  typedef CL0StructArray<L0Type, Cze_module_constants_t_V1> CSArray;
  static const char* NAME;
  Cze_module_constants_t_V1() = default;
  Cze_module_constants_t_V1(const L0Type& value);
  Cze_module_constants_t_V1(const L0Type* value);
  virtual const char* Name() const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

  virtual void Write(CCodeOStream& stream) const;
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
  virtual void Declare(CCodeOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
  L0Type operator*();
  L0Type* Ptr();
  virtual bool DeclarationNeeded() const;
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
  virtual void Declare(CCodeOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
  L0Type operator*();
  L0Type* Ptr();
  virtual bool DeclarationNeeded() const;
  virtual std::set<uint64_t> GetMappedPointers();
  virtual std::string GetProgramSourceName() const;
};

} // namespace l0
} // namespace gits
