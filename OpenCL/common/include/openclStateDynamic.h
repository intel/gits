// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclStateDynamic.h
*
* @brief Declaration of OpenCL library implementation.
*
*/
#pragma once

#include "openclHeader.h"

#include "openclTools.h"
#include "texture_converter.h"
#include "MemorySniffer.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gits {
class CArgument;
class CFunction;
namespace OpenCL {
struct CCLState {
public:
  CCLState() : _refCount(0), _restored(false) {}
  virtual ~CCLState() {}

  void Retain() {
    ++_refCount;
  }
  void Release() {
    --_refCount;
  }
  cl_uint GetRefCount() {
    return _refCount;
  }

  void RestoreFinished() {
    _restored = true;
  }
  bool Restored() {
    return _restored;
  }

protected:
  cl_uint _refCount = 0;
  bool _restored = false;
};
struct CCLPlatformIDState : public CCLState {
private:
  std::set<std::string> _extensionFunctions;
  std::unordered_map<cl_device_id, cl_device_type> _devices;
  bool _isIntelPlatform = false;

public:
  std::set<std::string> GetExtensionFunctions() const;
  void AddExtensionFunction(const char* name);
  bool ExtensionFunctionExists(const char* name) const;
  void AddDevice(const cl_device_id& device, const cl_device_type& deviceType);
  cl_device_id GetDeviceType(const cl_device_type& deviceType) const;
  void SetIntelPlatform();
  bool IsIntelPlatform() const;
};

struct CCLDeviceIDState : public CCLState {
  cl_platform_id platform = nullptr;
  cl_device_type type = 0;
  std::vector<cl_device_partition_property> properties;
  cl_device_id parentDevice = nullptr;
  CCLDeviceIDState();
  CCLDeviceIDState(cl_device_id parentDevice, const cl_device_partition_property* properties);
  CCLDeviceIDState(cl_platform_id platform, cl_device_type type);
};

struct CCLContextState : public CCLState {
  std::vector<cl_command_queue> commandQueueArray;
  std::vector<cl_context_properties> properties;
  std::vector<cl_device_id> devices;
  cl_platform_id platform = nullptr;
  cl_device_type type = 0;
  bool fromType = false;
  cl_command_queue fakeQueue = nullptr;
  CCLContextState(bool fromType,
                  const cl_context_properties* properties,
                  const cl_device_type type = CL_DEVICE_TYPE_DEFAULT);
};

struct CCLEventState : public CCLState {
  cl_context context = nullptr;
  bool isGLSharingEvent = false;
  bool isDXSharingEvent = false;
  bool isUserEvent = false;
  // if event is sharing and mapping is already scheduled.
  bool isSharingUserEvent = false;
  CCLEventState();
};

struct CCLCommandQueueState : public CCLState {
  cl_context context = nullptr;
  cl_device_id device = nullptr;
  bool commandQueue2_0 = false;
  // deprecated from 2.0 (Replaced by properties)
  // valid if commandQueue2_0 == false
  cl_command_queue_properties queueProperties1_2 = 0;
  // valid if commandQueue2_0 == true
  std::vector<cl_queue_properties> properties2_0;
  std::vector<std::shared_ptr<CCLEventState>> eventArray;
  CCLCommandQueueState(cl_context context,
                       cl_device_id device,
                       cl_command_queue_properties queue_properties);
  CCLCommandQueueState(cl_context context,
                       cl_device_id device,
                       const cl_queue_properties* properties);
};

class CCLProgramState;
struct CCLKernelState : public CCLState {
private:
  struct ArgInfo {
    size_t argSize = 0;
    const void* argValue = nullptr;
    KernelArgType type = KernelArgType::pointer;
    KernelSetType kernelSetType = KernelSetType::normal;
    ArgInfo() = default;
  };
  std::map<cl_uint, ArgInfo> args;
  std::vector<std::unique_ptr<uintptr_t>> pointers;

public:
  std::shared_ptr<CCLProgramState> programState;
  cl_program program = nullptr;
  std::string name;
  uint32_t indirectUsmTypes = 0;
  std::vector<void*> indirectUsmPointers;
  cl_kernel clonedKernel = nullptr;

