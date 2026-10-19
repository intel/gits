// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/shared/protocol/rtsrSerialization.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <sstream>

namespace gits::rtsr {

// Field order determines the version-2 wire bytes. The initial fields retain
// RTSR's ordering, while Studio extensions are accepted only after explicit
// protocol negotiation.

template <class Archive>
void serialize(Archive& archive, RTSR_COMMUNICATION_SHADER_STAGE_INFO& data) {
  archive(data.ShaderStage, data.Bytecode, data.BytecodeHash, data.EntrypointName,
          data.BytecodeType, data.D3DShaderVersion);
}

template <class Archive>
void serialize(Archive& archive, RTSR_COMMUNICATION_PIPELINE_UPDATE_INFO& data) {
  archive(data.Id, data.Type, data.State, data.StateHash, data.Incarnation, data.ShaderReferences);
}

template <class Archive>
void serialize(Archive& archive, RTSR_BASIC_CALLBACK_RESPONSE& data) {
  archive(data.Context, data.Result);
}

template <class Archive>
void serialize(Archive& archive, RTSR_NEGOTIATE_PROTOCOL_REQUEST& data) {
  archive(data.Major, data.Minor);
}

template <class Archive>
void serialize(Archive& archive, RTSR_BIND_CONTEXT_REQUEST& data) {
  archive(data.Context);
}

template <class Archive>
void serialize(Archive& archive, RTSR_CREATE_CONTEXT_MESSAGE& data) {
  archive(data.ApplicationPath, data.ApplicationName, data.UsedApi, data.ProtocolMajor);
}

template <class Archive>
void serialize(Archive& archive, RTSR_PIPELINE_CREATED_MESSAGE& data) {
  archive(data.Context, data.Id, data.Type, data.State, data.StateHash, data.Incarnation,
          data.ReplacementEligible, data.ReplacementDiagnostic, data.ShaderStages);
}

template <class Archive>
void serialize(Archive& archive, RTSR_PIPELINE_ELIGIBILITY_MESSAGE& data) {
  archive(data.Context, data.PipelineId, data.Incarnation, data.ReplacementEligible,
          data.Diagnostic);
}

template <class Archive>
void serialize(Archive& archive, RTSR_SET_PIPELINE_NAME_MESSAGE& data) {
  archive(data.Context, data.PipelineId, data.Name);
}

template <class Archive>
void serialize(Archive& archive, RTSR_PIPELINE_DELETED_MESSAGE& data) {
  archive(data.Context, data.PipelineId);
}

template <class Archive>
void serialize(Archive& archive, RTSR_LOG_MESSAGE& data) {
  archive(data.Context, data.Message);
}

template <class Archive>
void serialize(Archive& archive, RTSR_DELETE_CONTEXT_MESSAGE& data) {
  archive(data.Context);
}

template <class Archive>
void serialize(Archive& archive, RTSR_UPDATE_PIPELINES_REQUEST& data) {
  archive(data.UpdatedPipelines, data.ShaderStages);
}

template <class Archive>
void serialize(Archive& archive, RTSR_PIPELINE_RESET_INFO& data) {
  archive(data.Id, data.Incarnation);
}

template <class Archive>
void serialize(Archive& archive, RTSR_RESET_PIPELINES_REQUEST& data) {
  archive(data.Pipelines);
}

template <class Archive>
void serialize(Archive& archive, RTSR_EMPTY_REQUEST& data) {}

template <class Archive>
void serialize(Archive& archive, RTSR_PIPELINE_TIME_STATS_INFO& data) {
  archive(data.Id, data.Begin, data.Duration);
}

template <class Archive>
void serialize(Archive& archive, RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO& data) {
  archive(data.Begin, data.Duration, data.PipelineStats);
}

template <class Archive>
void serialize(Archive& archive, RTSR_REPORT_TIME_STATS_MESSAGE& data) {
  archive(data.Context, data.FrameNumber, data.Begin, data.Duration, data.CommandBufferStats);
}

template <class Archive>
void serialize(Archive& archive, RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST& data) {
  archive(data.enable);
}

template <typename T>
Message Serialize(const T& obj, RTSR_MESSAGE_TYPE type) {
  Message message;
  message.Header.Type = type;

  try {
    std::ostringstream stream;
    {
      cereal::BinaryOutputArchive archive(stream);
      archive(obj);
    }
    const std::string payload = stream.str();
    if (!IsPlausibleLength(static_cast<int32_t>(payload.size()))) {
      message.Header.Len = 0;
      return message;
    }
    message.Data.assign(payload.begin(), payload.end());
    message.Header.Len = static_cast<int32_t>(message.Data.size());
  } catch (...) {
    message.Data.clear();
    message.Header.Len = 0;
  }

  return message;
}

template <typename T>
std::optional<T> Deserialize(const Message& message) {
  if (message.Header.Len <= 0 || message.Data.size() != static_cast<size_t>(message.Header.Len)) {
    return std::nullopt;
  }

  try {
    std::istringstream stream(
        std::string(reinterpret_cast<const char*>(message.Data.data()), message.Data.size()));
    cereal::BinaryInputArchive archive(stream);

    T obj{};
    archive(obj);
    return obj;
  } catch (...) {
    return std::nullopt;
  }
}

// An empty request has no fields, so cereal produces a zero length payload and
// the generic guard above would reject it. It is the one type deserialized by
// message type alone.
template <>
std::optional<RTSR_EMPTY_REQUEST> Deserialize<RTSR_EMPTY_REQUEST>(const Message& message) {
  return RTSR_EMPTY_REQUEST{};
}

#define RTSR_INSTANTIATE_WIRE_TYPE(type)                                                           \
  template Message Serialize<type>(const type&, RTSR_MESSAGE_TYPE);                                \
  template std::optional<type> Deserialize<type>(const Message&)

RTSR_INSTANTIATE_WIRE_TYPE(RTSR_BASIC_CALLBACK_RESPONSE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_NEGOTIATE_PROTOCOL_REQUEST);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_BIND_CONTEXT_REQUEST);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_CREATE_CONTEXT_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_PIPELINE_CREATED_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_PIPELINE_ELIGIBILITY_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_SET_PIPELINE_NAME_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_PIPELINE_DELETED_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_LOG_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_DELETE_CONTEXT_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_UPDATE_PIPELINES_REQUEST);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_RESET_PIPELINES_REQUEST);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_REPORT_TIME_STATS_MESSAGE);
RTSR_INSTANTIATE_WIRE_TYPE(RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST);

template Message Serialize<RTSR_EMPTY_REQUEST>(const RTSR_EMPTY_REQUEST&, RTSR_MESSAGE_TYPE);

#undef RTSR_INSTANTIATE_WIRE_TYPE

} // namespace gits::rtsr
