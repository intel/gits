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
// format/type/dimensions shape), i.e. gl.xml's COMPSIZE(format,type,width,height[,depth])
// pseudo-length - see pointer_metadata.py's LengthSourceKind.PIXEL_TRANSFER_SIZE. GL itself does not expose
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
// GL_(UN)PACK_ROW_LENGTH/GL_(UN)PACK_SKIP_ROWS/GL_(UN)PACK_SKIP_PIXELS are incorporated
// directly into the byte-range arithmetic below, exactly per the GL pixel-store spec's own
// addressing formula - see `rowLength`/`skipRows`/`skipPixels`'s own comment further down for
// the full derivation and the live-testing verification behind it (this applies uniformly to
// every pixel-transfer call, 1D/2D/3D alike, not just a 3D-shaped one: a sub-rectangle upload
// via glTexSubImage2D whose source data is embedded in a larger, non-default-ROW_LENGTH image
// - a very common real-world pattern - is the concrete motivating case). Unlike
// GL_(UN)PACK_IMAGE_HEIGHT/GL_(UN)PACK_SKIP_IMAGES below (still gated - see that pair's own
// comment for why), these three are never a source of "size unknown" on their own; only a
// negative value (which a spec-conforming driver can never actually produce - see
// pixelTransferSize.cpp's own comment) still gates capture off, as a defensive can't-happen
// guard rather than a real, expected-in-practice limitation.
//
// Returns 0 for a non-positive `width`/`height`/`depth` (a genuine, real zero-byte transfer -
// see this function's own comment below), an unrecognized or invalid `format`/`type`
// combination (see ComputeTexelByteSize()), non-default GL_(UN)PACK_IMAGE_HEIGHT/GL_(UN)PACK_SKIP_IMAGES state on a 3D
// transfer (still gated - see below), a negative GL_(UN)PACK_ROW_LENGTH/SKIP_ROWS/SKIP_PIXELS
// (a can't-happen defensive guard - see above), or an intermediate/final size computation that
// would overflow uint64_t - rather than guessing, or silently returning a wrapped-around value
// that no longer reflects the real transfer size.
//
// A caller must NOT treat every 0 return the same way.
// Non-positive `width`/`height`/`depth` is the ONE case where 0 genuinely means "this call
// transfers zero real bytes, no matter what" - GL itself will not read or write a single byte
// through the data pointer for such a call, so it remains perfectly safe (and, for a command
// with real state-mutating side effects beyond the pixel copy itself - e.g. glTexImage2D
// resizing/reformatting a texture level even at width=0 - necessary for replay fidelity) to
// still issue the real driver call with an empty/null-equivalent buffer. Every OTHER 0-
// returning case above means the size genuinely COULD NOT be determined, and is generally
// nonzero at replay time - issuing the real driver call there against an empty captured
// buffer risks exactly the out-of-bounds read/write, heap corruption, or crash this function
// was written to avoid in the first place. See PixelTransferSizeIsUnknown() immediately below
// for the one, shared, correct way every caller must tell these two cases apart - do not
// re-derive this distinction ad hoc at a call site.
//
// Returns uint64_t, not a 32-bit type: width/height/depth/bytesPerPixel's product can
// legitimately approach or exceed 4 GiB for a large-enough transfer, and the sole caller
// (ArrayArgument<T>'s own templated, size_t-based constructor/Capture()) already accepts any
// integer width, so there is no reason to narrow this function's own result before returning
// it - see pixelTransferSize.cpp's own comment on why the arithmetic itself is done in
// uint64_t too, rather than merely widening the final cast.
//
// `depth` defaults to 1 for the plain 2D shape (glTexImage2D/glReadPixels/glTexSubImage2D:
// COMPSIZE(format,type,width,height), no third dimension at all in gl.xml) - a real 3D
// transfer (glTexImage3D/glTexSubImage3D: COMPSIZE(format,type,width,height,depth)) passes its
// own `depth` sibling instead. Per the GL pixel-store spec's own multi-image formula, each
// image below the last is a full `height` rows of full `rowStride` each (the next image starts
// immediately after, so even that image's own last row needs full padding); only the very
// last row of the very last image needs no trailing padding past its own pixel data. That
// collapses to the same closed form as the plain 2D case with `height` replaced by
// `depth * height` - see pixelTransferSize.cpp's own comment at the one line that changed.
//
// `imageHeight`/`skipImages` (default 0/0, GL's own defaults) are GL_UNPACK_IMAGE_HEIGHT/
// GL_UNPACK_SKIP_IMAGES (or, for an OUT-direction transfer, their GL_PACK_* counterparts) -
// see QueryUnpackImageHeight()/QueryUnpackSkipImages()/QueryPackImageHeight()/
// QueryPackSkipImages() below. Unlike ROW_LENGTH/SKIP_ROWS/SKIP_PIXELS above (a bounded,
// documented "a few bytes short/long" limitation), an app that sets a non-default
// GL_UNPACK_IMAGE_HEIGHT (which changes how many rows each layer of a 3D transfer actually
// occupies in client memory) or a non-zero GL_UNPACK_SKIP_IMAGES (which shifts every captured
// layer's real offset later by that many full layers) before a 3D transfer would make the
// plain `depth * height` formula UNDER-count by a multiple of a full image's worth of rows -
// not a few bytes, but potentially the entire remainder of the buffer. Rather than guess at
// incorporating these into the byte-range arithmetic
// without a way to verify the result against a real driver, this function takes the safe,
// conservative option instead - if either is non-default for a call that actually has a third
// dimension (i.e. a real caller passed a non-zero `imageHeight`/`skipImages` argument; see
// commandsAuto.h.mako/recorderWrappersAuto.cpp.mako, which only ever do this for a command with
// its own `depth` param), this function returns 0 (this header's own existing "size unknown,
// capture degrades gracefully" sentinel - see this comment's paragraph on that above) instead
// of a wrong, under-counted byte range, and logs a single warning per unique `commandName` (via
// LOG_WARNING - see pixelTransferSize.cpp) so the gap is visible rather than silent. A 2D-or-1D
// caller never passes a non-zero `imageHeight`/`skipImages` at all (GL_UNPACK_IMAGE_HEIGHT/
// GL_UNPACK_SKIP_IMAGES have no effect on a 2D-or-1D transfer regardless of their live value),
// so this can never fire for one - only a genuine 3D transfer with actually non-default
// pixel-store state loses capture, not every 3D transfer unconditionally.
//
// `rowLength`/`skipRows`/`skipPixels` (default 0/0/0, GL's own defaults) are
// GL_UNPACK_ROW_LENGTH/GL_UNPACK_SKIP_ROWS/GL_UNPACK_SKIP_PIXELS (or, for an OUT-direction
// transfer, their GL_PACK_* counterparts) - see QueryUnpackRowLength()/QueryUnpackSkipRows()/
// QueryUnpackSkipPixels()/QueryPackRowLength()/QueryPackSkipRows()/QueryPackSkipPixels()
// below. Unlike IMAGE_HEIGHT/SKIP_IMAGES above (which GL ignores entirely for
// a transfer with no third dimension), ROW_LENGTH/SKIP_ROWS affect every transfer with more
// than one real row to advance between/skip (2D and 3D alike) - but, per the GL pixel-store
// spec, NEITHER has any effect on a genuine 1D transfer (glTexImage1D/glTexSubImage1D: no
// `height` sibling at all - there is only ever one row, so there is no second row to stride
// to and no row to skip over). SKIP_PIXELS is the one exception that still applies
// unconditionally, 1D included (it shifts where within that single row's own client buffer the
// real pixel data starts). This function itself stays dimension-agnostic and trusts whatever
// `rowLength`/`skipRows` it is handed - it is each Mako call site's own responsibility
// (commandsAuto.h.mako/recorderWrappersAuto.cpp.mako) to pass literal 0s for both, rather than
// the live-queried GL_(UN)PACK_ROW_LENGTH/SKIP_ROWS state, whenever the command being sized has
// no `height` param (see those templates' own conditionals, keyed on the same
// `pixel_height_param` presence/absence PIXEL_TRANSFER_SIZE already uses to gate
// IMAGE_HEIGHT/SKIP_IMAGES above) - otherwise a legitimate, nonzero GL_(UN)PACK_SKIP_ROWS the
// application set for some OTHER, unrelated 2D/3D transfer elsewhere would still be live at a
// 1D call, and this function would spuriously add skipRows * rowStride bytes to the computed
// size that the driver itself never reads for that 1D call - an out-of-bounds host read past
// the application's real allocation, since the caller's eager buffer copy walks exactly that
// many bytes regardless of whether GL would ever look at them.
//
// For every transfer where ROW_LENGTH/SKIP_ROWS genuinely do apply (2D and 3D), the concrete
// motivating case is: a 1x2 RGBA/UNSIGNED_BYTE transfer with ROW_LENGTH=1024
// actually touches 4,100 bytes of real client memory (1024 bytes for the row ROW_LENGTH
// describes, plus 4 bytes for the second row's own real pixel data), which is exactly what
// this function computes: the GL pixel-store spec's own addressing formula for the memory
// offset of pixel (i, j) in an unpacked image is
//     rowStride = roundUpToAlignment(bytesPerPixel * (ROW_LENGTH > 0 ? ROW_LENGTH : width))
//     byteOffset(row) = (SKIP_ROWS + row) * rowStride + SKIP_PIXELS * bytesPerPixel
// - i.e. GL_UNPACK_ROW_LENGTH only ever changes the STRIDE used to advance between rows (never
// how many real bytes the final row itself contributes), and GL_UNPACK_SKIP_ROWS/SKIP_PIXELS
// only ever shift where within the client-supplied buffer reading starts (never how many bytes
// are read). Since the exact same pixel-store state this function read at capture time is
// itself captured (as an ordinary glPixelStorei call) and replayed before the pixel-transfer
// call it applies to, a captured buffer covering every byte from the client pointer's offset 0
// up through the last real byte of the final row - i.e. this formula evaluated at
// row = SKIP_ROWS + depth*height - 1, plus `width` more bytes of that row's own real pixel
// data - is both necessary and sufficient for replay to read back byte-for-byte identical
// data through the very same addressing the original driver used; there is no reconciliation
// against a different, replay-time buffer layout to get wrong. Verified against a live capture/
// replay round-trip with a non-default GL_UNPACK_ROW_LENGTH sub-rectangle upload (see this
// change's own commit message for the exact repro and result) rather than derived from the
// spec text alone. A negative value for any of the three (see this header's own top comment,
// and pixelTransferSize.cpp's own comment) is the one remaining case that still gates capture
// off, logging its own single warning per unique `commandName` (see pixelTransferSize.cpp) -
// distinct from, and independent of, the IMAGE_HEIGHT/SKIP_IMAGES gate above.
//
// `commandName` (default nullptr) is used only for the two diagnostics above - never part of
// the size computation itself. Given the previous paragraph, a caller now risks the gated case
// on every call (not just a 3D one), so every enabled pixel-transfer call site
// (commandsAuto.h.mako/recorderWrappersAuto.cpp.mako) always passes it explicitly; omitting it
// is only meaningful for a hypothetical caller outside the generated code path that can
// independently guarantee every one of GL_(UN)PACK_ROW_LENGTH/SKIP_ROWS/SKIP_PIXELS/
// IMAGE_HEIGHT/SKIP_IMAGES is already at its GL default.
uint64_t ComputePixelTransferByteSize(GLenum format,
                                      GLenum type,
                                      GLsizei width,
                                      GLsizei height,
                                      GLint alignment,
                                      GLsizei depth = 1,
                                      GLint imageHeight = 0,
                                      GLint skipImages = 0,
                                      GLint rowLength = 0,
                                      GLint skipRows = 0,
                                      GLint skipPixels = 0,
                                      const char* commandName = nullptr);

