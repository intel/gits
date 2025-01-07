// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclTools.h"

#include "openclArguments.h"
#include "openclArgumentsAuto.h"
#include "openclDrivers.h"
#include "openclHeader.h"
#include "openclStateDynamic.h"
#include "openclFunctionsAuto.h"

#include "gits.h"
#include "exception.h"
#include "recorder.h"

#include "openglEnums.h"

#include <array>
#include <utility>
#include <filesystem>
#include <regex>

namespace gits {
namespace OpenCL {
namespace {
bool CheckArgTypeInfo(cl_kernel kernel, unsigned index) {
  cl_kernel_arg_type_qualifier type_qualifier = 0;
  cl_int err =
      drvOcl.clGetKernelArgInfo(kernel, index, CL_KERNEL_ARG_TYPE_QUALIFIER,
                                sizeof(cl_kernel_arg_type_qualifier), &type_qualifier, nullptr);
  if (err) {
    Log(TRACE) << "Error during clGetKernelArgInfo. Couldn't determine if buffer is constant.";
    return false;
  }
  return type_qualifier & CL_KERNEL_ARG_TYPE_CONST;
}
bool IsReadOnlyObject(cl_kernel kernel, cl_uint index) {
  const auto& cfg = Config::Get();
  auto isReadOnly = false;
  if ((cfg.IsPlayer() ? cfg.opencl.player.omitReadOnlyObjects
                      : cfg.opencl.recorder.omitReadOnlyObjects)) {
    const auto& kernelState = SD().GetKernelState(kernel, EXCEPTION_MESSAGE);
    const auto& arg = kernelState.GetArgument(index);
    if (kernelState.programState->isKernelArgInfoAvailable && CheckArgTypeInfo(kernel, index)) {
      isReadOnly = true;
    } else if (arg.type == KernelArgType::mem) {
      const cl_mem memObj = *reinterpret_cast<const cl_mem*>(arg.argValue);
      const auto& memState = SD().GetMemState(memObj, EXCEPTION_MESSAGE);
      isReadOnly = IsReadOnlyBuffer(memState.flags, !memState.intel_mem_properties.empty()
                                                        ? memState.intel_mem_properties.data()
                                                        : nullptr);
    } else if (arg.type == KernelArgType::svm) {
      const auto allocPtr = GetSvmPtrFromRegion(const_cast<void*>(arg.argValue),
                                                arg.kernelSetType == KernelSetType::normal)
                                .first;
      const auto flags = SD().GetSVMAllocState(allocPtr, EXCEPTION_MESSAGE).flags;
      isReadOnly = IsReadOnlyBuffer(flags);
    } else if (arg.type == KernelArgType::usm) {
      auto allocPtr = GetUsmPtrFromRegion(const_cast<void*>(arg.argValue),
                                          arg.kernelSetType == KernelSetType::normal)
                          .first;
      const auto& props = SD().GetUSMAllocState(allocPtr, EXCEPTION_MESSAGE).properties;
      isReadOnly = IsReadOnlyBuffer(0, props.data());
    }
  }
  return isReadOnly;
}
bool DumpableObject(KernelArgType type) {
  return (type == KernelArgType::mem) || (type == KernelArgType::usm) ||
         (type == KernelArgType::svm);
}
} // namespace

void MaskAppend(std::string& str, const std::string& maskStr) {
  if (str.size()) {
    str += " | ";
  }
  str += maskStr;
}

COclLog::COclLog(LogLevel prefix, LogStyle style) : CLog(prefix, style) {}

COclLog& COclLog::operator<<(manip t) {
  _buffer << t;
  return *this;
}

COclLog& COclLog::operator<<(const char c) {
  return operator<< <int>(c);
}

COclLog& COclLog::operator<<(const unsigned char c) {
  return operator<< <unsigned>(c);
}

COclLog& COclLog::operator<<(const char* c) {
  if (c != nullptr) {
    _buffer << c;
  } else {
    _buffer << (const void*)c;
  }
  return *this;
}

COclLog& COclLog::operator<<(char* c) {
  _buffer << (const void*)c;
  return *this;
}

size_t PixelSize(const cl_image_format& imageFormat) {
  int channelSize = 0;
  switch (imageFormat.image_channel_data_type) {
  case CL_SNORM_INT8:
  case CL_UNORM_INT8:
  case CL_SIGNED_INT8:
  case CL_UNSIGNED_INT8:
    channelSize = 1;
    break;
  case CL_SNORM_INT16:
  case CL_UNORM_INT16:
  case CL_UNORM_SHORT_565:
  case CL_UNORM_SHORT_555:
  case CL_SIGNED_INT16:
  case CL_UNSIGNED_INT16:
  case CL_HALF_FLOAT:
    channelSize = 2;
    break;
  case CL_UNORM_INT_101010:
  case CL_SIGNED_INT32:
  case CL_UNSIGNED_INT32:
  case CL_FLOAT:
    channelSize = 4;
    break;
  default:
    throw ENotSupported(EXCEPTION_MESSAGE);
  }

  int planesNum = 0;
  switch (imageFormat.image_channel_order) {
  case CL_R:
  case CL_A:
  case CL_INTENSITY:
  case CL_LUMINANCE:
  case CL_DEPTH:
    planesNum = 1;
    break;
  case CL_RG:
  case CL_RA:
  case CL_Rx:
    planesNum = 2;
    break;
  case CL_RGB:
  case CL_RGx:
    planesNum = 3;
    break;
  case CL_RGBA:
  case CL_BGRA:
  case CL_ARGB:
  case CL_RGBx:
    planesNum = 4;
    break;
  default:
    throw ENotSupported(EXCEPTION_MESSAGE);
  }

  return channelSize * planesNum;
}

size_t QuerySize(const cl_mem clMem) {
  size_t size;
  if (drvOcl.clGetMemObjectInfo(clMem, CL_MEM_SIZE, sizeof(size_t), &size, nullptr) != CL_SUCCESS) {
    Log(ERR) << "Could not obtain mem object information for an OpenGL buffer";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  return size;
}

cl_image_desc GetImageDescFromQueryImageParams(const cl_mem clMem, cl_image_format& format) {
  cl_image_desc desc = {};
  cl_int err = drvOcl.clGetMemObjectInfo(clMem, CL_MEM_TYPE, sizeof(cl_mem_object_type),
                                         &desc.image_type, nullptr);
  if (desc.image_type == CL_MEM_OBJECT_BUFFER || desc.image_type == CL_MEM_OBJECT_PIPE) {
    Log(ERR) << "cl_mem object is not an image.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_WIDTH, sizeof(size_t), &desc.image_width, nullptr);
  err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_HEIGHT, sizeof(size_t), &desc.image_height, nullptr);
  err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_ROW_PITCH, sizeof(size_t), &desc.image_row_pitch,
                               nullptr);
  err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_FORMAT, sizeof(cl_image_format), &format, nullptr);
  switch (desc.image_type) {
  case CL_MEM_OBJECT_IMAGE1D:
  case CL_MEM_OBJECT_IMAGE1D_BUFFER:
    desc.image_height = 1;
    break;
  case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_ARRAY_SIZE, sizeof(size_t), &desc.image_array_size,
                                 nullptr);
    break;
  case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_SLICE_PITCH, sizeof(size_t),
                                 &desc.image_slice_pitch, nullptr);
    err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_ARRAY_SIZE, sizeof(size_t), &desc.image_array_size,
                                 nullptr);
    break;
  case CL_MEM_OBJECT_IMAGE3D:
    err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_SLICE_PITCH, sizeof(size_t),
                                 &desc.image_slice_pitch, nullptr);
    err |= drvOcl.clGetImageInfo(clMem, CL_IMAGE_DEPTH, sizeof(size_t), &desc.image_depth, nullptr);
    break;
  }
  if (err != CL_SUCCESS) {
    Log(ERR) << "Unable to query image data. Error code was "
             << gits::OpenCL::CLResultToString(err);
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return desc;
}

