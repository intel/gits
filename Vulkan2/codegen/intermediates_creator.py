#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from intermediates import Parameter, Command, Member, Enum, Bitmask, Flag, Union, Structure, Handle
from generator_helpers import get_full_decl, get_length, get_enum_value
import xml.etree.ElementTree
import re

def get_xml_root(xml_path):
    vk_xml_root = xml.etree.ElementTree.parse(xml_path).getroot()
    return vk_xml_root

def parse_commands(vk_xml_root) -> list[Command]:
    commands_map = {}

    for command_node in vk_xml_root.findall("./commands/command"):
        export_node = command_node.get("export")
        if export_node is not None and "vulkan" not in export_node.split(","):
            continue
        proto_node = command_node.find("proto")
        if proto_node is not None:
            name_node = proto_node.find("name")
            proto_type_node = proto_node.find("type")
            command_name = name_node.text.strip()
            return_type = proto_type_node.text.strip()
            success_codes = command_node.get("successcodes").split(",") if command_node.get("successcodes") is not None else []
            error_codes = command_node.get("errorcodes").split(",") if command_node.get("errorcodes") is not None else []
            params = []
            param_nodes = command_node.findall("param")
            for param_node in param_nodes:
                api = param_node.get("api")
                if api is not None and "vulkan" not in api.split(","):
                    continue
                name, base_type, full_type = get_full_decl(param_node)
                param = Parameter()
                param.name = name
                param.base_type = base_type
                param.full_type = full_type
                param.is_const = 'const' in full_type
                param.is_pointer_to_pointer = bool(re.search(r'\*\s*(const\s*)?\*', full_type))
                param.is_pointer = '*' in full_type and not param.is_pointer_to_pointer
                param.is_void = base_type == 'void'
                fixed_array_size = [dim.strip() for dim in re.findall(r'\[([^\]]+)\]', full_type)]
                param.fixed_array_size = fixed_array_size
                length, is_null_terminated = get_length(param_node)
                param.length = length
                param.is_null_terminated = is_null_terminated
                params.append(param)
            command = Command(
                name=command_name,
                return_type=return_type,
                params=params,
                success_codes=success_codes,
                error_codes=error_codes)
            commands_map[command_name] = command

        elif command_node.get("alias") is not None:
            alias_name = command_node.get("name")
            base_command_name = command_node.get("alias")
            base_command = commands_map[base_command_name]
            aliased_command = Command(
                name=alias_name,
                return_type=base_command.return_type,
                params=base_command.params.copy(),
                success_codes=base_command.success_codes.copy(),
                error_codes=base_command.error_codes.copy()
            )
            commands_map[alias_name] = aliased_command
    return list(commands_map.values())

def parse_member(node) -> Member:
    name, base_type, full_type = get_full_decl(node)
    fixed_array_size = [dim.strip() for dim in re.findall(r'\[([^\]]+)\]', full_type)]
    length, is_null_terminated = get_length(node)
    values = node.get('values') or ''
    is_pointer_to_pointer=bool(re.search(r'\*\s*(const\s*)?\*', full_type))
    is_pointer='*' in full_type and not is_pointer_to_pointer

    member = Member(
        name=name,
        base_type=base_type,
        full_type=full_type,
        is_const='const' in full_type,
        is_void=base_type == 'void',
        is_pointer_to_pointer=is_pointer_to_pointer,
        is_pointer=is_pointer,
        length=length,
        is_null_terminated=is_null_terminated,
        fixed_array_size=fixed_array_size,
        bitfield=int(full_type.split(':')[1]) if ':' in full_type else None,
        values=values
    )

    return member

def parse_structures(vk_xml_root) -> list[Structure]:
    structs_map = {}
    alias_map = {}
    struct_nodes = vk_xml_root.findall("./types/type[@category='struct']")
    for struct_node in struct_nodes:
        name = struct_node.get('name')
        alias = struct_node.get("alias")
        if alias:
            alias_map[name] = alias
            continue
        struct_extends_str = struct_node.get('structextends') or ''
        struct_extends = [s.strip() for s in struct_extends_str.split(',')] if struct_extends_str else []
        returned_only = struct_node.get('returnedonly') == 'true'
        members = []
        stype_value = ''
        member_nodes = struct_node.findall("member")
        for member_node in member_nodes:
            api = member_node.get("api")
            if api is not None and "vulkan" not in api.split(","):
                continue
            member = parse_member(member_node)
            if member.name == 'sType' and member.values:
                stype_value = member.values
            members.append(member)
        struct = Structure(
            name=name,
            members=members,
            pnext_input=bool(struct_extends) and not returned_only,
            pnext_output=returned_only and bool(struct_extends),
            stype_value=stype_value,
            struct_extends=struct_extends,
        )
        structs_map[name] = struct

    for alias_name, base_name in alias_map.items():
        structs_map[base_name].aliases.append(alias_name)

    return list(structs_map.values())

