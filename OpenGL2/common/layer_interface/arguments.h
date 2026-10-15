// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gits::opengl2 {

// Clamps a signed count to zero before narrowing to size_t - used by every IN-direction
// eager-copy constructor below (ArrayArgument<T>, HandleArrayArgument<T>, ArrayOfArrays<T>) for
// their sibling-length ('n'/'count'/'size'/...) parameter. GL defines a negative count as a
// client error the real driver itself must reject, not something the recorder may crash on: a
// bare `static_cast<size_t>(-1)` turns into a ~16-exabyte value and drives a same-sized
// std::vector::assign()/resize() here, before the (invalid) call is even forwarded - see
// modern-opengl-backend-architecture.md's numeric-safety decision record. Only affects this
// backend's own CAPTURE bookkeeping (what gets copied into the Command for serialization) -
// callers still forward the original, unclamped parameter value to the real driver call
// unchanged (see recorderWrappersAuto.cpp.mako's realFunction(...) call, which always uses the
// wrapper's own original parameters, never anything read back out of a Command field), so a
// negative count still reaches the real driver exactly as the application passed it and fails
// there the same way it would have without GITS in the picture. Templated (SizeT deduced at
// each call site) since the actual GL sibling type varies - GLsizei (32-bit signed) for most
// callers, GLsizeiptr (64-bit signed) for glBufferData/glBufferSubData's `size`.
template <typename SizeT>
size_t ClampCountToSizeT(SizeT count) {
  return count > 0 ? static_cast<size_t>(count) : size_t{0};
}

template <typename CharT, typename LengthT>
size_t TextOutputLength(const CharT* buffer, size_t available, const LengthT* reportedLength) {
  if (!buffer || !available) {
    return 0;
  }
  const size_t limit =
      reportedLength ? std::min(available, ClampCountToSizeT(*reportedLength)) : available;
  return static_cast<size_t>(std::find(buffer, buffer + limit, CharT{}) - buffer);
}

template <typename CharT, typename LengthT>
size_t TextOutputCaptureSize(const CharT* buffer, LengthT capacity, const LengthT* reportedLength) {
  const size_t available = buffer ? ClampCountToSizeT(capacity) : 0;
  const size_t length = TextOutputLength(buffer, available, reportedLength);
  if (length < available && buffer[length] == CharT{}) {
    return length + 1;
  }
  return length;
}

// Stream-stable object id, minted by CaptureManager::CreateHandleKey() and looked up via
// ContextHandleMapService - see modern-opengl-backend-architecture.md's "Real key-based
// mapping for handles GITS creates" decision. Same purpose and bit width as Vulkan's
// GITSKey (Vulkan/common/layer_interface/arguments.h), kept as an independent typedef
// rather than shared, per "Keep abstractions backend-local until reuse is demonstrated".
typedef uint64_t GITSKey;

// Deliberately small subset of Vulkan's arguments.h: only the wrapper kinds every
// currently-enabled command needs (TypeFacts.is_scalar / is_opaque_handle -> Argument<T>,
// TypeFacts.is_mapped_handle -> HandleArgument<T>, is_array_element_eligible-with-an-override
// -> ArrayArgument<T>, a GL object name (GLuint) with an object_name_lifecycle.py override ->
// HandleArgument<T>/HandleArrayArgument<T>, a doubly-indirect pointer
// (is_array_of_arrays_eligible, glShaderSource's `string`) -> ArrayOfArrays<T>, see below).
// Vulkan's PointerArgument/HandleArrayOutputArgument/etc. have no OpenGL2 equivalent yet -
// COMPSIZE-style lengths remain deferred (see modern-opengl-backend-architecture.md's
// scale_pointer decision record). Unlike Vulkan, OpenGL2 has exactly one HandleArrayArgument<T>
// shape covering both the array-input (glDeleteTextures) and array-output (glGenTextures) case -
// see HandleArrayArgument<T>'s own comment for why one type suffices here.

// Plain fixed-size field: a real scalar (GLenum, GLfloat, ...) or an opaque native handle
// captured verbatim (HDC - GITS never intercepts its creation, so there is no key to give
// it; see intermediates.py's _OPAQUE_HANDLE_BASE_TYPES). Either way the wire format is just
// sizeof(T) raw bytes of Value - see argumentCoders.h.
template <typename T>
struct Argument {
  T Value{};
  Argument() {}
  Argument(T v) : Value(v) {}
};