using image_format_description = std::pair<cl_channel_order, cl_channel_type>;

std::map<image_format_description, texel_type> imageFormatTexelMap = {
    {{CL_A, CL_UNORM_INT8}, texel_type::A8},
    {{CL_R, CL_UNORM_INT8}, texel_type::R8},
    {{CL_R, CL_SNORM_INT8}, texel_type::R8snorm},
    {{CL_R, CL_UNSIGNED_INT8}, texel_type::R8ui},
    {{CL_R, CL_SIGNED_INT8}, texel_type::R8i},

    {{CL_R, CL_UNORM_INT16}, texel_type::R16},
    {{CL_R, CL_SNORM_INT16}, texel_type::R16snorm},
    {{CL_R, CL_UNSIGNED_INT16}, texel_type::R16ui},
    {{CL_R, CL_SIGNED_INT16}, texel_type::R16i},
    {{CL_R, CL_HALF_FLOAT}, texel_type::R16f},

    {{CL_RG, CL_UNORM_INT8}, texel_type::RG8},
    {{CL_RG, CL_SNORM_INT8}, texel_type::RG8snorm},
    {{CL_RG, CL_UNSIGNED_INT8}, texel_type::RG8ui},
    {{CL_RG, CL_SIGNED_INT8}, texel_type::RG8i},

    {{CL_R, CL_UNSIGNED_INT32}, texel_type::R32ui},
    {{CL_R, CL_SIGNED_INT32}, texel_type::R32i},
    {{CL_R, CL_FLOAT}, texel_type::R32f},

    {{CL_RG, CL_UNORM_INT16}, texel_type::RG16},
    {{CL_RG, CL_SNORM_INT16}, texel_type::RG16snorm},
    {{CL_RG, CL_UNSIGNED_INT16}, texel_type::RG16ui},
    {{CL_RG, CL_SIGNED_INT16}, texel_type::RG16i},
    {{CL_RG, CL_HALF_FLOAT}, texel_type::RG16f},

    {{CL_RGBA, CL_UNORM_INT8}, texel_type::RGBA8},
    {{CL_RGBA, CL_SNORM_INT8}, texel_type::RGBA8snorm},
    {{CL_RGBA, CL_UNSIGNED_INT8}, texel_type::RGBA8ui},
    {{CL_RGBA, CL_SIGNED_INT8}, texel_type::RGBA8i},

    {{CL_RG, CL_UNSIGNED_INT32}, texel_type::RG32ui},
    {{CL_RG, CL_SIGNED_INT32}, texel_type::RG32i},
    {{CL_RG, CL_FLOAT}, texel_type::RG32f},

    {{CL_RGBA, CL_UNORM_INT16}, texel_type::RGBA16},
    {{CL_RGBA, CL_SNORM_INT16}, texel_type::RGBA16snorm},
    {{CL_RGBA, CL_UNSIGNED_INT16}, texel_type::RGBA16ui},
    {{CL_RGBA, CL_SIGNED_INT16}, texel_type::RGBA16i},
    {{CL_RGBA, CL_HALF_FLOAT}, texel_type::RGBA16f},

    {{CL_RGBA, CL_UNSIGNED_INT32}, texel_type::RGBA32ui},
    {{CL_RGBA, CL_SIGNED_INT32}, texel_type::RGBA32i},
    {{CL_RGBA, CL_FLOAT}, texel_type::RGBA32f},

    {{CL_BGRA, CL_UNORM_INT8}, texel_type::BGRA8},
    {{CL_BGRA, CL_SNORM_INT8}, texel_type::BGRA8snorm},
    {{CL_BGRA, CL_UNSIGNED_INT8}, texel_type::BGRA8ui},
    {{CL_BGRA, CL_SIGNED_INT8}, texel_type::BGRA8i},
};

texel_type GetTexelToConvertFromImageFormat(const cl_image_format& format) {
  auto it = imageFormatTexelMap.find(
      std::make_pair(format.image_channel_order, format.image_channel_data_type));
  if (it != imageFormatTexelMap.end()) {
    return it->second;
  }
  throw ENotImplemented((std::string)EXCEPTION_MESSAGE +
                        "\nclCaptureImages not implemented for:\n\t" +
                        cl_channel_orderToString(format.image_channel_order) + "\n\t" +
                        cl_channel_typeToString(format.image_channel_data_type));
}

std::array<size_t, 3> GetSimplifiedImageSizes(const cl_image_desc& desc) {
  switch (desc.image_type) {
  case CL_MEM_OBJECT_IMAGE1D:
    return {desc.image_width, 1, 1};
  case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    return {desc.image_width, desc.image_array_size, 1};
  case CL_MEM_OBJECT_IMAGE2D:
    return {desc.image_width, desc.image_height, 1};
  case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    return {desc.image_width, desc.image_height, desc.image_array_size};
  case CL_MEM_OBJECT_IMAGE3D:
    return {desc.image_width, desc.image_height, desc.image_depth};
  default:
    Log(WARN) << "Unsupported image type: " << desc.image_type;
    return {1, 1, 1};
  }
}

void SaveImage(char* image,
               const cl_image_format& format,
               const cl_image_desc& desc,
               const std::string& name) {
  auto& cfg = Config::Get();
  const unsigned rgba8TexelSize = 4;
  const auto dumpPath = GetDumpPath(cfg);
  std::filesystem::create_directories(dumpPath);

  const size_t imagesMemorySize = CountImageSize(format, desc);
  const std::array<size_t, 3> sizes = GetSimplifiedImageSizes(desc);
  const size_t width = sizes[0], height = sizes[1], depth = sizes[2];

  try {
    auto texelType = GetTexelToConvertFromImageFormat(format);
    const size_t imageSize = imagesMemorySize / depth;
    for (unsigned i = 0; i < depth; i++) {
      std::vector<uint8_t> frame(image + imageSize * i, image + imageSize * (i + 1));
      if (format.image_channel_data_type == CL_FLOAT) {
        normalize_texture_data(texelType, frame, static_cast<int>(width), static_cast<int>(height));
      }
      std::vector<uint8_t> convertedData(width * height * rgba8TexelSize);
      convert_texture_data(texelType, frame, texel_type::BGRA8, convertedData,
                           static_cast<int>(width), static_cast<int>(height));

      std::string fileName = (depth == 1 ? name : name + "-" + std::to_string(i)) + ".png";
      std::filesystem::path path = dumpPath / fileName;
      CGits::Instance().WriteImage(path.string(), width, height, true, convertedData, false, true);
    }
  } catch (const ENotImplemented& ex) {
    Log(ERR) << ex.what();
  }
}

