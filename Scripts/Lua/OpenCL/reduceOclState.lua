-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

print("******************************************************************************************")
print(" ")
print("reduce_ocl_state.lua")
print("This script is to reduce OCL state to buffers, kernels and programs that are in use by subsequent enqueuendrange API calls.")
print("By default it works in \'filter\' mode where unused API calls are getting removed based on files dumped by the script in \'dump\' mode.")
print("Run player with \'--scriptArgsStr dump\' option to force \'dump\' mode, where csv files with objects in use are being saved.")
print(" ")
print("NOTES")
print("Works with 64 bit player only!!")
print("Script has not been fully tested")
print("Before using stream modified this way, at least verify it's output with \'--logLevel TRACE\' option for any INVALID, ERROR, FAIL messages.")
print(" ")
print("******************************************************************************************")

-- Structs
Usm = {
  address = 0,
  size = 0,
  type = nil
}

-- Constants
-- CL_KERNEL_EXEC_INFO:
CL_KERNEL_EXEC_INFO_SVM_PTRS                                 = 0x11b6
CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM                    = 0x11b7
CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL               = 0x4200
CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL             = 0x4201
CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL             = 0x4202
CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL                           = 0x4203

-- Possible usm allocations
USM_HOST_ALLOC                                               = 1
USM_DEVICE_ALLOC                                             = 2
USM_SHARED_ALLOC                                             = 3
USM_SVM_ALLOC                                                = 4

-- Global variables
mem_to_num_map = {}
kern_to_num_map = {}
prg_to_num_map = {}
usm_to_num_map = {}

mem_nums_req = {}
kern_nums_req = {}
prg_nums_req = {}
usm_nums_req = {}

mem_dep = {}
usm_dep = {}
usm_mem_dep = {}
mem_usm_dep = {}

usm_structs = {}

dump_file_name = "reduce_ocl_state_dump.csv"

kern_to_attribs = {}
KernAttribs = { program=0, mems={}, usms={} }

mem_ctr = 0
kern_ctr = 0
prg_ctr = 0
usm_ctr = 0

skipped_funcs_ctr = 0

dump_mode = false --dump_mode/filter_mode switch

-- Struct operations
function Usm:new(address, size, type)
  local usm = {}
  setmetatable(usm, {__index = self})
  usm.address = address
  usm.size = size
  usm.type = type
  return usm
end

-- Intercepted special implementation functions

function clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
  mem_ctr = mem_ctr + 1
  memory = gits.nullUdt()
  if dump_mode then
    memory = drvCl.clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    mem_to_num_map[gits.udtToInt(memory)] = mem_ctr
    if usm_to_num_map[gits.udtToInt(host_ptr)] ~= nil then
      mem_usm_dep[gits.udtToInt(memory)] = gits.udtToInt(host_ptr)
    end
  elseif mem_nums_req[mem_ctr] ~= nil then
    memory = drvCl.clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return memory
end

function clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
  mem_ctr = mem_ctr + 1
  memory = gits.nullUdt()
  if dump_mode then
    memory = drvCl.clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
    mem_to_num_map[gits.udtToInt(memory)] = mem_ctr
    if usm_to_num_map[gits.udtToInt(host_ptr)] ~= nil then
      mem_usm_dep[gits.udtToInt(memory)] = gits.udtToInt(host_ptr)
    end
  elseif mem_nums_req[mem_ctr] ~= nil then
    memory = drvCl.clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return memory
end