// A handle GITS owns the full lifecycle of (currently only HGLRC - see intermediates.py's
// _MAPPED_HANDLE_BASE_TYPES). Value is the live, process-local handle; Key is the
// stream-stable id that actually goes on the wire (see argumentCoders.h) - mirrors
// Vulkan's HandleArgument<T> exactly, just for a single handle type instead of a bucketed
// multi-type map (see ContextHandleMapService).
template <typename T>
struct HandleArgument {
  T Value{};
  GITSKey Key{};
  HandleArgument() {}
  HandleArgument(T v) : Value(v) {}
};

// Tag type selecting the null-terminated-scan constructor overload below, so
// ArrayArgument<T>(ptr, count) and ArrayArgument<T>(ptr, NullTerminated) can never be
// ambiguous regardless of T (unlike, say, overloading on a second T parameter for a
// sentinel value, which would collide with a same-typed count parameter for small integer
// T like GLchar/uint8_t).
struct NullTerminatedTag {};
inline constexpr NullTerminatedTag NullTerminated{};

// Tag type selecting the pairs-aware key/value-list-scan constructor overload below - same
// disambiguation rationale as NullTerminatedTag above, just for a second, differently-shaped
// termination rule (see pointer_metadata.py's LengthSourceKind.KEY_VALUE_LIST for why a WGL
// attribute list needs its own scan instead of reusing NullTerminatedTag's).
struct KeyValueListTag {};
inline constexpr KeyValueListTag KeyValueList{};

// A single-indirection pointer to a fixed-size element type (TypeFacts.is_array_element_eligible
// - see intermediates.py), captured as an owned copy. Deliberately does NOT keep a raw pointer
// member the way Vulkan's ArrayArgument/PointerArgument do: OpenGL2's generated wrapper
// (recorderWrappersAuto.cpp.mako) never reads member state to make the real driver call - it
// always uses the original function parameters directly - so there is no code path that needs a
// live pointer back into application memory once the copy exists. This also means no hand-written
// copy/move constructor is needed (contrast Vulkan's PointerArgument): std::vector's own copy/move
// semantics are already correct, since nothing aliases into Data from outside the class.
//
// HasValue distinguishes a null pointer (HasValue == false, Data empty) from a valid, explicitly
// zero-length array (HasValue == true, Data empty) - see argumentCoders.h for why this matters
// for round-trip fidelity.
template <typename T>
struct ArrayArgument {
  std::vector<T> Data{};
  bool HasValue{false};

  ArrayArgument() {}

  // Eager copy at construction - used for IN-direction parameters, where the Command
  // constructor runs before the real driver call (see modern-opengl-backend-architecture.md's
  // "Classify payloads from evidence" decision), satisfying "required input bytes must be
  // copied from application-owned memory immediately before the driver call."
  //
  // Templated SizeT (mirrors Vulkan's BufferArgument - Vulkan/common/layer_interface/
  // arguments.h - exactly, same rationale): accepts whatever width the actual GL sibling
  // parameter is (GLsizei for most SIBLING_PARAM callers, GLsizeiptr for glBufferData/
  // glBufferSubData's `size`) and narrows via ClampCountToSizeT(), sidestepping MSVC's C2398
  // brace-init narrowing check the same way a per-call cast would, without scattering those
  // casts through codegen templates. Clamps a negative count to zero first, exactly like
  // ArrayOfArrays<T>'s own constructor (see that constructor's own comment and
  // ClampCountToSizeT's) - an application passing a negative size/count to an enabled command
  // is a GL client error, not something the recorder may itself crash/OOM on by narrowing it
  // into a ~16-exabyte unsigned value first; see ClampCountToSizeT's own comment for why this
  // capture-side clamp does not change what the real driver call itself receives.
  template <typename SizeT>
  ArrayArgument(const T* v, SizeT count) {
    if (v) {
      HasValue = true;
      const size_t safeCount = ClampCountToSizeT(count);
      Data.assign(v, v + safeCount);
    }
  }

