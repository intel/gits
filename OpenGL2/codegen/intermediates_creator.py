#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# This module turns the GL/WGL/GLX/EGL XML registries into the intermediate
# representation (IR) defined in intermediates.py, then derives auxiliary
# postprocessed data (enum canonical-name resolution, bitmask group detection)
# from that IR.

import re
from collections import defaultdict
from pathlib import Path
from typing import Any, NamedTuple

from array_of_arrays_metadata import (
    ARRAY_OF_ARRAYS_PARAM_OVERRIDES,
    ArrayOfArraysOverride,
    consumed_pointer_params,
)
from intermediates import Api, CmdType, Command, EnumType, EnumValue, Parameter, ReturnValue, TypeFacts
from object_name_lifecycle import (
    HANDLE_ARB_CLASSES,
    IDENTITY_NAME_CLASSES,
    UNMAPPED_HANDLE_ARB_FIELDS,
    ObjectNameOverride,
    ObjectNameRole,
    resolve_object_name_override,
)
from pixel_transfer_metadata import derive_pixel_transfer_override
from pname_value_counts import derive_pname_value_count_override
from pointer_metadata import POINTER_PARAM_OVERRIDES, LengthSourceKind, PointerDirection, PointerParamOverride
from registry import Registry
from pointer_length_derivation import derive_pointer_param_override
from tex_sampler_parameter_metadata import derive_tex_sampler_parameter_override
from uniform_setter_metadata import derive_uniform_setter_override

THIRD_PARTY_DIR = Path(__file__).resolve().parents[2] / 'third_party'
# TODO: Take dirs as arguments, like Vulkan's new generator.py does.
FILE_TO_API: dict[Path, Api] = {
    THIRD_PARTY_DIR / 'OpenGL-Registry/xml/gl.xml': Api.GL,
    THIRD_PARTY_DIR / 'OpenGL-Registry/xml/wgl.xml': Api.WGL,
    THIRD_PARTY_DIR / 'OpenGL-Registry/xml/glx.xml': Api.GLX,
    THIRD_PARTY_DIR / 'EGL-Registry/api/egl.xml': Api.EGL,
}


# ---------------------------------------------------------------------------
# Raw XML extraction.
# ---------------------------------------------------------------------------

def normalize_type(type_text: str) -> str:
    """Normalize whitespace in reconstructed C/C++ type text."""
    text = re.sub(r'\s+', ' ', type_text).strip()
    # Ensure single space before asterisk (or a group of them).
    return re.sub(r'\s*(\*+)', r' \1', text)


def reconstruct_type_from_xml_elem(elem: Any) -> str:
    """Reconstruct declaration type text from XML mixed content before <name>.

    XML proto and param elements split C declarations across text nodes, an
    optional <ptype> child, and their tails. This function joins everything up
    to the <name> child and normalizes whitespace to recover the type string
    (e.g. 'const void *') used for the Command's return value and parameter types.
    """
    fragments: list[str] = []
    if elem.text:
        fragments.append(elem.text)

    for child in elem:
        if child.tag == 'name':
            break
        if child.text:
            fragments.append(child.text)
        if child.tail:
            fragments.append(child.tail)

    return normalize_type(''.join(fragments))


def extract_commands_from_registry(registry: Registry) -> list[dict[str, Any]]:
    """Extract raw command data from a registry's cmddict.

    For each <command> returns a dict with:
      - 'command_attribs': <command> attributes ('name' injected by reg.py plus
        an optional 'comment').
      - 'return_value': {'type': return type reconstructed from <proto>, 'attribs':
        <proto> attributes (group/class/kind)}.
      - 'params': list of {'name', 'type', 'attribs'}, where 'attribs' are the
        <param> attributes (group/class/kind/len).
      - 'alias_attribs': <alias> attributes (the aliased command's 'name').

    Type strings are reconstructed from each element's mixed content (everything
    before its <name> child). The <glx>/<vecequiv> child elements and the info
    reg.py derives from them are ignored.
    """
    raw_commands: list[dict[str, Any]] = []

    for cmd_obj in registry.cmddict.values():
        cmd_elem = cmd_obj
        proto_elem = cmd_elem.find('proto')
        alias_elem = cmd_elem.find('alias')

        cmd_name = cmd_elem.attrib.get('name')
        if cmd_name is None:
            raise ValueError("<command> element has no 'name' attribute")
        if proto_elem is None:
            raise ValueError(f"<command> '{cmd_name}' has no <proto> element")

        params: list[dict[str, Any]] = []
        for param_elem in cmd_elem.findall('param'):
            name_elem = param_elem.find('name')
            if name_elem is None:
                raise ValueError(f"<param> in command '{cmd_name}' has no <name> element")
            params.append({
                'name': name_elem.text or '',
                'type': reconstruct_type_from_xml_elem(param_elem),
                'attribs': dict(param_elem.attrib),
            })

        raw_commands.append({
            'command_attribs': dict(cmd_elem.attrib),
            'return_value': {
                'type': reconstruct_type_from_xml_elem(proto_elem),
                'attribs': dict(proto_elem.attrib),
            },
            'params': params,
            'alias_attribs': dict(alias_elem.attrib) if alias_elem is not None else {},
        })

    return raw_commands


# ---------------------------------------------------------------------------
# Attribute validation.
# ---------------------------------------------------------------------------

def find_unknown_command_attributes(raw_commands: list[dict[str, Any]]) -> set[str]:
    """Return XML attributes on commands (and their children) not on the known list.

    Inspects XML attributes on <command> and its children (<proto>, <param>, and <alias>;
    <glx> is ignored). Anything not on the allowlists below is reported so we notice
    new/unexpected attributes when the registries change.

    Attributes encountered across the four registries (gl/wgl/glx/egl):
        | element  | gl.xml                  | wgl.xml | glx.xml | egl.xml |
        |----------|-------------------------|---------|---------|---------|
        | command  | name*, comment          | name*   | name*   | name*   |
        | proto    | group, class, kind      | -       | -       | -       |
        | param    | group, class, kind, len | -       | -       | -       |
        | alias    | name                    | -       | -       | name    |

    Note: <command> has no 'name' attribute in the XML files; reg.py injects it based on
    <proto>'s <name>.
    """
    known_command_attribs: set[str] = {
        'name',  # reg.py injects this from <proto><name>; not present in the XML.
        'comment',
    }
    known_proto_attribs: set[str] = {
        'group',
        'class',
        'kind',
    }
    known_param_attribs: set[str] = {
        'group',
        'class',
        'kind',
        'len',
    }
    known_alias_attribs: set[str] = {
        'name',
    }

    novel: set[str] = set()

    for cmd in raw_commands:
        for attrib in cmd['command_attribs']:
            if attrib not in known_command_attribs:
                novel.add(f'command:{attrib}')
        for attrib in cmd['return_value']['attribs']:
            if attrib not in known_proto_attribs:
                novel.add(f'proto:{attrib}')
        for param in cmd['params']:
            for attrib in param['attribs']:
                if attrib not in known_param_attribs:
                    novel.add(f'param:{attrib}')
        for attrib in cmd['alias_attribs']:
            if attrib not in known_alias_attribs:
                novel.add(f'alias:{attrib}')

    return novel


def find_unknown_enum_attributes(raw_enums: list[dict[str, Any]]) -> set[str]:
    """Return attributes on enums that aren't on the known list."""
    known_attribs = set(EnumValue.__dataclass_fields__.keys())
    known_attribs.remove('groups')
    known_attribs.add('group') # XMLs use 'group', our EnumValue uses 'groups'.

    novel: set[str] = set()
    for enum_dict in raw_enums:
        for attrib in enum_dict:
            if attrib not in known_attribs:
                novel.add(attrib)

    return novel


# ---------------------------------------------------------------------------
# Raw dict -> IR conversion.
# ---------------------------------------------------------------------------

def calculate_types(enums_list: list[dict[str, Any]]) -> None:
    """Replace C literal suffix strs with EnumType enums."""
    enum: dict[str, Any]
    for enum in enums_list:
        type_literal_suffix: str = enum.get('type', '')
        enum['type'] = EnumType(type_literal_suffix)


def split_groups(enums_list: list[dict[str, Any]]) -> None:
    """Split the comma-separated group string into a tuple of group names."""
    enum: dict[str, Any]
    for enum in enums_list:
        group: str | None = enum.get('group')
        if group:
            enum['groups'] = tuple(group.split(','))
            del enum['group']
        else:
            enum['groups'] = ()


