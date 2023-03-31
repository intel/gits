// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclStateRestore.cpp
*
* @brief Definition of OpenCL State Restore.
*
*/
#include "openclStateRestore.h"
#include "apis_iface.h"
#include "openclDrivers.h"
#include "openclFunctionsAuto.h"
#include "openclHeader.h"
#include "openclTools.h"
#include "recorder.h"
#include "openclHelperFunctions.h"

namespace gits {
namespace OpenCL {
namespace {
struct RestoreQueueInfo {
  cl_command_queue commandQueue = nullptr;
  bool fake = false;
  RestoreQueueInfo(cl_command_queue queue, bool artificial)
      : commandQueue(queue), fake(artificial){};
};
std::map<cl_context, RestoreQueueInfo> commandQueuesContext;
std::map<cl_device_id, RestoreQueueInfo> commandQueuesDevice;

cl_command_queue GetCommandQueue(CScheduler& scheduler,
                                 CStateDynamic& sd,
                                 cl_device_id device,
                                 cl_context context) {
  if (context) {
    if (commandQueuesContext.find(context) == commandQueuesContext.end()) {
      cl_device_id cqDevice = device;
      if (!cqDevice) {
        const auto& contextState = sd.GetContextState(context, EXCEPTION_MESSAGE);
        if (contextState.devices.size() > 1) {
          throw ENotImplemented(
              "Subcaptures for contexts with multiple devices are not implemented");
        }
        cqDevice = contextState.devices.at(0);
      }
      cl_int err = CL_SUCCESS;
      const auto cq = drvOcl.clCreateCommandQueue(context, cqDevice, 0, &err);
      if (err == CL_SUCCESS) {
        const auto queueInfo = RestoreQueueInfo(cq, true);
        commandQueuesContext.emplace(context, queueInfo);
        commandQueuesDevice.emplace(cqDevice, queueInfo);
        scheduler.Register(new CclCreateCommandQueue(cq, context, cqDevice, 0, &err));
      } else {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    }
    return commandQueuesContext.at(context).commandQueue;
  } else if (device) {
    if (commandQueuesDevice.find(device) == commandQueuesDevice.end()) {
      auto errcode_ret = CL_SUCCESS;
      const auto& randomContext = sd._contextStates.begin()->first;
      auto cq = drvOcl.clCreateCommandQueue(randomContext, device, 0, &errcode_ret);
      if (errcode_ret == CL_SUCCESS) {
        const auto queueInfo = RestoreQueueInfo(cq, true);
        commandQueuesDevice.emplace(device, queueInfo);
        commandQueuesContext.emplace(randomContext, queueInfo);
        scheduler.Register(new CclCreateCommandQueue(cq, randomContext, device, 0, &errcode_ret));
      } else {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    }
    return commandQueuesDevice.at(device).commandQueue;
  }
  return sd._commandQueueStates.begin()->first;
}

CFunction* CreateProgramToken(cl_program stateInstance, CCLProgramState program) {
  CFunction* token;
  if (program.SourcesCount() > 0) {
    token = new CclCreateProgramWithSource(stateInstance, program.Context(), program.SourcesCount(),
                                           program.Sources(), program.SourceLengths(), nullptr);
  } else if (program.BinariesCount() > 0) {
    if (!program.IsIL()) {
      token = new CclCreateProgramWithBinary(
          stateInstance, program.Context(), program.DevicesCount(), program.DeviceList(),
          program.BinarySizes(), program.Binaries(), nullptr, nullptr);
    } else {
      token = new CclCreateProgramWithIL(stateInstance, program.Context(), program.Binaries()[0],
                                         program.BinarySizes()[0], nullptr);
    }
  } else {
    token = new CclCreateProgramWithBuiltInKernels(stateInstance, program.Context(),
                                                   program.DevicesCount(), program.DeviceList(),
                                                   program.KernelNames().c_str(), nullptr);
  }
  return token;
}

void RestoreSubBuffer(gits::CScheduler& scheduler,
                      const cl_mem& memObj,
                      std::shared_ptr<CCLMemState>& state) {
  cl_buffer_region region = {state->origin, state->size};
  scheduler.Register(new CclCreateSubBuffer(memObj, state->bufferObj, state->flags,
                                            state->buffer_create_type, static_cast<void*>(&region),
                                            nullptr));
}

void RestoreBuffer(gits::CScheduler& scheduler,
                   const cl_mem& memObj,
                   std::shared_ptr<CCLMemState>& state,
                   const cl_command_queue& commandQueue) {
  std::vector<char> buffer(state->size);
  void* bufferPtr = nullptr;
  drvOcl.clEnqueueReadBuffer(commandQueue, memObj, CL_TRUE, 0, state->size, buffer.data(), 0,
                             nullptr, nullptr);
  if ((state->flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR)) != 0U) {
    bufferPtr = buffer.data();
  }
  if (state->intel_mem_properties.empty()) {
    scheduler.Register(
        new CclCreateBuffer(memObj, state->context, state->flags, state->size, bufferPtr, nullptr));
  } else {
    scheduler.Register(new CclCreateBufferWithPropertiesINTEL(
        memObj, state->context, &state->intel_mem_properties[0], state->flags, state->size,
        bufferPtr, nullptr));
  }
  if (bufferPtr == nullptr) {
    scheduler.Register(new CclEnqueueWriteBuffer(CL_SUCCESS, commandQueue, memObj, CL_TRUE, 0,
                                                 state->size, buffer.data(), 0, nullptr, nullptr));
  }
}

void RestoreImage(CScheduler& scheduler,
                  const cl_mem& memObj,
                  std::shared_ptr<CCLMemState>& state,
                  const cl_command_queue& commandQueue) {
  std::vector<char> buffer(state->size);
  void* bufferPtr = nullptr;
  if (state->image_desc.mem_object != nullptr && state->memStateCreatedFrom &&
      !state->memStateCreatedFrom->Restored()) {
    RestoreMemObject(scheduler, state->image_desc.mem_object, state->memStateCreatedFrom,
                     commandQueue);
  }
  size_t origin[3] = {0, 0, 0};
  size_t region[3];
  GetRegionForWholeImage(state->image_desc, region);
  drvOcl.clEnqueueReadImage(commandQueue, memObj, CL_TRUE, origin, region,
                            state->image_desc.image_row_pitch, state->image_desc.image_slice_pitch,
                            buffer.data(), 0, nullptr, nullptr);
  if ((state->flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR)) != 0U) {
    bufferPtr = buffer.data();
  }
  if (state->intel_mem_properties.empty()) {
    scheduler.Register(new CclCreateImage(memObj, state->context, state->flags,
                                          &state->image_format, &state->image_desc, bufferPtr,
                                          nullptr));
  } else {
    scheduler.Register(new CclCreateImageWithPropertiesINTEL(
        memObj, state->context, &state->intel_mem_properties[0], state->flags, &state->image_format,
        &state->image_desc, bufferPtr, nullptr));
  }
  if (bufferPtr == nullptr) {
    scheduler.Register(new CclEnqueueWriteImage(CL_SUCCESS, commandQueue, memObj, CL_TRUE, origin,
                                                region, state->image_desc.image_row_pitch,
                                                state->image_desc.image_slice_pitch, buffer.data(),
                                                0, nullptr, nullptr));
  }
}
} // namespace
} // namespace OpenCL
} // namespace gits