  // Scans forward from v for a default-constructed T{} sentinel (0 for every integer/enum
  // element type this increment uses) and captures through and including it, so a decoded
  // buffer is itself already a valid null-terminated array a player can pass straight back to
  // a replay call with no reconstruction step. Always IN-direction in this increment (see
  // pointer_metadata.py's PointerDirection - nothing produces a null-terminated OUTPUT array
  // in the initial command set).
  ArrayArgument(const T* v, NullTerminatedTag) {
    if (v) {
      HasValue = true;
      const T* end = v;
      while (!(*end == T{})) {
        ++end;
      }
      Data.assign(v, end + 1);
    }
  }

  // Scans forward from v in (key, value) PAIRS for a pair whose KEY (the even-indexed
  // element - v[0], v[2], v[4], ...) is a default-constructed T{} sentinel, and captures
  // through and including that terminating key - see pointer_metadata.py's
  // LengthSourceKind.KEY_VALUE_LIST for why this must check only every other element instead
  // of every element the way the plain NullTerminatedTag constructor above does: a zero VALUE
  // (the odd-indexed element) is legal WGL attribute-list content (e.g.
  // WGL_CONTEXT_FLAGS_ARB's value can legitimately be 0) and must not be mistaken for the
  // terminator. Assumes a well-formed input list (an even number of (key, value) elements
  // before the terminating zero key) exactly as NullTerminatedTag's own scan assumes a real
  // terminator exists at all - a malformed list is already undefined behavior for the real
  // driver call this capture is standing in front of, not a new risk this constructor
  // introduces.
  ArrayArgument(const T* v, KeyValueListTag) {
    if (v) {
      HasValue = true;
      const T* end = v;
      while (!(*end == T{})) {
        end += 2; // Skip this pair's key and value; land on the next pair's key.
      }
      Data.assign(v, end + 1);
    }
  }

  // Same eager-copy timing as the IN constructor above, for the opaque-pointer-array shape
  // (glMultiDrawElementsBaseVertex's `indices` - see pointer_metadata.py's
  // LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY, and commandsAuto.h.mako's own construction
  // branch for it). Takes `const void* const*` directly and converts each element with its
  // own reinterpret_cast<uintptr_t>(...) - a well-defined pointer-to-integer conversion
  // ([expr.reinterpret.cast]) - rather than the call site reinterpreting the whole array's
  // pointer type first (`reinterpret_cast<const uintptr_t*>(v)`) and bulk-copying through
  // that via the plain `ArrayArgument(const T*, SizeT)` constructor above: that would read
  // each element's stored bytes through a type (uintptr_t) other than the one they were
  // actually stored as (const void*) - a strict-aliasing violation ([basic.lval]), not merely
  // a style concern. A real compiler's alias analysis is entitled to treat such a read as not
  // aliasing the original `const void*` store, which can silently miscompile exactly this
  // bulk-copy-through-a-reinterpreted-pointer pattern (undefined behavior is not "usually
  // works," even where it happens to on today's compilers/flags). Only ever instantiated with
  // T = uintptr_t in practice (this repo's sole OPAQUE_BIT_PATTERN_ARRAY user) - kept generic
  // in T anyway for consistency with every other constructor in this class.
  template <typename SizeT>
  ArrayArgument(const void* const* v, SizeT count) {
    if (v) {
      HasValue = true;
      const size_t safeCount = ClampCountToSizeT(count);
      Data.resize(safeCount);
      for (size_t i = 0; i < safeCount; ++i) {
        Data[i] = static_cast<T>(reinterpret_cast<uintptr_t>(v[i]));
      }
    }
  }

  // Populates an OUT-direction parameter after the real driver call has written into the
  // application's own buffer. Deliberately a plain method, not a constructor - an OUT
  // parameter's Command member is default-constructed (empty, HasValue == false) at
  // construction time, before the call, since the pointed-to memory is not yet meaningful.
  // Templated SizeT for the same reason as the IN constructor above; clamps a negative count
  // to zero the same way too, for the same reason (see the IN constructor's own comment).
  template <typename SizeT>
  void Capture(const T* v, SizeT count) {
    if (v) {
      HasValue = true;
      const size_t safeCount = ClampCountToSizeT(count);
      Data.assign(v, v + safeCount);
    }
  }
};