  CCLKernelState();
  CCLKernelState(const cl_program program, const char* name);
  void SetArgument(cl_uint index, size_t size, const void* value);
  const ArgInfo& GetArgument(cl_uint index) const;
  const std::map<cl_uint, ArgInfo>& GetArguments() const;
  void ClearArguments();
  void SetArgType(const cl_uint& index, const KernelArgType& type);
  void SetArgSetType(const cl_uint& index, const KernelSetType& type);
};

class CCLProgramState : public CCLState {
public:
  bool isBinary = false;
  std::string fileName;
  bool isKhrApi = false;
  bool isKernelArgInfoAvailable = false;

  CCLProgramState();
  // clCreateProgramFromSource
  CCLProgramState(const cl_context& context,
                  const cl_uint& num,
                  const char** sources,
                  const size_t* lengths,
                  std::string filename);
  // clCreateProgramFromBinary
  CCLProgramState(const cl_context& context,
                  const cl_uint& num,
                  const cl_device_id* device_list,
                  const unsigned char** binaries,
                  const size_t* lengths);
  // clCreateProgramFromIL
  CCLProgramState(const cl_context& context, const void* il, const size_t& length);
  // clCreateProgramFromBuiltInKernels
  CCLProgramState(const cl_context& context,
                  const cl_uint& num,
                  const cl_device_id* device_list,
                  const char* kernelNames);
  // clLinkProgram
  CCLProgramState(const cl_context& context,
                  const cl_uint& num,
                  const cl_program* inputPrograms,
                  const char* options);

  void BuildProgram(const cl_uint& num_devices, const char* options);
  void CompileProgram(const cl_uint& num_devices,
                      const cl_uint& num,
                      const cl_program* inputHeaders,
                      const char** headerIncludeNames,
                      const char* options);

  void GetProgramInfoBinarySizes(const size_t& size, void* value);
  void GetProgramInfoNumDevices(const cl_uint& num_devices);
  void GetProgramInfoBinaries(const size_t& size, void* value);
  std::vector<uint64_t> GetBinaryHash() const;

  const cl_context Context() const;
  const std::string BuildOptions() const;
  const cl_device_id* DeviceList() const;
  const char** Sources();
  const unsigned char** Binaries();
  const size_t* SourceLengths() const;
  const cl_uint SourcesCount();
  const size_t* BinarySizes() const;
  const size_t BinariesCount() const;
  const cl_uint DevicesCount() const;
  const std::string KernelNames() const;
  const bool IsIL() const;
  const std::vector<std::string> HeaderIncludeNames() const;
  const std::vector<cl_program> InputProgramList() const;
  const std::vector<cl_program> InputHeaders() const;
  const bool HasHeaders() const;
  const std::map<cl_program, std::shared_ptr<CCLProgramState>> GetProgramStates() const;
  void ReallocBinaries();
  void AppendGlobalPointer(void* globalPtr);
  std::vector<void*> GetGlobalPointers() const;
  cl_program GetBinaryLinkedProgram() const;
  void SetBuildOptions(const std::string& options);
  void SetBinaryLinkedProgram(const cl_program& program);

  void Sources(const cl_uint num, const char** sources, const size_t* lengths);
  void Binaries(const cl_uint num, const unsigned char* const* binaries, const size_t* lengths);
  void IL(const void* il, const size_t& length);

  uint64_t ID() {
    return _uniqueStateID;
  }

private:
  typedef std::basic_string<char> string;
  typedef std::basic_string<unsigned char> ustring;

  void Update(const cl_uint& num, const size_t* lengths);
  void UpdateSourcePtrs();
  void UpdateBinaries(const cl_uint& num, const size_t* lengths);
  void UpdateBinariesPtrs();

