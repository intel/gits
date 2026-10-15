// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Generic, type-driven GetSize/Encode/Decode overloads - the building blocks that
// generated per-command coders (commandCodersAuto.cpp) are composed from. Mirrors
// Vulkan/common/coders/argumentCoders.h, scoped down to the wrapper kinds arguments.h
// currently defines. A generated command coder never memcpy's a field itself; it just
// calls GetSize/Encode/Decode on each member in a fixed order (m_Key, m_ThreadId, each
// parameter, m_Return) and these overloads resolve per member type.
//
// GetSize returns uint64_t, and Encode/Decode's running `offset` accumulator is uint64_t,
// throughout this whole file (including the plain-scalar/Argument<T>/HandleArgument<T>
// overloads whose OWN payload can never be large) rather than only on the specific overloads
// whose own payload can be large: every overload's
// `offset` parameter is one shared accumulator threaded through an entire command's fields in
// a fixed order (commandCodersAuto.cpp.mako's generated Encode/Decode), so narrowing any one
// overload's own accumulator width back down to uint32_t would silently reintroduce overflow
// risk for every field encoded/decoded AFTER a large one, even if that specific overload's own
// arithmetic looks safe in isolation - there is exactly one accumulator per command, and it
// must stay wide for the whole walk, not just the one field that happens to need it.
//
// ArrayArgument<T>/HandleArrayArgument<T>/ArrayOfArrays<T>'s own wire-level element count(s)
// are likewise uint64_t, not uint32_t, mirroring Vulkan/common/layer_interface/arguments.h's
// BufferArgument, which already writes a full size_t length on the wire for exactly this
// reason (a large-buffer payload, e.g. this backend's glBufferData/glBufferSubData `data`
// parameter capturing a GLsizeiptr-sized ArrayArgument<uint8_t>, can legitimately approach or
// exceed 4 GiB - see arguments.h's own comment on why ArrayArgument<T>'s IN-MEMORY count was
// already widened to size_t for this same reason). A uint32_t wire-level count would silently
// truncate - to exactly 0 at precisely 4 GiB - independently of, and in addition to, the
// shared-accumulator overflow risk above; widening only the accumulator without also widening
// the count itself would still corrupt exactly this scenario.
//
// This mirrors, and now subsumes, mappedDataMetaTypes.h's own previously-bespoke 64-bit
// GetMappedPayloadSize/EncodeMappedPayload/DecodeMappedPayload - those existed only because
// this file's own overloads were 32-bit at the time; now that ArrayArgument<uint8_t>'s generic
// overload is 64-bit-safe end-to-end too, MappedDataMetaCommand's own m_Data field uses this
// file's ordinary ArrayArgument<T> overload directly, like every other field on every other
// command - see that header's own comment.

#pragma once

#include "arguments.h"

#include <cstdint>
#include <cstring>

