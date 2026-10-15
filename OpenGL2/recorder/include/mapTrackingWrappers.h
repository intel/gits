// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written recorder wrappers for glMapBufferRange/glUnmapBuffer, bypassing the generated
// per-command wrapper path entirely - see docs/development/opengl2-buffer-map-tracking-plan.md's
// Decision 3 for why (a mapped pointer return value cannot be expressed in
// pointer_metadata.py's vocabulary, and resolving the bound buffer needs an extra driver call no
// other generated wrapper needs). Mirrors unsupportedCommand.h's "small, hand-written, not
// generated" precedent in shape, applied to command *dispatch* instead of command *reporting*.

namespace gits::opengl2 {

// Returns &CustomWrapper_glMapBufferRange/&CustomWrapper_glUnmapBuffer for those two names,
// nullptr for every other name - checked by GetGeneratedRecorderWrapper
// (recorderWrappersAuto.cpp.mako) before its own generated static map, so these two names never
// reach their generated (and, for these two names only, unused - see Decision 3's Consequences)
// Wrapper_glMapBufferRange/Wrapper_glUnmapBuffer counterparts.
void* GetCustomRecorderWrapper(const char* name);

// Guards against treating a pixel-transfer "pointer" as client memory when it is actually a
// buffer-relative byte offset (glTexImage2D-shaped commands
// crash/read-garbage when a pixel-unpack buffer is bound; glReadPixels-shaped commands crash/
// write-garbage when a pixel-pack buffer is bound). Both query the real, current
// GL_PIXEL_(UN)PACK_BUFFER_BINDING directly via the real driver (same technique as
// ResolveBoundBuffer() above, but quiet - "nothing bound" is the overwhelmingly common,
// unremarkable case here, unlike ResolveBoundBuffer's own map/unmap-specific callers).
//
// Returns `pixels` unchanged when no such buffer object is bound (the common, client-memory
// case this backend already supports). Returns nullptr - safe to pass to ArrayArgument<T>'s
// constructor/Capture(), which are already null-pointer-safe regardless of any accompanying
// byte count - when one is bound, logging a one-time-per-command-name degraded-fidelity
// warning instead of copying `byteSize` bytes from what would actually be a small integer
// offset misread as an address. Full PBO round-trip support (recording the offset and tying it
// to the buffer's own tracked contents) is out of scope for this pass - see
// modern-opengl-backend-architecture.md's PBO scope decision.
const void* GuardUnpackPixelsPointer(const char* commandName, const void* pixels);
const void* GuardPackPixelsPointer(const char* commandName, const void* pixels);

// Note: pixelTransferSize.h's QueryUnpackAlignment()/QueryPackAlignment() are declared there
// (shared common/layer_interface code) but DEFINED in this file's .cpp, not this header - see
// that header's own comment for why (recorder-only, needs the real driver dispatch table via
// glGetIntegerv, same technique as ResolveBoundBuffer()/QueryBoundBufferQuiet() above).

} // namespace gits::opengl2
