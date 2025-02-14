/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 *
 * File Name:  igdext.h
 *
 * Abstract:   Intel(R) Graphics Extensions SDK Public Header File
 *
 * Notes:      This file is intended to be included by the application to use
 *             Graphics Extensions framework
 */

#ifndef _IGDEXTAPI_H_
#define _IGDEXTAPI_H_

#include "stdint.h"

#pragma warning( push )
#pragma warning( disable : 4201 )

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
/// @brief Intel(R) Graphics Extensions infrastructure structures
//////////////////////////////////////////////////////////////////////////

// Device Extension Context structure (required by all D3D11 or D3D12 Device extensions)
struct INTCExtensionContext;

// Intel(R) Graphics Device detailed information (returned after successful Device Extension Context creation)
struct INTCDeviceInfo
{
    uint32_t GPUMaxFreq;
    uint32_t GPUMinFreq;
    uint32_t GTGeneration;
    uint32_t EUCount;
    uint32_t PackageTDP;
    uint32_t MaxFillRate;
    wchar_t  GTGenerationName[ 64 ];
};

// Intel(R) Graphics Extensions version structure
struct INTCExtensionVersion
{
    uint32_t HWFeatureLevel; ///< HW Feature Level, based on the Intel HW Platform
    uint32_t APIVersion;     ///< API Version
    uint32_t Revision;       ///< Revision number
};

// Intel(R) Graphics Extensions detailed information structure
struct INTCExtensionInfo
{
    INTCExtensionVersion RequestedExtensionVersion; ///< [in] Graphics Extension API version requested

    INTCDeviceInfo IntelDeviceInfo;         ///< [out] Intel Graphics Device detailed information
    const wchar_t* pDeviceDriverDesc;       ///< [out] Intel Graphics Driver description
    const wchar_t* pDeviceDriverVersion;    ///< [out] Intel Graphics Driver version string
    uint32_t       DeviceDriverBuildNumber; ///< [out] Intel Graphics Driver build number
};

// Application and Engine information structure for the Graphics Extensions and/or Intel Graphics Driver
// Deprecated - use INTCExtensionAppInfo1 instead!
struct INTCExtensionAppInfo
{
    const wchar_t* pApplicationName;   ///< [in] Application name
    uint32_t       ApplicationVersion; ///< [in] Application version
    const wchar_t* pEngineName;        ///< [in] Engine name
    uint32_t       EngineVersion;      ///< [in] Engine version
};

// Structure describing Graphics Extensions Version layout
struct INTCAppInfoVersion
{
    union
    {
        struct
        {
            uint32_t major;
            uint32_t minor;
            uint32_t patch;
            uint32_t reserved;
        };
        uint8_t raw[ 16 ];
    };
};

// Application and Engine information structure for the Intel(R) Graphics Extensions and/or Intel Graphics Driver
struct INTCExtensionAppInfo1
{
    const wchar_t*     pApplicationName;   ///< [in] Application name
    INTCAppInfoVersion ApplicationVersion; ///< [in] Application version
    const wchar_t*     pEngineName;        ///< [in] Engine name
    INTCAppInfoVersion EngineVersion;      ///< [in] Engine version
};

// Graphics Extensions Tools Support API callback table (igdext_tools.h)
#ifdef INTC_IGDEXT_D3D11
struct INTC_D3D11_API_CALLBACKS;
#endif // INTC_IGDEXT_D3D11
#ifdef INTC_IGDEXT_D3D12
struct INTC_D3D12_API_CALLBACKS;
#endif // INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief D3D11 Graphics Extensions data structure definitions
//////////////////////////////////////////////////////////////////////////

#ifdef INTC_IGDEXT_D3D11

// Texture2D Descriptor for D3D11
struct INTC_D3D11_TEXTURE2D_DESC
{
    union
    {
        D3D11_TEXTURE2D_DESC* pD3D11Desc; ///< Pointer to the D3D11 texture2D descriptor
    };
    BOOL EmulatedTyped64bitAtomics; ///< Emulated Typed 64bit Atomics
};

#endif // INTC_IGDEXT_D3D11

//////////////////////////////////////////////////////////////////////////
/// @brief D3D11 Graphics Extensions data structure definitions
//////////////////////////////////////////////////////////////////////////

#ifdef INTC_IGDEXT_D3D12

// Command Queue Throttle Policy
enum INTC_D3D12_COMMAND_QUEUE_THROTTLE_POLICY
{
    INTC_D3D12_COMMAND_QUEUE_THROTTLE_DYNAMIC         = 0,   ///< Dynamic
    INTC_D3D12_COMMAND_QUEUE_THROTTLE_MAX_PERFORMANCE = 255, ///< Max Performance
};

// Shader Input Type
enum INTC_D3D12_SHADER_INPUT_TYPE
{
    NONE        = 0, ///< None
    CM          = 1, ///< CM
    CM_SPIRV    = 2, ///< CM SPIRV
    OpenCL      = 3, ///< OpenCL
    SPIRV       = 4, ///< SPIRV
    HLSL        = 5, ///< HLSL
    CL_BIN      = 6, ///< CL BIN
    ESIMD_SPIRV = 7, ///< ESIMD SPIRV
    ZEBIN_ELF   = 8, ///< ZEBIN ELF
};

// Build Raytracing Acceleration Structure Descriptor Type
enum INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_TYPE
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EXT_INSTANCE_COMPARISON = 0, ///< Instance Comparison
};

// Intel D3D12 Features
enum INTC_D3D12_FEATURES
{
    INTC_D3D12_FEATURE_D3D12_OPTIONS1 = 0, ///< D3D12 Options 1
    INTC_D3D12_FEATURE_D3D12_OPTIONS2 = 1, ///< D3D12 Options 2
};

// Command Queue Descriptor
struct INTC_D3D12_COMMAND_QUEUE_DESC_0001
{
    union
    {
        D3D12_COMMAND_QUEUE_DESC* pD3D12Desc; ///< Pointer to the D3D12 command queue descriptor
    };
    INTC_D3D12_COMMAND_QUEUE_THROTTLE_POLICY CommandThrottlePolicy; ///< Command Queue Throttle Policy
};

// Command Queue Descriptor
typedef INTC_D3D12_COMMAND_QUEUE_DESC_0001 INTC_D3D12_COMMAND_QUEUE_DESC;

