
/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 *
 * File Name:  igdext_tools.h
 *
 * Abstract:   Intel(R) Graphics Extensions SDK Tools Support Header File
 *
 * Notes:      This file is intended to be used by capture and replay tools. It allows
 *             the tool to interpose the Graphics Extensions API functions without the
 *             need for DLL hooks.
 */

#pragma once

#ifdef INTC_IGDEXT_D3D11

typedef HRESULT ( *PFNINTCDX11EXT_GETSUPPORTEDVERSIONS )(
    const ID3D11Device*   pDevice,
    INTCExtensionVersion* pSupportedExtVersions,
    uint32_t*             pSupportedExtVersionsCount );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_GETSUPPORTEDVERSIONS )(
    PFNINTCDX11EXT_GETSUPPORTEDVERSIONS pfnGetSupportedVersions,
    const ID3D11Device*                 pDevice,
    INTCExtensionVersion*               pSupportedExtVersions,
    uint32_t*                           pSupportedExtVersionsCount );

typedef HRESULT ( *PFNINTCDX11EXT_CREATEDEVICEEXTENSIONCONTEXT )(
    const ID3D11Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo*  pExtensionAppInfo );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT )(
    PFNINTCDX11EXT_CREATEDEVICEEXTENSIONCONTEXT pfnCreateDeviceExtensionContext,
    const ID3D11Device*                         pDevice,
    INTCExtensionContext**                      ppExtensionContext,
    INTCExtensionInfo*                          pExtensionInfo,
    INTCExtensionAppInfo*                       pExtensionAppInfo );

typedef HRESULT ( *PFNINTCDX11EXT_CREATEDEVICEEXTENSIONCONTEXT1 )(
    const ID3D11Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT1 )(
    PFNINTCDX11EXT_CREATEDEVICEEXTENSIONCONTEXT1 pfnCreateDeviceExtensionContext1,
    const ID3D11Device*                          pDevice,
    INTCExtensionContext**                       ppExtensionContext,
    INTCExtensionInfo*                           pExtensionInfo,
    INTCExtensionAppInfo1*                       pExtensionAppInfo );

typedef HRESULT ( *PFNINTCDX11EXT_BEGINUAVOVERLAP )(
    INTCExtensionContext* pExtensionContext );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_BEGINUAVOVERLAP )(
    PFNINTCDX11EXT_BEGINUAVOVERLAP pfnBeginUAVOverlap,
    INTCExtensionContext*          pExtensionContext );

typedef HRESULT ( *PFNINTCDX11EXT_ENDUAVOVERLAP )(
    INTCExtensionContext* pExtensionContext );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_ENDUAVOVERLAP )(
    PFNINTCDX11EXT_ENDUAVOVERLAP pfnEndUAVOverlap,
    INTCExtensionContext*        pExtensionContext );

typedef HRESULT ( *PFNINTCDX11EXT_BEGIN_RETRIEVE_RESOURCE_HANDLE )(
    INTCExtensionContext* pExtensionContext );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_BEGIN_RETRIEVE_RESOURCE_HANDLE )(
    PFNINTCDX11EXT_BEGIN_RETRIEVE_RESOURCE_HANDLE pfnBegin_Retrieve_Resource_Handle,
    INTCExtensionContext*                         pExtensionContext );

typedef HRESULT ( *PFNINTCDX11EXT_END_RETRIEVE_RESOURCE_HANDLE )(
    INTCExtensionContext* pExtensionContext );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_END_RETRIEVE_RESOURCE_HANDLE )(
    PFNINTCDX11EXT_END_RETRIEVE_RESOURCE_HANDLE pfnEnd_Retrieve_Resource_Handle,
    INTCExtensionContext*                       pExtensionContext );

typedef void ( *PFNINTCDX11EXT_MULTIDRAWINSTANCEDINDIRECT )(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    UINT                  drawCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );
typedef void ( *PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINSTANCEDINDIRECT )(
    PFNINTCDX11EXT_MULTIDRAWINSTANCEDINDIRECT pfnMultiDrawInstancedIndirect,
    INTCExtensionContext*                     pExtensionContext,
    ID3D11DeviceContext*                      pDeviceContext,
    UINT                                      drawCount,
    ID3D11Buffer*                             pBufferForArgs,
    UINT                                      alignedByteOffsetForArgs,
    UINT                                      byteStrideForArgs );

typedef void ( *PFNINTCDX11EXT_MULTIDRAWINDEXEDINSTANCEDINDIRECT )(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    UINT                  drawCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );
typedef void ( *PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINDEXEDINSTANCEDINDIRECT )(
    PFNINTCDX11EXT_MULTIDRAWINDEXEDINSTANCEDINDIRECT pfnMultiDrawIndexedInstancedIndirect,
    INTCExtensionContext*                            pExtensionContext,
    ID3D11DeviceContext*                             pDeviceContext,
    UINT                                             drawCount,
    ID3D11Buffer*                                    pBufferForArgs,
    UINT                                             alignedByteOffsetForArgs,
    UINT                                             byteStrideForArgs );

typedef void ( *PFNINTCDX11EXT_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT )(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    ID3D11Buffer*         pBufferForDrawCount,
    UINT                  alignedByteOffsetForDrawCount,
    UINT                  maxCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );
typedef void ( *PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT )(
    PFNINTCDX11EXT_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT pfnMultiDrawInstancedIndirectCountIndirect,
    INTCExtensionContext*                                  pExtensionContext,
    ID3D11DeviceContext*                                   pDeviceContext,
    ID3D11Buffer*                                          pBufferForDrawCount,
    UINT                                                   alignedByteOffsetForDrawCount,
    UINT                                                   maxCount,
    ID3D11Buffer*                                          pBufferForArgs,
    UINT                                                   alignedByteOffsetForArgs,
    UINT                                                   byteStrideForArgs );

typedef void ( *PFNINTCDX11EXT_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT )(
    INTCExtensionContext* pExtensionContext,
    ID3D11DeviceContext*  pDeviceContext,
    ID3D11Buffer*         pBufferForDrawCount,
    UINT                  alignedByteOffsetForDrawCount,
    UINT                  maxCount,
    ID3D11Buffer*         pBufferForArgs,
    UINT                  alignedByteOffsetForArgs,
    UINT                  byteStrideForArgs );
typedef void ( *PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT )(
    PFNINTCDX11EXT_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT pfnMultiDrawIndexedInstancedIndirectCountIndirect,
    INTCExtensionContext*                                         pExtensionContext,
    ID3D11DeviceContext*                                          pDeviceContext,
    ID3D11Buffer*                                                 pBufferForDrawCount,
    UINT                                                          alignedByteOffsetForDrawCount,
    UINT                                                          maxCount,
    ID3D11Buffer*                                                 pBufferForArgs,
    UINT                                                          alignedByteOffsetForArgs,
    UINT                                                          byteStrideForArgs );

typedef void ( *PFNINTCDX11EXT_SETDEPTHBOUNDS )(
    INTCExtensionContext* pExtensionContext,
    BOOL                  bEnable,
    FLOAT                 Min,
    FLOAT                 Max );
typedef void ( *PFNINTCDX11EXT_API_CALLBACK_SETDEPTHBOUNDS )(
    PFNINTCDX11EXT_SETDEPTHBOUNDS pfnSetDepthBounds,
    INTCExtensionContext*         pExtensionContext,
    BOOL                          bEnable,
    FLOAT                         Min,
    FLOAT                         Max );