// The required companion predicate to
// ComputePixelTransferByteSize() above: distinguishes its one truly-safe 0 return (a genuine,
// real zero-byte transfer - non-positive `width`/`height`/`depth`) from every other 0 return
// (untracked pixel-store state, an unrecognized `format`/`type` combination, or an overflowing
// size computation), none of which mean "call away with an empty buffer, GL will not touch
// it" - see that function's own header comment above for the full rationale.
//
// Takes the EXACT same parameter list (names, order, types, defaults) as
// ComputePixelTransferByteSize() above, by design - every call site
// (commandsAuto.h.mako/recorderWrappersAuto.cpp.mako) shares one argument-list expression for
// both calls, so the two can never silently drift out of sync with each other at a given call
// site. Implemented purely in terms of that same function (see pixelTransferSize.cpp) - this
// is not a second, independent size/gating computation to keep in sync by hand.
//
// Returns true when the captured size is unknown/unreplayable (every case above OTHER than
// non-positive `width`/`height`/`depth`) - commandRunnersAuto.cpp.mako's Run() checks this on
// every PIXEL_TRANSFER_SIZE-shaped parameter (see generator_helpers.py's
// pointer_needs_pixel_transfer_unknown_flag()) before issuing the real driver call, and skips
// the call entirely (logging a degraded-fidelity warning) whenever any of them answers true,
// rather than replaying against a buffer whose captured size does not match what the driver
// will actually read/write. Returns false for a genuine zero-byte transfer (the real driver
// call is safe, and for some commands necessary for replay fidelity - see
// ComputePixelTransferByteSize()'s own header comment) and for a genuine, successfully
// computed nonzero size alike - both are "known", just possibly zero.
bool PixelTransferSizeIsUnknown(GLenum format,
                                GLenum type,
                                GLsizei width,
                                GLsizei height,
                                GLint alignment,
                                GLsizei depth = 1,
                                GLint imageHeight = 0,
                                GLint skipImages = 0,
                                GLint rowLength = 0,
                                GLint skipRows = 0,
                                GLint skipPixels = 0,
                                const char* commandName = nullptr);