def parse_unions(vk_xml_root) -> list[Union]:
    unions_map = {}
    alias_map = {}
    union_nodes = vk_xml_root.findall("./types/type[@category='union']")
    for union_node in union_nodes:
        name_node = union_node.get('name')
        alias = union_node.get("alias")
        if alias:
            alias_map[name_node] = alias
            continue
        members = []
        member_nodes = union_node.findall("member")
        for member_node in member_nodes:
            member = parse_member(member_node)
            members.append(member)
        union = Union(
            name=name_node,
            members=members
        )
        unions_map[name_node] = union

    for name, alias in alias_map.items():
        base_union = unions_map[alias]
        aliased_union = Union(
            name=name,
            members=base_union.members.copy()
        )
        unions_map[name] = aliased_union
    return list(unions_map.values())

def parse_handles(vk_xml_root) -> list[Handle]:
    handles_map = {}
    alias_map = {}
    handle_nodes = vk_xml_root.findall("./types/type[@category='handle']")
    for handle_node in handle_nodes:
        alias = handle_node.get("alias")
        if alias:
            name = handle_node.get('name')
            alias_map[name] = alias
            continue
        name = handle_node.find('name').text.strip()
        parent = handle_node.get('parent', '')
        type = handle_node.get('objtypeenum')
        dispatchable = handle_node.find('type').text == 'VK_DEFINE_HANDLE'
        handle = Handle(
            name=name,
            type=type,
            parent=parent,
            dispatchable=dispatchable,
        )
        handles_map[name] = handle
    for name, alias in alias_map.items():
        base_handle = handles_map[alias]
        aliased_handle = Handle(
            name=name,
            type=base_handle.type,
            parent=base_handle.parent,
            dispatchable=base_handle.dispatchable,
        )
        handles_map[name] = aliased_handle
    return list(handles_map.values())

def parse_enums(vk_xml_root) -> list[Enum]:
    enums_map = {}

    enum_nodes = vk_xml_root.findall("./enums[@type='enum']")
    for enum_node in enum_nodes:
        name = enum_node.get('name')
        enumerator_nodes = enum_node.findall("enum")
        values = {}
        for enumerator_node in enumerator_nodes:
            alias = enumerator_node.get("alias")
            if alias is not None:
                continue
            enumerator_name = enumerator_node.get('name')
            enumerator_value = get_enum_value(enumerator_node)
            values[enumerator_name] = enumerator_value
        enum = Enum(
            name=name,
            values=values
        )
        enums_map[name] = enum

    for feature_node in vk_xml_root.findall("./feature"):
        api = feature_node.get("api")
        if api is not None and 'vulkan' not in api.split(","):
            continue
        for require_node in feature_node.findall("./require"):
            require_api = require_node.get("api")
            if require_api is not None and 'vulkan' not in require_api.split(","):
                continue
            for enum_node in require_node.findall("./enum"):
                extends = enum_node.get("extends")
                if extends is None or extends not in enums_map:
                    continue
                alias = enum_node.get("alias")
                if alias is not None:
                    continue
                name = enum_node.get("name")
                value = get_enum_value(enum_node)
                enums_map[extends].values[name] = value

    for ext_node in vk_xml_root.findall("./extensions/extension"):
        supported = ext_node.get("supported")
        if supported is not None and 'vulkan' not in supported.split(","):
            continue
        ext_number = ext_node.get("number")
        for require_node in ext_node.findall("./require"):
            require_api = require_node.get("api")
            if require_api is not None and 'vulkan' not in require_api.split(","):
                continue
            for enum_node in require_node.findall("./enum"):
                extends = enum_node.get("extends")
                if extends is None or extends not in enums_map:
                    continue
                alias = enum_node.get("alias")
                if alias is not None:
                    continue
                name = enum_node.get("name")
                value = get_enum_value(enum_node, ext_number)
                enums_map[extends].values[name] = value
    return list(enums_map.values())