typedef HRESULT ( *PFNINTCDX11EXT_CREATETEXTURE2D )(
    INTCExtensionContext*            pExtensionContext,
    const INTC_D3D11_TEXTURE2D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA*    pInitialData,
    ID3D11Texture2D**                ppTexture2D );
typedef HRESULT ( *PFNINTCDX11EXT_API_CALLBACK_CREATETEXTURE2D )(
    PFNINTCDX11EXT_CREATETEXTURE2D   pfnCreateTexture2D,
    INTCExtensionContext*            pExtensionContext,
    const INTC_D3D11_TEXTURE2D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA*    pInitialData,
    ID3D11Texture2D**                ppTexture2D );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSCREATECONTEXT )(
    INTCExtensionContext*           pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE* phContext );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSCREATECONTEXT )(
    PFNINTCDX11EXT_XESSCREATECONTEXT pfnXessCreateContext,
    INTCExtensionContext*            pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE*  phContext );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSDESTROYCONTEXT )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSDESTROYCONTEXT )(
    PFNINTCDX11EXT_XESSDESTROYCONTEXT pfnXessDestroyContext,
    INTCExtensionContext*             pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE    hContext );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSINIT )(
    INTCExtensionContext*              pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE     hContext,
    const INTC_D3D11_XESS_INIT_PARAMS* pInitParams );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSINIT )(
    PFNINTCDX11EXT_XESSINIT            pfnXessInit,
    INTCExtensionContext*              pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE     hContext,
    const INTC_D3D11_XESS_INIT_PARAMS* pInitParams );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETINITPARAMS )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    INTC_D3D11_XESS_INIT_PARAMS*   pInitParams );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETINITPARAMS )(
    PFNINTCDX11EXT_XESSGETINITPARAMS pfnXessGetInitParams,
    INTCExtensionContext*            pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE   hContext,
    INTC_D3D11_XESS_INIT_PARAMS*     pInitParams );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSEXECUTE )(
    INTCExtensionContext*                 pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE        hContext,
    const INTC_D3D11_XESS_EXECUTE_PARAMS* pExecParams );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSEXECUTE )(
    PFNINTCDX11EXT_XESSEXECUTE            pfnXessExecute,
    INTCExtensionContext*                 pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE        hContext,
    const INTC_D3D11_XESS_EXECUTE_PARAMS* pExecParams );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETVERSION )(
    INTCExtensionContext*    pExtensionContext,
    INTC_D3D11_XESS_VERSION* pVersion );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETVERSION )(
    PFNINTCDX11EXT_XESSGETVERSION pfnXessGetVersion,
    INTCExtensionContext*         pExtensionContext,
    INTC_D3D11_XESS_VERSION*      pVersion );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETINTELXEFXVERSION )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    INTC_D3D11_XESS_VERSION*       pVersion );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETINTELXEFXVERSION )(
    PFNINTCDX11EXT_XESSGETINTELXEFXVERSION pfnXessGetIntelXeFXVersion,
    INTCExtensionContext*                  pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE         hContext,
    INTC_D3D11_XESS_VERSION*               pVersion );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETINPUTRESOLUTION )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    const INTC_D3D11_XESS_2D*      pOutputResolution,
    INTC_D3D11_QUALITY_SETTINGS    qualitySettings,
    INTC_D3D11_XESS_2D*            pInputResolution );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETINPUTRESOLUTION )(
    PFNINTCDX11EXT_XESSGETINPUTRESOLUTION pfnXessGetInputResolution,
    INTCExtensionContext*                 pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE        hContext,
    const INTC_D3D11_XESS_2D*             pOutputResolution,
    INTC_D3D11_QUALITY_SETTINGS           qualitySettings,
    INTC_D3D11_XESS_2D*                   pInputResolution );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETOPTIMALINPUTRESOLUTION )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    const INTC_D3D11_XESS_2D*      pOutputResolution,
    INTC_D3D11_QUALITY_SETTINGS    qualitySettings,
    INTC_D3D11_XESS_2D*            pInputResolutionOptimal,
    INTC_D3D11_XESS_2D*            pInputResolutionMin,
    INTC_D3D11_XESS_2D*            pInputResolutionMax );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETOPTIMALINPUTRESOLUTION )(
    PFNINTCDX11EXT_XESSGETOPTIMALINPUTRESOLUTION pfnXessGetOptimalInputResolution,
    INTCExtensionContext*                        pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE               hContext,
    const INTC_D3D11_XESS_2D*                    pOutputResolution,
    INTC_D3D11_QUALITY_SETTINGS                  qualitySettings,
    INTC_D3D11_XESS_2D*                          pInputResolutionOptimal,
    INTC_D3D11_XESS_2D*                          pInputResolutionMin,
    INTC_D3D11_XESS_2D*                          pInputResolutionMax );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETJITTERSCALE )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    float*                         pX,
    float*                         pY );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETJITTERSCALE )(
    PFNINTCDX11EXT_XESSGETJITTERSCALE pfnXessGetJitterScale,
    INTCExtensionContext*             pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE    hContext,
    float*                            pX,
    float*                            pY );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETVELOCITYSCALE )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    float*                         pX,
    float*                         pY );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETVELOCITYSCALE )(
    PFNINTCDX11EXT_XESSGETVELOCITYSCALE pfnXessGetVelocityScale,
    INTCExtensionContext*               pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE      hContext,
    float*                              pX,
    float*                              pY );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSSETJITTERSCALE )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    float                          x,
    float                          y );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSSETJITTERSCALE )(
    PFNINTCDX11EXT_XESSSETJITTERSCALE pfnXessSetJitterScale,
    INTCExtensionContext*             pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE    hContext,
    float                             x,
    float                             y );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSSETVELOCITYSCALE )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    float                          x,
    float                          y );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSSETVELOCITYSCALE )(
    PFNINTCDX11EXT_XESSSETVELOCITYSCALE pfnXessSetVelocityScale,
    INTCExtensionContext*               pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE      hContext,
    float                               x,
    float                               y );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSSETEXPOSUREMULTIPLIER )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    float                          scale );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSSETEXPOSUREMULTIPLIER )(
    PFNINTCDX11EXT_XESSSETEXPOSUREMULTIPLIER pfnXessSetExposureMultiplier,
    INTCExtensionContext*                    pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE           hContext,
    float                                    scale );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETEXPOSUREMULTIPLIER )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    float*                         pScale );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETEXPOSUREMULTIPLIER )(
    PFNINTCDX11EXT_XESSGETEXPOSUREMULTIPLIER pfnXessGetExposureMultiplier,
    INTCExtensionContext*                    pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE           hContext,
    float*                                   pScale );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSSETLOGGINGCALLBACK )(
    INTCExtensionContext*            pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE   hContext,
    INTC_D3D11_XESS_LOGGING_LEVEL    loggingLevel,
    PFNINTCDX11EXT_CALLBACK_XESS_LOG loggingCallback );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSSETLOGGINGCALLBACK )(
    PFNINTCDX11EXT_XESSSETLOGGINGCALLBACK pfnXessSetLoggingCallback,
    INTCExtensionContext*                 pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE        hContext,
    INTC_D3D11_XESS_LOGGING_LEVEL         loggingLevel,
    PFNINTCDX11EXT_CALLBACK_XESS_LOG      loggingCallback );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSISOPTIMALDRIVER )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSISOPTIMALDRIVER )(
    PFNINTCDX11EXT_XESSISOPTIMALDRIVER pfnXessIsOptimalDriver,
    INTCExtensionContext*              pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE     hContext );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSSELECTNETWORKMODEL )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    INTC_D3D11_XESS_NETWORK_MODEL  network );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSSELECTNETWORKMODEL )(
    PFNINTCDX11EXT_XESSSELECTNETWORKMODEL pfnXessSelectNetworkModel,
    INTCExtensionContext*                 pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE        hContext,
    INTC_D3D11_XESS_NETWORK_MODEL         network );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSGETPROFILINGDATA )(
    INTCExtensionContext*            pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE   hContext,
    INTC_D3D11_XESS_PROFILING_DATA** pProfilingData );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSGETPROFILINGDATA )(
    PFNINTCDX11EXT_XESSGETPROFILINGDATA pfnXessGetProfilingData,
    INTCExtensionContext*               pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE      hContext,
    INTC_D3D11_XESS_PROFILING_DATA**    pProfilingData );

typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_XESSFORCELEGACYSCALEFACTORS )(
    INTCExtensionContext*          pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE hContext,
    bool                           force );
typedef INTC_D3D11_XESS_RESULT ( *PFNINTCDX11EXT_API_CALLBACK_XESSFORCELEGACYSCALEFACTORS )(
    PFNINTCDX11EXT_XESSFORCELEGACYSCALEFACTORS pfnXessForceLegacyScaleFactors,
    INTCExtensionContext*                      pExtensionContext,
    INTC_D3D11_XESS_CONTEXT_HANDLE             hContext,
    bool                                       force );

#endif // INTC_IGDEXT_D3D11

#ifdef INTC_IGDEXT_D3D12

typedef HRESULT ( *PFNINTCDX12EXT_GETSUPPORTEDVERSIONS )(
    const ID3D12Device*   pDevice,
    INTCExtensionVersion* pSupportedExtVersions,
    uint32_t*             pSupportedExtVersionsCount );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_GETSUPPORTEDVERSIONS )(
    PFNINTCDX12EXT_GETSUPPORTEDVERSIONS pfnGetSupportedVersions,
    const ID3D12Device*                 pDevice,
    INTCExtensionVersion*               pSupportedExtVersions,
    uint32_t*                           pSupportedExtVersionsCount );

typedef HRESULT ( *PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT )(
    const ID3D12Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo*  pExtensionAppInfo );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT )(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT pfnCreateDeviceExtensionContext,
    const ID3D12Device*                         pDevice,
    INTCExtensionContext**                      ppExtensionContext,
    INTCExtensionInfo*                          pExtensionInfo,
    INTCExtensionAppInfo*                       pExtensionAppInfo );

typedef HRESULT ( *PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT1 )(
    const ID3D12Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo*     pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT1 )(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT1 pfnCreateDeviceExtensionContext1,
    const ID3D12Device*                          pDevice,
    INTCExtensionContext**                       ppExtensionContext,
    INTCExtensionInfo*                           pExtensionInfo,
    INTCExtensionAppInfo1*                       pExtensionAppInfo );

typedef HRESULT ( *PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT2 )(
    const ID3D12Device*    pDevice,
    INTCExtensionContext** ppExtensionContext,
    INTCExtensionInfo1*    pExtensionInfo,
    INTCExtensionAppInfo1* pExtensionAppInfo );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT2 )(
    PFNINTCDX12EXT_CREATEDEVICEEXTENSIONCONTEXT2 pfnCreateDeviceExtensionContext2,
    const ID3D12Device*                          pDevice,
    INTCExtensionContext**                       ppExtensionContext,
    INTCExtensionInfo1*                          pExtensionInfo,
    INTCExtensionAppInfo1*                       pExtensionAppInfo );

typedef HRESULT ( *PFNINTCDX12EXT_CREATECOMMANDQUEUE )(
    INTCExtensionContext*                pExtensionContext,
    const INTC_D3D12_COMMAND_QUEUE_DESC* pDesc,
    REFIID                               riid,
    void**                               ppCommandQueue );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATECOMMANDQUEUE )(
    PFNINTCDX12EXT_CREATECOMMANDQUEUE    pfnCreateCommandQueue,
    INTCExtensionContext*                pExtensionContext,
    const INTC_D3D12_COMMAND_QUEUE_DESC* pDesc,
    REFIID                               riid,
    void**                               ppCommandQueue );

typedef HRESULT ( *PFNINTCDX12EXT_CREATECOMPUTEPIPELINESTATE )(
    INTCExtensionContext*                         pExtensionContext,
    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID                                        riid,
    void**                                        ppPipelineState );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATECOMPUTEPIPELINESTATE )(
    PFNINTCDX12EXT_CREATECOMPUTEPIPELINESTATE     pfnCreateComputePipelineState,
    INTCExtensionContext*                         pExtensionContext,
    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID                                        riid,
    void**                                        ppPipelineState );

typedef HRESULT ( *PFNINTCDX12EXT_CREATERESERVEDRESOURCE )(
    INTCExtensionContext*           pExtensionContext,
    const INTC_D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES           InitialState,
    const D3D12_CLEAR_VALUE*        pOptimizedClearValue,
    REFIID                          riid,
    void**                          ppvResource );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATERESERVEDRESOURCE )(
    PFNINTCDX12EXT_CREATERESERVEDRESOURCE pfnCreateReservedResource,
    INTCExtensionContext*                 pExtensionContext,
    const INTC_D3D12_RESOURCE_DESC*       pDesc,
    D3D12_RESOURCE_STATES                 InitialState,
    const D3D12_CLEAR_VALUE*              pOptimizedClearValue,
    REFIID                                riid,
    void**                                ppvResource );

typedef HRESULT ( *PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE )(
    INTCExtensionContext*                pExtensionContext,
    const D3D12_HEAP_PROPERTIES*         pHeapProperties,
    D3D12_HEAP_FLAGS                     HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES                InitialResourceState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riidResource,
    void**                               ppvResource );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATECOMMITTEDRESOURCE )(
    PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE pfnCreateCommittedResource,
    INTCExtensionContext*                  pExtensionContext,
    const D3D12_HEAP_PROPERTIES*           pHeapProperties,
    D3D12_HEAP_FLAGS                       HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0001*   pDesc,
    D3D12_RESOURCE_STATES                  InitialResourceState,
    const D3D12_CLEAR_VALUE*               pOptimizedClearValue,
    REFIID                                 riidResource,
    void**                                 ppvResource );

