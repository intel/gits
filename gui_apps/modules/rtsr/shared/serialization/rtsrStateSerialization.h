// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsrPipelineStateStorage.h"
#include "rtsrSerdeTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// D3D12 pipeline state serialization in the JSON format RTSR and RTSREditor
// exchange. The documents produced here are byte compatible with the ones
// RTSR_StateSerialization produces; only the hash algorithm differs.
//
// The module deliberately depends on nothing but the DirectX headers, nlohmann
// json and xxHash, so that it can be linked into a GITS plugin as well as into
// the launcher UI.
namespace gits::rtsr {

enum class SerializationStatus {
  Success = 0,
  // The pipeline state handed to the serializer is incomplete or inconsistent,
  // for example an array pointer that is null while its element count is not.
  InvalidArgument,
  // The text handed to the deserializer is not a valid pipeline state document.
  InvalidData,
  // The document was produced for another graphics API than DirectX 12.
  UnsupportedApi,
  // The pipeline type has no representation in this format (mesh shaders).
  UnsupportedPipelineType,
  // Unexpected failure, for example an allocation that could not be satisfied.
  Fail
};

// Outcome of a conversion. Value is only meaningful when Ok(), Error carries a
// human readable description otherwise. This replaces RTSR's global
// std::ostringstream that a separate RTSR_SerializationError call drained.
template <typename T>
struct Result {
  SerializationStatus Status{SerializationStatus::Success};
  std::string Error;
  T Value{};

  bool Ok() const {
    return Status == SerializationStatus::Success;
  }
  explicit operator bool() const {
    return Ok();
  }
};

struct SerializedPipelineState {
  // Complete document, without a trailing null character. Use c_str() for
  // interfaces that expect a null terminated string.
  std::string Json;
  // Hash of the serialized state, also stored in the document as Header/Hash.
  uint64_t Hash{0};
};

using SerializeResult = Result<SerializedPipelineState>;
using DeserializeResult = Result<DeserializedPipelineState>;

// Serializes a graphics or compute pipeline state.
//
// This replaces RTSR's two pass convention, where
// RTSR_SerializePipelineState(input, &length, output) was called once to obtain
// the length and once more to fill a buffer of that length: the document is
// built once and returned by value. Its bytes are what RTSR's second pass
// writes; RTSR additionally padded its output buffer with zeros up to the
// length its first pass reported, which is not part of the document.
//
// Never throws, failures are reported through the returned status.
SerializeResult SerializePipelineState(const PipelineStateInfo& state);

// Deserializes a document produced by SerializePipelineState or by RTSR.
//
// The returned DeserializedPipelineState owns the PipelineStateInfo and every
// descriptor it points to, and they stay valid until it is destroyed. Several
// pipeline states can therefore be deserialized and used at the same time,
// which RTSR's single process wide memory pool did not allow.
//
// The text may be null terminated or padded with null characters, as RTSR
// transports it; everything from the first null character on is ignored.
//
// Never throws, failures are reported through the returned status.
DeserializeResult DeserializePipelineState(std::string_view serializedState);

} // namespace gits::rtsr
