# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED GLSLANG_DIR)
  add_thirdparty_arg("--with-glslang")
  set(GLSLANG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glslang")
endif()

# Studio-side GLSL compilation only. The dependency is initialized explicitly
# by rtsr_module_toolchain so it never reaches either GITS plugin DLL.
function(init_glslang)
  if(NOT TARGET glslang)
    if(CMAKE_VERSION VERSION_LESS 3.22.1)
      message(FATAL_ERROR "RTSR GLSL support requires CMake 3.22.1 or newer")
    endif()

    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    # dxil-spirv already supplied the matching SPIR-V headers. Build its pinned
    # SPIRV-Tools checkout once and let glslang use that target instead of
    # downloading/configuring another copy.
    if(NOT TARGET SPIRV-Tools-static)
      set(SPIRV_SKIP_TESTS ON)
      set(SPIRV_WERROR OFF)
      set(SKIP_SPIRV_TOOLS_INSTALL ON)
      add_subdirectory(
        "${DXIL_SPIRV_DIR}/third_party/SPIRV-Tools"
        "${CMAKE_BINARY_DIR}/third_party/spirv-tools"
        EXCLUDE_FROM_ALL
      )
    endif()

    set(BUILD_EXTERNAL OFF)
    set(BUILD_TESTING OFF)
    set(ENABLE_CTEST OFF)
    set(ENABLE_GLSLANG_BINARIES OFF)
    set(ENABLE_GLSLANG_JS OFF)
    set(ENABLE_HLSL OFF)
    set(ENABLE_OPT ON)
    set(ENABLE_PCH OFF)
    set(ENABLE_SPVREMAPPER OFF)
    set(ALLOW_EXTERNAL_SPIRV_TOOLS ON)
    set(SKIP_GLSLANG_INSTALL ON)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(
      "${GLSLANG_DIR}"
      "${CMAKE_BINARY_DIR}/third_party/glslang"
      EXCLUDE_FROM_ALL
    )
  endif()

  # SPIRV-Tools is added as its own add_subdirectory (not nested under
  # glslang's tree), so assign both roots into External.
  gits_assign_folder_recursive(
    "${DXIL_SPIRV_DIR}/third_party/SPIRV-Tools" "External")
  gits_assign_folder_recursive("${GLSLANG_DIR}" "External")
endfunction()
