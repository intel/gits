# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import argparse
import os
import sys

from generators import execute_task
from generator_step import Step
from generator_task import GeneratorTask
from template_manager import TemplateManager

def step_type(step_str):
    try:
        return Step(step_str)
    except ValueError:
        valid_values = ', '.join([s.value for s in Step])
        raise argparse.ArgumentTypeError(f"Invalid step: {step_str}. Valid steps are: {valid_values}")


def file_path(path):
    if os.path.isfile(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"Invalid file path: {path}")


def folder_path(path):
    if os.path.isdir(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"Invalid folder path: {path}")


def main():
    parser = argparse.ArgumentParser(description="Generate configuration & enum code/docs/...")
    parser.add_argument('step', type=step_type, help='Step of the process')
    parser.add_argument('metafileConfig', type=file_path, help='Path to the configuration metafile')
    parser.add_argument('metafileEnum', type=file_path, help='Path to the enum metafile')
    parser.add_argument('outputPath', type=folder_path, help='Path to the output base folder')

    try:
      args = parser.parse_args()
    except argparse.ArgumentTypeError as e:
        print(f"Argument error: {e}")
        return False
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return False
    output_path = os.path.abspath(args.outputPath)
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_directory = os.path.join(script_dir, '..', 'templates')
    root_directory = os.path.join(script_dir, '..')
    template_manager = TemplateManager(template_directory, root_directory)

    task = GeneratorTask(args.step, args.metafileConfig, args.metafileEnum, output_path)
    print(f"Generator step `{task.type}` ==> {output_path}")
    execute_task(task, template_manager)

    print(f"done.")

    return True


if __name__ == '__main__':
    if not main():
        sys.exit(1)