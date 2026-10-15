// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingWrappers.h"

#include "captureManager.h"
#include "contextHandleMapService.h"
#include "handleArgumentUpdaters.h"
#include "log.h"
#include "mappedDataMetaTypes.h"
#include "mapTrackingService.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "pixelTransferSize.h"
#include "recursionGuard.h"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Hand-declared instead of including <windows.h> - see recorderWrappersAuto.cpp.mako's
// identical GetCurrentThreadId precedent (that header comment explains why this backend
// avoids <windows.h> in any translation unit that also needs glTypes.h's own declarations).
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace gits::opengl2 {

namespace {

// GL enum values used by this file, hand-inlined rather than including <GL/gl.h> - same
// convention as hdcVirtualizationService.cpp's PFD_*/<wingdi.h> constants, values confirmed
// against third_party/OpenGL-Registry/xml/gl.xml (see the plan doc's "Sources consulted").
// Only the buffer targets this backend's rendering-pipeline scope plausibly maps are covered -
// see docs/development/opengl2-buffer-map-tracking-plan.md's Decision 4.
constexpr GLenum kArrayBuffer = 0x8892;
constexpr GLenum kArrayBufferBinding = 0x8894;
constexpr GLenum kElementArrayBuffer = 0x8893;
constexpr GLenum kElementArrayBufferBinding = 0x8895;
constexpr GLenum kPixelPackBuffer = 0x88EB;
constexpr GLenum kPixelPackBufferBinding = 0x88ED;
constexpr GLenum kPixelUnpackBuffer = 0x88EC;
constexpr GLenum kPixelUnpackBufferBinding = 0x88EF;
constexpr GLenum kUniformBuffer = 0x8A11;
constexpr GLenum kUniformBufferBinding = 0x8A28;
constexpr GLenum kTransformFeedbackBuffer = 0x8C8E;
constexpr GLenum kTransformFeedbackBufferBinding = 0x8C8F;
constexpr GLenum kCopyReadBuffer = 0x8F36;
constexpr GLenum kCopyReadBufferBinding = 0x8F36; // Same value as the target itself - a
                                                  // real gl.xml quirk, not a typo.
constexpr GLenum kCopyWriteBuffer = 0x8F37;
constexpr GLenum kCopyWriteBufferBinding = 0x8F37; // Same quirk as GL_COPY_READ_BUFFER above.
constexpr GLenum kDrawIndirectBuffer = 0x8F3F;
constexpr GLenum kDrawIndirectBufferBinding = 0x8F43;
constexpr GLenum kAtomicCounterBuffer = 0x92C0;
constexpr GLenum kAtomicCounterBufferBinding = 0x92C1;
constexpr GLenum kDispatchIndirectBuffer = 0x90EE;
constexpr GLenum kDispatchIndirectBufferBinding = 0x90EF;
constexpr GLenum kShaderStorageBuffer = 0x90D2;
constexpr GLenum kShaderStorageBufferBinding = 0x90D3;
constexpr GLenum kQueryBuffer = 0x9192;
constexpr GLenum kQueryBufferBinding = 0x9193;

// For ContextSupportsGLFeature() below. Every one of these four is chosen specifically because
// it is unconditionally legal to query on ANY GL context/version/profile this function might
// be probing from - see that function's own header comment for the full rationale of why
// nothing riskier (GL_MAJOR_VERSION, GL_NUM_EXTENSIONS itself on a <3.0 context, GL_EXTENSIONS
// itself on a 3.2+ core-profile context) is used unconditionally the same way.
constexpr GLenum kVersionEnum = 0x1F02;       // GL_VERSION - legal on every GL version/profile.
constexpr GLenum kExtensionsEnum = 0x1F03;    // GL_EXTENSIONS (legacy single string) - legal on
                                              // any pre-3.2 context, and on any compatibility-
                                              // profile context regardless of version; a 3.2+
                                              // core-profile context rejects it.
constexpr GLenum kNumExtensionsEnum = 0x821D; // GL_NUM_EXTENSIONS - legal GL 3.0+ only.

constexpr GLbitfield kMapReadBit = 0x0001;
constexpr GLbitfield kMapWriteBit = 0x0002;

// glUnmapBuffer's own GL_TRUE/GL_FALSE return value: GL_FALSE means the data store's contents
// became corrupted/undefined for implementation-specific reasons (e.g. a screen-mode change on
// some platforms) - the spec does not require
// this to reflect what the application itself actually wrote, so a real driver reading back
// through a temporary re-map still succeeds (no crash, no invalid memory), but the bytes read
// back are meaningless. CustomWrapper_glUnmapBuffer must not record those as a valid
// MappedDataMetaCommand snapshot.
constexpr GLboolean kGlTrue = 1;

// For QueryUnpackAlignment()/QueryPackAlignment() below - see pixelTransferSize.h's own
// comment on why the real, current value (not a hardcoded default) is queried here.
constexpr GLenum kUnpackAlignment = 0x0CF5;
constexpr GLenum kPackAlignment = 0x0D05;

// For QueryUnpackImageHeight()/QueryUnpackSkipImages()/QueryPackImageHeight()/
// QueryPackSkipImages() below - see pixelTransferSize.h's own comment on why these are queried
// live rather than assumed to be 0 (GL's own default for all four).
constexpr GLenum kPackSkipImages = 0x806B;
constexpr GLenum kPackImageHeight = 0x806C;
constexpr GLenum kUnpackSkipImages = 0x806D;
constexpr GLenum kUnpackImageHeight = 0x806E;

// For QueryUnpackRowLength()/QueryUnpackSkipRows()/QueryUnpackSkipPixels()/
// QueryPackRowLength()/QueryPackSkipRows()/QueryPackSkipPixels() below - see
// pixelTransferSize.h's own comment on why these six are queried live too (GL's own default
// for all six is 0).
constexpr GLenum kUnpackRowLength = 0x0CF2;
constexpr GLenum kUnpackSkipRows = 0x0CF3;
constexpr GLenum kUnpackSkipPixels = 0x0CF4;
constexpr GLenum kPackRowLength = 0x0D02;
constexpr GLenum kPackSkipRows = 0x0D03;
constexpr GLenum kPackSkipPixels = 0x0D04;

// See Decision 4: the driver, not a hand-rolled binding tracker, is the source of truth for
// which buffer is currently bound to `target`. Returns 0 for a target this plan does not track
// (see Decision 1's scope) - the caller treats that identically to "recognized target, nothing
// bound".
GLenum BindingPnameForTarget(GLenum target) {
  switch (target) {
  case kArrayBuffer:
    return kArrayBufferBinding;
  case kElementArrayBuffer:
    return kElementArrayBufferBinding;
  case kPixelPackBuffer:
    return kPixelPackBufferBinding;
  case kPixelUnpackBuffer:
    return kPixelUnpackBufferBinding;
  case kUniformBuffer:
    return kUniformBufferBinding;
  case kTransformFeedbackBuffer:
    return kTransformFeedbackBufferBinding;
  case kCopyReadBuffer:
    return kCopyReadBufferBinding;
  case kCopyWriteBuffer:
    return kCopyWriteBufferBinding;
  case kDrawIndirectBuffer:
    return kDrawIndirectBufferBinding;
  case kAtomicCounterBuffer:
    return kAtomicCounterBufferBinding;
  case kDispatchIndirectBuffer:
    return kDispatchIndirectBufferBinding;
  case kShaderStorageBuffer:
    return kShaderStorageBufferBinding;
  case kQueryBuffer:
    return kQueryBufferBinding;
  default:
    return 0;
  }
}

// Warn-once state for the two "cannot resolve a buffer for this target" cases Decision 4's
// Consequences names (unrecognized target, recognized target with nothing bound) - low
// frequency path (map/unmap, not per-draw-call), so a plain mutex is fine, mirroring
// ObjectNameMapService's own warn-once convention.
std::mutex g_WarnedTargetsMutex;
std::unordered_set<GLenum> g_WarnedUnrecognizedTargets;
std::unordered_set<GLenum> g_WarnedNoBufferBound;

// Resolves the real, live GLuint currently bound to `target` via glGetIntegerv - see
// Decision 4. Returns 0 (with a single warning per unique target) if `target` is not one of
// the buffer targets this plan tracks, or if the query succeeds but nothing is bound there.
GLuint ResolveBoundBuffer(GLenum target) {
  const GLenum pname = BindingPnameForTarget(target);
  if (pname == 0) {
    std::lock_guard<std::mutex> lock(g_WarnedTargetsMutex);
    if (g_WarnedUnrecognizedTargets.insert(target).second) {
      LOG_WARNING << "CustomWrapper_glMapBufferRange/glUnmapBuffer: target " << target
                  << " is not a buffer target this plan resolves (see "
                     "opengl2-buffer-map-tracking-plan.md's Decision 4) - buffer-map "
                     "tracking skipped for this target";
    }
    return 0;
  }
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(pname, &value);
  const GLuint buffer = static_cast<GLuint>(value);
  if (buffer == 0) {
    std::lock_guard<std::mutex> lock(g_WarnedTargetsMutex);
    if (g_WarnedNoBufferBound.insert(target).second) {
      LOG_WARNING << "CustomWrapper_glMapBufferRange/glUnmapBuffer: target " << target
                  << " has no buffer bound - buffer-map tracking skipped for this target";
    }
  }
  return buffer;
}

// Quiet counterpart of ResolveBoundBuffer() above, for GuardUnpackPixelsPointer/
// GuardPackPixelsPointer below: "nothing bound" is the expected, common case on every single
// glTexImage2D/glReadPixels-shaped call (no pixel buffer object in use at all), not a warning
// -worthy anomaly the way it is for ResolveBoundBuffer's own map/unmap-specific callers - so
// this never logs.
GLuint QueryBoundBufferQuiet(GLenum bindingPname) {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(bindingPname, &value);
  return static_cast<GLuint>(value);
}

// Parses a GL_VERSION string's leading "<major>.<minor>" (e.g. "3.3.0 NVIDIA 555.85",
// "4.6 (Core Profile) Mesa 24.0.5") into (major, minor). Returns (0, 0) - deliberately "older
// than every real version" rather than some sentinel that could accidentally compare >= a
// small requested version - if the string is null or does not start with two dot-separated
// integers, matching ContextSupportsGLFeature's own fail-closed philosophy (see that
// function's own header comment): a version this cannot parse is treated as "does not support
// the feature being probed for," never as "must be new enough."
std::pair<int, int> ParseGlVersionMajorMinor(const char* versionString) {
  if (versionString == nullptr) {
    return {0, 0};
  }
  int major = 0;
  int minor = 0;
  if (std::sscanf(versionString, "%d.%d", &major, &minor) != 2) {
    return {0, 0};
  }
  return {major, minor};
}

// True if the GL_EXTENSIONS space-separated legacy string contains `extensionName` as a whole
// word - not merely as a substring of some other, longer extension's name (e.g.
// "GL_ARB_query_buffer_object" must not match inside a hypothetical
// "GL_ARB_query_buffer_object_v2").
bool ExtensionsStringContainsWholeWord(const char* extensions, const char* extensionName) {
  if (extensions == nullptr) {
    return false;
  }
  const std::string haystack = extensions;
  const std::string needle = extensionName;
  size_t pos = haystack.find(needle);
  while (pos != std::string::npos) {
    const bool leftOk = (pos == 0 || haystack[pos - 1] == ' ');
    const size_t rightIdx = pos + needle.size();
    const bool rightOk = (rightIdx == haystack.size() || haystack[rightIdx] == ' ');
    if (leftOk && rightOk) {
      return true;
    }
    pos = haystack.find(needle, pos + 1);
  }
  return false;
}

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
  auto realGetString = g_DispatchTable.glGetString.load(std::memory_order_acquire);
  const char* versionString = reinterpret_cast<const char*>(realGetString(kVersionEnum));
  const auto [major, minor] = ParseGlVersionMajorMinor(versionString);
  bool supported = false;
  if (major > coreSinceMajor || (major == coreSinceMajor && minor >= coreSinceMinor)) {
    // Unconditionally part of core as of this version - no extension string lookup needed.
    supported = true;
  } else if (major >= 3) {
    // GL_NUM_EXTENSIONS/glGetStringi are themselves only legal from 3.0 on - safe here only
    // because of the version check just performed.
    GLint numExtensions = 0;
    auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
    realGetIntegerv(kNumExtensionsEnum, &numExtensions);
    auto realGetStringi = g_DispatchTable.glGetStringi.load(std::memory_order_acquire);
    for (GLint i = 0; i < numExtensions && !supported; ++i) {
      const char* extension =
          reinterpret_cast<const char*>(realGetStringi(kExtensionsEnum, static_cast<GLuint>(i)));
      if (extension == nullptr) {
        continue;
      }
      // Either candidate name is sufficient - see ContextSupportsPixelBufferObject()'s own
      // comment for why GL_EXT_pixel_buffer_object is passed as `extensionNameAlias` here.
      if (std::string(extension) == extensionName ||
          (extensionNameAlias != nullptr && std::string(extension) == extensionNameAlias)) {
        supported = true;
      }
    }
  } else {
    // Pre-3.0: no profile concept exists yet (core/compatibility profiles start at 3.2), so
    // the legacy single-string form is always legal here - and it is the only enumeration
    // strategy that even exists pre-3.0.
    const char* extensions = reinterpret_cast<const char*>(realGetString(kExtensionsEnum));
    supported = ExtensionsStringContainsWholeWord(extensions, extensionName) ||
                (extensionNameAlias != nullptr &&
                 ExtensionsStringContainsWholeWord(extensions, extensionNameAlias));
  }
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
// Also accepts the older GL_EXT_pixel_buffer_object as sufficient, not
// only GL_ARB_pixel_buffer_object. Confirmed via third_party/OpenGL-Registry/xml/gl.xml: EXT's
// own <require> block defines GL_PIXEL_PACK_BUFFER_EXT/GL_PIXEL_UNPACK_BUFFER_EXT/
// GL_PIXEL_PACK_BUFFER_BINDING_EXT/GL_PIXEL_UNPACK_BUFFER_BINDING_EXT at the exact same numeric
// values (0x88EB/0x88EC/0x88ED/0x88EF) this file's own kPixelPackBuffer/kPixelUnpackBuffer/
// kPixelPackBufferBinding/kPixelUnpackBufferBinding constants already use for the ARB/core
// enums - EXT_pixel_buffer_object is not merely similarly-named, it is bit-for-bit the same
// binding-point enum surface ARB_pixel_buffer_object (and later core 2.1) reused verbatim.
// A pre-2.1 context exposing only the EXT form (no ARB string) would previously fail this
// check outright, making ContextSupportsPixelBufferObject() return false and GuardPixelsPointer
// skip its bound-buffer check entirely on such a context - exactly the "dereference a
// buffer-relative byte offset as a real client pointer" hazard this guard exists to prevent,
// on a context that in fact has a pixel buffer bound via the EXT path.
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

// Builds and records the MappedDataMetaCommand for a just-captured snapshot (see Decision 6).
// `key` must already be reserved by the caller via CaptureManager::CreateCommandKey(), before
// the real glUnmapBuffer dispatch - see CustomWrapper_glUnmapBuffer's own comment on why (mirrors
// recorderWrappersAuto.cpp.mako's identical pre-dispatch reservation for every generated
// wrapper). `bytes.size()` is passed straight through to ArrayArgument<uint8_t>'s own templated,
// size_t-based constructor with no narrowing cast here - a mapped range can legitimately
// approach or exceed 4 GiB on this x64-only backend (see mappedDataMetaTypes.h's own comment on
// why MappedDataMetaCommand's coding path is 64-bit throughout); narrowing to uint32_t here,
// before ever reaching that already-64-bit-safe coding path, would silently reintroduce the
// exact truncation bug that path was fixed to avoid.
void RecordMappedDataMetaCommand(uint64_t key,
                                 GLuint buffer,
                                 GLintptr offset,
                                 GLsizeiptr length,
                                 const std::vector<uint8_t>& bytes) {
  MappedDataMetaCommand command(static_cast<uint32_t>(GetCurrentThreadId()));
  command.m_Buffer.Value = buffer;
  UpdateHandle(command.m_Buffer, GLObjectKind::Buffer);
  command.m_Offset.Value = static_cast<uint64_t>(offset);
  command.m_Length.Value = static_cast<uint64_t>(length);
  command.m_Data = ArrayArgument<uint8_t>(bytes.data(), bytes.size());
  command.m_Key = key;
  CaptureManager::Get().RecordCommand(command, new MappedDataMetaSerializer(command));
}

void* APIENTRY CustomWrapper_glMapBufferRange(GLenum target,
                                              GLintptr offset,
                                              GLsizeiptr length,
                                              GLbitfield access) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* result = realFunction(target, offset, length, access);
  if (record && result != nullptr) {
    const GLuint buffer = ResolveBoundBuffer(target);
    if (buffer != 0) {
      MapTrackingService::Get().StoreData(CurrentShareGroup(), buffer,
                                          MappedRange{result, offset, length, access});
    }
  }
  return result;
}

GLboolean APIENTRY CustomWrapper_glUnmapBuffer(GLenum target) {
  const RecursionGuard guard;
  const bool record = !guard.IsNested();
  auto realFunction = g_DispatchTable.glUnmapBuffer.load(std::memory_order_acquire);
  if (!record) {
    return realFunction(target);
  }
  const GLuint buffer = ResolveBoundBuffer(target);
  std::optional<MappedRange> range =
      buffer != 0 ? MapTrackingService::Get().TakeData(CurrentShareGroup(), buffer) : std::nullopt;
  // Nothing tracked (map wasn't via glMapBufferRange, buffer unresolved, ...), or a
  // genuinely read-only mapping - see Decision 5's third bullet. Skip straight to the real
  // unmap; no MappedDataMetaCommand, so no ordering key is reserved for this call at all.
  if (!range || (range->Access & kMapWriteBit) == 0) {
    return realFunction(target);
  }
  // Reserved here - before the real unmap dispatch below, for both the read-mapping and
  // write-only branches - rather than inside RecordMappedDataMetaCommand() after the fact, for
  // the same reason every generated wrapper reserves its own key before dispatch (see
  // recorderWrappersAuto.cpp.mako's header comment): a concurrent top-level call on another
  // thread must not be able to race this one's key assignment based on which driver call
  // happens to return first. Every path below that reaches this point (i.e. a
  // MappedDataMetaCommand is genuinely intended) must either record a command with this key
  // (RecordMappedDataMetaCommand) or explicitly release it via CaptureManager::SkipCommandKey -
  // never just drop it, or OrderingRecorder::CheckPendingSerializers() would stall forever
  // waiting for a key that will never arrive.
  const uint64_t key = CaptureManager::Get().CreateCommandKey();
  std::vector<uint8_t> bytes(static_cast<size_t>(range->Length));
  if (range->Access & kMapReadBit) {
    // Safe to read directly through the still-live original pointer, right before the real
    // unmap invalidates it - see Decision 5's first bullet.
    std::memcpy(bytes.data(), range->Pointer, bytes.size());
    const GLboolean result = realFunction(target);
    if (result == kGlTrue) {
      RecordMappedDataMetaCommand(key, buffer, range->Offset, range->Length, bytes);
    } else {
      // GL_FALSE: the data store's contents are corrupted/undefined per spec (see kGlTrue's
      // own comment) - `bytes` was captured from the pointer that was live a moment ago, but
      // that snapshot no longer reflects anything meaningful now that the real unmap reports
      // failure. Recording it anyway would have replay inject bytes capture itself never
      // validly established.
      LOG_WARNING << "CustomWrapper_glUnmapBuffer: real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      // `key` was already reserved above - it must still be released even though nothing will
      // ever be recorded under it, or OrderingRecorder would wait for it forever (see this
      // function's own comment above `key`'s declaration).
      CaptureManager::Get().SkipCommandKey(key);
    }
    return result;
  }
  // Write-only (or write-plus-invalidate): the original pointer is not guaranteed readable per
  // spec - see Decision 5's second bullet. Real unmap first (the one, app-visible unmap - its
  // result is what the app's own call returns), then a temporary read-only remap/memcpy/unmap
  // purely for capture, mirroring CgitsUnmapBuffer's own fallback.
  const GLboolean result = realFunction(target);
  auto realMapBufferRange = g_DispatchTable.glMapBufferRange.load(std::memory_order_acquire);
  void* readPointer = realMapBufferRange(target, range->Offset, range->Length, kMapReadBit);
  if (readPointer != nullptr) {
    std::memcpy(bytes.data(), readPointer, bytes.size());
    realFunction(target); // Second, purely internal unmap - its result is not the app's.
    if (result == kGlTrue) {
      RecordMappedDataMetaCommand(key, buffer, range->Offset, range->Length, bytes);
    } else {
      // GL_FALSE from the app's own (first) unmap above: the data store's contents are
      // corrupted/undefined per spec, even though this temporary re-map/memcpy itself
      // succeeded (reading back through valid, driver-owned memory whose *contents* are
      // simply meaningless now, not invalid memory - see kGlTrue's own comment). Recording it
      // anyway would have replay inject bytes capture never validly established.
      LOG_WARNING << "CustomWrapper_glUnmapBuffer: real unmap of buffer " << buffer
                  << " returned GL_FALSE (data store contents corrupted per spec) - no "
                     "MappedDataMetaCommand recorded for this unmap";
      CaptureManager::Get().SkipCommandKey(key);
    }
  } else {
    LOG_WARNING << "CustomWrapper_glUnmapBuffer: read-only re-map of buffer " << buffer
                << " failed - no MappedDataMetaCommand recorded for this unmap";
    // `key` was already reserved above - it must still be released even though nothing will
    // ever be recorded under it, or OrderingRecorder would wait for it forever (see this
    // function's own comment above `key`'s declaration).
    CaptureManager::Get().SkipCommandKey(key);
  }
  return result;
}

} // namespace

