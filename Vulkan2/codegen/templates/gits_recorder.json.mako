## ===================== begin_copyright_notice ============================
##
## Copyright (C) 2023-2026 Intel Corporation
##
## SPDX-License-Identifier: MIT
##
## ===================== end_copyright_notice ==============================
{
    "file_format_version" : "1.2.0",
    "layer" : {
        "name": "VK_LAYER_INTEL_vulkan_GITS_recorder_2",
        "type": "GLOBAL",
        "library_path": "${vk_layer_bin_path}",
        "api_version": "1.4.335",
        "implementation_version": "1",
        "description": "Vulkan layer used to record GITS Vulkan streams",
        "functions": {
            "vkGetInstanceProcAddr": "vkGetInstanceProcAddrGITSLayer",
            "vkGetDeviceProcAddr": "vkGetDeviceProcAddrGITSLayer",
            "vkGetPhysicalDeviceProcAddr": "vkGetPhysicalDeviceProcAddrGITSLayer",
            "vkNegotiateLoaderLayerInterfaceVersion": "vkNegotiateLoaderLayerInterfaceVersionGITSLayer"
        }
    }
}
