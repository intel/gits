// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   window.cpp
 * 
 * @brief Definition of a base class for graphic display windows implementation.
 * 
 */

#include "window.h"
#include "player.h"

gits::CWindow::CWindow(const std::string& title, const CWindowInfo& info, CPlayer& player)
    : _title(title), _info(info), _player(player) {}

gits::CWindow::~CWindow() {}

void gits::CWindow::Draw() const {
  if (_player.State() == CPlayer::STATE_RUNNING) {
    // draw next functions group
    _player.Run();
  }
}

void gits::CWindow::Key(unsigned code, bool shift, bool ctrl, bool alt) const {
  _player.Key(code, shift, ctrl, alt);
}