void gits::OpenCL::RestorePlatforms(CScheduler& scheduler, CStateDynamic& sd) {
  std::vector<cl_platform_id> platforms;
  for (auto& state : sd._platformIDStates) {
    platforms.push_back(state.first);
    state.second->RestoreFinished();
  }
  scheduler.Register(
      new CclGetPlatformIDs(CL_SUCCESS, platforms.size(), platforms.data(), nullptr));
}

void gits::OpenCL::RestoreDevices(CScheduler& scheduler, CStateDynamic& sd) {
  std::unordered_map<cl_device_id, std::vector<cl_device_id>> devicesMap;
  for (auto& state : sd._deviceIDStates) {
    cl_device_id stateInstance = state.first;
    if (state.second->properties.empty()) {
      scheduler.Register(new CclGetDeviceIDs(CL_SUCCESS, state.second->platform, state.second->type,
                                             1, &stateInstance, nullptr));
      state.second->RestoreFinished();
    } else {
      devicesMap[state.second->parentDevice].push_back(state.first);
    }
  }

  for (auto& device : devicesMap) {
    auto& deviceState = sd._deviceIDStates[*device.second.begin()];
    scheduler.Register(new CclCreateSubDevices(CL_SUCCESS, device.first,
                                               deviceState->properties.data(), device.second.size(),
                                               device.second.data(), nullptr));
  }
}

