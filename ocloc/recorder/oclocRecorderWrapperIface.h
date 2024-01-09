// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <functional>

#include "platform.h"

#include "oclocHeader.h"

namespace gits {
namespace ocloc {
class CDriver;

class IRecorderWrapper {
public:
  virtual void StreamFinishedEvent(std::function<void()> e) = 0;
  virtual void CloseRecorderIfRequired() = 0;
  virtual CDriver& Drivers() const = 0;
  virtual void InitializeDriver() const = 0;
  virtual void MarkRecorderForDeletion() = 0;

  virtual void oclocInvoke(int return_value,
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
                           char*** nameOutputs) const = 0;
  virtual void oclocFreeOutput(int return_value,
                               uint32_t* numOutputs,
                               uint8_t*** dataOutputs,
                               uint64_t** lenOutputs,
                               char*** nameOutputs) const = 0;
};
} // namespace ocloc
} // namespace gits

typedef gits::ocloc::IRecorderWrapper*(STDCALL* FGITSRecoderOcloc)();

extern "C" {
gits::ocloc::IRecorderWrapper* STDCALL GITSRecorderOcloc() VISIBLE;
}
