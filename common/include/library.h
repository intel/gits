// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   library.h
 *
 * @brief Declaration of a base class for graphic libraries implementation.
 *
 */

#pragma once

#include "tools.h"
#include "id.h"
#include "timer.h"
#include <string>
#include <vector>

namespace gits {
class CToken;
class CFunction;
class CTokenFontsCreate;
class CState;
class CBinOStream;
class CBinIStream;
class CDisplay;
class CRecorderWrapper;
class CPlayer;
class CWindowInfo;

/**
   * @brief Base class for graphic libraries implementation.
   *
   * gits::CLibrary is a base class for graphic libraries implementation.
   * It provides methods to create function calls wrappers based on their
   * identifiers, to create wrappers that are responsible for obtaining
   * and comparing library state. It is also responsible for creating
   * graphic display that will present captured calls.
   */
class CLibrary : private gits::noncopyable {
public:
  typedef std::function<CState*()> state_creator_t;
  enum TId {
    ID_OPENGL, /**< @brief identifier of OpenGL library */
    ID_OPENCL, /**< @brief identifier of OpenCL library */
    ID_VULKAN,
    ID_LEVELZERO,
    ID_OCLOC,
    ID_NUM
  };

private:
  CId _id;
  state_creator_t _func;

public:
  explicit CLibrary(TId id, state_creator_t func = state_creator_t());

  virtual ~CLibrary();

  TId Id() const;

  virtual CFunction* FunctionCreate(unsigned id) const = 0;
  CState* StateCreate() const;

  virtual const char* Name() const = 0;
};

} // namespace gits
