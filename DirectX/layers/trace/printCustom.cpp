// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "printCustom.h"
#include "command.h"
#include "to_string/guidToStrAuto.h"
#include "to_string/toStr.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

FastOStream& printObjectKey(FastOStream& stream, unsigned key) {
  if (!key) {
    stream << "nullptr";
  } else {
    stream << "O";
    stream << keyToStr(key);
  }
  return stream;
}

FastOStream& printString(FastOStream& stream, const wchar_t* s) {
  if (s) {
    std::wstring strW(s);
    std::string str(strW.begin(), strW.end());
    stream << "\"" << str << "\"";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& printString(FastOStream& stream, const char* s) {
  if (s) {
    stream << "\"" << s << "\"";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, REFIID riid) {
  return stream << toStr(riid);
}

FastOStream& operator<<(FastOStream& stream, const UINT* value) {
  if (!value) {
    return stream << "nullptr";
  } else {
    return stream << *value;
  }
}

FastOStream& operator<<(FastOStream& stream, const BOOL* value) {
  if (!value) {
    return stream << "nullptr";
  } else {
    return stream << (*value ? "TRUE" : "FALSE");
  }
}

FastOStream& operator<<(FastOStream& stream, const UINT8& value) {
  return stream << static_cast<unsigned>(value);
}

FastOStream& operator<<(FastOStream& stream, const UINT64* value) {
  if (!value) {
    return stream << "nullptr";
  } else {
    return stream << *value;
  }
}

FastOStream& operator<<(FastOStream& stream, const LARGE_INTEGER& value) {
  return stream << "LARGE_INTEGER{}";
}

FastOStream& operator<<(FastOStream& stream, const D3D12_RECT& value) {
  stream << "D3D12_RECT{";
  stream << value.left << ", ";
  stream << value.top << ", ";
  stream << value.right << ", ";
  stream << value.bottom << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_RECT* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const POINT& value) {
  stream << "POINT{";
  stream << value.x << ", ";
  stream << value.y << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_RENDER_TARGET_VIEW_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_RENDER_TARGET_VIEW_DESC* value) {
  if (value) {
    stream << "D3D12_RENDER_TARGET_VIEW_DESC{";
    stream << value->Format << ", ";
    stream << value->ViewDimension << ", {";
    switch (value->ViewDimension) {
    case D3D12_RTV_DIMENSION_BUFFER:
      stream << value->Buffer.FirstElement << ", " << value->Buffer.NumElements;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE1D:
      stream << value->Texture1D.MipSlice;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
      stream << value->Texture1DArray.MipSlice << ", " << value->Texture1DArray.FirstArraySlice
             << ", " << value->Texture1DArray.ArraySize;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2D:
      stream << value->Texture2D.MipSlice << ", " << value->Texture2D.PlaneSlice;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
      stream << value->Texture2DArray.MipSlice << ", " << value->Texture2DArray.FirstArraySlice
             << ", " << value->Texture2DArray.ArraySize << ", " << value->Texture2DArray.PlaneSlice;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
      stream << value->Texture2DMS.UnusedField_NothingToDefine;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
      stream << value->Texture2DMSArray.FirstArraySlice << ", "
             << value->Texture2DMSArray.ArraySize;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE3D:
      stream << value->Texture3D.MipSlice << ", " << value->Texture3D.FirstWSlice << ", "
             << value->Texture3D.WSize;
      break;
    }
    stream << "}}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_COMMAND_SIGNATURE_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_COMMAND_SIGNATURE_DESC* value) {
  if (value) {
    stream << "D3D12_COMMAND_SIGNATURE_DESC{";
    stream << value->ByteStride << ", ";
    stream << value->NumArgumentDescs << ", [";
    for (unsigned i = 0; i < value->NumArgumentDescs; ++i) {
      stream << (i > 0 ? ", " : "") << "{" << value->pArgumentDescs[i].Type;
      switch (value->pArgumentDescs[i].Type) {
      case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
        stream << ", {" << value->pArgumentDescs[i].VertexBuffer.Slot;
        break;
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
        stream << ", {" << value->pArgumentDescs[i].Constant.RootParameterIndex << ", ";
        stream << value->pArgumentDescs[i].Constant.DestOffsetIn32BitValues << ", ";
        stream << value->pArgumentDescs[i].Constant.Num32BitValuesToSet << "}";
        break;
      case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
        stream << ", {" << value->pArgumentDescs[i].ConstantBufferView.RootParameterIndex << "}";
        break;
      case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
        stream << ", {" << value->pArgumentDescs[i].ShaderResourceView.RootParameterIndex << "}";
        break;
      case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
        stream << ", {" << value->pArgumentDescs[i].UnorderedAccessView.RootParameterIndex << "}";
        break;
      }
      stream << "}";
    }
    stream << "], " << value->NodeMask << "}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* value) {
  if (value) {
    stream << "D3D12_VERSIONED_ROOT_SIGNATURE_DESC{";
    stream << value->Version << ", {";
    switch (value->Version) {
    case D3D_ROOT_SIGNATURE_VERSION_1_0:
      stream << value->Desc_1_0;
      break;
    case D3D_ROOT_SIGNATURE_VERSION_1_1:
      stream << value->Desc_1_1;
      break;
    case D3D_ROOT_SIGNATURE_VERSION_1_2:
      stream << value->Desc_1_2;
      break;
    }
    stream << "}}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_SIGNATURE_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_SIGNATURE_DESC* value) {
  if (value) {
    stream << "D3D12_ROOT_SIGNATURE_DESC{";
    stream << value->NumParameters << ", [";
    for (unsigned i = 0; i < value->NumParameters; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pParameters[i];
    }
    stream << "], " << value->NumStaticSamplers << ", [";
    for (unsigned i = 0; i < value->NumStaticSamplers; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pStaticSamplers[i];
    }
    stream << "], " << value->Flags;
    stream << "}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_SIGNATURE_DESC1& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_SIGNATURE_DESC1* value) {
  if (value) {
    stream << "D3D12_ROOT_SIGNATURE_DESC1{";
    stream << value->NumParameters << ", [";
    for (unsigned i = 0; i < value->NumParameters; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pParameters[i];
    }
    stream << "], " << value->NumStaticSamplers << ", [";
    for (unsigned i = 0; i < value->NumStaticSamplers; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pStaticSamplers[i];
    }
    stream << "], " << value->Flags;
    stream << "}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_SIGNATURE_DESC2& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_SIGNATURE_DESC2* value) {
  if (value) {
    stream << "D3D12_ROOT_SIGNATURE_DESC2{";
    stream << value->NumParameters << ", [";
    for (unsigned i = 0; i < value->NumParameters; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pParameters[i];
    }
    stream << "], " << value->NumStaticSamplers << ", [";
    for (unsigned i = 0; i < value->NumStaticSamplers; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pStaticSamplers[i];
    }
    stream << "], " << value->Flags;
    stream << "}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_DESCRIPTOR_TABLE& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_DESCRIPTOR_TABLE* value) {
  if (value) {
    stream << "D3D12_ROOT_DESCRIPTOR_TABLE{";
    stream << value->NumDescriptorRanges << ", [";
    for (unsigned i = 0; i < value->NumDescriptorRanges; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pDescriptorRanges[i];
    }
    stream << "]}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_DESCRIPTOR_TABLE1& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_DESCRIPTOR_TABLE1* value) {
  if (value) {
    stream << "D3D12_ROOT_DESCRIPTOR_TABLE1{";
    stream << value->NumDescriptorRanges << ", [";
    for (unsigned i = 0; i < value->NumDescriptorRanges; ++i) {
      stream << (i == 0 ? "" : ", ");
      stream << value->pDescriptorRanges[i];
    }
    stream << "]}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_PARAMETER& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_PARAMETER* value) {
  if (value) {
    stream << "D3D12_ROOT_PARAMETER{";
    stream << value->ParameterType << ", ";
    switch (value->ParameterType) {
    case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
      stream << value->DescriptorTable << ", ";
      break;
    case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
      stream << value->Constants << ", ";
      break;
    case D3D12_ROOT_PARAMETER_TYPE_CBV:
    case D3D12_ROOT_PARAMETER_TYPE_SRV:
    case D3D12_ROOT_PARAMETER_TYPE_UAV:
      stream << value->Descriptor << ", ";
      break;
    }
    stream << value->ShaderVisibility;
    stream << "}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_PARAMETER1& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_ROOT_PARAMETER1* value) {
  if (value) {
    stream << "D3D12_ROOT_PARAMETER1{";
    stream << value->ParameterType << ", ";
    switch (value->ParameterType) {
    case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
      stream << value->DescriptorTable << ", ";
      break;
    case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
      stream << value->Constants << ", ";
      break;
    case D3D12_ROOT_PARAMETER_TYPE_CBV:
    case D3D12_ROOT_PARAMETER_TYPE_SRV:
    case D3D12_ROOT_PARAMETER_TYPE_UAV:
      stream << value->Descriptor << ", ";
      break;
    }
    stream << value->ShaderVisibility;
    stream << "}";
    return stream;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const DML_SCALAR_UNION& value) {
  stream << "DML_SCALAR_UNION{";
  stream << value.UInt64 << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_SHADER_BYTECODE& value) {
  stream << "D3D12_SHADER_BYTECODE{";
  stream << value.BytecodeLength;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_EXPORT_DESC& value) {
  stream << "{";
  printString(stream, value.Name);
  stream << ", ";
  printString(stream, value.ExportToRename);
  stream << ", " << value.Flags << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_EXPORT_DESC* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_UNORDERED_ACCESS_VIEW_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_UNORDERED_ACCESS_VIEW_DESC* value) {
  if (value) {
    stream << "D3D12_UNORDERED_ACCESS_VIEW_DESC{";
    stream << value->Format << ", ";
    stream << value->ViewDimension << ", {";
    switch (value->ViewDimension) {
    case D3D12_UAV_DIMENSION_BUFFER:
      stream << value->Buffer.FirstElement << ", " << value->Buffer.NumElements;
      break;
    case D3D12_UAV_DIMENSION_TEXTURE1D:
      stream << value->Texture1D.MipSlice;
      break;
    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
      stream << value->Texture1DArray.MipSlice << ", " << value->Texture1DArray.FirstArraySlice
             << ", " << value->Texture1DArray.ArraySize;
      break;
    case D3D12_UAV_DIMENSION_TEXTURE2D:
      stream << value->Texture2D.MipSlice << ", " << value->Texture2D.PlaneSlice;
      break;
    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
      stream << value->Texture2DArray.MipSlice << ", " << value->Texture2DArray.FirstArraySlice
             << ", " << value->Texture2DArray.ArraySize << ", " << value->Texture2DArray.PlaneSlice;
      break;
    case D3D12_UAV_DIMENSION_TEXTURE2DMS:
      break;
    case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
      break;
    case D3D12_UAV_DIMENSION_TEXTURE3D:
      stream << value->Texture3D.MipSlice << ", " << value->Texture3D.FirstWSlice << ", "
             << value->Texture3D.WSize;
      break;
    }
    stream << "}}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_DEPTH_STENCIL_VIEW_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_DEPTH_STENCIL_VIEW_DESC* value) {
  if (value) {
    stream << "D3D12_DEPTH_STENCIL_VIEW_DESC{";
    stream << value->Format << ", ";
    stream << value->ViewDimension << ", ";
    stream << value->Flags << ", {";
    switch (value->ViewDimension) {
    case D3D12_DSV_DIMENSION_TEXTURE1D:
      stream << value->Texture1D.MipSlice;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
      stream << value->Texture1DArray.MipSlice << ", " << value->Texture1DArray.FirstArraySlice
             << ", " << value->Texture1DArray.ArraySize;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2D:
      stream << value->Texture2D.MipSlice;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
      stream << value->Texture2DArray.MipSlice << ", " << value->Texture2DArray.FirstArraySlice
             << ", " << value->Texture2DArray.ArraySize;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2DMS:
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
      break;
    }
    stream << "}}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_CLEAR_VALUE& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_CLEAR_VALUE* value) {
  if (value) {
    stream << "D3D12_CLEAR_VALUE{";
    stream << value->Format << ", {[";
    stream << value->Color[0] << ", ";
    stream << value->Color[1] << ", ";
    stream << value->Color[2] << ", ";
    stream << value->Color[3];
    stream << "], ";
    stream << "{";
    stream << value->DepthStencil.Depth << ", ";
    stream << value->DepthStencil.Stencil;
    stream << "}}}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_STREAM_OUTPUT_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_STREAM_OUTPUT_DESC* value) {
  if (value) {
    stream << "D3D12_STREAM_OUTPUT_DESC{";
    if (value->pSODeclaration) {
      stream << "[";
      for (UINT i = 0; i < value->NumEntries; ++i) {
        stream << value->pSODeclaration[i];
        if (i < value->NumEntries - 1) {
          stream << ", ";
        }
      }
      stream << "], ";
    } else {
      stream << "nullptr, ";
    }
    stream << value->NumStrides << ", ";
    if (value->pBufferStrides) {
      stream << "[";
      for (UINT i = 0; i < value->NumStrides; ++i) {
        stream << value->pBufferStrides[i];
        if (i < value->NumStrides - 1) {
          stream << ", ";
        }
      }
      stream << "], ";
    } else {
      stream << "nullptr, ";
    }
    stream << value->NumStrides << ", ";
    stream << value->RasterizedStream;
    stream << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_INPUT_LAYOUT_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_INPUT_LAYOUT_DESC* value) {
  if (value) {
    stream << "D3D12_INPUT_LAYOUT_DESC{";
    if (value->pInputElementDescs) {
      stream << "[";
      for (UINT i = 0; i < value->NumElements; ++i) {
        stream << value->pInputElementDescs[i];
        if (i < value->NumElements - 1) {
          stream << ", ";
        }
      }
      stream << "], ";
    } else {
      stream << "nullptr, ";
    }
    stream << value->NumElements;
    stream << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_INPUT_ELEMENT_DESC& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_INPUT_ELEMENT_DESC* value) {
  if (value) {
    stream << "{";
    stream << std::string(value->SemanticName) << ", ";
    stream << value->SemanticIndex << ", ";
    stream << value->Format << ", ";
    stream << value->InputSlot << ", ";
    stream << value->AlignedByteOffset << ", ";
    stream << value->InputSlotClass << ", ";
    stream << value->InstanceDataStepRate;
    stream << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_SO_DECLARATION_ENTRY& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_SO_DECLARATION_ENTRY* value) {
  if (value) {
    stream << "{";
    stream << value->Stream << ", ";
    stream << std::string(value->SemanticName) << ", ";
    stream << value->SemanticIndex << ", ";
    stream << value->StartComponent << ", ";
    stream << value->ComponentCount << ", ";
    stream << value->OutputSlot;
    stream << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_CACHED_PIPELINE_STATE& value) {
  return stream << &value;
}

FastOStream& operator<<(FastOStream& stream, const D3D12_CACHED_PIPELINE_STATE* value) {
  if (value) {
    stream << "D3D12_CACHED_PIPELINE_STATE{" << value->CachedBlobSizeInBytes << "}";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const DSTORAGE_REQUEST& value) {
  stream << "DSTORAGE_REQUEST{";
  stream << value.Options << ", ";
  switch (value.Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_MEMORY:
    stream << value.Source.Memory << ", ";
    break;
  case DSTORAGE_REQUEST_SOURCE_FILE:
    stream << value.Source.File << ", ";
    break;
  default:
    stream << "unknown, ";
  }
  switch (value.Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_MEMORY:
    stream << value.Destination.Memory << ", ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    stream << value.Destination.Buffer << ", ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    stream << value.Destination.Texture << ", ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    stream << value.Destination.MultipleSubresources << ", ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    stream << value.Destination.Tiles << ", ";
    break;
  default:
    stream << "unknown, ";
  }
  stream << value.UncompressedSize << ", ";
  stream << value.CancellationTag << ", ";
  if (value.Name) {
    stream << value.Name;
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const DSTORAGE_REQUEST* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionInfo& value) {
  stream << "INTCExtensionInfo{";
  stream << value.RequestedExtensionVersion << ", ";
  stream << value.IntelDeviceInfo << ", ";
  printString(stream, value.pDeviceDriverDesc) << ", ";
  printString(stream, value.pDeviceDriverVersion) << ", ";
  stream << value.DeviceDriverBuildNumber;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionInfo* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionVersion& value) {
  stream << "INTCExtensionVersion{";
  stream << value.HWFeatureLevel << ", ";
  stream << value.APIVersion << ", ";
  stream << value.Revision;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionVersion* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCDeviceInfo& value) {
  stream << "INTCDeviceInfo{";
  stream << value.GPUMaxFreq << ", ";
  stream << value.GPUMinFreq << ", ";
  stream << value.GTGeneration << ", ";
  stream << value.EUCount << ", ";
  stream << value.PackageTDP << ", ";
  stream << value.MaxFillRate << ", ";
  printString(stream, value.GTGenerationName);
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCDeviceInfo* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo& value) {
  stream << "INTCExtensionAppInfo{";
  printString(stream, value.pApplicationName) << ", ";
  stream << value.ApplicationVersion << ", ";
  printString(stream, value.pEngineName) << ", ";
  stream << value.EngineVersion;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCAppInfoVersion& value) {
  stream << "INTCAppInfoVersion{";
  stream << value.major << ", ";
  stream << value.minor << ", ";
  stream << value.patch;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCAppInfoVersion* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo1& value) {
  stream << "INTCExtensionAppInfo1{";
  printString(stream, value.pApplicationName) << ", ";
  stream << value.ApplicationVersion << ", ";
  printString(stream, value.pEngineName) << ", ";
  stream << value.EngineVersion;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo1* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS_EX value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_EX value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_FORMAT value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_LSS_ENDCAP_MODE value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_LSS_PRIMITIVE_FORMAT value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BUILD_FLAGS value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_TYPE value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_FLAGS value) {
  return stream << toStr(value);
}

FastOStream& operator<<(
    FastOStream& stream,
    NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_CLUSTER_FLAGS value) {
  return stream << toStr(value);
}

FastOStream& operator<<(
    FastOStream& stream,
    NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_GEOMETRY_FLAGS value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_INDEX_FORMAT value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_MODE value) {
  return stream << toStr(value);
}

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_MOVE_TYPE value) {
  return stream << toStr(value);
}

FastOStream& operator<<(
    FastOStream& stream,
    NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_ADDRESS_RESOLUTION_FLAGS value) {
  return stream << toStr(value);
}

} // namespace DirectX
} // namespace gits
