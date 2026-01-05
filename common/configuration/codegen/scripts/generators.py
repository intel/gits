# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from configuration_element import parse_config_yaml
from configuration_enum import parse_enums_yaml
from template_manager import Step


def generate_argumentparser(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, ordered_groups = parse_config_yaml(
        task.input_config, template_manager.root_directory)
    enums = parse_enums_yaml(task.input_enum, template_manager.root_directory)
    types = [enum.name for enum in enums]

    context = {'groups': ordered_groups, 'types': types, 'data': configuration}

    template_manager.render_task(task, context)


def generate_configuration(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, ordered_groups = parse_config_yaml(
        task.input_config, template_manager.root_directory)
    context = {'groups': ordered_groups, 'data': configuration, 'platform': task.platform}

    template_manager.render_task(task, context)


def generate_enums(task, template_manager) -> None:
    print(f"  Reading input(s):{task.get_input_str()}")

    enums = parse_enums_yaml(task.input_enum, template_manager.root_directory)
    context = {'enums': enums}
    template_manager.render_task(task, context)


def generate_default_configuration(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, _ = parse_config_yaml(
        task.input_config, template_manager.root_directory)
    conditions = []
    if task.compute:
        conditions = ['is_compute']
    context = {'data': configuration, 'platform': task.platform, 'installpath': task.installpath, 'conditions': conditions}
    template_manager.render_task(task, context)


def generate_documentation_enums(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    enums = parse_enums_yaml(task.input_enum, template_manager.root_directory)
    context = {'enums': enums}

    template_manager.render_task(task, context)


def generate_documentation_config(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, ordered_groups = parse_config_yaml(
        task.input_config, template_manager.root_directory)
    enums = parse_enums_yaml(task.input_enum, template_manager.root_directory)
    context = {'groups': ordered_groups, 'data': configuration, 'enums': enums}

    template_manager.render_task(task, context)


def generate(task, template_manager):
    if task.step == Step.CONFIG:
        generate_configuration(task, template_manager)

    elif task.step == Step.ARGUMENTS:
        generate_argumentparser(task, template_manager)

    elif task.step == Step.ENUMS:
        generate_enums(task, template_manager)

    elif task.step == Step.DEFAULT_CONFIG:
        generate_default_configuration(task, template_manager)

    elif task.step == Step.DOCS_ENUMS:
        generate_documentation_enums(task, template_manager)

    elif task.step == Step.DOCS_CONFIG:
        generate_documentation_config(task, template_manager)

    else:
        print(f"Unsupported/unknown task step {task.step}")

