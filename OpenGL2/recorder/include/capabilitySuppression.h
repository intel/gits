// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

namespace gits::opengl2 {

// Hides capabilities from the application: the extensions in OpenGL.Recorder.SuppressExtensions,
// and, with OpenGL.Recorder.SuppressProgramBinary, binaries only the recording driver can load, so
// the application compiles its shaders from source. See codegen/capability_suppression.py for the
// wrappers calling these. Each answer leaves the driver's error state as the driver's own call
// would.

// Answers a state query in place of the driver: the number of extensions left after hiding, no
// program binary formats, and only the SPIR-V shader binary formats. Returns false, leaving the
// query to the driver, for any other `pname` or when there is nothing to hide.
bool AnswerStateQuery(GLenum pname, GLboolean* data);
bool AnswerStateQuery(GLenum pname, GLint* data);
bool AnswerStateQuery(GLenum pname, GLint64* data);
bool AnswerStateQuery(GLenum pname, GLfloat* data);
bool AnswerStateQuery(GLenum pname, GLdouble* data);

// The extension string without the hidden extensions. The result stays valid for the process
// lifetime.
const GLubyte* HideExtensions(GLenum name, const GLubyte* extensions);
const char* HideExtensions(const char* extensions);

// The driver's index of the extension at `index` in the current context's list without the hidden
// extensions. An index past the end of that list stays past the end of the driver's.
GLuint DriverExtensionIndex(GLenum name, GLuint index);

// Whether a binary load in a hidden format must be neither executed nor recorded. A new program
// then stays unlinked, as after a load the driver rejects; one linked before keeps its
// executable.
bool SkipsProgramBinary(GLenum binaryFormat);
bool SkipsShaderBinary(GLenum binaryFormat);

} // namespace gits::opengl2
