// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
#include "oclocFunctions.h"
#include "oclocStateDynamic.h"
#endif

namespace gits {
namespace l0 {

namespace {
std::vector<uint8_t>* FindOclocInputModule(ocloc::CStateDynamic& sd, const std::string& filename) {
  auto bufIt = sd.player.find(filename);
  if (bufIt != sd.player.end()) {
    return &bufIt->second;
  }
  const std::string backwardCompatibleSuffix = ".bin";
  const auto suffixLength = backwardCompatibleSuffix.length();
  const auto lengthDifference = filename.length() - suffixLength;
  if (filename.length() >= suffixLength &&
      filename.substr(lengthDifference, suffixLength) == backwardCompatibleSuffix) {
    bufIt = sd.player.find(filename.substr(0, lengthDifference));
    if (bufIt != sd.player.end()) {
      return &bufIt->second;
    }
  }
  return nullptr;
}

} // namespace
class Cze_module_constants_t : public CArgument {
private:
  typedef ze_module_constants_t L0Type;
  L0Type _struct;
  Cuint32_t _numConstants;
  Cuint32_t::CSArray _constantIds;
  CvoidPtr::CSArray _constantValues;

public:
  typedef CL0StructArray<L0Type, Cze_module_constants_t> CSArray;
  static const char* NAME;
  Cze_module_constants_t() {}
  Cze_module_constants_t(const L0Type& value)
      : _numConstants(value.numConstants),
        _constantIds(value.numConstants, value.pConstantIds),
        _constantValues(value.numConstants, const_cast<void**>(value.pConstantValues)) {}
  Cze_module_constants_t(const L0Type* value) : Cze_module_constants_t(*value) {}
  virtual const char* Name() const {
    return "ze_module_constants_t";
  }
  virtual void Write(CBinOStream& stream) const {
    _numConstants.Write(stream);
    _constantIds.Write(stream);
    _constantValues.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    _numConstants.Read(stream);
    _constantIds.Read(stream);
    _constantValues.Read(stream);
  }
  virtual void Declare(CCodeOStream& stream) const {
    VariableNameRegister(stream, false);
    if (_numConstants.DeclarationNeeded()) {
      _numConstants.VariableNameRegister(stream, false);
      _numConstants.Declare(stream);
    }
    if (_constantIds.DeclarationNeeded()) {
      _constantIds.VariableNameRegister(stream, false);
      _constantIds.Declare(stream);
    }
    if (_constantValues.DeclarationNeeded()) {
      _constantValues.VariableNameRegister(stream, false);
      _constantValues.Declare(stream);
    }
    std::string varName = stream.VariableName(ScopeKey());
    stream.Indent() << Name() << " " << varName << ";\n";
    stream.Indent() << varName << ".numConstants = ";
    if (_numConstants.DeclarationNeeded()) {
      stream << stream.VariableName(_numConstants.ScopeKey());
    } else {
      _numConstants.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".pConstantIds = ";
    if (_constantIds.DeclarationNeeded()) {
      stream << stream.VariableName(_constantIds.ScopeKey());
    } else {
      _constantIds.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".pConstantValues = ";
    if (_constantValues.DeclarationNeeded()) {
      stream << stream.VariableName(_constantValues.ScopeKey());
    } else {
      _constantValues.Write(stream);
    }
    stream << ";\n";
  }
  virtual void Write(CCodeOStream& /*stream*/) const {}
  L0Type operator*() {
    const auto* ptr = Ptr();
    if (ptr == nullptr) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    return *ptr;
  }
  L0Type* Ptr() {
    _struct.numConstants = *_numConstants;
    _struct.pConstantIds = *_constantIds;
    _struct.pConstantValues = (const void**)*_constantValues;
    return &_struct;
  }
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

class Cze_module_desc_t : public CArgument {
private:
  typedef ze_module_desc_t L0Type;
  L0Type _struct;
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
  Cze_module_desc_t() {}
  Cze_module_desc_t(const L0Type& value)
      : _stype(value.stype),
        _pNext(value.pNext),
        _format(value.format),
        _inputSize(value.inputSize),
        _pInputModule(value.inputSize, value.pInputModule),
        _pBuildFlags(value.pBuildFlags, 0, 1),
        _pConstants(0, value.pConstants) {
#ifdef WITH_OCLOC
    uint64_t hash = ComputeHash(value.pInputModule, value.inputSize, THashType::XX);
    if (ocloc::SD().recorder.find(hash) != ocloc::SD().recorder.end()) {
      _filename = ocloc::SD().recorder.at(hash);
    }
#endif
  }
  Cze_module_desc_t(const L0Type* value) : Cze_module_desc_t(*value) {}
  virtual const char* Name() const {
    return "ze_module_desc_t";
  }
  virtual void Write(CBinOStream& stream) const {
    _stype.Write(stream);
    _pNext.Write(stream);
    _format.Write(stream);
#ifdef WITH_OCLOC
    stream << '"' << _filename << '"';
#endif
    _inputSize.Write(stream);
    _pInputModule.Write(stream);
    _pBuildFlags.Write(stream);
    _pConstants.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    _stype.Read(stream);
    _pNext.Read(stream);
    _format.Read(stream);
#ifdef WITH_OCLOC
    stream.get_delimited_string(_filename, '"');
#endif
    _inputSize.Read(stream);
    _pInputModule.Read(stream);
    _pBuildFlags.Read(stream);
    _pConstants.Read(stream);
  }
  virtual void Declare(CCodeOStream& stream) const {
    VariableNameRegister(stream, false);
    if (_stype.DeclarationNeeded()) {
      _stype.VariableNameRegister(stream, false);
      _stype.Declare(stream);
    }
    if (_pNext.DeclarationNeeded()) {
      _pNext.VariableNameRegister(stream, false);
      _pNext.Declare(stream);
    }
    if (_format.DeclarationNeeded()) {
      _format.VariableNameRegister(stream, false);
      _format.Declare(stream);
    }
    if (_inputSize.DeclarationNeeded()) {
      _inputSize.VariableNameRegister(stream, false);
      _inputSize.Declare(stream);
    }
    if (_pInputModule.DeclarationNeeded()) {
      _pInputModule.VariableNameRegister(stream, false);
      _pInputModule.Declare(stream);
    }
    if (_pBuildFlags.DeclarationNeeded()) {
      _pBuildFlags.VariableNameRegister(stream, false);
      _pBuildFlags.Declare(stream);
    }
    if (_pConstants.DeclarationNeeded()) {
      _pConstants.VariableNameRegister(stream, false);
      _pConstants.Declare(stream);
    }
    std::string varName = stream.VariableName(ScopeKey());
    stream.Indent() << Name() << " " << varName << ";\n";
    stream.Indent() << varName << ".stype = ";
    if (_stype.DeclarationNeeded()) {
      stream << stream.VariableName(_stype.ScopeKey());
    } else {
      _stype.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".pNext = ";
    if (_pNext.DeclarationNeeded()) {
      stream << stream.VariableName(_pNext.ScopeKey());
    } else {
      _pNext.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".inputSize = ";
    if (_inputSize.DeclarationNeeded()) {
      stream << stream.VariableName(_inputSize.ScopeKey());
    } else {
      _inputSize.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".format = ";
    if (_format.DeclarationNeeded()) {
      stream << stream.VariableName(_format.ScopeKey());
    } else {
      _format.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".pInputModule = ";
    if (_pInputModule.DeclarationNeeded()) {
      stream << stream.VariableName(_pInputModule.ScopeKey());
    } else {
      _pInputModule.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".pBuildFlags = ";
    if (_pBuildFlags.DeclarationNeeded()) {
      stream << stream.VariableName(_pBuildFlags.ScopeKey());
    } else {
      _pBuildFlags.Write(stream);
    }
    stream << ";\n";
    stream.Indent() << varName << ".pConstants = ";
    if (_pConstants.DeclarationNeeded() && _pConstants.AmpersandNeeded()) {
      if (dynamic_cast<const CSArray*>(&_pConstants)) {
        stream << "&";
      }
      stream << stream.VariableName(_pConstants.ScopeKey());
    } else {
      _pConstants.Write(stream);
    }
    stream << ";\n";
  }
  virtual void Write(CCodeOStream& /*stream*/) const {}
  L0Type operator*() {
    const auto* ptr = Ptr();
    if (ptr == nullptr) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    return *ptr;
  }
  L0Type* Ptr() {
    _struct.stype = *_stype;
    _struct.pNext = *_pNext;
    _struct.format = *_format;
    bool oclocHandle = false;
#ifdef WITH_OCLOC
    const auto* oclocInputModule = FindOclocInputModule(ocloc::SD(), _filename);
    if (oclocInputModule != nullptr && !oclocInputModule->empty()) {
      _struct.inputSize = oclocInputModule->size();
      _struct.pInputModule = oclocInputModule->data();
      oclocHandle = true;
    }
#endif
    if (!oclocHandle) {
      _struct.inputSize = *_inputSize;
      _struct.pInputModule = *_pInputModule;
    }
    _struct.pBuildFlags = *_pBuildFlags;
    _struct.pConstants = /*_pConstants.Ptr()*/ nullptr;
    return &_struct;
  }
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

} // namespace l0
} // namespace gits
