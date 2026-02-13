// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeStructsAuto.h"
#include "ccodeTypes.h"
#include "ccodeStream.h"
#include "log.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>
#include <sstream>

namespace gits {
namespace DirectX {
namespace ccode {

static void initialize(const INTCDeviceInfo& value,
                       std::ostringstream& ss,
                       const std::string& name) {
  ss << name << ".GPUMaxFreq = " << value.GPUMaxFreq << ";" << std::endl;
  ss << name << ".GPUMinFreq = " << value.GPUMinFreq << ";" << std::endl;
  ss << name << ".GTGeneration = " << value.GTGeneration << ";" << std::endl;
  ss << name << ".EUCount = " << value.EUCount << ";" << std::endl;
  ss << name << ".PackageTDP = " << value.PackageTDP << ";" << std::endl;
  ss << name << ".MaxFillRate = " << value.MaxFillRate << ";" << std::endl;
  ss << "wcscpy_s(" << name << ".GTGenerationName"
     << ", L\"" << toStr(value.GTGenerationName) << "\");" << std::endl;
}

void initialize(const INTC_D3D12_RESOURCE_DESC& value,
                std::ostringstream& ss,
                const std::string& name,
                const CppParameterInfo& parentInfo) {
  CppParameterInfo pD3D12DescInfo("D3D12_RESOURCE_DESC", "pD3D12Desc", parentInfo);
  pD3D12DescInfo.isPtr = true;
  CppParameterOutput pD3D12DescOut;
  toCpp(value.pD3D12Desc, pD3D12DescInfo, pD3D12DescOut);
  ss << pD3D12DescOut.initialization;

  ss << name << ".pD3D12Desc = &" << pD3D12DescOut.value << ";" << std::endl;
  ss << name << ".Texture2DArrayMipPack = " << value.Texture2DArrayMipPack << ";" << std::endl;
}

// Custom structs (Intel Extensions)
void toCpp(const INTCDeviceInfo& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  initialize(value, ss, name);

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCDeviceInfo1& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  initialize(value, ss, name);
  ss << name << ".GMDID = " << value.GMDID << ";" << std::endl;
  ss << name << ".XeCoresCount = " << value.XeCoresCount << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCExtensionVersion& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".HWFeatureLevel = " << value.HWFeatureLevel << ";" << std::endl;
  ss << name << ".APIVersion = " << value.APIVersion << ";" << std::endl;
  ss << name << ".Revision = " << value.Revision << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCExtensionInfo& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  CppParameterInfo versionInfo("INTCExtensionVersion", "RequestedExtensionVersion");
  CppParameterOutput versionOut;
  toCpp(value.RequestedExtensionVersion, versionInfo, versionOut);
  ss << versionOut.initialization;

  CppParameterInfo deviceInfo("INTCDeviceInfo", "IntelDeviceInfo");
  CppParameterOutput deviceOut;
  toCpp(value.IntelDeviceInfo, deviceInfo, deviceOut);
  ss << deviceOut.initialization;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".RequestedExtensionVersion = " << versionOut.value << ";" << std::endl;
  ss << name << ".IntelDeviceInfo = " << deviceOut.value << ";" << std::endl;
  ss << name << ".pDeviceDriverDesc = L\"" << toStr(value.pDeviceDriverDesc) << "\";" << std::endl;
  ss << name << ".pDeviceDriverVersion = L\"" << toStr(value.pDeviceDriverVersion) << "\";"
     << std::endl;
  ss << name << ".DeviceDriverBuildNumber = " << value.DeviceDriverBuildNumber << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCExtensionInfo1& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  CppParameterInfo versionInfo("INTCExtensionVersion", "RequestedExtensionVersion", info);
  CppParameterOutput versionOut;
  toCpp(value.RequestedExtensionVersion, versionInfo, versionOut);
  ss << versionOut.initialization;

  CppParameterInfo deviceInfo("INTCDeviceInfo1", "IntelDeviceInfo", info);
  CppParameterOutput deviceOut;
  toCpp(value.IntelDeviceInfo, deviceInfo, deviceOut);
  ss << deviceOut.initialization;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".RequestedExtensionVersion = " << versionOut.value << ";" << std::endl;
  ss << name << ".IntelDeviceInfo = " << deviceOut.value << ";" << std::endl;
  ss << name << ".pDeviceDriverDesc = L\"" << toStr(value.pDeviceDriverDesc) << "\";" << std::endl;
  ss << name << ".pDeviceDriverVersion = L\"" << toStr(value.pDeviceDriverVersion) << "\";"
     << std::endl;
  ss << name << ".DeviceDriverBuildNumber = " << value.DeviceDriverBuildNumber << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCExtensionAppInfo& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".pApplicationName = L\"" << toStr(value.pApplicationName) << "\";" << std::endl;
  ss << name << ".ApplicationVersion = " << value.ApplicationVersion << ";" << std::endl;
  ss << name << ".pEngineName = L\"" << toStr(value.pEngineName) << "\";" << std::endl;
  ss << name << ".EngineVersion = " << value.EngineVersion << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCExtensionAppInfo1& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  CppParameterInfo appVersionInfo("INTCAppInfoVersion", "ApplicationVersion", info);
  CppParameterOutput appVersionOut;
  toCpp(value.ApplicationVersion, appVersionInfo, appVersionOut);
  ss << appVersionOut.initialization;

  CppParameterInfo engineVersionInfo("INTCAppInfoVersion", "EngineVersion", info);
  CppParameterOutput engineVersionOut;
  toCpp(value.EngineVersion, engineVersionInfo, engineVersionOut);
  ss << engineVersionOut.initialization;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".pApplicationName = L\"" << toStr(value.pApplicationName) << "\";" << std::endl;
  ss << name << ".ApplicationVersion = " << appVersionOut.value << ";" << std::endl;
  ss << name << ".pEngineName = L\"" << toStr(value.pEngineName) << "\";" << std::endl;
  ss << name << ".EngineVersion = " << engineVersionOut.value << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTCAppInfoVersion& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".major = " << value.major << ";" << std::endl;
  ss << name << ".minor = " << value.minor << ";" << std::endl;
  ss << name << ".patch = " << value.patch << ";" << std::endl;
  ss << name << ".reserved = " << value.reserved << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTC_D3D12_FEATURE& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".EmulatedTyped64bitAtomics = " << toStr(value.EmulatedTyped64bitAtomics) << ";"
     << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTC_D3D12_RESOURCE_DESC& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  initialize(value, ss, name, info);

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTC_D3D12_RESOURCE_DESC_0001& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  initialize(value, ss, name, info);
  ss << name << ".EmulatedTyped64bitAtomics = " << value.EmulatedTyped64bitAtomics << ";"
     << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTC_D3D12_HEAP_DESC& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  CppParameterInfo pD3D12DescInfo("D3D12_HEAP_DESC", "pD3D12Desc", info);
  pD3D12DescInfo.isPtr = true;
  CppParameterOutput pD3D12DescOut;
  toCpp(value.pD3D12Desc, pD3D12DescInfo, pD3D12DescOut);
  ss << pD3D12DescOut.initialization;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".pD3D12Desc = &" << pD3D12DescOut.value << ";" << std::endl;
  ss << name << ".HeapFlagCpuVisibleVideoMemory = " << value.HeapFlagCpuVisibleVideoMemory << ";"
     << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const INTC_D3D12_COMMAND_QUEUE_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  CppParameterInfo pD3D12DescInfo("D3D12_COMMAND_QUEUE_DESC", "pD3D12Desc", info);
  pD3D12DescInfo.isPtr = true;
  CppParameterOutput pD3D12DescOut;
  toCpp(value.pD3D12Desc, pD3D12DescInfo, pD3D12DescOut);
  ss << pD3D12DescOut.initialization;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".pD3D12Desc = &" << pD3D12DescOut.value << ";" << std::endl;
  ss << name << ".CommandThrottlePolicy = " << value.CommandThrottlePolicy << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_CLEAR_VALUE& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  if (value.Format == DXGI_FORMAT_D32_FLOAT || value.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ||
      value.Format == DXGI_FORMAT_D24_UNORM_S8_UINT || value.Format == DXGI_FORMAT_D16_UNORM) {
    CppParameterInfo depthStencilInfo("D3D12_DEPTH_STENCIL_VALUE", "DepthStencil");
    CppParameterOutput depthStencilOut;
    toCpp(value.DepthStencil, depthStencilInfo, depthStencilOut);
    ss << depthStencilOut.initialization;
    ssUnion << name << ".DepthStencil = " << depthStencilOut.value << ";" << std::endl;
  } else {
    for (int i = 0; i < 4; ++i) {
      ssUnion << name << ".Color[" << i << "] = " << toStr(value.Color[i]) << ";" << std::endl;
    }
  }

  ss << info.type << " " << name << " = {};" << std::endl;
  ss << name << ".Format = " << toStr(value.Format) << ";" << std::endl;
  ss << ssUnion.str();
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_ROOT_PARAMETER1& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  CppParameterInfo paramInfo("", "", info);
  CppParameterOutput paramOut;
  switch (value.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    paramInfo.type = "D3D12_ROOT_DESCRIPTOR_TABLE1";
    paramInfo.name = "DescriptorTable";
    toCpp(value.DescriptorTable, paramInfo, paramOut);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    paramInfo.type = "D3D12_ROOT_CONSTANTS";
    paramInfo.name = "Constants";
    toCpp(value.Constants, paramInfo, paramOut);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    paramInfo.type = "D3D12_ROOT_DESCRIPTOR1";
    paramInfo.name = "Descriptor";
    toCpp(value.Descriptor, paramInfo, paramOut);
    break;
  default:
    GITS_ASSERT(false, "Unknown ParameterType");
    break;
  }
  ss << paramOut.initialization;
  ssUnion << name << "." << paramInfo.name << " = " << paramOut.value << ";" << std::endl;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".ParameterType = " << toStr(value.ParameterType) << ";" << std::endl;
  ss << name << ".ShaderVisibility = " << toStr(value.ShaderVisibility) << ";" << std::endl;
  ss << ssUnion.str();

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  // Union parameter
  CppParameterInfo descInfo("", "", info);
  CppParameterOutput descOut;
  std::string descName = "";
  if (value.Version == D3D_ROOT_SIGNATURE_VERSION_1_0) {
    descInfo.type = "D3D12_ROOT_SIGNATURE_DESC";
    descInfo.name = "desc_1_0";
    descName = "Desc_1_0";
    toCpp(value.Desc_1_0, descInfo, descOut);
  } else if (value.Version == D3D_ROOT_SIGNATURE_VERSION_1_1) {
    descInfo.type = "D3D12_ROOT_SIGNATURE_DESC1";
    descInfo.name = "desc_1_1";
    descName = "Desc_1_1";
    toCpp(value.Desc_1_1, descInfo, descOut);
  } else if (value.Version == D3D_ROOT_SIGNATURE_VERSION_1_2) {
    descInfo.type = "D3D12_ROOT_SIGNATURE_DESC2";
    descInfo.name = "desc_1_2";
    descName = "Desc_1_2";
    toCpp(value.Desc_1_2, descInfo, descOut);
  }
  ss << descOut.initialization;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << ";" << std::endl;
  }
  ss << name << ".Version = " << toStr(value.Version) << ";" << std::endl;
  ss << name << "." << descName << " = " << descOut.value << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}
void toCpp(const D3D12_SAMPLER_DESC2& value, CppParameterInfo& info, CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = info.name;
}

void toCpp(const D3D12_UNORDERED_ACCESS_VIEW_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  CppParameterInfo paramInfo("", "");
  CppParameterOutput paramOut;
  switch (value.ViewDimension) {
  case D3D12_UAV_DIMENSION_BUFFER:
    paramInfo.type = "D3D12_BUFFER_UAV";
    paramInfo.name = "Buffer";
    toCpp(value.Buffer, paramInfo, paramOut);
    break;
  case D3D12_UAV_DIMENSION_TEXTURE1D:
    paramInfo.type = "D3D12_TEX1D_UAV";
    paramInfo.name = "Texture1D";
    toCpp(value.Texture1D, paramInfo, paramOut);
    break;
  case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
    paramInfo.type = "D3D12_TEX1D_ARRAY_UAV";
    paramInfo.name = "Texture1DArray";
    toCpp(value.Texture1DArray, paramInfo, paramOut);
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2D:
    paramInfo.type = "D3D12_TEX2D_UAV";
    paramInfo.name = "Texture2D";
    toCpp(value.Texture2D, paramInfo, paramOut);
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
    paramInfo.type = "D3D12_TEX2D_ARRAY_UAV";
    paramInfo.name = "Texture2DArray";
    toCpp(value.Texture2DArray, paramInfo, paramOut);
    break;
  case D3D12_UAV_DIMENSION_TEXTURE3D:
    paramInfo.type = "D3D12_TEX3D_UAV";
    paramInfo.name = "Texture3D";
    toCpp(value.Texture3D, paramInfo, paramOut);
    break;
  default:
    GITS_ASSERT(false, "Unknown D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDimension");
  }
  ss << paramOut.initialization;
  ssUnion << name << "." << paramInfo.name << " = " << paramOut.value << ";" << std::endl;

  ss << info.type << " " << name << " = {};" << std::endl;
  ss << name << ".Format = " << toStr(value.Format) << ";" << std::endl;
  ss << name << ".ViewDimension = " << toStr(value.ViewDimension) << ";" << std::endl;
  ss << ssUnion.str();

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "&";
}

void toCpp(const D3D12_RENDER_TARGET_VIEW_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  // Handle the union based on ViewDimension
  switch (value.ViewDimension) {
  case D3D12_RTV_DIMENSION_BUFFER:
    ssUnion << name << ".Buffer.FirstElement = " << value.Buffer.FirstElement << ";" << std::endl;
    ssUnion << name << ".Buffer.NumElements = " << value.Buffer.NumElements << ";" << std::endl;
    break;

  case D3D12_RTV_DIMENSION_TEXTURE1D:
    ssUnion << name << ".Texture1D.MipSlice = " << value.Texture1D.MipSlice << ";" << std::endl;
    break;

  case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
    ssUnion << name << ".Texture1DArray.MipSlice = " << value.Texture1DArray.MipSlice << ";"
            << std::endl;
    ssUnion << name << ".Texture1DArray.FirstArraySlice = " << value.Texture1DArray.FirstArraySlice
            << ";" << std::endl;
    ssUnion << name << ".Texture1DArray.ArraySize = " << value.Texture1DArray.ArraySize << ";"
            << std::endl;
    break;

  case D3D12_RTV_DIMENSION_TEXTURE2D:
    ssUnion << name << ".Texture2D.MipSlice = " << value.Texture2D.MipSlice << ";" << std::endl;
    ssUnion << name << ".Texture2D.PlaneSlice = " << value.Texture2D.PlaneSlice << ";" << std::endl;
    break;

  case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
    ssUnion << name << ".Texture2DArray.MipSlice = " << value.Texture2DArray.MipSlice << ";"
            << std::endl;
    ssUnion << name << ".Texture2DArray.FirstArraySlice = " << value.Texture2DArray.FirstArraySlice
            << ";" << std::endl;
    ssUnion << name << ".Texture2DArray.ArraySize = " << value.Texture2DArray.ArraySize << ";"
            << std::endl;
    ssUnion << name << ".Texture2DArray.PlaneSlice = " << value.Texture2DArray.PlaneSlice << ";"
            << std::endl;
    break;

  case D3D12_RTV_DIMENSION_TEXTURE2DMS:
    // No additional members for Texture2DMS
    break;

  case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
    ssUnion << name
            << ".Texture2DMSArray.FirstArraySlice = " << value.Texture2DMSArray.FirstArraySlice
            << ";" << std::endl;
    ssUnion << name << ".Texture2DMSArray.ArraySize = " << value.Texture2DMSArray.ArraySize << ";"
            << std::endl;
    break;

  case D3D12_RTV_DIMENSION_TEXTURE3D:
    ssUnion << name << ".Texture3D.MipSlice = " << value.Texture3D.MipSlice << ";" << std::endl;
    ssUnion << name << ".Texture3D.FirstWSlice = " << value.Texture3D.FirstWSlice << ";"
            << std::endl;
    ssUnion << name << ".Texture3D.WSize = " << value.Texture3D.WSize << ";" << std::endl;
    break;

  default:
    GITS_ASSERT(false, "Unsupported D3D12_RTV_DIMENSION");
    break;
  }

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".Format = " << toStr(value.Format) << ";" << std::endl;
  ss << name << ".ViewDimension = " << toStr(value.ViewDimension) << ";" << std::endl;
  ss << ssUnion.str();

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_DEPTH_STENCIL_VIEW_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  switch (value.ViewDimension) {
  case D3D12_DSV_DIMENSION_TEXTURE1D:
    ssUnion << name << ".Texture1D.MipSlice = " << value.Texture1D.MipSlice << ";" << std::endl;
    break;

  case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
    ssUnion << name << ".Texture1DArray.MipSlice = " << value.Texture1DArray.MipSlice << ";"
            << std::endl;
    ssUnion << name << ".Texture1DArray.FirstArraySlice = " << value.Texture1DArray.FirstArraySlice
            << ";" << std::endl;
    ssUnion << name << ".Texture1DArray.ArraySize = " << value.Texture1DArray.ArraySize << ";"
            << std::endl;
    break;

  case D3D12_DSV_DIMENSION_TEXTURE2D:
    ssUnion << name << ".Texture2D.MipSlice = " << value.Texture2D.MipSlice << ";" << std::endl;
    break;

  case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
    ssUnion << name << ".Texture2DArray.MipSlice = " << value.Texture2DArray.MipSlice << ";"
            << std::endl;
    ssUnion << name << ".Texture2DArray.FirstArraySlice = " << value.Texture2DArray.FirstArraySlice
            << ";" << std::endl;
    ssUnion << name << ".Texture2DArray.ArraySize = " << value.Texture2DArray.ArraySize << ";"
            << std::endl;
    break;

  case D3D12_DSV_DIMENSION_TEXTURE2DMS:
    // No additional members for Texture2DMS
    break;

  case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
    ssUnion << name
            << ".Texture2DMSArray.FirstArraySlice = " << value.Texture2DMSArray.FirstArraySlice
            << ";" << std::endl;
    ssUnion << name << ".Texture2DMSArray.ArraySize = " << value.Texture2DMSArray.ArraySize << ";"
            << std::endl;
    break;

  default:
    GITS_ASSERT(false, "Unsupported D3D12_DSV_DIMENSION");
    break;
  }

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".Format = " << toStr(value.Format) << ";" << std::endl;
  ss << name << ".ViewDimension = " << toStr(value.ViewDimension) << ";" << std::endl;
  ss << name << ".Flags = " << toStr(value.Flags) << ";" << std::endl;
  ss << ssUnion.str();

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_SET_PROGRAM_DESC& value, CppParameterInfo& info, CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_DISPATCH_GRAPH_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_ROOT_PARAMETER& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  CppParameterInfo paramInfo("", "", info);
  CppParameterOutput paramOut;

  switch (value.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    paramInfo.type = "D3D12_ROOT_DESCRIPTOR_TABLE";
    paramInfo.name = "descriptorTable";
    toCpp(value.DescriptorTable, paramInfo, paramOut);
    ss << paramOut.initialization;
    ssUnion << name << ".DescriptorTable = " << paramOut.value << ";" << std::endl;
    break;

  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    paramInfo.type = "D3D12_ROOT_CONSTANTS";
    paramInfo.name = "constants";
    toCpp(value.Constants, paramInfo, paramOut);
    ss << paramOut.initialization;
    ssUnion << name << ".Constants = " << paramOut.value << ";" << std::endl;
    break;

  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    paramInfo.type = "D3D12_ROOT_DESCRIPTOR";
    paramInfo.name = "descriptor";
    toCpp(value.Descriptor, paramInfo, paramOut);
    ss << paramOut.initialization;
    ssUnion << name << ".Descriptor = " << paramOut.value << ";" << std::endl;
    break;

  default:
    GITS_ASSERT(false, "Unsupported D3D12_ROOT_PARAMETER_TYPE");
    break;
  }

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".ParameterType = " << toStr(value.ParameterType) << ";" << std::endl;
  ss << name << ".ShaderVisibility = " << toStr(value.ShaderVisibility) << ";" << std::endl;
  ss << ssUnion.str();

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_INDIRECT_ARGUMENT_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  switch (value.Type) {
  case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
  case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
  case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
  case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
  case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS:
  case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH:
    // No additional members
    break;

  case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
    ssUnion << name << ".VertexBuffer.Slot = " << value.VertexBuffer.Slot << ";" << std::endl;
    break;

  case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
    ssUnion << name << ".Constant.RootParameterIndex = " << value.Constant.RootParameterIndex << ";"
            << std::endl;
    ssUnion << name
            << ".Constant.DestOffsetIn32BitValues = " << value.Constant.DestOffsetIn32BitValues
            << ";" << std::endl;
    ssUnion << name << ".Constant.Num32BitValuesToSet = " << value.Constant.Num32BitValuesToSet
            << ";" << std::endl;
    break;

  case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
    ssUnion << name << ".ConstantBufferView.RootParameterIndex = "
            << value.ConstantBufferView.RootParameterIndex << ";" << std::endl;
    break;

  case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
    ssUnion << name << ".ShaderResourceView.RootParameterIndex = "
            << value.ShaderResourceView.RootParameterIndex << ";" << std::endl;
    break;

  case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
    ssUnion << name << ".UnorderedAccessView.RootParameterIndex = "
            << value.UnorderedAccessView.RootParameterIndex << ";" << std::endl;
    break;

  case D3D12_INDIRECT_ARGUMENT_TYPE_INCREMENTING_CONSTANT:
    ssUnion << name << ".IncrementingConstant.RootParameterIndex = "
            << value.IncrementingConstant.RootParameterIndex << ";" << std::endl;
    ssUnion << name << ".IncrementingConstant.DestOffsetIn32BitValues = "
            << value.IncrementingConstant.DestOffsetIn32BitValues << ";" << std::endl;
    break;

  default:
    GITS_ASSERT(false, "Unsupported D3D12_INDIRECT_ARGUMENT_TYPE");
    break;
  }

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".Type = " << toStr(value.Type) << ";" << std::endl;
  ss << ssUnion.str();
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_NODE& value, CppParameterInfo& info, CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_RENDER_PASS_BEGINNING_ACCESS& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_RENDER_PASS_ENDING_ACCESS& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const DXGI_FRAME_STATISTICS& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }

