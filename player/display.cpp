// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   display.cpp
 * 
 * @brief Definitions of a base class for graphic libraries implementation.
 * 
 */

#include "display.h"

/** 
 * @brief Constructor
 * 
 * Constructor of gits::CDisplay class.
 */
gits::CDisplay::CDisplay(CPlayer& player) : _player(player) {}

/** 
 * @brief Destructor
 * 
 * Destructor of gits::CDisplay class.
 */
gits::CDisplay::~CDisplay() {}
