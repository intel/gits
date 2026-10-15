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

// Guards against treating a pixel-transfer "pointer" as client memory when it is actually a
// buffer-relative byte offset: without this guard, glTexImage2D-shaped commands would
// crash/read garbage when a pixel-unpack buffer is bound, and glReadPixels-shaped commands
// would crash/write garbage when a pixel-pack buffer is bound. Both query the real, current
// GL_PIXEL_(UN)PACK_BUFFER_BINDING directly via the real driver (same technique as
// ResolveBoundBuffer() in mapTrackingService.cpp, but quiet - "nothing bound" is the
// overwhelmingly common, unremarkable case here, unlike ResolveBoundBuffer's own map/unmap-
// specific callers) - but only once ContextSupportsPixelBufferObject()
// (clientMemoryCapture.cpp) has confirmed the binding-point enum itself is legal on the current
// context/version: an unconditional query here would otherwise risk injecting a spurious
// GL_INVALID_ENUM into the APPLICATION's own GL error state on a context old enough to lack the
// feature entirely.
//
// Returns `pixels` unchanged when no such buffer object is bound (the common, client-memory
// case this backend already supports). Returns nullptr when one is bound, logging a
// one-time-per-command-name degraded-fidelity warning instead of copying `byteSize` bytes from
// what would actually be a small integer offset misread as an address - nullptr here is
// exclusively a signal to the CALLER (recorderWrappersAuto.cpp.mako) to skip the client-memory
// ArrayArgument<T> capture for this call, never itself the value serialized onto the wire:
// every caller of either guard ALSO unconditionally captures this parameter's
// own raw, un-guarded bit pattern into a companion m_<param>RawPointer field (see
// pointer_needs_raw_offset_capture()'s own header comment, generator_helpers.py), so a bound
// buffer's real offset still round-trips to replay correctly even though the byte payload
// itself does not. Full PBO round-trip support (tying the offset to the buffer's own tracked
// contents, rather than just preserving the offset value) remains out of scope for this pass -
// see modern-opengl-backend-architecture.md's PBO scope decision.
const void* GuardUnpackPixelsPointer(const char* commandName, const void* pixels);
const void* GuardPackPixelsPointer(const char* commandName, const void* pixels);

// A PIXEL_TRANSFER_SIZE-shaped command's
// "captured size is unknown, skip the real driver call" gate (commandRunnersAuto.cpp.mako's
// pixel_transfer_size_params handling) must NOT
// fire for a call whose pointer parameter was actually routed through buffer-offset mode by
// GuardUnpackPixelsPointer()/GuardPackPixelsPointer() above - in that mode no client-memory
// payload or computed byte count is needed at all: replay just needs the same pixel-store
// state, the same buffer binding, and the same raw offset (already preserved verbatim in
// m_<param>RawPointer - see pointer_needs_raw_offset_capture()'s own header comment,
// generator_helpers.py), and the real driver performs the transfer correctly against its own
// bound buffer. PixelTransferSizeIsUnknown() (pixelTransferSize.h) cannot tell this case apart
// from a genuine capture failure on its own - it only ever reasons about
// format/type/dimensions/pixel-store state, never about buffer bindings - so a caller needing
// to distinguish "size unknown because client memory could not be captured" (must still skip)
// from "size unknown/irrelevant because this was a buffer-offset call all along" (must NOT
// skip) needs this pair as an independent, explicit signal.
//
// Each returns true iff GuardUnpackPixelsPointer()/GuardPackPixelsPointer() (respectively)
// would - queried at this exact moment - return nullptr because a pixel (un)pack buffer
// object is bound, i.e. exactly the same condition that guard already computes internally
// (see clientMemoryCapture.cpp's shared PixelBufferObjectIsBound()); never re-derives that
// condition independently, so it can never disagree with whichever mode the guard itself
// actually chose for this call. The condition depends only on whether a pixel (un)pack buffer
// is currently bound - never on `pixels`' own numeric value, which is accepted purely to keep
// this pair's signature self-documenting/symmetric with the guard functions above, not
// inspected: a null `pixels` with a buffer bound is GL_ARB_pixel_buffer_object's own spec-legal
// way of saying "offset 0 into the bound buffer", a perfectly ordinary case that must still
// report true here, not "not buffer-offset mode".
bool UnpackPixelsPointerIsBufferOffset(const void* pixels);
bool PackPixelsPointerIsBufferOffset(const void* pixels);

// Guards against treating
// glGetQueryObjecti64v/glGetQueryObjectui64v/glGetQueryObjectuiv's OUT-direction `params`
// as real, driver-writable client memory when it is actually a buffer-relative byte offset
// the driver redirects its result write into instead - see pointer_metadata.py's
// PointerParamOverride.query_buffer_guard for the full ARB_query_buffer_object rationale.
// Same technique/shape as GuardUnpackPixelsPointer/GuardPackPixelsPointer above (quiet
// GL_QUERY_BUFFER_BINDING query via the real driver, "nothing bound" the common case, and the
// same ContextSupportsQueryBufferObject() version/extension gate before
// ever querying GL_QUERY_BUFFER_BINDING itself: glGetQueryObject* is valid as far back as GL
// 1.5/ARB_occlusion_query, well below GL_ARB_query_buffer_object's own 4.4 core promotion, so
// an unconditional query would inject a spurious GL_INVALID_ENUM into the application's own
// error state on any context between those two versions that never separately enables the
// extension), just a different binding point and a distinct warn-once message (this is an
// eager POST-call OUT-direction read, not a pre-call IN-direction one, so the risk being
// guarded against is different even though the mechanism looks the same). Returns `params`
// unchanged when no query buffer is bound; returns nullptr when one is, logging a one-time-
// per-command-name degraded-fidelity warning instead of reading `byteSize` bytes from what
// would actually be a small integer offset misread as an address - and, exactly like
// GuardUnpackPixelsPointer/GuardPackPixelsPointer above (see that pair's own
// comment), every caller also unconditionally captures `params`' own raw bit pattern into a
// companion m_paramsRawPointer field, so a bound query buffer's real offset still replays
// against the correct byte range instead of silently defaulting to offset 0.
const void* GuardQueryBufferPointer(const char* commandName, const void* params);

// Returns nullptr when a GL_DRAW_INDIRECT_BUFFER is bound, in which case an indirect draw's
// `indirect` is a byte offset into that buffer; returns `indirect` unchanged otherwise (client
// memory, compatibility profiles only). No warning: both forms replay correctly, since the
// wrapper also records the raw pointer (see pointer_needs_raw_offset_capture(),
// generator_helpers.py). Needs no feature probe, unlike the guards above: every command this
// guards exists only where GL_DRAW_INDIRECT_BUFFER_BINDING is a legal enum.
const void* GuardDrawIndirectPointer(const void* indirect);

// glGetBufferParameteriv of the buffer bound to `target`, through the core entry point or, on a
// context exposing only ARB_vertex_buffer_object, glGetBufferParameterivARB. False (and `value`
// untouched) if neither resolved.
bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint& value);

// Note: pixelTransferSize.h's QueryUnpackAlignment()/QueryPackAlignment() are declared there
// (shared common/layer_interface code) but DEFINED in this file's .cpp, not this header - see
// that header's own comment for why (recorder-only, needs the real driver dispatch table via
// glGetIntegerv, same technique as QueryBoundBufferQuiet() in clientMemoryCapture.cpp and
// ResolveBoundBuffer() in mapTrackingService.cpp).

} // namespace gits::opengl2
