// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "clientMemoryCapture.h"

#include "glEnumsAuto.h"
#include "glVersionString.h"
#include "handleMapService.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "pixelTransferSize.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace gits::opengl2 {

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

} // namespace

std::pair<int, int> CurrentContextGLVersion() {
  auto realGetString = g_DispatchTable.glGetString.load(std::memory_order_acquire);
  return ParseGlVersionMajorMinor(reinterpret_cast<const char*>(realGetString(GL_VERSION)));
}

bool CurrentContextHasExtension(int major, const char* extensionName) {
  if (major >= 3) {
    // GL_NUM_EXTENSIONS/glGetStringi are themselves only legal from 3.0 on.
    GLint numExtensions = 0;
    auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
    realGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    auto realGetStringi = g_DispatchTable.glGetStringi.load(std::memory_order_acquire);
    for (GLint i = 0; i < numExtensions; ++i) {
      const char* extension =
          reinterpret_cast<const char*>(realGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
      if (extension != nullptr && std::string(extension) == extensionName) {
        return true;
      }
    }
    return false;
  }
  // Pre-3.0: no profile concept exists yet (core/compatibility profiles start at 3.2), so the
  // legacy single-string form is always legal here - and it is the only enumeration strategy
  // that even exists pre-3.0.
  auto realGetString = g_DispatchTable.glGetString.load(std::memory_order_acquire);
  return ExtensionsStringContainsWholeWord(
      reinterpret_cast<const char*>(realGetString(GL_EXTENSIONS)), extensionName);
}

namespace {

// Whether the CURRENT context (whichever HGLRC is current on this thread right
// now, per the real driver) supports a given GL feature that is core as of
// (coreSinceMajor, coreSinceMinor) or available via `extensionName` on an older context - e.g.
// GL_QUERY_BUFFER_BINDING/GL_ARB_query_buffer_object (core since 4.4) or
// GL_PIXEL_(UN)PACK_BUFFER_BINDING/GL_ARB_pixel_buffer_object (core since 2.1) - before
// GuardQueryBufferPointer/GuardPixelsPointer ever query that feature's own binding-point enum.
// Motivating hazard: GL_QUERY_BUFFER_BINDING is not a legal enum on, say, a 3.3 context that
// never separately enables GL_ARB_query_buffer_object, but glGetQueryObject* itself is valid
// there (core since 1.5/ARB_occlusion_query) - querying GL_QUERY_BUFFER_BINDING unconditionally
// on such a context would inject a spurious GL_INVALID_ENUM into the APPLICATION's own error
// state purely as a side effect of capture, even though the app's own call was entirely valid.
// See modern-opengl-backend-architecture.md's "GITS must not perturb the app's observable GL
// error state" decision. GL_PIXEL_(UN)PACK_BUFFER_BINDING has the same theoretical hazard on a
// pre-1.5 context (buffer objects, and therefore any buffer *binding point*, do not exist at
// all before GL_ARB_vertex_buffer_object/1.5) even though glTexImage2D/glCompressedTexImage2D
// themselves are valid as far back as 1.0/1.3 - narrower in practice (no known driver still
// ships a sub-1.5 context) but the exact same class of bug, so guarded identically rather than
// assumed safe.
//
// Every enum this function itself queries is chosen so it is unconditionally legal on whatever
// context version led to that particular query executing: GL_VERSION (queried first, always
// legal on any GL version/profile) decides which of the two extension-enumeration strategies
// below is itself safe to use next - GL_NUM_EXTENSIONS/glGetStringi (legal 3.0+ only) vs. the
// legacy glGetString(GL_EXTENSIONS) single string (legal on any pre-3.2 context, and on any
// compatibility-profile context regardless of version) - so this function can never itself
// become a second instance of the exact hazard it exists to prevent.
//
// Cached per-(context, feature) behind a mutex: a context's own GL version/extension set is
// fixed for its entire lifetime once created, so re-parsing GL_VERSION and re-enumerating every
// extension string on every single guarded call would be needlessly expensive. `cache` is
// owned by the caller (one static map per feature) rather than shared, so two features can
// never collide on the same context key.
//
// Keyed by the context's stable GITSKey (via ContextHandleMapService::GetKey(), which
// mints/returns the SAME key that UpdateOutputHandle/UpdateHandle already use for every other
// HGLRC-typed argument - see handleArgumentUpdaters.h), never by the raw live HGLRC value -
// mirrors WarmUpResolvableFunctions()'s own identical technique in openglDriversAuto.cpp.mako
// (see that function's header comment for the full rationale). Windows can and does reuse a
// destroyed
// context's numeric HGLRC value for a later, entirely unrelated wglCreateContext[ARB] call;
// keying this cache on the raw handle would let a NEW context inherit a STALE cached bool from
// whichever OLD, now-destroyed context previously held that same HGLRC number - a stale `true`
// re-introduces the GL_INVALID_ENUM side-effect bug this cache exists to avoid (this function
// probing an unsupported enum), while a stale `false` is worse: it makes GuardPixelsPointer/
// GuardQueryBufferPointer skip their safety check entirely for a context that actually DOES
// support the feature, letting a PBO/query-buffer byte offset be dereferenced as a real
// client pointer - exactly the crash/OOB-read hazard those guards exist to prevent.
//
// No matching wglDeleteContext-side eviction, for the same reason WarmUpResolvableFunctions has
// none: a GITSKey is minted exactly once per logical context (ContextHandleMapService::Register,
// called only from wglCreateContext(ARB)'s own UpdateOutputHandle) and is never reused/recycled
// even after ContextHandleMapService::Erase() removes that context's HGLRC<->key mapping on
// destroy, so a destroyed context's leftover cache entry can never collide with a later,
// genuinely different context's key - there is nothing for it to collide with even if it is
// never removed (mirrors ContextHandleMapService::Erase's own "player-side cleanup on destroy
// is not load-bearing within a single replay" precedent). A GL feature's support also cannot
// change over a still-live context's lifetime, so a cache hit is always still correct for that
// same key regardless of how long it has been sitting in the map.
//
// GetKey() returns the shared sentinel 0 - never a
// real, distinct key - for any context ContextHandleMapService has no registration for (see
// that method's own comment: "never created, or created before recording started" - i.e. a
// context that existed before this recorder attached and therefore never went through
// wglCreateContext(ARB)'s own UpdateOutputHandle). Caching under that shared sentinel would let
// the FIRST such pre-recording context to probe a feature poison the cached answer for every
// OTHER, potentially GL-version/vendor-DIFFERENT pre-recording context encountered afterwards -
// exactly the same stale-entry hazard this function's own header comment above describes for
// HGLRC reuse, just reached through a different, shared-instead-of-reassigned key. Fixed the
// same way this codebase always prefers when a cheap, always-correct alternative identity
// cannot be derived (see this function's own "No matching wglDeleteContext-side eviction"
// paragraph for the precedent of preferring provable correctness over micro-optimizing an
// already-rare path): bypass the cache entirely - skip both the lookup AND the insertion -
// whenever contextKey is this shared sentinel, always re-querying the driver fresh instead.
// Slightly less efficient for pre-recording contexts specifically (every guarded call on such a
// context re-parses GL_VERSION/re-enumerates extensions), but those are inherently rare (a
// context must be current across a whole wglMakeCurrent-to-app-exit span without this recorder
// ever having seen its creation) and correctness matters far more here than that micro-cost.
//
// The feature probes below deliberately use GL_VERSION, GL_EXTENSIONS, and GL_NUM_EXTENSIONS
// only where each enum is legal for the current context/version/profile. GL_VERSION is legal on
// every GL context. GL_NUM_EXTENSIONS and glGetStringi are queried only after establishing GL
// 3.0 or newer, while the legacy GL_EXTENSIONS string is queried only before GL 3.0, where no
// core-profile restriction exists. In particular, do not replace the unconditional GL_VERSION
// query with GL_MAJOR_VERSION, or query either extension form before its corresponding version
// check.
bool ContextSupportsGLFeature(std::mutex& cacheMutex,
                              std::unordered_map<GITSKey, bool>& cache,
                              const char* extensionName,
                              int coreSinceMajor,
                              int coreSinceMinor,
                              const char* extensionNameAlias = nullptr) {
  auto currentContextFn = g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  const HGLRC currentContext = currentContextFn ? currentContextFn() : nullptr;
  const GITSKey contextKey = ContextHandleMapService::Get().GetKey(currentContext);
  const bool cacheable = contextKey != 0;
  if (cacheable) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cache.find(contextKey);
    if (it != cache.end()) {
      return it->second;
    }
  }
  const auto [major, minor] = CurrentContextGLVersion();
  // Either candidate name is sufficient - see ContextSupportsPixelBufferObject()'s own comment
  // for why GL_EXT_pixel_buffer_object is passed as `extensionNameAlias` there.
  const bool supported =
      major > coreSinceMajor || (major == coreSinceMajor && minor >= coreSinceMinor) ||
      CurrentContextHasExtension(major, extensionName) ||
      (extensionNameAlias != nullptr && CurrentContextHasExtension(major, extensionNameAlias));
  if (cacheable) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache[contextKey] = supported;
  }
  return supported;
}

