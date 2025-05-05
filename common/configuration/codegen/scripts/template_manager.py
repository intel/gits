# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os

from mako.template import Template
from datetime import datetime

from generator_step import Step


class TemplateManager:

    def __init__(self, template_directory, root_directory):
        self.root_directory = root_directory

        self.templates = {}
        self.templates[Step.ARGUMENTS] = [
            ('argumentsAuto.h.mako', os.path.join('include', 'argumentsAuto.h'), ()),
            ('argumentsAuto.cpp.mako', 'argumentsAuto.cpp', ()),
        ]
        self.templates[Step.CONFIG] = [
            ('configurationAuto.h.mako', os.path.join(
                'include', 'configurationAuto.h'), ()),
            ('configurationAuto.cpp.mako', os.path.join('configurationAuto.cpp'), ()),
            ('configurationYAMLAuto.h.mako', os.path.join(
                'include', 'configurationYAMLAuto.h'), ()),
            ('configurationYAMLAuto.cpp.mako',
                os.path.join('configurationYAMLAuto.cpp'), ()),
        ]
        self.templates[Step.ENUMS] = [
            ('enumsAuto.h.mako', os.path.join('include', 'enumsAuto.h'), ()),
            ('enumsAuto.cpp.mako', os.path.join('enumsAuto.cpp'), ()),
            ('enumsYAMLAuto.h.mako', os.path.join('include', 'enumsYAMLAuto.h'), ()),
            ('enumsYAMLAuto.cpp.mako', os.path.join('enumsYAMLAuto.cpp'), ()),
        ]
        self.templates[Step.DEFAULT_CONFIG] = [
            ('default.yaml.mako', os.path.join('gits_config.yml'), ()),
        ]
        self.templates[Step.DOCS_ENUMS] = [
            ('documentationEnumAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'EnumsAuto.md'), ()),
        ]
        self.templates[Step.DOCS_CONFIG] = [
            ('documentationConfigAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'Auto.md'), ('Common', 'octicons/book-24')),
            ('documentationConfigAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'Auto.md'), ('DirectX', 'material/microsoft')),
            ('documentationConfigAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'Auto.md'), ('OpenGL', 'simple/opengl')),
            ('documentationConfigAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'Auto.md'), ('OpenCL', 'simple/opengl')),
            ('documentationConfigAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'Auto.md'), ('Vulkan', 'simple/vulkan')),
            ('documentationConfigAuto.md.mako', 
             os.path.join('documentation', 'configuration', 'Auto.md'), ('LevelZero', 'simple/intel')),
        ]

        self.template_directory = os.path.normpath(template_directory)

    def build_output_path(self, task, file_name):
        return os.path.join(self.root_directory, task.output_path, file_name)
    
    def render_single_file(self, template, context, output_file):
        rendered = template.render(**context)
        rendered = rendered.replace('\r\n', '\n').replace('\r', '\n')
        cleaned_lines = []
        counter = 0
        for line in rendered.split('\n'):
            if line.strip() == "":
                counter += 1
            else:
                counter = 0
            if counter < 2:
                cleaned_lines.append(line)
        rendered = '\n'.join(cleaned_lines)
        with open(output_file, 'w', newline='\n') as file:
            file.write(rendered)

    def render_task(self, task, context):
        print(f"  [{task.type}] - {len(self.templates[task.type])} file(s) to write")

        context['time'] = datetime.now()
        for i, (templateName, output_file_path, meta_data) in enumerate(self.templates[task.type]):
            template = Template(filename=os.path.join(
                self.template_directory, templateName))
            if len(meta_data) > 0:
                directory, filename = os.path.split(output_file_path)
                output_file = os.path.join(self.root_directory, task.output_path, directory, meta_data[0] + filename)
            else:
                output_file = os.path.join(self.root_directory, task.output_path, output_file_path)
            context['meta_data'] = meta_data
            print(output_file)

            self.render_single_file(template, context, output_file)

            print(
                f"  {i + 1}/{len(self.templates[task.type])} - {templateName} -> {os.path.normpath(output_file)}")

    def __str__(self):
        return f'TemplateManager: templates base path={self.template_directory}'