// Queries the real, current GL_UNPACK_ALIGNMENT/GL_PACK_ALIGNMENT via glGetIntegerv, for use as
// ComputePixelTransferByteSize()'s `alignment` argument - see that function's own comment for
// which of the two applies to a given command's direction. Forward-declared here (rather than
// included via a recorder-only header) and defined in
// OpenGL2/recorder/clientMemoryCapture.cpp, which has access to the real driver dispatch table
// these need - this header (and the two call sites that use these, both effectively
// recorder-only despite living in code compiled into the shared common/layer_interface library
// too, since only the recorder's own generated code ever actually invokes the constructor/
// Capture() overloads that call these - see commandsAuto.h.mako/recorderWrappersAuto.cpp.mako)
// must stay includable/compilable from layer_interface's own translation units (e.g.
// commandCodersAuto.cpp), which have no access to OpenGL2/recorder/include.
GLint QueryUnpackAlignment();
GLint QueryPackAlignment();

// Queries the real, current GL_UNPACK_IMAGE_HEIGHT/GL_UNPACK_SKIP_IMAGES (or their GL_PACK_*
// counterparts) via glGetIntegerv, for use as ComputePixelTransferByteSize()'s `imageHeight`/
// `skipImages` arguments - see that function's own comment for why these matter and when a
// caller must pass them. Same forward-declared-here/defined-in-clientMemoryCapture.cpp
// convention as QueryUnpackAlignment()/QueryPackAlignment() above, for the same reason. Only
// ever called by a Mako-emitted call site for a command with an actual `depth` dimension (see
// commandsAuto.h.mako/recorderWrappersAuto.cpp.mako) - a 2D-or-1D command has no reason to
// query either, since GL ignores both for a transfer with no third dimension.
GLint QueryUnpackImageHeight();
GLint QueryUnpackSkipImages();
GLint QueryPackImageHeight();
GLint QueryPackSkipImages();

