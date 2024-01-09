// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   runner.h
* 
* @brief GITS tokens runner.
* 
*/

#pragma once

#include "tools.h"

namespace gits {

class CToken;
class CFunction;
class CAction;

class CRunner : private gits::noncopyable {
public:
  class CHandler;
  struct TResultType;

private:
  std::vector<std::shared_ptr<CHandler>> _handlerList;
  bool _hasHandlers;
  TResultType RunWithHandlers(CAction& action, CToken& token) const;

public:
  CRunner();
  void Register(std::shared_ptr<CHandler> plugin);
  TResultType operator()(CAction& action, CToken& token) const;
};

class CRunner::CHandler : private gits::noncopyable {
public:
  enum TSkipType { SKIP_YES, SKIP_NO, SKIP_FORCE };

  CHandler(const CHandler& other) = delete;
  CHandler& operator=(const CHandler& other) = delete;
  virtual ~CHandler() {}
  virtual bool Match(const CFunction& func) = 0;
  virtual CRunner::TResultType PreHandler(CFunction& func) = 0;
  virtual void PostHandler(CFunction& func, bool skipped) = 0;
};

class CAction : private gits::noncopyable {
public:
  virtual void Run(CToken& token);
  virtual ~CAction() {}
};

struct CRunner::TResultType {
  CHandler::TSkipType skip;
  bool schedule;
  TResultType() : skip(CHandler::SKIP_NO), schedule(true) {}
  TResultType(CHandler::TSkipType skip, bool schedule) : skip(skip), schedule(schedule) {}
  bool Skipped() {
    return skip != CRunner::CHandler::SKIP_NO;
  }
};

} // namespace gits
