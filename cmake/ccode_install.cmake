# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Copy CCode project template
install(DIRECTORY ccode/ DESTINATION CCode)
install(FILES .clang-format DESTINATION CCode)

# Copy third-party dependencies

# Plog
install(DIRECTORY third_party/plog/include/ DESTINATION CCode/third_party/plog/include)
install(FILES 
  third_party/plog/README.md
  third_party/plog/LICENSE
  DESTINATION CCode/third_party/plog/
)

# ArgsHxx
install(FILES 
  third_party/argshxx/args.hxx
  third_party/argshxx/README.md
  third_party/argshxx/LICENSE
  DESTINATION CCode/third_party/argshxx/
)

# Agility SDK
set(AGILITY_SDK_DIRS
  include
  src
  bin/x64
)
foreach(DIR ${AGILITY_SDK_DIRS})
  install(DIRECTORY third_party/AgilitySdk/${DIR}/
    DESTINATION CCode/third_party/AgilitySdk/${DIR}
	PATTERN "*.pdb" EXCLUDE
  )
endforeach()
install(FILES 
  third_party/AgilitySdk/README.md
  third_party/AgilitySdk/LICENSE.txt
  third_party/AgilitySdk/LICENSE-CODE.txt
  DESTINATION CCode/third_party/AgilitySdk/
)