void gits::OpenCL::RestoreContexts(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd._contextStates) {
    cl_context stateInstance = state.first;
    cl_int err = CL_SUCCESS;
    if (state.second->fromType) {
      scheduler.Register(new CclCreateContextFromType(stateInstance,
                                                      state.second->properties.data(),
                                                      state.second->type, nullptr, nullptr, &err));
      if (state.second->devices.size() > 0) {
        scheduler.Register(
            new CclGetContextInfo(CL_SUCCESS, stateInstance, CL_CONTEXT_DEVICES,
                                  state.second->devices.size() * sizeof(cl_device_id),
                                  state.second->devices.data(), nullptr));
      }
    } else {
      if (state.second->devices.size() > 0) {
        scheduler.Register(new CclCreateContext(
            stateInstance, state.second->properties.data(), (cl_uint)state.second->devices.size(),
            state.second->devices.data(), nullptr, nullptr, &err));
      }
    }
    cl_uint refCount = state.second->GetRefCount();
    for (cl_uint i = 1; i < refCount; i++) {
      scheduler.Register(new CclRetainContext(CL_SUCCESS, stateInstance));
    }
    state.second->RestoreFinished();
  }
}

void gits::OpenCL::RestoreCommandQueues(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd._commandQueueStates) {
    if (!ResourceExists(state.second->context)) {
      continue;
    }
    cl_command_queue stateInstance = state.first;
    // to make sure all writes are done, before RestoreMemObjects
    OpenCL::drvOcl.clFinish(stateInstance);
    if (commandQueuesDevice.find(state.second->device) == commandQueuesDevice.end()) {
      const auto queueInfo = RestoreQueueInfo(stateInstance, false);
      commandQueuesDevice.emplace(state.second->device, queueInfo);
    }
    if (commandQueuesContext.find(state.second->context) == commandQueuesContext.end()) {
      const auto queueInfo = RestoreQueueInfo(stateInstance, false);
      commandQueuesContext.emplace(state.second->context, queueInfo);
    }
    if (state.second->commandQueue2_0) {
      scheduler.Register(new CclCreateCommandQueueWithProperties(
          stateInstance, state.second->context, state.second->device,
          state.second->properties2_0.data(), nullptr));
    } else {
      scheduler.Register(new CclCreateCommandQueue(stateInstance, state.second->context,
                                                   state.second->device,
                                                   state.second->queueProperties1_2, nullptr));
    }
    cl_uint refCount = state.second->GetRefCount();
    for (cl_uint i = 1; i < refCount; i++) {
      scheduler.Register(new CclRetainCommandQueue(CL_SUCCESS, stateInstance));
    }
    state.second->RestoreFinished();
  }
}