// An array of GL object names (GLuint texture/buffer names, ...) GITS remaps - the array
// counterpart of HandleArgument<T>, for glGenTextures/glGenBuffers/glDeleteTextures (see
// docs/development/opengl2-object-name-mapping-plan.md). Mirrors ArrayArgument<T>'s shape and
// constructor/Capture() signatures exactly (same eager-copy-before-call / OUT-Capture-after-call
// timing - see modern-opengl-backend-architecture.md's "Classify payloads from evidence"), so
// commandsAuto.h.mako's existing IN/OUT construction logic (driven purely by
// pointer_metadata.py's PointerDirection, not by which wrapper type is being constructed) needs
// no changes to build one of these instead of a plain ArrayArgument<T>.
//
// Differs from ArrayArgument<T> in what crosses the wire: Value holds live, process-local GL
// object names (the recorder's raw driver-assigned names, or the player's own real names once
// resolved/newly-created - see recorder/include/handleArgumentUpdaters.h and
// player/include/handleArgumentUpdaters.h), while Key - not Value - is what argumentCoders.h
// actually serializes, one GITSKey per element, exactly like HandleArgument<T>::Key. Key is
// always the same length as Value; a default-constructed (all-zero) Key element means "not yet
// looked up/minted", filled in by an ObjectNameMapService call at the same points in
// recorderWrappersAuto.cpp.mako/commandRunnersAuto.cpp.mako that UpdateHandle/UpdateOutputHandle/
// ResolveHandle/RegisterCreatedHandle already run for a scalar HandleArgument<T>.
template <typename T>
struct HandleArrayArgument {
  std::vector<T> Value{};
  std::vector<GITSKey> Key{};
  bool HasValue{false};

  HandleArrayArgument() {}

  // Eager copy at construction, for an IN-direction array of *existing* names (glDeleteTextures)
  // - Key stays zero-filled until the recorder wrapper looks up each name's key afterward (see
  // recorder/include/handleArgumentUpdaters.h's UpdateHandleArray). Mirrors ArrayArgument<T>'s
  // own IN constructor exactly, including its templated SizeT and its ClampCountToSizeT() use
  // (see that constructor's comment for why this must run before the real driver call, why a
  // negative count is clamped here rather than trusted, and for the Vulkan BufferArgument
  // parity this templated shape follows).
  template <typename SizeT>
  HandleArrayArgument(const T* v, SizeT count) {
    if (v) {
      HasValue = true;
      const size_t safeCount = ClampCountToSizeT(count);
      Value.assign(v, v + safeCount);
      Key.assign(safeCount, GITSKey{0});
    }
  }

  // Populates an OUT-direction array of *newly-created* names (glGenTextures/glGenBuffers) after
  // the real driver call has written into the application's own buffer - mirrors
  // ArrayArgument<T>::Capture() exactly, including its templated SizeT and clamp. Key stays
  // zero-filled until the recorder wrapper mints one key per element afterward (see
  // handleArgumentUpdaters.h's UpdateOutputHandleArray).
  template <typename SizeT>
  void Capture(const T* v, SizeT count) {
    if (v) {
      HasValue = true;
      const size_t safeCount = ClampCountToSizeT(count);
      Value.assign(v, v + safeCount);
      Key.assign(safeCount, GITSKey{0});
    }
  }
};

// Doubly-indirect array-of-strings parameter (glShaderSource's `string`/`length` pair) - the
// GL-flavored analogue of Vulkan's ArrayOfArrays<T> (Vulkan/common/layer_interface/arguments.h),
// adapted for a per-element length source that is itself conditional at the C level: gl.xml's
// own glShaderSource convention is that length[i] < 0 (or a null length array altogether) means
// "string i is null-terminated," while length[i] >= 0 gives an explicit byte count. See
// docs/development/opengl2-rendering-pipeline-pointer-support-plan.md.
//
// Always owns its bytes (mirrors ArrayArgument<T>'s "no borrowed pointer" rule - see that
// type's comment) via a vector of independently-sized, independently-owned byte buffers.
// Lengths stores gl.xml's own length[i] *semantic* value verbatim (a non-negative explicit
// byte count, or -1 for "null-terminated") rather than each Value[i]'s actual byte count, so a
// player can pass Lengths straight back to the real driver call unchanged (see
// argumentCoders.h/commandRunnersAuto.cpp.mako) and get bit-for-bit original call semantics -
// including which strings were null-terminated vs. explicitly sized at record time - without
// needing to re-derive that fact from Value's contents.
template <typename T>
struct ArrayOfArrays {
  std::vector<std::vector<T>> Value{};
  std::vector<int32_t> Lengths{};
  bool HasValue{false};

