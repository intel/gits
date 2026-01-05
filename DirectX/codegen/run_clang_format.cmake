# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

file(GLOB_RECURSE FILES
  "${OUT_DIR}/*Auto.cpp"
  "${OUT_DIR}/*Auto.h"
)

if(FILES)
  execute_process(
    COMMAND ${CLANG_FORMAT} -i -style=file ${FILES}
  )
endif()