typedef HRESULT ( *PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE1 )(
    INTCExtensionContext*                pExtensionContext,
    const D3D12_HEAP_PROPERTIES*         pHeapProperties,
    D3D12_HEAP_FLAGS                     HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0002* pDesc,
    D3D12_RESOURCE_STATES                InitialResourceState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riidResource,
    void**                               ppvResource );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATECOMMITTEDRESOURCE1 )(
    PFNINTCDX12EXT_CREATECOMMITTEDRESOURCE1 pfnCreateCommittedResource1,
    INTCExtensionContext*                   pExtensionContext,
    const D3D12_HEAP_PROPERTIES*            pHeapProperties,
    D3D12_HEAP_FLAGS                        HeapFlags,
    const INTC_D3D12_RESOURCE_DESC_0002*    pDesc,
    D3D12_RESOURCE_STATES                   InitialResourceState,
    const D3D12_CLEAR_VALUE*                pOptimizedClearValue,
    REFIID                                  riidResource,
    void**                                  ppvResource );

typedef HRESULT ( *PFNINTCDX12EXT_CREATEHEAP )(
    INTCExtensionContext*       pExtensionContext,
    const INTC_D3D12_HEAP_DESC* pDesc,
    REFIID                      riid,
    void**                      ppvHeap );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATEHEAP )(
    PFNINTCDX12EXT_CREATEHEAP   pfnCreateHeap,
    INTCExtensionContext*       pExtensionContext,
    const INTC_D3D12_HEAP_DESC* pDesc,
    REFIID                      riid,
    void**                      ppvHeap );

typedef HRESULT ( *PFNINTCDX12EXT_CREATEPLACEDRESOURCE )(
    INTCExtensionContext*                pExtensionContext,
    ID3D12Heap*                          pHeap,
    UINT64                               HeapOffset,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES                InitialState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riid,
    void**                               ppvResource );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATEPLACEDRESOURCE )(
    PFNINTCDX12EXT_CREATEPLACEDRESOURCE  pfnCreatePlacedResource,
    INTCExtensionContext*                pExtensionContext,
    ID3D12Heap*                          pHeap,
    UINT64                               HeapOffset,
    const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
    D3D12_RESOURCE_STATES                InitialState,
    const D3D12_CLEAR_VALUE*             pOptimizedClearValue,
    REFIID                               riid,
    void**                               ppvResource );

typedef HRESULT ( *PFNINTCDX12EXT_CREATEHOSTRTASRESOURCE )(
    INTCExtensionContext* pExtensionContext,
    size_t                SizeInBytes,
    DWORD                 Flags,
    REFIID                riidResource,
    void**                ppvResource );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATEHOSTRTASRESOURCE )(
    PFNINTCDX12EXT_CREATEHOSTRTASRESOURCE pfnCreateHostRTASResource,
    INTCExtensionContext*                 pExtensionContext,
    size_t                                SizeInBytes,
    DWORD                                 Flags,
    REFIID                                riidResource,
    void**                                ppvResource );

typedef void ( *PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST )(
    INTCExtensionContext*                                     pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    const D3D12_GPU_VIRTUAL_ADDRESS*                          pInstanceGPUVAs,
    UINT                                                      NumInstances );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST )(
    PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST  pfnBuildRaytracingAccelerationStructure_Host,
    INTCExtensionContext*                                     pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* pDesc,
    const D3D12_GPU_VIRTUAL_ADDRESS*                          pInstanceGPUVAs,
    UINT                                                      NumInstances );

typedef void ( *PFNINTCDX12EXT_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST )(
    INTCExtensionContext*                             pExtensionContext,
    void*                                             DestAccelerationStructureData,
    const void*                                       SourceAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST )(
    PFNINTCDX12EXT_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST pfnCopyRaytracingAccelerationStructure_Host,
    INTCExtensionContext*                                   pExtensionContext,
    void*                                                   DestAccelerationStructureData,
    const void*                                             SourceAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE       Mode );

typedef void ( *PFNINTCDX12EXT_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST )(
    INTCExtensionContext*                                       pExtensionContext,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE InfoType,
    void*                                                       DestBuffer,
    const void*                                                 SourceRTAS );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST )(
    PFNINTCDX12EXT_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST pfnEmitRaytracingAccelerationStructurePostbuildInfo_Host,
    INTCExtensionContext*                                                pExtensionContext,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE          InfoType,
    void*                                                                DestBuffer,
    const void*                                                          SourceRTAS );

typedef void ( *PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST )(
    INTCExtensionContext*                                       pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*      pInfo );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST )(
    PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST pfnGetRaytracingAccelerationStructurePrebuildInfo_Host,
    INTCExtensionContext*                                              pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*        pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*             pInfo );

typedef void ( *PFNINTCDX12EXT_TRANSFERHOSTRTAS )(
    INTCExtensionContext*                             pExtensionContext,
    ID3D12GraphicsCommandList*                        pCommandList,
    D3D12_GPU_VIRTUAL_ADDRESS                         DestAccelerationStructureData,
    D3D12_GPU_VIRTUAL_ADDRESS                         SrcAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_TRANSFERHOSTRTAS )(
    PFNINTCDX12EXT_TRANSFERHOSTRTAS                   pfnTransferHostRTAS,
    INTCExtensionContext*                             pExtensionContext,
    ID3D12GraphicsCommandList*                        pCommandList,
    D3D12_GPU_VIRTUAL_ADDRESS                         DestAccelerationStructureData,
    D3D12_GPU_VIRTUAL_ADDRESS                         SrcAccelerationStructureData,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode );

typedef void ( *PFNINTCDX12EXT_SETDRIVEREVENTMETADATA )(
    INTCExtensionContext*      pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    UINT64                     Metadata );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_SETDRIVEREVENTMETADATA )(
    PFNINTCDX12EXT_SETDRIVEREVENTMETADATA pfnSetDriverEventMetadata,
    INTCExtensionContext*                 pExtensionContext,
    ID3D12GraphicsCommandList*            pCommandList,
    UINT64                                Metadata );

typedef void ( *PFNINTCDX12EXT_QUERYCPUVISIBLEVIDMEM )(
    INTCExtensionContext* pExtensionContext,
    UINT64*               pTotalBytes,
    UINT64*               pFreeBytes );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_QUERYCPUVISIBLEVIDMEM )(
    PFNINTCDX12EXT_QUERYCPUVISIBLEVIDMEM pfnQueryCpuVisibleVidmem,
    INTCExtensionContext*                pExtensionContext,
    UINT64*                              pTotalBytes,
    UINT64*                              pFreeBytes );

typedef HRESULT ( *PFNINTCDX12EXT_CREATESTATEOBJECT )(
    INTCExtensionContext*               pExtensionContext,
    const INTC_D3D12_STATE_OBJECT_DESC* pDesc,
    REFIID                              riid,
    void**                              ppPipelineState );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CREATESTATEOBJECT )(
    PFNINTCDX12EXT_CREATESTATEOBJECT    pfnCreateStateObject,
    INTCExtensionContext*               pExtensionContext,
    const INTC_D3D12_STATE_OBJECT_DESC* pDesc,
    REFIID                              riid,
    void**                              ppPipelineState );

typedef void ( *PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE )(
    INTCExtensionContext*                                                                   pExtensionContext,
    ID3D12GraphicsCommandList*                                                              pCommandList,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*                               pDesc,
    UINT                                                                                    NumPostbuildInfoDescs,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*                      pPostbuildInfoDescs,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pComparisonDataDesc );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_BUILDRAYTRACINGACCELERATIONSTRUCTURE )(
    PFNINTCDX12EXT_BUILDRAYTRACINGACCELERATIONSTRUCTURE                                     pfnBuildRaytracingAccelerationStructure,
    INTCExtensionContext*                                                                   pExtensionContext,
    ID3D12GraphicsCommandList*                                                              pCommandList,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*                               pDesc,
    UINT                                                                                    NumPostbuildInfoDescs,
    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*                      pPostbuildInfoDescs,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pComparisonDataDesc );