  ArrayOfArrays() {}

  // v: `count` pointers, each to an independently-owned buffer of element type T (one GL
  // "string"), or itself nullptr for an individual null string pointer (which stays an empty
  // Value[i] - see this constructor's own comment below for why the null/empty distinction is
  // deliberately not tracked any more precisely than that). lengths: optional (nullptr
  // allowed) array of `count` per-element lengths; lengths[i] < 0 (or lengths == nullptr)
  // means "scan v[i] for a null terminator instead" - mirrors glShaderSource's own `length`
  // parameter semantics exactly (see gl.xml).
  //
  // Templated SizeT and a size_t-width Value/Lengths count, for consistency with
  // ArrayArgument<T>/HandleArrayArgument<T>'s own templated, size_t-based constructors above -
  // glShaderSource's real `count` is a GLsizei (32-bit signed, no 64-bit variant anywhere in
  // gl.xml, unlike GLsizeiptr), so nothing here actually needs more than 32 bits of range
  // today, but using size_t uniformly for every count this backend stores avoids a second,
  // narrower convention to keep track of and preemptively covers any future
  // ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry with a wider outer-count type. Like those two types'
  // own constructors, this one clamps a negative count to zero via ClampCountToSizeT() first -
  // see that function's own comment for why (a client error the recorder must not crash on)
  // independently of this width choice; the clamp and the width are orthogonal fixes.
  //
  // A null v[i] and a non-null v[i] pointing at a real, explicitly-zero-length string
  // (lengths[i] == 0) both produce an empty Value[i], and that distinction is intentionally
  // NOT recorded anywhere (no per-element presence flag): GL itself draws no behavioral line
  // between "no string was ever supplied here" and "an unused, explicitly zero-length string
  // was supplied here" - a replayed call cannot observe which one actually happened, only that
  // element i contributed zero bytes either way. The one real hazard - reconstructing v[i] as
  // Value[i].data() when Value[i] happens to be empty is not guaranteed to yield a
  // dereferenceable non-null pointer - is a player-side pointer-safety concern with a
  // player-side fix (a stable dummy element substituted for an empty Value[i] ONLY when
  // Lengths[i] == 0 - the one case where zero bytes are guaranteed to ever be read through
  // that pointer; every other empty-Value[i] case, i.e. Lengths[i] > 0 or the -1
  // null-terminated sentinel, gets a real nullptr instead, reproducing the original call's own
  // null-source-pointer semantics rather than risking an over-read through a synthetic
  // address - see commandRunnersAuto.cpp.mako's own `<param>Ptrs[i]` rebuild loop for the exact
  // three-way split), not something that needs its own wire-visible bit here.
  template <typename SizeT>
  ArrayOfArrays(const T* const* v, SizeT count, const int32_t* lengths) {
    if (!v) {
      return;
    }
    HasValue = true;
    const size_t safeCount = ClampCountToSizeT(count);
    Value.resize(safeCount);
    Lengths.resize(safeCount);
    for (size_t i = 0; i < safeCount; ++i) {
      const bool explicitLength = lengths != nullptr && lengths[i] >= 0;
      Lengths[i] = explicitLength ? lengths[i] : -1;
      if (v[i] == nullptr) {
        continue; // An individual null string pointer stays an empty buffer; the real driver
                  // call would itself reject this, but capturing it verbatim is safer than
                  // dereferencing a null pointer here.
      }
      if (explicitLength) {
        Value[i].assign(v[i], v[i] + lengths[i]);
      } else {
        const T* end = v[i];
        while (!(*end == T{})) {
          ++end;
        }
        Value[i].assign(v[i], end + 1); // Include the terminator, like ArrayArgument<T>'s
                                        // NullTerminated constructor - see that type's comment.
      }
    }
  }
};

} // namespace gits::opengl2
