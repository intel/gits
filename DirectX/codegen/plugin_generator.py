#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import os
import importlib.util

def import_function_from_file(file_path, function_name):
    if not os.path.isfile(file_path):
        print(f"File '{file_path}' not found.")
        return None

    module_name = os.path.splitext(os.path.basename(file_path))[0]

    spec = importlib.util.spec_from_file_location(module_name, file_path)
    if spec is None:
        print(f"Could not load spec for module '{module_name}' from file '{file_path}'.")
        return None

    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except Exception as e:
        print(f"Error loading module '{module_name}': {e}")
        return None

    if not hasattr(module, function_name):
        print(f"Function '{function_name}' not found in module '{module_name}'.")
        return None

    return getattr(module, function_name)

def list_subdirectories(directories):
    subdirectories = {}
    for directory in directories:
        if os.path.isdir(directory):
            subdirectories[directory] = [os.path.join(directory, sub_dir) for sub_dir in os.listdir(directory) if os.path.isdir(os.path.join(directory, sub_dir))]
        else:
            print(f"Warning: {directory} is not a valid directory.")
    return subdirectories

def find_file(directory, filename):
    for root, dirs, files in os.walk(directory):
        if filename in files:
            return os.path.join(root, filename)
    return None

def invoke_script(script_path, *args):
    command = ['python', script_path] + list(args)
    result = subprocess.run(command, capture_output=True, text=True)
    return result.stdout, result.stderr

file_to_find = 'generator.py'
function_name = 'generate_files'

def generate_plugin_artifacts(context, plugin_directories):

    subdirs = list_subdirectories(plugin_directories)
    for dir, subdirs_list in subdirs.items():
        #print(f"{dir}: {subdirs_list}")

        for sub_dir in subdirs_list :
            search_dir = sub_dir + "/codegen"

            file_path = find_file(search_dir, file_to_find)
            if file_path:

                out_path = sub_dir
                imported_function = import_function_from_file(file_path, function_name)
                if imported_function:
                    result = imported_function(context, out_path)
                    #print(f"Result from '{function_name}': {result}")