const void* GuardUnpackPixelsPointer(const char* commandName, const void* pixels) {
  return GuardPixelsPointer(commandName, pixels, kPixelUnpackBufferBinding);
}

const void* GuardPackPixelsPointer(const char* commandName, const void* pixels) {
  return GuardPixelsPointer(commandName, pixels, kPixelPackBufferBinding);
}

// See these two functions' own header comment
// (mapTrackingWrappers.h) for the full rationale: a PIXEL_TRANSFER_SIZE-shaped command's
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
  return PixelBufferObjectIsBound(kPixelUnpackBufferBinding);
}

bool PackPixelsPointerIsBufferOffset(const void* /*pixels*/) {
  return PixelBufferObjectIsBound(kPixelPackBufferBinding);
}

// See this function's own header comment (mapTrackingWrappers.h) for the
// full rationale. Structurally mirrors GuardPixelsPointer() above (quiet binding query, warn-
// once, nullptr-on-bound), kept as its own function (rather than a third bindingPname argument
// to GuardPixelsPointer itself) because the two guards protect opposite directions
// (GuardPixelsPointer's callers are IN-direction pre-call reads; this guard's own callers are
// OUT-direction post-call reads) with their own, distinct warning text, and share no callers
// today. Uses g_QueryBufferGuardMutex/g_WarnedQueryBufferGuardCommands (declared above, inside
// the anonymous namespace above - visible here via the unnamed namespace's implicit
// using-directive into this translation unit, same as GuardPixelsPointer's own use of
// QueryBoundBufferQuiet()/kPixelUnpackBufferBinding/kPixelPackBufferBinding from that same
// scope).
const void* GuardQueryBufferPointer(const char* commandName, const void* params) {
  if (params == nullptr || !ContextSupportsQueryBufferObject() ||
      QueryBoundBufferQuiet(kQueryBufferBinding) == 0) {
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

GLint QueryUnpackAlignment() {
  GLint value = 4;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kUnpackAlignment, &value);
  return value;
}

GLint QueryPackAlignment() {
  GLint value = 4;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kPackAlignment, &value);
  return value;
}