void gits::OpenCL::RestoreMemObject(CScheduler& scheduler,
                                    const cl_mem& memObj,
                                    std::shared_ptr<CCLMemState>& state,
                                    const cl_command_queue& commandQueue) {
  const auto contextExists = ResourceExists(state->context);
  if (state->context == nullptr && !state->buffer && !state->image) {
    RestoreSubBuffer(scheduler, memObj, state);
  } else if (contextExists && state->buffer) {
    RestoreBuffer(scheduler, memObj, state, commandQueue);
  } else if (contextExists && state->image) {
    RestoreImage(scheduler, memObj, state, commandQueue);
  }
  // Adjust refcount
  cl_uint refCount = state->GetRefCount();
  for (cl_uint i = 1; i < refCount; i++) {
    scheduler.Register(new CclRetainMemObject(CL_SUCCESS, memObj));
  }
  state->RestoreFinished();
}

void gits::OpenCL::RestoreMemObjects(CScheduler& scheduler, CStateDynamic& sd) {
  std::vector<std::pair<cl_mem, std::shared_ptr<CCLMemState>>> memStates(sd._memStates.begin(),
                                                                         sd._memStates.end());
  struct {
    bool operator()(const std::pair<cl_mem, std::shared_ptr<CCLMemState>>& lhs,
                    const std::pair<cl_mem, std::shared_ptr<CCLMemState>>& rhs) {
      return lhs.second->index < rhs.second->index;
    }
  } customCompare;
  std::sort(memStates.begin(), memStates.end(), customCompare);
  for (auto& state : memStates) {
    if (state.second->pipe) {
      throw gits::ENotImplemented("Subcaptures with pipe objects are not implemented");
    }
    // Create command queue to sync data
    const auto& commandQueue = GetCommandQueue(scheduler, sd, nullptr, state.second->context);
    RestoreMemObject(scheduler, state.first, state.second, commandQueue);
  }
}

void gits::OpenCL::RestoreSamplers(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd._samplerStates) {
    if (!ResourceExists(state.second->context)) {
      continue;
    }
    cl_sampler stateInstance = state.first;
    scheduler.Register(
        new CclCreateSampler(stateInstance, state.second->context, state.second->normalized_coords,
                             state.second->addressing_mode, state.second->filter_mode, nullptr));
    cl_uint refCount = state.second->GetRefCount();
    for (cl_uint i = 1; i < refCount; i++) {
      scheduler.Register(new CclRetainSampler(CL_SUCCESS, stateInstance));
    }
    state.second->RestoreFinished();
  }
}

void gits::OpenCL::RestoreUsm(CScheduler& scheduler, CStateDynamic& sd) {
  cl_int errcode_ret = 0;
  for (const auto& state : sd._usmAllocStates) {
    void* stateInstance = state.first;
    switch (state.second->type) {
    case UnifiedMemoryType::device: {
      if (!state.second->context) {
        scheduler.Register(new CclGetDeviceGlobalVariablePointerINTEL(
            CL_SUCCESS, state.second->device, state.second->program,
            state.second->global_variable_name, &state.second->size, &stateInstance));
      } else {
        scheduler.Register(new CclDeviceMemAllocINTEL(
            stateInstance, state.second->context, state.second->device,
            state.second->properties.empty() ? nullptr : state.second->properties.data(),
            state.second->size, state.second->alignment, &errcode_ret));
      }
      cl_command_queue cq =
          GetCommandQueue(scheduler, sd, state.second->device, state.second->context);
      std::vector<char> buffer(state.second->size);
      errcode_ret = OpenCL::drvOcl.clEnqueueMemcpyINTEL(
          cq, CL_BLOCKING, buffer.data(), stateInstance, state.second->size, 0, nullptr, nullptr);
      if (errcode_ret != CL_SUCCESS) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      scheduler.Register(new CclEnqueueMemcpyINTEL(CL_SUCCESS, cq, CL_BLOCKING, stateInstance,
                                                   buffer.data(), state.second->size, 0, nullptr,
                                                   nullptr));
      break;
    }
    case UnifiedMemoryType::host: {
      scheduler.Register(new CclHostMemAllocINTEL(
          stateInstance, state.second->context,
          state.second->properties.empty() ? nullptr : state.second->properties.data(),
          state.second->size, state.second->alignment, &errcode_ret));
      scheduler.Register(new CGitsClMemoryRestore(stateInstance));
      break;
    }
    case UnifiedMemoryType::shared: {
      scheduler.Register(new CclSharedMemAllocINTEL(
          stateInstance, state.second->context, state.second->device,
          state.second->properties.empty() ? nullptr : state.second->properties.data(),
          state.second->size, state.second->alignment, &errcode_ret));
      scheduler.Register(new CGitsClMemoryRestore(stateInstance));
      break;
    }
    }
    state.second->RestoreFinished();
  }
}

