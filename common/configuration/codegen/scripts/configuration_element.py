# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import os
import yaml

from utils import fixCapitalizationName, get_if_present, fixUpCapitalizationInstance

blank_types = []


class ConfigurationEntry:
    def __init__(self, node, namespace, instance_namespace):
        self.leaf_count = 1
        self.args_count = 0
        self.derived_count = 0

        self.name = fixCapitalizationName(node['Name'])
        self.config_name = fixCapitalizationName(get_if_present(node, 'ConfigName', node['Name']))
        self.tags = []

        # ensure the first letter is lowercase
        self.instance_name = node['Name']
        if len(self.instance_name) > 1:
            self.instance_name = node['Name'][0].lower() + node['Name'][1:]
        else:
            self.instance_name = node['Name'][0].lower()
        self.instance_name = get_if_present(node, 'InstanceName', self.instance_name)

        self.namespace = namespace
        if not namespace:
            self.namespace = [self.name]

        self.instance_namespace = instance_namespace
        if not instance_namespace:
            self.instance_namespace = [self.instance_name]

        self.type = get_if_present(node, 'Type', 'Entry')
        self.is_group = self.type == 'Group'

        # process accessibility
        self.is_derived = False
        self.argument_only = False
        if 'Accessibility' in node:
            if node['Accessibility'] == 'Derived':
                self.is_derived = True
                self.derived_count = 1
            elif node['Accessibility'] == 'ArgumentOnly':
                self.is_derived = True
                self.argument_only = True

        # Tags are optional
        self.tags = list(get_if_present(node, 'Tags', namespace))
        self.tags.extend(get_if_present(node, 'OSVisibility', []))

        self.description = ''
        self.short_description = ''

        if 'LongDescription' in node and 'Description' in node:
            self.short_description = node['Description']
            self.description = node['LongDescription']
        else:
            key = None
            if 'LongDescription' in node:
                key = 'LongDescription'
            elif 'Description' in node:
                key = 'Description'
            if key:
                self.description = node[key]
                self.short_description = node[key]

        self.os_visibility = None
        if 'OSVisibility' in node:
            self.os_visibility = node['OSVisibility']


    def __str__(self):
        return f'Type: {self.type}'


    def __repr__(self):
        return str(self)


    def get_tags_escaped(self):
        return "{" + ", ".join([f"\"{tag}\"" for tag in self.get_tags()]) + "}"


    def get_tags(self):
        return [tag for tag in self.tags if tag != 'Configuration']


    def has_leafs(self):
        return self.leaf_count - self.derived_count > 0


    def is_os_visible(self, platform):
        if self.os_visibility is None:
            return True
        if platform == 'win32':
            os = "WINDOWS"
        else:
            os = "X11"
        return os in self.os_visibility

    def is_os_limited(self):
        return self.os_visibility is not None


