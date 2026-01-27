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
install(DIRECTORY third_party/plog DESTINATION CCode/third_party)
install(DIRECTORY third_party/argshxx DESTINATION CCode/third_party)
install(DIRECTORY third_party/AgilitySdk DESTINATION CCode/third_party)

# Copy D3D12 runtime files
set(D3D12_RUNTIME_FILES
  ${AGILITY_SDK_DIR}/bin/x64/D3D12Core.dll
  ${AGILITY_SDK_DIR}/bin/x64/d3d12SDKLayers.dll
)

install(FILES
  ${D3D12_RUNTIME_FILES} DESTINATION CCode/D3D12
)
