# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================


init_dxil_spirv()
init_glslang()

add_library(rtsr_module_toolchain STATIC rtsrShaderToolchain.cpp rtsrShaderToolchain.h)
target_compile_features(rtsr_module_toolchain PRIVATE cxx_std_20)
target_include_directories(rtsr_module_toolchain PUBLIC ${GUI_MODULES_DIR})
target_link_libraries(rtsr_module_toolchain PRIVATE
  dxil-spirv-c-shared
  glslang
  glslang-default-resource-limits
  SPIRV
  SPIRV-Tools-static
  spirv-cross-glsl
  spirv-cross-hlsl
)

# Keep dxil-spirv's private SPIR-V builder behind a DLL boundary; linking it
# statically beside glslang's SPIRV target violates ODR for spv::Builder.
# TARGETS is intentional: on Unix it installs the SONAME symlink chain as well
# as the versioned shared object.
foreach(component gits_studio gits_launcher)
  install(TARGETS dxil-spirv-c-shared
    RUNTIME DESTINATION . COMPONENT ${component}
    LIBRARY DESTINATION . COMPONENT ${component}
  )
  install(FILES ${DXIL_SPIRV_DIR}/LICENSE.MIT
    DESTINATION Licenses/RTSR RENAME LICENSE_dxil-spirv.txt COMPONENT ${component})
  install(FILES ${GLSLANG_DIR}/LICENSE.txt
    DESTINATION Licenses/RTSR RENAME LICENSE_glslang.txt COMPONENT ${component})
  install(FILES ${SPIRV_CROSS_DIR}/LICENSE
    DESTINATION Licenses/RTSR RENAME LICENSE_SPIRV-Cross.txt COMPONENT ${component})
  install(FILES ${DXIL_SPIRV_DIR}/third_party/SPIRV-Tools/LICENSE
    DESTINATION Licenses/RTSR RENAME LICENSE_SPIRV-Tools.txt COMPONENT ${component})
  install(FILES ${DXIL_SPIRV_DIR}/third_party/spirv-headers/LICENSE
    DESTINATION Licenses/RTSR RENAME LICENSE_SPIRV-Headers.txt COMPONENT ${component})
endforeach()

if(WIN32 AND WITH_DIRECTX)
  target_include_directories(rtsr_module_toolchain PRIVATE ${DXC_DIR}/include)
  target_compile_definitions(rtsr_module_toolchain PRIVATE
    RTSR_HAS_DXC NOMINMAX _WIN32_WINNT=0x0A00)
  foreach(component gits_studio gits_launcher)
    install(FILES
      ${DXC_DIR}/bin/x64/dxcompiler.dll
      ${DXC_DIR}/bin/x64/dxil.dll
      DESTINATION Player/D3D12
      COMPONENT ${component}
    )
    install(FILES ${DXC_DIR}/LICENSE-MS.txt
      DESTINATION Player/D3D12
      RENAME LICENSE_DXC_dxil.txt
      COMPONENT ${component}
    )
    install(FILES ${DXC_DIR}/LICENSE-LLVM.txt
      DESTINATION Player/D3D12
      RENAME LICENSE_DXC_dxcompiler.txt
      COMPONENT ${component}
    )
  endforeach()
endif()

set_target_properties(rtsr_module_toolchain PROPERTIES FOLDER "${GUI_MODULES_FOLDER}/rtsr")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/rtsrShaderToolchain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rtsrShaderToolchain.h)
