#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os

from generator_helpers import generate_file

# Commands recorded into command buffers that also change the state the resources dump tracks,
# implemented in resourcesDumpLayerCustom.cpp.
RESOURCES_DUMP_LAYER_CUSTOM_COMMANDS = [
    'vkCmdPipelineBarrier',
    'vkCmdPipelineBarrier2',
    'vkCmdPipelineBarrier2KHR',
    'vkCmdWaitEvents',
    'vkCmdWaitEvents2',
    'vkCmdWaitEvents2KHR',
    'vkCmdBeginRenderPass',
    'vkCmdBeginRenderPass2',
    'vkCmdBeginRenderPass2KHR',
    'vkCmdEndRenderPass',
    'vkCmdEndRenderPass2',
    'vkCmdEndRenderPass2KHR',
    'vkCmdBeginRendering',
    'vkCmdBeginRenderingKHR',
    'vkCmdEndRendering',
    'vkCmdEndRenderingKHR',
    'vkCmdEndRendering2EXT',
    'vkCmdEndRendering2KHR',
    'vkCmdExecuteCommands',
    'vkCmdBeginVideoCodingKHR',
    'vkCmdEndVideoCodingKHR',
]


def is_command_buffer_command(command):
    return (command.name.startswith('vkCmd') and command.params and
            command.params[0].base_type == 'VkCommandBuffer')


def generate_resource_dumping_files(context, out_path):
    additional_context = {
        'is_command_buffer_command': is_command_buffer_command,
        'custom_commands': RESOURCES_DUMP_LAYER_CUSTOM_COMMANDS,
    }
    os.makedirs(out_path, exist_ok=True)
    files_to_generate = [
        'resourcesDumpLayerAuto.h',
        'resourcesDumpLayerAuto.cpp',
    ]

    for file_name in files_to_generate:
        generate_file(context | additional_context, file_name, out_path)
