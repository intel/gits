// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <stdint.h>

#ifdef WIN32
#define __ocloccall __cdecl
#else
#define __ocloccall
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef int(__ocloccall* pfn_oclocInvoke)(unsigned int argc,
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
                                          char*** nameOutputs);

typedef int(__ocloccall* pfn_oclocFreeOutput)(uint32_t* numOutputs,
                                              uint8_t*** dataOutputs,
                                              uint64_t** lenOutputs,
                                              char*** nameOutputs);

#if defined(__cplusplus)
}
#endif