  uint64_t _uniqueStateID = 0ULL;
  // clCompileProgram attaches binaries to program created from source
  cl_context _context = nullptr;
  cl_uint _num_devices = 0;
  bool _il = false;
  bool _hasHeaders = false;
  std::vector<cl_device_id> _deviceList;
  std::vector<const char*> _cStrArray;
  std::vector<const unsigned char*> _cBinArray;
  std::vector<string> _sources;
  std::vector<ustring> _binaries;
  std::vector<size_t> _binarySizes;
  std::vector<size_t> _sourceLengths;
  std::vector<cl_program> _inputPrograms;
  std::vector<cl_program> _inputHeaders;
  std::vector<string> _headerIncludeNames;
  std::string _kernelNames;
  std::string _buildOptions;
  std::map<cl_program, std::shared_ptr<CCLProgramState>> _stateProgs;
  std::vector<void*> _globalPtrs;
  cl_program _prebuiltProgram = nullptr;
  std::vector<uint64_t> _binaryHash;
};

struct CCLSamplerState : public CCLState {
  cl_context context = nullptr;
  cl_bool normalized_coords = false;
  cl_addressing_mode addressing_mode = 0;
  cl_filter_mode filter_mode = 0;
  bool crateSamplerWithProperties = false;
  std::vector<cl_sampler_properties> properties;

public:
  CCLSamplerState(cl_context context,
                  cl_bool normalized_coords,
                  cl_addressing_mode addressing_mode,
                  cl_filter_mode filter_mode);
  CCLSamplerState(cl_context context, const cl_sampler_properties* properties);
};

struct CCLMemState : public CCLState {
  cl_context context = nullptr;
  cl_mem_flags flags = 0;
  std::vector<cl_mem_properties_intel> intel_mem_properties;
  size_t size = 0;
  size_t origin = 0;
  cl_image_desc image_desc = {};
  cl_image_format image_format = {};
  cl_mem bufferObj = nullptr; // for subbuffer
  cl_buffer_create_type buffer_create_type = 0;
  size_t buffer_number = 0; // for async buffers
  bool buffer = false;
  bool image = false;
  // Pipe >
  bool pipe = false;
  cl_uint pipe_packet_size = 0U;
  cl_uint pipe_max_packets = 0U;
  std::vector<cl_pipe_properties> pipe_properties;
  // Pipe <
  //for ordering
  int index = 0;
  int& count() {
    static int cnt;
    return cnt;
  }
  std::shared_ptr<CCLMemState> memStateCreatedFrom;

public:
  CCLMemState();
  CCLMemState(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr = nullptr);
  CCLMemState(cl_context context,
              cl_mem_flags flags,
              cl_mem_properties_intel* props,
              size_t size,
              void* host_ptr = nullptr);
  CCLMemState(cl_context context,
              cl_mem_flags flags,
              size_t size,
              const cl_image_format* image_format,
              const cl_image_desc* image_desc,
              void* host_ptr = nullptr);
  CCLMemState(cl_context context,
              cl_mem_properties_intel* props,
              size_t size,
              const cl_image_format* image_format,
              const cl_image_desc* image_desc,
              void* host_ptr = nullptr);
  CCLMemState(cl_mem buffer,
              cl_mem_flags flags,
              cl_buffer_create_type buffer_create_type,
              const void* buffer_create_info);
  CCLMemState(cl_context context,
              cl_mem_flags flags,
              cl_uint pipe_packet_size,
              cl_uint pipe_max_packets,
              const cl_pipe_properties* properties);
  CCLMemState(const CCLMemState& other) = default;
  CCLMemState& operator=(const CCLMemState& other) = delete;
  ~CCLMemState();
};

struct CCLSVMAllocState : public CCLState {
  cl_context context = nullptr;
  cl_svm_mem_flags flags = 0;
  size_t size = 0;
  cl_uint alignment = 0;
  PagedMemoryRegionHandle sniffedRegionHandle = nullptr;
  std::unordered_map<cl_kernel, bool> toUpdate;
  std::map<size_t, bool> indirectPointersOffsets;

  CCLSVMAllocState(cl_context context, cl_svm_mem_flags flags, size_t size, cl_uint alignment);
  CCLSVMAllocState(const CCLSVMAllocState& other) = delete;
  CCLSVMAllocState& operator=(const CCLSVMAllocState& other) = delete;
  ~CCLSVMAllocState();
};

struct CCLUSMAllocState : public CCLState {
  cl_context context = nullptr;
  cl_device_id device = nullptr;
  size_t size = 0;
  cl_uint alignment = 0;
  std::vector<cl_mem_properties_intel> properties;
  UnifiedMemoryType type = UnifiedMemoryType::device;
  PagedMemoryRegionHandle sniffedRegionHandle = nullptr;
  std::unordered_map<cl_kernel, bool> toUpdate;
  // cl_intel_global_variable_pointers
  cl_program program = nullptr;
  const char* global_variable_name = nullptr;
  // clGitsIndirectAllocationOffsets
  std::map<size_t, bool> indirectPointersOffsets;