// Queries the real, current GL_UNPACK_ROW_LENGTH/GL_UNPACK_SKIP_ROWS/
// GL_UNPACK_SKIP_PIXELS (or their GL_PACK_* counterparts) via glGetIntegerv, for use as
// ComputePixelTransferByteSize()'s `rowLength`/`skipRows`/`skipPixels` arguments - see that
// function's own comment for why these matter and why, unlike QueryUnpackImageHeight() et al.
// above, every enabled pixel-transfer call site queries and passes all three unconditionally
// (1D/2D/3D alike), not just a command with a `depth` param. Same forward-declared-here/
// defined-in-clientMemoryCapture.cpp convention as every other Query*() in this header, for
// the same reason (real driver dispatch table access).
GLint QueryUnpackRowLength();
GLint QueryUnpackSkipRows();
GLint QueryUnpackSkipPixels();
GLint QueryPackRowLength();
GLint QueryPackSkipRows();
GLint QueryPackSkipPixels();

// Computes the element count of glClearBufferfv/glClearBufferiv/glClearBufferuiv's `value`
// array (gl.xml's COMPSIZE(buffer) pseudo-length) - see pointer_metadata.py's
// LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT. 4 for GL_COLOR (RGBA), 1 for GL_DEPTH/
// GL_STENCIL. Returns 1 (a safe, non-zero default) for any other `buffer` value rather than 0,
// since unlike a pixel-transfer format/type combination there is no meaningful "unrecognized"
// case in practice - `buffer` only ever takes one of these three values for a well-formed call.
uint32_t ComputeClearBufferValueCount(GLenum buffer);

