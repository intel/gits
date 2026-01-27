# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.13)
project(gitsCCode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ARGS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/argshxx/)
set(PLOG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/plog/)
set(DX12_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/AgilitySDK/)

# Set the executable name from config.cmake if it exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/config.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/generated/config.cmake")
else()
    set(EXECUTABLE_NAME "gitsCCode")
    message(WARNING "config.cmake not found, using defaults")
endif()

# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EXECUTABLE_NAME})

file(GLOB GENERATED_SRC 
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.h")
source_group("Generated" FILES ${GENERATED_SRC})

add_executable(${EXECUTABLE_NAME}
  main.cpp
  dataService.h
  dataService.cpp
  windowService.h
  windowService.cpp
  directx/directx.h
  directx/utils.h
  directx/utils.cpp
  directx/descriptorHeapService.h
  directx/descriptorHeapService.cpp
  directx/gpuAddressService.h
  directx/gpuAddressService.cpp
  directx/mapTrackingService.h
  directx/mapTrackingService.cpp
  directx/heapAllocationService.h
  directx/heapAllocationService.cpp
  ${GENERATED_SRC}
)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
  DATA_BIN_PATH="${CMAKE_CURRENT_SOURCE_DIR}/generated/data.bin"
)

target_include_directories(${EXECUTABLE_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PLOG_SOURCE_DIR}/include
  ${DX12_SOURCE_DIR}/include
  ${ARGS_SOURCE_DIR}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
  "d3d12.lib" 
  "dxguid.lib" 
  "dxgi.lib" 
)

if(MSVC)
  target_compile_options(${EXECUTABLE_NAME} PRIVATE 
    /bigobj
    /GS-
    /MP
  )
  target_link_options(${EXECUTABLE_NAME} PRIVATE 
    /INCREMENTAL:NO # Disable incremental linking
  )
endif()

# Copy D3D12 runtime files
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/D3D12"
        "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/D3D12"
    COMMENT "Copying D3D12 runtime files to executable directory"
)

# Use clang-format to format generated files on first configuration
find_program(PYTHON "python3")
if(NOT PYTHON)
    find_program(PYTHON "python")
endif()
if(PYTHON)
  set(FORMAT_STAMP_FILE "${CMAKE_BINARY_DIR}/format_done.stamp")
  if(NOT EXISTS ${FORMAT_STAMP_FILE})
    message(STATUS "Formatting all the generated C++ files...")
    execute_process(
      COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/clang_format.py
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE FORMAT_RESULT
      OUTPUT_VARIABLE FORMAT_OUTPUT
      ERROR_VARIABLE FORMAT_ERROR
    )
    if(FORMAT_RESULT EQUAL 0)
      message(STATUS "Formatting completed successfully.")
      if(FORMAT_OUTPUT)
        message(STATUS "${FORMAT_OUTPUT}")
      endif()
      file(WRITE ${FORMAT_STAMP_FILE} "Formatted on: ${CMAKE_CURRENT_LIST_FILE}")
    else()
      message(WARNING "Formatting failed: ${FORMAT_ERROR}")
    endif()
  else()
    message(STATUS "Generated files already formatted in this build directory")
  endif()
else()
  message(WARNING "Python not found. Generated files will not be formatted.")
endif()