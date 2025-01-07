-- ===================== begin_copyright_notice ============================
--
-- Copyright (C) 2023-2025 Intel Corporation
--
-- SPDX-License-Identifier: MIT
--
-- ===================== end_copyright_notice ==============================

-- Prints return codes from errcode_ret.

function clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateBuffer(context, flags, size, host_ptr, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateCommandQueue(context, device, properties, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateCommandQueue(context, device, properties, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateCommandQueueWithProperties(context, device, properties, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateCommandQueueWithProperties(context, device, properties, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateContext(properties, num_devices, devices, pfn_notify, user_data, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateContext(properties, num_devices, devices, pfn_notify, user_data, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateContextFromType(properties, device_type, pfn_notify, user_data, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateContextFromType(properties, device_type, pfn_notify, user_data, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateEventFromGLsyncKHR(context, sync, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateEventFromGLsyncKHR(context, sync, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromD3D10BufferKHR(context, flags, resource, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromD3D10BufferKHR(context, flags, resource, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromD3D10Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromD3D10Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromD3D10Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromD3D10Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromD3D11BufferKHR(context, flags, resource, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromD3D11BufferKHR(context, flags, resource, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromD3D11Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvdrvCl.clCreateFromD3D11Texture2DKHR(context, flags, resource, subresource, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromD3D11Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromD3D11Texture3DKHR(context, flags, resource, subresource, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromDX9MediaSurfaceINTEL(context, flags, resource, shared_handle, plane, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromDX9MediaSurfaceINTEL(context, flags, resource, shared_handle, plane, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromGLBuffer(context, flags, bufobj, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromGLBuffer(context, flags, bufobj, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromGLRenderbuffer(context, flags, bufobj, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromGLRenderbuffer(context, flags, bufobj, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromGLTexture(context, flags, target, miplevel, texture, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromGLTexture(context, flags, target, miplevel, texture, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromGLTexture2D(context, flags, target, miplevel, texture, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromGLTexture2D(context, flags, target, miplevel, texture, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateFromGLTexture3D(context, flags, target, miplevel, texture, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateFromGLTexture3D(context, flags, target, miplevel, texture, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateImage2D(context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateImage2D(context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateImage3D(context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateImage3D(context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateKernel(program, kernel_name, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateKernel(program, kernel_name, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateProgramWithSource(context, count, strings, lengths, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateSampler(context, normalized_coords, addressing_mode, filter_mode, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateSampler(context, normalized_coords, addressing_mode, filter_mode, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clCreateUserEvent(context, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clCreateUserEvent(context, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end

function clLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret)
    local injected = false
    if gits.udtToInt(errcode_ret) == 0 then
        errcode_ret = gits.allocBytes(4)
        injected = true
    end
    local ret = drvCl.clLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret)
    gits.log(1, "^ errcode_ret = " .. drvCl.statusToStr(gits.getInt(errcode_ret, 0)))
    if injected == true then
        gits.freeBytes(errcode_ret)
    end
    return ret
end