// Byte size of ONE texel described by `format`/`type` - gl.xml's COMPSIZE(format,type) on
// glClearTexImage/glClearTexSubImage/glClearBufferData/... (a single clear value; pixel-store
// state does not apply). See pointer_metadata.py's LengthSourceKind.TEXEL_BYTE_SIZE. Returns 0
// for an unrecognized or invalid format/type combination (such as a packed type paired with a
// format it cannot describe, GL_STENCIL_INDEX with a floating-point type, or
// GL_DEPTH_COMPONENT with a signed integer type), which the driver rejects without reading the
// clear value, so it is not captured.
uint32_t ComputeTexelByteSize(GLenum format, GLenum type);

// Element count of an audited COMPSIZE(pname) setter's value array (glLightfv, glFogfv,
// glTexEnvfv, ...) - see pointer_metadata.py's LengthSourceKind.PNAME_VALUE_COUNT. Defined in
// the generated pnameValueCountsAuto.cpp from codegen/pname_value_counts.py.
uint32_t ComputePnameValueCount(GLenum pname);

// Byte size of glCallLists' `lists` array: `n` list offsets of the ListNameType `type` - see
// pointer_metadata.py's LengthSourceKind.LIST_NAME_ARRAY_BYTE_SIZE. Returns 0 for a negative `n`
// or an unrecognized `type`, which the driver rejects without reading the array. 64-bit, since
// `n` 4-byte elements can exceed 4 GiB.
uint64_t ComputeListNameArrayByteSize(GLsizei n, GLenum type);

// Bytes the driver reads for `count` indirect draw commands of `commandSize` bytes each, `stride`
// bytes apart (0: tightly packed) - see pointer_metadata.py's
// LengthSourceKind.INDIRECT_COMMANDS_BYTE_SIZE. The last command ends at its own size, not at the
// next stride, so an application allocating exactly that much is never over-read. Returns 0 for
// a non-positive `count`, or a negative `stride` or one that is not a multiple of four, which the
// driver rejects without reading.
uint64_t ComputeIndirectCommandsByteSize(GLsizei count, GLsizei stride, uint32_t commandSize);