// Compute Pipeline State Descriptor
struct INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC
{
    union
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC* pD3D12Desc; ///< Pointer to the D3D12 compute pipeline state descriptor
    };
    D3D12_SHADER_BYTECODE        CS;              ///< Compute Shader Bytecode
    INTC_D3D12_SHADER_INPUT_TYPE ShaderInputType; ///< Input Shader Type
    void*                        CompileOptions;  ///< Compilation Options
    void*                        InternalOptions; ///< Internal CrossCompile Options
};

// Resource Descriptor
struct INTC_D3D12_RESOURCE_DESC
{
    union
    {
        D3D12_RESOURCE_DESC* pD3D12Desc; ///< Pointer to the D3D12 resource descriptor
    };
    BOOL Texture2DArrayMipPack; ///< Reserved Resources Texture2D Array with Mip Packing
};

// Resource Descriptor with Emulated Typed 64bit Atomics
struct INTC_D3D12_RESOURCE_DESC_0001 : INTC_D3D12_RESOURCE_DESC
{
    BOOL EmulatedTyped64bitAtomics; ///< Emulated Typed 64bit Atomics
};

// Resource Descriptor with Cpu Visible Video Memory
struct INTC_D3D12_RESOURCE_DESC_0002 : INTC_D3D12_RESOURCE_DESC_0001
{
    BOOL ResourceFlagCpuVisibleVideoMemory; ///< Cpu Visible Video Memory
};

// Heap Descriptor
struct INTC_D3D12_HEAP_DESC
{
    union
    {
        D3D12_HEAP_DESC* pD3D12Desc; ///< Pointer to the D3D12 heap descriptor
    };
    BOOL HeapFlagCpuVisibleVideoMemory; ///< Cpu Visible Video Memory
};

// Raytracing Pipeline State Object Descriptor
struct INTC_D3D12_STATE_OBJECT_DESC
{
    union
    {
        D3D12_STATE_OBJECT_DESC* pD3D12Desc; ///< Pointer to the D3D12 state object descriptor
    };
    D3D12_SHADER_BYTECODE* DXILLibrary;  ///< Raytracing Shader Byte code
    unsigned int           numLibraries; ///< Number of libraries
};

// Instance Comparison Data
struct INTC_D3D12_INSTANCE_COMPARISON_DATA
{
    UINT InstanceValue : 8;              ///< Instance Value
    UINT InstanceComparisonOperator : 8; ///< Instance Comparison Operator
};

// Build Raytracing Acceleration Structure Descriptor Instance Comparison Data
struct INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA
{
    void*                     pNext;                  ///< Pointer to next extension or 0
    UINT                      ExtType;                ///< Type of the extension
    D3D12_GPU_VIRTUAL_ADDRESS InstanceComparisonData; ///< GPU virtual address of a buffer that contains an array of INTC_D3D12_INSTANCE_COMPARISON_DATA
};

// Build Raytracing Acceleration Structure Descriptor
struct INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC
{
    union
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc; ///< Pointer to the D3D12 build raytracing acceleration structure descriptor
    };
    INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pCompDataDesc; ///< Pointer to the comparison data descriptor
};

// Get Raytracing Acceleration Structure Prebuild Info Descriptor
struct INTC_D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILDINFO_DESC
{
    union
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc; ///< Pointer to the D3D12 build raytracing acceleration structure inputs
    };
    INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pCompDataDesc; ///< Pointer to the comparison data descriptor
};

// Feature
struct INTC_D3D12_FEATURE
{
    BOOL EmulatedTyped64bitAtomics; ///< Emulated Typed 64bit Atomics
};

// Feature Data D3D12 Options 1
struct INTC_D3D12_FEATURE_DATA_D3D12_OPTIONS1
{
    BOOL XMXEnabled;                ///< XMX Enabled
    BOOL DLBoostEnabled;            ///< DL Boost Enabled
    BOOL EmulatedTyped64bitAtomics; ///< Emulated Typed 64bit Atomics
};

// Feature Data D3D12 Options 2
struct INTC_D3D12_FEATURE_DATA_D3D12_OPTIONS2
{
    BOOL SIMD16Required;            ///< SIMD16 Required
    BOOL LSCSupported;              ///< LSC Supported
    BOOL LegacyTranslationRequired; ///< Legacy Translation Required
};

// Latency Reduction Status
struct INTC_D3D12_LATENCY_REDUCTION_STATUS
{
    uint32_t driverSupportedVersion;     ///< Driver Supported Version
    BOOL     latencyReductionEnabled;    ///< Latency Reduction Enabled
    BOOL     renderSubmitTimingsEnabled; ///< Render Submit Timings Enabled
    uint32_t usedVersion;                ///< Used Version
};

#endif // INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief Graphics Extensions data structure definitions
//////////////////////////////////////////////////////////////////////////

// GPU Crash Flags
enum INTC_GPU_CRASH_FLAGS
{
    INTC_GPU_CRASH_FLAG_MARKERS           = 1, ///< Markers
    INTC_GPU_CRASH_FLAG_RESOURCE_TRACKING = 2, ///< Resource Tracking
    INTC_GPU_CRASH_FLAG_SHADER_DEBUG_INFO = 4, ///< Shader Debug Info
    INTC_GPU_CRASH_FLAG_CALL_STACK        = 8, ///< Call Stack
};

// Crashdump Status
enum INTC_CRASHDUMP_STATUS
{
    INTC_CRASHDUMP_STATUS_NOT_READY = 0, ///< Not Ready
    INTC_CRASHDUMP_STATUS_READY     = 1, ///< Ready
};

// Crashdump Section Type
enum INTC_CRASHDUMP_SECTION_TYPE
{
    INTC_CRASHDUMP_SECTION_TYPE_BLOB              = 1, ///< Crash Dump Blob
    INTC_CRASHDUMP_SECTION_TYPE_MARKERS           = 2, ///< Markers
    INTC_CRASHDUMP_SECTION_TYPE_PAGEFAULT         = 3, ///< Pagefault
    INTC_CRASHDUMP_SECTION_TYPE_SHADER_DEBUG_INFO = 4, ///< Shader Debug Info
    INTC_CRASHDUMP_SECTION_TYPE_CALL_STACK        = 5, ///< Call Stack
};

