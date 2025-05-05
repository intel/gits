# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from configuration_element import parse_config_yaml
from configuration_enum import parse_enums_yaml
from generator_step import Step
import os, csv


def generate_argumentparser(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, ordered_groups = parse_config_yaml(
        task.input_config, template_manager.root_directory)
    enums = parse_enums_yaml(task.input_enum, template_manager.root_directory)
    types = [enum.name for enum in enums]

    # TODO: the types need to actually come from the enums.
    context = {'groups': ordered_groups, 'types': types, 'data': configuration}

    template_manager.render_task(task, context)


def generate_configuration(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, ordered_groups = parse_config_yaml(
        task.input_config, template_manager.root_directory)
    context = {'groups': ordered_groups, 'data': configuration}

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
    context = {'data': configuration}
    template_manager.render_task(task, context)


def print_to_csv(config, writer):
    for option in config.options:
        writer.writerow(option.get_csv_dictionary())
        if option.is_group:
            print_to_csv(option, writer)


def generate_tables(task, template_manager):
    print(f"  Reading input(s):{task.get_input_str()}")

    configuration, _ = parse_config_yaml(
        task.input_config, template_manager.root_directory)

    # for now this seems "ok" to be here...
    path = os.path.abspath(template_manager.build_output_path(task, 'config.csv'))
    print(f"  Writing CSV to: {path}")
    with open(path, 'w', newline='') as csvfile:
        fieldnames = ['Path', 'Name', 'Type', 'Default', 'IsDerived', 'ArgumentOnly', 'Description', 'Tags', 'Arguments', 'LongDescription']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        print_to_csv(configuration, writer)
    print(f" done.")

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

def execute_task(task, template_manager):
    if task.type == Step.CONFIG:
        generate_configuration(task, template_manager)

    elif task.type == Step.ARGUMENTS:
        generate_argumentparser(task, template_manager)

    elif task.type == Step.ENUMS:
        generate_enums(task, template_manager)

    elif task.type == Step.DEFAULT_CONFIG:
        generate_default_configuration(task, template_manager)

    elif task.type == Step.DOCS_ENUMS:
        generate_documentation_enums(task, template_manager)

    elif task.type == Step.DOCS_CONFIG:
        generate_documentation_config(task, template_manager)

    else:
        print(f"Unsupported/unknown task type {task.type}")

