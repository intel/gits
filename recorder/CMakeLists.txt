# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(control_handler )
set(recorder_DEF )
if(MSVC)
  set(control_handler controlHandler.cpp)
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
endif()

add_library(GitsRecorder SHARED
  include/gitsLoader.h
  include/recorder.h
  include/recorderBehaviors.h
  include/recorderIface.h
  include/state.h

  recorder.cpp
  recorderBehaviors.cpp
  recorderIface.cpp
  state.cpp
  ${control_handler}
  ${recorder_DEF}
)

add_definitions(-DGITS_RECORDER_EXPORTS)

set(recorderIfaceLibs )
set(recorderCommonLibs )
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

if(UNIX)
  set(recorderIfaceLibs ${recorderIfaceLibs} -Wl,-no-whole-archive)
endif()

target_link_libraries(GitsRecorder ${recorderIfaceLibs} ${recorderCommonLibs} common png lua zlib)

if(UNIX)
  target_link_libraries(GitsRecorder pthread rt xcb X11-xcb X11 GL wayland-client wayland-egl)
elseif(MSVC)
  target_link_libraries(GitsRecorder OpenGL32)
endif()

if(WIN32)
  set(PLATFORM "win32")
  set_target_properties(GitsRecorder PROPERTIES OUTPUT_NAME "gitsRecorder")
else()
  if(${ARCH} STREQUAL "-m32")
    set(PLATFORM "lnx_32")
  elseif(${ARCH} STREQUAL "-m64")
    set(PLATFORM "lnx_64")
  else()
    set(PLATFORM "lnx_arm")
  endif()
endif()

install(TARGETS GitsRecorder
  RUNTIME DESTINATION Recorder
  LIBRARY DESTINATION Recorder)

set(CONFIG_GENERATOR_DIR "${CMAKE_SOURCE_DIR}/Scripts/ConfigGenerator")
set(GITS_CONFIG "gits_config.txt")
set(CONFIG_OUTPUT "${CONFIG_GENERATOR_DIR}/${GITS_CONFIG}")
set(CONFIG_GENERATOR "${CONFIG_GENERATOR_DIR}/gits_config_generator.py")
set(CONFIG_TEMPLATE "${CONFIG_GENERATOR_DIR}/gits_config.mako")
install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR} --templatepath ${CONFIG_TEMPLATE} \
  --outputpath ${CONFIG_OUTPUT} --platform ${PLATFORM} --installpath \${CMAKE_INSTALL_PREFIX})")

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:GitsRecorder> DESTINATION Recorder OPTIONAL)
endif()
install(FILES ${CONFIG_OUTPUT} DESTINATION Recorder/${OpenGL_INSTALL_SUBDIR})
install(FILES ${CONFIG_OUTPUT} DESTINATION Recorder/${OpenGLES_INSTALL_SUBDIR})
if(WITH_OPENCL)
  install(FILES ${CONFIG_OUTPUT} DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR})
endif()
if(WITH_VULKAN)
  install(FILES ${CONFIG_OUTPUT} DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR})
  install(FILES ${CONFIG_OUTPUT} DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR})
endif()
if(WITH_LEVELZERO)
  install(FILES ${CONFIG_OUTPUT} DESTINATION Recorder/${L0_INSTALL_SUBDIR})
endif()

install(CODE "file(REMOVE ${CONFIG_OUTPUT})")

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