// Event Marker Event
enum INTC_EVENT_MARKER_EVENT
{
    INTC_EVENT_MARKER         = 1,  ///< Event Marker
    INTC_EVENT_MARKER_BEGIN   = 2,  ///< Event Marker Begin
    INTC_EVENT_MARKER_END     = 4,  ///< Event Marker End
    INTC_EVENT_MARKER_BUFFER  = 8,  ///< Event Marker Buffer
    INTC_EVENT_MARKER_STRING  = 16, ///< Event Marker String
    INTC_EVENT_MARKER_WSTRING = 32, ///< Event Marker WString
    INTC_EVENT_MARKER_PTR     = 64, ///< Event Marker Pointer
};

// GPU Crash Dump Callback
typedef void ( *PFNINTC_GPUCRASHDUMPCB )(
    const void*    pBuffer,     ///< Pointer to the buffer containing the crash dump data.
    const uint32_t bufferSize,  ///< Size of the buffer.
    void*          pPrivateData ///< Pointer to private data.
);

// Shader Debug Callback
typedef void ( *PFNINTC_SHADERDEBUGCB )(
    const void* pBuffer,     ///< Pointer to the buffer containing the shader debug data.
    uint32_t    bufferSize,  ///< Size of the buffer.
    void*       pPrivateData ///< Pointer to private data.
);

// Resolve Marker Callback
typedef void ( *PFNINTC_RESOLVEMARKERCB )(
    const void* pBuffer,            ///< Pointer to the buffer containing the marker data.
    uint32_t    bufferSize,         ///< Size of the buffer.
    void*       pPrivateData,       ///< Pointer to private data.
    void**      ppResolvedBuffer,   ///< Pointer to the resolved buffer.
    uint32_t*   pResolvedBufferSize ///< Pointer to the size of the resolved buffer.
);

// GPU Crash Dump callback functions
struct INTC_CALLBACKS
{
    PFNINTC_GPUCRASHDUMPCB  GPUCrashDumpCb;  ///< GPU Crash Dump callback
    PFNINTC_SHADERDEBUGCB   ShaderDebugCb;   ///< Shader Debug callback
    PFNINTC_RESOLVEMARKERCB ResolveMarkerCb; ///< Resolve Marker callback
};

// Crashdump Breadcrumb Data
struct INTC_CRASHDUMP_BREADCRUMB_DATA
{
    uint32_t eventType;   ///< Event type
    void*    buffer;      ///< Pointer to the buffer
    uint32_t bufferSize;  ///< Buffer size
    wchar_t* pMarkerName; ///< Marker name
    bool     completed;   ///< Flag indicating if the event is completed
};

// Crashdump Section
struct INTC_CRASHDUMP_SECTION
{
    uint32_t sectionType;    ///< Section type
    wchar_t* pSectionInfo;   ///< Section information
    uint32_t dataEntryCount; ///< Section size
    void*    pDataEntry;     ///< Pointer to the section data
};

// Crashdump Information
struct INTC_CRASHDUMP_INFO
{
    INTC_CRASHDUMP_STATUS   status;                ///< Crashdump status
    uint32_t                crashDumpSectionCount; ///< Crashdump section count
    INTC_CRASHDUMP_SECTION* pCrashDumpSections;    ///< Pointer to the crashdump sections
};

//////////////////////////////////////////////////////////////////////////
/// @brief D3D11 Graphics Extensions API function prototypes
//////////////////////////////////////////////////////////////////////////

#ifdef INTC_IGDEXT_D3D11

//////////////////////////////////////////////////////////////////////////
/// @brief BeginUAVOverlap marks the beginning point for disabling GPU synchronization between consecutive draws and dispatches that share UAV resources.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
HRESULT INTC_D3D11_BeginUAVOverlap(
    INTCExtensionContext* pExtensionContext );

//////////////////////////////////////////////////////////////////////////
/// @brief EndUAVOverlap marks the end point for disabling GPU synchronization between consecutive draws and dispatches that share UAV resources.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
HRESULT INTC_D3D11_EndUAVOverlap(
    INTCExtensionContext* pExtensionContext );

//////////////////////////////////////////////////////////////////////////
/// @brief MultiDrawInstancedIndirect function submits multiple DrawInstancedIndirect in one call.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDeviceContext A pointer to the device context that will be used to generate rendering commands.
/// @param drawCount The number of draws.
/// @param pBufferForArgs Pointer to the Arguments Buffer.
/// @param alignedByteOffsetForArgs Offset into the Arguments Buffer.
/// @param byteStrideForArgs The stride between elements in the Argument Buffer.
void INTC_D3D11_MultiDrawInstancedIndirect(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    UINT                  drawCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );

//////////////////////////////////////////////////////////////////////////
/// @brief MultiDrawIndexedInstancedIndirect function submits multiple DrawIndexedInstancedIndirect in one call.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDeviceContext A pointer to the device context that will be used to generate rendering commands.
/// @param drawCount The number of draws.
/// @param pBufferForArgs Pointer to the Arguments Buffer.
/// @param alignedByteOffsetForArgs Offset into the Arguments Buffer.
/// @param byteStrideForArgs The stride between elements in the Argument Buffer.
void INTC_D3D11_MultiDrawIndexedInstancedIndirect(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    UINT                  drawCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );

//////////////////////////////////////////////////////////////////////////
/// @brief MultiDrawInstancedIndirect function submits multiple DrawInstancedIndirect in one call. The number of draws are passed using Draw Count Buffer. It must be less or equal the Max Count argument.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDeviceContext A pointer to the device context that will be used to generate rendering commands.
/// @param pBufferForDrawCount Buffer that contains the number of draws.
/// @param alignedByteOffsetForDrawCount Offset into the Draw Count Buffer.
/// @param maxCount Maximum count of draws generated by this call.
/// @param pBufferForArgs Pointer to the Arguments Buffer.
/// @param alignedByteOffsetForArgs Offset into the Arguments Buffer.
/// @param byteStrideForArgs The stride between elements in the Argument Buffer.
void INTC_D3D11_MultiDrawInstancedIndirectCountIndirect(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    ID3D11Buffer*         pBufferForDrawCount,
    UINT                  alignedByteOffsetForDrawCount,
    UINT                  maxCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );

//////////////////////////////////////////////////////////////////////////
/// @brief MultiDrawIndexedInstancedIndirect function submits multiple DrawInstancedIndirect in one call. The number of draws are passed using Draw Count Buffer. It must be less or equal the Max Count argument.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDeviceContext A pointer to the device context that will be used to generate rendering commands.
/// @param pBufferForDrawCount Buffer that contains the number of draws.
/// @param alignedByteOffsetForDrawCount Offset into the Draw Count Buffer.
/// @param maxCount Maximum count of draws generated by this call.
/// @param pBufferForArgs Pointer to the Arguments Buffer.
/// @param alignedByteOffsetForArgs Offset into the Arguments Buffer.
/// @param byteStrideForArgs The stride between elements in the Argument Buffer.
void INTC_D3D11_MultiDrawIndexedInstancedIndirectCountIndirect(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    ID3D11Buffer*         pBufferForDrawCount,
    UINT                  alignedByteOffsetForDrawCount,
    UINT                  maxCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );

//////////////////////////////////////////////////////////////////////////
/// @brief SetDepthBounds method enables you to change the depth bounds dynamically.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param bEnable Enable or disable depth bounds test.
/// @param Min Specifies the minimum depth bounds. The default value is 0. NaN values silently convert to 0.
/// @param Max Specifies the maximum depth bounds. The default value is 1. NaN values silently convert to 0.
void INTC_D3D11_SetDepthBounds(
    INTCExtensionContext* pExtensionContext,
    BOOL                  bEnable,
    FLOAT                 Min,
    FLOAT                 Max );

//////////////////////////////////////////////////////////////////////////
/// @brief Create an array of 2D textures. Supported extensions: Emulated64bitTypedAtomics - Enable usage of 64bit Typed Atomics on a texture created.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc A pointer to a INTC_D3D11_TEXTURE2D_DESC structure that describes a 2D texture resource.
/// @param pInitialData A pointer to an array of D3D11_SUBRESOURCE_DATA structures that describe subresources for the 2D texture resource.
/// @param ppTexture2D A pointer to a buffer that receives a pointer to a ID3D11Texture2D interface for the created texture.
HRESULT INTC_D3D11_CreateTexture2D(
    INTCExtensionContext*            pExtensionContext,
    const INTC_D3D11_TEXTURE2D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA*    pInitialData,
    ID3D11Texture2D**                ppTexture2D );

#endif // INTC_IGDEXT_D3D11

//////////////////////////////////////////////////////////////////////////
/// @brief D3D12 Graphics Extensions API function prototypes
//////////////////////////////////////////////////////////////////////////

#ifdef INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a command queue.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc A pointer to a D3D12_COMPUTE_PIPELINE_STATE_DESC structure that describes compute pipeline state.
/// @param riid The globally unique identifier (GUID) for the command queue interface.
/// @param ppCommandQueue A pointer to a memory block that receives a pointer to the ID3D12CommandQueue interface for the command queue.
HRESULT INTC_D3D12_CreateCommandQueue(
    INTCExtensionContext*                pExtensionContext,
    const INTC_D3D12_COMMAND_QUEUE_DESC* pDesc,
    REFIID                               riid,
    void**                               ppCommandQueue );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a compute pipeline state object.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc A pointer to a D3D12_COMPUTE_PIPELINE_STATE_DESC structure that describes compute pipeline state.
/// @param riid The globally unique identifier (GUID) for the pipeline state interface.
/// @param ppPipelineState A pointer to a memory block that receives a pointer to the ID3D12PipelineState interface for the pipeline state object.
HRESULT INTC_D3D12_CreateComputePipelineState(
    INTCExtensionContext*                         pExtensionContext,
    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID                                        riid,
    void**                                        ppPipelineState );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a resource that is reserved, which is not yet mapped to any pages in a heap.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc A pointer to a overridden D3D12_RESOURCE_DESC structure that describes the resource.
/// @param InitialState The initial state of the resource, as a bitwise-OR'd combination of D3D12_RESOURCE_STATES enumeration constants.
/// @param pOptimizedClearValue Specifies a D3D12_CLEAR_VALUE that describes the default value for a clear color.
/// @param riid The globally unique identifier (GUID) for the resource interface.
/// @param ppvResource A pointer to a memory block that receives a pointer to the resource.
HRESULT INTC_D3D12_CreateReservedResource(
    INTCExtensionContext*           pExtensionContext,
    const INTC_D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES           InitialState,
    const D3D12_CLEAR_VALUE*        pOptimizedClearValue,
    REFIID                          riid,
    void**                          ppvResource );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates both a resource and an implicit heap, such that the heap is big enough to contain the entire resource and the resource is mapped to the heap.
/// @param pExtensionContext A pointer to a D3D12_HEAP_PROPERTIES structure that provides properties for the resource's heap.
/// @param pHeapProperties A pointer to the ID3D12Heap interface that represents the heap in which the resource is placed.
/// @param HeapFlags The offset, in bytes, to the resource.
/// @param pDesc A pointer to a overridden D3D12_RESOURCE_DESC structure that describes the resource.
/// @param InitialResourceState The initial state of the resource, as a bitwise-OR'd combination of D3D12_RESOURCE_STATES enumeration constants.
/// @param pOptimizedClearValue Specifies a D3D12_CLEAR_VALUE that describes the default value for a clear color.
/// @param riidResource The globally unique identifier (GUID) for the resource interface.
/// @param ppvResource A pointer to memory that receives the requested interface pointer to the created resource object.
HRESULT INTC_D3D12_CreateCommittedResource(
    INTCExtensionContext*                pExtensionContext,
    const D3D12_HEAP_PROPERTIES*         pHeapProperties,
    D3D12_HEAP_FLAGS                     HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES                InitialResourceState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riidResource,
    void**                               ppvResource );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates both a resource and an implicit heap, such that the heap is big enough to contain the entire resource and the resource is mapped to the heap.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pHeapProperties A pointer to the ID3D12Heap interface that represents the heap in which the resource is placed.
/// @param HeapFlags The offset, in bytes, to the resource.
/// @param pDesc A pointer to a overridden D3D12_RESOURCE_DESC structure that describes the resource.
/// @param InitialResourceState The initial state of the resource, as a bitwise-OR'd combination of D3D12_RESOURCE_STATES enumeration constants.
/// @param pOptimizedClearValue Specifies a D3D12_CLEAR_VALUE that describes the default value for a clear color.
/// @param riidResource The globally unique identifier (GUID) for the resource interface.
/// @param ppvResource A pointer to memory that receives the requested interface pointer to the created resource object.
HRESULT INTC_D3D12_CreateCommittedResource1(
    INTCExtensionContext*                pExtensionContext,
    const D3D12_HEAP_PROPERTIES*         pHeapProperties,
    D3D12_HEAP_FLAGS                     HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0002* pDesc,
    D3D12_RESOURCE_STATES                InitialResourceState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riidResource,
    void**                               ppvResource );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a heap.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc A pointer to a overridden D3D12_HEAP_DESC structure that describes the heap.