typedef void ( *PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO )(
    INTCExtensionContext*                                                                   pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*                             pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*                                  pInfo,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pComparisonDataDesc );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO )(
    PFNINTCDX12EXT_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO                           pfnGetRaytracingAccelerationStructurePrebuildInfo,
    INTCExtensionContext*                                                                   pExtensionContext,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*                             pDesc,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO*                                  pInfo,
    const INTC_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_INSTANCE_COMPARISON_DATA* pComparisonDataDesc );

typedef HRESULT ( *PFNINTCDX12EXT_SETFEATURESUPPORT )(
    INTCExtensionContext* pExtensionContext,
    INTC_D3D12_FEATURE*   pFeature );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_SETFEATURESUPPORT )(
    PFNINTCDX12EXT_SETFEATURESUPPORT pfnSetFeatureSupport,
    INTCExtensionContext*            pExtensionContext,
    INTC_D3D12_FEATURE*              pFeature );

typedef D3D12_RESOURCE_ALLOCATION_INFO ( *PFNINTCDX12EXT_GETRESOURCEALLOCATIONINFO )(
    INTCExtensionContext*                pExtensionContext,
    UINT                                 visibleMask,
    UINT                                 numResourceDescs,
    const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs );
typedef D3D12_RESOURCE_ALLOCATION_INFO ( *PFNINTCDX12EXT_API_CALLBACK_GETRESOURCEALLOCATIONINFO )(
    PFNINTCDX12EXT_GETRESOURCEALLOCATIONINFO pfnGetResourceAllocationInfo,
    INTCExtensionContext*                    pExtensionContext,
    UINT                                     visibleMask,
    UINT                                     numResourceDescs,
    const INTC_D3D12_RESOURCE_DESC_0001*     pResourceDescs );

typedef HRESULT ( *PFNINTCDX12EXT_CHECKFEATURESUPPORT )(
    INTCExtensionContext* pExtensionContext,
    INTC_D3D12_FEATURES   Feature,
    void*                 pFeatureSupportData,
    UINT                  FeatureSupportDataSize );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_CHECKFEATURESUPPORT )(
    PFNINTCDX12EXT_CHECKFEATURESUPPORT pfnCheckFeatureSupport,
    INTCExtensionContext*              pExtensionContext,
    INTC_D3D12_FEATURES                Feature,
    void*                              pFeatureSupportData,
    UINT                               FeatureSupportDataSize );

typedef HRESULT ( *PFNINTCDX12EXT_ADDSHADERBINARIESPATH )(
    INTCExtensionContext* pExtensionContext,
    const wchar_t*        filePath );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_ADDSHADERBINARIESPATH )(
    PFNINTCDX12EXT_ADDSHADERBINARIESPATH pfnAddShaderBinariesPath,
    INTCExtensionContext*                pExtensionContext,
    const wchar_t*                       filePath );

typedef HRESULT ( *PFNINTCDX12EXT_REMOVESHADERBINARIESPATH )(
    INTCExtensionContext* pExtensionContext,
    const wchar_t*        filePath );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_REMOVESHADERBINARIESPATH )(
    PFNINTCDX12EXT_REMOVESHADERBINARIESPATH pfnRemoveShaderBinariesPath,
    INTCExtensionContext*                   pExtensionContext,
    const wchar_t*                          filePath );

typedef HRESULT ( *PFNINTCDX12EXT_SETAPPLICATIONINFO )(
    INTCExtensionAppInfo1* pExtensionAppInfo );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_SETAPPLICATIONINFO )(
    PFNINTCDX12EXT_SETAPPLICATIONINFO pfnSetApplicationInfo,
    INTCExtensionAppInfo1*            pExtensionAppInfo );

typedef HRESULT ( *PFNINTCDX12EXT_SETNUMGENERATEDFRAMES )(
    INTCExtensionContext* pExtensionContext,
    UINT                  NumFrames );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_SETNUMGENERATEDFRAMES )(
    PFNINTCDX12EXT_SETNUMGENERATEDFRAMES pfnSetNumGeneratedFrames,
    INTCExtensionContext*                pExtensionContext,
    UINT                                 NumFrames );

typedef HRESULT ( *PFNINTCDX12EXT_SETPRESENTSEQUENCENUMBER )(
    INTCExtensionContext* pExtensionContext,
    UINT                  PresentSequenceNumber );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_SETPRESENTSEQUENCENUMBER )(
    PFNINTCDX12EXT_SETPRESENTSEQUENCENUMBER pfnSetPresentSequenceNumber,
    INTCExtensionContext*                   pExtensionContext,
    UINT                                    PresentSequenceNumber );

typedef HRESULT ( *PFNINTCDX12EXT_GETDISPLAYTELEMETRY )(
    INTCExtensionContext* pExtensionContext,
    void*                 pTelemetryData,
    UINT                  size );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_GETDISPLAYTELEMETRY )(
    PFNINTCDX12EXT_GETDISPLAYTELEMETRY pfnGetDisplayTelemetry,
    INTCExtensionContext*              pExtensionContext,
    void*                              pTelemetryData,
    UINT                               size );

typedef HRESULT ( *PFNINTCDX12EXT_GETLATENCYREDUCTIONSTATUS )(
    INTCExtensionContext*                pExtensionContext,
    INTC_D3D12_LATENCY_REDUCTION_STATUS* pLatencyReductionStatus );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_GETLATENCYREDUCTIONSTATUS )(
    PFNINTCDX12EXT_GETLATENCYREDUCTIONSTATUS pfnGetLatencyReductionStatus,
    INTCExtensionContext*                    pExtensionContext,
    INTC_D3D12_LATENCY_REDUCTION_STATUS*     pLatencyReductionStatus );

typedef HRESULT ( *PFNINTCDX12EXT_LATENCYREDUCTIONEXT )(
    INTCExtensionContext* pExtensionContext,
    uint32_t              version,
    BOOL                  latencyReductionEnabled,
    BOOL                  renderSubmitTimingsEnabled,
    uint32_t              timingSlots );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_LATENCYREDUCTIONEXT )(
    PFNINTCDX12EXT_LATENCYREDUCTIONEXT pfnLatencyReductionExt,
    INTCExtensionContext*              pExtensionContext,
    uint32_t                           version,
    BOOL                               latencyReductionEnabled,
    BOOL                               renderSubmitTimingsEnabled,
    uint32_t                           timingSlots );

typedef HRESULT ( *PFNINTCDX12EXT_LATENCYREDUCTIONGETRENDERSUBMITTIMINGSBUFFERS )(
    INTCExtensionContext* pExtensionContext,
    void**                ppRenderSubmitCpuTimings,
    void**                ppRenderSubmitGpuTimings );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_LATENCYREDUCTIONGETRENDERSUBMITTIMINGSBUFFERS )(
    PFNINTCDX12EXT_LATENCYREDUCTIONGETRENDERSUBMITTIMINGSBUFFERS pfnLatencyReductionGetRenderSubmitTimingsBuffers,
    INTCExtensionContext*                                        pExtensionContext,
    void**                                                       ppRenderSubmitCpuTimings,
    void**                                                       ppRenderSubmitGpuTimings );

