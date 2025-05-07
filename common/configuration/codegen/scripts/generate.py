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
from pathlib import Path

from generators import generate
from template_manager import TemplateManager, Step


class GeneratorTask:
    def __init__(self, step, metafile_config, metafile_enum, output_path, label = "", platform="", installpath="", compute=False):
        self.name = label
        self.step = step
        self.input_config = metafile_config
        self.input_enum = metafile_enum
        self.output_path = output_path
        self.platform = platform
        self.installpath = installpath
        self.compute = compute


    def __str__(self) -> str:
        return f'Label: {self.name}, Type: {self.step}, Input: {self.input}, Output Path: {self.output_path}'


    def __repr__(self) -> str:
        return str(self)


    def get_input_str(self) -> str:
        return ','.join([f"\n  - {config}" for config in [self.input_config, self.input_enum] if config])


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
    parser.add_argument('--step', type=step_type, help='Step of the process', required=True)
    parser.add_argument('--configYML', type=file_path, help='Path to the configuration metafile', required=True)
    parser.add_argument('--enumYML', type=file_path, help='Path to the enum metafile', required=True)
    parser.add_argument('--outDir', help='Path to the output base folder', required=True)

    parser.add_argument('--platform', help='Target platform', default='')
    parser.add_argument('--installpath', help='Installation path', default='skip')
    parser.add_argument('--compute', help='Internal build flag', action='store_true')

    try:
      args = parser.parse_args()
    except argparse.ArgumentTypeError as e:
        print(f"Argument error: {e}")
        return False
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return False

    try:
      output_path = Path(args.outDir).absolute()
      if not output_path.exists():
        output_path.mkdir(parents=True, exist_ok=True)

      if args.installpath == 'skip':
        install_path = "${Gits_install_dir}"
      else:
        install_path = os.path.abspath(args.installpath)

      script_dir = os.path.dirname(os.path.abspath(__file__))
      template_directory = os.path.join(script_dir, '..', 'templates')
      root_directory = os.path.join(script_dir, '..')
    except Exception as e:
      print(f"Error setting up paths: {e}")
      return False

    try:
      template_manager = TemplateManager(template_directory, root_directory)

      task = GeneratorTask(args.step, args.configYML, args.enumYML, output_path, "", args.platform, install_path, args.compute)
      print(f" Generator step `{task.step}` ==> {output_path}")
      generate(task, template_manager)
      print(f" done.")
    except Exception as e:
      print(f"An error occurred during generation: {e}")
      return False

    return True


if __name__ == '__main__':
    if not main():
        sys.exit(1)
