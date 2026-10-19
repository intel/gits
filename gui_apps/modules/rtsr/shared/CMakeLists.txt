# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Code shared between the rtsr module's ui and its gitsPlugins: the RTSR wire
# protocol, its transport, and D3D12 pipeline state serialization. Deliberately
# independent of both the GITS GUI and the GITS DirectX internals, because it is
# linked into the plugin DLL as the client and into the module as the server.

set(RTSR_SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND RTSR_SHARED_SOURCE
  ${RTSR_SHARED_DIR}/rtsrCommon.h
  ${RTSR_SHARED_DIR}/rtsrCommon.cpp

  ${RTSR_SHARED_DIR}/protocol/rtsrWireTypes.h
  ${RTSR_SHARED_DIR}/protocol/rtsrMessage.h
  ${RTSR_SHARED_DIR}/protocol/rtsrMessage.cpp
  ${RTSR_SHARED_DIR}/protocol/rtsrSerialization.h
  ${RTSR_SHARED_DIR}/protocol/rtsrSerialization.cpp

  ${RTSR_SHARED_DIR}/transport/rtsrTransport.h
  ${RTSR_SHARED_DIR}/transport/rtsrTransport.cpp

  ${RTSR_SHARED_DIR}/client/rtsrClient.h
  ${RTSR_SHARED_DIR}/client/rtsrClient.cpp

  ${RTSR_SHARED_DIR}/server/rtsrSessionStore.h
  ${RTSR_SHARED_DIR}/server/rtsrSessionStore.cpp
  ${RTSR_SHARED_DIR}/server/rtsrServer.h
  ${RTSR_SHARED_DIR}/server/rtsrServer.cpp
)

# The protocol and transport are API agnostic; only the pipeline state
# serialization needs the D3D12 headers.
if(WITH_DIRECTX)
  list(APPEND RTSR_SHARED_SOURCE
    ${RTSR_SHARED_DIR}/serialization/rtsrSerdeTypes.h
    ${RTSR_SHARED_DIR}/serialization/rtsrPipelineStateStorage.h
    ${RTSR_SHARED_DIR}/serialization/rtsrJsonSerialization.h
    ${RTSR_SHARED_DIR}/serialization/rtsrJsonSerialization.cpp
    ${RTSR_SHARED_DIR}/serialization/rtsrJsonDeserialization.h
    ${RTSR_SHARED_DIR}/serialization/rtsrJsonDeserialization.cpp
    ${RTSR_SHARED_DIR}/serialization/rtsrStateSerialization.h
    ${RTSR_SHARED_DIR}/serialization/rtsrStateSerialization.cpp
  )
endif()

add_library(rtsr_module_shared STATIC ${RTSR_SHARED_SOURCE})

source_group(TREE ${RTSR_SHARED_DIR} FILES ${RTSR_SHARED_SOURCE})

# Headers are addressed as "rtsr/shared/<folder>/<file>", the same convention the
# module's ui uses, so the include root is the modules directory.
target_include_directories(rtsr_module_shared
PUBLIC
  ${GUI_MODULES_DIR}
)

if(WITH_DIRECTX)
  # AGILITY_SDK_DIR is set at root scope by import_agilitysdk. D3D12_HEADERS_DIR
  # is not usable here: it is set inside the DirectX and plugins directories.
  target_include_directories(rtsr_module_shared PRIVATE ${AGILITY_SDK_DIR}/include)
endif()

# Asio needs a Windows API level. Scoped to this target so the rest of the build
# keeps whatever the Windows SDK defaults to.
if(WIN32)
  target_compile_definitions(rtsr_module_shared PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(rtsr_module_shared PRIVATE xxhash)

target_compile_options(rtsr_module_shared PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:
    /wd4267  # size_t conversion warnings
  >
)

set_target_properties(rtsr_module_shared PROPERTIES FOLDER "${GUI_MODULES_FOLDER}/rtsr")