typedef HRESULT ( *PFNINTCDX12EXT_RENDERSUBMITSTART )(
    INTCExtensionContext* pExtensionContext,
    uint32_t              frameId );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_RENDERSUBMITSTART )(
    PFNINTCDX12EXT_RENDERSUBMITSTART pfnRenderSubmitStart,
    INTCExtensionContext*            pExtensionContext,
    uint32_t                         frameId );

typedef uint64_t ( *PFNINTCDX12EXT_GETCOMMANDLISTHANDLE )(
    INTCExtensionContext* pExtensionContext,
    void*                 pCommandList );
typedef uint64_t ( *PFNINTCDX12EXT_API_CALLBACK_GETCOMMANDLISTHANDLE )(
    PFNINTCDX12EXT_GETCOMMANDLISTHANDLE pfnGetCommandListHandle,
    INTCExtensionContext*               pExtensionContext,
    void*                               pCommandList );

typedef void ( *PFNINTCDX12EXT_SETEVENTMARKER )(
    INTCExtensionContext*      pExtensionContext,
    ID3D12GraphicsCommandList* pCommandList,
    uint32_t                   eventType,
    const void*                marker,
    uint32_t                   markerSize );
typedef void ( *PFNINTCDX12EXT_API_CALLBACK_SETEVENTMARKER )(
    PFNINTCDX12EXT_SETEVENTMARKER pfnSetEventMarker,
    INTCExtensionContext*         pExtensionContext,
    ID3D12GraphicsCommandList*    pCommandList,
    uint32_t                      eventType,
    const void*                   marker,
    uint32_t                      markerSize );

typedef HRESULT ( *PFNINTCDX12EXT_GETCURRENTSHADERHEAPUSAGE )(
    INTCExtensionContext* pExtensionContext,
    UINT64*               pShaderHeapUsage );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_GETCURRENTSHADERHEAPUSAGE )(
    PFNINTCDX12EXT_GETCURRENTSHADERHEAPUSAGE pfnGetCurrentShaderHeapUsage,
    INTCExtensionContext*                    pExtensionContext,
    UINT64*                                  pShaderHeapUsage );

typedef HRESULT ( *PFNINTCDX12EXT_SETDEVICEPARAMS )(
    INTCDeviceParams* pDeviceParams );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_SETDEVICEPARAMS )(
    PFNINTCDX12EXT_SETDEVICEPARAMS pfnSetDeviceParams,
    INTCDeviceParams*              pDeviceParams );

typedef HRESULT ( *PFNINTCDX12EXT_GETCACHEDBLOB )(
    INTCExtensionContext*        pExtensionContext,
    ID3D12PipelineState*         pPipelineState,
    ID3DBlob**                   ppBlob,
    INTC_D3D12_CACHED_BLOB_FLAGS flags );
typedef HRESULT ( *PFNINTCDX12EXT_API_CALLBACK_GETCACHEDBLOB )(
    PFNINTCDX12EXT_GETCACHEDBLOB pfnGetCachedBlob,
    INTCExtensionContext*        pExtensionContext,
    ID3D12PipelineState*         pPipelineState,
    ID3DBlob**                   ppBlob,
    INTC_D3D12_CACHED_BLOB_FLAGS flags );

#endif // INTC_IGDEXT_D3D12

typedef HRESULT ( *PFNINTCEXT_DESTROYDEVICEEXTENSIONCONTEXT )(
    INTCExtensionContext** ppExtensionContext );
typedef HRESULT ( *PFNINTCEXT_API_CALLBACK_DESTROYDEVICEEXTENSIONCONTEXT )(
    PFNINTCEXT_DESTROYDEVICEEXTENSIONCONTEXT pfnDestroyDeviceExtensionContext,
    INTCExtensionContext**                   ppExtensionContext );

#ifdef INTC_IGDEXT_D3D11
// Function pointers to Device Extension Context D3D11 API functions
struct INTC_D3D11_API_CALLBACKS
{
    PFNINTCDX11EXT_API_CALLBACK_GETSUPPORTEDVERSIONS          INTC_D3D11_GetSupportedVersions;
    PFNINTCDX11EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT  INTC_D3D11_CreateDeviceExtensionContext;
    PFNINTCDX11EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT1 INTC_D3D11_CreateDeviceExtensionContext1;
    PFNINTCEXT_API_CALLBACK_DESTROYDEVICEEXTENSIONCONTEXT     INTC_DestroyDeviceExtensionContext;

    PFNINTCDX11EXT_API_CALLBACK_BEGINUAVOVERLAP                                INTC_D3D11_BeginUAVOverlap;
    PFNINTCDX11EXT_API_CALLBACK_ENDUAVOVERLAP                                  INTC_D3D11_EndUAVOverlap;
    PFNINTCDX11EXT_API_CALLBACK_BEGIN_RETRIEVE_RESOURCE_HANDLE                 INTC_D3D11_Begin_Retrieve_Resource_Handle;
    PFNINTCDX11EXT_API_CALLBACK_END_RETRIEVE_RESOURCE_HANDLE                   INTC_D3D11_End_Retrieve_Resource_Handle;
    PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINSTANCEDINDIRECT                     INTC_D3D11_MultiDrawInstancedIndirect;
    PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINDEXEDINSTANCEDINDIRECT              INTC_D3D11_MultiDrawIndexedInstancedIndirect;
    PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT        INTC_D3D11_MultiDrawInstancedIndirectCountIndirect;
    PFNINTCDX11EXT_API_CALLBACK_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT INTC_D3D11_MultiDrawIndexedInstancedIndirectCountIndirect;
    PFNINTCDX11EXT_API_CALLBACK_SETDEPTHBOUNDS                                 INTC_D3D11_SetDepthBounds;
    PFNINTCDX11EXT_API_CALLBACK_CREATETEXTURE2D                                INTC_D3D11_CreateTexture2D;
    PFNINTCDX11EXT_API_CALLBACK_XESSCREATECONTEXT                              INTC_D3D11_XessCreateContext;
    PFNINTCDX11EXT_API_CALLBACK_XESSDESTROYCONTEXT                             INTC_D3D11_XessDestroyContext;
    PFNINTCDX11EXT_API_CALLBACK_XESSINIT                                       INTC_D3D11_XessInit;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETINITPARAMS                              INTC_D3D11_XessGetInitParams;
    PFNINTCDX11EXT_API_CALLBACK_XESSEXECUTE                                    INTC_D3D11_XessExecute;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETVERSION                                 INTC_D3D11_XessGetVersion;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETINTELXEFXVERSION                        INTC_D3D11_XessGetIntelXeFXVersion;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETINPUTRESOLUTION                         INTC_D3D11_XessGetInputResolution;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETOPTIMALINPUTRESOLUTION                  INTC_D3D11_XessGetOptimalInputResolution;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETJITTERSCALE                             INTC_D3D11_XessGetJitterScale;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETVELOCITYSCALE                           INTC_D3D11_XessGetVelocityScale;
    PFNINTCDX11EXT_API_CALLBACK_XESSSETJITTERSCALE                             INTC_D3D11_XessSetJitterScale;
    PFNINTCDX11EXT_API_CALLBACK_XESSSETVELOCITYSCALE                           INTC_D3D11_XessSetVelocityScale;
    PFNINTCDX11EXT_API_CALLBACK_XESSSETEXPOSUREMULTIPLIER                      INTC_D3D11_XessSetExposureMultiplier;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETEXPOSUREMULTIPLIER                      INTC_D3D11_XessGetExposureMultiplier;
    PFNINTCDX11EXT_API_CALLBACK_XESSSETLOGGINGCALLBACK                         INTC_D3D11_XessSetLoggingCallback;
    PFNINTCDX11EXT_API_CALLBACK_XESSISOPTIMALDRIVER                            INTC_D3D11_XessIsOptimalDriver;
    PFNINTCDX11EXT_API_CALLBACK_XESSSELECTNETWORKMODEL                         INTC_D3D11_XessSelectNetworkModel;
    PFNINTCDX11EXT_API_CALLBACK_XESSGETPROFILINGDATA                           INTC_D3D11_XessGetProfilingData;
    PFNINTCDX11EXT_API_CALLBACK_XESSFORCELEGACYSCALEFACTORS                    INTC_D3D11_XessForceLegacyScaleFactors;
};
#endif // INTC_IGDEXT_D3D11