void gits::OpenCL::RestoreSvm(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd._svmAllocStates) {
    void* stateInstance = state.first;
    scheduler.Register(new CclSVMAlloc(stateInstance, state.second->context, state.second->flags,
                                       state.second->size, state.second->alignment));
    if (state.second->flags & CL_MEM_SVM_FINE_GRAIN_BUFFER) {
      scheduler.Register(new CGitsClMemoryRestore(stateInstance));
    } else {
      cl_command_queue cq = GetCommandQueue(scheduler, sd, nullptr, state.second->context);
      std::vector<char> buffer(state.second->size);
      cl_int err = OpenCL::drvOcl.clEnqueueSVMMemcpy(cq, CL_BLOCKING, buffer.data(), stateInstance,
                                                     state.second->size, 0, nullptr, nullptr);
      if (err != CL_SUCCESS) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      scheduler.Register(new CclEnqueueSVMMemcpy_V1(CL_SUCCESS, cq, CL_BLOCKING, stateInstance,
                                                    buffer.data(), state.second->size, 0, nullptr,
                                                    nullptr));
    }
    state.second->RestoreFinished();
  }
}

void gits::OpenCL::RestoreMappedPointers(CScheduler& scheduler, CStateDynamic& sd) {
  if (gits::CGits::Instance().apis.IfaceCompute().CfgRec_IsKernelsRangeMode()) {
    for (auto& state : sd._mappedBufferStates) {
      for (auto& mappedBuffer : state.second) {
        const auto& memState = sd.GetMemState(mappedBuffer.bufferObj, EXCEPTION_MESSAGE);
        cl_int errCode = CL_SUCCESS;
        if (memState.image) {
          size_t region[3] = {0, 0, 0};
          size_t origin[3] = {0, 0, 0};
          auto imageRowPitch = memState.image_desc.image_row_pitch;
          auto imageSlicePitch = memState.image_desc.image_slice_pitch;
          GetRegionForWholeImage(memState.image_desc, region);
          scheduler.Register(new CclEnqueueMapImage(
              state.first, mappedBuffer.commandQueue, mappedBuffer.bufferObj, CL_BLOCKING,
              mappedBuffer.mapFlags, origin, region, &imageRowPitch, &imageSlicePitch, 0, nullptr,
              nullptr, &errCode));
        } else {
          scheduler.Register(new CclEnqueueMapBuffer(
              state.first, mappedBuffer.commandQueue, mappedBuffer.bufferObj, CL_BLOCKING,
              mappedBuffer.mapFlags, 0, memState.size, 0, nullptr, nullptr, &errCode));
        }
        mappedBuffer.RestoreFinished();
      }
    }
  }
}

