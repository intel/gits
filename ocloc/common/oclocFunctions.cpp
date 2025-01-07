// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
#include <vector>

#include "oclocFunctions.h"
#include "oclocDrivers.h"
#include "oclocStateDynamic.h"
#include "oclocStateTracking.h"
#include "oclocTools.h"
#include "arDecoder.h"

#ifdef WITH_LEVELZERO
#include "l0StateDynamic.h"
#include "l0Tools.h"
#endif
#include "exception.h"
#include "log.h"

using namespace gits::ocloc;

namespace gits {
namespace ocloc {
CoclocInvoke::CoclocInvoke(int return_value,
                           unsigned int argc,
                           const char** argv,
                           const uint32_t numSources,
                           const uint8_t** sources,
                           const uint64_t* sourceLens,
                           const char** sourcesNames,
                           const uint32_t numInputHeaders,
                           const uint8_t** dataInputHeaders,
                           const uint64_t* lenInputHeaders,
                           const char** nameInputHeaders,
                           uint32_t* numOutputs,
                           uint8_t*** dataOutputs,
                           uint64_t** lenOutputs,
                           char*** nameOutputs)
    : _return_value(return_value),
      _argc(argc),
      _argv(argc, argv),
      _numSource(numSources),
      _sources(numSources, (const char**)sources, (const size_t*)sourceLens, sourcesNames),
      _sourceLens(numSources, sourceLens),
      _sourcesNames(numSources, sourcesNames),
      _numInputHeader(numInputHeaders),
      _inputHeaders(numInputHeaders, dataInputHeaders),
      _lenInputHeaders(numInputHeaders, lenInputHeaders),
      _headerIncludeNames(numInputHeaders, nameInputHeaders) {}

gits::CArgument& CoclocInvoke::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _argc, _argv, _numSource, _sources, _sourceLens,
                       _sourcesNames, _numInputHeader, _inputHeaders, _lenInputHeaders,
                       _headerIncludeNames);
}

gits::CArgument& CoclocInvoke::Result(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _return_value);
}

void CoclocInvoke::Run() {
  drv.Initialize();
  uint32_t outputsNum = 0;
  uint64_t* outputsSizes;
  uint8_t** outputsData;
  char** outputsNames;
  const char** argV = *_argv;
  std::vector<std::string> arguments;
  for (unsigned int i = 0; i < *_argc; i++) {
    arguments.push_back(argV[i]);
    if (std::string(argV[i]) == "-options" && i + 1 < *_argc) {
      std::string argument = std::string(argV[++i]);
#ifdef GITS_PLATFORM_WINDOWS
      std::replace(argument.begin(), argument.end(), '\'', '"');
#endif
      arguments.push_back(argument + " -I" + Config::Get().common.player.streamDir.string() +
                          "/gitsFiles");
    }
#ifdef WITH_LEVELZERO
    if (std::string(argV[i]) == "-device" && ++i <= *_argc) {
      uint32_t deviceIp = 0;
      auto& drvl0 = gits::l0::drv;
      if (!CGits::Instance().apis.HasCompute()) {
        drvl0.Initialize();
        if (drvl0.IsInitialized()) {
          drvl0.inject.zeInit(ZE_INIT_FLAG_GPU_ONLY);
        }
      }
      if (drvl0.IsInitialized()) {
        deviceIp = gits::l0::GetDeviceIpVersion(gits::l0::SD(), drvl0);
      }
      arguments.push_back(deviceIp != 0 ? std::to_string(deviceIp) : argV[i]);
    }
#endif
  }
  std::vector<const char*> args;
  std::transform(arguments.begin(), arguments.end(), std::back_inserter(args),
                 [](auto& str) { return str.c_str(); });
  std::vector<uint64_t> sourceLens;
  for (auto i = 0U; i < *_numSource; i++) {
    sourceLens.push_back(_sources.Lengths()[i] + 1U);
  }
  _return_value.Value() = drv.oclocInvoke(
      *_argc, args.data(), *_numSource, *_sources, sourceLens.empty() ? nullptr : sourceLens.data(),
      *_sourcesNames, *_numInputHeader, *_inputHeaders, *_lenInputHeaders, *_headerIncludeNames,
      &outputsNum, &outputsData, &outputsSizes, &outputsNames);
  oclocInvoke_SD(nullptr, *_return_value, *_argc, args.data(), *_numSource, *_sources,
                 sourceLens.empty() ? nullptr : sourceLens.data(), *_sourcesNames, *_numInputHeader,
                 *_inputHeaders, *_lenInputHeaders, *_headerIncludeNames, &outputsNum, &outputsData,
                 &outputsSizes, &outputsNames);
  if (_return_value.Value() == 0 && outputsData != nullptr) {
    for (uint32_t i = 0U; i < outputsNum; ++i) {
      const auto moduleData =
          std::vector<uint8_t>(outputsData[i], outputsData[i] + outputsSizes[i]);
      SD().deprecatedPlayer[std::string(outputsNames[i])] = std::move(moduleData);
    }
  }
  drv.oclocFreeOutput(&outputsNum, &outputsData, &outputsSizes, &outputsNames);
}

