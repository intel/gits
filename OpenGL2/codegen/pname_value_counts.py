#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Element counts for IN-direction COMPSIZE(pname) setters (glLightfv, glLightModelfv, glFogfv,
# glTexEnvfv, glTexGenfv, ...), derived as LengthSourceKind.PNAME_VALUE_COUNT.
#
# gl.xml gives group membership for `pname` but no per-pname count. A setter is derived only when
# its `pname` sibling's group is in PNAME_GROUPS, meaning every member of that group was audited:
# members named in MULTI_VALUE_PNAMES take that many elements, all others take exactly one.
# validate_pname_groups() fails generation when a group's gl.xml membership changes, so a registry
# update forces a re-audit instead of silently sizing a new multi-value pname as one element.
#
# The generated runtime lookup (pnameValueCountsAuto.cpp) switches on the value, not the group,
# because gl.xml group tags are incomplete: some legal pnames (GL_FOG_COORD_SRC for glFogfv) carry
# no group tag at all. None of the values below means something else in another audited group.
#
# Families with their own derivation keep it: glTexParameter*v/glSamplerParameter*v (swizzle
# legality depends on the family, see tex_sampler_parameter_metadata.py), and the explicit
# glMaterialfv/glPointParameterfv/glPatchParameterfv table entries.

from intermediates import Api, EnumValue, Parameter
from pointer_metadata import LengthSourceKind, PointerDirection, PointerParamOverride