// Size in bytes of one element index of `type` (GL_UNSIGNED_BYTE/SHORT/INT), or 0 for any other
// value, which the driver rejects.
uint32_t ElementIndexTypeSize(GLenum type);

// Bytes the driver reads for `count` element indices of `type` - see pointer_metadata.py's
// LengthSourceKind.ELEMENT_INDICES_BYTE_SIZE. Returns 0 for a non-positive `count` or an
// unrecognized `type`, which the driver rejects without reading.
uint64_t ComputeElementIndicesByteSize(GLsizei count, GLenum type);

// Distinguishes the two command families that share ComputeTexOrSamplerParameterValueCount()
// below - see that function's own comment for why this matters.
// GL_TEXTURE_SWIZZLE_RGBA (and its single-channel GL_TEXTURE_SWIZZLE_R/G/B/A siblings) is a
// TextureParameterName-only pname: verified directly against
// third_party/OpenGL-Registry/xml/gl.xml, glTexParameterfv/iv/Iiv/Iuiv's own `pname` parameter
// all carry group="TextureParameterName" (identical across all four Tex- forms, "I" integer
// forms included), while glSamplerParameterfv/iv/Iiv/Iuiv's own `pname` carries
// group="SamplerParameterF" (fv only) or group="SamplerParameterI" (iv/Iiv/Iuiv) - neither
// Sampler group's own enum membership in gl.xml includes GL_TEXTURE_SWIZZLE_RGBA/R/G/B/A
// anywhere in the registry (texture swizzle is per-texture-OBJECT state; a sampler object has
// no swizzle state at all to set). GL_TEXTURE_BORDER_COLOR is unaffected by this distinction -
// it is a legal 4-element pname for every one of the 8 commands in both families (its own
// gl.xml <enum> tag carries "TextureParameterName" among its groups, and the real GL/GLES
// reference pages document it as valid for every glSamplerParameter{,I}{f,i,ui}v form too,
// despite gl.xml's own "SamplerParameterI" group tag not listing it explicitly - a known gap
// in that particular group's tagging, not evidence border color is actually illegal there).
enum class TexOrSamplerParameterCommand {
  kTexParameter,     // glTexParameter{f,i,Ii,Iui}v - GL_TEXTURE_SWIZZLE_RGBA is a legal,
                     // 4-element pname for every one of these four forms.
  kSamplerParameter, // glSamplerParameter{f,i,Ii,Iui}v - GL_TEXTURE_SWIZZLE_RGBA is not a
                     // legal pname for any of these four forms at all (see this enum's own
                     // comment above) - a real app call using it here is itself a
                     // GL_INVALID_ENUM client error the driver must reject, but this helper is
                     // invoked BEFORE that real driver call happens (to size the eager
                     // capture-time read of the caller-supplied `param` array), so it must
                     // still answer 1, never 4, for this family/pname combination rather than
                     // relying on the driver to catch the malformed call first - avoids
                     // the out-of-bounds read this would otherwise risk for this
                     // family/pname combination.
};

// Computes the element count of glTexParameterfv/glTexParameteriv/glSamplerParameterfv's
// `params`/`param` array (gl.xml's COMPSIZE(pname) pseudo-length) - see pointer_metadata.py's
// LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT. 4 for GL_TEXTURE_BORDER_COLOR (legal for
// every Tex-/Sampler- form - see TexOrSamplerParameterCommand's own comment) regardless of
// `command`; 4 for GL_TEXTURE_SWIZZLE_RGBA and GL_TEXTURE_CROP_RECT_OES only when `command` is
// kTexParameter (1 when it is kSamplerParameter - see TexOrSamplerParameterCommand's own
// comment, and this function's own case comments, for why); 1 for every other legal pname.
// Returns 1 (a safe, non-zero default matching ComputeClearBufferValueCount()'s own fallback)
// for any other `pname` value rather than 0.
uint32_t ComputeTexOrSamplerParameterValueCount(GLenum pname, TexOrSamplerParameterCommand command);

