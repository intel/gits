// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Structs.h"

namespace gits {
namespace l0 {
namespace {
#ifdef WITH_OCLOC
std::vector<uint8_t>* FindOclocInputModule(ocloc::CStateDynamic& sd, const std::string& filename) {
  auto bufIt = sd.deprecatedPlayer.find(filename);
  if (bufIt != sd.deprecatedPlayer.end()) {
    return &bufIt->second;
  }
  const std::string backwardCompatibleSuffix = ".bin";
  if (StringEndsWith(filename, backwardCompatibleSuffix)) {
    const auto lengthDifference = filename.length() - backwardCompatibleSuffix.length();

    bufIt = sd.deprecatedPlayer.find(filename.substr(0, lengthDifference));
    if (bufIt != sd.deprecatedPlayer.end()) {
      return &bufIt->second;
    }
  }
  return nullptr;
}

std::vector<uint8_t>* FindOclocInputModule(ocloc::CStateDynamic& sd,
                                           const size_t& size,
                                           const uint8_t* moduleData) {
  const auto originalHash = ComputeHash(moduleData, size, THashType::XX);
  for (const auto& oclocState : sd.oclocStates) {
    for (const auto& originalOclocHash : oclocState.second->originalHashes) {
      if (originalOclocHash == originalHash) {
        for (auto i = 0U; i < oclocState.second->outputNames.size(); i++) {
          if (StringEndsWith(oclocState.second->outputNames[i], ".spv") ||
              StringEndsWith(oclocState.second->outputNames[i], ".bin")) {
            Log(TRACEV) << "Ocloc output's original hash matched: " << originalOclocHash
                        << ", file name: " << oclocState.second->outputNames[i];
            return &oclocState.second->outputData[i];
          }
        }
      }
    }
  }
  return nullptr;
}
#endif
} // namespace

Cze_module_constants_t::Cze_module_constants_t(const L0Type& value)
    : _numConstants(value.numConstants),
      _constantIds(value.numConstants, value.pConstantIds),
      _constantValues(value.numConstants, const_cast<void**>(value.pConstantValues)) {}

Cze_module_constants_t::Cze_module_constants_t(const L0Type* value)
    : Cze_module_constants_t(*value) {}

const char* Cze_module_constants_t::Name() const {
  return Cze_module_constants_t::NAME;
}

void Cze_module_constants_t::Write(CBinOStream& stream) const {
  _numConstants.Write(stream);
  _constantIds.Write(stream);
  _constantValues.Write(stream);
}

void Cze_module_constants_t::Read(CBinIStream& stream) {
  _numConstants.Read(stream);
  _constantIds.Read(stream);
  _constantValues.Read(stream);
}

void Cze_module_constants_t::Declare(CCodeOStream& stream) const {
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

void Cze_module_constants_t::Write([[maybe_unused]] CCodeOStream& stream) const {}

Cze_module_constants_t::L0Type Cze_module_constants_t::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_constants_t::L0Type* Cze_module_constants_t::Ptr() {
  _struct.numConstants = *_numConstants;
  _struct.pConstantIds = *_constantIds;
  _struct.pConstantValues = (const void**)*_constantValues;
  return &_struct;
}

bool Cze_module_constants_t::DeclarationNeeded() const {
  return true;
}

std::set<uint64_t> Cze_module_constants_t::GetMappedPointers() {
  return std::set<uint64_t>();
}

Cze_module_constants_t_V1::Cze_module_constants_t_V1(const L0Type& value) : _version(0) {
  if (value.numConstants > 0) {
    Log(ERR) << "Specialization constants are not supported";
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

Cze_module_constants_t_V1::Cze_module_constants_t_V1(const L0Type* value)
    : Cze_module_constants_t_V1(*value) {}

const char* Cze_module_constants_t_V1::Name() const {
  return Cze_module_constants_t_V1::NAME;
}

void Cze_module_constants_t_V1::Write(CBinOStream& stream) const {
  _version.Write(stream);
}

void Cze_module_constants_t_V1::Read(CBinIStream& stream) {
  _version.Read(stream);
}

void Cze_module_constants_t_V1::Write([[maybe_unused]] CCodeOStream& stream) const {
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

Cze_module_constants_t_V1::L0Type Cze_module_constants_t_V1::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_constants_t_V1::L0Type* Cze_module_constants_t_V1::Ptr() {
  _struct.numConstants = 0U;
  _struct.pConstantIds = nullptr;
  _struct.pConstantValues = nullptr;
  return &_struct;
}

std::set<uint64_t> Cze_module_constants_t_V1::GetMappedPointers() {
  return std::set<uint64_t>();
}

Cze_module_desc_t::Cze_module_desc_t(const L0Type& value)
    : _stype(value.stype),
      _pNext(value.pNext),
      _format(value.format),
      _inputSize(value.inputSize),
      _pInputModule(value.inputSize, value.pInputModule),
      _pBuildFlags(value.pBuildFlags, 0, 1),
      _pConstants(0, value.pConstants) {}

Cze_module_desc_t::Cze_module_desc_t(const L0Type* value) : Cze_module_desc_t(*value) {}

const char* Cze_module_desc_t::Name() const {
  return Cze_module_desc_t::NAME;
}

void Cze_module_desc_t::Write(CBinOStream& stream) const {
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

void Cze_module_desc_t::Read(CBinIStream& stream) {
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

void Cze_module_desc_t::Declare(CCodeOStream& stream) const {
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

void Cze_module_desc_t::Write([[maybe_unused]] CCodeOStream& stream) const {}

Cze_module_desc_t::L0Type Cze_module_desc_t::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_desc_t::L0Type* Cze_module_desc_t::Ptr() {
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

bool Cze_module_desc_t::DeclarationNeeded() const {
  return true;
}

std::set<uint64_t> Cze_module_desc_t::GetMappedPointers() {
  return std::set<uint64_t>();
}

Cze_module_desc_t_V1::Cze_module_desc_t_V1(const L0Type& value)
    : _stype(value.stype),
      _pNext(value.pNext),
      _format(value.format),
      _inputSize(value.inputSize),
      _pInputModule(value.pInputModule, value.inputSize, value.format),
      _pBuildFlags(value.pBuildFlags, 0, 1) {
  if (value.pConstants != nullptr) {
    _pConstants =
        Cze_module_constants_t_V1::CSArray(value.pConstants->numConstants, value.pConstants);
  } else {
    _pConstants = Cze_module_constants_t_V1::CSArray();
  }
}

Cze_module_desc_t_V1::Cze_module_desc_t_V1(const L0Type* value) : Cze_module_desc_t_V1(*value) {}

const char* Cze_module_desc_t_V1::Name() const {
  return Cze_module_desc_t_V1::NAME;
}

void Cze_module_desc_t_V1::Write(CBinOStream& stream) const {
  _stype.Write(stream);
  _pNext.Write(stream);
  _format.Write(stream);
  _inputSize.Write(stream);
  _pInputModule.Write(stream);
  _pBuildFlags.Write(stream);
  _pConstants.Write(stream);
}

void Cze_module_desc_t_V1::Read(CBinIStream& stream) {
  _stype.Read(stream);
  _pNext.Read(stream);
  _format.Read(stream);
  _inputSize.Read(stream);
  _pInputModule.Read(stream);
  _pBuildFlags.Read(stream);
  _pConstants.Read(stream);
}

void Cze_module_desc_t_V1::Declare([[maybe_unused]] CCodeOStream& stream) const {
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

void Cze_module_desc_t_V1::Write([[maybe_unused]] CCodeOStream& stream) const {
  throw EOperationFailed(EXCEPTION_MESSAGE);
}

Cze_module_desc_t_V1::L0Type Cze_module_desc_t_V1::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_desc_t_V1::L0Type* Cze_module_desc_t_V1::Ptr() {
  _struct.stype = *_stype;
  _struct.pNext = *_pNext;
  _struct.format = *_format;
  bool oclocHandle = false;
#ifdef WITH_OCLOC
  const auto* oclocInputModule = FindOclocInputModule(ocloc::SD(), *_inputSize, *_pInputModule);
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
  _struct.pConstants = *_pConstants;
  return &_struct;
}

bool Cze_module_desc_t_V1::DeclarationNeeded() const {
  return true;
}

std::set<uint64_t> Cze_module_desc_t_V1::GetMappedPointers() {
  return std::set<uint64_t>();
}

std::string Cze_module_desc_t_V1::GetProgramSourceName() const {
  return _pInputModule.FileName();
}

} // namespace l0
} // namespace gits
