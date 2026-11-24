// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "clientMemoryCapture.h"

#include "glContextCapabilities.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "pixelTransferSize.h"

#include <mutex>
#include <string>
#include <unordered_set>

namespace gits::opengl {

namespace {

// Quiet counterpart of ResolveBoundBuffer() in mapTrackingService.cpp, for
// GuardUnpackPixelsPointer/GuardPackPixelsPointer below: "nothing bound" is the expected,
// common case on every single glTexImage2D/glReadPixels-shaped call (no pixel buffer object in
// use at all), not a warning-worthy anomaly the way it is for ResolveBoundBuffer's own
// map/unmap-specific callers - so this never logs.
GLuint QueryBoundBufferQuiet(GLenum bindingPname) {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(bindingPname, &value);
  return static_cast<GLuint>(value);
}

// The exact, sole condition under which
// GuardPixelsPointer() below redirects `pixels` into buffer-offset mode (returns nullptr
// instead of the real pointer) - factored out so UnpackPixelsPointerIsBufferOffset()/
// PackPixelsPointerIsBufferOffset() below can ask the identical question independently,
// without duplicating (and risking drifting out of sync with) GuardPixelsPointer's own logic.
// Deliberately does NOT look at `pixels` itself at all: buffer-offset mode is determined
// purely by whether a pixel (un)pack buffer object is actually bound, never by the numeric
// value the app passed as `pixels`. A null `pixels` with a PBO bound is
// GL_ARB_pixel_buffer_object's own spec-legal way of saying "offset 0 into the bound buffer" -
// a perfectly ordinary, common offset value, not an absent/inapplicable one. Gating this
// predicate on `pixels != nullptr` would misreport that legal offset-0 case as "not
// buffer-offset mode": UnpackPixelsPointerIsBufferOffset()/PackPixelsPointerIsBufferOffset()
// feed directly into commandRunnersAuto.cpp.mako's `SizeUnknown && !IsBufferOffset` skip gate,
// so misreporting IsBufferOffset as false there would incorrectly skip a perfectly valid,
// PBO-backed call at offset 0 instead of letting it run normally against the bound buffer.
bool PixelBufferObjectIsBound(GLenum bindingPname) {
  return ContextSupportsPixelBufferObject() && QueryBoundBufferQuiet(bindingPname) != 0;
}

const void* GuardPixelsPointer(const void* pixels, GLenum bindingPname) {
  return PixelBufferObjectIsBound(bindingPname) ? nullptr : pixels;
}

std::mutex g_QueryBufferGuardMutex;
std::unordered_set<std::string> g_WarnedQueryBufferGuardCommands;
} // namespace

bool QueryBufferIsBound() {
  return ContextSupportsQueryBufferObject() && QueryBoundBufferQuiet(GL_QUERY_BUFFER_BINDING) != 0;
}

const void* GuardUnpackPixelsPointer(const char* /*commandName*/, const void* pixels) {
  return GuardPixelsPointer(pixels, GL_PIXEL_UNPACK_BUFFER_BINDING);
}

const void* GuardPackPixelsPointer(const char* /*commandName*/, const void* pixels) {
  return GuardPixelsPointer(pixels, GL_PIXEL_PACK_BUFFER_BINDING);
}

// See these two functions' own header comment
// (clientMemoryCapture.h) for the full rationale: a PIXEL_TRANSFER_SIZE-shaped command's
// "skip the real driver call, the captured size is unknown" gate
// (commandRunnersAuto.cpp.mako's ${cmd.name}PixelTransferSizeUnknown) must not fire for a call
// whose pixels/pixels-equivalent parameter was ACTUALLY a buffer-relative offset all along -
// PixelTransferSizeIsUnknown() only ever reasons about format/type/dimensions/pixel-store
// state, never about buffer bindings, so it cannot tell the two cases apart on its own. These
// mirror GuardUnpackPixelsPointer()/GuardPackPixelsPointer() immediately above bit-for-bit
// (via the shared PixelBufferObjectIsBound() predicate above) rather than re-deriving the
// condition, so this can never disagree with whichever mode the guard itself actually chose
// for this exact call. `pixels` itself is intentionally unused here (kept only for a
// call-site-readable, self-documenting signature) - see PixelBufferObjectIsBound()'s own
// comment for why buffer-offset mode never depends on the pointer's own value, including
// nullptr (a legal offset-0 encoding whenever a PBO is actually bound).
bool UnpackPixelsPointerIsBufferOffset(const void* /*pixels*/) {
  return PixelBufferObjectIsBound(GL_PIXEL_UNPACK_BUFFER_BINDING);
}

bool PackPixelsPointerIsBufferOffset(const void* /*pixels*/) {
  return PixelBufferObjectIsBound(GL_PIXEL_PACK_BUFFER_BINDING);
}

// See this function's own header comment (clientMemoryCapture.h) for the
// full rationale. Structurally mirrors GuardPixelsPointer() above (quiet binding query, warn-
// once, nullptr-on-bound), kept as its own function (rather than a third bindingPname argument
// to GuardPixelsPointer itself) because the two guards protect opposite directions
// (GuardPixelsPointer's callers are IN-direction pre-call reads; this guard's own callers are
// OUT-direction post-call reads) with their own, distinct warning text, and share no callers
// today. Uses g_QueryBufferGuardMutex/g_WarnedQueryBufferGuardCommands (declared above, inside
// the anonymous namespace above - visible here via the unnamed namespace's implicit
// using-directive into this translation unit, same as GuardPixelsPointer's own use of
// QueryBoundBufferQuiet() and the pixel buffer binding enums from that same
// scope).
const void* GuardQueryBufferPointer(const char* commandName, const void* params) {
  if (params == nullptr || !QueryBufferIsBound()) {
    return params;
  }
  std::lock_guard<std::mutex> lock(g_QueryBufferGuardMutex);
  if (g_WarnedQueryBufferGuardCommands.insert(commandName).second) {
    LOG_WARNING << commandName
                << ": a query buffer object is bound (GL_QUERY_BUFFER_BINDING != 0) - the "
                   "params parameter is a buffer-relative byte offset, not a client pointer "
                   "the driver actually wrote through; this call's result will not be "
                   "captured (degraded fidelity) rather than risk reading from an invalid "
                   "address - see ARB_query_buffer_object and pointer_metadata.py's "
                   "PointerParamOverride.query_buffer_guard";
  }
  return nullptr;
}

const void* GuardDrawIndirectPointer(const void* indirect) {
  return QueryBoundBufferQuiet(GL_DRAW_INDIRECT_BUFFER_BINDING) != 0 ? nullptr : indirect;
}

bool ElementArrayBufferIsBound() {
  return ContextSupportsBufferObjects() &&
         QueryBoundBufferQuiet(GL_ELEMENT_ARRAY_BUFFER_BINDING) != 0;
}

const void* GuardElementArrayPointer(const void* indices) {
  return ElementArrayBufferIsBound() ? nullptr : indices;
}

GLint QueryUnpackAlignment() {
  GLint value = 4;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_UNPACK_ALIGNMENT, &value);
  return value;
}

