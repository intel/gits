# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED DIRECTXTEX_ROOT)
  install_dependencies("--with-directxtex")
  set(DIRECTXTEX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/DirectXTex/DirectXTex")
endif()

add_library(directxtex STATIC)

target_sources(directxtex
PRIVATE
  ${DIRECTXTEX_ROOT}/BC.cpp
  ${DIRECTXTEX_ROOT}/BC.h
  ${DIRECTXTEX_ROOT}/BC4BC5.cpp
  ${DIRECTXTEX_ROOT}/BC6HBC7.cpp
  ${DIRECTXTEX_ROOT}/d3dx12.h
  ${DIRECTXTEX_ROOT}/DDS.h
  ${DIRECTXTEX_ROOT}/DirectXTex.h
  ${DIRECTXTEX_ROOT}/DirectXTex.inl
  ${DIRECTXTEX_ROOT}/DirectXTexCompress.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexCompressGPU.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexConvert.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexD3D11.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexD3D12.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexDDS.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexFlipRotate.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexHDR.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexImage.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexMipmaps.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexMisc.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexNormalMaps.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexP.h
  ${DIRECTXTEX_ROOT}/DirectXTexPMAlpha.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexResize.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexTGA.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexUtil.cpp
  ${DIRECTXTEX_ROOT}/DirectXTexWIC.cpp
)
set_target_properties(directxtex PROPERTIES FOLDER External)

include_directories(SYSTEM ${DIRECTXTEX_ROOT}/DirectXTex)