# gl.xml `group` -> the member enum names it had when audited.
PNAME_GROUPS: dict[str, frozenset[str]] = {
    'LightParameter': frozenset({
        'GL_AMBIENT', 'GL_CONSTANT_ATTENUATION', 'GL_DIFFUSE', 'GL_LINEAR_ATTENUATION',
        'GL_POSITION', 'GL_QUADRATIC_ATTENUATION', 'GL_SPECULAR', 'GL_SPOT_CUTOFF',
        'GL_SPOT_DIRECTION', 'GL_SPOT_EXPONENT',
    }),
    'LightModelParameter': frozenset({
        'GL_LIGHT_MODEL_AMBIENT', 'GL_LIGHT_MODEL_COLOR_CONTROL',
        'GL_LIGHT_MODEL_COLOR_CONTROL_EXT', 'GL_LIGHT_MODEL_LOCAL_VIEWER',
        'GL_LIGHT_MODEL_TWO_SIDE',
    }),
    'FogParameter': frozenset({
        'GL_FOG_COLOR', 'GL_FOG_DENSITY', 'GL_FOG_END', 'GL_FOG_INDEX', 'GL_FOG_MODE',
        'GL_FOG_OFFSET_VALUE_SGIX', 'GL_FOG_START',
    }),
    'FogPName': frozenset({
        'GL_FOG_COORD_SRC', 'GL_FOG_DENSITY', 'GL_FOG_END', 'GL_FOG_INDEX', 'GL_FOG_MODE',
        'GL_FOG_START',
    }),
    'TextureEnvParameter': frozenset({
        'GL_ADD_SIGNED', 'GL_ADD_SIGNED_ARB', 'GL_ADD_SIGNED_EXT', 'GL_ALPHA_SCALE', 'GL_COMBINE',
        'GL_COMBINE_ALPHA', 'GL_COMBINE_ALPHA_ARB', 'GL_COMBINE_ALPHA_EXT', 'GL_COMBINE_ARB',
        'GL_COMBINE_EXT', 'GL_COMBINE_RGB', 'GL_COMBINE_RGB_ARB', 'GL_COMBINE_RGB_EXT',
        'GL_CONSTANT', 'GL_CONSTANT_ARB', 'GL_CONSTANT_EXT', 'GL_CONSTANT_NV', 'GL_COORD_REPLACE',
        'GL_INTERPOLATE', 'GL_INTERPOLATE_ARB', 'GL_INTERPOLATE_EXT', 'GL_OPERAND0_ALPHA',
        'GL_OPERAND0_ALPHA_ARB', 'GL_OPERAND0_ALPHA_EXT', 'GL_OPERAND0_RGB', 'GL_OPERAND0_RGB_ARB',
        'GL_OPERAND0_RGB_EXT', 'GL_OPERAND1_ALPHA', 'GL_OPERAND1_ALPHA_ARB',
        'GL_OPERAND1_ALPHA_EXT', 'GL_OPERAND1_RGB', 'GL_OPERAND1_RGB_ARB', 'GL_OPERAND1_RGB_EXT',
        'GL_OPERAND2_ALPHA', 'GL_OPERAND2_ALPHA_ARB', 'GL_OPERAND2_ALPHA_EXT', 'GL_OPERAND2_RGB',
        'GL_OPERAND2_RGB_ARB', 'GL_OPERAND2_RGB_EXT', 'GL_OPERAND3_ALPHA_NV', 'GL_OPERAND3_RGB_NV',
        'GL_PREVIOUS', 'GL_PREVIOUS_ARB', 'GL_PREVIOUS_EXT', 'GL_PRIMARY_COLOR',
        'GL_PRIMARY_COLOR_ARB', 'GL_PRIMARY_COLOR_EXT', 'GL_RGB_SCALE', 'GL_RGB_SCALE_ARB',
        'GL_RGB_SCALE_EXT', 'GL_SOURCE0_ALPHA', 'GL_SOURCE0_ALPHA_ARB', 'GL_SOURCE0_ALPHA_EXT',
        'GL_SOURCE0_RGB', 'GL_SOURCE0_RGB_ARB', 'GL_SOURCE0_RGB_EXT', 'GL_SOURCE1_ALPHA',
        'GL_SOURCE1_ALPHA_ARB', 'GL_SOURCE1_ALPHA_EXT', 'GL_SOURCE1_RGB', 'GL_SOURCE1_RGB_ARB',
        'GL_SOURCE1_RGB_EXT', 'GL_SOURCE2_ALPHA', 'GL_SOURCE2_ALPHA_ARB', 'GL_SOURCE2_ALPHA_EXT',
        'GL_SOURCE2_RGB', 'GL_SOURCE2_RGB_ARB', 'GL_SOURCE2_RGB_EXT', 'GL_SOURCE3_ALPHA_NV',
        'GL_SOURCE3_RGB_NV', 'GL_SRC0_ALPHA', 'GL_SRC0_RGB', 'GL_SRC1_ALPHA', 'GL_SRC1_ALPHA_EXT',
        'GL_SRC1_RGB', 'GL_SRC2_ALPHA', 'GL_SRC2_RGB', 'GL_TEXTURE_ENV_COLOR',
        'GL_TEXTURE_ENV_MODE', 'GL_TEXTURE_LOD_BIAS',
    }),
    # GL_EYE_POINT_SGIS/GL_OBJECT_POINT_SGIS/GL_EYE_LINE_SGIS/GL_OBJECT_LINE_SGIS stay at one
    # element: no desktop driver exposes SGIS_point_line_texgen, so the driver rejects the pname
    # and one element never over-reads application memory.
    'TextureGenParameter': frozenset({
        'GL_EYE_LINE_SGIS', 'GL_EYE_PLANE', 'GL_EYE_POINT_SGIS', 'GL_OBJECT_LINE_SGIS',
        'GL_OBJECT_PLANE', 'GL_OBJECT_POINT_SGIS', 'GL_TEXTURE_GEN_MODE', 'GL_TEXTURE_GEN_MODE_OES',
    }),
    'MaterialParameter': frozenset({
        'GL_AMBIENT', 'GL_AMBIENT_AND_DIFFUSE', 'GL_COLOR_INDEXES', 'GL_DIFFUSE', 'GL_EMISSION',
        'GL_SHININESS', 'GL_SPECULAR',
    }),
    'PointParameterNameARB': frozenset({
        'GL_DISTANCE_ATTENUATION_EXT', 'GL_DISTANCE_ATTENUATION_SGIS',
        'GL_POINT_DISTANCE_ATTENUATION', 'GL_POINT_DISTANCE_ATTENUATION_ARB',
        'GL_POINT_FADE_THRESHOLD_SIZE', 'GL_POINT_FADE_THRESHOLD_SIZE_ARB',
        'GL_POINT_FADE_THRESHOLD_SIZE_EXT', 'GL_POINT_FADE_THRESHOLD_SIZE_SGIS',
        'GL_POINT_SIZE_MAX', 'GL_POINT_SIZE_MAX_ARB', 'GL_POINT_SIZE_MAX_EXT',
        'GL_POINT_SIZE_MAX_SGIS', 'GL_POINT_SIZE_MIN', 'GL_POINT_SIZE_MIN_ARB',
        'GL_POINT_SIZE_MIN_EXT', 'GL_POINT_SIZE_MIN_SGIS',
    }),
    'ColorTableParameterPName': frozenset({
        'GL_COLOR_TABLE_ALPHA_SIZE', 'GL_COLOR_TABLE_ALPHA_SIZE_SGI', 'GL_COLOR_TABLE_BIAS',
        'GL_COLOR_TABLE_BIAS_SGI', 'GL_COLOR_TABLE_BLUE_SIZE', 'GL_COLOR_TABLE_BLUE_SIZE_SGI',
        'GL_COLOR_TABLE_FORMAT', 'GL_COLOR_TABLE_FORMAT_SGI', 'GL_COLOR_TABLE_GREEN_SIZE',
        'GL_COLOR_TABLE_GREEN_SIZE_SGI', 'GL_COLOR_TABLE_INTENSITY_SIZE',
        'GL_COLOR_TABLE_INTENSITY_SIZE_SGI', 'GL_COLOR_TABLE_LUMINANCE_SIZE',
        'GL_COLOR_TABLE_LUMINANCE_SIZE_SGI', 'GL_COLOR_TABLE_RED_SIZE',
        'GL_COLOR_TABLE_RED_SIZE_SGI', 'GL_COLOR_TABLE_SCALE', 'GL_COLOR_TABLE_SCALE_SGI',
        'GL_COLOR_TABLE_WIDTH', 'GL_COLOR_TABLE_WIDTH_SGI',
    }),
    'ConvolutionParameter': frozenset({
        'GL_CONVOLUTION_BORDER_COLOR', 'GL_CONVOLUTION_BORDER_MODE',
        'GL_CONVOLUTION_BORDER_MODE_EXT', 'GL_CONVOLUTION_FILTER_BIAS',
        'GL_CONVOLUTION_FILTER_BIAS_EXT', 'GL_CONVOLUTION_FILTER_SCALE',
        'GL_CONVOLUTION_FILTER_SCALE_EXT', 'GL_CONVOLUTION_FORMAT', 'GL_CONVOLUTION_FORMAT_EXT',
        'GL_CONVOLUTION_HEIGHT', 'GL_CONVOLUTION_HEIGHT_EXT', 'GL_CONVOLUTION_WIDTH',
        'GL_CONVOLUTION_WIDTH_EXT', 'GL_MAX_CONVOLUTION_HEIGHT', 'GL_MAX_CONVOLUTION_HEIGHT_EXT',
        'GL_MAX_CONVOLUTION_WIDTH', 'GL_MAX_CONVOLUTION_WIDTH_EXT',
    }),
}