std::mutex g_PixelBufferFeatureMutex;
std::unordered_map<GITSKey, bool> g_PixelBufferFeatureByContext;

// GL_ARB_pixel_buffer_object was promoted to core in OpenGL 2.1 - see
// ContextSupportsGLFeature's own header comment for why this is checked at all (a narrower,
// largely theoretical counterpart of the GL_QUERY_BUFFER_BINDING hazard below, kept for
// consistency rather than assumed safe).
//
// Also accepts the older GL_EXT_pixel_buffer_object as sufficient, not only
// GL_ARB_pixel_buffer_object. Confirmed via third_party/OpenGL-Registry/xml/gl.xml: EXT's own
// <require> block defines GL_PIXEL_PACK_BUFFER_EXT/GL_PIXEL_UNPACK_BUFFER_EXT/
// GL_PIXEL_PACK_BUFFER_BINDING_EXT/GL_PIXEL_UNPACK_BUFFER_BINDING_EXT at the exact same numeric
// values (0x88EB/0x88EC/0x88ED/0x88EF) that the GL_PIXEL_* ARB/core enums use.
// EXT_pixel_buffer_object is not merely similarly-named, it is bit-for-bit the same binding-point
// enum surface ARB_pixel_buffer_object (and later core 2.1) reused verbatim. A pre-2.1 context
// exposing only the EXT form (no ARB string) would previously fail this check outright, making
// ContextSupportsPixelBufferObject() return false and GuardPixelsPointer skip its bound-buffer
// check entirely on such a context - exactly the "dereference a buffer-relative byte offset as a
// real client pointer" hazard this guard exists to prevent, on a context that in fact has a pixel
// buffer bound via the EXT path.
bool ContextSupportsPixelBufferObject() {
  return ContextSupportsGLFeature(g_PixelBufferFeatureMutex, g_PixelBufferFeatureByContext,
                                  "GL_ARB_pixel_buffer_object", 2, 1, "GL_EXT_pixel_buffer_object");
}

