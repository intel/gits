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

# Intel Extensions
install(DIRECTORY third_party/IntelExtensions/ DESTINATION CCode/third_party/IntelExtensions)

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

# STB Image
install(FILES 
  third_party/stb/stb_image.h
  third_party/stb/stb_image_write.h
  third_party/stb/README.md
  third_party/stb/LICENSE
  DESTINATION CCode/third_party/stb/
)

# NVAPI
install(DIRECTORY third_party/nvapi/
  DESTINATION CCode/third_party/nvapi
  PATTERN ".git" EXCLUDE
  PATTERN ".git/*" EXCLUDE
  PATTERN "Sample_Code" EXCLUDE
  PATTERN "Sample_Code/*" EXCLUDE
  PATTERN "docs" EXCLUDE
  PATTERN "docs/*" EXCLUDE
  PATTERN "x86" EXCLUDE
  PATTERN "x86/*" EXCLUDE
)

# DirectStorage
install(DIRECTORY third_party/DirectStorage/native/include/
  DESTINATION CCode/third_party/DirectStorage/native/include)
install(DIRECTORY third_party/DirectStorage/native/lib/x64/
  DESTINATION CCode/third_party/DirectStorage/native/lib/x64
  FILES_MATCHING PATTERN "*.lib")
install(DIRECTORY third_party/DirectStorage/native/bin/x64/
  DESTINATION CCode/third_party/DirectStorage/native/bin/x64
  FILES_MATCHING PATTERN "*.dll")
install(FILES third_party/DirectStorage/LICENSE.txt
  DESTINATION CCode/third_party/DirectStorage)

# XeSS / XeLL / XeFG
install(DIRECTORY third_party/xess/inc/
  DESTINATION CCode/third_party/xess/inc
  PATTERN "xess/xess_d3d11.h" EXCLUDE
  PATTERN "xess/xess_vk.h" EXCLUDE
  PATTERN "xess/xess_vk_debug.h" EXCLUDE
)
install(DIRECTORY third_party/xess/lib/
  DESTINATION CCode/third_party/xess/lib
  FILES_MATCHING PATTERN "*.lib"
  PATTERN "libxess_dx11.lib" EXCLUDE
)
install(DIRECTORY third_party/xess/bin/
  DESTINATION CCode/third_party/xess/bin
  FILES_MATCHING PATTERN "*.dll"
  PATTERN "libxess_dx11.dll" EXCLUDE
  PATTERN "basic_sample_shaders" EXCLUDE
  PATTERN "basic_sample_shaders/*" EXCLUDE
)
install(FILES third_party/xess/LICENSE.txt DESTINATION CCode/third_party/xess)