GLint QueryPackAlignment() {
  GLint value = 4;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_PACK_ALIGNMENT, &value);
  return value;
}

GLint QueryUnpackImageHeight() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &value);
  return value;
}

GLint QueryUnpackSkipImages() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_UNPACK_SKIP_IMAGES, &value);
  return value;
}

GLint QueryPackImageHeight() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_PACK_IMAGE_HEIGHT, &value);
  return value;
}

GLint QueryPackSkipImages() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_PACK_SKIP_IMAGES, &value);
  return value;
}

// See pixelTransferSize.h's own comment on QueryUnpackRowLength() et al.'s
// declarations for why these six exist and why every enabled pixel-transfer call site queries
// them unconditionally now, unlike QueryUnpackImageHeight() et al. above.
GLint QueryUnpackRowLength() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_UNPACK_ROW_LENGTH, &value);
  return value;
}

GLint QueryUnpackSkipRows() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_UNPACK_SKIP_ROWS, &value);
  return value;
}

GLint QueryUnpackSkipPixels() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_UNPACK_SKIP_PIXELS, &value);
  return value;
}

GLint QueryPackRowLength() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_PACK_ROW_LENGTH, &value);
  return value;
}

GLint QueryPackSkipRows() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_PACK_SKIP_ROWS, &value);
  return value;
}

