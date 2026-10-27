// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openglArguments.h"

namespace gits {
/**
  * @brief OpenGL library specific GITS namespace
  */
namespace OpenGL {

// Class responsible for storing and applying client arrays updates
class ClientArraysUpdate : public CArgument {
  CDataUpdate _update;
  // Returns true if any attrib comes from client side
  bool CheckClientAttribs();
  // Dump attribs data memory updates
  void DumpAttribsUpdate(GLuint frontindex,
                         GLuint backindex,
                         GLuint instances,
                         GLuint baseinstance);
  // Dumps indices data memory updates
  void DumpIndicesUpdate(GLuint buff,
                         GLenum type,
                         GLuint count,
                         const GLvoid* indices,
                         GLuint basevertex,
                         std::set<GLuint>& sorted);
  // Dumps indices attribs data memory in continous indices ranges. It uses
  // DumpAttribsUpdate under the hood.
  void DumpAttribsUpdateOptimized(std::set<GLuint> indices, GLuint instances, GLuint baseinstance);

public:
  ClientArraysUpdate() {}
  ClientArraysUpdate(GLuint index);
  ClientArraysUpdate(GLuint start, GLuint count, GLuint instances, GLuint baseinstance);
  // for glMultiDrawArrays
  ClientArraysUpdate(const GLsizei start[], const GLsizei count[], GLuint primcount);
  // for glMultiDrawElements
  ClientArraysUpdate(GLenum type,
                     const GLsizei* count,
                     const void* const* indices,
                     GLuint primcount);
  ClientArraysUpdate(GLsizei count,
                     GLenum type,
                     const GLvoid* indices,
                     GLuint instances,
                     GLuint baseinstance,
                     GLuint basevertex);

  void Apply() {
    _update.Apply();
  } // Player exclusive function applying update to memory

  virtual const char* Name() const {
    return "";
  }
  virtual unsigned Length() const {
    return _update.Length();
  }
  virtual void Write(CBinOStream& stream) const {
    _update.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    _update.Read(stream);
  }
  virtual uint64_t Size() const override {
    return _update.Size();
  }
};
} // namespace OpenGL
} // namespace gits
