#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file
import platform

def generate_vk_layer_json(context, output_path):
    vk_layer_bin_path = ''
    match platform.system():
        case 'Windows':
            vk_layer_bin_path = '.\\\\VkLayer_vulkan_GITS_recorder.dll'
        case 'Linux':
            vk_layer_bin_path = './libVkLayer_vulkan_GITS_recorder.so'
        case other_system:
            raise NotImplementedError(f"Path to Vulkan layer dynamic library is unknown for system '{other_system}'.")
            
    additional_context = {
        'vk_layer_bin_path': vk_layer_bin_path
    }
    
    files_to_generate = [
        'VkLayer_vulkan_GITS_recorder.json'
    ]
    
    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, output_path)
