// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstdint>

namespace gits::opengl2 {

// Computes the byte size of a client-memory pixel transfer buffer (glTexImage2D's `pixels`,
// and any future glTex*Image*/glReadPixels-family command that shares the same
// format/type/dimensions shape), i.e. gl.xml's COMPSIZE(format,type,width,height) pseudo-length
// - see pointer_metadata.py's LengthSourceKind.PIXEL_TRANSFER_SIZE. GL itself does not expose
// this as a queryable value; the driver derives it internally from `format`+`type` (bytes per
// pixel) times `width`*`height` (pixel count), so this reimplements that derivation for the
// common, non-packed component/type combinations real GL 3.2+ core-profile callers actually use.
//
// Accounts for row-stride padding at the CALLER-SUPPLIED `alignment` (rows are padded up to a
// multiple of `alignment` bytes, per the GL pixel-store spec's own image-size formula) - this
// is the one pixel-store variable applied here, chosen because it affects nearly every real
// capture with a non-default-aligned row (e.g. any odd-width GL_RGB image, or any app that
// calls glPixelStorei(GL_(UN)PACK_ALIGNMENT, 1) - a very common real-world pattern for tightly
// packed texture data) and is wrong even for an app that never touches pixel-store state at all
// if ignored, which the original, naive width*height*bytesPerPixel computation was.
//
// `alignment` must be the REAL, current value of whichever of GL_PACK_ALIGNMENT/
// GL_UNPACK_ALIGNMENT applies to the calling command (GL_UNPACK_ALIGNMENT for an IN-direction
// transfer like glTexImage2D, GL_PACK_ALIGNMENT for an OUT-direction one like glReadPixels) -
// see QueryUnpackAlignment()/QueryPackAlignment() below, which query it live via glGetIntegerv
// at each call site (commandsAuto.h.mako/recorderWrappersAuto.cpp.mako) rather than assuming a
// fixed default. GL only ever allows this value to be 1, 2, 4, or 8 (glPixelStorei rejects any
// other value with GL_INVALID_VALUE and leaves the state unchanged) - an `alignment` outside
// that set (which should never happen from a real driver query) falls back to GL's own default
// of 4 rather than risk a division-adjacent computation on a nonsensical value.
//
// Still does NOT account for GL_(UN)PACK_ROW_LENGTH/GL_(UN)PACK_SKIP_ROWS/
// GL_(UN)PACK_SKIP_PIXELS - tracking those is out of scope for this increment (see
// docs/development/opengl2-player-skeleton-plan.md's Decision 10). An app that uses a non-zero
// ROW_LENGTH/SKIP_ROWS/SKIP_PIXELS before an IN-direction pixel transfer with a non-null
// pointer or an OUT-direction one may still have its payload captured a few bytes short/long;
// this is a known, documented limitation, not a silent correctness assumption - unlike the
// original no-padding-at-all computation, and unlike always assuming the default alignment
// regardless of real pixel-store state, it no longer produces a wrong result for the
// overwhelmingly common cases of an app that never touches pixel-store state at all, or one
// that only changes alignment (without ROW_LENGTH/SKIP_ROWS/SKIP_PIXELS).
//
// Returns 0 for an unrecognized `format`/`type` combination, a non-positive `width`/`height`,
// or an intermediate/final size computation that
// would overflow uint64_t - rather than guessing, or silently returning a wrapped-around value
// that no longer reflects the real transfer size. Callers must treat 0 as "size unknown",
// matching how ArrayArgument<T> already treats a null pointer (see arguments.h): the
// constructor never dereferences the pointer when count is 0 either, so any of these cases
// degrades to an empty/lost payload instead of an out-of-bounds read, an oversized allocation,
// or a crash.
//
// Returns uint64_t, not a 32-bit type: width/height/bytesPerPixel's product can legitimately
// approach or exceed 4 GiB for a large-enough transfer, and the sole caller (ArrayArgument<T>'s
// own templated, size_t-based constructor/Capture()) already accepts any integer width, so
// there is no reason to narrow this function's own result before returning it - see
// pixelTransferSize.cpp's own comment on why the arithmetic itself is done in uint64_t too,
// rather than merely widening the final cast.
uint64_t ComputePixelTransferByteSize(
    GLenum format, GLenum type, GLsizei width, GLsizei height, GLint alignment);

// Queries the real, current GL_UNPACK_ALIGNMENT/GL_PACK_ALIGNMENT via glGetIntegerv, for use as
// ComputePixelTransferByteSize()'s `alignment` argument - see that function's own comment for
// which of the two applies to a given command's direction. Forward-declared here (rather than
// included via a recorder-only header) and defined in
// OpenGL2/recorder/mapTrackingWrappers.cpp, which has access to the real driver dispatch table
// these need - this header (and the two call sites that use these, both effectively
// recorder-only despite living in code compiled into the shared common/layer_interface library
// too, since only the recorder's own generated code ever actually invokes the constructor/
// Capture() overloads that call these - see commandsAuto.h.mako/recorderWrappersAuto.cpp.mako)
// must stay includable/compilable from layer_interface's own translation units (e.g.
// commandCodersAuto.cpp), which have no access to OpenGL2/recorder/include.
GLint QueryUnpackAlignment();
GLint QueryPackAlignment();

// Computes the element count of glClearBufferfv/glClearBufferiv/glClearBufferuiv's `value`
// array (gl.xml's COMPSIZE(buffer) pseudo-length) - see pointer_metadata.py's
// LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT. 4 for GL_COLOR (RGBA), 1 for GL_DEPTH/
// GL_STENCIL. Returns 1 (a safe, non-zero default) for any other `buffer` value rather than 0,
// since unlike a pixel-transfer format/type combination there is no meaningful "unrecognized"
// case in practice - `buffer` only ever takes one of these three values for a well-formed call.
uint32_t ComputeClearBufferValueCount(GLenum buffer);

} // namespace gits::opengl2