#ifdef INTC_IGDEXT_D3D12
// Function pointers to Device Extension Context D3D12 API functions
// Deprecated: Use INTC_D3D12_API_CALLBACKS1 instead
struct INTC_D3D12_API_CALLBACKS
{
    PFNINTCDX12EXT_API_CALLBACK_GETSUPPORTEDVERSIONS          INTC_D3D12_GetSupportedVersions;
    PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT  INTC_D3D12_CreateDeviceExtensionContext;
    PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT1 INTC_D3D12_CreateDeviceExtensionContext1;
    PFNINTCEXT_API_CALLBACK_DESTROYDEVICEEXTENSIONCONTEXT     INTC_DestroyDeviceExtensionContext;

    PFNINTCDX12EXT_API_CALLBACK_CREATECOMMANDQUEUE                                    INTC_D3D12_CreateCommandQueue;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMPUTEPIPELINESTATE                            INTC_D3D12_CreateComputePipelineState;
    PFNINTCDX12EXT_API_CALLBACK_CREATERESERVEDRESOURCE                                INTC_D3D12_CreateReservedResource;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMMITTEDRESOURCE                               INTC_D3D12_CreateCommittedResource;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMMITTEDRESOURCE1                              INTC_D3D12_CreateCommittedResource1;
    PFNINTCDX12EXT_API_CALLBACK_CREATEHEAP                                            INTC_D3D12_CreateHeap;
    PFNINTCDX12EXT_API_CALLBACK_CREATEPLACEDRESOURCE                                  INTC_D3D12_CreatePlacedResource;
    PFNINTCDX12EXT_API_CALLBACK_CREATEHOSTRTASRESOURCE                                INTC_D3D12_CreateHostRTASResource;
    PFNINTCDX12EXT_API_CALLBACK_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST             INTC_D3D12_BuildRaytracingAccelerationStructure_Host;
    PFNINTCDX12EXT_API_CALLBACK_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST              INTC_D3D12_CopyRaytracingAccelerationStructure_Host;
    PFNINTCDX12EXT_API_CALLBACK_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_Host;
    PFNINTCDX12EXT_API_CALLBACK_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST   INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_Host;
    PFNINTCDX12EXT_API_CALLBACK_TRANSFERHOSTRTAS                                      INTC_D3D12_TransferHostRTAS;
    PFNINTCDX12EXT_API_CALLBACK_SETDRIVEREVENTMETADATA                                INTC_D3D12_SetDriverEventMetadata;
    PFNINTCDX12EXT_API_CALLBACK_QUERYCPUVISIBLEVIDMEM                                 INTC_D3D12_QueryCpuVisibleVidmem;
    PFNINTCDX12EXT_API_CALLBACK_CREATESTATEOBJECT                                     INTC_D3D12_CreateStateObject;
    PFNINTCDX12EXT_API_CALLBACK_BUILDRAYTRACINGACCELERATIONSTRUCTURE                  INTC_D3D12_BuildRaytracingAccelerationStructure;
    PFNINTCDX12EXT_API_CALLBACK_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO        INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo;
    PFNINTCDX12EXT_API_CALLBACK_SETFEATURESUPPORT                                     INTC_D3D12_SetFeatureSupport;
    PFNINTCDX12EXT_API_CALLBACK_GETRESOURCEALLOCATIONINFO                             INTC_D3D12_GetResourceAllocationInfo;
    PFNINTCDX12EXT_API_CALLBACK_CHECKFEATURESUPPORT                                   INTC_D3D12_CheckFeatureSupport;
    PFNINTCDX12EXT_API_CALLBACK_ADDSHADERBINARIESPATH                                 INTC_D3D12_AddShaderBinariesPath;
    PFNINTCDX12EXT_API_CALLBACK_REMOVESHADERBINARIESPATH                              INTC_D3D12_RemoveShaderBinariesPath;
    PFNINTCDX12EXT_API_CALLBACK_SETAPPLICATIONINFO                                    INTC_D3D12_SetApplicationInfo;
    PFNINTCDX12EXT_API_CALLBACK_SETNUMGENERATEDFRAMES                                 INTC_D3D12_SetNumGeneratedFrames;
    PFNINTCDX12EXT_API_CALLBACK_SETPRESENTSEQUENCENUMBER                              INTC_D3D12_SetPresentSequenceNumber;
    PFNINTCDX12EXT_API_CALLBACK_GETDISPLAYTELEMETRY                                   INTC_D3D12_GetDisplayTelemetry;
    PFNINTCDX12EXT_API_CALLBACK_GETLATENCYREDUCTIONSTATUS                             INTC_D3D12_GetLatencyReductionStatus;
    PFNINTCDX12EXT_API_CALLBACK_LATENCYREDUCTIONEXT                                   INTC_D3D12_LatencyReductionExt;
    PFNINTCDX12EXT_API_CALLBACK_LATENCYREDUCTIONGETRENDERSUBMITTIMINGSBUFFERS         INTC_D3D12_LatencyReductionGetRenderSubmitTimingsBuffers;
    PFNINTCDX12EXT_API_CALLBACK_RENDERSUBMITSTART                                     INTC_D3D12_RenderSubmitStart;
    PFNINTCDX12EXT_API_CALLBACK_GETCOMMANDLISTHANDLE                                  INTC_D3D12_GetCommandListHandle;
    PFNINTCDX12EXT_API_CALLBACK_SETEVENTMARKER                                        INTC_D3D12_SetEventMarker;
};

// Function pointers to Device Extension Context D3D12 API functions
struct INTC_D3D12_API_CALLBACKS1
{
    // Compatibility header (added in version 2)
    struct
    {
        uint32_t structVersion; // Current structure version
        uint32_t structSize;    // Size of this structure
        uint32_t reserved[ 2 ]; // Reserved for future use
    } header;