std::mutex g_PixelBufferGuardMutex;
std::unordered_set<std::string> g_WarnedPixelBufferGuardCommands;

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

const void* GuardPixelsPointer(const char* commandName, const void* pixels, GLenum bindingPname) {
  if (!PixelBufferObjectIsBound(bindingPname)) {
    return pixels;
  }
  std::lock_guard<std::mutex> lock(g_PixelBufferGuardMutex);
  if (g_WarnedPixelBufferGuardCommands.insert(commandName).second) {
    LOG_WARNING << commandName
                << ": a pixel buffer object is bound - the pixels/data parameter "
                   "is a buffer-relative byte offset, not a client pointer; this "
                   "call's pixel payload will not be captured (degraded fidelity) "
                   "rather than risk copying from an invalid address";
  }
  return nullptr;
}

std::mutex g_QueryBufferGuardMutex;
std::unordered_set<std::string> g_WarnedQueryBufferGuardCommands;
std::mutex g_QueryBufferFeatureMutex;
std::unordered_map<GITSKey, bool> g_QueryBufferFeatureByContext;

// GL_ARB_query_buffer_object was promoted to core in OpenGL 4.4 - see
// ContextSupportsGLFeature's own header comment for the full rationale: glGetQueryObject*
// itself is valid as far back as GL 1.5/ARB_occlusion_query, well below 4.4, so this gate must
// probe GL_ARB_query_buffer_object's own version/extension support separately.
bool ContextSupportsQueryBufferObject() {
  return ContextSupportsGLFeature(g_QueryBufferFeatureMutex, g_QueryBufferFeatureByContext,
                                  "GL_ARB_query_buffer_object", 4, 4);
}

} // namespace

