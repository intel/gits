// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
                                           const uint8_t* moduleData,
                                           const ze_module_format_t format) {
  const auto originalHash = ComputeHash(moduleData, size, THashType::XX);
  for (const auto& oclocState : sd.oclocStates) {
    for (const auto& originalOclocHash : oclocState.second->originalHashes) {
      if (originalOclocHash == originalHash) {
        for (auto i = 0U; i < oclocState.second->outputNames.size(); i++) {
          if ((format == ZE_MODULE_FORMAT_IL_SPIRV &&
               StringEndsWith(oclocState.second->outputNames[i], ".spv")) ||
              (format == ZE_MODULE_FORMAT_NATIVE &&
               (StringEndsWith(oclocState.second->outputNames[i], ".bin") ||
                StringEndsWith(oclocState.second->outputNames[i], ".ar")))) {
            LOG_TRACEV << "Ocloc output's original hash matched: " << originalOclocHash
                       << ", file name: " << oclocState.second->outputNames[i];
            return &oclocState.second->outputData[i];
          }
        }
        LOG_WARNING << "Ocloc output's original hash matched: " << originalOclocHash
                    << " but there is no file that matches given format: "
                    << ToStringHelper(format);
        return nullptr;
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

std::set<uint64_t> Cze_module_constants_t::GetMappedPointers() {
  return std::set<uint64_t>();
}

Cze_module_constants_t_V1::Cze_module_constants_t_V1(const L0Type& value)
    : _version(1U),
      _numConstants(value.numConstants),
      _constantIds(value.numConstants, value.pConstantIds) {
  if (value.numConstants > 0U && value.pConstantValues != nullptr) {
    std::vector<uint64_t> specConstValues(value.numConstants);
    for (auto i = 0U; i < value.numConstants; i++) {
      const uint64_t* ptr = reinterpret_cast<const uint64_t*>(value.pConstantValues[i]);
      const uint64_t ptrValue = *ptr;
      specConstValues[i] = ptrValue;
    }
    _constantValues = Cuint64_t::CSArray(value.numConstants, specConstValues.data());
  } else {
    _constantValues = Cuint64_t::CSArray();
  }
}

Cze_module_constants_t_V1::Cze_module_constants_t_V1(const L0Type* value)
    : Cze_module_constants_t_V1(*value) {}

const char* Cze_module_constants_t_V1::Name() const {
  return Cze_module_constants_t_V1::NAME;
}

void Cze_module_constants_t_V1::Write(CBinOStream& stream) const {
  _version.Write(stream);
  if (*_version == 1U) {
    _numConstants.Write(stream);
    _constantIds.Write(stream);
    _constantValues.Write(stream);
  }
}

void Cze_module_constants_t_V1::Read(CBinIStream& stream) {
  _version.Read(stream);
  if (*_version == 1U) {
    _numConstants.Read(stream);
    _constantIds.Read(stream);
    _constantValues.Read(stream);
  }
}

Cze_module_constants_t_V1::L0Type Cze_module_constants_t_V1::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_constants_t_V1::L0Type* Cze_module_constants_t_V1::Ptr() {
  if (*_version == 0U) {
    _struct.numConstants = 0U;
    _struct.pConstantIds = nullptr;
    _struct.pConstantValues = nullptr;
  } else if (*_version == 1U) {
    _struct.numConstants = *_numConstants;
    if (_struct.numConstants > 0U) {
      _struct.pConstantIds = *_constantIds;
      pPointers.push_back(std::make_unique<uintptr_t[]>(_struct.numConstants));
      for (auto i = 0U; i < _struct.numConstants; i++) {
        const void* ptr = &_constantValues.Vector()[i];
        std::memcpy(&pPointers.back().get()[i], &ptr, sizeof(uintptr_t));
      }
      _struct.pConstantValues = (const void**)pPointers.back().get();
    }
  } else {
    LOG_ERROR << "Not implemented Cze_module_constants version: " << ToStringHelper(*_version);
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
  return &_struct;
}

std::set<uint64_t> Cze_module_constants_t_V1::GetMappedPointers() {
  return std::set<uint64_t>();
}

const char* Cze_module_constants_ptr::NAME = "const ze_module_constants_t*";

Cze_module_constants_ptr::Cze_module_constants_ptr(const L0Type& value)
    : _pConstants(value != nullptr ? 1U : 0U, value) {}

Cze_module_constants_ptr::Cze_module_constants_ptr(const L0Type* value)
    : Cze_module_constants_ptr(*value) {}

const char* Cze_module_constants_ptr::Name() const {
  return Cze_module_constants_ptr::NAME;
}

void Cze_module_constants_ptr::Write(CBinOStream& stream) const {
  _pConstants.Write(stream);
}

void Cze_module_constants_ptr::Read(CBinIStream& stream) {
  _pConstants.Read(stream);
}

Cze_module_constants_ptr::L0Type Cze_module_constants_ptr::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_constants_ptr::L0Type* Cze_module_constants_ptr::Ptr() {
  return &_pointer;
}

std::set<uint64_t> Cze_module_constants_ptr::GetMappedPointers() {
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

std::set<uint64_t> Cze_module_desc_t::GetMappedPointers() {
  return std::set<uint64_t>();
}

Cze_module_desc_t_V1::Cze_module_desc_t_V1(const L0Type& value)
    : _stype(value.stype),
      _pNext(value.pNext),
      _format(value.format),
      _inputSize(value.inputSize),
      _pInputModule(value.pInputModule, value.inputSize, value.format),
      _pBuildFlags(value.pBuildFlags, 0, 1),
      _pConstants(value.pConstants != nullptr ? 1U : 0U, value.pConstants) {}

Cze_module_desc_t_V1::Cze_module_desc_t_V1(const L0Type* value)
    : _stype(value->stype),
      _pNext(value->pNext),
      _format(value->format),
      _inputSize(value->inputSize),
      _pInputModule(value->pInputModule, value->inputSize, value->format),
      _pBuildFlags(value->pBuildFlags, 0, 1),
      _pConstants(value->pConstants != nullptr ? 1U : 0U, value->pConstants) {}

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
  const auto* oclocInputModule =
      FindOclocInputModule(ocloc::SD(), *_inputSize, *_pInputModule, *_format);
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
  if (_struct.pConstants != nullptr && _struct.pConstants->numConstants == 0U) {
    _struct.pConstants = nullptr;
  }
  return &_struct;
}

std::set<uint64_t> Cze_module_desc_t_V1::GetMappedPointers() {
  return std::set<uint64_t>();
}

std::vector<std::string> Cze_module_desc_t_V1::GetProgramSourcesNames() {
  if (IsPNextOfType(*_pNext, ZE_STRUCTURE_TYPE_MODULE_PROGRAM_EXP_DESC)) {
    auto moduleProgram = reinterpret_cast<Cze_module_program_exp_desc_t*>(_pNext.GetCPtrType());
    return moduleProgram->GetProgramSourcesNames();
  } else {
    std::vector<std::string> fileNames;
    fileNames.push_back(_pInputModule.FileName());
    return fileNames;
  }
}

Cze_module_program_exp_desc_t::Cze_module_program_exp_desc_t(const L0Type& value)
    : _stype(value.stype),
      _pNext(value.pNext),
      _count(value.count),
      _inputSizes(value.count, value.inputSizes),
      _pInputModules(value.count, value.pInputModules, value.inputSizes),
      _pBuildFlags(value.pBuildFlags != nullptr ? value.count : 0U, value.pBuildFlags),
      _pConstants(value.pConstants != nullptr ? value.count : 0U, value.pConstants) {}

Cze_module_program_exp_desc_t::Cze_module_program_exp_desc_t(const L0Type* value)
    : _stype(value->stype),
      _pNext(value->pNext),
      _count(value->count),
      _inputSizes(value->count, value->inputSizes),
      _pInputModules(value->count, value->pInputModules, value->inputSizes),
      _pBuildFlags(value->pBuildFlags != nullptr ? value->count : 0U, value->pBuildFlags),
      _pConstants(value->pConstants != nullptr ? value->count : 0U, value->pConstants) {}

const char* Cze_module_program_exp_desc_t::Name() const {
  return Cze_module_program_exp_desc_t::NAME;
}

void Cze_module_program_exp_desc_t::Write(CBinOStream& stream) const {
  _stype.Write(stream);
  _pNext.Write(stream);
  _count.Write(stream);
  _inputSizes.Write(stream);
  _pInputModules.Write(stream);
  _pBuildFlags.Write(stream);
  _pConstants.Write(stream);
}

void Cze_module_program_exp_desc_t::Read(CBinIStream& stream) {
  _stype.Read(stream);
  _pNext.Read(stream);
  _count.Read(stream);
  _inputSizes.Read(stream);
  _pInputModules.Read(stream);
  _pBuildFlags.Read(stream);
  _pConstants.Read(stream);
}

Cze_module_program_exp_desc_t::L0Type Cze_module_program_exp_desc_t::operator*() {
  const auto* ptr = Ptr();
  if (ptr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return *ptr;
}

Cze_module_program_exp_desc_t::L0Type* Cze_module_program_exp_desc_t::Ptr() {
  _struct.stype = *_stype;
  _struct.pNext = *_pNext;
  _struct.count = *_count;

  bool oclocHandle = false;
  newInputSizes.clear();
  newInputModules.clear();
  for (size_t i = 0; i < *_count; i++) {
    const auto& program = _pInputModules.Vector()[i];
#ifdef WITH_OCLOC
    const auto* oclocInputModule = FindOclocInputModule(
        ocloc::SD(), program.data.size(), program.data.data(), ZE_MODULE_FORMAT_IL_SPIRV);
    if (oclocInputModule != nullptr && !oclocInputModule->empty()) {
      newInputSizes.push_back(oclocInputModule->size());
      newInputModules.push_back(const_cast<uint8_t*>(oclocInputModule->data()));
      oclocHandle = true;
    } else {
      newInputSizes.push_back((*_inputSizes)[i]);
      newInputModules.push_back(
          const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(program.data.data())));
    }
#else
    newInputSizes.push_back((*_inputSizes)[i]);
    newInputModules.push_back(const_cast<uint8_t*>(program.data.data()));
#endif
  }
  if (!oclocHandle) {
    _struct.inputSizes = *_inputSizes;
    _struct.pInputModules = *_pInputModules;
  } else {
    _struct.inputSizes = newInputSizes.data();
    _struct.pInputModules = const_cast<const uint8_t**>(newInputModules.data());
  }

  _struct.pBuildFlags = *_pBuildFlags;
  _struct.pConstants = *_pConstants;
  return &_struct;
}

std::set<uint64_t> Cze_module_program_exp_desc_t::GetMappedPointers() {
  return std::set<uint64_t>();
}

std::vector<std::string> Cze_module_program_exp_desc_t::GetProgramSourcesNames() {
  std::vector<std::string> fileNames;
  for (auto& file : _pInputModules.Vector()) {
    fileNames.push_back(file.filename);
  }
  return fileNames;
}

void* Cze_module_program_exp_desc_t::GetPtrType() {
  return (void*)Ptr();
}
} // namespace l0
} // namespace gits