class ConfigurationOption(ConfigurationEntry):
    ENVIRONMENT_PREFIX = 'GITS_'
    ARGS_SUFFIX_HIDE_OPTION = '!'
    ARGS_TAG_PREFIX_ACCESS_LEVEL = 'accLvl'

    VALUETYPES_NEED_QUOTES = ['std::string', 'std::filesystem::path', 'BitRange', 'VulkanObjectRange']
    STRING_TYPES = ['std::string', 'std::filesystem::path']


    def __init__(self, option, namespace, instance_namespace):
        super().__init__(option, namespace, instance_namespace)

        # namespace (- "Configuration" prefix) + current name
        namespace_lst = namespace + [self.name]
        if len(namespace_lst) > 1:
            if namespace_lst[0] == "Configuration":
                namespace_lst = namespace_lst[1:]
        self.namespace_str = '::'.join(namespace)
        self.argument_path = '.'.join(namespace_lst)
        self.default = option['Default']
        if 'NumericFormat' in option:
            try:
                target_format = option['NumericFormat']
                if target_format == 'Hexadecimal':
                    self.default = hex(self.default)
                elif target_format == 'Binary':
                    self.default = bin(self.default)
                elif target_format == 'Decimal':
                    pass
                else:
                    raise ValueError(f"Unknown NumericFormat: {target_format}")
            except Exception as e:
                print(f"Error processing NumericFormat for {self.name}: NumericFormat={target_format}: {e}")
        else:
            self.default = self.process_default(self.default)

        self.defaults_per_platform = {}
        if 'DefaultsPerPlatform' in option:
            defaults_per_platform_lst = option['DefaultsPerPlatform']
            for d in defaults_per_platform_lst:
                for key, value in d.items():
                    self.defaults_per_platform[key] = self.process_default(value)

        self.shorthands = get_if_present(option, 'Arguments', [])
        self.has_custom_shorthands = len(self.shorthands) > 0

        self.is_vector_type = self.type.startswith('std::vector') or self.type.startswith('std::set')
        self.needs_quotes_in_yml = self.type in ConfigurationOption.VALUETYPES_NEED_QUOTES
        self.is_string_type = self.type in ConfigurationOption.STRING_TYPES

        self.access_level = get_if_present(option, 'AccessLevel', 'Developer')
        self.tags.append(f"{ConfigurationOption.ARGS_TAG_PREFIX_ACCESS_LEVEL}:{self.access_level}")

        self.default_condition = {}
        if 'DefaultCondition' in option:
            default_condition_lst = option['DefaultCondition']
            for d in default_condition_lst:
                for key, value in d.items():
                    self.default_condition[key] = self.process_default(value)


    def __str__(self):
        return (f'Name: {self.name}, Type: {self.type}, Default: {self.default}, '
                f'Short Description: {self.short_description}, Description: {self.description}')


    def __repr__(self):
        return str(self)


    def process_default(self, default) -> str:
        global blank_types
        result = default
        if self.type == 'std::string':
            result = f'{default}'
        elif self.type == 'std::filesystem::path':
            result = f'{default}'
            result = result.replace('\\\\', '/')
            result = result.replace('\\', '/')
        elif self.type == 'std::vector<std::string>':
            if default == '':
                result = "'[]'"
            else:
                result = default
        elif self.type == 'bool':
            if default == '':
                default = 'false'
            result = f'{str(default).lower()}'
        elif default == '' and not (self.is_derived or self.argument_only):
            if self.type == 'int':
                result = '0'
            elif self.type == 'float':
                result = '0.0'
            elif self.type == 'uint64_t':
                result = '0'
            elif self.type == 'uint32_t':
                result = '0'
            elif self.type == 'unsigned int':
                result = '0'
            else:
                blank_types.append(self.type)

        return str(result)


    def get_default(self, platform: str = None, install_path = None, conditions=[]) -> str:
        value = self.default
        for condition in conditions:
            if condition in self.default_condition.keys():
                value = self.default_condition[condition]
        if platform:
          if self.defaults_per_platform and platform in self.defaults_per_platform:
              value = self.defaults_per_platform[platform]

          if platform != 'win32':
            if platform == "lnx_32":
              arch = 'i386'
            elif platform == "lnx_64":
              arch = 'x86_64'
            elif platform == "lnx_arm":
              arch = 'aarch64'
            else:
              raise ValueError(f"Unknown platform: {platform}")

            value = value.replace('{arch}', arch)
        if install_path:
            value = value.replace('{install_path}', install_path)
        return value


    def get_argument_type(self) -> str:
        if self.type == 'bool':
            return f"args::Flag"
        else:
            return f"args::ValueFlag<{self.type}>"


    def get_shorthands(self) -> str:
        tmp_list= [self.argument_path + ConfigurationOption.ARGS_SUFFIX_HIDE_OPTION] + self.shorthands
        lst = [f"'{item}'" for item in tmp_list if len(item) == 1]
        lst.extend([f'"{item}"' for item in tmp_list if len(item) > 1])
        return ", ".join(lst)

    def get_custom_shorthands(self) -> str:
        lst = [f"`-{item}`" for item in self.shorthands if len(item) == 1]
        lst.extend([f"`--{item}`" for item in self.shorthands if len(item) > 1])
        return ", ".join(lst)


    def get_environment_string(self) -> str:
        return f"{ConfigurationOption.ENVIRONMENT_PREFIX}{self.argument_path.upper().replace('.', '_')}"


    def get_csv_dictionary(self):
        return {
            'Path': self.namespace_str,
            'Name': self.name,
            'Type': self.type,
            'LongDescription': self.description,
            'Description': self.short_description,
            'Tags': self.get_tags(),
            'Arguments': ','.join(self.shorthands),
            'IsDerived': str(self.is_derived),
            'ArgumentOnly': str(self.argument_only),
            'Default': self.default
        }


