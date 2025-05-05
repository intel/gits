# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

SET(GENERATED_FILES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

SET(GENERATED_FILES
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
set_target_properties(config_codegen PROPERTIES FOLDER config)

SET(META_DIR ${CMAKE_CURRENT_SOURCE_DIR}/metafiles)
SET(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
SET(TEMPLATES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/templates)

SET(META_SRC
  ${META_DIR}/config.yml
  ${META_DIR}/enums.yml
)
source_group("meta" FILES ${META_SRC})

set(SCRIPTS_SRC
  ${SCRIPTS_DIR}/configuration_element.py
  ${SCRIPTS_DIR}/configuration_enum.py
  ${SCRIPTS_DIR}/generate.py
  ${SCRIPTS_DIR}/generators.py
  ${SCRIPTS_DIR}/generator_step.py
  ${SCRIPTS_DIR}/generator_task.py
  ${SCRIPTS_DIR}/template_manager.py
  ${SCRIPTS_DIR}/utils.py
  ${SCRIPTS_DIR}/__init__.py
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

set(METAFILE_CONFIG ${META_DIR}/config.yml)
set(METAFILE_ENUM ${META_DIR}/enums.yml)

SET(DOCUMENTATION_DIR ${CMAKE_SOURCE_DIR}/docs)

add_custom_command(
  OUTPUT ${GENERATED_FILES}
  COMMAND ${CMAKE_COMMAND} -E echo "Running configuration code generation script..."
  DEPENDS ${DEPENDENT_FILES}
  COMMAND ${Python3_EXECUTABLE} ${SCRIPTS_DIR}/generate.py Enum ${METAFILE_CONFIG} ${METAFILE_ENUM} ${GENERATED_FILES_DIR}
  COMMAND ${Python3_EXECUTABLE} ${SCRIPTS_DIR}/generate.py Configuration ${METAFILE_CONFIG} ${METAFILE_ENUM} ${GENERATED_FILES_DIR}
  COMMAND ${Python3_EXECUTABLE} ${SCRIPTS_DIR}/generate.py Argumentparser ${METAFILE_CONFIG} ${METAFILE_ENUM} ${GENERATED_FILES_DIR}
  WORKING_DIRECTORY ${SCRIPTS_DIR}
)

# add_custom_command(generate_docs_config_enums
# OUTPUT ${GENERATED_FILES}
# COMMAND ${CMAKE_COMMAND} -E echo "Generate documentation for configuration and enums."
# DEPENDS ${DEPENDENT_FILES}
# COMMAND ${Python3_EXECUTABLE} ${SCRIPTS_DIR}/generate.py DocsEnum ${METAFILE_CONFIG} ${METAFILE_ENUM} ${DOCUMENTATION_DIR}
# COMMAND ${Python3_EXECUTABLE} ${SCRIPTS_DIR}/generate.py DocsConfiguration ${METAFILE_CONFIG} ${METAFILE_ENUM} ${DOCUMENTATION_DIR}
# WORKING_DIRECTORY ${SCRIPTS_DIR}
# )
