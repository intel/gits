// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   window.h
 * 
 * @brief Declaration of a base class for graphic display windows implementation.
 * 
 */

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "windowInfo.h"
#include <string>

namespace gits {

class CPlayer;

class CWindow {
  std::string _title;
  const CWindowInfo _info;
  CPlayer& _player;

public:
  CWindow(const std::string& title, const CWindowInfo& info, CPlayer& player);
  virtual ~CWindow();

  const CWindowInfo& WindowInfo() const {
    return _info;
  }
  CPlayer& Player() const {
    return _player;
  }

  void Draw() const;
  void Key(unsigned code, bool shift, bool ctrl, bool alt) const;

  virtual void FrameEnd() const = 0;
};

} // namespace gits

#endif /* __WINDOW_H__ */