void SaveBuffer(const std::string& name, const std::vector<char>& data) {
  auto& cfg = Config::Get();
  std::string filename = name + ".dat";
  const auto dumpPath = GetDumpPath(cfg);
  std::filesystem::path path = dumpPath / filename;
  std::filesystem::create_directories(path.parent_path());
  std::ofstream binStream(path, std::ofstream::binary);
  binStream.write(data.data(), data.size());
  binStream.close();
}

void SaveBuffer(const std::string& name, const CBinaryResource& data) {
  auto& cfg = Config::Get();
  std::string filename = name + ".dat";
  const auto dumpPath = GetDumpPath(cfg);
  std::filesystem::path path = dumpPath / filename;
  std::filesystem::create_directories(path.parent_path());
  std::ofstream binStream(path, std::ofstream::binary);
  binStream.write((const char*)data.Data(), data.Data().Size());
  binStream.close();
}

void D3DWarning() {
  static auto called = false;
  if (!called && !IsDXUnsharingEnabled(Config::Get())) {
    Log(WARN) << "DX-sharing is used. In order to make the stream replayable, include 'DX' in "
                 "RemoveAPISharing recorder's option";
    called = true;
  }
}

size_t CountImageSize(const cl_image_format& imageFormat,
                      size_t imageWidth,
                      size_t imageHeight,
                      size_t rowPitch) {
  return rowPitch ? rowPitch * imageHeight : imageWidth * PixelSize(imageFormat) * imageHeight;
}

size_t CountImageSize(const cl_image_format& imageFormat,
                      size_t imageWidth,
                      size_t imageHeight,
                      size_t imageDepth,
                      size_t rowPitch,
                      size_t slicePitch) {
  auto rowSize = rowPitch ? rowPitch : imageWidth * PixelSize(imageFormat);
  auto sliceSize = slicePitch ? slicePitch : rowSize * imageHeight;
  return sliceSize * imageDepth;
}

size_t CountImageSize(const cl_image_format& imageFormat,
                      const size_t region[],
                      size_t rowPitch,
                      size_t slicePitch) {
  return CountImageSize(imageFormat, region[0], region[1], region[2], rowPitch, slicePitch);
}

