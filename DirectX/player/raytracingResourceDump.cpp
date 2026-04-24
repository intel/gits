// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingResourceDump.h"
#include "log.h"

#include <fstream>
#include <iomanip>

namespace gits {
namespace DirectX {

RaytracingResourceDump::~RaytracingResourceDump() {
  WaitUntilDumped();
}

void RaytracingResourceDump::DumpResource(ID3D12GraphicsCommandList* commandList,
                                          ID3D12Resource* resource,
                                          unsigned offset,
                                          unsigned size,
                                          unsigned stride,
                                          BarrierState resourceState,
                                          const std::wstring& dumpName,
                                          DumpContentKind contentKind,
                                          bool fromCapture) {

  RaytracingDumpInfo* dumpInfo = new RaytracingDumpInfo();
  dumpInfo->ContentKind = contentKind;
  dumpInfo->Offset = offset;
  dumpInfo->Size = size;
  dumpInfo->Stride = stride;
  dumpInfo->DumpName = dumpName;
  dumpInfo->FromCapture = fromCapture;

  StageResource(commandList, resource, resourceState, *dumpInfo);
}

void RaytracingResourceDump::DumpBuffer(DumpInfo& dumpInfo, void* data) {
  RaytracingDumpInfo& info = static_cast<RaytracingDumpInfo&>(dumpInfo);
  if (info.ContentKind == DumpContentKind::Instances) {
    DumpInstancesBuffer(info, data);
  } else if (info.ContentKind == DumpContentKind::InstancesArrayOfPointers) {
    DumpInstancesArrayOfPointersBuffer(info, data);
  } else if (info.ContentKind == DumpContentKind::BindingTable) {
    DumpBindingTableBuffer(info, data);
  }
}

void RaytracingResourceDump::DumpInstancesBuffer(RaytracingDumpInfo& dumpInfo, void* data) {
  std::ofstream stream(dumpInfo.DumpName);
  D3D12_RAYTRACING_INSTANCE_DESC* instances = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(data);
  unsigned count = dumpInfo.Size / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

  for (unsigned i = 0; i < count; ++i) {
    stream << "INSTANCE " << i + 1 << "\n";

    stream << "  Transform {";
    for (unsigned j = 0; j < 3; ++j) {
      if (j > 0) {
        stream << ", ";
      }
      stream << "{";
      for (unsigned k = 0; k < 3; ++k) {
        if (k > 0) {
          stream << ", ";
        }
        stream << instances[i].Transform[j][k];
      }
      stream << "}";
    }
    stream << "}\n";

    stream << "  InstanceID " << instances[i].InstanceID << "\n";
    stream << "  InstanceMask " << instances[i].InstanceMask << "\n";
    stream << "  InstanceContributionToHitGroupIndex "
           << instances[i].InstanceContributionToHitGroupIndex << "\n";
    stream << "  Flags " << instances[i].Flags << "\n";

    stream << "  AccelerationStructure " << std::hex << std::setw(16) << std::setfill('0')
           << instances[i].AccelerationStructure << std::dec;
    CapturePlayerGpuAddressService::ResourceInfo* info{};
    unsigned offset{};
    if (dumpInfo.FromCapture) {
      info = m_AddressService.GetResourceInfoByCaptureAddress(instances[i].AccelerationStructure);
    } else {
      info = m_AddressService.GetResourceInfoByPlayerAddress(instances[i].AccelerationStructure);
    }
    if (!info) {
      stream << " NOT FOUND\n";
    } else {
      if (dumpInfo.FromCapture) {
        offset = instances[i].AccelerationStructure - info->CaptureStart;
      } else {
        offset = instances[i].AccelerationStructure - info->PlayerStart;
      }
      stream << " blas O" << info->Key << " offset " << offset << "\n";
    }
  }
}

void RaytracingResourceDump::DumpInstancesArrayOfPointersBuffer(RaytracingDumpInfo& dumpInfo,
                                                                void* data) {
  std::ofstream stream(dumpInfo.DumpName);
  D3D12_GPU_VIRTUAL_ADDRESS* addresses = static_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(data);
  unsigned count = dumpInfo.Size / sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
  for (unsigned i = 0; i < count; ++i) {
    PrintGpuAddress(stream, addresses[i], dumpInfo.FromCapture);
    stream << "\n";
  }
}

void RaytracingResourceDump::DumpBindingTableBuffer(RaytracingDumpInfo& dumpInfo, void* data) {
  std::ofstream stream(dumpInfo.DumpName);
  unsigned recordCount = dumpInfo.Size / dumpInfo.Stride;
  for (unsigned recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
    stream << "BINDING TABLE " << recordIndex + 1 << " size " << dumpInfo.Stride << "\n";

    uint8_t* p = static_cast<uint8_t*>(data) + recordIndex * dumpInfo.Stride;
    CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
    memcpy(shaderIdentifier.data(), p, shaderIdentifier.size());
    stream << std::hex;
    for (uint8_t byte : shaderIdentifier) {
      stream << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    }
    stream << " ";
    std::wstring wstr;
    if (dumpInfo.FromCapture) {
      wstr = m_ShaderIdentifierService.GetExportNameByCaptureIdentifier(shaderIdentifier);
    } else {
      wstr = m_ShaderIdentifierService.GetExportNameByPlayerIdentifier(shaderIdentifier);
    }
    std::string str(wstr.begin(), wstr.end());
    stream << std::dec << str << "\n";
    p += shaderIdentifier.size();

    unsigned count = (dumpInfo.Stride - shaderIdentifier.size()) / sizeof(UINT64);
    for (unsigned i = 0; i < count; ++i) {
      UINT64* address = reinterpret_cast<UINT64*>(p + sizeof(UINT64) * i);
      stream << std::hex << std::setw(16) << std::setfill('0') << *address << std::dec;

      CapturePlayerGpuAddressService::ResourceInfo* resourceInfo{};
      if (dumpInfo.FromCapture) {
        resourceInfo = m_AddressService.GetResourceInfoByCaptureAddress(*address);
      } else {
        resourceInfo = m_AddressService.GetResourceInfoByPlayerAddress(*address);
      }
      if (resourceInfo) {
        unsigned offset{};
        if (dumpInfo.FromCapture) {
          offset = *address - resourceInfo->CaptureStart;
        } else {
          offset = *address - resourceInfo->PlayerStart;
        }
        stream << " resource O" << resourceInfo->Key << " offset " << offset;
      }

      CapturePlayerDescriptorHandleService::DescriptorHeapInfo* heapInfo{};
      unsigned stride{};
      if (dumpInfo.FromCapture) {
        heapInfo = m_DescriptorHandleService.GetViewDescriptorHeapInfoByCaptureHandle(*address);
        stride = m_DescriptorHandleService.ViewHeapIncrement();
        if (!heapInfo) {
          heapInfo =
              m_DescriptorHandleService.GetSamplerDescriptorHeapInfoByCaptureHandle(*address);
          stride = m_DescriptorHandleService.SamplerHeapIncrement();
        }
      } else {
        heapInfo = m_DescriptorHandleService.GetViewDescriptorHeapInfoByPlayerHandle(*address);
        stride = m_DescriptorHandleService.ViewHeapIncrement();
        if (!heapInfo) {
          heapInfo = m_DescriptorHandleService.GetSamplerDescriptorHeapInfoByPlayerHandle(*address);
          stride = m_DescriptorHandleService.SamplerHeapIncrement();
        }
      }
      if (heapInfo) {
        unsigned index{};
        if (dumpInfo.FromCapture) {
          index = (*address - heapInfo->CaptureStart) / stride;
        } else {
          index = (*address - heapInfo->PlayerStart) / stride;
        }
        stream << " descriptor heap O" << heapInfo->Key << " index " << index;
      }

      stream << "\n";
    }
  }
}

void RaytracingResourceDump::PrintGpuAddress(std::ostream& stream,
                                             D3D12_GPU_VIRTUAL_ADDRESS address,
                                             bool fromCapture) {
  std::ios state(nullptr);
  state.copyfmt(stream);

  stream << "{0x" << std::hex << std::setw(16) << std::setfill('0') << address << std::dec;
  if (address) {
    CapturePlayerGpuAddressService::ResourceInfo* info{};
    unsigned offset{};
    if (fromCapture) {
      info = m_AddressService.GetResourceInfoByCaptureAddress(address);
    } else {
      info = m_AddressService.GetResourceInfoByPlayerAddress(address);
    }
    if (!info) {
      stream << ", NOT FOUND";
    } else {
      if (fromCapture) {
        offset = address - info->CaptureStart;
      } else {
        offset = address - info->PlayerStart;
      }
      stream << ", key O" << info->Key << ", offset " << offset;
    }
  }
  stream << "}";

  stream.copyfmt(state);
}

} // namespace DirectX
} // namespace gits
