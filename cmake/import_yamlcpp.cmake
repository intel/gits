# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED YAMLCPP_ABS_PATH)
  install_dependencies("--with-yamlcpp")
  set(YAMLCPP_ABS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp")
endif()

add_definitions(-DYAML_CPP_BUILD_CONTRIB=OFF -DYAML_CPP_BUILD_TOOLS=OFF -DBUILD_SHARED_LIBS=OFF -DYAML_BUILD_SHARED_LIBS=OFF -DYAML_CPP_INSTALL=OFF -DYAML_CPP_FORMAT_SOURCE=OFF -DYAML_CPP_STATIC_DEFINE)

set(src-pattern "${YAMLCPP_ABS_PATH}/src/*.cpp")
file(GLOB yaml-cpp-sources ${src-pattern})

add_library(yamlcpp
  ${yaml-cpp-sources}
)

include_directories(SYSTEM "${YAMLCPP_ABS_PATH}/include")
link_libraries(yamlcpp)

set_target_properties(yamlcpp PROPERTIES FOLDER External)