  ss << name << ".PresentCount = " << value.PresentCount << ";" << std::endl;
  ss << name << ".PresentRefreshCount = " << value.PresentRefreshCount << ";" << std::endl;
  ss << name << ".SyncRefreshCount = " << value.SyncRefreshCount << ";" << std::endl;
  ss << name << ".SyncQPCTime.QuadPart = " << value.SyncQPCTime.QuadPart << ";" << std::endl;
  ss << name << ".SyncGPUTime.QuadPart = " << value.SyncGPUTime.QuadPart << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const D3D12_PIPELINE_STATE_STREAM_DESC& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  // Needs to be custom generated because pPipelineStateSubobjectStreamInfo has _Inexpressible_ size

  std::ostringstream ss;
  CppParameterInfo pPipelineStateSubobjectStreamInfo("void", "pPipelineStateSubobjectStream", info);
  pPipelineStateSubobjectStreamInfo.isPtr = true;
  pPipelineStateSubobjectStreamInfo.size = value.SizeInBytes;
  CppParameterOutput pPipelineStateSubobjectStreamOut;
  toCpp(value.pPipelineStateSubobjectStream, pPipelineStateSubobjectStreamInfo,
        pPipelineStateSubobjectStreamOut);
  ss << pPipelineStateSubobjectStreamOut.initialization;