    // Version 1 function pointers (always present)
    PFNINTCDX12EXT_API_CALLBACK_GETSUPPORTEDVERSIONS                                  INTC_D3D12_GetSupportedVersions;
    PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT                          INTC_D3D12_CreateDeviceExtensionContext;
    PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT1                         INTC_D3D12_CreateDeviceExtensionContext1;
    PFNINTCEXT_API_CALLBACK_DESTROYDEVICEEXTENSIONCONTEXT                             INTC_DestroyDeviceExtensionContext;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMMANDQUEUE                                    INTC_D3D12_CreateCommandQueue;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMPUTEPIPELINESTATE                            INTC_D3D12_CreateComputePipelineState;
    PFNINTCDX12EXT_API_CALLBACK_CREATERESERVEDRESOURCE                                INTC_D3D12_CreateReservedResource;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMMITTEDRESOURCE                               INTC_D3D12_CreateCommittedResource;
    PFNINTCDX12EXT_API_CALLBACK_CREATECOMMITTEDRESOURCE1                              INTC_D3D12_CreateCommittedResource1;
    PFNINTCDX12EXT_API_CALLBACK_CREATEHEAP                                            INTC_D3D12_CreateHeap;
    PFNINTCDX12EXT_API_CALLBACK_CREATEPLACEDRESOURCE                                  INTC_D3D12_CreatePlacedResource;
    PFNINTCDX12EXT_API_CALLBACK_CREATEHOSTRTASRESOURCE                                INTC_D3D12_CreateHostRTASResource;
    PFNINTCDX12EXT_API_CALLBACK_BUILDRAYTRACINGACCELERATIONSTRUCTURE_HOST             INTC_D3D12_BuildRaytracingAccelerationStructure_Host;
    PFNINTCDX12EXT_API_CALLBACK_COPYRAYTRACINGACCELERATIONSTRUCTURE_HOST              INTC_D3D12_CopyRaytracingAccelerationStructure_Host;
    PFNINTCDX12EXT_API_CALLBACK_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO_HOST INTC_D3D12_EmitRaytracingAccelerationStructurePostbuildInfo_Host;
    PFNINTCDX12EXT_API_CALLBACK_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO_HOST   INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo_Host;
    PFNINTCDX12EXT_API_CALLBACK_TRANSFERHOSTRTAS                                      INTC_D3D12_TransferHostRTAS;
    PFNINTCDX12EXT_API_CALLBACK_SETDRIVEREVENTMETADATA                                INTC_D3D12_SetDriverEventMetadata;
    PFNINTCDX12EXT_API_CALLBACK_QUERYCPUVISIBLEVIDMEM                                 INTC_D3D12_QueryCpuVisibleVidmem;
    PFNINTCDX12EXT_API_CALLBACK_CREATESTATEOBJECT                                     INTC_D3D12_CreateStateObject;
    PFNINTCDX12EXT_API_CALLBACK_BUILDRAYTRACINGACCELERATIONSTRUCTURE                  INTC_D3D12_BuildRaytracingAccelerationStructure;
    PFNINTCDX12EXT_API_CALLBACK_GETRAYTRACINGACCELERATIONSTRUCTUREPREBUILDINFO        INTC_D3D12_GetRaytracingAccelerationStructurePrebuildInfo;
    PFNINTCDX12EXT_API_CALLBACK_SETFEATURESUPPORT                                     INTC_D3D12_SetFeatureSupport;
    PFNINTCDX12EXT_API_CALLBACK_GETRESOURCEALLOCATIONINFO                             INTC_D3D12_GetResourceAllocationInfo;
    PFNINTCDX12EXT_API_CALLBACK_CHECKFEATURESUPPORT                                   INTC_D3D12_CheckFeatureSupport;
    PFNINTCDX12EXT_API_CALLBACK_ADDSHADERBINARIESPATH                                 INTC_D3D12_AddShaderBinariesPath;
    PFNINTCDX12EXT_API_CALLBACK_REMOVESHADERBINARIESPATH                              INTC_D3D12_RemoveShaderBinariesPath;
    PFNINTCDX12EXT_API_CALLBACK_SETAPPLICATIONINFO                                    INTC_D3D12_SetApplicationInfo;
    PFNINTCDX12EXT_API_CALLBACK_SETNUMGENERATEDFRAMES                                 INTC_D3D12_SetNumGeneratedFrames;
    PFNINTCDX12EXT_API_CALLBACK_SETPRESENTSEQUENCENUMBER                              INTC_D3D12_SetPresentSequenceNumber;
    PFNINTCDX12EXT_API_CALLBACK_GETDISPLAYTELEMETRY                                   INTC_D3D12_GetDisplayTelemetry;
    PFNINTCDX12EXT_API_CALLBACK_GETLATENCYREDUCTIONSTATUS                             INTC_D3D12_GetLatencyReductionStatus;
    PFNINTCDX12EXT_API_CALLBACK_LATENCYREDUCTIONEXT                                   INTC_D3D12_LatencyReductionExt;
    PFNINTCDX12EXT_API_CALLBACK_LATENCYREDUCTIONGETRENDERSUBMITTIMINGSBUFFERS         INTC_D3D12_LatencyReductionGetRenderSubmitTimingsBuffers;
    PFNINTCDX12EXT_API_CALLBACK_RENDERSUBMITSTART                                     INTC_D3D12_RenderSubmitStart;
    PFNINTCDX12EXT_API_CALLBACK_GETCOMMANDLISTHANDLE                                  INTC_D3D12_GetCommandListHandle;
    PFNINTCDX12EXT_API_CALLBACK_SETEVENTMARKER                                        INTC_D3D12_SetEventMarker;
    PFNINTCDX12EXT_API_CALLBACK_GETCURRENTSHADERHEAPUSAGE                             INTC_D3D12_GetCurrentShaderHeapUsage;

    // Version 2+ function pointers (check header.structVersion >= 2)

    // Version 2 additions
    PFNINTCDX12EXT_API_CALLBACK_CREATEDEVICEEXTENSIONCONTEXT2 INTC_D3D12_CreateDeviceExtensionContext2;
    PFNINTCDX12EXT_API_CALLBACK_SETDEVICEPARAMS               INTC_D3D12_SetDeviceParams;
    PFNINTCDX12EXT_API_CALLBACK_GETCACHEDBLOB                 INTC_D3D12_GetCachedBlob;
};

// Version constants
#define INTC_D3D12_API_CALLBACKS_VERSION_CURRENT 2
#define INTC_D3D12_API_CALLBACKS_SIZE_CURRENT sizeof( INTC_D3D12_API_CALLBACKS )

// Helper macros for version checking
#define INTC_D3D12_API_CALLBACKS_INIT( callbacks )                                     \
    do {                                                                               \
        ( callbacks ).header.structVersion = INTC_D3D12_API_CALLBACKS_VERSION_CURRENT; \
        ( callbacks ).header.structSize    = INTC_D3D12_API_CALLBACKS_SIZE_CURRENT;    \
        ( callbacks ).header.reserved[ 0 ] = 0;                                        \
        ( callbacks ).header.reserved[ 1 ] = 0;                                        \
    } while( 0 )

#define INTC_D3D12_API_CALLBACKS_CHECK_VERSION( callbacks, min_version ) ( ( callbacks ).header.structVersion >= ( min_version ) )

#define INTC_D3D12_API_CALLBACKS_CHECK_SIZE( callbacks, required_size ) ( ( callbacks ).header.structSize >= ( required_size ) )

#endif // INTC_IGDEXT_D3D12
