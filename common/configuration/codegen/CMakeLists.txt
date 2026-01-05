# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_FILES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(GENERATED_FILES
  ${GENERATED_FILES_DIR}/argumentsAuto.cpp
  ${GENERATED_FILES_DIR}/configurationAuto.cpp
  ${GENERATED_FILES_DIR}/configurationYAMLAuto.cpp
  ${GENERATED_FILES_DIR}/enumsAuto.cpp
  ${GENERATED_FILES_DIR}/enumsYAMLAuto.cpp
  ${GENERATED_FILES_DIR}/include/argumentsAuto.h
  ${GENERATED_FILES_DIR}/include/configurationAuto.h
  ${GENERATED_FILES_DIR}/include/enumsAuto.h
  ${GENERATED_FILES_DIR}/include/enumsYAMLAuto.h
)

add_custom_target(config_codegen DEPENDS ${GENERATED_FILES})
set_target_properties(config_codegen PROPERTIES FOLDER "common/config")

set(META_DIR ${CMAKE_CURRENT_SOURCE_DIR}/metafiles)
set(CONFIG_GENERATOR_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts CACHE INTERNAL "Path to the configuration generator scripts")
set(TEMPLATES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/templates)

set(META_SRC
  ${META_DIR}/config.yml
  ${META_DIR}/enums.yml
)
source_group("meta" FILES ${META_SRC})

set(SCRIPTS_SRC
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/configuration_element.py
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/configuration_enum.py
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/generate.py
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/generators.py
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/template_manager.py
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/utils.py
  ${CONFIG_GENERATOR_SCRIPTS_DIR}/__init__.py
)
source_group("scripts" FILES ${SCRIPTS_SRC})

set(MAKO_TEMPLATES
  ${TEMPLATES_DIR}/argumentsAuto.cpp.mako
  ${TEMPLATES_DIR}/argumentsAuto.h.mako
  ${TEMPLATES_DIR}/configurationAuto.cpp.mako
  ${TEMPLATES_DIR}/configurationAuto.h.mako
  ${TEMPLATES_DIR}/configurationYAMLAuto.cpp.mako
  ${TEMPLATES_DIR}/configurationYAMLAuto.h.mako
  ${TEMPLATES_DIR}/default.yaml.mako
  ${TEMPLATES_DIR}/documentationConfigAuto.md.mako
  ${TEMPLATES_DIR}/documentationEnumAuto.md.mako
  ${TEMPLATES_DIR}/enumsAuto.cpp.mako
  ${TEMPLATES_DIR}/enumsAuto.h.mako
  ${TEMPLATES_DIR}/enumsYAMLAuto.cpp.mako
  ${TEMPLATES_DIR}/enumsYAMLAuto.h.mako
)
source_group(mako FILES ${MAKO_TEMPLATES})

set(DEPENDENT_FILES ${META_SRC} ${SCRIPTS_SRC} ${MAKO_TEMPLATES})

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  target_sources(config_codegen PRIVATE
    ${MAKO_TEMPLATES}
    ${SCRIPTS_SRC}
    ${META_SRC}
  )
endif()


set(CONFIG_GENERATOR ${CONFIG_GENERATOR_SCRIPTS_DIR}/generate.py  CACHE INTERNAL "Path to the configuration generator script")
set(METAFILE_CONFIG ${META_DIR}/config.yml CACHE INTERNAL "Path to the configuration metafile")
set(METAFILE_ENUM ${META_DIR}/enums.yml CACHE INTERNAL "Path to the enums metafile")

if(WIN32)
  set(PLATFORM "win32")
else()
  if("${ARCH}" STREQUAL "-m32")
    set(PLATFORM "lnx_32")
  elseif("${ARCH}" STREQUAL "-m64")
    set(PLATFORM "lnx_64")
  else()
    set(PLATFORM "lnx_arm")
  endif()
endif()

add_custom_command(
  OUTPUT ${GENERATED_FILES}
  COMMAND ${CMAKE_COMMAND} -E echo "Running configuration code generation script..."
  DEPENDS ${DEPENDENT_FILES}
  COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR} --step Enum --configYML ${METAFILE_CONFIG} --enumYML ${METAFILE_ENUM} --outDir ${GENERATED_FILES_DIR} --platform ${PLATFORM}
  COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR} --step Configuration --configYML ${METAFILE_CONFIG} --enumYML ${METAFILE_ENUM} --outDir ${GENERATED_FILES_DIR} --platform ${PLATFORM}
  COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR} --step Argumentparser --configYML ${METAFILE_CONFIG} --enumYML ${METAFILE_ENUM} --outDir ${GENERATED_FILES_DIR} --platform ${PLATFORM}
  WORKING_DIRECTORY ${CONFIG_GENERATOR_SCRIPTS_DIR}
)

set(GENERATED_DOCS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../docs/documentation/configuration)

set(GENERATED_DOCS
  ${GENERATED_DOCS_DIR}/CommonAuto.md
  ${GENERATED_DOCS_DIR}/DirectXAuto.md
  ${GENERATED_DOCS_DIR}/EnumsAuto.md
  ${GENERATED_DOCS_DIR}/LevelZeroAuto.md
  ${GENERATED_DOCS_DIR}/OpenCLAuto.md
  ${GENERATED_DOCS_DIR}/OpenGLAuto.md
  ${GENERATED_DOCS_DIR}/VulkanAuto.md
)

add_custom_command(
  OUTPUT ${GENERATED_DOCS}
  COMMAND ${CMAKE_COMMAND} -E echo "Running configuration documentation generation script..."
  DEPENDS ${DEPENDENT_FILES}
  COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR} --step DocsConfiguration --configYML ${METAFILE_CONFIG} --enumYML ${METAFILE_ENUM} --outDir ${GENERATED_DOCS_DIR} --platform ${PLATFORM}
  COMMAND ${Python3_EXECUTABLE} ${CONFIG_GENERATOR} --step DocsEnum --configYML ${METAFILE_CONFIG} --enumYML ${METAFILE_ENUM} --outDir ${GENERATED_DOCS_DIR} --platform ${PLATFORM}
  WORKING_DIRECTORY ${CONFIG_GENERATOR_SCRIPTS_DIR}
)

add_custom_target(config_docs_gen DEPENDS ${GENERATED_DOCS})
set_target_properties(config_docs_gen PROPERTIES FOLDER "common/config")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
  target_sources(config_docs_gen PRIVATE
    ${MAKO_TEMPLATES}
    ${SCRIPTS_SRC}
    ${META_SRC}
  )
endif()

add_dependencies(config_codegen config_docs_gen)