void gits::OpenCL::RestorePrograms(CScheduler& scheduler, CStateDynamic& sd) {
  std::unordered_map<cl_program, bool> builtPrograms;
  std::set<cl_program> compileList;
  std::unordered_map<cl_program, CCLProgramState> deadPrograms;
  for (const auto& state : sd._programStates) {
    cl_program stateInstance = state.first;
    auto& program = state.second;
    if (!ResourceExists(program->Context())) {
      continue;
    }
    builtPrograms[stateInstance] = false;
    if (!program->InputProgramList().empty()) {
      for (const auto& prog : program->InputProgramList()) {
        compileList.insert(prog);
        if (sd._programStates.find(prog) == sd._programStates.end()) {
          deadPrograms[prog] = *program->GetProgramStates().at(prog);
        }
      }
      continue;
    }
    scheduler.Register(CreateProgramToken(stateInstance, *program));
  }
  for (auto& state : deadPrograms) {
    scheduler.Register(
        CreateProgramToken(reinterpret_cast<cl_program>(state.second.ID()), state.second));
  }
  for (const auto& prog : compileList) {
    auto progExists = deadPrograms.find(prog) == deadPrograms.end();
    auto stateProg = progExists ? sd.GetProgramState(prog, EXCEPTION_MESSAGE) : deadPrograms[prog];
    auto program = progExists ? prog : reinterpret_cast<cl_program>(deadPrograms[prog].ID());
    scheduler.Register(new CclCompileProgram(
        CL_SUCCESS, program, 0, nullptr, stateProg.BuildOptions().c_str(),
        stateProg.HasHeaders() ? stateProg.InputHeaders().size() : 0,
        stateProg.HasHeaders() ? stateProg.InputHeaders().data() : nullptr,
        stateProg.HasHeaders() ? (const char**)stateProg.HeaderIncludeNames().data() : nullptr,
        nullptr, nullptr));
    builtPrograms[prog] = true;
  }
  for (const auto& state : sd._programStates) {
    if (!ResourceExists(state.second->Context())) {
      continue;
    }
    if (!state.second->InputProgramList().empty()) {
      auto program = deadPrograms.find(state.first) != deadPrograms.end()
                         ? reinterpret_cast<cl_program>(deadPrograms[state.first].ID())
                         : state.first;
      auto progList = state.second->InputProgramList();
      for (auto& prog : progList) {
        if (deadPrograms.find(prog) != deadPrograms.end()) {
          prog = reinterpret_cast<cl_program>(deadPrograms[prog].ID());
        }
      }
      scheduler.Register(new CclLinkProgram(program, state.second->Context(), 0, nullptr,
                                            state.second->BuildOptions().c_str(), progList.size(),
                                            progList.data(), nullptr, nullptr, nullptr));
      builtPrograms[state.first] = true;
    }
    if (!builtPrograms[state.first]) {
      scheduler.Register(new CclBuildProgram(CL_SUCCESS, state.first, 0, nullptr,
                                             state.second->BuildOptions().c_str(), nullptr,
                                             nullptr));
      builtPrograms[state.first] = true;
    }
    const auto& refCount = state.second->GetRefCount();
    for (auto i = 1u; i < refCount; i++) {
      scheduler.Register(new CclRetainProgram(CL_SUCCESS, state.first));
    }
    state.second->RestoreFinished();
  }
  for (auto& state : deadPrograms) {
    scheduler.Register(
        new CclReleaseProgram(CL_SUCCESS, reinterpret_cast<cl_program>(state.second.ID())));
  }
}

