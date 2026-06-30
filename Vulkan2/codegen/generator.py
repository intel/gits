#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import os
import argparse
from pathlib import Path

from intermediates_creator import get_xml_root, parse_enums, parse_bitmasks, parse_flags, parse_unions, parse_structures, parse_handles, parse_commands, get_platform_names, postprocess
from command_ids import build_command_ids
from generator_layer import generate_layer_files
from generator_recorder import generate_recorder_files
from generator_coders import generate_coders_files
from generator_player import generate_player_files
from generator_trace import generate_trace_files
from generator_vk_layer import generate_vk_layer_json
from generator_interceptor import generate_interceptor_files
from plugin_generator import generate_plugin_artifacts
from generator_subcapture import generate_subcapture_files

def main():
    parser = argparse.ArgumentParser(description='Generate vulkan files.')
    parser.add_argument('--xml', type=Path, required=True, help='Path to the vk.xml file.')
    parser.add_argument('--output', type=Path, required=True, help='Directory to output generated files.')
    args = parser.parse_args()
    xml_path = args.xml.absolute()
    output_path = args.output.absolute()

    xml_root = get_xml_root(xml_path)

    enums = parse_enums(xml_root)
    bitmasks = parse_bitmasks(xml_root)
    flags = parse_flags(xml_root)
    unions = parse_unions(xml_root)
    structures = parse_structures(xml_root)
    handles = parse_handles(xml_root)
    commands = parse_commands(xml_root)

    excluded_names, platform_map = get_platform_names(xml_root)

    postprocess(commands, structures, unions, handles, enums, bitmasks, flags, excluded_names, platform_map)

    context = {
        'enums': enums,
        'bitmasks': bitmasks,
        'flags': flags,
        'unions': unions,
        'structures': structures,
        'handles': handles,
        'commands': commands
    }

    context['command_ids'] = build_command_ids(context, os.path.join(output_path, 'codegen'))

    generate_layer_files(context, os.path.join(output_path, 'common/layer_interface'))
    generate_coders_files(context, os.path.join(output_path, 'common/coders'))
    generate_recorder_files(context, os.path.join(output_path, 'recorder'))
    generate_player_files(context, os.path.join(output_path, 'player'))
    generate_trace_files(context, os.path.join(output_path, 'layers/trace'))
    generate_subcapture_files(context, os.path.join(output_path, 'subcapture'))
    generate_vk_layer_json(context, os.path.join(output_path, 'layer'))
    generate_interceptor_files(context, os.path.join(output_path, 'interceptor'))

    plugin_directories = [
        os.path.join(output_path, '../plugins/Vulkan'),
        os.path.join(output_path, '../plugins/internal/Vulkan')
    ]
    generate_plugin_artifacts(context, plugin_directories)

if __name__ == "__main__":
    main()
