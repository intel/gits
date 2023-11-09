// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openglLibraryRecorder.cpp
 *
 * @brief Definition of OpenGL recorder part library implementation.
 *
 */

#include "openglLibraryRecorder.h"
#include "openglState.h"
#include "openglRecorderWrapper.h"

namespace gits {
namespace OpenGL {

CLibraryRecorder::CLibraryRecorder() : CLibrary([] { return new CState; }) {}

CLibraryRecorder::~CLibraryRecorder() {
  try {
    SD().WriteClientSizes();
  } catch (...) {
    topmost_exception_handler("CLibraryRecorder::~CLibraryRecorder");
  }
}

} // namespace OpenGL
} // namespace gits