GLint QueryUnpackImageHeight() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kUnpackImageHeight, &value);
  return value;
}

GLint QueryUnpackSkipImages() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kUnpackSkipImages, &value);
  return value;
}

GLint QueryPackImageHeight() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kPackImageHeight, &value);
  return value;
}

GLint QueryPackSkipImages() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kPackSkipImages, &value);
  return value;
}

// See pixelTransferSize.h's own comment on QueryUnpackRowLength() et al.'s
// declarations for why these six exist and why every enabled pixel-transfer call site queries
// them unconditionally now, unlike QueryUnpackImageHeight() et al. above.
GLint QueryUnpackRowLength() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kUnpackRowLength, &value);
  return value;
}

GLint QueryUnpackSkipRows() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kUnpackSkipRows, &value);
  return value;
}

GLint QueryUnpackSkipPixels() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kUnpackSkipPixels, &value);
  return value;
}

GLint QueryPackRowLength() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kPackRowLength, &value);
  return value;
}

GLint QueryPackSkipRows() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kPackSkipRows, &value);
  return value;
}

GLint QueryPackSkipPixels() {
  GLint value = 0;
  auto realGetIntegerv = g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
  realGetIntegerv(kPackSkipPixels, &value);
  return value;
}

void* GetCustomRecorderWrapper(const char* name) {
  if (std::strcmp(name, "glMapBufferRange") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glMapBufferRange);
  }
  if (std::strcmp(name, "glUnmapBuffer") == 0) {
    return reinterpret_cast<void*>(&CustomWrapper_glUnmapBuffer);
  }
  return nullptr;
}

} // namespace gits::opengl2
