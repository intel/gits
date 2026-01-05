// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

case ID_CL_ADD_COMMENT_INTEL:
return new CclAddCommentINTEL;
case ID_CL_BUILD_PROGRAM:
return new CclBuildProgram;
case ID_CL_CLONE_KERNEL:
return new CclCloneKernel;
case ID_CL_COMPILE_PROGRAM:
return new CclCompileProgram;
case ID_CL_CREATE_BUFFER:
return new CclCreateBuffer;
case ID_CL_CREATE_BUFFER_WITH_PROPERTIES_INTEL:
return new CclCreateBufferWithPropertiesINTEL;
case ID_CL_CREATE_COMMAND_QUEUE:
return new CclCreateCommandQueue;
case ID_CL_CREATE_COMMAND_QUEUE_WITH_PROPERTIES:
return new CclCreateCommandQueueWithProperties;
case ID_CL_CREATE_COMMAND_QUEUE_WITH_PROPERTIES_INTEL:
return new CclCreateCommandQueueWithPropertiesINTEL;
case ID_CL_CREATE_CONTEXT:
return new CclCreateContext;
case ID_CL_CREATE_CONTEXT_FROM_TYPE:
return new CclCreateContextFromType;
case ID_CL_CREATE_EVENT_FROM_GLSYNC_KHR:
return new CclCreateEventFromGLsyncKHR;
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D10_BUFFER_KHR:
return new CclCreateFromD3D10BufferKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D10_TEXTURE2D_KHR:
return new CclCreateFromD3D10Texture2DKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D10_TEXTURE3D_KHR:
return new CclCreateFromD3D10Texture3DKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D11_BUFFER_KHR:
return new CclCreateFromD3D11BufferKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D11_TEXTURE2D_KHR:
return new CclCreateFromD3D11Texture2DKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D11_TEXTURE3D_KHR:
return new CclCreateFromD3D11Texture3DKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D11_BUFFER_NV:
return new CclCreateFromD3D11BufferNV;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D11_TEXTURE2D_NV:
return new CclCreateFromD3D11Texture2DNV;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_D3D11_TEXTURE3D_NV:
return new CclCreateFromD3D11Texture3DNV;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_DX9_MEDIA_SURFACE_INTEL:
return new CclCreateFromDX9MediaSurfaceINTEL;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_CREATE_FROM_DX9_MEDIA_SURFACE_KHR:
return new CclCreateFromDX9MediaSurfaceKHR;
#endif
case ID_CL_CREATE_FROM_GLBUFFER:
return new CclCreateFromGLBuffer;
case ID_CL_CREATE_FROM_GLRENDERBUFFER:
return new CclCreateFromGLRenderbuffer;
case ID_CL_CREATE_FROM_GLTEXTURE:
return new CclCreateFromGLTexture;
case ID_CL_CREATE_FROM_GLTEXTURE2D:
return new CclCreateFromGLTexture2D;
case ID_CL_CREATE_FROM_GLTEXTURE3D:
return new CclCreateFromGLTexture3D;
case ID_CL_CREATE_IMAGE:
return new CclCreateImage;
case ID_CL_CREATE_IMAGE2D:
return new CclCreateImage2D;
case ID_CL_CREATE_IMAGE3D:
return new CclCreateImage3D;
case ID_CL_CREATE_IMAGE_WITH_PROPERTIES_INTEL:
return new CclCreateImageWithPropertiesINTEL;
case ID_CL_CREATE_KERNEL:
return new CclCreateKernel;
case ID_CL_CREATE_KERNELS_IN_PROGRAM:
return new CclCreateKernelsInProgram;
case ID_CL_CREATE_PIPE:
return new CclCreatePipe;
case ID_CL_CREATE_PROGRAM_WITH_BINARY:
return new CclCreateProgramWithBinary;
case ID_CL_CREATE_PROGRAM_WITH_BINARY_V1:
return new CclCreateProgramWithBinary_V1;
case ID_CL_CREATE_PROGRAM_WITH_BUILT_IN_KERNELS:
return new CclCreateProgramWithBuiltInKernels;
case ID_CL_CREATE_PROGRAM_WITH_IL:
return new CclCreateProgramWithIL;
case ID_CL_CREATE_PROGRAM_WITH_SOURCE:
return new CclCreateProgramWithSource;
case ID_CL_CREATE_SAMPLER:
return new CclCreateSampler;
case ID_CL_CREATE_SAMPLER_WITH_PROPERTIES:
return new CclCreateSamplerWithProperties;
case ID_CL_CREATE_SUB_BUFFER:
return new CclCreateSubBuffer;
case ID_CL_CREATE_SUB_DEVICES:
return new CclCreateSubDevices;
case ID_CL_CREATE_SUB_DEVICES_EXT:
return new CclCreateSubDevicesEXT;
case ID_CL_CREATE_USER_EVENT:
return new CclCreateUserEvent;
case ID_CL_DEVICE_MEM_ALLOC_INTEL:
return new CclDeviceMemAllocINTEL;
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_ACQUIRE_D3D10_OBJECTS_KHR:
return new CclEnqueueAcquireD3D10ObjectsKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_ACQUIRE_D3D11_OBJECTS_KHR:
return new CclEnqueueAcquireD3D11ObjectsKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_ACQUIRE_D3D11_OBJECTS_NV:
return new CclEnqueueAcquireD3D11ObjectsNV;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_ACQUIRE_DX9_MEDIA_SURFACES_KHR:
return new CclEnqueueAcquireDX9MediaSurfacesKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_ACQUIRE_DX9_OBJECTS_INTEL:
return new CclEnqueueAcquireDX9ObjectsINTEL;
#endif
case ID_CL_ENQUEUE_ACQUIRE_GLOBJECTS:
return new CclEnqueueAcquireGLObjects;
case ID_CL_ENQUEUE_BARRIER:
return new CclEnqueueBarrier;
case ID_CL_ENQUEUE_BARRIER_WITH_WAIT_LIST:
return new CclEnqueueBarrierWithWaitList;
case ID_CL_ENQUEUE_COPY_BUFFER:
return new CclEnqueueCopyBuffer;
case ID_CL_ENQUEUE_COPY_BUFFER_RECT:
return new CclEnqueueCopyBufferRect;
case ID_CL_ENQUEUE_COPY_BUFFER_TO_IMAGE:
return new CclEnqueueCopyBufferToImage;
case ID_CL_ENQUEUE_COPY_IMAGE:
return new CclEnqueueCopyImage;
case ID_CL_ENQUEUE_COPY_IMAGE_TO_BUFFER:
return new CclEnqueueCopyImageToBuffer;
case ID_CL_ENQUEUE_FILL_BUFFER:
return new CclEnqueueFillBuffer;
case ID_CL_ENQUEUE_FILL_IMAGE:
return new CclEnqueueFillImage;
case ID_CL_ENQUEUE_MAP_BUFFER:
return new CclEnqueueMapBuffer;
case ID_CL_ENQUEUE_MAP_IMAGE:
return new CclEnqueueMapImage;
case ID_CL_ENQUEUE_MARKER:
return new CclEnqueueMarker;
case ID_CL_ENQUEUE_MARKER_WITH_WAIT_LIST:
return new CclEnqueueMarkerWithWaitList;
case ID_CL_ENQUEUE_MEM_ADVISE_INTEL:
return new CclEnqueueMemAdviseINTEL;
case ID_CL_ENQUEUE_MEM_FILL_INTEL:
return new CclEnqueueMemFillINTEL;
case ID_CL_ENQUEUE_MEMCPY_INTEL:
return new CclEnqueueMemcpyINTEL;
case ID_CL_ENQUEUE_MEMSET_INTEL:
return new CclEnqueueMemsetINTEL;
case ID_CL_ENQUEUE_MIGRATE_MEM_INTEL:
return new CclEnqueueMigrateMemINTEL;
case ID_CL_ENQUEUE_MIGRATE_MEM_OBJECTS:
return new CclEnqueueMigrateMemObjects;
case ID_CL_ENQUEUE_NDCOUNT_KERNEL_INTEL:
return new CclEnqueueNDCountKernelINTEL;
case ID_CL_ENQUEUE_NDRANGE_KERNEL:
return new CclEnqueueNDRangeKernel;
case ID_CL_ENQUEUE_READ_BUFFER:
return new CclEnqueueReadBuffer;
case ID_CL_ENQUEUE_READ_BUFFER_RECT:
return new CclEnqueueReadBufferRect;
case ID_CL_ENQUEUE_READ_IMAGE:
return new CclEnqueueReadImage;
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_RELEASE_D3D10_OBJECTS_KHR:
return new CclEnqueueReleaseD3D10ObjectsKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_RELEASE_D3D11_OBJECTS_KHR:
return new CclEnqueueReleaseD3D11ObjectsKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_RELEASE_DX9_MEDIA_SURFACES_KHR:
return new CclEnqueueReleaseDX9MediaSurfacesKHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_ENQUEUE_RELEASE_DX9_OBJECTS_INTEL:
return new CclEnqueueReleaseDX9ObjectsINTEL;
#endif
case ID_CL_ENQUEUE_RELEASE_GLOBJECTS:
return new CclEnqueueReleaseGLObjects;
case ID_CL_ENQUEUE_RESOURCES_BARRIER_INTEL:
return new CclEnqueueResourcesBarrierINTEL;
case ID_CL_ENQUEUE_SVMFREE:
return new CclEnqueueSVMFree;
case ID_CL_ENQUEUE_SVMMAP:
return new CclEnqueueSVMMap;
case ID_CL_ENQUEUE_SVMMEM_FILL:
return new CclEnqueueSVMMemFill;
case ID_CL_ENQUEUE_SVMMEM_FILL_V1:
return new CclEnqueueSVMMemFill_V1;
case ID_CL_ENQUEUE_SVMMEMCPY:
return new CclEnqueueSVMMemcpy;
case ID_CL_ENQUEUE_SVMMEMCPY_V1:
return new CclEnqueueSVMMemcpy_V1;
case ID_CL_ENQUEUE_SVMMIGRATE_MEM:
return new CclEnqueueSVMMigrateMem;
case ID_CL_ENQUEUE_SVMUNMAP:
return new CclEnqueueSVMUnmap;
case ID_CL_ENQUEUE_TASK:
return new CclEnqueueTask;
case ID_CL_ENQUEUE_UNMAP_MEM_OBJECT:
return new CclEnqueueUnmapMemObject;
case ID_CL_ENQUEUE_VERIFY_MEMORY_INTEL:
return new CclEnqueueVerifyMemoryINTEL;
case ID_CL_ENQUEUE_WAIT_FOR_EVENTS:
return new CclEnqueueWaitForEvents;
case ID_CL_ENQUEUE_WRITE_BUFFER:
return new CclEnqueueWriteBuffer;
case ID_CL_ENQUEUE_WRITE_BUFFER_RECT:
return new CclEnqueueWriteBufferRect;
case ID_CL_ENQUEUE_WRITE_IMAGE:
return new CclEnqueueWriteImage;
case ID_CL_FINISH:
return new CclFinish;
case ID_CL_FLUSH:
return new CclFlush;
case ID_CL_GET_COMMAND_QUEUE_INFO:
return new CclGetCommandQueueInfo;
case ID_CL_GET_CONTEXT_INFO:
return new CclGetContextInfo;
case ID_CL_GET_DEVICE_AND_HOST_TIMER:
return new CclGetDeviceAndHostTimer;
case ID_CL_GET_DEVICE_FUNCTION_POINTER_INTEL:
return new CclGetDeviceFunctionPointerINTEL;
case ID_CL_GET_DEVICE_GLOBAL_VARIABLE_POINTER_INTEL:
return new CclGetDeviceGlobalVariablePointerINTEL;
case ID_CL_GET_DEVICE_IDS:
return new CclGetDeviceIDs;
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_GET_DEVICE_IDS_FROM_D3D10_KHR:
return new CclGetDeviceIDsFromD3D10KHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_GET_DEVICE_IDS_FROM_D3D11_KHR:
return new CclGetDeviceIDsFromD3D11KHR;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_GET_DEVICE_IDS_FROM_D3D11_NV:
return new CclGetDeviceIDsFromD3D11NV;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_GET_DEVICE_IDS_FROM_DX9_INTEL:
return new CclGetDeviceIDsFromDX9INTEL;
#endif
#ifdef GITS_PLATFORM_WINDOWS
case ID_CL_GET_DEVICE_IDS_FROM_DX9_MEDIA_ADAPTER_KHR:
return new CclGetDeviceIDsFromDX9MediaAdapterKHR;
#endif
case ID_CL_GET_DEVICE_INFO:
return new CclGetDeviceInfo;
case ID_CL_GET_EVENT_INFO:
return new CclGetEventInfo;
case ID_CL_GET_EVENT_PROFILING_INFO:
return new CclGetEventProfilingInfo;
case ID_CL_GET_EXTENSION_FUNCTION_ADDRESS:
return new CclGetExtensionFunctionAddress;
case ID_CL_GET_EXTENSION_FUNCTION_ADDRESS_FOR_PLATFORM:
return new CclGetExtensionFunctionAddressForPlatform;
case ID_CL_GET_GLCONTEXT_INFO_KHR:
return new CclGetGLContextInfoKHR;
case ID_CL_GET_GLOBJECT_INFO:
return new CclGetGLObjectInfo;
case ID_CL_GET_GLTEXTURE_INFO:
return new CclGetGLTextureInfo;
case ID_CL_GET_IMAGE_INFO:
return new CclGetImageInfo;
case ID_CL_GET_KERNEL_ARG_INFO:
return new CclGetKernelArgInfo;
case ID_CL_GET_KERNEL_INFO:
return new CclGetKernelInfo;
case ID_CL_GET_KERNEL_MAX_CONCURRENT_WORK_GROUP_COUNT_INTEL:
return new CclGetKernelMaxConcurrentWorkGroupCountINTEL;
case ID_CL_GET_KERNEL_SUB_GROUP_INFO:
return new CclGetKernelSubGroupInfo;
case ID_CL_GET_KERNEL_SUB_GROUP_INFO_KHR:
return new CclGetKernelSubGroupInfoKHR;
case ID_CL_GET_KERNEL_SUGGESTED_LOCAL_WORK_SIZE_INTEL:
return new CclGetKernelSuggestedLocalWorkSizeINTEL;
case ID_CL_GET_KERNEL_WORK_GROUP_INFO:
return new CclGetKernelWorkGroupInfo;
case ID_CL_GET_MEM_ALLOC_INFO_INTEL:
return new CclGetMemAllocInfoINTEL;
case ID_CL_GET_MEM_OBJECT_INFO:
return new CclGetMemObjectInfo;
case ID_CL_GET_PIPE_INFO:
return new CclGetPipeInfo;
case ID_CL_GET_PLATFORM_IDS:
return new CclGetPlatformIDs;
case ID_CL_GET_PLATFORM_INFO:
return new CclGetPlatformInfo;
case ID_CL_GET_PROGRAM_BUILD_INFO:
return new CclGetProgramBuildInfo;
case ID_CL_GET_PROGRAM_INFO:
return new CclGetProgramInfo;
case ID_CL_GET_SAMPLER_INFO:
return new CclGetSamplerInfo;
case ID_CL_GET_SUPPORTED_IMAGE_FORMATS:
return new CclGetSupportedImageFormats;
case ID_CL_GITS_INDIRECT_ALLOCATION_OFFSETS:
return new CclGitsIndirectAllocationOffsets;
case ID_CL_HOST_MEM_ALLOC_INTEL:
return new CclHostMemAllocINTEL;
case ID_CL_LINK_PROGRAM:
return new CclLinkProgram;
case ID_CL_MEM_BLOCKING_FREE_INTEL:
return new CclMemBlockingFreeINTEL;
case ID_CL_MEM_FREE_INTEL:
return new CclMemFreeINTEL;
case ID_CL_RELEASE_COMMAND_QUEUE:
return new CclReleaseCommandQueue;
case ID_CL_RELEASE_CONTEXT:
return new CclReleaseContext;
case ID_CL_RELEASE_DEVICE:
return new CclReleaseDevice;
case ID_CL_RELEASE_DEVICE_EXT:
return new CclReleaseDeviceEXT;
case ID_CL_RELEASE_EVENT:
return new CclReleaseEvent;
case ID_CL_RELEASE_KERNEL:
return new CclReleaseKernel;
case ID_CL_RELEASE_MEM_OBJECT:
return new CclReleaseMemObject;
case ID_CL_RELEASE_PROGRAM:
return new CclReleaseProgram;
case ID_CL_RELEASE_SAMPLER:
return new CclReleaseSampler;
case ID_CL_RETAIN_COMMAND_QUEUE:
return new CclRetainCommandQueue;
case ID_CL_RETAIN_CONTEXT:
return new CclRetainContext;
case ID_CL_RETAIN_DEVICE:
return new CclRetainDevice;
case ID_CL_RETAIN_DEVICE_EXT:
return new CclRetainDeviceEXT;
case ID_CL_RETAIN_EVENT:
return new CclRetainEvent;
case ID_CL_RETAIN_KERNEL:
return new CclRetainKernel;
case ID_CL_RETAIN_MEM_OBJECT:
return new CclRetainMemObject;
case ID_CL_RETAIN_PROGRAM:
return new CclRetainProgram;
case ID_CL_RETAIN_SAMPLER:
return new CclRetainSampler;
case ID_CL_SVMALLOC:
return new CclSVMAlloc;
case ID_CL_SVMFREE:
return new CclSVMFree;
case ID_CL_SET_COMMAND_QUEUE_PROPERTY:
return new CclSetCommandQueueProperty;
case ID_CL_SET_EVENT_CALLBACK:
return new CclSetEventCallback;
case ID_CL_SET_KERNEL_ARG:
return new CclSetKernelArg;
case ID_CL_SET_KERNEL_ARG_MEM_POINTER_INTEL:
return new CclSetKernelArgMemPointerINTEL;
case ID_CL_SET_KERNEL_ARG_SVMPOINTER:
return new CclSetKernelArgSVMPointer;
case ID_CL_SET_KERNEL_ARG_V1:
return new CclSetKernelArg_V1;
case ID_CL_SET_KERNEL_EXEC_INFO:
return new CclSetKernelExecInfo;
case ID_CL_SET_KERNEL_EXEC_INFO_V1:
return new CclSetKernelExecInfo_V1;
case ID_CL_SET_MEM_OBJECT_DESTRUCTOR_CALLBACK:
return new CclSetMemObjectDestructorCallback;
case ID_CL_SET_PROGRAM_SPECIALIZATION_CONSTANT:
return new CclSetProgramSpecializationConstant;
case ID_CL_SET_USER_EVENT_STATUS:
return new CclSetUserEventStatus;
case ID_CL_SHARED_MEM_ALLOC_INTEL:
return new CclSharedMemAllocINTEL;
case ID_CL_UNLOAD_COMPILER:
return new CclUnloadCompiler;
case ID_CL_UNLOAD_PLATFORM_COMPILER:
return new CclUnloadPlatformCompiler;
case ID_CL_WAIT_FOR_EVENTS:
return new CclWaitForEvents;
