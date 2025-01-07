// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "oclocRecorderWrapperIface.h"

namespace gits {
class CRecorder;
namespace ocloc {
class CDriver;

class CRecorderWrapper : public IRecorderWrapper {
  CRecorder& _recorder;
  CRecorderWrapper(const CRecorderWrapper& ref);            // do not allow copy construction
  CRecorderWrapper& operator=(const CRecorderWrapper& ref); // do not allow class assignment

public:
  CRecorderWrapper(CRecorder& recorder);
  ~CRecorderWrapper() = default;
  void StreamFinishedEvent(std::function<void()> e);
  void CloseRecorderIfRequired() override;
  CDriver& Drivers() const override;
  void InitializeDriver() const override;
  void MarkRecorderForDeletion() override;
  std::recursive_mutex& GetInterceptorMutex() const override;

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
                           char*** nameOutputs) const override;
  virtual void oclocFreeOutput(int return_value,
                               uint32_t* numOutputs,
                               uint8_t*** dataOutputs,
                               uint64_t** lenOutputs,
                               char*** nameOutputs) const override;
};
} // namespace ocloc
} // namespace gits