GLint QueryPackSkipPixels() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(GL_PACK_SKIP_PIXELS, &value);
  return value;
}

namespace {

// 0 for a target that does not name one image of a texture level.
int TextureLevelDimensions(GLenum target) {
  switch (target) {
  case GL_TEXTURE_1D:
    return 1;
  case GL_TEXTURE_2D:
  case GL_TEXTURE_1D_ARRAY:
  case GL_TEXTURE_RECTANGLE:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    return 2;
  case GL_TEXTURE_3D:
  case GL_TEXTURE_2D_ARRAY:
  case GL_TEXTURE_CUBE_MAP_ARRAY:
    return 3;
  default:
    return 0;
  }
}

template <typename SizeFunction>
auto WithTextureLevelPackTransfer(GLenum target,
                                  GLint level,
                                  GLenum format,
                                  GLenum type,
                                  const char* commandName,
                                  SizeFunction sizeFunction) {
  const int dimensions = TextureLevelDimensions(target);
  GLint width = 0;
  GLint height = 1;
  GLint depth = 1;
  if (dimensions != 0 && level >= 0) {
    auto getLevelParameter =
        g_DispatchTable.glGetTexLevelParameteriv.load(std::memory_order_acquire);
    getLevelParameter(target, level, GL_TEXTURE_WIDTH, &width);
    if (dimensions >= 2) {
      getLevelParameter(target, level, GL_TEXTURE_HEIGHT, &height);
    }
    if (dimensions == 3) {
      getLevelParameter(target, level, GL_TEXTURE_DEPTH, &depth);
    }
  }
  // A 1D image is packed as an image of height 1, which the pack row state applies to as well.
  return sizeFunction(dimensions != 0 && level >= 0, format, type, width, height,
                      QueryPackAlignment(), depth, dimensions == 3 ? QueryPackImageHeight() : 0,
                      dimensions == 3 ? QueryPackSkipImages() : 0, QueryPackRowLength(),
                      QueryPackSkipRows(), QueryPackSkipPixels(), commandName);
}

} // namespace

uint64_t ComputeTextureLevelPackByteSize(
    GLenum target, GLint level, GLenum format, GLenum type, const char* commandName) {
  return WithTextureLevelPackTransfer(
      target, level, format, type, commandName, [](bool known, auto... transfer) -> uint64_t {
        return known ? ComputePixelTransferByteSize(transfer...) : 0;
      });
}

bool TextureLevelPackSizeIsUnknown(
    GLenum target, GLint level, GLenum format, GLenum type, const char* commandName) {
  return WithTextureLevelPackTransfer(target, level, format, type, commandName,
                                      [](bool known, auto... transfer) {
                                        return !known || PixelTransferSizeIsUnknown(transfer...);
                                      });
}

bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint& value) {
  if (!TryResolveRealFunction("glGetBufferParameteriv")) {
    return false;
  }
  auto get = g_DispatchTable.glGetBufferParameteriv.load(std::memory_order_acquire);
  get(target, pname, &value);
  return true;
}

bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint64& value) {
  if (pname == GL_BUFFER_SIZE) {
    return GetBoundBufferSize(target, value);
  }
  GLint result = 0;
  if (!GetBoundBufferParameter(target, pname, result)) {
    return false;
  }
  value = result;
  return true;
}

bool GetBoundBufferSize(GLenum target, GLint64& size) {
  if (GlVersionAtLeast(CurrentContextGLVersion(), 3, 2)) {
    if (!TryResolveRealFunction("glGetBufferParameteri64v")) {
      return false;
    }
    auto get = g_DispatchTable.glGetBufferParameteri64v.load(std::memory_order_acquire);
    get(target, GL_BUFFER_SIZE, &size);
    return true;
  }
  GLint value = 0;
  if (!GetBoundBufferParameter(target, GL_BUFFER_SIZE, value)) {
    return false;
  }
  size = value;
  return true;
}

} // namespace gits::opengl
