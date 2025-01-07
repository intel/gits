// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "tools.h"
#include <vector>

namespace gits {
class CScheduler;

/**
   * @brief Library state component getter class
   *
   * gits::CComponentState class is responsible for obtaining library current state on current context.
   * It contains a list of gits::CComponentState::CVariable classes that handle
   * state for each library variable. The state is obtained with Get().
   * Schedule() method is ment to compare obtained state with the last
   * obtained state (the default state at first time), and if states are
   * different, to schedule library calls that will force that state in
   * the library.
   */
class CComponentState {
public:
  /**
     * @brief Library state variable getter class
     *
     * gits::CComponentState::CVariable class is responsible for managing of library
     * state. Constructor sets default library variable value. The value can
     * be updated to current value by using Get() method. Schedule() method
     * is used to compare the state with the previous one. If the variable
     * value is different, the method should create and schedule new
     * library call that will force that state in the library.
     */
  class CVariable : private gits::noncopyable {
  public:
    virtual ~CVariable() {}

    /**
       * @brief Get library state component for variable
       *
       * Method is responsible for obtaining variable specific library state.
       */
    virtual void Get() = 0;

    /**
       * @brief Schedules library calls to force library state
       *
       * Method is ment to compare obtained variable specific state with
       * the last obtained value, and if states are different, to schedule
       * library calls that will force that state in the library.
       *
       * @param scheduler Scheduler to use.
       * @param lastValue Variable value to compare
       *
       * @exception std::bad_cast Bad @p lastValue class given
       */
    virtual void Schedule(CScheduler& scheduler, const CVariable& lastValue) const = 0;
  };

private:
  std::vector<CVariable*> _variableList; /**< @brief list of library state variables */

protected:
  void Register(CVariable* variable);

public:
  virtual ~CComponentState() = 0;
  CComponentState() {}
  CComponentState(const CComponentState& other) = delete;
  CComponentState& operator=(const CComponentState& other) = delete;

  void Get();
  void Schedule(CScheduler& scheduler, const CComponentState& lastState) const;
};

/**
   * @brief Library state getter class
   *
   * gits::CState class is responsible for obtaining
   * current state component on each context (TODO)
   */
class CState {
public:
  struct CStateDataObsoleted {
    CComponentState* sharedState;
    CComponentState* contextState;
  };

  struct CStateData {
    std::shared_ptr<CComponentState> sharedState;
    std::shared_ptr<CComponentState> contextState;
  };

protected:
  typedef std::map<void*, CStateData> CContextStateDataMap;
  CStateDataObsoleted _stateDataObsoleted;
  CContextStateDataMap _contextStateDataMap;

public:
  virtual ~CState() {}

  virtual const CStateDataObsoleted& StateDataObsoleted() const {
    return _stateDataObsoleted;
  }
  virtual const CContextStateDataMap& ContextStateDataMap() const {
    return _contextStateDataMap;
  }
  virtual void Get() = 0;
  virtual void Schedule(CScheduler& scheduler) const = 0;
  virtual void Finish(CScheduler& scheduler) const = 0;
  virtual void Prepare() const {};
  virtual void PostSchedule(CScheduler& /*scheduler*/) const {};
};

} // namespace gits
