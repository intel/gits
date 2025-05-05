# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from datetime import datetime


def do_print(txt, file_handle, indentation_depth=0, symbol='  '):
    indent = indentation_depth
    print((symbol * indent) + txt, file=file_handle)


def replace_chars(source, spacer):
    for char in ['.', ' ', '-', '/']:
        source = source.replace(char, spacer)
    return source


def remove_last_suffix(s):
    last_dot_index = s.rfind('.')
    if last_dot_index != -1:
        return s[:last_dot_index]
    return s


def extract_last_suffix(s):
    last_dot_index = s.rfind('.')
    if last_dot_index != -1:
        return s[last_dot_index+1:]
    return s


def sanitize_struct_name(struct_path, prefix='', suffix=''):
    return prefix + replace_chars(struct_path, '') + suffix


def sanitize_variable_name(struct_path, prefix='', suffix=''):
    return prefix + replace_chars(struct_path, '') + suffix


def render_to_file(template, context, output_file):
    context['time'] = datetime.now()
    rendered = template.render(**context)
    rendered = rendered.replace('\r\n', '\n').replace('\r', '\n')
    with open(output_file, 'w', newline='\n') as file:
        file.write(rendered)