# Every audited pname taking more than one element. Aliases with the same value need no entry.
MULTI_VALUE_PNAMES: dict[str, int] = {
    'GL_AMBIENT': 4,
    'GL_DIFFUSE': 4,
    'GL_SPECULAR': 4,
    'GL_POSITION': 4,
    'GL_SPOT_DIRECTION': 3,
    'GL_EMISSION': 4,
    'GL_AMBIENT_AND_DIFFUSE': 4,
    'GL_COLOR_INDEXES': 3,
    'GL_LIGHT_MODEL_AMBIENT': 4,
    'GL_FOG_COLOR': 4,
    'GL_TEXTURE_ENV_COLOR': 4,
    'GL_EYE_PLANE': 4,
    'GL_OBJECT_PLANE': 4,
    'GL_POINT_DISTANCE_ATTENUATION': 3,
    'GL_COLOR_TABLE_SCALE': 4,
    'GL_COLOR_TABLE_BIAS': 4,
    'GL_CONVOLUTION_BORDER_COLOR': 4,
    'GL_CONVOLUTION_FILTER_SCALE': 4,
    'GL_CONVOLUTION_FILTER_BIAS': 4,
}


def derive_pname_value_count_override(
    command_name: str, param: Parameter, other_params: list[Parameter],  # noqa: ARG001
) -> PointerParamOverride | None:
    """PNAME_VALUE_COUNT override for an IN-direction COMPSIZE(pname) setter whose `pname`
    group is audited in PNAME_GROUPS, or None."""
    if param.len != 'COMPSIZE(pname)':
        return None
    facts = param.type_facts
    if facts.pointer_depth != 1 or facts.base_type == 'void' or not facts.is_array_element_eligible:
        return None
    if not param.type.startswith('const '):
        return None
    pname = next((p for p in other_params if p.name == 'pname'), None)
    if pname is None or not pname.type_facts.is_scalar or pname.group not in PNAME_GROUPS:
        return None
    return PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.PNAME_VALUE_COUNT,
        pname_param='pname',
    )


def validate_pname_groups(enums_by_api: dict[Api, list[EnumValue]]) -> None:
    """Raise if gl.xml no longer matches the audit behind PNAME_GROUPS/MULTI_VALUE_PNAMES."""
    gl_enums = enums_by_api[Api.GL]
    for group, expected in PNAME_GROUPS.items():
        actual = {e.name for e in gl_enums if group in e.groups}
        if actual != expected:
            raise ValueError(
                f"gl.xml group {group!r} changed since audit (added {sorted(actual - expected)}, "
                f"removed {sorted(expected - actual)}) - re-audit its multi-value pnames in "
                f"pname_value_counts.py and update PNAME_GROUPS"
            )
    names = {e.name for e in gl_enums}
    missing = sorted(set(MULTI_VALUE_PNAMES) - names)
    if missing:
        raise ValueError(f"MULTI_VALUE_PNAMES names enums missing from gl.xml: {missing}")


def multi_value_pname_cases(enums_by_api: dict[Api, list[EnumValue]]) -> list[tuple[str, int, list[str]]]:
    """(value, count, enum names) for every multi-value pname, sorted by value - the switch
    cases of the generated ComputePnameValueCount()."""
    by_value: dict[int, tuple[str, int, list[str]]] = {}
    for e in enums_by_api[Api.GL]:
        count = MULTI_VALUE_PNAMES.get(e.name)
        if count is None:
            continue
        key = int(e.value, 0)
        value, existing_count, enum_names = by_value.setdefault(key, (e.value, count, []))
        if existing_count != count:
            raise ValueError(f"MULTI_VALUE_PNAMES gives value {value} two different counts")
        enum_names.append(e.name)
    return [by_value[key] for key in sorted(by_value)]