def parse_bitmasks(vk_xml_root) -> list[Bitmask]:
    bitmasks_map = {}

    for bitmask_node in vk_xml_root.findall("./enums[@type='bitmask']"):
        name = bitmask_node.get('name')
        bitwidth = int(bitmask_node.get('bitwidth', '32'))
        flag_name = name.replace('FlagBits', 'Flags')
        flagbit_nodes = bitmask_node.findall("enum")
        bits = {}
        for flagbit_node in flagbit_nodes:
            bit_name = flagbit_node.get('name')
            value = get_enum_value(flagbit_node)
            bits[bit_name] = value
        bitmask = Bitmask(
            name=name,
            bitwidth=bitwidth,
            flag_name=flag_name,
            bits=bits
        )
        bitmasks_map[name] = bitmask
    for feature_node in vk_xml_root.findall("./feature"):
        api = feature_node.get("api")
        if api is not None and 'vulkan' not in api.split(","):
            continue
        for require_node in feature_node.findall("./require"):
            require_api = require_node.get("api")
            if require_api is not None and 'vulkan' not in require_api.split(","):
                continue
            for enum_node in require_node.findall("./enum"):
                extends = enum_node.get("extends")
                if extends is None or extends not in bitmasks_map:
                    continue
                alias = enum_node.get("alias")
                if alias is not None:
                    continue
                name = enum_node.get("name")
                value = get_enum_value(enum_node)
                bitmasks_map[extends].bits[name] = value

    for ext_node in vk_xml_root.findall("./extensions/extension"):
        supported = ext_node.get("supported")
        if supported is not None and 'vulkan' not in supported.split(","):
            continue
        ext_number = ext_node.get("number")
        for require_node in ext_node.findall("./require"):
            require_api = require_node.get("api")
            if require_api is not None and 'vulkan' not in require_api.split(","):
                continue
            for enum_node in require_node.findall("./enum"):
                extends = enum_node.get("extends")
                if extends is None or extends not in bitmasks_map:
                    continue
                alias = enum_node.get("alias")
                if alias is not None:
                    continue
                name = enum_node.get("name")
                value = get_enum_value(enum_node, ext_number)
                bitmasks_map[extends].bits[name] = value

    return list(bitmasks_map.values())

def parse_flags(vk_xml_root) -> list[Flag]:
    flags_map = {}
    flags_alias_map = {}
    for flag_node in vk_xml_root.findall("./types/type[@category='bitmask']"):
        api = flag_node.get("api")
        if api is not None and 'vulkan' not in api.split(","):
            continue
        name_node = flag_node.find("name")
        if name_node is None:
            alias = flag_node.get("alias")
            if alias is not None:
                name = flag_node.get('name')
                flags_alias_map[name] = alias
                continue
        name = name_node.text.strip()
        type = flag_node.find("type")
        base_type = type.text if type is not None else 'VkFlags'
        bitwidth = 64 if base_type == 'VkFlags64' else 32
        bitmask = flag_node.get('bitvalues') or flag_node.get('requires')
        flag = Flag(
            name=name,
            bitmask_name=bitmask,
            bitwidth=bitwidth,
            base_type=base_type
        )
        flags_map[name] = flag
    for name, alias in flags_alias_map.items():
        base_flag = flags_map[alias]
        aliased_flag = Flag(
            name=name,
            bitmask_name=base_flag.bitmask_name,
            bitwidth=base_flag.bitwidth,
            base_type=base_flag.base_type
        )
        flags_map[name] = aliased_flag
    return list(flags_map.values())

def get_platform_names(vk_xml_root) -> tuple[set[str], dict[str, str]]:
    platform_to_skip = [
      'xlib_xrandr',
      'directfb',
      'android',
      'vi',
      'ios',
      'macos',
      'metal',
      'fuchsia',
      'ggp',
      'sci',
      'screen',
      'ohos'
    ]

    excluded = set()
    platform_map = {}

    for feature_node in vk_xml_root.findall("./feature"):
        api = feature_node.get("api")
        if api is None or 'vulkan' in api.split(","):
            continue
        for require_node in feature_node.findall("./require"):
            for type_node in require_node.findall("type"):
                name = type_node.get("name")
                if name is not None:
                    excluded.add(name)
            for command_node in require_node.findall("command"):
                name = command_node.get("name")
                if name is not None:
                    excluded.add(name)
            for enum_node in require_node.findall("enum"):
                name = enum_node.get("name")
                if name is not None:
                    excluded.add(name)

    for ext_node in vk_xml_root.findall("./extensions/extension"):
        platform = ext_node.get("platform")
        supported = ext_node.get("supported")
        ext_exclude = (platform is not None and platform in platform_to_skip) or (supported is not None and "vulkan" not in supported.split(","))

        for require_node in ext_node.findall("./require"):
            require_api = require_node.get("api")
            should_exclude = ext_exclude or (require_api is not None and "vulkan" not in require_api.split(","))
            for type_node in require_node.findall("type"):
                name = type_node.get("name")
                if name is not None:
                    if should_exclude:
                        excluded.add(name)
                    elif platform is not None:
                        platform_map[name] = platform

            for command_node in require_node.findall("command"):
                name = command_node.get("name")
                if name is not None:
                    if should_exclude:
                        excluded.add(name)
                    elif platform is not None:
                        platform_map[name] = platform

            for enum_node in require_node.findall("enum"):
                name = enum_node.get("name")
                if name is not None:
                    if should_exclude:
                        excluded.add(name)
                    elif platform is not None:
                        platform_map[name] = platform

    return excluded, platform_map

