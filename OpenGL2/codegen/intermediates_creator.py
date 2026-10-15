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

from intermediates import Api, CmdType, Command, EnumType, EnumValue, Parameter, ReturnValue
from registry import Registry

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

    # Known non-GLenum values.
    if 'SpecialNumbers' in enum.groups:
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
