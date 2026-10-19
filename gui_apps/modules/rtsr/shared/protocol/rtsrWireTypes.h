// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Wire format spoken by RTSREditor. Enumerator values, field order and field
// widths are all observable on the wire, so this file mirrors RTSR's
// Source/Common/Include/RTSR.h and RTSR_Private.h field for field and keeps its
// identifier names, which makes diffing against RTSR straightforward.
//
// The archives are cereal binary, which writes raw platform representations, so
// both ends must agree on size_t width and enum underlying type. Underlying
// types are therefore fixed explicitly rather than left implementation defined.
// This is a same-machine, same-architecture protocol.

namespace gits::rtsr {

enum RTSR_RESULT : int32_t {
  RTSR_RESULT_FAIL = INT32_MIN,
  RTSR_RESULT_INCOMPLETE = -2,
  RTSR_RESULT_INVALID_ARGUMENT = -1,
  RTSR_RESULT_SUCCESS = 0
};

inline bool RtsrSucceeded(RTSR_RESULT r) {
  return r >= 0;
}

inline bool RtsrFailed(RTSR_RESULT r) {
  return r < 0;
}

enum RTSR_API : int32_t {
  RTSR_API_UNKNOWN = 0,
  RTSR_API_VULKAN_1_0,
  RTSR_API_VULKAN_1_1,
  RTSR_API_VULKAN_1_2,
  RTSR_API_VULKAN_1_3,
  RTSR_API_DX12
};

enum RTSR_PIPELINE_TYPE : int32_t {
  RTSR_PIPELINE_TYPE_UNKNOWN = 0,
  RTSR_PIPELINE_TYPE_GRAPHICS,
  RTSR_PIPELINE_TYPE_COMPUTE,
  RTSR_PIPELINE_TYPE_RAY_TRACING
};

enum RTSR_SHADER_STAGE : int32_t {
  RTSR_SHADER_STAGE_UNKNOWN = 0,
  RTSR_SHADER_STAGE_VERTEX,
  RTSR_SHADER_STAGE_TESSELLATION_CONTROL,
  RTSR_SHADER_STAGE_TESSELLATION_HULL = RTSR_SHADER_STAGE_TESSELLATION_CONTROL,
  RTSR_SHADER_STAGE_TESSELLATION_EVALUATION,
  RTSR_SHADER_STAGE_TESSELLATION_DOMAIN = RTSR_SHADER_STAGE_TESSELLATION_EVALUATION,
  RTSR_SHADER_STAGE_GEOMETRY,
  RTSR_SHADER_STAGE_FRAGMENT,
  RTSR_SHADER_STAGE_PIXEL = RTSR_SHADER_STAGE_FRAGMENT,
  RTSR_SHADER_STAGE_COMPUTE,
  RTSR_SHADER_STAGE_RAY_TRACING_RAYGEN,
  RTSR_SHADER_STAGE_RAY_TRACING_ANY_HIT,
  RTSR_SHADER_STAGE_RAY_TRACING_CLOSEST_HIT,
  RTSR_SHADER_STAGE_RAY_TRACING_MISS,
  RTSR_SHADER_STAGE_RAY_TRACING_INTERSECTION
};

enum RTSR_BYTECODE_TYPE : int32_t {
  RTSR_BYTECODE_TYPE_UNKNOWN = 0,
  RTSR_BYTECODE_TYPE_SPIRV,
  RTSR_BYTECODE_TYPE_DXBC,
  RTSR_BYTECODE_TYPE_DXIL
};

enum RTSR_SPIRV_TARGET_ENV : int32_t {
  RTSR_SPIRV_TARGET_ENV_UNKNOWN,
  RTSR_SPIRV_TARGET_ENV_1_0,
  RTSR_SPIRV_TARGET_ENV_1_1,
  RTSR_SPIRV_TARGET_ENV_1_2,
  RTSR_SPIRV_TARGET_ENV_1_3,
  RTSR_SPIRV_TARGET_ENV_1_4,
  RTSR_SPIRV_TARGET_ENV_1_5,
  RTSR_SPIRV_TARGET_ENV_1_6,
  RTSR_SPIRV_TARGET_ENV_LATEST = RTSR_SPIRV_TARGET_ENV_1_6,
  RTSR_SPIRV_TARGET_ENV_COUNT
};

enum RTSR_MESSAGE_TYPE : int32_t {
  RTSR_BASIC_CALLBACK_RESPONSE_MESSAGE_TYPE = 0,
  RTSR_CREATE_CONTEXT_REQUEST_MESSAGE_TYPE,
  RTSR_PIPELINE_CREATED_MESSAGE_TYPE,
  RTSR_SET_PIPELINE_NAME_MESSAGE_TYPE,
  RTSR_PIPELINE_DELETED_MESSAGE_TYPE,
  RTSR_REPORT_TIME_STATS_MESSAGE_TYPE,
  RTSR_LOG_MESSAGE_TYPE,
  RTSR_DELETE_CONTEXT_REQUEST_MESSAGE_TYPE,
  RTSR_UPDATE_PIPELINES_REQUEST_MESSAGE_TYPE,
  RTSR_RESET_PIPELINES_REQUEST_MESSAGE_TYPE,
  RTSR_CLOSE_CONNECTION_REQUEST_MESSAGE_TYPE,
  RTSR_CHECK_IF_ALIVE_REQUEST_MESSAGE_TYPE,
  RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST_MESSAGE_TYPE,
  RTSR_NEGOTIATE_PROTOCOL_REQUEST_MESSAGE_TYPE,
  RTSR_BIND_CONTEXT_REQUEST_MESSAGE_TYPE,
  RTSR_PIPELINE_ELIGIBILITY_MESSAGE_TYPE
};

using RTSR_ID = uint64_t;
using RTSR_HASH = uint64_t;
using RTSR_CONTEXT = uint64_t;
using RTSR_PIPELINE_INCARNATION = uint64_t;
using RTSR_SERIALIZED_PIPELINE_STATE = std::string;

// The shader toolchain on the editor side defaults to this entrypoint name.
constexpr const char* RTSR_DEFAULT_ENTRYPOINT_NAME = "main";
inline constexpr uint32_t RTSR_PROTOCOL_MAJOR = 2;
inline constexpr uint32_t RTSR_PROTOCOL_MINOR = 1;

// **********************************************************************************************
// * Payloads                                                                                   *
// **********************************************************************************************

struct RTSR_COMMUNICATION_SHADER_STAGE_INFO {
  RTSR_SHADER_STAGE ShaderStage{RTSR_SHADER_STAGE_UNKNOWN};
  std::vector<uint8_t> Bytecode;
  RTSR_HASH BytecodeHash{};
  std::string EntrypointName;
  RTSR_BYTECODE_TYPE BytecodeType{RTSR_BYTECODE_TYPE_UNKNOWN};
  size_t D3DShaderVersion{};
};

struct RTSR_COMMUNICATION_PIPELINE_UPDATE_INFO {
  RTSR_ID Id{};
  RTSR_PIPELINE_TYPE Type{RTSR_PIPELINE_TYPE_UNKNOWN};
  RTSR_SERIALIZED_PIPELINE_STATE State;
  RTSR_HASH StateHash{};
  RTSR_PIPELINE_INCARNATION Incarnation{};
  // Indices into the ShaderStages vector of the same message, not hashes.
  std::vector<uint32_t> ShaderReferences;
};

struct RTSR_PIPELINE_TIME_STATS_INFO {
  RTSR_ID Id{};
  size_t Begin{};
  size_t Duration{};
};

struct RTSR_BASIC_CALLBACK_RESPONSE {
  RTSR_CONTEXT Context{};
  RTSR_RESULT Result{RTSR_RESULT_FAIL};
};

struct RTSR_NEGOTIATE_PROTOCOL_REQUEST {
  uint32_t Major{RTSR_PROTOCOL_MAJOR};
  uint32_t Minor{RTSR_PROTOCOL_MINOR};
};

struct RTSR_BIND_CONTEXT_REQUEST {
  RTSR_CONTEXT Context{};
};

struct RTSR_CREATE_CONTEXT_MESSAGE {
  std::string ApplicationPath;
  std::string ApplicationName;
  RTSR_API UsedApi{RTSR_API_UNKNOWN};
  uint32_t ProtocolMajor{RTSR_PROTOCOL_MAJOR};
};

struct RTSR_PIPELINE_CREATED_MESSAGE {
  RTSR_CONTEXT Context{};
  RTSR_ID Id{};
  RTSR_PIPELINE_TYPE Type{RTSR_PIPELINE_TYPE_UNKNOWN};
  RTSR_SERIALIZED_PIPELINE_STATE State;
  RTSR_HASH StateHash{};
  RTSR_PIPELINE_INCARNATION Incarnation{};
  bool ReplacementEligible{true};
  std::string ReplacementDiagnostic;
  std::vector<RTSR_COMMUNICATION_SHADER_STAGE_INFO> ShaderStages;
};

struct RTSR_PIPELINE_ELIGIBILITY_MESSAGE {
  RTSR_CONTEXT Context{};
  RTSR_ID PipelineId{};
  RTSR_PIPELINE_INCARNATION Incarnation{};
  bool ReplacementEligible{};
  std::string Diagnostic;
};

struct RTSR_SET_PIPELINE_NAME_MESSAGE {
  RTSR_CONTEXT Context{};
  RTSR_ID PipelineId{};
  std::string Name;
};

struct RTSR_PIPELINE_DELETED_MESSAGE {
  RTSR_CONTEXT Context{};
  RTSR_ID PipelineId{};
};

struct RTSR_LOG_MESSAGE {
  RTSR_CONTEXT Context{};
  std::string Message;
};

struct RTSR_DELETE_CONTEXT_MESSAGE {
  RTSR_CONTEXT Context{};
};

struct RTSR_UPDATE_PIPELINES_REQUEST {
  std::vector<RTSR_COMMUNICATION_PIPELINE_UPDATE_INFO> UpdatedPipelines;
  std::vector<RTSR_COMMUNICATION_SHADER_STAGE_INFO> ShaderStages;
};

struct RTSR_PIPELINE_RESET_INFO {
  RTSR_ID Id{};
  RTSR_PIPELINE_INCARNATION Incarnation{};
};

struct RTSR_RESET_PIPELINES_REQUEST {
  std::vector<RTSR_PIPELINE_RESET_INFO> Pipelines;
};

// Requests that carry no data. They are identified by RTSR_MESSAGE_TYPE alone.
struct RTSR_EMPTY_REQUEST {};

struct RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO {
  size_t Begin{};
  size_t Duration{};
  std::vector<RTSR_PIPELINE_TIME_STATS_INFO> PipelineStats;
};

struct RTSR_REPORT_TIME_STATS_MESSAGE {
  RTSR_CONTEXT Context{};
  size_t FrameNumber{};
  size_t Begin{};
  size_t Duration{};
  std::vector<RTSR_COMMUNICATION_COMMAND_BUFFER_TIME_STATS_INFO> CommandBufferStats;
};

struct RTSR_ENABLE_PERFORMANCE_MEASUREMENTS_REQUEST {
  bool enable{};
};

} // namespace gits::rtsr