  CCLUSMAllocState(cl_context context,
                   cl_mem_properties_intel* properties,
                   size_t size,
                   cl_uint alignment,
                   UnifiedMemoryType type);
  CCLUSMAllocState(cl_context context,
                   cl_device_id device,
                   cl_mem_properties_intel* properties,
                   size_t size,
                   cl_uint alignment,
                   UnifiedMemoryType type);
  CCLUSMAllocState(cl_device_id device,
                   size_t size,
                   UnifiedMemoryType type,
                   cl_program program,
                   const char* global_variable_name);
  CCLUSMAllocState(const CCLUSMAllocState& other) = delete;
  CCLUSMAllocState& operator=(const CCLUSMAllocState& other) = delete;
  ~CCLUSMAllocState();
};

struct CCLMappedBufferState : public CCLState {
  size_t size = 0;
  cl_mem bufferObj = nullptr;
  std::vector<char> buffer;
  cl_command_queue commandQueue = nullptr;
  cl_map_flags mapFlags = 0;

  CCLMappedBufferState();
  CCLMappedBufferState(const size_t& size,
                       const cl_mem& buffer,
                       const cl_command_queue& commandQueue,
                       const cl_map_flags& mapFlags);
};

class LayoutBuilder {
private:
  boost::property_tree::ptree _layout;
  boost::property_tree::ptree _clKernels;
  std::string _latestFileName;
  int _enqueueCallNumber = 0;
  std::string BuildFileName(const int argNumber, const bool isBuffer = true);
  std::string GetExecutionKeyId();
  boost::property_tree::ptree GetImageDescription(const cl_image_format& imageFormat,
                                                  const cl_image_desc& imageDesc);
  std::string ModifyRecorderBuildOptions(const std::string& options, const bool& hasHeaders);

public:
  LayoutBuilder();
  void UpdateLayout(const CCLKernelState& ks, int enqNum, int argNum);
  std::string GetFileName() const;
  void SaveLayoutToJsonFile();
  template <typename T, typename K>
  void Add(const T& key, const K& value) {
    std::stringstream ss;
    ss << GetExecutionKeyId() << "." << key;
    _clKernels.add(ss.str(), value);
  }
  template <typename T, typename K>
  void AddChild(const T& key, const K& value) {
    std::stringstream ss;
    ss << GetExecutionKeyId() << "." << key;
    _clKernels.add_child(ss.str(), value);
  }
};

/************************************************************************/
/* CStateDynamic holds all contexts state data                          */
/************************************************************************/
class CStateDynamic {
private:
  CStateDynamic();
  CStateDynamic(const CStateDynamic& other) = delete;
  CStateDynamic& operator=(const CStateDynamic& other) = delete;

public:
  typedef std::map<cl_platform_id, std::shared_ptr<CCLPlatformIDState>> CCLPlatformIDStates;
  typedef std::unordered_map<cl_device_id, std::shared_ptr<CCLDeviceIDState>> CCLDeviceIDStates;
  typedef std::unordered_map<cl_context, std::shared_ptr<CCLContextState>> CCLContextStates;
  typedef std::unordered_map<cl_command_queue, std::shared_ptr<CCLCommandQueueState>>
      CCLCommandQueueStates;
  typedef std::unordered_map<cl_event, std::shared_ptr<CCLEventState>> CCLEventStates;
  typedef std::unordered_map<cl_kernel, std::shared_ptr<CCLKernelState>> CCLKernelStates;
  typedef std::unordered_map<cl_program, std::shared_ptr<CCLProgramState>> CCLProgramStates;
  typedef std::unordered_map<cl_sampler, std::shared_ptr<CCLSamplerState>> CCLSamplerStates;
  typedef std::unordered_map<cl_mem, std::shared_ptr<CCLMemState>> CCLMemStates;
  typedef std::unordered_map<void*, std::shared_ptr<CCLSVMAllocState>> CCLSVMAllocStates;
  typedef std::unordered_map<void*, std::vector<CCLMappedBufferState>> CCLMappedBufferStates;
  typedef std::unordered_map<void*, std::shared_ptr<CCLUSMAllocState>> CCLUSMAllocStates;