CoclocInvoke_V1::CoclocInvoke_V1(int return_value,
                                 unsigned int argc,
                                 const char** argv,
                                 const uint32_t numSources,
                                 const uint8_t** sources,
                                 const uint64_t* sourceLens,
                                 const char** sourcesNames,
                                 const uint32_t numInputHeaders,
                                 const uint8_t** dataInputHeaders,
                                 const uint64_t* lenInputHeaders,
                                 const char** nameInputHeaders,
                                 uint32_t* numOutputs,
                                 uint8_t*** dataOutputs,
                                 uint64_t** lenOutputs,
                                 char*** nameOutputs)
    : _return_value(return_value),
      _argc(argc),
      _argv(argc, argv),
      _numSource(numSources),
      _sources(numSources, (const char**)sources, (const size_t*)sourceLens, sourcesNames),
      _sourceLens(numSources, sourceLens),
      _sourcesNames(numSources, sourcesNames),
      _numInputHeader(numInputHeaders),
      _inputHeaders(numInputHeaders, dataInputHeaders),
      _lenInputHeaders(numInputHeaders, lenInputHeaders),
      _headerIncludeNames(numInputHeaders, nameInputHeaders) {

  if (return_value == 0 && dataOutputs != nullptr) {
    std::vector<uint64_t> hashes;
    for (uint32_t i = 0U; i < *numOutputs; ++i) {
      uint64_t hash = ComputeHash((*dataOutputs)[i], (*lenOutputs)[i], THashType::XX);
      hashes.push_back(hash);
      if (IsAr((*dataOutputs)[i])) {
        Ar archive((*dataOutputs)[i], (*lenOutputs)[i]);
        for (const auto& file : archive.files) {
          hash = ComputeHash(file.fileData.data(), file.fileData.size(), THashType::XX);
          hashes.push_back(hash);
        }
      }
    }
    _originalHashes = Cuint64_t::CSArray(hashes);
  }
}

gits::CArgument& CoclocInvoke_V1::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _argc, _argv, _numSource, _sources, _sourceLens,
                       _sourcesNames, _numInputHeader, _inputHeaders, _lenInputHeaders,
                       _headerIncludeNames, _originalHashes);
}

gits::CArgument& CoclocInvoke_V1::Result(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _return_value);
}

void CoclocInvoke_V1::Run() {
  drv.Initialize();
  uint32_t outputsNum = 0;
  uint64_t* outputsSizes;
  uint8_t** outputsData;
  char** outputsNames;
  const char** argV = *_argv;
  std::vector<std::string> arguments;
  for (uint32_t i = 0U; i < *_argc; i++) {
    arguments.push_back(argV[i]);
    if (std::string(argV[i]) == "-options" && i + 1 < *_argc) {
      std::string argument = std::string(argV[++i]);
#ifdef GITS_PLATFORM_WINDOWS
      std::replace(argument.begin(), argument.end(), '\'', '"');
#endif
      arguments.push_back(argument + " -I" + Config::Get().common.player.streamDir.string() +
                          "/gitsFiles");
    }
#ifdef WITH_LEVELZERO
    if (std::string(argV[i]) == "-device" && ++i <= *_argc) {
      uint32_t deviceIp = 0;
      auto& drvl0 = gits::l0::drv;
      if (!CGits::Instance().apis.HasCompute()) {
        drvl0.Initialize();
        if (drvl0.IsInitialized()) {
          drvl0.inject.zeInit(ZE_INIT_FLAG_GPU_ONLY);
        }
      }
      if (drvl0.IsInitialized()) {
        deviceIp = gits::l0::GetDeviceIpVersion(gits::l0::SD(), drvl0);
      }
      arguments.push_back(deviceIp != 0 ? std::to_string(deviceIp) : argV[i]);
    }
#endif
  }
  std::vector<const char*> args;
  std::transform(arguments.begin(), arguments.end(), std::back_inserter(args),
                 [](auto& str) { return str.c_str(); });
  std::vector<uint64_t> sourceLens;
  for (auto i = 0U; i < *_numSource; i++) {
    sourceLens.push_back(_sources.Lengths()[i] + 1U);
  }
  _return_value.Value() = drv.oclocInvoke(
      *_argc, args.data(), *_numSource, *_sources, sourceLens.empty() ? nullptr : sourceLens.data(),
      *_sourcesNames, *_numInputHeader, *_inputHeaders, *_lenInputHeaders, *_headerIncludeNames,
      &outputsNum, &outputsData, &outputsSizes, &outputsNames);
  oclocInvoke_SD(this, *_return_value, *_argc, args.data(), *_numSource, *_sources,
                 sourceLens.empty() ? nullptr : sourceLens.data(), *_sourcesNames, *_numInputHeader,
                 *_inputHeaders, *_lenInputHeaders, *_headerIncludeNames, &outputsNum, &outputsData,
                 &outputsSizes, &outputsNames);
  drv.oclocFreeOutput(&outputsNum, &outputsData, &outputsSizes, &outputsNames);
}

CoclocFreeOutput::CoclocFreeOutput(int return_value,
                                   uint32_t* numOutputs,
                                   uint8_t*** dataOutputs,
                                   uint64_t** lenOutputs,
                                   char*** nameOutputs)
    : _return_value(return_value) {}

gits::CArgument& CoclocFreeOutput::Argument(unsigned idx) {
  return get_cargument(__FUNCTION__, idx);
}

gits::CArgument& CoclocFreeOutput::Result(unsigned idx) {
  return get_cargument(__FUNCTION__, idx, _return_value);
}

void CoclocFreeOutput::Run() {
  // do nothing
}
} // namespace ocloc
} // namespace gits
