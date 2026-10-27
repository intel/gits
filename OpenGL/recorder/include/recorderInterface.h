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
#define OPENGL_RECORDER_EXPORT
#else
#define OPENGL_RECORDER_CALL
#define OPENGL_RECORDER_EXPORT __attribute__((visibility("default")))
#endif

namespace gits::opengl {

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

} // namespace gits::opengl

extern "C" OPENGL_RECORDER_EXPORT gits::opengl::IRecorder* OPENGL_RECORDER_CALL
GITSRecorderOpenGL();