class ConfigurationGroup(ConfigurationEntry):
    ARGUMENT_PREFIX = 'Arg'

    def __init__(self, node, options: list[ConfigurationOption] = None, namespace: list[str] = None,
                 instance_namespace: list[str] = None):
        super().__init__(node, namespace, instance_namespace)
        self.leaf_count = 0

        self.instance_name = fixUpCapitalizationInstance(self.instance_name)
        self.instance_name = get_if_present(node, 'InstanceName', self.instance_name)

        self.type = 'Group'
        self.options = []
        if options:
            self.options = options
            self.leaf_count = sum(option.leaf_count for option in options)
            self.args_count = sum(option.args_count for option in options)
            self.derived_count = sum(option.derived_count for option in options)

        self.namespace_str = '::'.join(self.namespace)

        self.argument_name = ConfigurationGroup.ARGUMENT_PREFIX + self.name
        self.argument_namespace = [
            f"{ConfigurationGroup.ARGUMENT_PREFIX}{namespace}" for namespace in self.namespace]
        self.argument_namespace_str = '::'.join(self.argument_namespace)


    def __str__(self):
        return f"Name: {self.name}, Options: [{', '.join(str(option) for option in self.options)}]"


    def __repr__(self):
        return str(self)


    def get_csv_dictionary(self):
        namespace_str = '::'.join(self.namespace[:-1])
        return {
            'Path': namespace_str,
            'Name': self.name,
            'Type': self.type,
            'LongDescription': self.description,
            'Description': self.short_description,
            'Tags': self.get_tags(),
            'Arguments': ','.join([]),
            'IsDerived': str(self.is_derived),
            'ArgumentOnly': str(self.argument_only),
            'Default': ''
        }


    def get_config_groups(self):
        # TODO - filter for groups that are in the configfile
        return [option for option in self.options if option.is_group]


    def get_config_options(self):
        # TODO - filter for groups that are in the configfile
        return [option for option in self.options if not option.is_group]

    def get_config_path(self):
        return '.'.join(self.namespace[1:])


def parse_group_node(node, parent_namespace=None, parent_instance_namespace=None):
    # assumption: we are called on a group
    options = []
    namespace = [node['Name'][0].upper() + node['Name'][1:]]
    if not parent_namespace is None:
        namespace = parent_namespace + namespace

    instance_name = node['Name']
    if len(instance_name) > 1:
        instance_name = node['Name'][0].lower() + node['Name'][1:]
    else:
        instance_name = node['Name'][0].lower()
    instance_name = fixUpCapitalizationInstance(instance_name)
    if 'InstanceName' in node:
        instance_name = node['InstanceName']
    instance_namespace = [instance_name]
    if parent_instance_namespace:
        instance_namespace = parent_instance_namespace + instance_namespace

    is_group_derived = False
    if 'IsDerived' in node:
        is_group_derived = node['IsDerived']

    dependencies = []
    for option in node['Options']:
        if option['Type'] == 'Group':
            entry, new_dependencies = parse_group_node(
                option, namespace, instance_namespace)
            dependencies += new_dependencies
        else:
            entry = ConfigurationOption(option, namespace, instance_namespace)
            if is_group_derived:
                entry.is_derived = True

        options.append(entry)

    group = ConfigurationGroup(node, options, namespace, instance_namespace)
    dependencies += [group]
    return group, dependencies


def parse_config_yaml(yaml_filepath: str, root_directory: str) -> ConfigurationGroup:
    configuration = None
    dependencies = []
    yaml_path = os.path.join(root_directory, yaml_filepath)
    with open(yaml_path, mode='r') as yaml_file:
        configuration_yaml = yaml.safe_load(yaml_file)['Configuration']
        config_node = {'Name': 'Configuration',
                       'Type': 'Group', 'Options': configuration_yaml}
        configuration, dependencies = parse_group_node(config_node)

    global blank_types
    print(
        f"  > Encountered the following types with empty arguments: {list(set(blank_types))}")

    return configuration, dependencies