def get_dispatch_level(param_type, handle_map):
    INSTANCE_LEVEL = {'VkInstance', 'VkPhysicalDevice'}
    DEVICE_LEVEL = {'VkDevice'}
    while param_type:
        if param_type in DEVICE_LEVEL:
            return 'device'
        if param_type in INSTANCE_LEVEL:
            return 'instance'
        param_type = handle_map.get(param_type).parent

def postprocess(commands, structures, unions, handles, enums, bitmasks, flags, excluded_names, platform_map):
    commands[:] = [c for c in commands if c.name not in excluded_names]
    structures[:] = [s for s in structures if s.name not in excluded_names]
    unions[:] = [u for u in unions if u.name not in excluded_names]
    handles[:] = [h for h in handles if h.name not in excluded_names]
    flags[:] = [f for f in flags if f.name not in excluded_names]

    for enum in enums:
        enum.values = {k: v for k, v in enum.values.items() if k not in excluded_names}

    for bitmask in bitmasks:
        bitmask.bits = {k: v for k, v in bitmask.bits.items() if k not in excluded_names}

    for command in commands:
        command.platform = platform_map.get(command.name, '')
    for structure in structures:
        structure.platform = platform_map.get(structure.name, '')
    for union in unions:
        union.platform = platform_map.get(union.name, '')
    for flag in flags:
        flag.platform = platform_map.get(flag.name, '')
    for enum in enums:
        enum.platform = platform_map.get(enum.name, '')
    for bitmask in bitmasks:
        bitmask.platform = platform_map.get(bitmask.name, '')

    handle_names = set(handle.name for handle in handles)
    handle_map = {handle.name: handle for handle in handles}
    structure_names = set(structure.name for structure in structures)
    union_names = set(union.name for union in unions)

    OPAQUE_POINTER_TYPES = {'xcb_connection_t', 'Display', 'wl_display', 'wl_surface'}

    # Structs that contain fixed-size arrays of output handles (handles filled in by the driver,
    # not looked up from existing registrations). UpdateOutputHandle must be called before
    # UpdateHandle for parameters of these types.
    STRUCTS_WITH_OUTPUT_HANDLES = {'VkPhysicalDeviceGroupProperties'}

    # Commands whose void* pData parameter carries descriptor update template data
    # and must be serialized as DescriptorTemplateDataArgument.
    DESCRIPTOR_TEMPLATE_COMMANDS = {
        'vkUpdateDescriptorSetWithTemplate',
        'vkUpdateDescriptorSetWithTemplateKHR',
        'vkCmdPushDescriptorSetWithTemplate',
        'vkCmdPushDescriptorSetWithTemplateKHR',
    }

    structure_with_handles = set()
    for structure in structures:
        for member in structure.members:
            if member.base_type in handle_names:
                member.is_handle = True
                structure.has_handles = True
                structure_with_handles.add(structure.name)
            if member.base_type in structure_names:
                member.is_struct = True
            if member.base_type in union_names:
                member.is_union = True
            if member.base_type in OPAQUE_POINTER_TYPES:
                member.is_opaque_pointer = True

    # Second pass to transitively mark structs that contain (directly or indirectly)
    # members whose type is a struct with handles, until convergence.
    changed = True
    while changed:
        changed = False
        for structure in structures:
            for member in structure.members:
                if member.base_type in structure_with_handles:
                    if not member.is_struct_with_handles:
                        member.is_struct_with_handles = True
                    if structure.name not in structure_with_handles:
                        structure_with_handles.add(structure.name)
                        changed = True

    for command in commands:
        first_param_type = command.params[0].base_type
        if first_param_type in handle_names:
            command.dispatch_level = get_dispatch_level(first_param_type, handle_map)
        else:
            command.dispatch_level = 'global'
        last_output_handle_param = None
        for param in command.params:
            if param.base_type in handle_names and param.is_pointer and not param.is_const:
                last_output_handle_param = param
        for param in command.params:
            if param.base_type in handle_names:
                if not param.is_pointer or (param.is_pointer and param.is_const):
                    param.is_handle = True
                elif param is last_output_handle_param:
                    param.is_handle_output = True
            if param.base_type in structure_with_handles:
                param.is_struct_with_handles = True
            if param.base_type in STRUCTS_WITH_OUTPUT_HANDLES:
                param.is_struct_with_output_handles = True
            if param.base_type in structure_names:
                param.is_struct = True
            if param.base_type in union_names:
                param.is_union = True
            if param.base_type in OPAQUE_POINTER_TYPES:
                param.is_opaque_pointer = True
            if command.name in DESCRIPTOR_TEMPLATE_COMMANDS and param.name == 'pData':
                param.is_descriptor_template_data = True