const void* GuardUnpackPixelsPointer(const char* commandName, const void* pixels) {
  return GuardPixelsPointer(commandName, pixels, GL_PIXEL_UNPACK_BUFFER_BINDING);
}

const void* GuardPackPixelsPointer(const char* commandName, const void* pixels) {
  return GuardPixelsPointer(commandName, pixels, GL_PIXEL_PACK_BUFFER_BINDING);
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
  if (params == nullptr || !ContextSupportsQueryBufferObject() ||
      QueryBoundBufferQuiet(GL_QUERY_BUFFER_BINDING) == 0) {
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

namespace {
std::mutex g_BufferObjectFeatureMutex;
std::unordered_map<GITSKey, bool> g_BufferObjectFeatureByContext;
std::mutex g_CopyBufferFeatureMutex;
std::unordered_map<GITSKey, bool> g_CopyBufferFeatureByContext;
std::mutex g_DirectStateAccessFeatureMutex;
std::unordered_map<GITSKey, bool> g_DirectStateAccessFeatureByContext;
std::mutex g_InternalformatQuery2FeatureMutex;
std::unordered_map<GITSKey, bool> g_InternalformatQuery2FeatureByContext;
} // namespace

bool ContextSupportsCopyBuffer() {
  return ContextSupportsGLFeature(g_CopyBufferFeatureMutex, g_CopyBufferFeatureByContext,
                                  "GL_ARB_copy_buffer", 3, 1);
}

bool ContextSupportsDirectStateAccess() {
  return ContextSupportsGLFeature(g_DirectStateAccessFeatureMutex,
                                  g_DirectStateAccessFeatureByContext, "GL_ARB_direct_state_access",
                                  4, 5);
}

bool ContextSupportsInternalformatQuery2() {
  return ContextSupportsGLFeature(g_InternalformatQuery2FeatureMutex,
                                  g_InternalformatQuery2FeatureByContext,
                                  "GL_ARB_internalformat_query2", 4, 3);
}

bool ElementArrayBufferIsBound() {
  return ContextSupportsGLFeature(g_BufferObjectFeatureMutex, g_BufferObjectFeatureByContext,
                                  "GL_ARB_vertex_buffer_object", 1, 5) &&
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

bool GetBoundBufferParameter(GLenum target, GLenum pname, GLint& value) {
  auto get = g_DispatchTable.glGetBufferParameteriv.load(std::memory_order_acquire);
  if (get == nullptr) {
    get = g_DispatchTable.glGetBufferParameterivARB.load(std::memory_order_acquire);
  }
  if (get == nullptr) {
    return false;
  }
  get(target, pname, &value);
  return true;
}

bool GetBoundBufferSize(GLenum target, GLint64& size) {
  const auto [major, minor] = CurrentContextGLVersion();
  if (major > 3 || (major == 3 && minor >= 2)) {
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

} // namespace gits::opengl2
