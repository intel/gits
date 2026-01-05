// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   l0StateRestore.h
*
* @brief Declaration of l0 State Restore.
*/

#pragma once
#include "library.h"
#include "state.h"
#include "scheduler.h"
#include "l0Library.h"
#include "l0StateDynamic.h"

namespace gits {
namespace l0 {
void RestoreDrivers(CScheduler& scheduler, CStateDynamic& sd);
void RestoreDevices(CScheduler& scheduler, CStateDynamic& sd);
void RestoreContext(CScheduler& scheduler, CStateDynamic& sd);
void RestoreImages(CScheduler& scheduler, CStateDynamic& sd);
void RestorePhysicalMemory(CScheduler& scheduler, CStateDynamic& sd);
void RestorePointers(CScheduler& scheduler, CStateDynamic& sd);
void RestoreModules(CScheduler& scheduler, CStateDynamic& sd);
void RestoreKernels(CScheduler& scheduler, CStateDynamic& sd);
void RestoreEventPools(CScheduler& scheduler, CStateDynamic& sd);
void RestoreEvents(CScheduler& scheduler, CStateDynamic& sd);
void RestoreCommandList(CScheduler& scheduler, CStateDynamic& sd);
void RestoreCommandQueue(CScheduler& scheduler, CStateDynamic& sd);
void RestoreFences(CScheduler& scheduler, CStateDynamic& sd);
void RestoreCommandListBuffer(CScheduler& scheduler, CStateDynamic& sd);

class CRestoreState : public gits::CState {
public:
  //CScheduler & _scheduler;
  CRestoreState() {}
  void Get() {
  } //Get state is not being used in l0 API. Objects are queried and scheduled in one step in "Schedule" function.
  void Schedule(CScheduler& scheduler) const {
    auto& sd = SD();
    RestoreDrivers(scheduler, sd);
    RestoreDevices(scheduler, sd);
    RestoreContext(scheduler, sd);
    RestoreCommandList(scheduler, sd);
    RestoreImages(scheduler, sd);
    RestoreModules(scheduler, sd);
    RestorePhysicalMemory(scheduler, sd);
    RestorePointers(scheduler, sd);
    RestoreKernels(scheduler, sd);
    RestoreEventPools(scheduler, sd);
    RestoreEvents(scheduler, sd);
    RestoreCommandQueue(scheduler, sd);
    RestoreFences(scheduler, sd);
  }
  void PostSchedule(CScheduler& scheduler) const {
    auto& sd = SD();
    RestoreCommandListBuffer(scheduler, sd);
  }
  void Finish(CScheduler& scheduler) const;
};
} // namespace l0
} // namespace gits