  template <class TStateObj, class TMap, class TObjType>
  TStateObj& GetMapStateObj(TObjType obj, TMap& map, CExceptionMessageInfo exception_message) {
    auto iter = map.find(obj);
    if (iter == map.end()) {
      throw std::runtime_error(exception_message);
    }
    return iter->second;
  }

  CCLPlatformIDStates _platformIDStates;
  CCLDeviceIDStates _deviceIDStates;
  CCLContextStates _contextStates;
  CCLCommandQueueStates _commandQueueStates;
  CCLEventStates _eventStates;
  CCLKernelStates _kernelStates;
  CCLProgramStates _programStates;
  CCLSamplerStates _samplerStates;
  CCLMemStates _memStates;
  CCLSVMAllocStates _svmAllocStates;
  CCLUSMAllocStates _usmAllocStates;
  CCLMappedBufferStates _mappedBufferStates;
  std::vector<std::vector<char>> _buffers;
  std::map<cl_command_queue, std::vector<std::vector<char>>> _enqueueBuffers;
  std::unordered_map<void*, size_t> _enqueueSvmMapSize;
  LayoutBuilder layoutBuilder;
  std::unordered_map<cl_platform_id, std::set<cl_device_id>> originalPlaybackPlatforms;

  CCLPlatformIDState& GetPlatformIDState(const cl_platform_id platformID,
                                         CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLPlatformIDState>, CCLPlatformIDStates,
                           cl_platform_id>(platformID, _platformIDStates, exception_message);
  }
  CCLDeviceIDState& GetDeviceIDState(const cl_device_id deviceID,
                                     CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLDeviceIDState>, CCLDeviceIDStates, cl_device_id>(
        deviceID, _deviceIDStates, exception_message);
  }
  CCLContextState& GetContextState(const cl_context context,
                                   CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLContextState>, CCLContextStates, cl_context>(
        context, _contextStates, exception_message);
  }
  CCLCommandQueueState& GetCommandQueueState(const cl_command_queue commandQueue,
                                             CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLCommandQueueState>, CCLCommandQueueStates,
                           cl_command_queue>(commandQueue, _commandQueueStates, exception_message);
  }
  CCLEventState& GetEventState(const cl_event event, CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLEventState>, CCLEventStates, cl_event>(
        event, _eventStates, exception_message);
  }
  CCLKernelState& GetKernelState(const cl_kernel kernel, CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLKernelState>, CCLKernelStates, cl_kernel>(
        kernel, _kernelStates, exception_message);
  }
  CCLProgramState& GetProgramState(const cl_program program,
                                   CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLProgramState>, CCLProgramStates, cl_program>(
        program, _programStates, exception_message);
  }
  CCLSamplerState& GetSamplerState(const cl_sampler sampler,
                                   CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLSamplerState>, CCLSamplerStates, cl_sampler>(
        sampler, _samplerStates, exception_message);
  }
  CCLMemState& GetMemState(const cl_mem mem, CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLMemState>, CCLMemStates, cl_mem>(mem, _memStates,
                                                                               exception_message);
  }
  CCLSVMAllocState& GetSVMAllocState(void* svmPtr, CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLSVMAllocState>, CCLSVMAllocStates, void*>(
        svmPtr, _svmAllocStates, exception_message);
  }
  CCLUSMAllocState& GetUSMAllocState(void* usmPtr, CExceptionMessageInfo exception_message) {
    return *GetMapStateObj<std::shared_ptr<CCLUSMAllocState>, CCLUSMAllocStates, void*>(
        usmPtr, _usmAllocStates, exception_message);
  }

  bool CheckIfSVMAllocExists(void* svmPtr) {
    return _svmAllocStates.find(svmPtr) != _svmAllocStates.end();
  }
  bool CheckIfUSMAllocExists(void* usmPtr) {
    return _usmAllocStates.find(usmPtr) != _usmAllocStates.end();
  }

  ~CStateDynamic();
  static CStateDynamic& Get();
};
inline CStateDynamic& SD() {
  return CStateDynamic::Get();
}
} // namespace OpenCL
} // namespace gits