def without_nongl(enums_list: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Return a list containing only GLenums with api="gl" or no api attribute."""
    return [enum for enum in enums_list if enum.get('api') in (None, 'gl')]


def get_enums_from_xml_data(raw_enums: dict[Api, list[dict[str, Any]]]) -> dict[Api, list[EnumValue]]:
    """Take raw enum data from XML and return processed enums; both grouped by API (registry)."""
    result: dict[Api, list[EnumValue]] = {api: [] for api in Api}

    for api, enum_list in raw_enums.items():
        # Operate on a list of shallow copies of each raw enum dict to avoid tracking
        # whether the enum list was already processed. Current processing functions
        # modify the dicts but not the values inside them, so a shallow copy is enough.
        # If you add new processing logic, think if it needs a deep copy or not.
        shallow_copies = [dict(e) for e in enum_list]

        # XML data needs some processing.
        calculate_types(shallow_copies)
        split_groups(shallow_copies)

        # Skip enums with api field set and different from 'gl'. This avoids
        # defining the same enum twice with different values.
        shallow_copies = without_nongl(shallow_copies)

        result[api] = [EnumValue(**e) for e in shallow_copies]

    return result


def normalize_attribs(attribs: dict[str, Any]) -> dict[str, Any]:
    """Normalize raw XML attributes for use as Parameter/ReturnValue keyword arguments.

    - Renames 'class' to 'class_' to avoid conflict with the Python keyword.
    - Converts 'kind' into 'kinds': a list of comma-separated values (stripped),
      since gl.xml uses multi-valued annotations like 'Clamped[0; 1],Color'.
    """
    result: dict[str, Any] = {}
    for k, v in attribs.items():
        if k == 'class':
            result['class_'] = v
        elif k == 'kind':
            result['kinds'] = tuple(part.strip() for part in v.split(',') if part.strip())
        else:
            result[k] = v
    return result


def _validate_pointer_override(
    command_name: str, param: Parameter, override: PointerParamOverride, other_params: list[Parameter],
) -> None:
    """Raise ValueError if an override table entry doesn't match this command's actual
    signature - a broken table entry is a codegen bug, caught at generation time, not a reason
    to silently leave the command disabled (contrast: a *missing* entry is always silent and
    safe - see _is_param_capturable)."""
    if override.length_multiplier < 1:
        raise ValueError(
            f"{command_name}.{param.name}: length_multiplier must be >= 1, got "
            f"{override.length_multiplier}"
        )
    # unpack_pbo_guard/query_buffer_guard each rely on their fixed
    # direction to decide which Mako call site actually wires the corresponding
    # GuardUnpackPixelsPointer()/GuardQueryBufferPointer() call in - see each field's own
    # comment (pointer_metadata.py). A mismatched direction would leave the flag silently
    # inert (checked nowhere, guarding nothing) rather than failing loud, exactly the kind of
    # broken-table-entry-as-a-bug this function already treats every other shape mismatch as.
    if override.unpack_pbo_guard and override.direction is not PointerDirection.IN:
        raise ValueError(
            f"{command_name}.{param.name}: unpack_pbo_guard requires IN direction - "
            f"GL_PIXEL_UNPACK_BUFFER only ever redirects an application-supplied (i.e. "
            f"IN-direction) pointer, never a driver-written OUT one"
        )
    if override.unpack_pbo_guard and override.length_source_kind is LengthSourceKind.PIXEL_TRANSFER_SIZE:
        raise ValueError(
            f"{command_name}.{param.name}: unpack_pbo_guard is redundant with "
            f"PIXEL_TRANSFER_SIZE, which already applies this exact guard unconditionally - "
            f"do not set both"
        )
    if override.query_buffer_guard and override.direction is not PointerDirection.OUT:
        raise ValueError(
            f"{command_name}.{param.name}: query_buffer_guard requires OUT direction - "
            f"GL_QUERY_BUFFER only ever redirects a driver-written (i.e. OUT-direction) "
            f"result, never an application-supplied IN one"
        )
    if override.length_source_kind is LengthSourceKind.SIBLING_PARAM:
        sibling = next((p for p in other_params if p.name == override.length_source_value), None)
        if sibling is None:
            raise ValueError(
                f"{command_name}.{param.name}: POINTER_PARAM_OVERRIDES names sibling length "
                f"parameter '{override.length_source_value}', which does not exist on this command"
            )
        if not sibling.type_facts.is_scalar:
            raise ValueError(
                f"{command_name}.{param.name}: length sibling '{sibling.name}' is not a plain "
                f"scalar ({sibling.type})"
            )
    elif override.length_source_kind is LengthSourceKind.FIXED_COUNT:
        if override.length_source_value is None or not override.length_source_value.isdigit():
            raise ValueError(
                f"{command_name}.{param.name}: FIXED_COUNT override needs a non-negative "
                f"integer literal, got {override.length_source_value!r}"
            )
    elif override.length_source_kind is LengthSourceKind.OPAQUE_BIT_PATTERN:
        if override.direction is not PointerDirection.IN:
            raise ValueError(
                f"{command_name}.{param.name}: OPAQUE_BIT_PATTERN override must be IN "
                f"direction - the pointer's bit pattern is a value, not a buffer the driver "
                f"writes into"
            )
        if override.length_source_value is not None:
            raise ValueError(
                f"{command_name}.{param.name}: OPAQUE_BIT_PATTERN override must not set "
                f"length_source_value - there is no length, only one opaque value"
            )
    elif override.length_source_kind is LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY:
        # Same IN-only direction restriction as the single-pointer OPAQUE_BIT_PATTERN case
        # above, for the identical reason: every element is a value (a client-memory pointer
        # or buffer-relative offset), never a buffer the driver writes into - there is no
        # OUT-direction "array of opaque values the driver produces" shape in gl.xml today, and
        # commandsAuto.h.mako's own OPAQUE_BIT_PATTERN_ARRAY branch only ever generates an
        # IN-direction (eager-capture-before-the-call) constructor call, with no OUT-direction
        # counterpart to fall back to.
        if override.direction is not PointerDirection.IN:
            raise ValueError(
                f"{command_name}.{param.name}: OPAQUE_BIT_PATTERN_ARRAY override must be IN "
                f"direction - each element is a value, not part of a buffer the driver writes "
                f"into"
            )
        # Same sibling-length shape/validation as SIBLING_PARAM above - see that
        # LengthSourceKind's own comment (pointer_metadata.py) for why the element treatment
        # differs even though the length-discovery mechanism does not.
        sibling = next((p for p in other_params if p.name == override.length_source_value), None)
        if sibling is None:
            raise ValueError(
                f"{command_name}.{param.name}: POINTER_PARAM_OVERRIDES names sibling length "
                f"parameter '{override.length_source_value}', which does not exist on this command"
            )
        if not sibling.type_facts.is_scalar:
            raise ValueError(
                f"{command_name}.{param.name}: length sibling '{sibling.name}' is not a plain "
                f"scalar ({sibling.type})"
            )
    elif override.length_source_kind is LengthSourceKind.PIXEL_TRANSFER_SIZE:
        if override.length_source_value is not None:
            raise ValueError(
                f"{command_name}.{param.name}: PIXEL_TRANSFER_SIZE override must not set "
                f"length_source_value - use pixel_format_param/pixel_type_param/"
                f"pixel_width_param/pixel_height_param/pixel_depth_param instead"
            )
        if override.pixel_depth_param is not None and override.pixel_height_param is None:
            raise ValueError(
                f"{command_name}.{param.name}: PIXEL_TRANSFER_SIZE's pixel_depth_param "
                f"requires pixel_height_param to also be set - no 1D-with-depth shape exists"
            )
        for field_name in ('pixel_format_param', 'pixel_type_param', 'pixel_width_param'):
            if getattr(override, field_name) is None:
                raise ValueError(
                    f"{command_name}.{param.name}: PIXEL_TRANSFER_SIZE override must set "
                    f"{field_name}"
                )
        for field_name in (
            'pixel_format_param', 'pixel_type_param', 'pixel_width_param',
            'pixel_height_param', 'pixel_depth_param',
        ):
            sibling_name = getattr(override, field_name)
            if sibling_name is None:
                continue  # pixel_height_param/pixel_depth_param may be legitimately absent -
                          # required-ness is already checked above.
            sibling = next((p for p in other_params if p.name == sibling_name), None)
            if sibling is None:
                raise ValueError(
                    f"{command_name}.{param.name}: PIXEL_TRANSFER_SIZE's {field_name} names "
                    f"'{sibling_name}', which does not exist on this command"
                )
            if not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: PIXEL_TRANSFER_SIZE's {field_name} sibling "
                    f"'{sibling.name}' is not a plain scalar ({sibling.type})"
                )
    elif override.length_source_kind in (
        LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT, LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT,
        LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT, LengthSourceKind.POINT_PARAMETER_VALUE_COUNT,
        LengthSourceKind.PNAME_VALUE_COUNT,
    ):
        if override.length_source_value is not None:
            raise ValueError(
                f"{command_name}.{param.name}: {override.length_source_kind} override must not "
                f"set length_source_value - use pname_param instead"
            )
        if override.direction is not PointerDirection.IN:
            # No enabled command needs an OUT-direction user of either kind today - see
            # commandsAuto.h.mako/recorderWrappersAuto.cpp.mako, which only ever generate an
            # IN-direction branch for these two kinds (mirroring OPAQUE_BIT_PATTERN's own
            # direction restriction above) - fail loud here rather than silently falling
            # through to a Mako branch that would try to print a nonexistent
            # length_source_value.
            raise ValueError(
                f"{command_name}.{param.name}: {override.length_source_kind} override must be "
                f"IN direction - no OUT-direction user exists yet, and the Mako templates have "
                f"no OUT-direction branch for it"
            )
        if override.pname_param is None:
            raise ValueError(
                f"{command_name}.{param.name}: {override.length_source_kind} override must "
                f"set pname_param"
            )
        sibling = next((p for p in other_params if p.name == override.pname_param), None)
        if sibling is None:
            raise ValueError(
                f"{command_name}.{param.name}: {override.length_source_kind}'s pname_param "
                f"names '{override.pname_param}', which does not exist on this command"
            )
        if not sibling.type_facts.is_scalar:
            raise ValueError(
                f"{command_name}.{param.name}: {override.length_source_kind}'s pname_param "
                f"sibling '{sibling.name}' is not a plain scalar ({sibling.type})"
            )
        # tex_sampler_family is TEX_SAMPLER_PARAMETER_VALUE_COUNT-only - it
        # disambiguates GL_TEXTURE_SWIZZLE_RGBA's family-dependent legality (see
        # pointer_metadata.py's TexOrSamplerParameterFamily comment), which none of
        # PATCH_PARAMETER_VALUE_COUNT/MATERIAL_PARAMETER_VALUE_COUNT/
        # POINT_PARAMETER_VALUE_COUNT has an equivalent of at all (each is the only command in
        # its own family - see tex_sampler_parameter_metadata.py's own header comment).
        # Enforce both halves explicitly rather than silently ignoring a mismatched/missing
        # field on either kind - a broken table/derivation entry is a bug, matching every other
        # check in this function.
        if override.length_source_kind is LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT:
            if override.tex_sampler_family is None:
                raise ValueError(
                    f"{command_name}.{param.name}: TEX_SAMPLER_PARAMETER_VALUE_COUNT override "
                    f"must set tex_sampler_family"
                )
        elif override.tex_sampler_family is not None:
            raise ValueError(
                f"{command_name}.{param.name}: {override.length_source_kind} override must not "
                f"set tex_sampler_family - only TEX_SAMPLER_PARAMETER_VALUE_COUNT uses it"
            )
    elif override.length_source_kind is LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT:
        # See that LengthSourceKind's own comment (pointer_metadata.py).
        # Mirrors the TEX_SAMPLER_PARAMETER_VALUE_COUNT/PATCH_PARAMETER_VALUE_COUNT block
        # above structurally (a pname_param sibling drives a real runtime helper's answer),
        # but with the opposite direction restriction: this is a getter's OUT-direction
        # `params`, never an IN-direction setter value.
        if override.length_source_value is not None:
            raise ValueError(
                f"{command_name}.{param.name}: VERTEX_ATTRIB_PARAMETER_VALUE_COUNT override "
                f"must not set length_source_value - use pname_param instead"
            )
        if override.direction is not PointerDirection.OUT:
            raise ValueError(
                f"{command_name}.{param.name}: VERTEX_ATTRIB_PARAMETER_VALUE_COUNT override "
                f"must be OUT direction - glGetVertexAttribiv's `params` is written by the "
                f"driver, not read from application memory"
            )
        if override.pname_param is None:
            raise ValueError(
                f"{command_name}.{param.name}: VERTEX_ATTRIB_PARAMETER_VALUE_COUNT override "
                f"must set pname_param"
            )
        sibling = next((p for p in other_params if p.name == override.pname_param), None)
        if sibling is None:
            raise ValueError(
                f"{command_name}.{param.name}: VERTEX_ATTRIB_PARAMETER_VALUE_COUNT's "
                f"pname_param names '{override.pname_param}', which does not exist on this "
                f"command"
            )
        if not sibling.type_facts.is_scalar:
            raise ValueError(
                f"{command_name}.{param.name}: VERTEX_ATTRIB_PARAMETER_VALUE_COUNT's "
                f"pname_param sibling '{sibling.name}' is not a plain scalar ({sibling.type})"
            )
        if override.tex_sampler_family is not None:
            raise ValueError(
                f"{command_name}.{param.name}: VERTEX_ATTRIB_PARAMETER_VALUE_COUNT override "
                f"must not set tex_sampler_family - only TEX_SAMPLER_PARAMETER_VALUE_COUNT "
                f"uses it"
            )
    elif override.length_source_kind is LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT:
        if override.length_source_value is not None:
            raise ValueError(
                f"{command_name}.{param.name}: CLEAR_BUFFER_VALUE_COUNT override must not set "
                f"length_source_value - use clear_buffer_param instead"
            )
        if override.clear_buffer_param is None:
            raise ValueError(
                f"{command_name}.{param.name}: CLEAR_BUFFER_VALUE_COUNT override must set "
                f"clear_buffer_param"
            )
        sibling = next((p for p in other_params if p.name == override.clear_buffer_param), None)
        if sibling is None:
            raise ValueError(
                f"{command_name}.{param.name}: CLEAR_BUFFER_VALUE_COUNT's clear_buffer_param "
                f"names '{override.clear_buffer_param}', which does not exist on this command"
            )
        if not sibling.type_facts.is_scalar:
            raise ValueError(
                f"{command_name}.{param.name}: CLEAR_BUFFER_VALUE_COUNT's clear_buffer_param "
                f"sibling '{sibling.name}' is not a plain scalar ({sibling.type})"
            )
    elif override.length_source_kind is LengthSourceKind.DISCARDED_OUTPUT:
        if override.direction is not PointerDirection.OUT:
            raise ValueError(
                f"{command_name}.{param.name}: DISCARDED_OUTPUT override must be OUT direction - "
                f"an IN value the driver reads can never be dropped"
            )
        if override.length_source_value is not None:
            sibling = next((p for p in other_params if p.name == override.length_source_value), None)
            if sibling is None or not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: DISCARDED_OUTPUT's length_source_value "
                    f"'{override.length_source_value}' must name a scalar capacity sibling"
                )
        if param.type_facts.base_type == 'void' and param.type_facts.pointer_depth != 2:
            raise ValueError(
                f"{command_name}.{param.name}: DISCARDED_OUTPUT needs a typed element - the "
                f"player's scratch buffer is sized in elements"
            )
    elif override.length_source_kind is LengthSourceKind.TEXEL_BYTE_SIZE:
        if override.length_source_value is not None:
            raise ValueError(
                f"{command_name}.{param.name}: TEXEL_BYTE_SIZE override must not set "
                f"length_source_value - use pixel_format_param/pixel_type_param instead"
            )
        if override.direction is not PointerDirection.IN:
            raise ValueError(
                f"{command_name}.{param.name}: TEXEL_BYTE_SIZE override must be IN direction - "
                f"the Mako templates have no OUT-direction branch for it"
            )
        for field_name in ('pixel_format_param', 'pixel_type_param'):
            sibling_name = getattr(override, field_name)
            sibling = next((p for p in other_params if p.name == sibling_name), None)
            if sibling is None or not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: TEXEL_BYTE_SIZE's {field_name} must name a "
                    f"plain scalar sibling (got {sibling_name!r})"
                )
    elif override.length_source_kind is LengthSourceKind.INDIRECT_COMMANDS_BYTE_SIZE:
        if override.direction is not PointerDirection.IN:
            raise ValueError(
                f"{command_name}.{param.name}: INDIRECT_COMMANDS_BYTE_SIZE override must be IN "
                f"direction - the driver only reads draw commands"
            )
        if override.indirect_command_size <= 0:
            raise ValueError(
                f"{command_name}.{param.name}: INDIRECT_COMMANDS_BYTE_SIZE needs a positive "
                f"indirect_command_size"
            )
        if (override.length_source_value is None) != (override.indirect_stride_param is None):
            raise ValueError(
                f"{command_name}.{param.name}: INDIRECT_COMMANDS_BYTE_SIZE needs both a count "
                f"and a stride sibling, or neither (a single draw command)"
            )
        for field_name in ('length_source_value', 'indirect_stride_param'):
            sibling_name = getattr(override, field_name)
            if sibling_name is None:
                continue
            sibling = next((p for p in other_params if p.name == sibling_name), None)
            if sibling is None or not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: INDIRECT_COMMANDS_BYTE_SIZE's {field_name} "
                    f"must name a plain scalar sibling (got {sibling_name!r})"
                )
    elif override.length_source_kind is LengthSourceKind.ELEMENT_INDICES_BYTE_SIZE:
        if override.direction is not PointerDirection.IN:
            raise ValueError(
                f"{command_name}.{param.name}: ELEMENT_INDICES_BYTE_SIZE override must be IN "
                f"direction - the driver only reads indices"
            )
        for field_name in ('length_source_value', 'index_type_param'):
            sibling_name = getattr(override, field_name)
            sibling = next((p for p in other_params if p.name == sibling_name), None)
            if sibling is None or not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: ELEMENT_INDICES_BYTE_SIZE's {field_name} "
                    f"must name a plain scalar sibling (got {sibling_name!r})"
                )
    elif override.length_source_kind is LengthSourceKind.LIST_NAME_ARRAY_BYTE_SIZE:
        if override.direction is not PointerDirection.IN:
            raise ValueError(
                f"{command_name}.{param.name}: LIST_NAME_ARRAY_BYTE_SIZE override must be IN "
                f"direction - the Mako templates have no OUT-direction branch for it"
            )
        for field_name in ('length_source_value', 'list_type_param'):
            sibling_name = getattr(override, field_name)
            sibling = next((p for p in other_params if p.name == sibling_name), None)
            if sibling is None or not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: LIST_NAME_ARRAY_BYTE_SIZE's {field_name} must "
                    f"name a plain scalar sibling (got {sibling_name!r})"
                )
    elif override.length_source_kind is LengthSourceKind.BITMAP_BYTE_SIZE:
        if override.direction is not PointerDirection.IN or not override.unpack_pbo_guard:
            raise ValueError(
                f"{command_name}.{param.name}: BITMAP_BYTE_SIZE override must be IN direction "
                f"with unpack_pbo_guard - the bitmap can be a GL_PIXEL_UNPACK_BUFFER offset"
            )
        if (override.pixel_width_param is None) != (override.pixel_height_param is None):
            raise ValueError(
                f"{command_name}.{param.name}: BITMAP_BYTE_SIZE needs both a width and a height "
                f"sibling, or neither (a 32x32 polygon stipple)"
            )
        for field_name in ('pixel_width_param', 'pixel_height_param'):
            sibling_name = getattr(override, field_name)
            if sibling_name is None:
                continue
            sibling = next((p for p in other_params if p.name == sibling_name), None)
            if sibling is None or not sibling.type_facts.is_scalar:
                raise ValueError(
                    f"{command_name}.{param.name}: BITMAP_BYTE_SIZE's {field_name} must name a "
                    f"plain scalar sibling (got {sibling_name!r})"
                )
    # NULL_TERMINATED/KEY_VALUE_LIST need no structural validation - neither has a sibling to
    # cross-check.


def _validate_array_of_arrays_override(
    command_name: str, param: Parameter, override: ArrayOfArraysOverride, other_params: list[Parameter],
) -> None:
    """Raise ValueError if an ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry doesn't match this
    command's actual signature - mirrors _validate_pointer_override's precedent exactly, for
    the two sibling parameters (outer_count_param/element_length_param) instead of one."""
    outer = next((p for p in other_params if p.name == override.outer_count_param), None)
    if outer is None:
        raise ValueError(
            f"{command_name}.{param.name}: ARRAY_OF_ARRAYS_PARAM_OVERRIDES names outer count "
            f"parameter '{override.outer_count_param}', which does not exist on this command"
        )
    if not outer.type_facts.is_scalar:
        raise ValueError(
            f"{command_name}.{param.name}: outer count '{outer.name}' is not a plain scalar "
            f"({outer.type})"
        )
    if override.element_length_param is None:
        # No element-length sibling to validate at all - see ArrayOfArraysOverride's own
        # comment for why this is legitimate (glCreateShaderProgramv/
        # glTransformFeedbackVaryings, neither of which gl.xml gives a `length` parameter).
        return
    length_param = next((p for p in other_params if p.name == override.element_length_param), None)
    if length_param is None:
        raise ValueError(
            f"{command_name}.{param.name}: ARRAY_OF_ARRAYS_PARAM_OVERRIDES names element "
            f"length parameter '{override.element_length_param}', which does not exist on "
            f"this command"
        )
    if not length_param.type_facts.is_array_element_eligible:
        raise ValueError(
            f"{command_name}.{param.name}: element length parameter '{length_param.name}' is "
            f"not a single-indirection pointer to a scalar type ({length_param.type})"
        )


def _requires_object_name_override(command_name: str, field: Parameter) -> bool:
    """Whether `field` (a parameter or a ReturnValue) is semantically a GL object name and
    therefore MUST resolve to a real object-name classification (see
    object_name_lifecycle.py's resolve_object_name_override) to be capturable - the systemic
    gate behind this module's own targeted, per-command fixes (see object_name_lifecycle.py's
    header comment and modern-opengl-backend-architecture.md's "scalar-enabled-but-unmapped"
    deferred-scope note, which this gate now enforces/produces instead of relying on manual
    audits to find each gap one at a time).

    "Semantically a GL object name" is answered by the registry itself, not by guessing from
    the field's name or C type: gl.xml's <param class="..."> / <proto class="..."> annotation
    (normalize_attribs' 'class' -> 'class_') is *only* ever used by the registry authors to tag
    a field as belonging to a specific object-name namespace (texture/buffer/program/shader/
    framebuffer/vertex array/query/renderbuffer/sampler/program pipeline/transform feedback/
    display list/perf query handle/perf query id) or a sync-object handle (class="sync",
    GLsync). No other `class` value exists in any of the four registries this generator reads
    (verified against gl.xml/wgl.xml/glx.xml/egl.xml - only gl.xml ever sets it). So
    `field.class_ is not None` is a complete, data-driven test with no hand-maintained keyword
    list to fall out of sync with the registries, exactly the kind of "classify from evidence"
    signal object_name_lifecycle.py's own header comment calls for.

    is_mapped_handle fields (HGLRC, GLsync) are always exempted up front, regardless of
    `class_`: those types get a real, dedicated GITSKey mapping of their own (arguments.h's
    HandleArgument<T> plus ContextHandleMapService/SyncHandleMapService - see
    intermediates.py's _MAPPED_HANDLE_BASE_TYPES header comment), which is a strictly stronger
    guarantee than anything ObjectNameMapService provides for GLuint-named objects. GLsync is
    the case that makes this exemption load-bearing rather than vacuous: gl.xml tags every
    GLsync field with class="sync" (the same <param class="..."> mechanism used for texture/
    buffer/program/... names), and object_name_lifecycle.py's _CLASS_TO_KIND deliberately maps
    'sync' to None (no GLObjectKind - GLsync objects are not part of any GLuint namespace, so
    building one for them would be actively wrong, not just unbuilt). Without this exemption,
    every enabled GLsync field would look identical to a real "unsupported object-name
    namespace" gap and permanently disable glFenceSync/glIsSync/glClientWaitSync/glWaitSync/
    glDeleteSync even though SyncHandleMapService already gives them a complete, real mapping.

    resolve_object_name_override() itself distinguishes "not an object name at all"
    (class_ is None) from "a namespace this backend has not built ObjectNameMapService support
    for yet" (e.g. "transform feedback", "display list", "perf query id"/"perf query handle")
    from "a real, resolvable classification" - but this gate treats the first two identically:
    either way, a field with no resolvable override leaves the whole command disabled until a
    real classification exists for every one of its object-name fields (see this module's own
    re-run against the codebase, and the deferred list this gate validates in
    modern-opengl-backend-architecture.md).
    """
    if field.type_facts.is_mapped_handle:
        return False
    if field.class_ in IDENTITY_NAME_CLASSES:
        return False
    if field.class_ is None and field.type_facts.base_type == 'GLhandleARB':
        return (command_name, field.name) not in UNMAPPED_HANDLE_ARB_FIELDS
    return (field.class_ is not None
            and resolve_object_name_override(command_name, field.name, field.class_) is None)


def _pointer_override_for_command_or_alias(
    command_name: str, param_name: str, alias: str | None,
) -> PointerParamOverride | None:
    """POINTER_PARAM_OVERRIDES lookup that falls back to `alias`'s entry for the same
    `param_name` when `command_name` has no entry of its own.

    gl.xml's <alias name="..."/> element (Command.alias, populated in
    get_commands_from_xml_data() below) marks `command_name` as a byte-for-byte functional
    duplicate of `alias`'s command - the same underlying entry point under a pre-promotion
    extension name (e.g. glDeleteBuffersARB/glGenBuffersARB/glBufferDataARB/
    glVertexAttribPointerARB/glCompressedTexImage2DARB alias their later-promoted-to-core
    glDeleteBuffers/glGenBuffers/glBufferData/glVertexAttribPointer/glCompressedTexImage2D, and
    glGenFramebuffersEXT aliases glGenFramebuffers), not merely a similarly-named, independently
    -shaped command - so an already-validated POINTER_PARAM_OVERRIDES entry keyed to the alias
    target applies unchanged. This makes an entire family of ARB/EXT/OES-suffixed names
    capturable from ONE hand-authored table entry (the promoted core command's own), instead of
    requiring a byte-for-byte duplicate entry per suffixed name - the "generic" fix; see
    modern-opengl-backend-architecture.md's command-support-contract section.

    Does not change validation: _validate_pointer_override is still called (by every caller of
    this function) with `command_name`'s OWN param/other_params, so an alias whose actual
    gl.xml shape (len=, sibling names) has drifted from its target is still caught, never
    silently trusted just because the alias link exists.

    Deliberately not extended to ARRAY_OF_ARRAYS_PARAM_OVERRIDES: glShaderSourceARB is the only
    ARB/EXT alias needing the doubly-indirect shape that table covers, and it has its own entry.
    """
    override = POINTER_PARAM_OVERRIDES.get((command_name, param_name))
    if override is not None:
        return override
    if alias is None:
        return None
    return POINTER_PARAM_OVERRIDES.get((alias, param_name))


def _is_param_capturable(
    command_name: str, param: Parameter, other_params: list[Parameter], alias: str | None = None,
) -> bool:
    """Whether a single parameter is safe to capture: a fixed-size scalar/enum, an opaque
    native handle, a native handle GITS gives a real GITSKey mapping (see TypeFacts.is_scalar /
    is_opaque_handle / is_mapped_handle); a parameter absorbed into a sibling's ArrayOfArrays<T>
    wrapper (see array_of_arrays_metadata.py's consumed_pointer_params - it gets no Command
    member of its own, but that is not a reason to leave the *command* disabled); a doubly
    -indirect pointer (is_array_of_arrays_eligible) with a validated
    ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry; or a pointer to a memcpy-safe element type
    (is_array_element_eligible) that also has an explicit, validated POINTER_PARAM_OVERRIDES
    entry, either `command_name`'s own or (see _pointer_override_for_command_or_alias)
    inherited from `alias` (see pointer_metadata.py and "Classify payloads from evidence" -
    direction/length are never inferred from the type alone). A pointer parameter with no
    override entry (own or inherited) simply stays uncapturable; that is always the safe
    outcome.

    On top of all of that, ANY of the above shapes is only actually capturable if it is not a
    GL object name missing its required OBJECT_NAME_PARAM_OVERRIDES entry - see
    _requires_object_name_override. This is deliberately checked for every branch (scalar,
    array-of-arrays, and array-element) rather than just the scalar case: an unmapped object
    name passed through as a raw ArrayArgument<GLuint>/HandleArrayArgument-less array is exactly
    as unsafe on replay as an unmapped scalar one (e.g. glDeleteBuffers' `buffers` - a plain
    is_array_element_eligible pointer with a valid POINTER_PARAM_OVERRIDES entry, but no
    OBJECT_NAME_PARAM_OVERRIDES entry of its own before this gate existed).
    """
    facts = param.type_facts
    if facts.is_scalar or facts.is_opaque_handle or facts.is_mapped_handle:
        return not _requires_object_name_override(command_name, param)
    if param.name in consumed_pointer_params(command_name):
        return True
    override = _pointer_override_for_command_or_alias(command_name, param.name, alias)
    if override is not None and override.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY:
        # Checked before is_array_of_arrays_eligible below on purpose: structurally this is a
        # pointer_depth == 2 `const void* const*` field exactly like glShaderSource's `string`
        # (is_array_of_arrays_eligible is true for it too), but its elements are opaque bit
        # patterns, never real byte arrays to copy - ArrayOfArrays<T>/
        # ARRAY_OF_ARRAYS_PARAM_OVERRIDES would be actively wrong here, not just an unrelated
        # alternative path. See that LengthSourceKind's own comment (pointer_metadata.py).
        if _requires_object_name_override(command_name, param):
            return False
        _validate_pointer_override(command_name, param, override, other_params)
        return True
    if override is None and facts.pointer_depth == 2 and facts.base_type == 'void':
        # glGet*Pointerv's `void **` result - see derive_pointer_query_output_override().
        override = derive_pointer_param_override(command_name, param, other_params)
        if override is None:
            return False
        _validate_pointer_override(command_name, param, override, other_params)
        return True
    if facts.is_array_of_arrays_eligible:
        if _requires_object_name_override(command_name, param):
            return False
        aoa_override = ARRAY_OF_ARRAYS_PARAM_OVERRIDES.get((command_name, param.name))
        if aoa_override is None:
            return False
        _validate_array_of_arrays_override(command_name, param, aoa_override, other_params)
        return True
    if not facts.is_array_element_eligible:
        return False
    if _requires_object_name_override(command_name, param):
        return False
    override = _pointer_override_for_command_or_alias(command_name, param.name, alias)
    if override is None:
        # Explicit table entry always wins; only fall back to the generic
        # COMPSIZE(format,type,<dims>) pixel-transfer auto-derivation when this (command,
        # param) has no hand-authored entry at all - see pixel_transfer_metadata.py's own
        # header comment for exactly what evidence it requires before returning non-None.
        override = derive_pixel_transfer_override(param, other_params)
    if override is None:
        # Second fallback: the glUniform*v/glProgramUniform*v/glUniformMatrix*v/
        # glProgramUniformMatrix*v sibling-family auto-derivation - see
        # uniform_setter_metadata.py's own header comment for exactly what evidence it
        # requires before returning non-None.
        override = derive_uniform_setter_override(command_name, param, other_params)
    if override is None:
        # Third fallback: the glTexParameter*v/glSamplerParameter*v sibling-family
        # auto-derivation - see tex_sampler_parameter_metadata.py's own header comment for
        # exactly what evidence it requires before returning non-None.
        override = derive_tex_sampler_parameter_override(command_name, param, other_params)
    if override is None:
        # Audited COMPSIZE(pname) setter groups - see pname_value_counts.py.
        override = derive_pname_value_count_override(command_name, param, other_params)
    if override is None:
        # Last fallback: the generic type-and-len= rule - see pointer_length_derivation.py's
        # derive_pointer_param_override().
        override = derive_pointer_param_override(command_name, param, other_params)
    if override is None:
        return False
    _validate_pointer_override(command_name, param, override, other_params)
    return True


# Commands that are structurally capturable - every param/return value would pass
# _is_param_capturable's/the return-value checks below on its own - but are deliberately kept
# disabled (routed through the generic UnsupportedCommand path, same as any other disabled
# command) because a generically-replayed Runner would be semantically wrong, not merely
# mechanically incomplete. This is a different kind of gap from everything else in this module:
# every other exclusion here says "this parameter's *shape* is not yet capturable"; this table
# says "this command's shape is perfectly capturable, but replaying it generically is actively
# incorrect against this backend's own replay model" - see each entry's own comment.
_FORCE_DISABLED_COMMANDS: frozenset[str] = frozenset({
    # glFlushMappedBufferRange(target, offset, length) is only valid GL usage when `target`
    # currently has an active mapping obtained with GL_MAP_FLUSH_EXPLICIT_BIT - but this
    # backend's buffer-map tracking (see OpenGL2/recorder/mapTrackingWrappers.cpp and
    # docs/development/opengl2-buffer-map-tracking-plan.md) never replays a real, ongoing
    # map/flush/unmap sequence at all: glMapBufferRange/glUnmapBuffer are bypassed by custom
    # recorder wrappers that record one synthetic, already-complete MappedDataMetaCommand
    # instead (mappedDataMetaRunner.cpp replays it as its own self-contained internal
    # map+memcpy+unmap against GL_COPY_WRITE_BUFFER, never leaving `target` mapped afterwards).
    # A generically-replayed glFlushMappedBufferRange would therefore always run against
    # replay-time buffer state that is never actually mapped, and fail with
    # GL_INVALID_OPERATION on every call. Properly
    # supporting it needs the custom map-tracking lifecycle extended to record real
    # per-command flushed sub-ranges (and, for a GL_MAP_FLUSH_EXPLICIT_BIT mapping, snapshot
    # only those flushed sub-ranges instead of the whole mapped range on unmap) - out of scope
    # here.
    'glFlushMappedBufferRange',

    # glImportSyncEXT(external_sync_type, external_sync, flags) -> GLsync (GL_EXT_EGL_sync)
    # creates a new sync object by importing an EXTERNAL fence - `external_sync` is a raw OS
    # resource identifier (e.g. an Android/EGL sync fence FD or a native HANDLE, per the
    # extension spec, not anything GITS owns or tracks) supplied directly by the app. Unlike
    # glFenceSync (see handle_lifecycle.py's GLSYNC_LIFECYCLE.creating_commands), the new GLsync this
    # command mints is not self-contained GPU-internal state: replaying the exact same
    # `external_sync` bit pattern captured at record time would hand the replay-time driver an
    # OS resource identifier from a completely different process (or long-closed/reused at
    # replay time even within the same process) - at best GL_INVALID_VALUE, at worst a
    # misdirected wait against whatever unrelated OS resource that identifier happens to
    # currently name. Correctly supporting this needs the same kind of dedicated, cross-process
    # external-fence-import tracking DirectX/Vulkan build for their own shared-fence import
    # commands, not the generic mapped-handle path - out of scope here; every other 4-of-5-
    # requested sync command (glIsSync/glClientWaitSync/glWaitSync/glDeleteSync take an
    # EXISTING GLsync, never mint one from external state, so none of them share this problem.
    'glImportSyncEXT',

    # wglReleaseVideoImageNV(hPbuffer, iVideoBuffer) releases what wglBindVideoImageNV bound,
    # but that bind stays unsupported (its HPVIDEODEV is no mapped handle), so the release
    # alone has nothing to undo on replay.
    'wglReleaseVideoImageNV',
})


_STRING_QUERY_RE = re.compile(r'^(?:glGetString|wglGetExtensionsString)')


def is_discarded_string_return(command_name: str, return_value: ReturnValue) -> bool:
    """Whether the return value is a driver-owned string from a pure query (glGetString,
    glGetStringi, wglGetExtensionsStringARB/EXT). The call is recorded and replayed, but the
    returned pointer is not captured: its address is meaningless in another process and nothing
    on replay reads its contents."""
    facts = return_value.type_facts
    return (_STRING_QUERY_RE.match(command_name) is not None
            and facts.pointer_depth == 1
            and facts.base_type in ('GLubyte', 'char')
            and return_value.type.startswith('const '))


def _is_fully_capturable_command(
    command_name: str, return_value: ReturnValue, params: list[Parameter], alias: str | None = None,
) -> bool:
    """Whether every param and the return value can be captured (see _is_param_capturable).

    Return-value pointers stay entirely out of scope this increment (see
    modern-opengl-backend-architecture.md's scale_pointer roadmap item) - a pointer/array return
    value (e.g. glGetString's null-terminated GLubyte*) still requires is_scalar/is_opaque_handle/
    is_mapped_handle exactly as before. This intentionally says nothing about whether the command
    is actually reachable yet - see _INTERCEPTED_APIS.

    Also unconditionally rejects any command named in _FORCE_DISABLED_COMMANDS above, regardless
    of how capturable its shape looks otherwise - see that table's own comment.

    `alias` is this command's gl.xml <alias name="..."/> target if it has one (e.g.
    glDeleteBuffersARB's alias is "glDeleteBuffers") - passed through to
    _is_param_capturable/_validate_object_name_overrides so a POINTER_PARAM_OVERRIDES entry
    keyed to the promoted core name also covers the ARB/EXT/OES alias that has no entry of its
    own. Return-value capturability itself needs no alias fallback: every currently-enabled
    return value is a plain scalar/opaque/mapped-handle shape that never consults
    POINTER_PARAM_OVERRIDES at all.
    """
    if command_name in _FORCE_DISABLED_COMMANDS:
        return False
    return_facts = return_value.type_facts
    if not (return_facts.is_scalar or return_facts.is_opaque_handle or return_facts.is_mapped_handle
            or is_discarded_string_return(command_name, return_value)):
        return False
    if _requires_object_name_override(command_name, return_value):
        return False
    _validate_object_name_overrides(command_name, return_value, params, alias)
    return all(_is_param_capturable(command_name, p, params, alias) for p in params)


# ObjectNameRole -> the PointerDirection an array-shaped field with that role must also carry
# in POINTER_PARAM_OVERRIDES, so the two independently-maintained tables can never silently
# disagree about a shared field's direction (see _validate_object_name_override).
_EXPECTED_POINTER_DIRECTION_FOR_ROLE: dict[ObjectNameRole, PointerDirection] = {
    ObjectNameRole.CREATE: PointerDirection.OUT,
    ObjectNameRole.DELETE: PointerDirection.IN,
}


def _validate_object_name_override(
    command_name: str, param_name: str, type_facts: TypeFacts, override: ObjectNameOverride,
    alias: str | None = None,
) -> None:
    """Raise ValueError if a resolved object-name override doesn't match this field's actual
    shape - mirrors _validate_pointer_override's precedent: a broken classification is a
    codegen bug, caught at generation time, never a reason to silently misclassify a field.
    Only called (see _validate_object_name_overrides below) for a field that is already known
    to actually reach generated code, so every branch here is a genuine invariant violation,
    not merely "this field happens to be currently uncapturable for an unrelated reason".
    """
    if type_facts.is_array_element_eligible:
        if override.role is ObjectNameRole.LOOKUP:
            raise ValueError(
                f"{command_name}.{param_name}: ObjectNameRole.LOOKUP is only defined for "
                f"scalar object-name fields today - no enabled command needs an "
                f"array-of-existing-names lookup yet"
            )
        pointer_override = _pointer_override_for_command_or_alias(command_name, param_name, alias)
        if pointer_override is None:
            raise ValueError(
                f"{command_name}.{param_name}: object-name array field with no matching "
                f"POINTER_PARAM_OVERRIDES entry (checked {command_name!r}"
                f"{f' and alias {alias!r}' if alias else ''})"
            )
        expected_direction = _EXPECTED_POINTER_DIRECTION_FOR_ROLE[override.role]
        if pointer_override.direction != expected_direction:
            raise ValueError(
                f"{command_name}.{param_name}: ObjectNameRole.{override.role} expects a "
                f"POINTER_PARAM_OVERRIDES direction of {expected_direction}, got "
                f"{pointer_override.direction}"
            )
    elif not type_facts.is_scalar:
        raise ValueError(
            f"{command_name}.{param_name}: object-name override on a field that is neither a "
            f"plain GLuint scalar nor a GLuint array ({type_facts})"
        )
    if param_name == 'return_value' and override.role is not ObjectNameRole.CREATE:
        raise ValueError(
            f"{command_name}.return_value: a return value can only be ObjectNameRole.CREATE - "
            f"a function's return value is never an existing-name lookup or a delete target"
        )


def _validate_object_name_overrides(
    command_name: str, return_value: ReturnValue, params: list[Parameter], alias: str | None = None,
) -> None:
    """Validate every resolved object-name override that actually matters for this command's
    generated code - see _validate_object_name_override.

    Restricted to a field that is either scalar (always capturable, always uses its override
    if one resolves) or array-eligible with a matching POINTER_PARAM_OVERRIDES entry already
    present, own or inherited from `alias` (see _pointer_override_for_command_or_alias) -
    meaning pointer_metadata.py has independently decided this array field is genuinely
    captured, so its object-name classification will actually reach generated code too). An
    array-eligible field with no POINTER_PARAM_OVERRIDES entry (own or inherited) stays
    uncapturable regardless of what resolve_object_name_override() says about its kind/role
    (see _is_param_capturable's own pointer-override check) - validating it here anyway would
    reject a perfectly fine (kind, role) pair (e.g. auto-derivation's LOOKUP-role `buffers` on
    glBindBuffersBase) purely because nobody has separately wired up a pointer_metadata.py
    entry for it yet, an entirely different, independent gap this function has no business
    raising about. This distinction did not exist before object-name auto-derivation: every one
    of the old hand-written table's ~80 entries happened to already satisfy it by construction
    (whoever added an entry made sure it was either scalar or paired with a matching pointer
    override), so this filter was previously unnecessary - auto-derivation produces
    classifications for hundreds of fields with no such per-entry curation, so the filter now
    has to be explicit.
    """
    for field in (return_value, *params):
        override = resolve_object_name_override(command_name, field.name, field.class_)
        if override is None:
            continue
        facts = field.type_facts
        reaches_codegen = (facts.is_scalar or
                            (facts.is_array_element_eligible and
                             _pointer_override_for_command_or_alias(
                                 command_name, field.name, alias) is not None))
        if reaches_codegen:
            _validate_object_name_override(command_name, field.name, facts, override, alias)


# APIs the Windows interceptor actually forwards today (see generator_interceptor.py's
# build_interceptor_command_list and modern-opengl-backend-architecture.md, "Only Windows GL
# and WGL are in the initial runtime scope. EGL and GLX remain represented in the
# code-generator intermediate representation so future platform support does not require
# redesigning..."). GLX (X11-only) and EGL (would need a separate libEGL.dll interceptor)
# have no trampoline, dispatch-table slot, or .def export yet, so marking one of their
# commands `enabled` would generate a typed Command/Serializer that nothing ever constructs.
# Revisit alongside that doc's "Revisit when: Runtime support is added for EGL or GLX".
_INTERCEPTED_APIS: frozenset[Api] = frozenset({Api.GL, Api.WGL})


def _inherit_handle_arb_classes(commands: list[Command]) -> None:
    """Gives each unclassified GLhandleARB field the class of its alias's field at the same
    position (both commands have the same shape - see _pointer_override_for_command_or_alias),
    or its HANDLE_ARB_CLASSES entry."""
    by_name = {command.name: command for command in commands}
    for command in commands:
        alias = by_name.get(command.alias) if command.alias else None
        fields = (command.return_value, *command.params)
        alias_fields = (alias.return_value, *alias.params) if alias else ()
        for index, field in enumerate(fields):
            if field.class_ is not None or field.type_facts.base_type != 'GLhandleARB':
                continue
            if len(alias_fields) == len(fields):
                field.class_ = alias_fields[index].class_
            if field.class_ is None:
                field.class_ = HANDLE_ARB_CLASSES.get((command.name, field.name))


def get_commands_from_xml_data(raw_commands: dict[Api, list[dict[str, Any]]]) -> dict[Api, list[Command]]:
    """Take raw command data from XML and return processed commands; both grouped by API (registry)."""
    result: dict[Api, list[Command]] = {api: [] for api in Api}

    for api, command_list in raw_commands.items():
        commands: list[Command] = []
        for raw_command in command_list:
            command_attribs: dict[str, Any] = raw_command['command_attribs']
            raw_return_value: dict[str, Any] = raw_command['return_value']

            params = [
                Parameter(
                    name=param['name'],
                    type=param['type'],
                    **normalize_attribs(param['attribs']),
                )
                for param in raw_command['params']
            ]
            return_value = ReturnValue(
                type=raw_return_value['type'],
                **normalize_attribs(raw_return_value['attribs']),
            )

            commands.append(Command(
                name=command_attribs['name'],
                api=api,
                enabled=False,
                command_type=CmdType.NONE,
                comment=command_attribs.get('comment'),
                alias=raw_command['alias_attribs'].get('name'),
                return_value=return_value,
                params=params,
            ))

        _inherit_handle_arb_classes(commands)
        for command in commands:
            command.enabled = (api in _INTERCEPTED_APIS and _is_fully_capturable_command(
                command.name, command.return_value, command.params, command.alias))

        result[api] = commands

    return result


# ---------------------------------------------------------------------------
# Top-level registry loading.
# ---------------------------------------------------------------------------

def load_registries_and_extract_xml_data(paths_and_apis: dict[Path, Api]) -> tuple[dict[Api, list[EnumValue]], dict[Api, list[Command]]]:
    """Load XML registry files, return (enums_by_api, commands_by_api)."""
    missing_paths = [path for path in paths_and_apis if not path.is_file()]
    if missing_paths:
        paths = '\n'.join(f'  - {path}' for path in missing_paths)
        raise FileNotFoundError(
            'Missing registry XML files:\n'
            f'{paths}\n'
            'Ensure OpenGL and EGL registries are downloaded in third_party.'
        )

    raw_enums_by_api: dict[Api, list[dict[str, Any]]] = {api: [] for api in Api}
    raw_commands_by_api: dict[Api, list[dict[str, Any]]] = {api: [] for api in Api}

    for path, api in paths_and_apis.items():
        registry = Registry()
        registry.loadFile(path)

        # Extract enums.
        raw_enums: list[dict[str, Any]]
        raw_enums = [dict(**e.attrib) for e in registry.enumdict.values()]
        raw_enums_by_api[api] = raw_enums

        # Check for unknown enum attributes.
        novel_enum_attribs: set[str] = find_unknown_enum_attributes(raw_enums)
        if novel_enum_attribs:
            print(f'WARNING: Unknown enum attributes in {path}: {novel_enum_attribs}')

        # Extract commands.
        raw_commands: list[dict[str, Any]] = extract_commands_from_registry(registry)
        raw_commands_by_api[api] = raw_commands

        # Check for unknown command attributes.
        novel_cmd_attribs: set[str] = find_unknown_command_attributes(raw_commands)
        if novel_cmd_attribs:
            print(f'WARNING: Unknown command attributes in {path}: {sorted(novel_cmd_attribs)}')

    # Convert raw enums to EnumValue objects.
    enums_by_api = get_enums_from_xml_data(raw_enums_by_api)

    # Convert raw commands to Command objects.
    commands_by_api = get_commands_from_xml_data(raw_commands_by_api)

    return enums_by_api, commands_by_api


# ---------------------------------------------------------------------------
# Enum canonicalization (postprocessing).
#
# GLenum values are reused ambiguously across many "groups" (the same numeric
# value means different things depending on which group/parameter it's used
# for). This section resolves, for every (api, value, group) combination, which
# enum name should be used when printing/logging a value - needed by the future
# interceptor's call logging.
# ---------------------------------------------------------------------------

# Helps pick best name to use when several enums match (api, value, group).
SUFFIX_RANK: dict[str, int] = {
    '':      0,
    'KHR':   1,
    'ARB':   2,
    'OES':   3,
    'EXT':   4,
    'ANGLE': 5,  'WEBGL': 5,
    'NV':    6,  'INTEL': 6,  'AMD':   6,
    'ATI':   7,  'NVX':   7,
    'APPLE': 8,  'IMG':   8,  'QCOM':  8,  'ANDROID': 8,
    'ARM':   8,  'OVR':   8,
    'MESA':  9,  'MESAX': 9,
    'SGI':  10,  'SGIS': 10,  'SGIX': 10,  'IBM':    10,
    'SUN':  10,  'SUNX': 10,  '3DFX': 10,  'HP':     10,
    'WIN':  10,  'PGI':  10,  'I3D':  10,  'TAO':    10,
    'NOK':  10,  'INGR': 10,  'OML':  10,  'REND':   10,
    'DMP':  10,  'VIV':  10,  'FJ':   10,
}

_known_suffixes = '|'.join(s for s in SUFFIX_RANK if s)
VENDOR_SUFFIX_REGEX = re.compile(rf'_({_known_suffixes})$')


def strip_suffix(name: str) -> str:
    """Remove the vendor suffix from an enum name string, if any."""
    return VENDOR_SUFFIX_REGEX.sub('', name)


def extract_suffix(name: str) -> str:
    """Return the vendor suffix of an enum name string, or '' if none."""
    match = VENDOR_SUFFIX_REGEX.search(name)
    return match.group(1) if match else ''


def get_suffix_rank(enum: EnumValue) -> int:
    """Return the suffix rank of a single enum."""
    return SUFFIX_RANK[extract_suffix(enum.name)]


def get_best_ranked_enum(group: list[EnumValue]) -> EnumValue:
    """Return the enum with lowest (best) suffix rank from a list of enums."""
    return min(group, key=get_suffix_rank)


def get_group_rank(group: list[EnumValue]) -> int:
    """Return the lowest (best) suffix rank among a list of enums."""
    return get_suffix_rank(get_best_ranked_enum(group))


SENTINEL_GROUP = ''  # Used for enums with no group annotation.

class EnumKey(NamedTuple):
    """Data to be mapped to a GLenum name string."""
    api:   Api  # which API this enum belongs to
    value: int  # converted from str to ignore formatting
    group: str  # group name, or SENTINEL_GROUP

EnumNameMap = dict[EnumKey, str]

# Manual overrides for (api, value, group) keys where automatic resolution
# would produce a tie between two no-suffix names with different generic names.
# All other genuine duplicates are resolved automatically by suffix ranking.
CANONICAL_NAME_OVERRIDES: EnumNameMap = {
    # GL_LOGIC_OP was split into GL_INDEX_LOGIC_OP for color-index buffers and
    # GL_COLOR_LOGIC_OP for RGBA buffers. Now a deprecated alias of the former.
    EnumKey(Api.GL, 0x0BF1, 'GetPName'):             'GL_INDEX_LOGIC_OP',
    # GL_TEXTURE_COMPONENTS was renamed when internal formats were formally defined.
    EnumKey(Api.GL, 0x1003, 'TextureParameterName'): 'GL_TEXTURE_INTERNAL_FORMAT',
    EnumKey(Api.GL, 0x1003, 'GetTextureParameter'):  'GL_TEXTURE_INTERNAL_FORMAT',
    # GL_BLEND_EQUATION was split into GL_BLEND_EQUATION_RGB and
    # GL_BLEND_EQUATION_ALPHA and is an alias of the former.
    EnumKey(Api.GL, 0x8009, 'GetPName'):             'GL_BLEND_EQUATION_RGB',
    # Canonical form in gl.xml, used in ARB_texture_env_combine, likely older.
    EnumKey(Api.GL, 0x8589, 'TextureEnvParameter'):  'GL_SOURCE1_ALPHA',
    # It seems GL_CLIP_PLANEn_IMG and GL_CLIP_DISTANCEn_APPLE should be marked
    # in gl.xml as aliases of the core GL_CLIP_PLANEn / GL_CLIP_DISTANCEn
    # enums but for some reason aren't. Prefer DISTANCE over PLANE since it's
    # the newer terminology despite PLANE being canonical in gl.xml.
    EnumKey(Api.GL, 0x0D32, SENTINEL_GROUP): 'GL_MAX_CLIP_DISTANCES',
    EnumKey(Api.GL, 0x3000, SENTINEL_GROUP): 'GL_CLIP_DISTANCE0',
    EnumKey(Api.GL, 0x3001, SENTINEL_GROUP): 'GL_CLIP_DISTANCE1',
    EnumKey(Api.GL, 0x3002, SENTINEL_GROUP): 'GL_CLIP_DISTANCE2',
    EnumKey(Api.GL, 0x3003, SENTINEL_GROUP): 'GL_CLIP_DISTANCE3',
    EnumKey(Api.GL, 0x3004, SENTINEL_GROUP): 'GL_CLIP_DISTANCE4',
    EnumKey(Api.GL, 0x3005, SENTINEL_GROUP): 'GL_CLIP_DISTANCE5',
    EnumKey(Api.GL, 0x3006, SENTINEL_GROUP): 'GL_CLIP_DISTANCE6',
    EnumKey(Api.GL, 0x3007, SENTINEL_GROUP): 'GL_CLIP_DISTANCE7',
    # GL_TEXTURE_3D_BINDING_OES and GL_TEXTURE_BINDING_3D_OES are unmarked
    # aliases. Pick the established GL_TEXTURE_BINDING_<target> pattern.
    EnumKey(Api.GL, 0x806A, SENTINEL_GROUP): 'GL_TEXTURE_BINDING_3D',
    # Accidental collisions with no true winner, acknowledged in gl.xml.
    # We choose non-debug enums over GL_DEBUG_*_MESA.
    EnumKey(Api.GL, 0x8759, SENTINEL_GROUP): 'GL_TEXTURE_1D_STACK_MESAX',
    EnumKey(Api.GL, 0x875A, SENTINEL_GROUP): 'GL_TEXTURE_2D_STACK_MESAX',
    EnumKey(Api.GL, 0x875B, SENTINEL_GROUP): 'GL_PROXY_TEXTURE_1D_STACK_MESAX',

    # WGL bit value 1 is used by few genuinely unrelated bitmasks.
    # WGL_SWAP_MAIN_PLANE and WGL_FONT_POLYGONS tie on rank. The former is a
    # core framebuffer concept, while the latter a flag for a niche function.
    EnumKey(Api.WGL, 0x0001, SENTINEL_GROUP): 'WGL_SWAP_MAIN_PLANE',

    # EGL bit value 1 is used by few unrelated bitmasks and the Boolean group.
    # EGL_TRUE, EGL_PBUFFER_BIT, EGL_OPENGL_ES_BIT tie on rank.
    # EGL_TRUE is the most often encountered of those three.
    EnumKey(Api.EGL, 0x0001, SENTINEL_GROUP): 'EGL_TRUE',
    # EGL_PIXMAP_BIT, EGL_OPENVG_BIT, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
    # tie on rank. The former is the oldest (EGL 1.0) surface type bit, while OpenVG 
    # is deprecated and compat profile is used only during context creation.
    EnumKey(Api.EGL, 0x0002, SENTINEL_GROUP): 'EGL_PIXMAP_BIT',
    # EGL_WINDOW_BIT and EGL_OPENGL_ES2_BIT tie on rank. The former is the
    # primary EGL surface type since EGL 1.0. We are unlikely to see ES2 apps.
    EnumKey(Api.EGL, 0x0004, SENTINEL_GROUP): 'EGL_WINDOW_BIT',
    # EGL_VG_ALPHA_FORMAT_PRE_BIT and EGL_OPENGL_ES3_BIT tie on rank.
    # OpenVG is deprecated, so choose the latter.
    EnumKey(Api.EGL, 0x0040, SENTINEL_GROUP): 'EGL_OPENGL_ES3_BIT',

    # These GLX values are shared across GLXAttribute (glXChooseVisual visual
    # attribs), GLXStringName, GLXErrorCode, and several bitmask groups.
    # GLXAttribute is the largest contiguous no-suffix sequence and covers the
    # most commonly encountered values in practice.
    EnumKey(Api.GLX, 0x0001, SENTINEL_GROUP): 'GLX_USE_GL',
    EnumKey(Api.GLX, 0x0002, SENTINEL_GROUP): 'GLX_BUFFER_SIZE',
    EnumKey(Api.GLX, 0x0003, SENTINEL_GROUP): 'GLX_LEVEL',
    EnumKey(Api.GLX, 0x0004, SENTINEL_GROUP): 'GLX_RGBA',
    EnumKey(Api.GLX, 0x0005, SENTINEL_GROUP): 'GLX_DOUBLEBUFFER',
    EnumKey(Api.GLX, 0x0006, SENTINEL_GROUP): 'GLX_STEREO',
    EnumKey(Api.GLX, 0x0007, SENTINEL_GROUP): 'GLX_AUX_BUFFERS',
    EnumKey(Api.GLX, 0x0008, SENTINEL_GROUP): 'GLX_RED_SIZE',
    EnumKey(Api.GLX, 0x0010, SENTINEL_GROUP): 'GLX_ACCUM_BLUE_SIZE',
    # The other 0x0011 (17) candidate __GLX_NUMBER_EVENTS is not a real enum.
    EnumKey(Api.GLX, 0x0011, SENTINEL_GROUP): 'GLX_ACCUM_ALPHA_SIZE',
    EnumKey(Api.GLX, 0x0020, SENTINEL_GROUP): 'GLX_CONFIG_CAVEAT',
}


def is_real_glenum(enum: EnumValue) -> bool:
    """Return True if the enum is a proper GLenum, False otherwise."""
    # Some enums aren't real GLenums, but abuses of the enum mechanism. They
    # are mostly grouped in the 'SpecialNumbers' group, which is only annotated
    # directly on <enum> tags in gl.xml. Other registries only ever annotate
    # group="SpecialNumbers" on <enums> tags, but reg.py provided by Khronos
    # doesn't provide any data from <enums> XML elements. This is why we use
    # this heuristic.
    # We allow enums marked with more groups than just 'SpecialNumbers', as
    # commands can be annotated with these other groups.

    # Known non-GLenum values.
    if enum.groups == ('SpecialNumbers',):
        return False  # Works for GL. For WGL/EGL/GLX, we need below heuristics.
    elif enum.value.startswith('EGL_CAST'):
        return False  # They are explicitly cast to a type different from GLenum.
    elif enum.type != EnumType.INT:
        return False  # u/ull type annotations only appear on SpecialNumbers enums.
    elif enum.value == '"GLX"':
        return False  # GLX registry XML admits this is abuse of the enum mechanism.

    return True


def enums_by_key(enums_by_api: dict[Api, list[EnumValue]]) -> dict[EnumKey, list[EnumValue]]:
    """Return mapping from EnumKey (api, value, group) to enums matching it.

    Enums that aren't proper GLenums are skipped. Enums with no group use
    SENTINEL_GROUP as their group.
    """
    result: dict[EnumKey, list[EnumValue]] = defaultdict(list)
    for api, enums in enums_by_api.items():
        for enum in enums:
            if not is_real_glenum(enum):
                continue

            try:
                norm = int(enum.value, 0) # 0 means auto-detect base.
            except (ValueError, TypeError) as error:
                raise ValueError(
                    f"Unexpected non-integer enum value {enum.name} = {enum.value}"
                ) from error

            if not enum.groups:
                result[EnumKey(api, norm, SENTINEL_GROUP)].append(enum)
            else:
                for group in enum.groups:
                    result[EnumKey(api, norm, group)].append(enum)
    return result


def build_enum_name_map(enums_by_api: dict[Api, list[EnumValue]]) -> EnumNameMap:
    """Return a mapping from (api, normalized_value, group) to a canonical name."""
    result: EnumNameMap = {}

    for key, enums_matching_key in enums_by_key(enums_by_api).items():
        if key.group == 'SpecialNumbers':
            # It's only used for grouping in the XML, no command actually uses it.
            continue

        if key in CANONICAL_NAME_OVERRIDES:
            result[key] = CANONICAL_NAME_OVERRIDES[key]
            continue

        # Possible alias/target group combinations:
        # 1. Alias group == target group: choose target, drop alias.
        # 2. Target has a group, alias doesn't: use target for group resolution.
        # 3. Alias has a group, target doesn't: keep alias for group resolution.
        # 4. Alias group != target group: keep alias for group resolution.
        # Cases 1 & 2 are common and expected. Cases 2, 3, 4 resolve target and
        # alias separately (different keys). In case 2, alias will most likely
        # be dropped in sentinel group resolution. Cases 3 & 4 are unusual:
        # alias isn't a simple synonym but has a different context in which
        # it's preferred. Examples: case 3: GL_PROGRAM_POINT_SIZE, case 4:
        # GL_SHADING_RATE_*_PIXELS_QCOM vs their EXT aliases.
        non_aliases: list[EnumValue] = [e for e in enums_matching_key if e.alias is None]
        candidates: list[EnumValue] = non_aliases if non_aliases else enums_matching_key

        if len(candidates) == 1:
            result[key] = candidates[0].name
            continue

        # Group candidates whose names differ only by vendor suffix.
        generic_name_groups: dict[str, list[EnumValue]] = defaultdict(list)
        for e in candidates:
            generic_name_groups[strip_suffix(e.name)].append(e)

        # Resolve name conflicts using vendor suffixes.
        if len(generic_name_groups) == 1:
            # One generic name with multiple suffixes; pick the best-ranked member.
            result[key] = get_best_ranked_enum(candidates).name
        else:
            # Multiple generic names; rank each group by their best member.
            ranked_groups = sorted(generic_name_groups.values(), key=get_group_rank)
            if get_group_rank(ranked_groups[0]) == get_group_rank(ranked_groups[1]):
                names = [e.name for g in ranked_groups for e in g]
                print(f'WARNING: unresolvable tie for {key}: {names}. '
                      f'Add an entry to CANONICAL_NAME_OVERRIDES.')
            else:
                winner_group = ranked_groups[0]
                result[key] = get_best_ranked_enum(winner_group).name

    return result


def get_bitmask_groups(enums_by_api: dict[Api, list[EnumValue]]) -> frozenset[str]:
    """Return the names of all groups that contain bitflag pseudo-enums."""
    # We don't simply get the group names from <enums type="bitmask"> tags because the
    # group names annotated on <enums> can differ from those on individual <enum> tags.
    # E.g., <enums group="PathRenderingTokenNV"> contains <enum group="PathCoordType">
    # tags. Instead, we exploit naming conventions: pseudo-enum bitflag constants use
    # the _BIT suffix. We exclude the _BITS suffix from our heuristic as it appears
    # not only in all-bits-set mask constants like GL_ALL_ATTRIB_BITS, but also in
    # scalar enums like GL_SUBPIXEL_BITS. Every bitmask group containing a _BITS mask
    # also contains _BIT flags, so excluding _BITS doesn't yield false negatives.
    bitmask_groups: set[str] = set()
    for enum_list in enums_by_api.values():
        for enum in enum_list:
            if '_BIT' in enum.name and '_BITS' not in enum.name:
                bitmask_groups.update(enum.groups)
    return frozenset(bitmask_groups)