// Computes the element count of glPatchParameterfv's `values` array (gl.xml's
// COMPSIZE(pname) pseudo-length) - see pointer_metadata.py's
// LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT. 4 for GL_PATCH_DEFAULT_OUTER_LEVEL, 2 for
// GL_PATCH_DEFAULT_INNER_LEVEL - the only two legal pnames. Returns 1 (a safe, non-zero
// default) for any other `pname` value rather than 0.
uint32_t ComputePatchParameterValueCount(GLenum pname);

// Computes the element count of glMaterialfv's `params` array (gl.xml's COMPSIZE(pname)
// pseudo-length) - see pointer_metadata.py's LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT.
// 4 for GL_AMBIENT/GL_DIFFUSE/GL_SPECULAR/GL_EMISSION/GL_AMBIENT_AND_DIFFUSE, 1 for
// GL_SHININESS, 3 for GL_COLOR_INDEXES - the only 7 legal pnames (verified exhaustively
// against every third_party/OpenGL-Registry/xml/gl.xml <enum> tag carrying
// group="MaterialParameter"). Returns 1 (a safe, non-zero default matching
// ComputeClearBufferValueCount()'s own fallback convention) for any other `pname` value.
uint32_t ComputeMaterialParameterValueCount(GLenum pname);

// Computes the element count of glPointParameterfv's `params` array (gl.xml's
// COMPSIZE(pname) pseudo-length) - see pointer_metadata.py's
// LengthSourceKind.POINT_PARAMETER_VALUE_COUNT. 3 for GL_POINT_DISTANCE_ATTENUATION, 1 for
// GL_POINT_SIZE_MIN/GL_POINT_SIZE_MAX/GL_POINT_FADE_THRESHOLD_SIZE - the only 4 legal pnames
// (verified exhaustively against every third_party/OpenGL-Registry/xml/gl.xml <enum> tag
// carrying group="PointParameterNameARB"). Returns 1 (a safe, non-zero default) for any other
// `pname` value.
uint32_t ComputePointParameterValueCount(GLenum pname);

// Computes the element count of glGetVertexAttribiv's `params` array - see pointer_metadata.py's
// LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT. Unlike every other
// COMPSIZE(pname)-shaped helper above, this is NOT a gl.xml COMPSIZE annotation at all - gl.xml
// gives glGetVertexAttribiv's own `params` a flat, unconditional len="4" (see that
// LengthSourceKind's own comment for why this registry annotation is actually wrong for the
// overwhelming majority of this command's legal pnames). Returns 4 only for
// GL_CURRENT_VERTEX_ATTRIB (0x8626) - the generic vertex attribute's own x/y/z/w, the sole
// member of gl.xml's VertexAttribPropertyARB enum group (the exact group glGetVertexAttribiv's
// own `pname` <param> carries) that is not a single scalar array-state property. Returns 1 for
// every other legal VertexAttribPropertyARB member (GL_VERTEX_ATTRIB_ARRAY_ENABLED/SIZE/
// STRIDE/TYPE/NORMALIZED/INTEGER/LONG/DIVISOR/BUFFER_BINDING, GL_VERTEX_ATTRIB_BINDING,
// GL_VERTEX_ATTRIB_RELATIVE_OFFSET - verified exhaustively against every
// third_party/OpenGL-Registry/xml/gl.xml <enum> tag carrying group="VertexAttribPropertyARB"),
// and, matching ComputeClearBufferValueCount()/ComputeTexOrSamplerParameterValueCount()'s own
// fallback convention, for any other/unrecognized `pname` value too (a safe, non-zero default
// rather than 0 - an out-of-bounds read one element short is a much smaller residual risk than
// a spurious "size unknown, skip capture" for a value this function simply doesn't recognize
// yet).
uint32_t ComputeVertexAttribParameterValueCount(GLenum pname);

} // namespace gits::opengl2
