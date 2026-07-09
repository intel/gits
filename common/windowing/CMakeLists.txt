# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(WINDOWING_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(WINDOWING_HEADER_DIR ${WINDOWING_SOURCE_DIR}/include)

set(WINDOWING_SRC_HEADERS
  ${WINDOWING_HEADER_DIR}/windowManager.h
  ${WINDOWING_HEADER_DIR}/playerWindow.h
)

set(WINDOWING_SRC_SOURCES
  ${WINDOWING_SOURCE_DIR}/windowManager.cpp
  ${WINDOWING_SOURCE_DIR}/playerWindowX11Shared.cpp
  ${WINDOWING_SOURCE_DIR}/playerWindowXlib.cpp
  ${WINDOWING_SOURCE_DIR}/playerWindowXcb.cpp
  ${WINDOWING_SOURCE_DIR}/playerWindowWayland.cpp
)

add_library(windowing STATIC ${WINDOWING_SRC_HEADERS} ${WINDOWING_SRC_SOURCES})
set_target_properties(windowing PROPERTIES FOLDER "common")

target_include_directories(windowing
PUBLIC
  ${WINDOWING_HEADER_DIR}
PRIVATE
  ${WINDOWING_SOURCE_DIR}
)

target_link_libraries(windowing PRIVATE
  log
  configuration
)

if(UNIX)
  find_program(WAYLAND_SCANNER wayland-scanner)
  if(NOT WAYLAND_SCANNER)
	message(FATAL_ERROR "wayland-scanner not found")
  endif()

  find_path(WAYLAND_PROTOCOLS_DIR
	NAMES stable/xdg-shell/xdg-shell.xml
	PATHS
	  /usr/share/wayland-protocols
	  /usr/local/share/wayland-protocols
  )
  if(NOT WAYLAND_PROTOCOLS_DIR)
	message(FATAL_ERROR "wayland-protocols not found")
  endif()

  set(XDG_SHELL_XML ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml)
  set(XDG_SHELL_HEADER ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h)
  set(XDG_SHELL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c)

  add_custom_target(generate_xdg_shell_protocol
	COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
	COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_SOURCE}
	BYPRODUCTS
	  ${XDG_SHELL_HEADER}
	  ${XDG_SHELL_SOURCE}
	COMMENT "Generating xdg-shell protocol files"
  )

  target_sources(windowing PRIVATE
	${XDG_SHELL_SOURCE}
	${XDG_SHELL_HEADER}
  )

  target_include_directories(windowing PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
  )

  add_dependencies(windowing generate_xdg_shell_protocol)

  target_link_libraries(windowing PRIVATE
	X11
	xcb
	X11-xcb
	wayland-client
	xkbcommon
	dl
  )
endif()
