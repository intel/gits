# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os
import yaml

from utils import get_if_present


class ConfigurationEnumValue:
    def __init__(self, node):
        self.value = node['Value']
        self.short_description = get_if_present(node, 'Description', '')
        self.description = get_if_present(node, 'LongDescription', '')

        if 'Labels' in node:
            self.labels = node['Labels']
        else:
            label_parts = self.value.split('_')
            label_parts = [part.capitalize() for part in label_parts]
            self.labels = [''.join(label_parts)]


    def __str__(self):
        return f'Value: {self.value}, Description: {self.short_description}, Long Description: {self.description}'


class ConfigurationEnum:
    def __init__(self, node, values: list[ConfigurationEnumValue] = None):
        self.name = node['Name']
        self.values = values
        if not values:
            self.values = []
        self.description = get_if_present(node, 'Description', '')
        self.type = get_if_present(node, 'Type', None)


    def __str__(self):
        return (f"Name: {self.name}, Description: {self.description},"
                f" Values: [{', '.join(str(value) for value in self.values)}]")


# Reads in the enums .yaml file and creates a list of ConfigurationEnum from it
def parse_enums_yaml(yaml_filepath: str, root_directory: str) -> list[ConfigurationEnum]:
    enums_list = []
    yaml_path = os.path.join(root_directory, yaml_filepath)
    with open(yaml_path, mode='r') as yaml_file:
        enums = yaml.safe_load(yaml_file)['Enums']
        for enum in enums:
            enum_values = []
            for enum_value in enum['Values']:
                # rstrip() is there to remove the unnecessary trailing newline in case of multi-line descriptions
                enum_values.append(
                    ConfigurationEnumValue(enum_value)
                )
            # The sort field is an optional field that signals that the enum values should get alphabetically sorted
            should_sort = False
            if 'Sort' in enum:
                should_sort = enum['Sort']
            enums_list.append(ConfigurationEnum(enum, sorted(
                enum_values, key=lambda x: x.value) if should_sort else enum_values))

    return sorted(enums_list, key=lambda x: x.name)
