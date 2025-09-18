// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "log2.h"

namespace gits {
namespace ocloc {
void LogOclocInvokeInput(unsigned int argc,
                         const char** argv,
                         const uint32_t numSources,
                         const uint8_t** sources,
                         const uint64_t* sourceLens,
                         const char** sourcesNames,
                         const uint32_t numInputHeaders,
                         const uint8_t** dataInputHeaders,
                         const uint64_t* lenInputHeaders,
                         const char** nameInputHeaders);
void LogOclocInvokeOutput(int ret,
                          uint32_t* numOutputs,
                          uint8_t*** dataOutputs,
                          uint64_t** lenOutputs,
                          char*** nameOutputs);

bool IsAr(const uint8_t* binary);
} // namespace ocloc
} // namespace gits
