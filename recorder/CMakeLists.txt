# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(recorder_DEF )
if(MSVC)
  set(recorder_DEF ${CMAKE_CURRENT_BINARY_DIR}/recorder.def)
  file(WRITE ${recorder_DEF}
    "LIBRARY gitsRecorder.dll\n"
    "EXPORTS\n"
    "  PrintHandlerGet\n"
    "  Configure\n"
    "  GITSRecorderOpenGL\n")
  if(WITH_OPENCL)
    file(APPEND ${recorder_DEF}
      "  GITSRecorderOpenCL\n")
  endif()
  if(WITH_VULKAN)
    file(APPEND ${recorder_DEF}
      "  GITSRecorderVulkan\n")
  endif()
  if(WITH_LEVELZERO)
    file(APPEND ${recorder_DEF}
      "  GITSRecorderL0\n")
  endif()
  if(WITH_OCLOC)
    file(APPEND ${recorder_DEF}
      "  GITSRecorderOcloc\n")
  endif()
  if(WITH_DIRECTX)
    file(APPEND ${recorder_DEF}
      "  GITSRecorderD3D12\n")
  endif()
endif()

list(APPEND recorder_SOURCES
  include/recorderIface.h
  include/recorderUtils.h
  recorderIface.cpp
  recorderDLL.cpp
  recorderUtils.cpp
  ${recorder_DEF}
)

add_library(GitsRecorder SHARED ${recorder_SOURCES})

add_definitions(-DGITS_RECORDER_EXPORTS)

set(recorderIfaceLibs )
set(recorderCommonLibs configuration)
set(frameworks )

set(libPrefix )

if(UNIX)
  set(recorderIfaceLibs ${recorderIfaceLibs} -Wl,-whole-archive)
endif()

set(recorderIfaceLibs ${recorderIfaceLibs} ${libPrefix} OpenGL_recorder)
set(recorderCommonLibs ${recorderCommonLibs} OpenGL_common)
set(frameworks ${frameworks} "-framework OpenGL")

if(WITH_VULKAN)
  set(recorderIfaceLibs ${recorderIfaceLibs} ${libPrefix} Vulkan_recorder)
  set(recorderCommonLibs ${recorderCommonLibs} Vulkan_common)
endif()
if(WITH_OPENCL)
  set(recorderIfaceLibs ${recorderIfaceLibs} ${libPrefix} OpenCL_recorder)
  set(recorderCommonLibs ${recorderCommonLibs} OpenCL_common)
  set(frameworks ${frameworks} "-framework OpenCL")
endif()
if(WITH_LEVELZERO)
  set(recorderIfaceLibs ${recorderIfaceLibs} ${libPrefix} L0_recorder)
  set(recorderCommonLibs ${recorderCommonLibs} L0_common)
endif()
if(WITH_OCLOC)
  set(recorderIfaceLibs ${recorderIfaceLibs} ${libPrefix} ocloc_recorder)
  set(recorderCommonLibs ${recorderCommonLibs} ocloc_common)
endif()
if(WITH_DIRECTX)
  set(recorderIfaceLibs 
  ${recorderIfaceLibs}
  ${libPrefix}
  d3d12.lib
  dxgi.lib
  DirectX_recorder
  detours
  directxtex
  ${DML_DIR}/bin/x64-win/DirectML.lib
  ${DSTORAGE_DIR}/native/lib/x64/dstorage.lib
  )
endif()

if(UNIX)
  set(recorderIfaceLibs ${recorderIfaceLibs} -Wl,-no-whole-archive)
endif()

add_dependencies(GitsRecorder config_codegen)
target_link_libraries(GitsRecorder ${recorderIfaceLibs} ${recorderCommonLibs} common png lua zlib)

if(UNIX)
  target_link_libraries(GitsRecorder pthread rt xcb X11-xcb X11 GL wayland-client wayland-egl)
elseif(MSVC)
  target_link_libraries(GitsRecorder OpenGL32)
endif()

if(WIN32)
  set_target_properties(GitsRecorder PROPERTIES OUTPUT_NAME "gitsRecorder")
endif()

install(TARGETS GitsRecorder
  RUNTIME DESTINATION Recorder
  LIBRARY DESTINATION Recorder)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:GitsRecorder> DESTINATION Recorder OPTIONAL)
endif()
if(UNIX)
  install(CODE "execute_process(COMMAND bash -c \"
    cd \${CMAKE_INSTALL_PREFIX}/Recorder/${OpenGL_INSTALL_SUBDIR}/
    ln -sf libGitsGLCapture.so libEGL.so.1
    ln -sf libGitsGLCapture.so libGLESv1_CM.so.1
    ln -sf libGitsGLCapture.so libGLESv2.so.2
    ln -sf libGitsGLCapture.so libGL.so.1
    cd ../${OpenCL_INSTALL_SUBDIR}/
    ln -sf libGitsGLCapture.so libEGL.so.1
    ln -sf libGitsGLCapture.so libGLESv1_CM.so.1
    ln -sf libGitsGLCapture.so libGLESv2.so.2
    ln -sf libGitsGLCapture.so libGL.so.1
    cd ../${Vulkan_INSTALL_SUBDIR}/
    ln -sf libVulkan.so libvulkan.so.1
  \")")
endif()
