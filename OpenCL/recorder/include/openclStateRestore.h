// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclStateRestore.h
*
* @brief Declaration of OpenCL State Restore.
*/

#pragma once
#include "library.h"
#include "state.h"
#include "scheduler.h"
#include "openclLibrary.h"
#include "openclStateDynamic.h"

namespace gits {
namespace OpenCL {
void RestorePlatforms(CScheduler& scheduler, CStateDynamic& sd);
void RestoreDevices(CScheduler& scheduler, CStateDynamic& sd);
void RestoreContexts(CScheduler& scheduler, CStateDynamic& sd);
void RestoreCommandQueues(CScheduler& scheduler, CStateDynamic& sd);
void RestoreMemObjects(CScheduler& scheduler, CStateDynamic& sd);
void RestoreSamplers(CScheduler& scheduler, CStateDynamic& sd);
void RestoreUsm(CScheduler& scheduler, CStateDynamic& sd);
void RestoreSvm(CScheduler& scheduler, CStateDynamic& sd);
void RestoreMappedPointers(CScheduler& scheduler, CStateDynamic& sd);
void RestorePrograms(CScheduler& scheduler, CStateDynamic& sd);
void RestoreKernels(CScheduler& scheduler, CStateDynamic& sd);
void RestoreEvents(CScheduler& scheduler, CStateDynamic& sd);
void RestoreMemObject(CScheduler& scheduler,
                      const cl_mem& memObj,
                      std::shared_ptr<CCLMemState>& state,
                      const cl_command_queue& commandQueue);

class CState : public gits::CState {
public:
  // CScheduler & _scheduler;
  CState() {}
  void Get() {} // Get state is not being used in OpenCL API. Objects are
                // queried and scheduled in one step in "Schedule" function.
  void Schedule(CScheduler& scheduler) const {
    auto& sd = SD();
    RestorePlatforms(scheduler, sd);
    RestoreDevices(scheduler, sd);
    RestoreContexts(scheduler, sd);
    RestoreCommandQueues(scheduler, sd);
    RestoreMemObjects(scheduler, sd);
    RestoreSamplers(scheduler, sd);
    RestoreSvm(scheduler, sd);
    RestoreMappedPointers(scheduler, sd);
    RestorePrograms(scheduler, sd);
    RestoreUsm(scheduler, sd);
    RestoreKernels(scheduler, sd);
    RestoreEvents(scheduler, sd);
  }
  void Finish(CScheduler& scheduler) const;
};
} // namespace OpenCL
} // namespace gits
