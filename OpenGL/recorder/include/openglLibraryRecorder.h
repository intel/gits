// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openglLibrary.h"

namespace gits {
namespace OpenGL {
/**
     * @brief OpenGL library class
     *
     * gits::OpenGL::CLibrary class provides OpenGL library tools
     * for GITS project. It is responsible for creating OpenGL
     * function call wrappers based on unique ID, OpenGL library state
     * getter class and for creating OpenGL display window for gitsPlayer.
     */
class CLibraryRecorder : public CLibrary {
public:
  CLibraryRecorder();
  CLibraryRecorder(const CLibraryRecorder& other) = delete;
  CLibraryRecorder& operator=(const CLibraryRecorder& other) = delete;
  virtual ~CLibraryRecorder();
};
} // namespace OpenGL
} // namespace gits
