// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsrStateSerialization.h"
#include "rtsrJsonSerialization.h"
#include "rtsrJsonDeserialization.h"

#include "xxhash.h"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace gits::rtsr {

namespace {

// RTSR hashes with CityHash64, this module uses the xxHash GITS already ships.
// Both are 64 bit hashes over the same text and the documents themselves are
// unchanged, so the only consequence is that RTSREditor's on disk cache, which
// is keyed by the hash, does not recognize states hashed here.
uint64_t Hash64(const void* data, size_t size) {
  return XXH64(data, size, 0);
}

uint64_t ComputeStateHash(const nlohmann::json& state) {
  std::string text = state.dump();
  // RTSR removes the structural characters before hashing, keeping only the
  // keys and values that describe the state.
  std::erase_if(text, [](char character) {
    return character == '{' || character == '}' || character == '[' || character == ']' ||
           character == '"';
  });
  return Hash64(text.data(), text.size());
}

} // namespace

SerializeResult SerializePipelineState(const PipelineStateInfo& state) {
  try {
    if (state.Type == PipelineStateType::MeshShader) {
      return {SerializationStatus::UnsupportedPipelineType,
              "Mesh shader pipeline states have no representation in the RTSR state format"};
    }

    const nlohmann::json serializedState = detail::ToJson(state);
    const uint64_t hash = ComputeStateHash(serializedState);

    const nlohmann::json header = {
        {"Api", SERIALIZED_API_DX12},
        {"Version", SERIALIZATION_VERSION},
        {"DeserializedSize", sizeof(PipelineStateInfo)},
        {"Hash", hash},
    };
    const nlohmann::json document = {
        {"Header", header},
        {"State", serializedState},
    };

    return {SerializationStatus::Success, {}, SerializedPipelineState{document.dump(), hash}};
  } catch (const std::invalid_argument& ex) {
    return {SerializationStatus::InvalidArgument, ex.what()};
  } catch (const std::exception& ex) {
    return {SerializationStatus::Fail, ex.what()};
  } catch (...) {
    return {SerializationStatus::Fail, "Unknown error while serializing a pipeline state"};
  }
}

DeserializeResult DeserializePipelineState(std::string_view serializedState) {
  try {
    // RTSR transports the document in a buffer that is padded with null
    // characters up to the size its serializer reported, so everything from the
    // first one on belongs to the transport, not to the document.
    const size_t terminator = serializedState.find('\0');
    if (terminator != std::string_view::npos) {
      serializedState = serializedState.substr(0, terminator);
    }
    if (serializedState.empty()) {
      return {SerializationStatus::InvalidArgument, "Serialized pipeline state not provided"};
    }

    const nlohmann::json document =
        nlohmann::json::parse(serializedState.begin(), serializedState.end());

    const auto& header = document.at("Header");
    if (header.at("Api").get<int>() != SERIALIZED_API_DX12) {
      return {SerializationStatus::UnsupportedApi,
              "Serialized pipeline state was not produced for DirectX 12"};
    }
    if (header.at("DeserializedSize").get<size_t>() == 0) {
      return {SerializationStatus::InvalidData,
              "Serialized pipeline state header does not describe any state"};
    }

    const auto& serializedInfo = document.at("State");
    const auto type = serializedInfo.at("Type").get<PipelineStateType>();
    if (type != PipelineStateType::Graphics && type != PipelineStateType::Compute) {
      return {SerializationStatus::UnsupportedPipelineType,
              "Serialized pipeline state is neither a graphics nor a compute pipeline state"};
    }

    PipelineStateStorage storage;
    PipelineStateInfo info{};
    detail::FromJson(serializedInfo, info, storage);

    return {SerializationStatus::Success, {}, DeserializedPipelineState(info, std::move(storage))};
  } catch (const nlohmann::json::exception& ex) {
    return {SerializationStatus::InvalidData, ex.what()};
  } catch (const std::invalid_argument& ex) {
    return {SerializationStatus::InvalidData, ex.what()};
  } catch (const std::exception& ex) {
    return {SerializationStatus::Fail, ex.what()};
  } catch (...) {
    return {SerializationStatus::Fail, "Unknown error while deserializing a pipeline state"};
  }
}

} // namespace gits::rtsr