function clCreatePipe(context, flags, pipe_packet_size, pipe_max_packets, properties, errcode_ret)
  mem_ctr = mem_ctr + 1
  memory = gits.nullUdt()
  if dump_mode then
    memory = drvCl.clCreatePipe(context, flags, pipe_packet_size, pipe_max_packets, properties, errcode_ret)
    mem_to_num_map[gits.udtToInt(memory)] = mem_ctr
  elseif mem_nums_req[mem_ctr] ~= nil then
    memory = drvCl.clCreatePipe(context, flags, pipe_packet_size, pipe_max_packets, properties, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return memory
end

function clCreateBufferWithPropertiesINTEL(context, properties, flags, size, host_ptr, err)
  mem_ctr = mem_ctr + 1
  memory = gits.nullUdt()
  if dump_mode then
    memory = drvCl.clCreateBufferWithPropertiesINTEL(context, properties, flags, size, host_ptr, err)
    mem_to_num_map[gits.udtToInt(memory)] = mem_ctr
    if usm_to_num_map[gits.udtToInt(host_ptr)] ~= nil then
      mem_usm_dep[gits.udtToInt(memory)] = gits.udtToInt(host_ptr)
    end
  elseif mem_nums_req[mem_ctr] ~= nil then
    memory = drvCl.clCreateBufferWithPropertiesINTEL(context, properties, flags, size, host_ptr, err)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return memory
end

function clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
  mem_ctr = mem_ctr + 1
  memory = gits.nullUdt()
  if dump_mode then
    memory = drvCl.clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
    mem_dep[gits.udtToInt(memory)] = gits.udtToInt(buffer)
    mem_to_num_map[gits.udtToInt(memory)] = mem_ctr
  elseif mem_nums_req[mem_ctr] ~= nil then
    memory = drvCl.clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return memory
end

function clCreateKernel(program, kernel_name, errcode_ret)
  kern_ctr = kern_ctr + 1
  kernel = gits.nullUdt()
  if dump_mode then
    kernel = drvCl.clCreateKernel(program, kernel_name, errcode_ret)
    kern_int = gits.udtToInt(kernel)
    kern_to_num_map[kern_int] = kern_ctr
    kern_to_attribs[kern_int] = deepcopy(KernAttribs)
    kern_to_attribs[kern_int].program = gits.udtToInt(program)
  elseif kern_nums_req[kern_ctr] ~= nil then
    kernel = drvCl.clCreateKernel(program, kernel_name, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return kernel
end

function clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
  --Api call used to get kernels count
  if kernels == gits.nullUdt() then
    return drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
  end
  --Check total kernels num
  INT_SIZE = 4
  kernels_count_ptr = gits.allocBytes(INT_SIZE)
  drvCl.clCreateKernelsInProgram(program, num_kernels, gits.nullUdt(), kernels_count_ptr)
  kernels_count = gits.getInt(kernels_count_ptr, 0)
  gits.freeBytes(kernels_count_ptr)
  if kernels_count == 0 then
    return drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
  end
  --Process
  if dump_mode then
    status = drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    if status ~= 0 then
      return status
    end
    for i=0, kernels_count-1 do
      kern_ctr = kern_ctr + 1
      kern_int = gits.getInt64(kernels, i)
      kern_to_num_map[kern_int] = kern_ctr
      kern_to_attribs[kern_int] = deepcopy(KernAttribs)
      kern_to_attribs[kern_int].program = gits.udtToInt(program)
    end
  else
    skip = true
    for i=0, kernels_count-1 do
      kern_ctr = kern_ctr + 1
      if kern_nums_req[kern_ctr] ~= nil then
        skip = false
      end
    end
    if skip == false then
      return drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    else
      skipped_funcs_ctr = skipped_funcs_ctr + 1
    end
  end
end

function clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
  prg_ctr = prg_ctr + 1
  program = gits.nullUdt()
  if dump_mode then
    program = drvCl.clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
    prg_to_num_map[gits.udtToInt(program)] = prg_ctr
  elseif prg_nums_req[prg_ctr] ~= nil then
    program = drvCl.clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return program
end

function clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
  prg_ctr = prg_ctr + 1
  program = gits.nullUdt()
  if dump_mode then
    program = drvCl.clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
    prg_to_num_map[gits.udtToInt(program)] = prg_ctr
  elseif prg_nums_req[prg_ctr] ~= nil then
    program = drvCl.clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return program
end

function clCreateProgramWithIL(context, il, length, errcode_ret)
  prg_ctr = prg_ctr + 1
  program = gits.nullUdt()
  if dump_mode then
    program = drvCl.clCreateProgramWithIL(context, il, length, errcode_ret)
    prg_to_num_map[gits.udtToInt(program)] = prg_ctr
  elseif prg_nums_req[prg_ctr] ~= nil then
    program = drvCl.clCreateProgramWithIL(context, il, length, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return program
end

function clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
  prg_ctr = prg_ctr + 1
  program = gits.nullUdt()
  if dump_mode then
    program = drvCl.clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
    prg_to_num_map[gits.udtToInt(program)] = prg_ctr
  elseif prg_nums_req[prg_ctr] ~= nil then
    program = drvCl.clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return program
end

function clSetKernelArg(kernel, arg_index, arg_size, arg_value)
  ret_val = 0
  if dump_mode then
    ret_val = drvCl.clSetKernelArg(kernel, arg_index, arg_size, arg_value)
    if gits.udtToInt(arg_value) ~= 0 then
      usm_num = part_of_usm(gits.getInt64(arg_value, 0))
      if usm_num ~= nil then
        kern_to_attribs[gits.udtToInt(kernel)].usms[usm_num] = true
      else
        mem = gits.getInt64(arg_value, 0)
        if mem_to_num_map[mem] ~= nil then
          kern_to_attribs[gits.udtToInt(kernel)].mems[mem] = true
        end
      end
    end
  else
    if gits.udtToInt(kernel) ~= 0 then
      ret_val = drvCl.clSetKernelArg(kernel, arg_index, arg_size, arg_value)
    else
      skipped_funcs_ctr = skipped_funcs_ctr + 1
    end
  end
  if ret_val ~= 0 then
    print("script error: clSetKernelArg failed. Returned value: "..drvCl.statusToStr(ret_val))
    os.exit()
  end
  return ret_val
end

function clSetKernelArgSVMPointer(kernel, arg_index, arg_value)
  ret_val = 0
  if dump_mode then
    ret_val = drvCl.clSetKernelArgSVMPointer(kernel, arg_index, arg_value)
    usm_num = part_of_usm(gits.udtToInt(arg_value))
    if usm_num ~= nil then
      kern_to_attribs[gits.udtToInt(kernel)].usms[usm_num] = true
    end
  else
    if gits.udtToInt(kernel) ~= 0 then
      ret_val = drvCl.clSetKernelArgSVMPointer(kernel, arg_index, arg_value)
    else
      skipped_funcs_ctr = skipped_funcs_ctr + 1
    end
  end
  if ret_val ~= 0 then
    print("script error: clSetKernelArgSVMPointer failed. Returned value: "..drvCl.statusToStr(ret_val))
    os.exit()
  end
  return ret_val
end

function clSetKernelArgMemPointerINTEL(kernel, arg_index, arg_value)
  ret_val = 0
  if dump_mode then
    ret_val = drvCl.clSetKernelArgMemPointerINTEL(kernel, arg_index, arg_value)
    usm_num = part_of_usm(gits.udtToInt(arg_value))
    if usm_num ~= nil then
      kern_to_attribs[gits.udtToInt(kernel)].usms[usm_num] = true
    end
  else
    if gits.udtToInt(kernel) ~= 0 then
      ret_val = drvCl.clSetKernelArgMemPointerINTEL(kernel, arg_index, arg_value)
    else
      skipped_funcs_ctr = skipped_funcs_ctr + 1
    end
  end
  if ret_val ~= 0 then
    print("script error: clSetKernelArgMemPointerINTEL failed. Returned value: "..drvCl.statusToStr(ret_val))
    os.exit()
  end
  return ret_val
end


function clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
  ret_val = drvCl.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event)
  --used data
  if dump_mode then
    kern = gits.udtToInt(kernel)
    kern_nums_req[kern_to_num_map[kern]] = true
    prg = kern_to_attribs[kern].program
    prg_nums_req[prg_to_num_map[prg]] = true

    mems = kern_to_attribs[kern].mems
    for k in pairs(mems) do
      mem_nums_req[mem_to_num_map[k]] = true
      if mem_dep[k] ~= nil then
        mem_nums_req[mem_to_num_map[mem_dep[k]]] = true
      end
      if mem_usm_dep[k] ~= nil then
        mem_nums_req[usm_to_num_map[mem_usm_dep[k]]] = true
      end
    end

    usms = kern_to_attribs[kern].usms
    for k in pairs(usms) do
      usm_nums_req[usm_to_num_map[k]] = true
      if usm_dep[k] ~= nil then
        mem_nums_req[usm_to_num_map[usm_dep[k]]] = true
      end
      if usm_mem_dep[k] ~= nil then
        mem_nums_req[mem_to_num_map[usm_mem_dep[k]]] = true
      end
    end
  end
  if ret_val ~= 0 then
    print("script error: clEnqueueNDRangeKernel failed. Returned value: "..drvCl.statusToStr(ret_val))
    os.exit()
  end
  return ret_val
end

--  have to save memory objects due to implementation in GITS player runwrap
function clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event)
  mem = gits.udtToInt(memobj)
  if mem ~= 0 then
    if dump_mode then
      mem_nums_req[mem_to_num_map[mem]] = true
      if mem_dep[mem] ~= nil then
        mem_nums_req[mem_to_num_map[mem_dep[mem]]] = true
      end
    end
    return drvCl.clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueMemcpyINTEL(command_queue, blocking, dst_ptr, src_ptr, size, num_events_in_wait_list, event_wait_list, event) 
  if dump_mode then
    -- Saving USM and memory dependencies
    src_num = gits.udtToInt(src_ptr)
    dst_num = gits.udtToInt(dst_ptr)
    if mem_to_num_map[src_num] ~= nil and usm_to_num_map[dst_num] ~= nil then
      usm_mem_dep[dst_num] = src_num
    end
    if mem_to_num_map[src_num] ~= nil and mem_to_num_map[dst_num] ~= nil then
      mem_dep[dst_num] = src_num
    end
    if usm_to_num_map[src_num] ~= nil and usm_to_num_map[dst_num] ~= nil then
      usm_dep[dst_num] = src_num
    end
    if usm_to_num_map[src_num] ~= nil and mem_to_num_map[dst_num] ~= nil then
      mem_usm_dep[dst_num] = src_num
    end
  end
  if gits.udtToInt(dst_ptr) ~= 0 and gits.udtToInt(src_ptr) ~= 0 then
    return drvCl.clEnqueueMemcpyINTEL(command_queue, blocking, dst_ptr, src_ptr, size, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clSetKernelExecInfo(kernel, param_name, param_value_size, param_value)
  if dump_mode then
    if param_name == CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL or param_name == CL_KERNEL_EXEC_INFO_SVM_PTRS then
      length = gits.udtToInt(param_value_size) / gits.getPtrSize()
      for i=0, i<length do
        param_num = gits.udtToInt(gits.getUdt(param_value, i))
        kern_to_attribs[gits.udtToInt(kernel)].usms[param_num] = true
      end
    elseif param_name == CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM then
      for _, usm_struct in pairs(usm_structs) do
        if usm_struct.type == USM_SVM_ALLOC then
          kern_to_attribs[gits.udtToInt(kernel)].usms[usm_struct.address] = true
        end
      end
    elseif param_name == CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL then
      for _, usm_struct in pairs(usm_structs) do
        if usm_struct.type == USM_HOST_ALLOC then
          kern_to_attribs[gits.udtToInt(kernel)].usms[usm_struct.address] = true
        end
      end
    elseif param_name == CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL then
      for _, usm_struct in pairs(usm_structs) do
        if usm_struct.type == USM_DEVICE_ALLOC then
          kern_to_attribs[gits.udtToInt(kernel)].usms[usm_struct.address] = true
        end
      end
    elseif param_name == CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL then
      for _, usm_struct in pairs(usm_structs) do
        if usm_struct.type == USM_SHARED_ALLOC then
          kern_to_attribs[gits.udtToInt(kernel)].usms[usm_struct.address] = true
        end
      end
    end
  end
  if gits.udtToInt(kernel) ~= 0 then
    return drvCl.clSetKernelExecInfo(kernel, param_name, param_value_size, param_value)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

--  USM allocations support 
function clSVMAlloc(context, flags, size, alignment)
  usm_ctr = usm_ctr + 1
  usm = gits.nullUdt()
  if dump_mode then
    usm = drvCl.clSVMAlloc(context, flags, size, alignment)
    save_usm_data(usm, size, USM_SVM_ALLOC)
  elseif usm_nums_req[usm_ctr] ~= nil then
    usm = drvCl.clSVMAlloc(context, flags, size, alignment)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return usm
end

function clHostMemAllocINTEL(context, properties, size, alignment, errcode_ret)
  usm_ctr = usm_ctr + 1
  usm = gits.nullUdt()
  if dump_mode then
    usm = drvCl.clHostMemAllocINTEL(context, properties, size, alignment, errcode_ret)
    save_usm_data(usm, size, USM_HOST_ALLOC)
  elseif usm_nums_req[usm_ctr] ~= nil then
    usm = drvCl.clHostMemAllocINTEL(context, properties, size, alignment, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return usm
end

function clDeviceMemAllocINTEL(context, device, properties, size, alignment, errcode_ret)
  usm_ctr = usm_ctr + 1
  usm = gits.nullUdt()
  if dump_mode then
    usm = drvCl.clDeviceMemAllocINTEL(context, device, properties, size, alignment, errcode_ret)
    save_usm_data(usm, size, USM_DEVICE_ALLOC)
  elseif usm_nums_req[usm_ctr] ~= nil then
    usm = drvCl.clDeviceMemAllocINTEL(context, device, properties, size, alignment, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return usm
end

function clSharedMemAllocINTEL(context, device, properties, size, alignment, errcode_ret)
  usm_ctr = usm_ctr + 1
  usm = gits.nullUdt()
  if dump_mode then
    usm = drvCl.clSharedMemAllocINTEL(context, device, properties, size, alignment, errcode_ret)
    save_usm_data(usm, size, USM_SHARED_ALLOC)
  elseif usm_nums_req[usm_ctr] ~= nil then
    usm = drvCl.clSharedMemAllocINTEL(context, device, properties, size, alignment, errcode_ret)
  else
    skipped_funcs_ctr = skipped_funcs_ctr + 1
  end
  return usm
end

-- Intercepted functions skipped for nullptr set on creation

function clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret)
  if gits.udtToInt(buffer) ~= 0 then
    return drvCl.clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
  if gits.udtToInt(image) ~= 0 then
    return drvCl.clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(buffer) ~= 0 then
    return drvCl.clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueWriteBufferRect(command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(buffer) ~= 0 then
    return drvCl.clEnqueueWriteBufferRect(command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(image) ~= 0 then
    return drvCl.clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(dst_buffer) ~= 0 then
    return drvCl.clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(dst_buffer) ~= 0 then
    return drvCl.clEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(dst_image) ~= 0 then
    return drvCl.clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueCopyImageToBuffer(command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(dst_buffer) ~= 0 then
    return drvCl.clEnqueueCopyImageToBuffer(command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueCopyBufferToImage(command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(dst_image) ~= 0 then
    return drvCl.clEnqueueCopyBufferToImage(command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueFillBuffer(command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(buffer) ~= 0 then
    return drvCl.clEnqueueFillBuffer(command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueFillImage(command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(image) ~= 0 then
    return drvCl.clEnqueueFillImage(command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueMigrateMemObjects(command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event)
  skip = false
  for i=0, num_mem_objects - 1 do
    if gits.getInt64(mem_objects, i) == 0 then
      skip = true
    end
  end
  if dump_mode or skip == false then
    return drvCl.clEnqueueMigrateMemObjects(command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueNativeKernel(command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event)
  skip = false
  for i=0, num_mem_objects - 1 do
    if gits.getInt64(mem_list, i) == 0 then
      skip = true
    end
  end
  if dump_mode or skip == false then
    return drvCl.clEnqueueNativeKernel(command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(buffer) ~= 0 then
    return drvCl.clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueReadBufferRect(command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(buffer) ~= 0 then
    return drvCl.clEnqueueReadBufferRect(command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(image) ~= 0 then
    return drvCl.clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueMemFillINTEL(command_queue, dst_ptr, pattern, pattern_size, size, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(dst_ptr) ~= 0 then
    return drvCl.clEnqueueMemFillINTEL(command_queue, dst_ptr, pattern, pattern_size, size, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueMigrateMemINTEL(command_queue, ptr, size, flags, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(ptr) ~= 0 then
    return drvCl.clEnqueueMigrateMemINTEL(command_queue, ptr, size, flags, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clEnqueueMemAdviseINTEL(command_queue, ptr, size, advice, num_events_in_wait_list, event_wait_list, event)
  if dump_mode or gits.udtToInt(ptr) ~= 0 then
    return drvCl.clEnqueueMemAdviseINTEL(command_queue, ptr, size, advice, num_events_in_wait_list, event_wait_list, event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetImageInfo(image, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(image) ~= 0 then
    return drvCl.clGetImageInfo(image, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetKernelArgInfo(kernel, arg_indx, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(kernel) ~= 0 then
    return drvCl.clGetKernelArgInfo(kernel, arg_indx, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetKernelInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret)
  if gits.udtToInt(kernel) ~= 0 then
    return drvCl.clGetKernelInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetKernelSubGroupInfo(kernel, device, param_name, input_value_size, input_value, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(kernel) ~= 0 then
    return drvCl.clGetKernelSubGroupInfo(kernel, device, param_name, input_value_size, input_value, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(kernel) ~= 0 then
    return drvCl.clGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(memobj) ~= 0 then
    return drvCl.clGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetPipeInfo(pipe, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(pipe) ~= 0 then
    return drvCl.clGetPipeInfo(pipe, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetProgramBuildInfo(program, device, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(program) ~= 0 then
    return drvCl.clGetProgramBuildInfo(program, device, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetProgramInfo(program, param_name, param_value_size, param_value, param_value_size_ret)
  if dump_mode or gits.udtToInt(program) ~= 0 then
    return drvCl.clGetProgramInfo(program, param_name, param_value_size, param_value, param_value_size_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data)
  if dump_mode or gits.udtToInt(program) ~= 0 then
    return drvCl.clCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret)
  skip = false
  for i=0, num_input_programs - 1 do
    if gits.getInt64(input_programs, i) == 0 then
      skip = true
    end
  end
  if dump_mode or skip == false then
    return drvCl.clLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clSetMemObjectDestructorCallback(memobj, pfn_notify, user_data)
  if dump_mode or gits.udtToInt(memobj) ~= 0 then
    return drvCl.clSetMemObjectDestructorCallback(memobj, pfn_notify, user_data)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
  if gits.udtToInt(program) ~= 0 then
    return drvCl.clBuildProgram(program, num_devices, device_list, options, pfn_notify, user_data)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clRetainMemObject(memobj)
  if dump_mode or gits.udtToInt(memobj) ~= 0 then
    return drvCl.clRetainMemObject(memobj)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clReleaseMemObject(memobj)
  if dump_mode or gits.udtToInt(memobj) ~= 0 then
    return drvCl.clReleaseMemObject(memobj)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clMemFreeINTEL(context, ptr)
  if dump_mode or gits.udtToInt(ptr) ~= 0 then
    return drvCl.clMemFreeINTEL(context, ptr)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clMemBlockingFreeINTEL(context, ptr)
  if dump_mode or gits.udtToInt(ptr) ~= 0 then
    return drvCl.clMemBlockingFreeINTEL(context, ptr)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clRetainProgram(program)
  if dump_mode or gits.udtToInt(program) ~= 0 then
    return drvCl.clRetainProgram(program)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clReleaseProgram(program)
  if dump_mode or gits.udtToInt(program) ~= 0 then
    return drvCl.clReleaseProgram(program)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clRetainKernel(kernel)
  if dump_mode or gits.udtToInt(kernel) ~= 0 then
    return drvCl.clRetainKernel(kernel)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clReleaseKernel(kernel)
  if dump_mode or gits.udtToInt(kernel) ~= 0 then
    return drvCl.clReleaseKernel(kernel)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clReleaseEvent(event)
  if dump_mode or gits.udtToInt(event) ~= 0 then
    return drvCl.clReleaseEvent(event)
  end
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

function clGetMemAllocInfoINTEL(context, ptr, param_name, param_value_size, param_value, param_value_size_ret)
  skipped_funcs_ctr = skipped_funcs_ctr + 1
  return 0
end

-- Program Start
function gitsProgramStart()
  args_str = gits.getArgsStr()
  if string.find(args_str, "dump") ~= nil then
    dump_mode = true
  else
    fread_objects(dump_file_name)
  end
end

-- Program End
function gitsProgramExit()
  if dump_mode then
    fwrite_objects(dump_file_name)
    print("*************** reduce_ocl_state.lua dumping summary *********************")
    print(" ")
    print("File dumped to CWD: ")
    print("- "..dump_file_name)
    print(" ")
    print("**************************************************************************")
  else
    print("*************** reduce_ocl_state.lua filtering summary *********************")
    print(" ")
    print("Skipped functions count: "..skipped_funcs_ctr)
    print(" ")
    print("**************************************************************************")
  end
end

-- Utilities
function deepcopy(src)
  local type_ = type(src)
  if type_ == 'table' then
    local dst = {}
    for srcKey, srcVal in pairs(src) do
      dst[ deepcopy(srcKey) ] = deepcopy(srcVal)
    end
    src_meta = getmetatable(src)
    setmetatable(dst, deepcopy(getmetatable(src)))
    return dst
  else
    local dst = src
    return dst
  end
end

function fwrite_objects(fname)
  file = io.open(fname, "w")
  if file == nil then
    print("script error: "..fname.." creation failed.")
    os.exit()
  end
  fwrite_numbers(file, "cl_mem", mem_nums_req)
  fwrite_numbers(file, "cl_kernel", kern_nums_req)
  fwrite_numbers(file, "cl_program", prg_nums_req)
  fwrite_numbers(file, "USM", usm_nums_req)
end

function fwrite_numbers(file, type, arr_req)
  io.output(file)
  io.write(type.."\n")
  for k in pairs(arr_req) do
    io.write(tostring(k),"\n")
  end
  io.write("end\n")
end

function fread_numbers(file, arr_req)
  io.input(file)
  val = io.read("l")
  while val ~= "end" do
    arr_req[tonumber(val)] = true
    val = io.read("l")
  end
end

function fread_objects(fname)
  file = io.open(fname, "r")
  if file == nil then
    print("script error: "..fname.." opening failed.")
    os.exit()
  end
  io.input(file)
  val = io.read("l")
  local ctr = 0
  while val ~= nil do
    if val == "cl_mem" then
      fread_numbers(file, mem_nums_req)
    elseif val == "cl_kernel" then
      fread_numbers(file, kern_nums_req)
    elseif val == "cl_program" then
      fread_numbers(file, prg_nums_req)
    elseif val == "USM" then
      fread_numbers(file, usm_nums_req)
    end
    val = io.read("l")
    ctr = ctr + 1
  end
  if ctr == 0 then
    print("script warning: "..fname.." empty.")
  end
end


function save_usm_data(usm, size, type)
  usm_num = gits.udtToInt(usm)
  usm_to_num_map[usm_num] = usm_ctr
  usm_struct = Usm:new(usm_num, size, type)
  table.insert(usm_structs, usm_struct)
end

function part_of_usm(usm_num)
  if usm_to_num_map[usm_num] ~= nil then
    return usm_num
  end
  for _, usm_struct in pairs(usm_structs) do
    if gits.isInt64InRange(usm_struct.address, usm_struct.size, usm_num) then
      return usm_struct.address
    end
  end
  return nil
end