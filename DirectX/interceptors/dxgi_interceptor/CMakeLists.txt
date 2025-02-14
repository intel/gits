# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(dxgi_interceptor SHARED)
target_link_libraries(dxgi_interceptor common detours)

set(COMMON_SRC
  ${CMAKE_SOURCE_DIR}/common/tools_lite.cpp
  ${CMAKE_SOURCE_DIR}/common/exception.cpp
  ${CMAKE_SOURCE_DIR}/common/timer.cpp
  ${CMAKE_SOURCE_DIR}/common/dynamic_linker.cpp
  ${CMAKE_SOURCE_DIR}/common/gitsLoader.cpp
  ${CMAKE_SOURCE_DIR}/common/log.cpp
)
source_group("common" FILES ${COMMON_SRC})

set(ROOT_SRC
  dxgi.cpp
  dxgi.def
)
source_group("" FILES ${ROOT_SRC})

target_sources(dxgi_interceptor PRIVATE
  ${COMMON_SRC}
  ${ROOT_SRC}
)

target_include_directories(dxgi_interceptor PRIVATE
  ${DIRECTX_DIR}/recorder
  ${DIRECTX_DIR}/common/layer_interface
  ${D3D12_HEADERS_DIR}
  ${DML_HEADERS_DIR}
  ${XESS_HEADERS_DIR}
  ${DSTORAGE_HEADERS_DIR}
)

set_target_properties(dxgi_interceptor PROPERTIES OUTPUT_NAME dxgi)
set_target_properties(dxgi_interceptor PROPERTIES FOLDER DirectX/DirectX_interceptors)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:dxgi_interceptor> DESTINATION Recorder/${DIRECTX_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS dxgi_interceptor
  RUNTIME DESTINATION Recorder/${DIRECTX_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${DIRECTX_INSTALL_SUBDIR})