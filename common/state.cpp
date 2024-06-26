// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "state.h"
#include "tools.h"
#include "exception.h"

namespace gits {

CComponentState::~CComponentState() {
  Purge(_variableList);
}

void CComponentState::Register(CVariable* variable) {
  _variableList.push_back(variable);
}

void CComponentState::Get() {
  for (auto var : _variableList) {
    var->Get();
  }
}

void gits::CComponentState::Schedule(CScheduler& scheduler,
                                     const CComponentState& lastState) const {
  if (_variableList.size() != lastState._variableList.size()) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  for (auto it = _variableList.begin(), itLast = lastState._variableList.begin();
       it != _variableList.end() && itLast != lastState._variableList.end(); ++it, ++itLast) {
    (*it)->Schedule(scheduler, *(*itLast));
  }
}

} // namespace gits
