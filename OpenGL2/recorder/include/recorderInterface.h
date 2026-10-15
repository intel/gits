// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#if defined(_WIN32)
#define OPENGL_RECORDER_CALL __stdcall
#else
#define OPENGL_RECORDER_CALL
#endif

namespace gits::opengl2 {

class IRecorder {
public:
  virtual ~IRecorder() = default;

  // Names are non-null, null-terminated strings. Every known command from
  // supported API has a wrapper; real-function resolution may return null and
  // remains retryable. SetResolvedFunction returns false only when the command
  // name is unknown.
  virtual void* GetWrapper(const char* name) = 0;
  virtual void* ResolveRealFunction(const char* name) = 0;
  virtual bool SetResolvedFunction(const char* name, void* pointer) = 0;
  virtual void Close() = 0;
};

} // namespace gits::opengl2

extern "C" gits::opengl2::IRecorder* OPENGL_RECORDER_CALL GITSRecorderOpenGL2();
