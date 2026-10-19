# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================


if(NOT DEFINED DXIL_SPIRV_DIR)
  add_thirdparty_arg("--with-dxil-spirv")
  set(DXIL_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxil-spirv")
endif()

if(NOT DEFINED SPIRV_CROSS_DIR)
  add_thirdparty_arg("--with-spirv-cross")
  set(SPIRV_CROSS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spirv-cross")
endif()

# Called only by the GUI toolchain. Merely registering the dependency must not
# add it to the player/plugin dependency graph or install upstream CLI binaries.
function(init_dxil_spirv)
  if(NOT TARGET dxil-spirv-c-static)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(DXIL_SPIRV_CLI OFF)
    set(DXIL_SPIRV_NATIVE_LLVM OFF)
    set(DXIL_SPV_MISC_CLI OFF)
    add_subdirectory("${DXIL_SPIRV_DIR}" "${CMAKE_BINARY_DIR}/third_party/dxil-spirv"
                     EXCLUDE_FROM_ALL)
  endif()

  # The converter bundles glslang-spirv-builder and links it transitively.
  # SPIRV-Cross is otherwise added only when the upstream CLI is enabled.
  if(NOT TARGET spirv-cross-core)
    set(SPIRV_CROSS_CLI OFF)
    set(SPIRV_CROSS_ENABLE_TESTS OFF)
    set(SPIRV_CROSS_STATIC ON)
    set(SPIRV_CROSS_SHARED OFF)
    set(SPIRV_CROSS_ENABLE_GLSL ON)
    set(SPIRV_CROSS_ENABLE_HLSL ON)
    set(SPIRV_CROSS_ENABLE_MSL OFF)
    set(SPIRV_CROSS_ENABLE_CPP OFF)
    set(SPIRV_CROSS_ENABLE_REFLECT OFF)
    set(SPIRV_CROSS_ENABLE_C_API OFF)
    set(SPIRV_CROSS_ENABLE_UTIL OFF)
    set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS OFF)
    set(SPIRV_CROSS_SKIP_INSTALL ON)
    add_subdirectory("${SPIRV_CROSS_DIR}"
                     "${CMAKE_BINARY_DIR}/third_party/spirv-cross" EXCLUDE_FROM_ALL)
  endif()
endfunction()
