// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pixelTransferSize.h"

#include "log.h"

#include <limits>
#include <mutex>
#include <string>
#include <unordered_set>

namespace gits::opengl2 {

namespace {

// Warn-once state for the ComputePixelTransferByteSize() gating below -
// mirrors OpenGL2/recorder/mapTrackingWrappers.cpp's own warn-once conventions (e.g.
// g_WarnedPixelBufferGuardCommands), kept local to this translation unit since this is the
// only place that needs it.
std::mutex g_PixelStoreWarningMutex;
std::unordered_set<std::string> g_WarnedPixelStoreCommands;

// Logs a single warning per unique `commandName` when ComputePixelTransferByteSize() gates a
// 3D transfer's capture off because of non-default GL_(UN)PACK_IMAGE_HEIGHT/
// GL_(UN)PACK_SKIP_IMAGES state - see that function's own header comment for
// why this is the safe choice rather than guessing at the full formula.
void WarnNonDefaultImagePixelStoreState(const char* commandName) {
  std::lock_guard<std::mutex> lock(g_PixelStoreWarningMutex);
  const std::string key = commandName != nullptr ? commandName : "<unknown pixel-transfer call>";
  if (g_WarnedPixelStoreCommands.insert(key).second) {
    LOG_WARNING << key
                << ": non-default GL_(UN)PACK_IMAGE_HEIGHT/GL_(UN)PACK_SKIP_IMAGES pixel-store "
                   "state on a 3D pixel-transfer call - this recorder does not track those two "
                   "parameters (see pixelTransferSize.h's own comment), and computing this "
                   "call's captured byte range without them would silently under-count it, "
                   "risking an out-of-bounds read/write on replay. Skipping capture for this "
                   "call (0 bytes) instead of guessing at the wrong size.";
  }
}

// Warn-once state for the row/skip-pixel gate below, kept separate from
// g_PixelStoreWarningMutex/g_WarnedPixelStoreCommands above (and its own distinct message) so
// the two gates remain independently diagnosable from the log alone - see
// ComputePixelTransferByteSize()'s own header comment (pixelTransferSize.h) for why they are
// two separate checks rather than one combined one.
std::mutex g_RowPixelStoreWarningMutex;
std::unordered_set<std::string> g_WarnedRowPixelStoreCommands;

void WarnNonDefaultRowPixelStoreState(const char* commandName) {
  std::lock_guard<std::mutex> lock(g_RowPixelStoreWarningMutex);
  const std::string key = commandName != nullptr ? commandName : "<unknown pixel-transfer call>";
  if (g_WarnedRowPixelStoreCommands.insert(key).second) {
    LOG_WARNING << key
                << ": non-default GL_(UN)PACK_ROW_LENGTH/GL_(UN)PACK_SKIP_ROWS/"
                   "GL_(UN)PACK_SKIP_PIXELS pixel-store state on a pixel-transfer call - this "
                   "recorder does not track these three parameters (see pixelTransferSize.h's "
                   "own comment), and computing this call's captured byte range without them "
                   "would silently under- or over-count it (unlike GL_(UN)PACK_IMAGE_HEIGHT/"
                   "GL_(UN)PACK_SKIP_IMAGES, which only affect a 3D transfer, these three "
                   "affect every pixel transfer regardless of dimensionality), risking an "
                   "out-of-bounds read/write on replay. Skipping capture for this call (0 "
                   "bytes) instead of guessing at the wrong size.";
  }
}

// Returns true if `a * b` would overflow uint64_t, without computing the (potentially wrapped)
// product itself: a naive uint64_t multiply/add chain
// does not trap on overflow, it silently wraps modulo 2^64 exactly like the uint32_t case this
// function was originally widened to fix, just at a much larger (but still very much reachable
// - see ComputePixelTransferByteSize's own comment) threshold. Written as a portable pre-check
// (divide-and-compare) rather than a compiler intrinsic (__builtin_mul_overflow/_mul128) so it
// behaves identically under MSVC today and a future Linux/GCC-or-Clang toolchain, with no
// __has_builtin/_MSC_VER branching.
bool MultiplyOverflows(uint64_t a, uint64_t b) {
  return a != 0 && b > (std::numeric_limits<uint64_t>::max)() / a;
}

// Packed types encode every component of one pixel into a single value; the byte size below is
// already the *whole pixel's* size, independent of `format`. Values verified against
// third_party/OpenGL-Registry/xml/gl.xml.
uint32_t PackedTypeByteSize(GLenum type) {
  switch (type) {
  case 0x8032: // GL_UNSIGNED_BYTE_3_3_2
  case 0x8362: // GL_UNSIGNED_BYTE_2_3_3_REV
    return 1;
  case 0x8363: // GL_UNSIGNED_SHORT_5_6_5
  case 0x8364: // GL_UNSIGNED_SHORT_5_6_5_REV
  case 0x8033: // GL_UNSIGNED_SHORT_4_4_4_4
  case 0x8365: // GL_UNSIGNED_SHORT_4_4_4_4_REV
  case 0x8034: // GL_UNSIGNED_SHORT_5_5_5_1
  case 0x8366: // GL_UNSIGNED_SHORT_1_5_5_5_REV
    return 2;
  case 0x8035: // GL_UNSIGNED_INT_8_8_8_8
  case 0x8367: // GL_UNSIGNED_INT_8_8_8_8_REV
  case 0x8036: // GL_UNSIGNED_INT_10_10_10_2
  case 0x8368: // GL_UNSIGNED_INT_2_10_10_10_REV
  case 0x84FA: // GL_UNSIGNED_INT_24_8
  case 0x8C3E: // GL_UNSIGNED_INT_5_9_9_9_REV
  case 0x8C3B: // GL_UNSIGNED_INT_10F_11F_11F_REV
    return 4;
  case 0x8DAD: // GL_FLOAT_32_UNSIGNED_INT_24_8_REV
    return 8;
  default:
    return 0; // Not a packed type.
  }
}

// Bytes per component for the plain (non-packed) pixel transfer types.
uint32_t ComponentByteSize(GLenum type) {
  switch (type) {
  case 0x1400: // GL_BYTE
  case 0x1401: // GL_UNSIGNED_BYTE
    return 1;
  case 0x1402: // GL_SHORT
  case 0x1403: // GL_UNSIGNED_SHORT
  case 0x140B: // GL_HALF_FLOAT
    return 2;
  case 0x1404: // GL_INT
  case 0x1405: // GL_UNSIGNED_INT
  case 0x1406: // GL_FLOAT
    return 4;
  default:
    return 0; // Unrecognized type.
  }
}

// Number of components (channels) the pixel `format` describes; multiplied by
// ComponentByteSize(type) to get bytes per pixel for non-packed types.
uint32_t ComponentCount(GLenum format) {
  switch (format) {
  case 0x1901: // GL_STENCIL_INDEX
  case 0x1902: // GL_DEPTH_COMPONENT
  case 0x1903: // GL_RED
  case 0x1904: // GL_GREEN
  case 0x1905: // GL_BLUE
  case 0x1906: // GL_ALPHA
  case 0x1909: // GL_LUMINANCE
  case 0x8D94: // GL_RED_INTEGER
  case 0x8D95: // GL_GREEN_INTEGER
  case 0x8D96: // GL_BLUE_INTEGER
    return 1;
  case 0x190A: // GL_LUMINANCE_ALPHA
  case 0x8227: // GL_RG
  case 0x8228: // GL_RG_INTEGER
  case 0x84F9: // GL_DEPTH_STENCIL
    return 2;
  case 0x1907: // GL_RGB
  case 0x80E0: // GL_BGR
  case 0x8D98: // GL_RGB_INTEGER
  case 0x8D9A: // GL_BGR_INTEGER
    return 3;
  case 0x1908: // GL_RGBA
  case 0x80E1: // GL_BGRA
  case 0x8D99: // GL_RGBA_INTEGER
  case 0x8D9B: // GL_BGRA_INTEGER
    return 4;
  default:
    return 0; // Unrecognized format.
  }
}

} // namespace

uint64_t ComputePixelTransferByteSize(GLenum format,
                                      GLenum type,
                                      GLsizei width,
                                      GLsizei height,
                                      GLint alignment,
                                      GLsizei depth,
                                      GLint imageHeight,
                                      GLint skipImages,
                                      GLint rowLength,
                                      GLint skipRows,
                                      GLint skipPixels,
                                      const char* commandName) {
  if (width <= 0 || height <= 0 || depth <= 0) {
    return 0;
  }
  // GL_(UN)PACK_IMAGE_HEIGHT/GL_(UN)PACK_SKIP_IMAGES gating - see this function's own header
  // comment for the full rationale. Only ever non-zero when the caller
  // (commandsAuto.h.mako/recorderWrappersAuto.cpp.mako) queried a genuinely live, non-default
  // value for a command that actually has a `depth` dimension - a 2D-or-1D caller never passes
  // anything but the defaults (0, 0), so this can never fire for one.
  if (imageHeight != 0 || skipImages != 0) {
    WarnNonDefaultImagePixelStoreState(commandName);
    return 0;
  }
  // GL_(UN)PACK_ROW_LENGTH/GL_(UN)PACK_SKIP_ROWS/GL_(UN)PACK_SKIP_PIXELS gating -
  // see this function's own header comment (pixelTransferSize.h) for the full rationale.
  // Unlike the IMAGE_HEIGHT/SKIP_IMAGES gate above, this one is queried and checked
  // UNCONDITIONALLY by every Mako call site regardless of dimensionality (these three affect
  // every pixel transfer, not just a 3D one) - so this can fire for a 1D or 2D call too, not
  // only a 3D one.
  if (rowLength != 0 || skipRows != 0 || skipPixels != 0) {
    WarnNonDefaultRowPixelStoreState(commandName);
    return 0;
  }
  // GL only ever allows 1/2/4/8 here (see this function's own header comment) - a caller-
  // supplied value outside that set (should never happen from a real glGetIntegerv query)
  // falls back to GL's own default rather than feeding a nonsensical value into the rounding
  // arithmetic below.
  if (alignment != 1 && alignment != 2 && alignment != 4 && alignment != 8) {
    alignment = 4;
  }

  uint32_t bytesPerPixel = PackedTypeByteSize(type);
  if (bytesPerPixel == 0) {
    const uint32_t components = ComponentCount(format);
    const uint32_t componentBytes = ComponentByteSize(type);
    if (components == 0 || componentBytes == 0) {
      return 0; // Unrecognized format/type combination - see this header's own comment.
    }
    bytesPerPixel = components * componentBytes;
  }

  // Row stride, rounded up to the real, current GL_PACK_ALIGNMENT/GL_UNPACK_ALIGNMENT the
  // caller queried and passed in - see this function's own header comment. Naively
  // multiplying width*height*bytesPerPixel with no
  // row padding at all is wrong even at the default alignment of 4 whenever
  // width*bytesPerPixel isn't itself a multiple of `alignment` (e.g. an odd-width GL_RGB/
  // GL_UNSIGNED_BYTE image, 3 bytes/pixel, at the default alignment) - it both omits real
  // padding bytes the driver's own buffer layout includes between
  // rows and, for a multi-row image, ends up computing a different (usually smaller, sometimes
  // larger) total than the driver's actual buffer footprint. The last row does not need
  // trailing padding past its own pixel data (matching the GL spec's own image-size formula),
  // so total size is (rowStride * (height - 1)) + unpaddedRowBytes, not rowStride * height.
  //
  // Computed entirely in uint64_t, and returned as uint64_t (see this function's own header
  // declaration): every multiplicand here (bytesPerPixel <= 16, width/height each up to
  // INT32_MAX) can make an intermediate or the final result overflow a 32-bit accumulator well
  // before reaching any GL-legal image size - e.g. a 32768x32768 RGBA/UNSIGNED_BYTE transfer
  // computes exactly 4 GiB, which would wrap to *exactly 0* in a uint32_t accumulator (silently
  // indistinguishable from an unrecognized-format 0 above), and other large-but-not-exactly-
  // 4-GiB-aligned dimensions would wrap to some smaller nonzero value instead - either way
  // silently producing an incomplete/wrong-size capture rather than an obvious failure. There is
  // no reason to narrow back down to 32 bits anywhere in this path: the only caller
  // (commandsAuto.h.mako/recorderWrappersAuto.cpp.mako, feeding straight into
  // ArrayArgument<T>'s own templated, size_t-based constructor/Capture()) already accepts any
  // integer width, and neither the wire-format coder that eventually serializes the resulting
  // ArrayArgument<uint8_t> nor the stream writer beneath it caps a record's byte count at 32
  // bits (see common/stream/src/streamWriter.cpp's own uint64_t size field) - so widening this
  // function's own arithmetic all the way through to its return type (mirroring the
  // other size/count widenings - e.g. ArrayArgument<T>/HandleArrayArgument<T>'s own SizeT,
  // MappedDataMetaCommand's coding path) removes the *32-bit* overflow risk this function used
  // to have entirely.
  //
  // uint64_t itself can still overflow here, though: the true worst case is
  // 16 bytes/pixel (RGBA/RGBA_INTEGER x FLOAT/INT/UNSIGNED_INT) at INT32_MAX x INT32_MAX, whose
  // final rowStride * height term is on the order of 16 * 2^31 * 2^31 ~= 7.9e19 -
  // comfortably past UINT64_MAX (~1.84e19). Unlike the 32-bit case above, a uint64_t multiply/
  // add does not trap on overflow either - it silently wraps modulo 2^64, and a wrapped result
  // is not guaranteed to come out *smaller* than the true value (mod-2^64 wraparound can land
  // anywhere in [0, 2^64)), so a wrapped size can still drive a memcpy-style copy past the real
  // buffer's bounds exactly like the pre-64-bit-widening bug this function was originally fixed
  // for (see MultiplyOverflows' own comment). Every multiplication below is therefore checked
  // explicitly; on overflow this returns 0, the same sentinel value already used above for an
  // unrecognized format/type combination and non-default pixel-store state - but, per this
  // function's own header comment and unlike THOSE cases, NOT the same
  // sentinel used for width/height/depth <= 0 further up: that one specific 0 is a genuine,
  // real zero-byte transfer, not "size could not be determined". See
  // PixelTransferSizeIsUnknown() immediately below for the single, correct way callers tell
  // these two 0-returning categories apart - never by re-inspecting this return value alone.
  const uint64_t rowAlignment = static_cast<uint64_t>(alignment);
  const uint64_t widthU64 = static_cast<uint64_t>(width);
  if (MultiplyOverflows(uint64_t{bytesPerPixel}, widthU64)) {
    return 0;
  }
  const uint64_t unpaddedRowBytes = uint64_t{bytesPerPixel} * widthU64;

  // unpaddedRowBytes + (rowAlignment - 1) cannot overflow given the bound just established
  // above (bytesPerPixel <= 16, width <= INT32_MAX puts unpaddedRowBytes on the order of 2^35,
  // nowhere near UINT64_MAX - 8), but the subsequent rounding-up multiply by rowAlignment (up to
  // 8x) is checked anyway rather than relied on to stay within that same margin by inspection.
  const uint64_t roundedUpRowBytes = unpaddedRowBytes + rowAlignment - 1;
  const uint64_t rowStrideUnits = roundedUpRowBytes / rowAlignment;
  if (MultiplyOverflows(rowStrideUnits, rowAlignment)) {
    return 0;
  }
  const uint64_t rowStride = rowStrideUnits * rowAlignment;

  // 3D-aware generalization of the plain 2D case (see this function's own header comment for
  // the derivation): every image below the last needs a full `height` rows of full padded
  // `rowStride` (the next image starts immediately after, so even that image's own last row
  // needs full padding), and only the very last row of the very last image needs no trailing
  // padding past its own pixel data - i.e. (depth*height - 1) fully-padded rows plus one
  // unpadded final row, collapsing to the plain 2D formula when depth == 1
  // (depth*height - 1 == height - 1). Only reached once the imageHeight/skipImages gating
  // above has already passed (both 0, i.e. GL's own defaults) - `height` is therefore already
  // known to be the real per-layer row count this transfer uses, not merely assumed to be.
  // Checked the same way as every other multiply/add here:
  // depth and height are each already known <= INT32_MAX (GLsizei), so their product alone
  // cannot overflow uint64_t, but it is still checked explicitly for consistency with every
  // other step in this function rather than relied on by inspection.
  const uint64_t depthU64 = static_cast<uint64_t>(depth);
  const uint64_t heightU64 = static_cast<uint64_t>(height);
  if (MultiplyOverflows(depthU64, heightU64)) {
    return 0;
  }
  const uint64_t imageRowsMinusOne = depthU64 * heightU64 - 1;
  if (MultiplyOverflows(rowStride, imageRowsMinusOne)) {
    return 0;
  }
  const uint64_t totalRowBytes = rowStride * imageRowsMinusOne;

  // totalRowBytes + unpaddedRowBytes: both operands are already known to be < UINT64_MAX (each
  // passed its own overflow check above), but their *sum* is a separate addition that can still
  // overflow independently - check it explicitly rather than assume headroom.
  if (totalRowBytes > (std::numeric_limits<uint64_t>::max)() - unpaddedRowBytes) {
    return 0;
  }
  return totalRowBytes + unpaddedRowBytes;
}

bool PixelTransferSizeIsUnknown(GLenum format,
                                GLenum type,
                                GLsizei width,
                                GLsizei height,
                                GLint alignment,
                                GLsizei depth,
                                GLint imageHeight,
                                GLint skipImages,
                                GLint rowLength,
                                GLint skipRows,
                                GLint skipPixels,
                                const char* commandName) {
  // Mirrors ComputePixelTransferByteSize()'s own very first check verbatim (this is the ONE
  // case its 0 return genuinely means "zero real bytes", not "size unknown" - see this
  // function's own header comment, pixelTransferSize.h) - checked independently here, rather
  // than inferred from the call below, so this short-circuit never depends on the sentinel
  // value ComputePixelTransferByteSize() happens to return for it.
  if (width <= 0 || height <= 0 || depth <= 0) {
    return false;
  }
  // Every other 0 return from here on IS the "size unknown/unreplayable" sentinel (untracked
  // pixel-store state, an unrecognized format/type combination, or an overflowing size
  // computation) - see that function's own header comment. Calling it again here (rather than
  // threading its already-computed result through) is deliberate: it keeps this predicate a
  // thin, always-in-sync wrapper with no size/gating logic of its own to drift out of step,
  // and every gate it might hit warns at most once per commandName regardless of how many
  // times it is called (see WarnNonDefaultImagePixelStoreState()/WarnNonDefaultRowPixelStoreState()
  // above), so this second call never double-logs.
  return ComputePixelTransferByteSize(format, type, width, height, alignment, depth, imageHeight,
                                      skipImages, rowLength, skipRows, skipPixels,
                                      commandName) == 0;
}

uint32_t ComputeClearBufferValueCount(GLenum buffer) {
  switch (buffer) {
  case 0x1800: // GL_COLOR
    return 4;
  case 0x1801: // GL_DEPTH
  case 0x1802: // GL_STENCIL
    return 1;
  default:
    return 1; // See this function's own header comment.
  }
}

uint32_t ComputeTexOrSamplerParameterValueCount(GLenum pname) {
  switch (pname) {
  case 0x1004: // GL_TEXTURE_BORDER_COLOR
  case 0x8E46: // GL_TEXTURE_SWIZZLE_RGBA
    return 4;
  default:
    return 1; // See this function's own header comment.
  }
}

uint32_t ComputePatchParameterValueCount(GLenum pname) {
  switch (pname) {
  case 0x8E74: // GL_PATCH_DEFAULT_OUTER_LEVEL
    return 4;
  case 0x8E73: // GL_PATCH_DEFAULT_INNER_LEVEL
    return 2;
  default:
    return 1; // See this function's own header comment.
  }
}

uint32_t ComputeVertexAttribParameterValueCount(GLenum pname) {
  switch (pname) {
  case 0x8626: // GL_CURRENT_VERTEX_ATTRIB - the sole 4-element member of VertexAttribPropertyARB
               // (see this function's own header comment) - every other member below (and any
               // unrecognized pname - see default:) returns exactly one GLint.
    return 4;
  case 0x82D4: // GL_VERTEX_ATTRIB_BINDING
  case 0x82D5: // GL_VERTEX_ATTRIB_RELATIVE_OFFSET
  case 0x8622: // GL_VERTEX_ATTRIB_ARRAY_ENABLED
  case 0x8623: // GL_VERTEX_ATTRIB_ARRAY_SIZE
  case 0x8624: // GL_VERTEX_ATTRIB_ARRAY_STRIDE
  case 0x8625: // GL_VERTEX_ATTRIB_ARRAY_TYPE
  case 0x874E: // GL_VERTEX_ATTRIB_ARRAY_LONG
  case 0x886A: // GL_VERTEX_ATTRIB_ARRAY_NORMALIZED
  case 0x889F: // GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
  case 0x88FD: // GL_VERTEX_ATTRIB_ARRAY_INTEGER
  case 0x88FE: // GL_VERTEX_ATTRIB_ARRAY_DIVISOR
    return 1;
  default:
    return 1; // See this function's own header comment - every legal VertexAttribPropertyARB
              // member is already listed explicitly above; this default exists only for a
              // value this function does not recognize at all (a real GL_INVALID_ENUM client
              // error the driver itself will reject without writing `params`).
  }
}

} // namespace gits::opengl2
