// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

namespace gits {
namespace DirectX {

enum class CommandId {
  // gits::CToken::ID_HELPER_TOKENS
  // Note: CommandId does not match token ids (TId in token.h)
  ID_INIT_START = 0x0,
  ID_INIT_END = 0x1,
  ID_FRAME_START = 0x2,
  ID_FRAME_END = 0x3,

  // gits::CToken::ID_DirectX (13 * 0x10000)
  ID_META_BEGIN = 0xD0000,
  ID_META_CREATE_WINDOW = 0xD0001,
  ID_MAPPED_DATA = 0xD0002,
  ID_CREATE_HEAP_ALLOCATION = 0xD0003,
  ID_WAIT_FOR_FENCE_SIGNALED = 0xD0004,

  // ID_META_BEGIN + 0x500
  ID_COMMON_BEGIN = 0xD0500,
  ID_IUNKNOWN_QUERYINTERFACE = 0xD0501,
  ID_IUNKNOWN_ADDREF = 0xD0502,
  ID_IUNKNOWN_RELEASE = 0xD0503,
  %for cmd_name, cmd_id in command_ids[Api.COMMON.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  // ID_COMMON_BEGIN + 0x500
  ID_DXGI_BEGIN = 0xD0A00,
  %for cmd_name, cmd_id in command_ids[Api.DXGI.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  // ID_DXGI_BEGIN + 0x500
  ID_DXGI_DEBUG_BEGIN = 0xD0F00,
  %for cmd_name, cmd_id in command_ids[Api.DXGI_DEBUG.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor
  
  // ID_DXGI_DEBUG_BEGIN + 0x500
  ID_D3D12_BEGIN = 0xD1400,
  %for cmd_name, cmd_id in command_ids[Api.D3D12.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor
  
  // ID_D3D12_BEGIN + 0x500
  ID_D3D12_DEBUG_BEGIN = 0xD1900,
  %for cmd_name, cmd_id in command_ids[Api.D3D12_DEBUG.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  // ID_D3D12_DEBUG_BEGIN + 0x500
  ID_DML_BEGIN = 0xD1E00,
  %for cmd_name, cmd_id in command_ids[Api.DML.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor
  
  // ID_DML_BEGIN + 0x500
  ID_INTEL_EXTENSIONS_BEGIN = 0xD2300,
  INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT = 0xD2301,
  INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1 = 0xD2302,
  INTC_DESTROYDEVICEEXTENSIONCONTEXT = 0xD2303,
  INTC_D3D12_CREATECOMMANDQUEUE = 0xD2304,
  INTC_D3D12_CREATECOMPUTEPIPELINESTATE = 0xD2305,
  INTC_D3D12_CREATERESERVEDRESOURCE = 0xD2306,
  INTC_D3D12_CREATECOMMITTEDRESOURCE = 0xD2307,
  INTC_D3D12_CREATECOMMITTEDRESOURCE1 = 0xD2308,
  INTC_D3D12_CREATEHEAP = 0xD2309,
  INTC_D3D12_CREATEPLACEDRESOURCE = 0xD230A,
  INTC_D3D12_CREATEHOSTRTASRESOURCE = 0xD230B,
  INTC_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST = 0xD230C,
  INTC_D3D12_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST = 0xD230D,
  INTC_D3D12_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST = 0xD230E,
  INTC_D3D12_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST = 0xD230F,
  INTC_D3D12_TRANSFERHOSTRTAS = 0xD2310,
  INTC_D3D12_SETDRIVEREVENTMETADATA = 0xD2311,
  INTC_D3D12_QUERYCPUVISIBLEVIDMEM = 0xD2312,
  INTC_D3D12_CREATESTATEOBJECT = 0xD2313,
  INTC_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTURE = 0xD2314,
  INTC_D3D12_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO = 0xD2315,
  INTC_D3D12_SETFEATURESUPPORT = 0xD2316,
  INTC_D3D12_GETRESOURCEALLOCATIONINFO = 0xD2317,
  INTC_D3D12_CHECKFEATURESUPPORT = 0xD2318,
  INTC_D3D12_ADDSHADERBINARIESPATH = 0xD2319,
  INTC_D3D12_REMOVESHADERBINARIESPATH = 0xD231A,
  INTC_D3D12_SETAPPLICATIONINFO = 0xD231B,
  INTC_D3D12_GETSUPPORTEDVERSIONS = 0xD231C,

  // ID_INTEL_EXTENSIONS_BEGIN + 0x500,
  ID_XESS_BEGIN = 0xD2800,
  %for cmd_name, cmd_id in command_ids[Api.XESS.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  // ID_XESS_BEGIN + 0x500
  ID_DSTORAGE_BEGIN = 0xD2D00,
  %for cmd_name, cmd_id in command_ids[Api.DSTORAGE.name].items():
  ${f'{cmd_name} = 0x{cmd_id:X},'}
  %endfor

  // ID_DSTORAGE_BEGIN + 0x500
  ID_NVAPI_BEGIN = 0xD3200,
  ID_NVAPI_INITIALIZE = 0xd3201,
  ID_NVAPI_UNLOAD = 0xd3202,
  ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD = 0xd3203,
  ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY = 0xd3204,
  ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX = 0xd3205,
  ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE = 0xd3206,
  ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION = 0xd3207,

  ID_END
};

} // namespace DirectX
} // namespace gits