/// @param riid The globally unique identifier (GUID) for the heap interface.
/// @param ppvHeap A pointer to a memory block that receives a pointer to the heap.
HRESULT INTC_D3D12_CreateHeap(
    INTCExtensionContext*       pExtensionContext,
    const INTC_D3D12_HEAP_DESC* pDesc,
    REFIID                      riid,
    void**                      ppvHeap );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a resource that is placed in a specific heap.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pHeap A pointer to the ID3D12Heap interface that represents the heap in which the resource is placed.
/// @param HeapOffset The offset, in bytes, to the resource.
/// @param pDesc A pointer to a overridden D3D12_RESOURCE_DESC structure that describes the resource.
/// @param InitialState The initial state of the resource, as a bitwise-OR'd combination of D3D12_RESOURCE_STATES enumeration constants.
/// @param pOptimizedClearValue Specifies a D3D12_CLEAR_VALUE that describes the default value for a clear color.
/// @param riid The globally unique identifier (GUID) for the resource interface.
/// @param ppvResource A pointer to a memory block that receives a pointer to the resource.
HRESULT INTC_D3D12_CreatePlacedResource(
    INTCExtensionContext*                pExtensionContext,
    ID3D12Heap*                          pHeap,
    UINT64                               HeapOffset,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES                InitialState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riid,
    void**                               ppvResource );

//////////////////////////////////////////////////////////////////////////
/// @brief Constructs a buffer resource suitable for storing raytracing acceleration structures build on the host.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param SizeInBytes Size of the resource, in bytes.
/// @param Flags Reserved, must be zero.
/// @param riidResource Interface id for the returned resource.
/// @param ppvResource Pointer to a pointer which is set to the output resource.
HRESULT INTC_D3D12_CreateHostRTASResource(
    INTCExtensionContext* pExtensionContext,
    size_t                SizeInBytes,
    DWORD                 Flags,
    REFIID                riidResource,
    void**                ppvResource );

//////////////////////////////////////////////////////////////////////////
/// @brief Builds an acceleration structure on the host timeline.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc An acceleration structure description. All GPUVA fields contained in the description are interpreted as host pointers cast to uintptr_t.
/// @param pInstanceGPUVAs For a top-level AS, this contains the GPUVA of the bottom-level AS for each instance which will be used for ray traversal.
/// @param NumInstances Number of addresses in 'pInstanceGPUVAs'.
void INTC_D3D12_BuildRaytracingAccelerationStructure_Host(
    INTCExtensionContext*                                     pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    const D3D12_GPU_VIRTUAL_ADDRESS*                          pInstanceGPUVAs,
    UINT                                                      NumInstances );

//////////////////////////////////////////////////////////////////////////
/// @brief Copies an acceleration structure on the host timeline.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param DestAccelerationStructureData Pointer to destination AS. Must point into a mapped Host RTAS resource.
/// @param SourceAccelerationStructureData Pointer to source AS. Must point into a mapped Host RTAS resource.
/// @param Mode See DXR spec.
void INTC_D3D12_CopyRaytracingAccelerationStructure_Host(
    INTCExtensionContext*                             pExtensionContext,
    void*                                             DestAccelerationStructureData,
    const void*                                       SourceAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode );

//////////////////////////////////////////////////////////////////////////
/// @brief Retrieves post-build info for a host-built AS on the host timeline.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param InfoType See DXR spec.
/// @param DestBuffer Pointer to output structure (see DXR spec).
/// @param SourceRTAS Pointer to source RTAS. Must lie within a Host rtas resource.
void INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_Host(
    INTCExtensionContext*                                       pExtensionContext,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE InfoType,
    void*                                                       DestBuffer,
    const void*                                                 SourceRTAS );