void gits::OpenCL::RestoreKernels(CScheduler& scheduler, CStateDynamic& sd) {
  std::vector<uint64_t> restoredFakePrograms;
  for (auto& state : sd._kernelStates) {
    cl_kernel stateInstance = state.first;
    auto& kernel = state.second;
    if (!ResourceExists(kernel->programState->Context())) {
      continue;
    }
    cl_program programInstance = kernel->program;
    // see if we need to create a fake program for this kernel
    // if program was released immediately after kernel was created it will not
    // be present in state tracking
    auto programStateIter = sd._programStates.find(programInstance);
    bool restoreProgram = ((programStateIter == sd._programStates.end()) ||
                           (programStateIter->second->ID() != kernel->programState->ID()));
    if (restoreProgram) {
      auto program = kernel->programState;
      programInstance = reinterpret_cast<cl_program>(program->ID()); // fake
      // see if fake program is restored already
      // this avoids creating multiple source/binary files with the same
      // contents
      auto fakeProgramIter =
          std::find(restoredFakePrograms.begin(), restoredFakePrograms.end(), program->ID());
      if (fakeProgramIter == restoredFakePrograms.end()) {
        //program is added to SD for CclBuildProgram call
        sd._programStates[programInstance] = program;
        scheduler.Register(CreateProgramToken(programInstance, *program));
        scheduler.Register(new CclBuildProgram(CL_SUCCESS, programInstance, 0, nullptr,
                                               program->BuildOptions().c_str(), nullptr, nullptr));
        restoredFakePrograms.push_back(program->ID());
      }
    }
    cl_int err = CL_SUCCESS;
    if (state.second->clonedKernel != nullptr) {
      scheduler.Register(new CclCloneKernel(stateInstance, state.second->clonedKernel, &err));
    } else {
      scheduler.Register(
          new CclCreateKernel(stateInstance, programInstance, kernel->name.c_str(), &err));
    }
    cl_uint refCount = state.second->GetRefCount();
    for (cl_uint i = 1; i < refCount; i++) {
      scheduler.Register(new CclRetainKernel(CL_SUCCESS, stateInstance));
    }
    if (state.second->indirectUsmTypes != 0) {
      cl_bool param_value = CL_TRUE;
      if (state.second->indirectUsmTypes & static_cast<unsigned>(UnifiedMemoryType::shared)) {
        scheduler.Register(new CclSetKernelExecInfo_V1(
            CL_SUCCESS, stateInstance, CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL,
            sizeof(param_value), &param_value));
      }
      if (state.second->indirectUsmTypes & static_cast<unsigned>(UnifiedMemoryType::host)) {
        scheduler.Register(new CclSetKernelExecInfo_V1(
            CL_SUCCESS, stateInstance, CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL,
            sizeof(param_value), &param_value));
      }
      if (state.second->indirectUsmTypes & static_cast<unsigned>(UnifiedMemoryType::device)) {
        scheduler.Register(new CclSetKernelExecInfo_V1(
            CL_SUCCESS, stateInstance, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL,
            sizeof(param_value), &param_value));
      }
    }
    if (!state.second->indirectUsmPointers.empty()) {
      scheduler.Register(new CclSetKernelExecInfo_V1(
          CL_SUCCESS, stateInstance, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL,
          state.second->indirectUsmPointers.size(), state.second->indirectUsmPointers.data()));
    }
    for (const auto& arg : kernel->GetArguments()) {
      if (arg.second.type == KernelArgType::mem) {
        if (sd._memStates.find(*reinterpret_cast<const cl_mem*>(arg.second.argValue)) ==
            sd._memStates.end()) {
          continue;
        }
      } else if (arg.second.type == KernelArgType::sampler) {
        if (sd._samplerStates.find(*reinterpret_cast<const cl_sampler*>(arg.second.argValue)) ==
            sd._samplerStates.end()) {
          continue;
        }
      } else if (arg.second.kernelSetType == KernelSetType::usm) {
        scheduler.Register(new CclSetKernelArgMemPointerINTEL(CL_SUCCESS, stateInstance, arg.first,
                                                              arg.second.argValue));
        continue;
      } else if (arg.second.kernelSetType == KernelSetType::svm) {
        scheduler.Register(new CclSetKernelArgSVMPointer(CL_SUCCESS, stateInstance, arg.first,
                                                         arg.second.argValue));
        continue;
      }
      scheduler.Register(new CclSetKernelArg_V1(CL_SUCCESS, stateInstance, arg.first,
                                                arg.second.argSize, arg.second.argValue));
    }
    kernel->ClearArguments();
    state.second->RestoreFinished();
  }

  // release fake programs
  for (auto fakeProgram : restoredFakePrograms) {
    cl_program programInstance = reinterpret_cast<cl_program>(fakeProgram); // fake
    scheduler.Register(new CclReleaseProgram(CL_SUCCESS, programInstance));
  }
}

