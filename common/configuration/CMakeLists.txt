# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

cmake_minimum_required(VERSION 3.10)

project(configuration)

set(CPP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

add_subdirectory(codegen)

if(NOT DEFINED BUILD_FOR_CCODE)
  set(CPP_FILES
    ${CPP_SRC}/configurator.cpp
    ${CPP_SRC}/customTypes.cpp
    ${CPP_SRC}/customTypesYAML.cpp
    ${CPP_SRC}/deriveData.cpp
    ${CPP_SRC}/helper.cpp
    ${CPP_SRC}/stringFromType.cpp
    ${CPP_SRC}/stringToType.cpp
  )
  set(HEADER_FILES
    ${HEADER_SRC}/argumentParser.h
    ${HEADER_SRC}/argumentValidators.h
    ${HEADER_SRC}/configurationLib.h
    ${HEADER_SRC}/configurator.h
    ${HEADER_SRC}/customTypes.h
    ${HEADER_SRC}/customTypesYAML.h
    ${HEADER_SRC}/deriveData.h
    ${HEADER_SRC}/helper.h
    ${HEADER_SRC}/stringFromType.h
    ${HEADER_SRC}/stringToType.h
    ${ARGS_SOURCE_DIR}/args.hxx
  )
  set(AUTO_FILES
    ${CPP_SRC}/argumentsAuto.cpp
    ${CPP_SRC}/configurationAuto.cpp
    ${CPP_SRC}/configurationYAMLAuto.cpp
    ${CPP_SRC}/enumsAuto.cpp
    ${CPP_SRC}/enumsYAMLAuto.cpp

    ${HEADER_SRC}/argumentsAuto.h
    ${HEADER_SRC}/configurationAuto.h
    ${HEADER_SRC}/configurationYAMLAuto.h
    ${HEADER_SRC}/enumsAuto.h
    ${HEADER_SRC}/enumsYAMLAuto.h
  )
else()
  set(CPP_FILES
    ${CPP_SRC}/configurator.cpp
    ${CPP_SRC}/customTypes.cpp
    ${CPP_SRC}/helper.cpp
    ${CPP_SRC}/stringFromType.cpp
    ${CPP_SRC}/stringToType.cpp
  )
  set(HEADER_FILES
    ${HEADER_SRC}/configurator.h
    ${HEADER_SRC}/customTypes.h
    ${HEADER_SRC}/helper.h
    ${HEADER_SRC}/configurationLib.h
    ${HEADER_SRC}/stringFromType.h
    ${HEADER_SRC}/stringToType.h
  )
  set(AUTO_FILES
    ${CPP_SRC}/configurationAuto.cpp
    ${CPP_SRC}/enumsAuto.cpp

    ${HEADER_SRC}/include/configurationAuto.h
    ${HEADER_SRC}/include/enumsAuto.h
  )
endif()

set_source_files_properties(${AUTO_FILES} PROPERTIES GENERATED 1)

source_group("Generated Files" REGULAR_EXPRESSION ".*Auto.*")

add_library(configuration STATIC ${CPP_FILES} ${HEADER_FILES} ${AUTO_FILES})
add_dependencies(configuration config_codegen)

set_target_properties(configuration PROPERTIES FOLDER "common/config")

SET(PRIVATE_INCLUDE_DIRS)

if(NOT DEFINED BUILD_FOR_CCODE)
  target_link_libraries(configuration PRIVATE yamlcpp)
  target_link_libraries(configuration PUBLIC common)
  SET(PRIVATE_INCLUDE_DIRS ${ARGS_SOURCE_DIR})
endif()

target_include_directories(configuration
  PUBLIC
  $<BUILD_INTERFACE:${HEADER_SRC}>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${PRIVATE_INCLUDE_DIRS}
)
