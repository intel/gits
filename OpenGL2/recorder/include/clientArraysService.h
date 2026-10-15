// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "clientArraysMetaTypes.h"
#include "glTypes.h"

#include <cstdint>

namespace gits::opengl2 {

// What one hooked draw reads, as filled in by ClientArraysLayer (clientArraysLayerAuto.cpp).
struct ClientArraysDraw {
  bool IsElements{false};
  GLint First{0};
  GLsizei Count{0};
  GLenum IndexType{0};
  // Client-memory index bytes captured by the draw command itself; HasValue is false when a
  // GL_ELEMENT_ARRAY_BUFFER is bound, in which case IndicesOffset is the offset into it.
  const ArrayArgument<uint8_t>* Indices{nullptr};
  uintptr_t IndicesOffset{0};
  GLint BaseVertex{0};
  GLsizei InstanceCount{1};
  GLuint BaseInstance{0};
  // Multi-draws (glMultiDrawArrays, glMultiDrawElementsBaseVertex): Counts is non-null and holds
  // DrawCount sub-draw counts, replacing Count. Firsts, IndicesOffsets and BaseVertices replace
  // First, IndicesOffset and BaseVertex per sub-draw; a null BaseVertices means 0.
  GLsizei DrawCount{0};
  const GLsizei* Counts{nullptr};
  const GLint* Firsts{nullptr};
  const uintptr_t* IndicesOffsets{nullptr};
  const GLint* BaseVertices{nullptr};
};

// Captures the client-memory vertex arrays a draw is about to read: every enabled array with no
// buffer object bound, over the vertex (and instance) range the draw reads. Array state is read
// from the driver at the draw, so VAO switches and glPopClientAttrib need no tracking; pointer
// setter calls only narrow which slots are queried. Bytes already sent and unchanged are
// referenced instead of re-sent. Only legal queries for the current context's version/profile
// are issued; see clientArraysService.cpp.
class ClientArraysService {
public:
  static ClientArraysService& Get();

  // Takes ownership of `key` (the spare key of the draw's key pair): records a
  // ClientArraysMetaCommand under it, or releases it via CaptureManager::SkipCommandKey() when
  // the draw reads no client memory.
  void Capture(uint32_t threadId, const ClientArraysDraw& draw, uint64_t key);

  // Called by every array pointer setter's wrapper, enabled or not (client_array_draws.py), so
  // draws only query slots ever set from client memory. `slot` is the texture unit or attribute
  // index; -1 for a texture coordinate array means the current client active texture unit.
  // `flags` are the CLIENT_ARRAY_FLAG_* the setter implies (glVertexAttribIPointer*: INTEGER,
  // glVertexAttribLPointer*: LONG), used where the context's array state cannot be queried for
  // them.
  void NoteArrayPointer(ClientArrayKind kind,
                        int64_t slot,
                        const void* pointer,
                        uint32_t flags = 0);
  // glInterleavedArrays: may set the vertex, normal, color and current texture coordinate arrays.
  void NoteInterleavedArrays(const void* pointer);

private:
  ClientArraysService();
};

} // namespace gits::opengl2
