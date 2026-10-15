#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Entry point for the OpenGL/WGL/GLX/EGL code generator. Builds the
# intermediate representation (see intermediates.py/intermediates_creator.py)
# from the XML registries and will eventually drive template-based generation
# of the new layered backend.

from pathlib import Path

from command_ids import build_command_ids
from generator_enums import generate_enum_files
from generator_interceptor import build_interceptor_command_list, generate_interceptor_files
from generator_layer_interface import generate_layer_interface_files, generate_pname_value_count_file
from generator_player import generate_player_files
from generator_plugins import generate_plugin_files
from generator_recorder import generate_recorder_files
from intermediates import Api, Command, EnumValue
from intermediates_creator import (
    FILE_TO_API,
    build_enum_name_map,
    get_bitmask_groups,
    load_registries_and_extract_xml_data,
)
from pointer_length_derivation import find_pointer_length_mismatches
from xml_data_exploration import (
    check_function_groups_against_enums,
    detect_duplicate_enum_values,
    inspect_cross_file_enums,
    inspect_enums,
    inspect_function_params,
    inspect_functions,
)


def main() -> None:
    """Build the OpenGL/WGL/GLX/EGL intermediate representation from the XML registries."""
    inspect_data: bool = False  # Disabled by default because of long output.

    enums: dict[Api, list[EnumValue]]
    commands: dict[Api, list[Command]]
    enums, commands = load_registries_and_extract_xml_data(FILE_TO_API)

    # Cross-validate every hand-authored POINTER_PARAM_OVERRIDES/ARRAY_OF_ARRAYS_PARAM_OVERRIDES
    # entry's length-source against what gl.xml's own len= attribute would independently derive
    # for the same (command, param) - pure lint, never changes what gets generated (see
    # pointer_length_derivation.py's find_pointer_length_mismatches() for what "mismatch" can
    # and cannot mean here).
    pointer_length_mismatches = find_pointer_length_mismatches(commands)
    if pointer_length_mismatches:
        print(f'WARNING: {len(pointer_length_mismatches)} pointer length-source mismatch(es) '
              f'against gl.xml len=:')
        for mismatch in pointer_length_mismatches:
            print(f'  {mismatch}')

    # IDs/typed classes are generated from the same post-rename, filtered command list the
    # interceptor and recorder wrappers use (GL + real WGL exports, e.g. "wglSwapBuffers" rather
    # than wgl.xml's raw "SwapBuffers" - see _WGL_RENAMED_FROM_UNPREFIXED), not the raw
    # per-registry lists. Using the raw lists here would let an enabled command be generated
    # under a name (e.g. "SwapBuffers") that no wrapper ever emits, while the wrapper's own name
    # (e.g. "wglSwapBuffers") gets no ID/class at all - a mismatch invisible until a wgl.xml
    # command that's both enabled and renamed actually exists (as HDC/HGLRC opaque-handle
    # support first made true for wglSwapBuffers/wglGetPixelFormat). GLX/EGL are absent from
    # this list, but since _INTERCEPTED_APIS excludes them from ever being enabled, they would
    # never contribute an ID or typed class either way.
    interceptor_commands = build_interceptor_command_list(commands)
    command_ids = build_command_ids(interceptor_commands, Path(__file__).with_name('command_ids.json'))
    generate_layer_interface_files(interceptor_commands, command_ids)
    generate_pname_value_count_file(enums)

    # Derived/postprocessed data (enum canonical names, bitmask groups).
    enum_name_map = build_enum_name_map(enums)
    bitmask_groups = get_bitmask_groups(enums)
    print(f'Built enum name map with {len(enum_name_map)} (api, value, group) entries '
          f'and {len(bitmask_groups)} bitmask groups.')
    generate_enum_files(enums, enum_name_map, bitmask_groups)

    if inspect_data:
        # Inspect enum data per API.
        for api, api_enums in enums.items():
            inspect_enums(api, api_enums)
            detect_duplicate_enum_values(api_enums)
        inspect_cross_file_enums(enums)

        # Inspect function data per API.
        for api, api_commands in commands.items():
            inspect_functions(api, api_commands)
            inspect_function_params(api, api_commands)
        check_function_groups_against_enums(commands, enums)

    generate_interceptor_files(interceptor_commands)
    generate_recorder_files(interceptor_commands)
    generate_player_files(interceptor_commands,
                          {key.group for key in enum_name_map if key.group})
    generate_plugin_files(interceptor_commands)


if __name__ == '__main__':
    main()
