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

from generator_interceptor import generate_interceptor_files
from intermediates import Api, Command, EnumValue
from intermediates_creator import (
    FILE_TO_API,
    build_enum_name_map,
    get_bitmask_groups,
    load_registries_and_extract_xml_data,
)
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

    # Derived/postprocessed data (enum canonical names, bitmask groups).
    enum_name_map = build_enum_name_map(enums)
    bitmask_groups = get_bitmask_groups(enums)
    print(f'Built enum name map with {len(enum_name_map)} (api, value, group) entries '
          f'and {len(bitmask_groups)} bitmask groups.') # TODO: for use later.

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

    generate_interceptor_files(commands)


if __name__ == '__main__':
    main()