namespace gits::opengl2 {

// Plain scalar (also covers command envelope fields like m_Key/m_ThreadId, which are not
// wrapped in Argument<T>): sizeof(T) raw bytes, byte-identical in and out.
template <typename T>
uint64_t GetSize(const T& arg) {
  return sizeof(arg);
}

template <typename T>
void Encode(char* dest, uint64_t& offset, const T& arg) {
  std::memcpy(dest + offset, &arg, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
void Decode(const char* src, uint64_t& offset, T& arg) {
  std::memcpy(&arg, src + offset, sizeof(T));
  offset += sizeof(T);
}

// Argument<T>: same raw-bytes treatment, applied to .Value instead of the wrapper itself
// (the wrapper is a capture-time convenience, not part of the wire format).
template <typename T>
uint64_t GetSize(const Argument<T>& arg) {
  return sizeof(arg.Value);
}

template <typename T>
void Encode(char* dest, uint64_t& offset, const Argument<T>& arg) {
  std::memcpy(dest + offset, &arg.Value, sizeof(arg.Value));
  offset += sizeof(arg.Value);
}

template <typename T>
void Decode(const char* src, uint64_t& offset, Argument<T>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(arg.Value));
  offset += sizeof(arg.Value);
}

// HandleArgument<T>: only .Key crosses the wire, never the live .Value - a captured
// process-local handle means nothing in a different process/replay run. Decode leaves
// .Value at its default; resolving .Key back to a live handle is a player-side concern
// (see handleArgumentUpdaters.h and, eventually, its player-side counterpart) not yet
// implemented (no OpenGL2 player exists - see modern-opengl-backend-architecture.md).
template <typename T>
uint64_t GetSize(const HandleArgument<T>& arg) {
  return sizeof(arg.Key);
}

template <typename T>
void Encode(char* dest, uint64_t& offset, const HandleArgument<T>& arg) {
  std::memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

template <typename T>
void Decode(const char* src, uint64_t& offset, HandleArgument<T>& arg) {
  std::memcpy(&arg.Key, src + offset, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

// ArrayArgument<T>: wire format is a 1-byte presence flag, then - only if present - a
// uint64_t element count and count*sizeof(T) raw bytes. The explicit presence flag (rather
// than Vulkan's convention of writing a full sizeof(void*) "null or not" marker derived from
// the live pointer's bit pattern - see EncodeNullPtr in Vulkan/common/coders/argumentCoders.h)
// is a deliberate simplification: OpenGL2's wire format has no reason to reserve pointer-sized
// space for a value that is never a meaningful pointer once decoded, and a 1-byte flag is
// unambiguous and smaller. HasValue == false and HasValue == true with Data.empty() are
// distinguished exactly as arguments.h's ArrayArgument<T> comment requires. The element count
// is uint64_t, not uint32_t - see this header's own top-of-file comment.
template <typename T>
uint64_t GetSize(const ArrayArgument<T>& arg) {
  uint64_t size = sizeof(uint8_t);
  if (arg.HasValue) {
    size += sizeof(uint64_t) + static_cast<uint64_t>(arg.Data.size()) * sizeof(T);
  }
  return size;
}

template <typename T>
void Encode(char* dest, uint64_t& offset, const ArrayArgument<T>& arg) {
  const uint8_t hasValue = arg.HasValue ? 1 : 0;
  std::memcpy(dest + offset, &hasValue, sizeof(hasValue));
  offset += sizeof(hasValue);
  if (!arg.HasValue) {
    return;
  }
  const uint64_t count = static_cast<uint64_t>(arg.Data.size());
  std::memcpy(dest + offset, &count, sizeof(count));
  offset += sizeof(count);
  if (count > 0) {
    std::memcpy(dest + offset, arg.Data.data(), static_cast<size_t>(count) * sizeof(T));
    offset += count * sizeof(T);
  }
}

template <typename T>
void Decode(const char* src, uint64_t& offset, ArrayArgument<T>& arg) {
  uint8_t hasValue{};
  std::memcpy(&hasValue, src + offset, sizeof(hasValue));
  offset += sizeof(hasValue);
  arg.HasValue = hasValue != 0;
  if (!arg.HasValue) {
    arg.Data.clear();
    return;
  }
  uint64_t count{};
  std::memcpy(&count, src + offset, sizeof(count));
  offset += sizeof(count);
  arg.Data.resize(static_cast<size_t>(count));
  if (count > 0) {
    std::memcpy(arg.Data.data(), src + offset, static_cast<size_t>(count) * sizeof(T));
    offset += count * sizeof(T);
  }
}

// HandleArrayArgument<T>: same presence-flag + count-prefixed layout as ArrayArgument<T>, but
// the payload is .Key (one GITSKey per element), never .Value - exactly like HandleArgument<T>
// only ever serializing .Key, never .Value. Decode leaves every .Value element at its default
// and resizes .Value to match .Key's decoded count, so a decoded HandleArrayArgument<T> already
// has a correctly-sized (if not yet resolved) Value buffer a player can pass straight to a
// driver call that writes into it (see commandRunnersAuto.cpp.mako) without a separate resize
// step - resolving/minting the live names themselves is a player/recorder-side concern (see
// handleArgumentUpdaters.h on both sides), not this coder's. The element count is uint64_t,
// not uint32_t - see this header's own top-of-file comment.
template <typename T>
uint64_t GetSize(const HandleArrayArgument<T>& arg) {
  uint64_t size = sizeof(uint8_t);
  if (arg.HasValue) {
    size += sizeof(uint64_t) + static_cast<uint64_t>(arg.Key.size()) * sizeof(GITSKey);
  }
  return size;
}

template <typename T>
void Encode(char* dest, uint64_t& offset, const HandleArrayArgument<T>& arg) {
  const uint8_t hasValue = arg.HasValue ? 1 : 0;
  std::memcpy(dest + offset, &hasValue, sizeof(hasValue));
  offset += sizeof(hasValue);
  if (!arg.HasValue) {
    return;
  }
  const uint64_t count = static_cast<uint64_t>(arg.Key.size());
  std::memcpy(dest + offset, &count, sizeof(count));
  offset += sizeof(count);
  if (count > 0) {
    std::memcpy(dest + offset, arg.Key.data(), static_cast<size_t>(count) * sizeof(GITSKey));
    offset += count * sizeof(GITSKey);
  }
}

template <typename T>
void Decode(const char* src, uint64_t& offset, HandleArrayArgument<T>& arg) {
  uint8_t hasValue{};
  std::memcpy(&hasValue, src + offset, sizeof(hasValue));
  offset += sizeof(hasValue);
  arg.HasValue = hasValue != 0;
  if (!arg.HasValue) {
    arg.Key.clear();
    arg.Value.clear();
    return;
  }
  uint64_t count{};
  std::memcpy(&count, src + offset, sizeof(count));
  offset += sizeof(count);
  arg.Key.resize(static_cast<size_t>(count));
  if (count > 0) {
    std::memcpy(arg.Key.data(), src + offset, static_cast<size_t>(count) * sizeof(GITSKey));
    offset += count * sizeof(GITSKey);
  }
  arg.Value.assign(static_cast<size_t>(count), T{});
}

// ArrayOfArrays<T>: presence flag, then - only if present - an outer uint64_t element count
// followed by, per element, an int32_t Lengths[i] (gl.xml's own length semantic - see
// arguments.h's comment, always a plain GL parameter value in its own right, never itself a
// wire-format array length that needs 64-bit range) and a uint64_t/raw-bytes pair for that
// element's Value[i], exactly like a nested ArrayArgument<T> per element. No per-element
// presence flag: a null individual string pointer (see arguments.h's constructor) already
// collapses to an empty Value[i], which round-trips correctly as a zero-length element
// without needing its own flag - see that constructor's own comment for why the null/empty
// distinction is deliberately not tracked any more precisely than that on either side of the
// wire. Both the outer and each per-element count are uint64_t, not uint32_t - see this
// header's own top-of-file comment.
template <typename T>
uint64_t GetSize(const ArrayOfArrays<T>& arg) {
  uint64_t size = sizeof(uint8_t);
  if (arg.HasValue) {
    size += sizeof(uint64_t);
    for (const auto& element : arg.Value) {
      size +=
          sizeof(int32_t) + sizeof(uint64_t) + static_cast<uint64_t>(element.size()) * sizeof(T);
    }
  }
  return size;
}

template <typename T>
void Encode(char* dest, uint64_t& offset, const ArrayOfArrays<T>& arg) {
  const uint8_t hasValue = arg.HasValue ? 1 : 0;
  std::memcpy(dest + offset, &hasValue, sizeof(hasValue));
  offset += sizeof(hasValue);
  if (!arg.HasValue) {
    return;
  }
  const uint64_t outerCount = static_cast<uint64_t>(arg.Value.size());
  std::memcpy(dest + offset, &outerCount, sizeof(outerCount));
  offset += sizeof(outerCount);
  for (uint64_t i = 0; i < outerCount; ++i) {
    std::memcpy(dest + offset, &arg.Lengths[i], sizeof(int32_t));
    offset += sizeof(int32_t);
    const uint64_t elementCount = static_cast<uint64_t>(arg.Value[i].size());
    std::memcpy(dest + offset, &elementCount, sizeof(elementCount));
    offset += sizeof(elementCount);
    if (elementCount > 0) {
      std::memcpy(dest + offset, arg.Value[i].data(),
                  static_cast<size_t>(elementCount) * sizeof(T));
      offset += elementCount * sizeof(T);
    }
  }
}

template <typename T>
void Decode(const char* src, uint64_t& offset, ArrayOfArrays<T>& arg) {
  uint8_t hasValue{};
  std::memcpy(&hasValue, src + offset, sizeof(hasValue));
  offset += sizeof(hasValue);
  arg.HasValue = hasValue != 0;
  if (!arg.HasValue) {
    arg.Value.clear();
    arg.Lengths.clear();
    return;
  }
  uint64_t outerCount{};
  std::memcpy(&outerCount, src + offset, sizeof(outerCount));
  offset += sizeof(outerCount);
  arg.Value.resize(static_cast<size_t>(outerCount));
  arg.Lengths.resize(static_cast<size_t>(outerCount));
  for (uint64_t i = 0; i < outerCount; ++i) {
    std::memcpy(&arg.Lengths[i], src + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    uint64_t elementCount{};
    std::memcpy(&elementCount, src + offset, sizeof(elementCount));
    offset += sizeof(elementCount);
    arg.Value[i].resize(static_cast<size_t>(elementCount));
    if (elementCount > 0) {
      std::memcpy(arg.Value[i].data(), src + offset, static_cast<size_t>(elementCount) * sizeof(T));
      offset += elementCount * sizeof(T);
    }
  }
}

} // namespace gits::opengl2