size_t CountImageSize(const cl_image_format& imageFormat, const cl_image_desc& imageDesc) {
  size_t retSize = 0;
  switch (imageDesc.image_type) {
  case CL_MEM_OBJECT_IMAGE1D:
  case CL_MEM_OBJECT_IMAGE1D_BUFFER:
    retSize = imageDesc.image_row_pitch ? imageDesc.image_row_pitch
                                        : imageDesc.image_width * PixelSize(imageFormat);
    break;
  case CL_MEM_OBJECT_IMAGE2D:
    retSize = imageDesc.image_row_pitch ? imageDesc.image_row_pitch
                                        : imageDesc.image_width * PixelSize(imageFormat);
    retSize *= imageDesc.image_height;
    break;
  case CL_MEM_OBJECT_IMAGE3D:
    retSize = CountImageSize(imageFormat, imageDesc.image_width, imageDesc.image_height,
                             imageDesc.image_depth, imageDesc.image_row_pitch,
                             imageDesc.image_slice_pitch);
    break;
  case CL_MEM_OBJECT_IMAGE1D_ARRAY:
  case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    retSize = CountImageSize(imageFormat, imageDesc.image_width, imageDesc.image_height,
                             imageDesc.image_array_size, imageDesc.image_row_pitch,
                             imageDesc.image_slice_pitch);
    break;
  default:
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  return retSize;
}

size_t CountBufferRectSize(const size_t region[], size_t rowPitch, size_t slicePitch) {
  auto rowSize = rowPitch ? rowPitch : region[0];
  auto sliceSize = slicePitch ? slicePitch : rowSize * region[1];
  return sliceSize * region[2];
}

void GetRegionForWholeImage(const cl_image_desc description, size_t* region) {
  region[0] = description.image_width;
  region[1] = 1;
  region[2] = 1;
  switch (description.image_type) {
    // IMAGE1D and IMAGE1D_BUFFER omitted intentionally
  case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    region[1] = description.image_array_size;
    break;
  case CL_MEM_OBJECT_IMAGE2D:
    region[1] = description.image_height;
    break;
  case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    region[2] = description.image_array_size;
    break;
  case CL_MEM_OBJECT_IMAGE3D:
    region[1] = description.image_height;
    region[2] = description.image_depth;
    break;
  }
}

bool ErrCodeSuccess(cl_int status) {
  return status == CL_SUCCESS;
}

bool ErrCodeSuccess(cl_int* errCodeRet) {
  return (errCodeRet == nullptr || *errCodeRet == CL_SUCCESS);
}

bool FlagUseHostPtr(cl_mem_flags flags) {
  return (flags & CL_MEM_USE_HOST_PTR);
}

cl_uint GetRefCount(cl_command_queue obj) {
  try {
    return SD().GetCommandQueueState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_context obj) {
  try {
    return SD().GetContextState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_device_id obj) {
  try {
    return SD().GetDeviceIDState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_event obj) {
  try {
    return SD().GetEventState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_kernel obj) {
  try {
    return SD().GetKernelState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_mem obj) {
  try {
    return SD().GetMemState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_program obj) {
  try {
    return SD().GetProgramState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(cl_sampler obj) {
  try {
    return SD().GetSamplerState(obj, EXCEPTION_MESSAGE).GetRefCount();
  } catch (std::runtime_error&) {
    return 0;
  }
}

cl_uint GetRefCount(void* obj) {
  // This is meant to be 0. CCLMappedPtr is not recursive and mapped differently,
  // than the rest of CCLArgObj.
  return 0;
}

template <class T>
void RegisterEvents(cl_event* event, cl_command_queue cmdQueue, T errCodeRet) {
  if (event && ErrCodeSuccess(errCodeRet)) {
    auto& eventState = SD()._eventStates[*event];
    eventState.reset(new CCLEventState());
    eventState->Retain();
    eventState->context = SD()._commandQueueStates[cmdQueue]->context;
    SD()._commandQueueStates[cmdQueue]->eventArray.push_back(eventState);
  }
}
template void RegisterEvents<cl_int>(cl_event*, cl_command_queue, cl_int);
template void RegisterEvents<cl_int*>(cl_event*, cl_command_queue, cl_int*);

bool IsDeviceQuery(cl_uint param_name) {
  return param_name == CL_QUEUE_DEVICE || param_name == CL_CONTEXT_DEVICES ||
         param_name == CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR ||
         param_name == CL_DEVICES_FOR_GL_CONTEXT_KHR || param_name == CL_PROGRAM_DEVICES;
}

bool IsGLSharingQuery(const cl_uint& param_name) {
  switch (param_name) {
  case CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR:
  case CL_DEVICES_FOR_GL_CONTEXT_KHR:
  case CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR:
    return true;
  default:
    return false;
  }
}

bool IsDXSharingQuery(const cl_uint& param_name) {
  switch (param_name) {
  case CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR:
  case CL_MEM_D3D10_RESOURCE_KHR:
  case CL_IMAGE_D3D10_SUBRESOURCE_KHR:
  case CL_COMMAND_ACQUIRE_D3D10_OBJECTS_KHR:
  case CL_COMMAND_RELEASE_D3D10_OBJECTS_KHR:
  case CL_CONTEXT_D3D11_PREFER_SHARED_RESOURCES_KHR:
  case CL_MEM_D3D11_RESOURCE_KHR:
  case CL_IMAGE_D3D11_SUBRESOURCE_KHR:
  case CL_COMMAND_ACQUIRE_D3D11_OBJECTS_KHR:
  case CL_COMMAND_RELEASE_D3D11_OBJECTS_KHR:
  case CL_MEM_DX9_MEDIA_ADAPTER_TYPE_KHR:
  case CL_MEM_DX9_MEDIA_SURFACE_INFO_KHR:
  case CL_IMAGE_DX9_MEDIA_PLANE_KHR:
  case CL_COMMAND_ACQUIRE_DX9_MEDIA_SURFACES_KHR:
  case CL_COMMAND_RELEASE_DX9_MEDIA_SURFACES_KHR:
  case CL_MEM_DX9_RESOURCE_INTEL:
  case CL_MEM_DX9_SHARED_HANDLE_INTEL:
  case CL_IMAGE_DX9_PLANE_INTEL:
  case CL_COMMAND_ACQUIRE_DX9_OBJECTS_INTEL:
  case CL_COMMAND_RELEASE_DX9_OBJECTS_INTEL:
    return true;
  default:
    return false;
  }
}

bool IsSharingQuery(const cl_uint& param_name) {
  return IsDXSharingQuery(param_name) || IsGLSharingQuery(param_name);
}

bool IsGLUnsharingEnabled(const Config& cfg) {
  return cfg.common.recorder.removeGLSharing;
}

bool IsDXUnsharingEnabled(const Config& cfg) {
  return cfg.common.recorder.removeDXSharing;
}

bool IsUnsharingEnabled(const Config& cfg) {
  return cfg.common.recorder.removeGLSharing || cfg.common.recorder.removeDXSharing;
}

bool IsGLSharingFunction(const std::string& functionName) {
  static const std::vector<std::string> sharingFunctions{
      "clGetGLContextInfoKHR",     "clGetGLObjectInfo",          "clGetGLTextureInfo",
      "clCreateFromGLBuffer",      "clCreateFromGLRenderbuffer", "clCreateFromGLTexture",
      "clCreateFromGLTexture2D",   "clCreateFromGLTexture3D",    "clEnqueueAcquireGLObjects",
      "clEnqueueReleaseGLObjects", "clCreateEventFromGLsyncKHR",
  };
  return std::find(sharingFunctions.begin(), sharingFunctions.end(), functionName) !=
         sharingFunctions.end();
}

bool IsDXSharingFunction(const std::string& functionName) {
  static const std::vector<std::string> sharingFunctions{"clGetDeviceIDsFromD3D10KHR",
                                                         "clCreateFromD3D10BufferKHR",
                                                         "clCreateFromD3D10Texture2DKHR",
                                                         "clCreateFromD3D10Texture2DKHR",
                                                         "clEnqueueAcquireD3D10ObjectsKHR",
                                                         "clEnqueueReleaseD3D10ObjectsKHR",
                                                         "clGetDeviceIDsFromD3D11KHR",
                                                         "clCreateFromD3D11BufferKHR",
                                                         "clCreateFromD3D11Texture2DKHR",
                                                         "clCreateFromD3D11Texture3DKHR",
                                                         "clEnqueueAcquireD3D11ObjectsKHR",
                                                         "clEnqueueReleaseD3D11ObjectsKHR",
                                                         "clGetDeviceIDsFromD3D11NV",
                                                         "clCreateFromD3D11BufferNV",
                                                         "clCreateFromD3D11Texture2DNV",
                                                         "clCreateFromD3D11Texture3DNV",
                                                         "clEnqueueAcquireD3D11ObjectsNV",
                                                         "clEnqueueReleaseD3D11ObjectsNV",
                                                         "clGetDeviceIDsFromDX9INTEL",
                                                         "clCreateFromDX9MediaSurfaceINTEL",
                                                         "clEnqueueAcquireDX9ObjectsINTEL",
                                                         "clEnqueueReleaseDX9ObjectsINTEL",
                                                         "clGetDeviceIDsFromDX9MediaAdapterKHR",
                                                         "clCreateFromDX9MediaSurfaceKHR",
                                                         "clEnqueueAcquireDX9MediaSurfacesKHR",
                                                         "clEnqueueReleaseDX9MediaSurfacesKHR"};
  return std::find(sharingFunctions.begin(), sharingFunctions.end(), functionName) !=
         sharingFunctions.end();
}

void clGetContextInfo_SetMapping(cl_device_id* old_devices,
                                 size_t num_old_devices,
                                 cl_device_id* devices,
                                 size_t paramValueSize,
                                 size_t* paramValueRetSize) {
  // The actualCount is a WA for Rightware which created a buffer, filled it with garbage and then used only the first
  // device anyway. Let's zero out the first device after all valid ones and just don't create objects for the rest.
  // This allows us to maintain backwards compatibility when reading streams and zero can be a valid garbage value anyway.
  if (paramValueRetSize != nullptr && paramValueSize > *paramValueRetSize) {
    auto actualCount = *paramValueRetSize / sizeof(cl_device_id);
    if (actualCount < num_old_devices) {
      devices[actualCount] = nullptr;
    }
  }

  size_t num_devices = paramValueSize / sizeof(cl_device_id);
  for (size_t i = 0; i < num_devices && devices[i]; i++) {
    Ccl_device_id::AddMapping(old_devices[i], devices[i]);
  }
}

template <typename T>
std::vector<T> PropertiesVectorWrapZeroEnded(const T* props) {
  try {
    std::vector<T> propsVec;
    while (*props) {
      propsVec.push_back(*props);
      propsVec.push_back(*(++props));
      ++props;
    }
    propsVec.push_back(0);
    return propsVec;
  } catch (std::runtime_error&) {
    Log(ERR) << "Exception during wrapping cl*properties std::vector";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}
template <typename T>
std::vector<T> RemoveProperties(const T* props, const std::vector<T> propsToRemove) {
  auto propsVec = PropertiesVectorWrapZeroEnded(props);
  std::vector<T> newProps;
  auto len = propsVec.size();
  for (size_t i = 0; i < len - 1; i += 2) {
    if (propsVec[i] != 0 &&
        (std::find(begin(propsToRemove), end(propsToRemove), propsVec[i]) == end(propsToRemove))) {
      newProps.push_back(propsVec[i]);
      newProps.push_back(propsVec[i + 1]);
    }
  }
  newProps.push_back(0);
  return newProps;
}

std::vector<cl_context_properties> RemoveGLSharingContextProperties(
    const cl_context_properties* props) {
  static const std::vector<cl_context_properties> propsToRemove{
      CL_GL_CONTEXT_KHR,
      CL_EGL_DISPLAY_KHR,
      CL_GLX_DISPLAY_KHR,
      CL_WGL_HDC_KHR,
  };
  return RemoveProperties(props, propsToRemove);
}

std::vector<cl_context_properties> RemoveDXSharingContextProperties(
    const cl_context_properties* props) {
  static const std::vector<cl_context_properties> propsToRemove{
      CL_CONTEXT_D3D10_DEVICE_KHR,    CL_CONTEXT_D3D11_DEVICE_KHR, CL_CONTEXT_ADAPTER_D3D9_KHR,
      CL_CONTEXT_ADAPTER_D3D9EX_KHR,  CL_CONTEXT_ADAPTER_DXVA_KHR, CL_CONTEXT_D3D9_DEVICE_INTEL,
      CL_CONTEXT_D3D9EX_DEVICE_INTEL, CL_CONTEXT_DXVA_DEVICE_INTEL};
  return RemoveProperties(props, propsToRemove);
}

cl_mem_object_type TextureGLEnumToCLMemType(cl_GLenum textureEnum) {
  cl_mem_object_type type;
  switch (textureEnum) {
  case GL_TEXTURE_1D:
    type = CL_MEM_OBJECT_IMAGE1D;
    break;
  case GL_TEXTURE_1D_ARRAY:
    type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
    break;
  case GL_TEXTURE_2D:
    type = CL_MEM_OBJECT_IMAGE2D;
    break;
  case GL_TEXTURE_2D_ARRAY:
    type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
    break;
  case GL_TEXTURE_3D:
    type = CL_MEM_OBJECT_IMAGE3D;
    break;
  default:
    Log(ERR) << "PLUGIN_UNSHARER: Creating image from GL image not implemented.";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  return type;
}

cl_platform_id ExtractPlatform(const cl_context_properties* props) {
  cl_platform_id platform = nullptr;
  auto propsVec = PropertiesVectorWrapZeroEnded(props);
  for (size_t i = 0; i < propsVec.size(); i += 2) {
    if (propsVec[i] == CL_CONTEXT_PLATFORM) {
      platform = reinterpret_cast<cl_platform_id>(propsVec[i + 1]);
      break;
    }
  }
  return platform;
}

gits::CFunction* NewTokenPtrCreateCLMem(cl_context context,
                                        cl_mem mem,
                                        cl_mem_flags flags,
                                        cl_mem_object_type type) {
  if (!mem) {
    return nullptr; // the API-sharing func call failed - omit creating the unshared call
  }
  gits::CFunction* newFunc = nullptr;
  auto errCode = CL_SUCCESS;
  if (type == CL_MEM_OBJECT_BUFFER) {
    auto size = QuerySize(mem);
    newFunc = new CclCreateBuffer(mem, context, flags, size, nullptr, &errCode);
  } else {
    cl_image_format format = {};
    cl_image_desc desc = GetImageDescFromQueryImageParams(mem, format);
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    // must be 0 if host_ptr == nullptr

    newFunc = new CclCreateImage(mem, context, flags, &format, &desc, nullptr, &errCode);
  }
  return newFunc;
}

gits::CFunction* NewTokenPtrGetDevices(cl_platform_id platform) {
  unsigned deviceType = CL_DEVICE_TYPE_ALL;
  cl_uint numDevices;
  auto err = drvOcl.clGetDeviceIDs(platform, deviceType, 0, nullptr, &numDevices);
  if (err != CL_SUCCESS) {
    Log(ERR) << "PLUGIN_UNSHARER: Unable to inject query of device number";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  //recorder.Schedule(new CclGetDeviceIDs(err, platform, deviceType, 0, nullptr, &numDevices));

  std::vector<cl_device_id> deviceArray(numDevices);
  err = drvOcl.clGetDeviceIDs(platform, deviceType, numDevices, deviceArray.data(), nullptr);
  if (err != CL_SUCCESS) {
    Log(ERR) << "PLUGIN_UNSHARER: Unable to inject device query";
    throw gits::EOperationFailed(EXCEPTION_MESSAGE);
  }
  return new CclGetDeviceIDs(err, platform, deviceType, numDevices, deviceArray.data(), nullptr);
}

void CreateStateFromSharedBuffer(cl_mem return_value,
                                 cl_context context,
                                 cl_mem_flags flags,
                                 cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, QuerySize(return_value)));
    memState->Retain();
  }
}

void CreateStateFromSharedImage(cl_mem return_value,
                                cl_context context,
                                cl_mem_flags flags,
                                cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    cl_image_format format = {};
    cl_image_desc desc = GetImageDescFromQueryImageParams(return_value, format);
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, CountImageSize(format, desc), &format, &desc));
    memState->Retain();
  }
}

bool IsSharingEventFilteringNeeded(const cl_event& event) {
  const auto& cfg = Config::Get().common.recorder;
  const auto& eventState = SD()._eventStates[event];
  const auto haveToRemoveGLEvent = cfg.removeGLSharing && eventState->isGLSharingEvent;
  const auto haveToRemoveDXEvent = cfg.removeDXSharing && eventState->isDXSharingEvent;
  return haveToRemoveDXEvent || haveToRemoveGLEvent;
}

std::vector<cl_event> FilterSharingEvents(const cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list) {
  std::vector<cl_event> filteredEvents;
  for (auto i = 0U; i < num_events_in_wait_list; i++) {
    if (!IsSharingEventFilteringNeeded(event_wait_list[i])) {
      filteredEvents.push_back(event_wait_list[i]);
    }
  }
  return filteredEvents;
}

void InjectKernelArgOperations(cl_kernel kernel,
                               const cl_command_queue command_queue,
                               cl_event* event) {
  const auto& kernelState = SD().GetKernelState(kernel, EXCEPTION_MESSAGE);
  std::vector<std::vector<char>> buffers(kernelState.GetArguments().size());
  std::unordered_map<cl_mem, cl_uint> injectedMemIndex;
  int enqueueCallNumber = CGits::Instance().CurrentKernelCount();
  const auto& cfg = Config::Get();
  for (auto it = kernelState.GetArguments().begin(); it != kernelState.GetArguments().end(); ++it) {
    if (DumpableObject(it->second.type)) {
      if (IsReadOnlyObject(kernel, it->first)) {
        continue;
      }
      SD().layoutBuilder.UpdateLayout(kernelState, enqueueCallNumber, it->first);
      if (cfg.opencl.player.dumpLayoutOnly) {
        continue;
      }
      if (it->second.type == KernelArgType::mem) {
        cl_mem handle = *reinterpret_cast<const cl_mem*>(it->second.argValue);
        if (injectedMemIndex.count(handle)) {
          std::string filename = SD().layoutBuilder.GetFileName();
          SaveBuffer(filename, buffers[injectedMemIndex[handle]]);
          const auto& memState = SD().GetMemState(handle, EXCEPTION_MESSAGE);
          if ((cfg.IsPlayer() ? cfg.opencl.player.captureImages : cfg.opencl.recorder.dumpImages) &&
              memState.image) {
            SaveImage(buffers[injectedMemIndex[handle]].data(), memState.image_format,
                      memState.image_desc, filename);
          }
          continue;
        } else {
          injectedMemIndex[handle] = it->first;
        }
      }
      buffers[it->first] = InjectObjOperations(command_queue, it->second.argValue, event,
                                               SD().layoutBuilder.GetFileName(), it->second.type,
                                               it->second.kernelSetType);
    }
  }
}

std::vector<char> InjectObjOperations(cl_command_queue cmdQ,
                                      const void* argValue,
                                      cl_event* event_wait_list,
                                      std::string fileName,
                                      KernelArgType type,
                                      KernelSetType setType) {
  static int injectedCounter = 0;
  auto& cfg = Config::Get();
  std::vector<char> buffer;
  std::map<size_t, bool> indirectionMap;
  void* hPointer = const_cast<void*>(argValue);
  if (type == KernelArgType::svm) {
    auto allocInfo = GetSvmPtrFromRegion(hPointer, setType == KernelSetType::normal);
    const auto& svmState = SD().GetSVMAllocState(allocInfo.first, EXCEPTION_MESSAGE);
    buffer = std::vector<char>(svmState.size - allocInfo.second, 0);
    indirectionMap = svmState.indirectPointersOffsets;
    drvOcl.clEnqueueSVMMemcpy(cmdQ, CL_BLOCKING, buffer.data(),
                              GetOffsetPointer(allocInfo.first, allocInfo.second), buffer.size(),
                              event_wait_list ? 1 : 0, event_wait_list, nullptr);
  } else if (type == KernelArgType::usm) {
    auto allocInfo = GetUsmPtrFromRegion(hPointer, setType == KernelSetType::normal);
    const auto& usmState = SD().GetUSMAllocState(allocInfo.first, EXCEPTION_MESSAGE);
    buffer = std::vector<char>(usmState.size - allocInfo.second, 0);
    indirectionMap = usmState.indirectPointersOffsets;
    drvOcl.clEnqueueMemcpyINTEL(cmdQ, CL_BLOCKING, buffer.data(),
                                GetOffsetPointer(allocInfo.first, allocInfo.second), buffer.size(),
                                event_wait_list ? 1 : 0, event_wait_list, nullptr);
  } else if (type == KernelArgType::mem) {
    cl_mem handle = *reinterpret_cast<const cl_mem*>(argValue);
    auto& memState = SD().GetMemState(handle, EXCEPTION_MESSAGE);
    size_t size = memState.size;
    if (cfg.opencl.player.aubSignaturesCL && memState.buffer) {
      size += sizeof(mem_signature_t);
    }
    buffer = std::vector<char>(size, 0);
    if (memState.image) {
      size_t origin[3] = {0, 0, 0};
      size_t region[3];
      GetRegionForWholeImage(memState.image_desc, region);
      drvOcl.clEnqueueReadImage(cmdQ, handle, CL_BLOCKING, origin, region,
                                memState.image_desc.image_row_pitch,
                                memState.image_desc.image_slice_pitch, buffer.data(),
                                event_wait_list ? 1 : 0, event_wait_list, nullptr);
    } else {
      drvOcl.clEnqueueReadBuffer(cmdQ, handle, CL_BLOCKING, 0, buffer.size(), buffer.data(),
                                 event_wait_list ? 1 : 0, event_wait_list, nullptr);
    }
    if ((cfg.IsPlayer() ? cfg.opencl.player.captureImages : cfg.opencl.recorder.dumpImages) &&
        memState.image) {
      SaveImage(buffer.data(), memState.image_format, memState.image_desc, fileName);
    }
  }
  if (!buffer.empty()) {
    Log(TRACE) << "^------------------ injected read #" << ++injectedCounter;
    const bool nullIndirection = Config::IsPlayer()
                                     ? !cfg.opencl.player.disableNullIndirectPointersInBuffer
                                     : cfg.opencl.recorder.nullIndirectPointersInBuffer;
    if (nullIndirection && !indirectionMap.empty()) {
      auto allocInfo = GetSvmOrUsmFromRegion(hPointer);
      const auto offset = allocInfo.second;
      for (const auto& pair : indirectionMap) {
        if (pair.first >= offset && pair.second) {
          auto it = std::next(buffer.begin(), pair.first - offset);
          auto itEnd = std::next(buffer.begin(), pair.first - offset + sizeof(void*));
          std::fill(it, itEnd, '\0');
        }
      }
    }
    SaveBuffer(fileName, buffer);
  }
  return buffer;
}

void DeleteBuffer(std::vector<char>& buffer) {
  std::vector<char> temp(0, 0);
  buffer.swap(temp);
}

void AddSignature(std::vector<char>& buffer, mem_signature_t signature) {
  constexpr size_t size = sizeof(mem_signature_t);
  std::array<char, size> signatureArray;
  for (unsigned i = 0; i < size; ++i) {
    signatureArray[i] = (signature >> ((size - 1 - i) * 8)) & 0xff;
  }
  buffer.insert(buffer.end(), signatureArray.begin(), signatureArray.end());
}

mem_signature_t GenerateSignature() {
  static mem_signature_t sign = 0;
  return ++sign;
}

bool ResourceExists(cl_context resource) {
  return SD()._contextStates.find(resource) != SD()._contextStates.end();
}

std::string RemoveDoubleDotHeaderSyntax(const std::string& src) {
  auto shaderSource = src;
  std::string finalShaderSource = "";
  const std::regex expr(R"(#include\s*["<]([^">]+))");
  std::smatch what;
  while (std::regex_search(shaderSource, what, expr) && what.size() >= 2) {
    finalShaderSource += what.prefix().str();
    std::string header = what.str(1);
    if (header.find("..") != std::string::npos) {
      header = std::filesystem::path(header).filename().string();
      finalShaderSource += "#include \"" + header;
    } else {
      finalShaderSource += what.str(0);
    }
    shaderSource = what.suffix().str();
  }
  finalShaderSource += shaderSource;
  return finalShaderSource;
}

std::string ToStringHelper(const void* handle) {
  if (handle == nullptr) {
    // used by CCode, nullptr will not compile
    return "0";
  }
  return gits::hex(handle).ToString();
}
template <>
std::string ToStringHelper(const cl_image_format handle) {
  return gits::OpenCL::Ccl_image_format(handle).ToString();
}
template <>
std::string ToStringHelper(const cl_image_desc handle) {
  return gits::OpenCL::Ccl_image_desc(handle).ToString();
}
template <>
std::string ToStringHelper(const cl_buffer_region handle) {
  return gits::OpenCL::Ccl_buffer_region(handle).ToString();
}
template <>
std::string ToStringHelper(const cl_resource_barrier_descriptor_intel handle) {
  return gits::OpenCL::Ccl_resource_barrier_descriptor_intel(handle).ToString();
}

void UpdateUsmPtrs(cl_kernel kernel) {
  for (auto& state : SD()._usmAllocStates) {
    if (state.second->toUpdate[kernel] && state.second->type != UnifiedMemoryType::device) {
      state.second->toUpdate[kernel] = false;
    }
  }
  for (auto& state : SD()._svmAllocStates) {
    if (state.second->toUpdate[kernel]) {
      state.second->toUpdate[kernel] = false;
    }
  }
}

bool HasUsmPtrsToUpdate(cl_kernel kernel) {
  for (auto& state : SD()._usmAllocStates) {
    if (state.second->toUpdate[kernel]) {
      return true;
    }
  }
  for (auto& state : SD()._svmAllocStates) {
    if (state.second->toUpdate[kernel]) {
      return true;
    }
  }
  return false;
}

void ResetAllUsmUpdateState(const cl_kernel& kernel) {
  for (auto& state : SD()._usmAllocStates) {
    state.second->toUpdate[kernel] = false;
  }
  for (auto& state : SD()._svmAllocStates) {
    state.second->toUpdate[kernel] = false;
  }
}

void DetermineUsmToUpdate(const cl_kernel& kernel) {
  ResetAllUsmUpdateState(kernel);
  auto& kernelState = SD().GetKernelState(kernel, EXCEPTION_MESSAGE);
  if (!kernelState.indirectUsmPointers.empty()) {
    for (const auto& ptr : kernelState.indirectUsmPointers) {
      if (SD().CheckIfUSMAllocExists(ptr)) {
        SD().GetUSMAllocState(ptr, EXCEPTION_MESSAGE).toUpdate[kernel] = true;
      } else {
        SD().GetSVMAllocState(ptr, EXCEPTION_MESSAGE).toUpdate[kernel] = true;
      }
    }
  }
  if (kernelState.indirectUsmTypes) {
    for (auto& state : SD()._usmAllocStates) {
      state.second->toUpdate[kernel] =
          static_cast<unsigned>(state.second->type) & kernelState.indirectUsmTypes;
    }
    for (auto& state : SD()._svmAllocStates) {
      state.second->toUpdate[kernel] =
          (state.second->flags & CL_MEM_SVM_FINE_GRAIN_BUFFER &&
           kernelState.indirectUsmTypes & static_cast<unsigned>(UnifiedMemoryType::shared));
    }
  }
  for (const auto& arg : kernelState.GetArguments()) {
    if (arg.second.type == KernelArgType::usm) {
      void* usmPtr = GetUsmPtrFromRegion(const_cast<void*>(arg.second.argValue),
                                         arg.second.kernelSetType == KernelSetType::normal)
                         .first;
      SD().GetUSMAllocState(usmPtr, EXCEPTION_MESSAGE).toUpdate[kernel] = true;
    }
    if (arg.second.type == KernelArgType::svm) {
      void* svmPtr = GetSvmPtrFromRegion(const_cast<void*>(arg.second.argValue),
                                         arg.second.kernelSetType == KernelSetType::normal)
                         .first;
      SD().GetSVMAllocState(svmPtr, EXCEPTION_MESSAGE).toUpdate[kernel] = true;
    }
  }
}

std::pair<void*, uintptr_t> GetSvmPtrFromRegion(void* svmPtr, bool fromKernelArg) {
  if (!SD().CheckIfSVMAllocExists(svmPtr) && !fromKernelArg) {
    uintptr_t offset = 0UL;
    for (const auto& state : SD()._svmAllocStates) {
      const auto svmPtrBegin = reinterpret_cast<uintptr_t>(state.first);
      const auto svmRegion = reinterpret_cast<uintptr_t>(svmPtr);
      if (svmRegion < svmPtrBegin + state.second->size && svmRegion > svmPtrBegin) {
        offset = svmRegion - svmPtrBegin;
      }
    }
    void* validPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(svmPtr) - offset);
    return std::make_pair(validPtr, offset);
  }
  if (fromKernelArg) {
    auto* ptrValue = reinterpret_cast<void*>(*reinterpret_cast<const uintptr_t*>(svmPtr));
    return GetSvmPtrFromRegion(ptrValue, false);
  }
  return std::make_pair(svmPtr, 0U);
}

std::pair<void*, uintptr_t> GetUsmPtrFromRegion(void* usmPtr, bool fromKernelArg) {
  if (!SD().CheckIfUSMAllocExists(usmPtr) && !fromKernelArg) {
    uintptr_t offset = 0UL;
    for (const auto& state : SD()._usmAllocStates) {
      const auto usmPtrBegin = reinterpret_cast<uintptr_t>(state.first);
      const auto usmRegion = reinterpret_cast<uintptr_t>(usmPtr);
      if (usmRegion < usmPtrBegin + state.second->size && usmRegion > usmPtrBegin) {
        offset = usmRegion - usmPtrBegin;
      }
    }
    void* validPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(usmPtr) - offset);
    return std::make_pair(validPtr, offset);
  }
  if (fromKernelArg) {
    auto* ptrValue = reinterpret_cast<void*>(*reinterpret_cast<const uintptr_t*>(usmPtr));
    return GetUsmPtrFromRegion(ptrValue, false);
  }
  return std::make_pair(usmPtr, 0U);
}

std::pair<void*, uintptr_t> GetSvmOrUsmFromRegion(void* ptr) {
  auto ptrFromRegion = GetSvmPtrFromRegion(ptr);
  if (!SD().CheckIfSVMAllocExists(ptrFromRegion.first)) {
    ptrFromRegion = GetUsmPtrFromRegion(ptr);
    if (!SD().CheckIfUSMAllocExists(ptrFromRegion.first)) {
      return std::make_pair(nullptr, 0U);
    }
  }
  return ptrFromRegion;
}

bool CheckIntelPlatform(const cl_platform_id& platform) {
  constexpr char intelPlatformVendorName[] = "Intel(R) Corporation";
  constexpr size_t intelPlatformVendorNameSize =
      sizeof(intelPlatformVendorName) / sizeof(intelPlatformVendorName[0]);
  auto& platformState = SD().GetPlatformIDState(platform, EXCEPTION_MESSAGE);
  if (platformState.IsIntelPlatform()) {
    return true;
  }
  size_t platformNameSize = 0;
  auto result =
      drvOcl.clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 0, nullptr, &platformNameSize);
  if (!ErrCodeSuccess(result) || platformNameSize != intelPlatformVendorNameSize) {
    return false;
  }
  char platformName[intelPlatformVendorNameSize];
  result = drvOcl.clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, intelPlatformVendorNameSize,
                                    platformName, nullptr);
  const auto isIntelPlatform =
      ErrCodeSuccess(result) && strcmp(platformName, intelPlatformVendorName) == 0;
  if (isIntelPlatform) {
    platformState.SetIntelPlatform();
  }
  return isIntelPlatform;
}

bool IsReadOnlyBuffer(const cl_mem_flags& memFlags, const cl_mem_properties_intel* properties) {
  bool isReadOnly = (memFlags & CL_MEM_READ_ONLY) != 0U;
  if (properties != nullptr) {
    isReadOnly = isReadOnly || (GetPropertyVal(properties, CL_MEM_FLAGS) & CL_MEM_READ_ONLY) != 0U;
  }
  return isReadOnly;
}

bool IsReadOnlyBuffer(const cl_svm_mem_flags& memFlags) {
  return memFlags & CL_MEM_READ_ONLY;
}

bool CheckCfgZeroInitialization(const Config& cfg, const bool& isReadOnlyObj) {
  const auto zeroInit = cfg.IsPlayer() ? cfg.opencl.player.injectBufferResetAfterCreate
                                       : cfg.opencl.recorder.bufferResetAfterCreate;
  if (!zeroInit) {
    return false;
  }
  const auto omitReadOnly = cfg.IsPlayer() ? cfg.opencl.player.omitReadOnlyObjects
                                           : cfg.opencl.recorder.omitReadOnlyObjects;
  if (omitReadOnly && isReadOnlyObj) {
    return false;
  }
  const auto dumpBuffers = cfg.IsPlayer() ? !cfg.opencl.player.captureKernels.empty()
                                          : !cfg.opencl.recorder.dumpKernels.empty();
  return dumpBuffers;
}

bool ZeroInitializeBuffer(const cl_command_queue& commandQueue,
                          const cl_mem& memObj,
                          const size_t& size) {
  if (commandQueue == nullptr) {
    return false;
  }
  const auto zeroBuffer = std::vector<char>(size, 0);
  drvOcl.clEnqueueWriteBuffer(commandQueue, memObj, CL_BLOCKING, 0, zeroBuffer.size(),
                              zeroBuffer.data(), 0, nullptr, nullptr);
  Log(INFO) << "^------------------ injected zero-initialization write";
  return true;
}

bool ZeroInitializeUsm(const cl_command_queue& commandQueue,
                       void* usmPtr,
                       const size_t& size,
                       const UnifiedMemoryType& type) {
  const auto zeroBuffer = std::vector<char>(size, 0);
  if (type == UnifiedMemoryType::device) {
    if (commandQueue == nullptr) {
      return false;
    }
    drvOcl.clEnqueueMemcpyINTEL(commandQueue, CL_BLOCKING, usmPtr, zeroBuffer.data(), size, 0,
                                nullptr, nullptr);
    Log(INFO) << "^------------------ injected zero-initialization write";
  } else {
    std::memcpy(usmPtr, zeroBuffer.data(), size);
  }
  return true;
}

bool ZeroInitializeSvm(const cl_command_queue& commandQueue,
                       void* svmPtr,
                       const size_t& size,
                       const bool& isFineGrain) {
  const auto zeroBuffer = std::vector<char>(size, 0);
  if (!isFineGrain) {
    if (commandQueue == nullptr) {
      return false;
    }
    drvOcl.clEnqueueSVMMemcpy(commandQueue, CL_BLOCKING, svmPtr, zeroBuffer.data(), size, 0,
                              nullptr, nullptr);
    Log(INFO) << "^------------------ injected zero-initialization write";
  } else {
    std::memcpy(svmPtr, zeroBuffer.data(), size);
  }
  return true;
}

bool ZeroInitializeImage(const cl_command_queue& commandQueue,
                         const cl_mem& memObj,
                         const size_t& size,
                         const size_t& width,
                         const size_t& height,
                         const size_t& depth,
                         const size_t& input_row_pitch,
                         const size_t& input_slice_pitch) {
  if (commandQueue == nullptr) {
    return false;
  }
  const auto zeroBuffer = std::vector<char>(size, 0);
  const std::vector<size_t> origin = {0, 0, 0};
  const std::vector<size_t> region = {width, height, depth};
  drvOcl.clEnqueueWriteImage(commandQueue, memObj, CL_BLOCKING, origin.data(), region.data(),
                             input_row_pitch, input_slice_pitch, zeroBuffer.data(), 0, nullptr,
                             nullptr);
  Log(INFO) << "^------------------ injected zero-initialization write";
  return true;
}

cl_device_id GetGpuDevice() {
  for (const auto& state : SD()._platformIDStates) {
    const auto device = state.second->GetDeviceType(CL_DEVICE_TYPE_GPU);
    if (device != nullptr) {
      return device;
    }
  }
  Log(WARN) << "Could not find any GPU devices.";
  return nullptr;
}

cl_command_queue GetCommandQueue(const cl_context& context,
                                 const cl_device_id& device,
                                 cl_int* err) {
  auto& fakeCq = SD().GetContextState(context, EXCEPTION_MESSAGE).fakeQueue;
  if (fakeCq != nullptr) {
    return fakeCq;
  }
  const auto cqIt = SD()._commandQueueStates.begin();
  if (cqIt != SD()._commandQueueStates.end()) {
    return cqIt->first;
  }
  const auto cq = drvOcl.clCreateCommandQueue(context, device, 0, err);
  if (!ErrCodeSuccess(err)) {
    return nullptr;
  }
  fakeCq = cq;
  return cq;
}

std::string AppendBuildOption(const std::string& options, const std::string& optionToAppend) {
  if (options.find(optionToAppend) == std::string::npos) {
    return options + " " + optionToAppend;
  }
  return options;
}

std::string AppendKernelArgInfoOption(const std::string& options) {
  const std::string kernelArgInfoOption = "-cl-kernel-arg-info";
  if (Config::Get().opencl.player.omitReadOnlyObjects) {
    return AppendBuildOption(options, kernelArgInfoOption);
  }
  return options;
}

std::string AppendStreamPathToIncludePath(const std::string& options, const bool& hasHeaders) {
  const auto& cfg = Config::Get();
  const auto& streamPath =
      Config::IsPlayer() ? cfg.common.player.streamDir : cfg.common.recorder.dumpPath;
  const std::string includeStreamDir = "-I \"" + streamPath.string() + "\"";
  std::string new_options = AppendBuildOption(options, includeStreamDir);

  const std::string includeClProgramsDir = "-I \"" + (streamPath / "clPrograms").string() + "\"";
  new_options = AppendBuildOption(new_options, includeClProgramsDir);

  if (hasHeaders) {
    const std::string includeGitsFiles = "-I \"" + (streamPath / "gitsFiles").string() + "\"";
    new_options = AppendBuildOption(new_options, includeGitsFiles);
  }
  return new_options;
}

void* GetOffsetPointer(void* ptr, const uintptr_t& offset) {
  return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + offset);
}

uintptr_t GetPointerDifference(void* ptrRegion, void* ptrStart) {
  return reinterpret_cast<uintptr_t>(ptrRegion) - reinterpret_cast<uintptr_t>(ptrStart);
}

void Log_clGitsIndirectAllocationOffsets(void* pAlloc, uint32_t numOffsets, size_t* pOffsets) {
  Log(TRACE, NO_NEWLINE) << "clGitsIndirectAllocationOffsets(";
  Log(TRACE, RAW) << pAlloc << ", ";
  Log(TRACE, RAW) << numOffsets << ", ";
  if (numOffsets > 0) {
    Log(TRACE, RAW) << "{";
    for (uint32_t i = 0U; i < numOffsets; i++) {
      Log(TRACE, RAW) << pOffsets[i] << (i + 1U < numOffsets ? ", " : "}");
    }
  } else {
    Log(TRACE, RAW) << pOffsets;
  }
  Log(TRACE, NO_PREFIX) << ")";
}

std::vector<uint64_t> HashBinaryData(const size_t& n,
                                     const uint8_t** binaries,
                                     const size_t* lengths) {
  std::vector<uint64_t> hashVector(n);
  for (auto i = 0U; i < n; i++) {
    const auto hash = ComputeHash(binaries[i], lengths[i], THashType::XX);
    hashVector.at(i) = hash;
  }
  return hashVector;
}

cl_device_type GetDeviceType(const cl_device_id& device) {
  cl_device_type deviceType = CL_DEVICE_TYPE_ALL;
  drvOcl.clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, nullptr);
  return deviceType;
}

std::pair<void*, uint32_t> GetOriginalMappedPtrFromRegion(void* originalPtr) {
  if (CCLMappedPtr::CheckMapping(originalPtr)) {
    return std::make_pair(CCLMappedPtr::GetMapping(originalPtr), 0U);
  }
  for (const auto& state : SD()._usmAllocStates) {
    void* usmOriginal = CCLMappedPtr::GetOriginal(state.first);
    const auto ptrBegin = reinterpret_cast<uintptr_t>(usmOriginal);
    const auto usmRegion = reinterpret_cast<uintptr_t>(originalPtr);
    if (ptrBegin < usmRegion && usmRegion < ptrBegin + (uintptr_t)state.second->size) {
      const auto offset = usmRegion - ptrBegin;
      return std::make_pair(state.first, offset);
    }
  }
  for (const auto& state : SD()._svmAllocStates) {
    void* svmOriginal = CCLMappedPtr::GetOriginal(state.first);
    const auto ptrBegin = reinterpret_cast<uintptr_t>(svmOriginal);
    const auto svmRegion = reinterpret_cast<uintptr_t>(originalPtr);
    if (ptrBegin < svmRegion && svmRegion < ptrBegin + (uintptr_t)state.second->size) {
      const auto offset = svmRegion - ptrBegin;
      return std::make_pair(state.first, offset);
    }
  }
  return std::make_pair(nullptr, 0U);
}
} // namespace OpenCL
} // namespace gits