//////////////////////////////////////////////////////////////////////////
/// @brief Retrieves pre-build info for a host-built AS.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc See DXR spec.
/// @param pInfo See DXR spec. This function gives the AS and scratch sizes for a host-built AS, which might not match the device-built equivalents.
void INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_Host(
    INTCExtensionContext*                                       pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*      pInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief Copy host-built acceleration structures to/from device memory on the GPU timeline.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pCommandList A command list to receive copy commands.
/// @param DestAccelerationStructureData GPU address of destination.
/// @param SrcAccelerationStructureData GPU address of source.
/// @param Mode See DXR spec. At least one of Dest and Src must be a Host RTAS resource. The other must be a conventional DXR acceleration structure buffer.
void INTC_D3D12_TransferHostRTAS(
    INTCExtensionContext*                             pExtensionContext,
    ID3D12GraphicsCommandList*                        pCommandList,
    D3D12_GPU_VIRTUAL_ADDRESS                         DestAccelerationStructureData,
    D3D12_GPU_VIRTUAL_ADDRESS                         SrcAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode );

//////////////////////////////////////////////////////////////////////////
/// @brief Set Metadata associated with the CommandList.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pCommandList A command list to set metadata.
/// @param Metadata Metadata.
void INTC_D3D12_SetDriverEventMetadata(
    INTCExtensionContext*      pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    UINT64                     Metadata );

//////////////////////////////////////////////////////////////////////////
/// @brief Queries total number of bytes and number of free bytes in CPU visible local memory.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pTotalBytes A pointer to total number of bytes in CPU visible vidmem.
/// @param pFreeBytes A pointer to number of free bytes in CPU visible vidmem.
void INTC_D3D12_QueryCpuVisibleVidmem(
    INTCExtensionContext* pExtensionContext,
    UINT64*               pTotalBytes,
    UINT64*               pFreeBytes );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates an ID3D12StateObject.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc The description of the state object to create.
/// @param riid The GUID of the interface to create.
/// @param ppPipelineState The returned Raytracing pipeline state object.
HRESULT INTC_D3D12_CreateStateObject(
    INTCExtensionContext*               pExtensionContext,
    const INTC_D3D12_STATE_OBJECT_DESC* pDesc,
    REFIID                              riid,
    void**                              ppPipelineState );

//////////////////////////////////////////////////////////////////////////
/// @brief Performs a raytracing acceleration structure build on the GPU.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pCommandList A command list to build acceleration structure.
/// @param pDesc Description of the acceleration structure to build.
/// @param NumPostbuildInfoDescs Size of the pPostbuildInfoDescs array.
/// @param pPostbuildInfoDescs Optional array of descriptions for post-build info to generate describing properties of the acceleration structure that was built.
/// @param pComparisonDataDesc Description of the comparison data.
void INTC_D3D12_BuildRaytracingAccelerationStructure(
    INTCExtensionContext*                                                                   pExtensionContext,
    ID3D12GraphicsCommandList*                                                              pCommandList,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*                               pDesc,
    UINT                                                                                    NumPostbuildInfoDescs,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*                      pPostbuildInfoDescs,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pComparisonDataDesc );

//////////////////////////////////////////////////////////////////////////
/// @brief Query the driver for resource requirements to build an acceleration structure.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pDesc Description of the acceleration structure build.
/// @param pInfo The result of the query.
/// @param pComparisonDataDesc Description of the comparison data.
void INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo(
    INTCExtensionContext*                                                                   pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*                             pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*                                  pInfo,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pComparisonDataDesc );

//////////////////////////////////////////////////////////////////////////
/// @brief Sets feature support.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pFeature Pointer to the feature to be set.
HRESULT INTC_D3D12_SetFeatureSupport(
    INTCExtensionContext* pExtensionContext,
    INTC_D3D12_FEATURE*   pFeature );

//////////////////////////////////////////////////////////////////////////
/// @brief Gets the size and alignment of memory required for a collection of resources on this adapter.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param visibleMask For single-GPU operation, set this to zero. If there are multiple GPU nodes, set bits to identify the nodes (the device's physical adapters). Each bit in the mask corresponds to a single node.
/// @param numResourceDescs The number of resource descriptors in the pResourceDescs array.
/// @param pResourceDescs A pointer to a overridden D3D12_RESOURCE_DESC structure that describes the resource.
D3D12_RESOURCE_ALLOCATION_INFO INTC_D3D12_GetResourceAllocationInfo(
    INTCExtensionContext*                pExtensionContext,
    UINT                                 visibleMask,
    UINT                                 numResourceDescs,
    const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs );

//////////////////////////////////////////////////////////////////////////
/// @brief Gets information about the features that are supported by the current graphics driver.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param Feature A constant from the D3D12_FEATURES enumeration describing the feature(s) that you want to query for support.
/// @param pFeatureSupportData A pointer to a data structure that corresponds to the value of the Feature parameter. To determine the corresponding data structure for each constant, see INTC_D3D12_FEATURES.
/// @param FeatureSupportDataSize The size of the structure pointed to by the pFeatureSupportData parameter.
HRESULT INTC_D3D12_CheckFeatureSupport(
    INTCExtensionContext* pExtensionContext,
    INTC_D3D12_FEATURES   Feature,
    void*                 pFeatureSupportData,
    UINT                  FeatureSupportDataSize );

//////////////////////////////////////////////////////////////////////////
/// @brief Sets a custom shader binary file path.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param filePath Path to a pre-built shader binary file.
HRESULT INTC_D3D12_AddShaderBinariesPath(
    INTCExtensionContext* pExtensionContext,
    const wchar_t*        filePath );

//////////////////////////////////////////////////////////////////////////
/// @brief Removes a custom shader binary file path.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param filePath Path to a pre-built shader binary file.
HRESULT INTC_D3D12_RemoveShaderBinariesPath(
    INTCExtensionContext* pExtensionContext,
    const wchar_t*        filePath );

//////////////////////////////////////////////////////////////////////////
/// @brief Register Application Info in the graphics driver.
/// @param pExtensionAppInfo Application Info to be passed to the driver.
HRESULT INTC_D3D12_SetApplicationInfo(
    INTCExtensionAppInfo1* pExtensionAppInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Sets NumFrames which notifies Display how many generated frames should be paced during next Present calls.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param NumFrames Number of generated frames that Display should pace.
HRESULT INTC_D3D12_SetNumGeneratedFrames(
    INTCExtensionContext* pExtensionContext,
    UINT                  NumFrames );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Sets PresentSequenceNumber which allows application and driver to establish mapping between present calls and entries returned by INTC_D3D12_GetDisplayTelemetry.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param PresentSequenceNumber Next Present's sequence number passed from an application to the driver.
HRESULT INTC_D3D12_SetPresentSequenceNumber(
    INTCExtensionContext* pExtensionContext,
    UINT                  PresentSequenceNumber );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Queries the driver for the current display telemetry data. The telemetry contains timing information about previous Present calls, presentation mode used and future vsync interval information.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pTelemetryData A pointer to memory allocated by application where the driver will write the telemetry data.
/// @param size Size of memory allocated by the caller for telemetry data.
HRESULT INTC_D3D12_GetDisplayTelemetry(
    INTCExtensionContext* pExtensionContext,
    void*                 pTelemetryData,
    UINT                  size );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Query Latency Reduction status.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pLatencyReductionStatus A pointer to INTC_D3D12_LATENCY_REDUCTION_STATUS structure.
HRESULT INTC_D3D12_GetLatencyReductionStatus(
    INTCExtensionContext*                pExtensionContext,
    INTC_D3D12_LATENCY_REDUCTION_STATUS* pLatencyReductionStatus );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Control Latency Reduction.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param version Requested Latency Reduction version.
/// @param latencyReductionEnabled Enable or disable Latency Reduction.
/// @param renderSubmitTimingsEnabled Enable or disable Render Submit Timings.
/// @param timingSlots Number of timing slots in the buffers.
HRESULT INTC_D3D12_LatencyReductionExt(
    INTCExtensionContext* pExtensionContext,
    uint32_t              version,
    BOOL                  latencyReductionEnabled,
    BOOL                  renderSubmitTimingsEnabled,
    uint32_t              timingSlots );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Retrieves buffers for render submit timings.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param ppRenderSubmitCpuTimings A pointer to pointer to the buffer that will receive CPU timings.
/// @param ppRenderSubmitGpuTimings A pointer to pointer to the buffer that will receive GPU timings.
HRESULT INTC_D3D12_LatencyReductionGetRenderSubmitTimingsBuffers(
    INTCExtensionContext* pExtensionContext,
    void**                ppRenderSubmitCpuTimings,
    void**                ppRenderSubmitGpuTimings );

//////////////////////////////////////////////////////////////////////////
/// @brief Internal use - Starts render submit detection.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param frameId Frame ID used in the current Render Submit detection.
HRESULT INTC_D3D12_RenderSubmitStart(
    INTCExtensionContext* pExtensionContext,
    uint32_t              frameId );

//////////////////////////////////////////////////////////////////////////
/// @brief Obtains an Graphics Extensions CommandListHandle associated with the D3D12 CommandList Object.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pCommandList Pointer to the D3D12 CommandList Object.
uint64_t INTC_D3D12_GetCommandListHandle(
    INTCExtensionContext* pExtensionContext,
    void*                 pCommandList );

//////////////////////////////////////////////////////////////////////////
/// @brief Places a marker in the Command List. Markers (breadcrumbs) are then decoded in the GPU crash dump.
/// @param pExtensionContext A pointer to the extension context associated with the current Device.
/// @param pCommandList A command list to set the event marker.
/// @param eventType The type of the event marker.
/// @param marker The marker data.
/// @param markerSize The size of the marker data.
void INTC_D3D12_SetEventMarker(
    INTCExtensionContext*      pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    uint32_t                   eventType,
    const void*                marker,
    uint32_t                   markerSize );

#endif // INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief Enables Intel Breadcrumbs GPU crash dumps - should be called *before* device creation.
/// @param flags Flags to enable the crash dump.
/// @param callbacks Callbacks to be used for the crash dump.
/// @param pPrivateData Private data that can be associated with the crash dump.
HRESULT INTC_EnableGpuCrashDumps(
    INTC_GPU_CRASH_FLAGS  flags,
    const INTC_CALLBACKS& callbacks,
    void*                 pPrivateData );

//////////////////////////////////////////////////////////////////////////
/// @brief Used to determine if application GPU crash dumps are enabled.
BOOL INTC_IsGpuCrashDumpsEnabled();

//////////////////////////////////////////////////////////////////////////
/// @brief Disables Intel Breadcrumbs GPU crash dumps.
void INTC_DisableGpuCrashDumps();

//////////////////////////////////////////////////////////////////////////
/// @brief Checks the status of the GPU Crash Dump.
/// @param Status The status of the GPU Crash Dump.
HRESULT INTC_GetGpuCrashDump(
    INTC_CRASHDUMP_STATUS& Status );

//////////////////////////////////////////////////////////////////////////
/// @brief Decodes the GPU Crash Dump and generates a text with the crash dump information.
/// @param crashDump The string buffer that receives GPU Crash Dump decoded text.
/// @param crashDumpSize The size of the crash dump. If crashDump is NULL, this will return the size of the crash dump string required.
HRESULT INTC_DecodeGpuCrashDump(
    wchar_t*  crashDump,
    uint32_t& crashDumpSize );

//////////////////////////////////////////////////////////////////////////
/// @brief Retrieves the GPU Crash Dump information in a structured format for further processing.
/// @param pCrashDumpInfo Pointer to the INTC_CRASHDUMP_INFO structure to be filled with the crash dump information.
HRESULT INTC_RetrieveGpuCrashDump(
    INTC_CRASHDUMP_INFO*& pCrashDumpInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief Loads the binary GPU Crash Dump data.
/// @param crashDump The raw buffer that receives GPU Crash Dump data.
/// @param crashDumpSize The size of the crash dump. If crashDump is NULL, this will return the size of the crash dump buffer required.
HRESULT INTC_LoadGpuCrashDump(
    uint8_t* crashDump,
    uint32_t crashDumpSize );

//////////////////////////////////////////////////////////////////////////
/// @brief D3D11 Graphics Extensions Framework extension function prototypes
//////////////////////////////////////////////////////////////////////////

#ifdef INTC_IGDEXT_D3D11

//////////////////////////////////////////////////////////////////////////
/// @brief Returns all Graphics Extensions interface versions supported on a current platform/driver/header file combination using D3D11 Device.
/// @param pDevice A pointer to the current D3D11 Device.
/// @param pSupportedExtVersions A pointer to the table of supported versions.
/// @param pSupportedExtVersionsCount A pointer to the variable that will hold the number of supported versions. Pointer is null if Init fails.
HRESULT INTC_D3D11_GetSupportedVersions(
    const ID3D11Device*   pDevice,
    INTCExtensionVersion* pSupportedExtVersions,
    uint32_t*             pSupportedExtVersionsCount );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates D3D11 Graphics Extensions Device Context and returns ppfnExtensionContext Extension Context object and ppfnExtensionFuncs extension function pointers table. This function must be called prior to using extensions.
/// @param pDevice A pointer to the current Device.
/// @param ppExtensionContext A pointer to a pointer to the extension context associated with the current Device.
/// @param pExtensionInfo A pointer to the ExtensionInfo structure. The requestedExtensionVersion member must be set prior to calling this function. The remaining members are filled in with device info about the Intel GPU and info about the graphics driver version.
/// @param pExtensionAppInfo A pointer to the ExtensionAppInfo structure that can be optionally passed to the driver identifying application and engine.
HRESULT INTC_D3D11_CreateDeviceExtensionContext(
    const ID3D11Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo*  pExtensionAppInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a device extension context for Direct3D 11.
/// @param pDevice A pointer to the Direct3D 11 device.
/// @param ppExtensionContext A pointer to a memory block that receives a pointer to the extension context.
/// @param pExtensionInfo A pointer to the extension information.
/// @param pExtensionAppInfo A pointer to the application-specific extension information.
HRESULT INTC_D3D11_CreateDeviceExtensionContext1(
    const ID3D11Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief INTC_D3D11_RegisterApplicationCallbacks function sets the callback table for D3D11 API handlers. All registered callbacks will be called when the corresponding original API is called.
/// @param pCallbacks A pointer to a INTC_D3D12_API_CALLBACKS structure that registers the callbacks to D3D12 API handlers.
HRESULT INTC_D3D11_RegisterApplicationCallbacks(
    const INTC_D3D11_API_CALLBACKS* pCallbacks );

#endif // INTC_IGDEXT_D3D11

//////////////////////////////////////////////////////////////////////////
/// @brief D3D12 Graphics Extensions Framework extension function prototypes
//////////////////////////////////////////////////////////////////////////

#ifdef INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief Returns all Graphics Extensions interface versions supported on a current platform/driver/header file combination using D3D12 Device.
/// @param pDevice A pointer to the current D3D11 Device.
/// @param pSupportedExtVersions A pointer to the table of supported versions.
/// @param pSupportedExtVersionsCount A pointer to the variable that will hold the number of supported versions. Pointer is null if Init fails.
HRESULT INTC_D3D12_GetSupportedVersions(
    const ID3D12Device*   pDevice,
    INTCExtensionVersion* pSupportedExtVersions,
    uint32_t*             pSupportedExtVersionsCount );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates D3D12 Graphics Extensions Device Context and returns ppfnExtensionContext Extension Context object and ppfnExtensionFuncs extension function pointers table. This function must be called prior to using extensions.
/// @param pDevice A pointer to the current Device.
/// @param ppExtensionContext A pointer to a pointer to the extension context associated with the current Device.
/// @param pExtensionInfo A pointer to the ExtensionInfo structure. The requestedExtensionVersion member must be set prior to calling this function. The remaining members are filled in with device info about the Intel GPU and info about the graphics driver version.
/// @param pExtensionAppInfo A pointer to the ExtensionAppInfo structure that can be optionally passed to the driver identifying application and engine.
HRESULT INTC_D3D12_CreateDeviceExtensionContext(
    const ID3D12Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo*  pExtensionAppInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates D3D12 Graphics Extensions Device Context and returns ppfnExtensionContext Extension Context object and ppfnExtensionFuncs extension function pointers table. This function must be called prior to using extensions.
/// @param pDevice A pointer to the current Device.
/// @param ppExtensionContext A pointer to a pointer to the extension context associated with the current Device.
/// @param pExtensionInfo A pointer to the ExtensionInfo structure. The requestedExtensionVersion member must be set prior to calling this function. The remaining members are filled in with device info about the Intel GPU and info about the graphics driver version.
/// @param pExtensionAppInfo A pointer to the ExtensionAppInfo1 structure that can optionally be used in the driver to identify workload.
HRESULT INTC_D3D12_CreateDeviceExtensionContext1(
    const ID3D12Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief INTC_D3D12_RegisterApplicationCallbacks function sets the callback table for D3D12 API handlers. All registered callbacks will be called when the corresponding original API is called.
/// @param pCallbacks A pointer to a INTC_D3D12_API_CALLBACKS structure that registers the callbacks to D3D12 API handlers.
HRESULT INTC_D3D12_RegisterApplicationCallbacks(
    const INTC_D3D12_API_CALLBACKS* pCallbacks );

#endif // INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief Graphics Extensions Framework extension function prototypes
//////////////////////////////////////////////////////////////////////////

#if defined( INTC_IGDEXT_D3D11 ) && defined( INTC_IGDEXT_D3D12 )

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a D3D Graphics Extensions Device Context and returns ppExtensionContext Extension Context object. This function must be called prior to using extensions when creating an extension context with a D3D11 and D3D12 device.
/// @param pD3D11Device A pointer to the current D3D11 device.
/// @param pD3D12Device A pointer to the current D3D12 device.
/// @param ppExtensionContext A pointer to a pointer to the extension context associated with the current Device.
/// @param pExtensionInfo A pointer to the ExtensionInfo structure. The requestedExtensionVersion member must be set prior to calling this function. The remaining members are filled in with device info about the Intel GPU and info about the graphics driver version.
/// @param pExtensionAppInfo A pointer to the ExtensionAppInfo structure that can be optionally passed to the driver identifying application and engine.
HRESULT INTC_CreateDeviceExtensionContext(
    const ID3D11Device*    pD3D11Device,
    const ID3D12Device*    pD3D12Device,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo*  pExtensionAppInfo );

//////////////////////////////////////////////////////////////////////////
/// @brief Creates a device extension context for DX12.
/// @param pD3D11Device Pointer to the D3D11 device.
/// @param pD3D12Device Pointer to the D3D12 device.
/// @param ppExtensionContext Pointer to the extension context.
/// @param pExtensionInfo Pointer to the extension information.
/// @param pExtensionAppInfo Pointer to the extension application information.
HRESULT INTC_CreateDeviceExtensionContext1(
    ID3D11Device*          pD3D11Device,
    ID3D12Device*          pD3D12Device,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo );

#endif // INTC_IGDEXT_D3D11 && INTC_IGDEXT_D3D12

//////////////////////////////////////////////////////////////////////////
/// @brief Extension library loading helper function.
///        Function helps load Graphics Extensions Framework module into the currently executing process.
///        If useCurrentProcessDir is set, the function tries to load the library from the current
///        process directory first. If that was unsuccessful or useCurrentProcessDir was not set,
///        it tries to find the full path to the Intel Graphics Driver module that must be loaded
///        by the current process. Library is loaded from the same path (whether it is DriverStore
///        location or system32 folder).
/// @param useCurrentProcessDir If true, this function attempts to load the Extensions Framework DLL
///       from the current process directory. If false, this function attempts to load the Extensions
///       Framework DLL from the installed graphics driver directory."
/// @param VendorID The Vendor ID of the graphics adapter.
///       NOTE: This function determines the path to the installed Intel graphics driver directory using
///       Intel's D3D11 or D3D12 user mode driver DLL, which is expected to already be loaded by the
///       current process. If this function is called before one of those DLLs is loaded, then the
///       Vendor and Device ID need to be supplied.
/// @param DeviceID The Device ID of the graphics adapter.
///       NOTE: This function determines the path to the installed Intel graphics driver directory using
///       Intel's D3D11 or D3D12 user mode driver DLL, which is expected to already be loaded by the
///       current process. If this function is called before one of those DLLs is loaded, then the
///       Vendor and Device ID need to be supplied.
HRESULT INTC_LoadExtensionsLibrary(
    bool   useCurrentProcessDir = false,
    size_t VendorID             = 0,
    size_t DeviceID             = 0 );

//////////////////////////////////////////////////////////////////////////
/// @brief Extension library unloading helper function.
void INTC_UnloadExtensionsLibrary();

//////////////////////////////////////////////////////////////////////////
/// @brief Destroys D3D12 Graphics Extensions Device Context and provides cleanup for the Graphics Extensions Framework. No D3D12 extensions can be used after calling this function.
/// @param ppExtensionContext A pointer to a pointer to the extension context associated with the current Device.
HRESULT INTC_DestroyDeviceExtensionContext(
    INTCExtensionContext** ppExtensionContext );

#ifdef __cplusplus
} // extern "C"
#endif

#pragma warning( pop )

#endif // _IGDEXTAPI_H_