void gits::OpenCL::RestoreEvents(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd._eventStates) {
    if (!ResourceExists(state.second->context)) {
      continue;
    }
    cl_event stateInstance = state.first;
    scheduler.Register(new CclCreateUserEvent(stateInstance, state.second->context, nullptr));
    scheduler.Register(new CclSetUserEventStatus(CL_SUCCESS, stateInstance, CL_COMPLETE));
    cl_uint refCount = state.second->GetRefCount();
    for (cl_uint i = 1; i < refCount; i++) {
      scheduler.Register(new CclRetainEvent(CL_SUCCESS, stateInstance));
    }
    state.second->RestoreFinished();
  }
}

void gits::OpenCL::CState::Finish(CScheduler& scheduler) const {
  auto& sd = SD();
  const auto& isKernelsRangeMode =
      gits::CGits::Instance().apis.IfaceCompute().CfgRec_IsKernelsRangeMode();
  if (isKernelsRangeMode) {
    for (auto& state : sd._mappedBufferStates) {
      for (auto& mappedBuffer : state.second) {
        if (mappedBuffer.Restored()) {
          scheduler.Register(new CclEnqueueUnmapMemObject(CL_SUCCESS, mappedBuffer.commandQueue,
                                                          mappedBuffer.bufferObj, state.first, 0,
                                                          nullptr, nullptr));
        }
      }
    }
  }
  for (auto& state : sd._commandQueueStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      scheduler.Register(new CclFinish(CL_SUCCESS, state.first));
    }
  }
  for (auto& state : sd._eventStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseEvent(CL_SUCCESS, state.first));
      }
    }
  }
  for (auto& state : sd._samplerStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseSampler(CL_SUCCESS, state.first));
      }
    }
  }
  std::unordered_map<cl_program, uint32_t> referenceMap;
  for (auto& state : sd._kernelStates) {
    if (referenceMap.find(state.second->program) == referenceMap.end()) {
      referenceMap[state.second->program] = 0;
    }
    referenceMap[state.second->program]++;
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseKernel(CL_SUCCESS, state.first));
      }
    }
  }
  for (auto& state : sd._programStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      if (referenceMap.find(state.first) == referenceMap.end()) {
        referenceMap[state.first] = 0;
      }
      cl_uint refCount = state.second->GetRefCount() - referenceMap[state.first];
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseProgram(CL_SUCCESS, state.first));
      }
    }
  }
  for (auto& state : sd._memStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseMemObject(CL_SUCCESS, state.first));
      }
    }
  }
  for (auto& state : sd._usmAllocStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        if (state.second->context) {
          scheduler.Register(new CclMemFreeINTEL(CL_SUCCESS, state.second->context, state.first));
        }
      }
    }
  }
  for (auto& state : sd._svmAllocStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclSVMFree(state.second->context, state.first));
      }
    }
  }
  for (auto& state : sd._commandQueueStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseCommandQueue(CL_SUCCESS, state.first));
      }
    }
    commandQueuesContext.erase(state.second->context);
    commandQueuesDevice.erase(state.second->device);
  }
  for (auto& state : commandQueuesContext) {
    if (state.second.fake) {
      scheduler.Register(new CclReleaseCommandQueue(CL_SUCCESS, state.second.commandQueue));
    }
  }
  for (auto& state : commandQueuesDevice) {
    const auto it = std::find_if(commandQueuesContext.begin(), commandQueuesContext.end(),
                                 [&state](std::pair<cl_context, RestoreQueueInfo> s) {
                                   return s.second.commandQueue == state.second.commandQueue;
                                 });
    if (it == commandQueuesContext.end() && state.second.fake) {
      scheduler.Register(new CclReleaseCommandQueue(CL_SUCCESS, state.second.commandQueue));
    }
  }
  for (auto& state : sd._contextStates) {
    if (state.second->Restored() || isKernelsRangeMode) {
      cl_uint refCount = state.second->GetRefCount();
      for (cl_uint i = 0; i < refCount; i++) {
        scheduler.Register(new CclReleaseContext(CL_SUCCESS, state.first));
      }
    }
  }
}
