# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os
import typing
import yaml

from generator_step import Step
from template_manager import TemplateManager
from generators import generate_configuration, generate_argumentparser, generate_enums, generate_default_configuration, generate_tables


class GeneratorTask:
    def __init__(self, type, metafile_config, metafile_enum, output_path, label = ""):
        self.name = label
        self.type = type
        self.input_config = metafile_config
        self.input_enum = metafile_enum
        self.output_path = output_path

    def __str__(self) -> str:
        return f'Label: {self.name}, Type: {self.type}, Input: {self.input}, Output Path: {self.output_path}'

    def __repr__(self) -> str:
        return str(self)

    def get_input_str(self) -> str:
        return ','.join([f"\n  - {config}" for config in [self.input_config, self.input_enum] if config])

    @staticmethod
    def load_tasklist_yaml(file_path: str):
        tasks = []
        with open(file_path, mode='r') as yaml_file:
            nodes = yaml.safe_load(yaml_file)['Tasks']
            meta_dir = os.path.dirname(file_path)
            for node in nodes:
                label = node['Label']
                type = Step.from_string(node['Type'])
                if not type:
                    raise ValueError(f"Invalid task type: {node['Type']}")
                metafile_config = None
                if 'InputConfig' in node:
                    metafile_config = node['InputConfig']
                    metafile_config = os.path.abspath(os.path.join(meta_dir, metafile_config))
                metafile_enum = None
                if 'InputEnum' in node:
                    metafile_enum = node['InputEnum']
                    metafile_enum = os.path.abspath(os.path.join(meta_dir, metafile_enum))
                output_path = node['OutputPath']
                output_path = os.path.abspath(os.path.join(meta_dir, output_path))
                
                tasks.append(GeneratorTask(type, metafile_config, metafile_enum, output_path, label))
        return tasks
