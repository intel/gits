// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocDrivers.h"
#include "oclocRecorderWrapperIface.h"
#include "gitsPluginOcloc.h"

#include "platform.h"

#include <mutex>

namespace {
// Avoid recording API - recursive functions.
uint32_t recursionDepth = 0;
constexpr uint32_t disableDepth = 1000;

std::vector<const char*> ParseArgs(unsigned int argc, const char** argv) {
  bool removeFamilySuffix = true;
  std::vector<const char*> arguments(argc);
  for (auto i = 0U; i < argc; i++) {
    if (std::string(argv[i]) == "-output_no_suffix" || std::string(argv[i]) == "query") {
      removeFamilySuffix = false;
    }
    arguments[i] = argv[i];
  }
  if (removeFamilySuffix) {
    arguments.emplace_back("-output_no_suffix");
  }
  return arguments;
}
} // namespace

using namespace gits::ocloc;

void PrePostDisableOcloc() {
  recursionDepth = disableDepth;
}

#define GITS_WRAPPER_PRE                                                                           \
  --recursionDepth;                                                                                \
  if (CGitsPlugin::Configuration().common.recorder.enabled && (recursionDepth == 0)) {             \
    try {

#define GITS_WRAPPER_POST                                                                          \
  wrapper.CloseRecorderIfRequired();                                                               \
  }                                                                                                \
  catch (...) {                                                                                    \
    topmost_exception_handler(__FUNCTION__);                                                       \
  }                                                                                                \
  }

#define GITS_ENTRY                                                                                 \
  IRecorderWrapper& wrapper = CGitsPlugin::RecorderWrapper();                                      \
  std::unique_lock<std::recursive_mutex> lock(wrapper.GetInterceptorMutex());                      \
  ++recursionDepth;                                                                                \
  CDriver& drv = wrapper.Drivers();

#define GITS_ENTRY_OCLOC GITS_ENTRY

#ifdef GITS_PLATFORM_WINDOWS
#define VISIBLE __declspec(dllexport)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

VISIBLE int __ocloccall oclocInvoke(unsigned int argc,
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
                                    char*** nameOutputs) {
  CGitsPlugin::Initialize();
  GITS_ENTRY_OCLOC
  wrapper.InitializeDriver();
  auto args = ParseArgs(argc, argv);
  auto return_value =
      drv.oclocInvoke(args.size(), args.data(), numSources, sources, sourceLens, sourcesNames,
                      numInputHeaders, dataInputHeaders, lenInputHeaders, nameInputHeaders,
                      numOutputs, dataOutputs, lenOutputs, nameOutputs);
  GITS_WRAPPER_PRE
  wrapper.oclocInvoke(return_value, args.size(), args.data(), numSources, sources, sourceLens,
                      sourcesNames, numInputHeaders, dataInputHeaders, lenInputHeaders,
                      nameInputHeaders, numOutputs, dataOutputs, lenOutputs, nameOutputs);
  GITS_WRAPPER_POST
  return return_value;
}

VISIBLE int __ocloccall oclocFreeOutput(uint32_t* numOutputs,
                                        uint8_t*** dataOutputs,
                                        uint64_t** lenOutputs,
                                        char*** nameOutputs) {
  GITS_ENTRY_OCLOC
  auto return_value = drv.oclocFreeOutput(numOutputs, dataOutputs, lenOutputs, nameOutputs);
  GITS_WRAPPER_PRE
  wrapper.oclocFreeOutput(return_value, numOutputs, dataOutputs, lenOutputs, nameOutputs);
  GITS_WRAPPER_POST
  return return_value;
}

#if defined(__cplusplus)
}
#endif