  // Store all the shader bytecodes and other data pointed to by the subobjects
  // On the C++ side, the pointers will be fixed up to point to the correct locations by D3D12_PIPELINE_STATE_STREAM_DESC_Argument handler
  size_t stateOffset = 0;
  size_t storedSize = 0;
  auto storeData = [&storedSize](const void* data, size_t size) {
    if (!data || size == 0) {
      return;
    }
    auto& ccodeStream = ccode::CCodeStream::getInstance();
    ccodeStream.writeData(data, size);
    storedSize += size;
  };
  while (stateOffset < value.SizeInBytes) {
    void* subobjectData = static_cast<char*>(value.pPipelineStateSubobjectStream) + stateOffset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(subobjectData);
      storeData(subobject->pShaderBytecode, subobject->BytecodeLength);
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(subobjectData);
      if (subobject->pSODeclaration) {
        storeData(subobject->pSODeclaration,
                  subobject->NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY));

        for (unsigned i = 0; i < subobject->NumEntries; ++i) {
          auto str =
              const_cast<D3D12_SO_DECLARATION_ENTRY*>(subobject->pSODeclaration)[i].SemanticName;
          storeData(str, strnlen_s(str, 256) + 1);
        }
      }
      if (subobject->pBufferStrides) {
        storeData(subobject->pBufferStrides, subobject->NumStrides * sizeof(UINT));
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      D3D12_INPUT_LAYOUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(subobjectData);
      if (subobject->pInputElementDescs) {
        storeData(subobject->pInputElementDescs,
                  subobject->NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));

        for (unsigned i = 0; i < subobject->NumElements; ++i) {
          auto str =
              const_cast<D3D12_INPUT_ELEMENT_DESC*>(subobject->pInputElementDescs)[i].SemanticName;
          storeData(str, strnlen_s(str, 256) + 1);
        }
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      D3D12_VIEW_INSTANCING_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(subobjectData);
      storeData(subobject->pViewInstanceLocations,
                subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION));
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    default:
      GITS_ASSERT(0 && "Unexpected subobject type");
      break;
    }
  }
  // Used by D3D12_PIPELINE_STATE_STREAM_DESC_Argument
  ss << "std::vector<std::byte> subobjectData(" << storedSize << ");" << std::endl;
  ss << "DataService::Get().Read(subobjectData.data(), subobjectData.size());" << std::endl;

  std::string name = info.getIndexedName();

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".SizeInBytes = " << toStr(value.SizeInBytes) << ";" << std::endl;
  ss << name << ".pPipelineStateSubobjectStream = " << pPipelineStateSubobjectStreamOut.decorator
     << pPipelineStateSubobjectStreamOut.value << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const DSTORAGE_ERROR_FIRST_FAILURE& value,
           CppParameterInfo& info,
           CppParameterOutput& out) {
  GITS_ASSERT(false, "Not implemented");
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
