# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import unittest
from dataclasses import replace
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_helpers import pointer_needs_raw_offset_capture
from intermediates import Api, Parameter
from intermediates_creator import (
    FILE_TO_API,
    _is_param_capturable,  # pyright: ignore[reportPrivateUsage]
    _validate_pointer_override,  # pyright: ignore[reportPrivateUsage]
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from pointer_metadata import (
    POINTER_PARAM_OVERRIDES,
    LengthSourceKind,
    PointerDirection,
    PointerParamOverride,
    TexOrSamplerParameterFamily,
)
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


class ValidatePointerOverrideTests(unittest.TestCase):
    def test_sibling_param_override_requires_existing_sibling(self) -> None:
        param = Parameter(name='textures', type='GLuint *')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGenTextures', param, override, other_params=[])

    def test_sibling_param_override_requires_scalar_sibling(self) -> None:
        param = Parameter(name='textures', type='GLuint *')
        sibling = Parameter(name='n', type='GLuint *')  # not scalar: itself a pointer
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGenTextures', param, override, other_params=[sibling])

    def test_sibling_param_override_accepts_valid_sibling(self) -> None:
        param = Parameter(name='textures', type='GLuint *')
        sibling = Parameter(name='n', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n')

        # Should not raise.
        _validate_pointer_override('glGenTextures', param, override, other_params=[sibling])

    def test_fixed_count_override_requires_nonnegative_integer_literal(self) -> None:
        param = Parameter(name='params', type='GLint *')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='not_a_number')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGetShaderiv', param, override, other_params=[])

    def test_fixed_count_override_accepts_integer_literal(self) -> None:
        param = Parameter(name='params', type='GLint *')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1')

        # Should not raise.
        _validate_pointer_override('glGetShaderiv', param, override, other_params=[])

    def test_null_terminated_override_needs_no_structural_validation(self) -> None:
        param = Parameter(name='name', type='const GLchar *')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED)

        # Should not raise, regardless of other_params.
        _validate_pointer_override('glGetUniformLocation', param, override, other_params=[])

    def test_key_value_list_override_needs_no_structural_validation(self) -> None:
        param = Parameter(name='attribList', type='const int *')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.KEY_VALUE_LIST)

        # Should not raise, regardless of other_params.
        _validate_pointer_override(
            'wglCreateContextAttribsARB', param, override, other_params=[])

    def test_opaque_bit_pattern_override_must_be_in_direction(self) -> None:
        param = Parameter(name='pointer', type='const void *')
        override = PointerParamOverride(
            direction=PointerDirection.OUT, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glVertexAttribPointer', param, override, other_params=[])

    def test_opaque_bit_pattern_override_must_not_set_length_source_value(self) -> None:
        param = Parameter(name='indices', type='const void *')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN,
            length_source_value='count')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glDrawElements', param, override, other_params=[])

    def test_opaque_bit_pattern_override_accepts_valid_shape(self) -> None:
        param = Parameter(name='indices', type='const void *')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN)

        # Should not raise.
        _validate_pointer_override('glDrawElements', param, override, other_params=[])

    def test_opaque_bit_pattern_array_override_must_be_in_direction(self) -> None:
        # Mirrors test_opaque_bit_pattern_override_must_be_in_direction above - see
        # _validate_pointer_override's own OPAQUE_BIT_PATTERN_ARRAY direction check and that
        # LengthSourceKind's own comment (pointer_metadata.py) for why an OUT-direction "array
        # of opaque values the driver produces" shape does not exist.
        param = Parameter(name='indices', type='const void * const *')
        sibling = Parameter(name='drawcount', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY,
            length_source_value='drawcount')

        with self.assertRaises(ValueError):
            _validate_pointer_override(
                'glMultiDrawElementsBaseVertex', param, override, other_params=[sibling])

    def test_opaque_bit_pattern_array_override_requires_existing_sibling(self) -> None:
        param = Parameter(name='indices', type='const void * const *')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY,
            length_source_value='drawcount')

        with self.assertRaises(ValueError):
            _validate_pointer_override(
                'glMultiDrawElementsBaseVertex', param, override, other_params=[])

    def test_opaque_bit_pattern_array_override_requires_scalar_sibling(self) -> None:
        param = Parameter(name='indices', type='const void * const *')
        sibling = Parameter(name='drawcount', type='GLsizei *')  # not scalar: itself a pointer
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY,
            length_source_value='drawcount')

        with self.assertRaises(ValueError):
            _validate_pointer_override(
                'glMultiDrawElementsBaseVertex', param, override, other_params=[sibling])

    def test_opaque_bit_pattern_array_override_accepts_valid_sibling(self) -> None:
        param = Parameter(name='indices', type='const void * const *')
        sibling = Parameter(name='drawcount', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY,
            length_source_value='drawcount')

        # Should not raise.
        _validate_pointer_override(
            'glMultiDrawElementsBaseVertex', param, override, other_params=[sibling])

    def test_length_multiplier_must_be_at_least_one(self) -> None:
        param = Parameter(name='value', type='const GLfloat *')
        sibling = Parameter(name='count', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value='count', length_multiplier=0)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glUniform4fv', param, override, other_params=[sibling])

    def test_length_multiplier_defaults_to_one_and_is_accepted(self) -> None:
        param = Parameter(name='value', type='const GLfloat *')
        sibling = Parameter(name='count', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value='count')

        # Should not raise.
        _validate_pointer_override('glUniform1fv', param, override, other_params=[sibling])

    def test_vertex_attrib_parameter_value_count_accepts_out_direction(self) -> None:
        # glGetVertexAttribiv's `params` is written by the driver, so this kind must be
        # OUT-direction, unlike TEX_SAMPLER_PARAMETER_VALUE_COUNT/PATCH_PARAMETER_VALUE_COUNT
        # above (both IN-only setters).
        param = Parameter(name='params', type='GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        # Should not raise.
        _validate_pointer_override('glGetVertexAttribiv', param, override, other_params=[pname])

    def test_vertex_attrib_parameter_value_count_rejects_in_direction(self) -> None:
        param = Parameter(name='params', type='GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGetVertexAttribiv', param, override, other_params=[pname])

    def test_vertex_attrib_parameter_value_count_requires_pname_param(self) -> None:
        param = Parameter(name='params', type='GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGetVertexAttribiv', param, override, other_params=[pname])

    def test_vertex_attrib_parameter_value_count_rejects_tex_sampler_family(self) -> None:
        # glGetVertexAttribiv has no family discriminator at all (only one command in its own
        # shape - see LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT's own comment).
        param = Parameter(name='params', type='GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT,
            pname_param='pname', tex_sampler_family=TexOrSamplerParameterFamily.TEX_PARAMETER)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGetVertexAttribiv', param, override, other_params=[pname])

    def test_bitmap_byte_size_requires_unpack_pbo_guard_and_paired_dimensions(self) -> None:
        param = Parameter(name='bitmap', type='const GLubyte *')
        dims = [Parameter(name='width', type='GLsizei'), Parameter(name='height', type='GLsizei')]
        for override in (
            PointerParamOverride(
                direction=PointerDirection.IN, length_source_kind=LengthSourceKind.BITMAP_BYTE_SIZE,
                pixel_width_param='width', pixel_height_param='height'),
            PointerParamOverride(
                direction=PointerDirection.IN, length_source_kind=LengthSourceKind.BITMAP_BYTE_SIZE,
                pixel_width_param='width', unpack_pbo_guard=True),
        ):
            with self.subTest(override=override), self.assertRaises(ValueError):
                _validate_pointer_override('glBitmap', param, override, other_params=dims)

        _validate_pointer_override('glBitmap', param, PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.BITMAP_BYTE_SIZE,
            pixel_width_param='width', pixel_height_param='height', unpack_pbo_guard=True),
            other_params=dims)

    def test_unpack_pbo_guard_requires_in_direction(self) -> None:
        # GL_PIXEL_UNPACK_BUFFER only ever redirects an IN-direction parameter - an
        # OUT-direction user would leave the flag silently inert (nothing checks it there).
        param = Parameter(name='data', type='const void *')
        image_size = Parameter(name='imageSize', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.OUT, length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value='imageSize', unpack_pbo_guard=True)

        with self.assertRaises(ValueError):
            _validate_pointer_override(
                'glCompressedTexImage2D', param, override, other_params=[image_size])

    def test_unpack_pbo_guard_accepts_in_direction_sibling_param(self) -> None:
        param = Parameter(name='data', type='const void *')
        image_size = Parameter(name='imageSize', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value='imageSize', unpack_pbo_guard=True)

        # Should not raise.
        _validate_pointer_override(
            'glCompressedTexImage2D', param, override, other_params=[image_size])

    def test_unpack_pbo_guard_rejects_pixel_transfer_size_combination(self) -> None:
        # Redundant with (and potentially confusable against) PIXEL_TRANSFER_SIZE's own
        # unconditional guard - see PointerParamOverride.unpack_pbo_guard's own comment.
        param = Parameter(name='pixels', type='const void *')
        fmt = Parameter(name='format', type='GLenum', group='PixelFormat')
        typ = Parameter(name='type', type='GLenum', group='PixelType')
        width = Parameter(name='width', type='GLsizei')
        height = Parameter(name='height', type='GLsizei')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.PIXEL_TRANSFER_SIZE,
            pixel_format_param='format', pixel_type_param='type', pixel_width_param='width',
            pixel_height_param='height', unpack_pbo_guard=True)

        with self.assertRaises(ValueError):
            _validate_pointer_override(
                'glTexImage2D', param, override, other_params=[fmt, typ, width, height])

    def test_query_buffer_guard_requires_out_direction(self) -> None:
        # GL_QUERY_BUFFER only ever redirects an OUT-direction (driver-written) result - an
        # IN-direction user would leave the flag silently inert.
        param = Parameter(name='params', type='GLint64 *')
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.FIXED_COUNT,
            length_source_value='1', query_buffer_guard=True)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glGetQueryObjecti64v', param, override, other_params=[])

    def test_query_buffer_guard_accepts_out_direction_fixed_count(self) -> None:
        param = Parameter(name='params', type='GLint64 *')
        override = PointerParamOverride(
            direction=PointerDirection.OUT, length_source_kind=LengthSourceKind.FIXED_COUNT,
            length_source_value='1', query_buffer_guard=True)

        # Should not raise.
        _validate_pointer_override('glGetQueryObjecti64v', param, override, other_params=[])

    def test_tex_sampler_parameter_value_count_requires_tex_sampler_family(self) -> None:
        # A TEX_SAMPLER_PARAMETER_VALUE_COUNT override that omits tex_sampler_family would
        # leave ComputeTexOrSamplerParameterValueCount() with no way
        # to know whether GL_TEXTURE_SWIZZLE_RGBA is legal for this call's own command family -
        # exactly the ambiguity that risked an out-of-bounds read before this field existed.
        # Fail loud at generation time rather than silently falling through.
        param = Parameter(name='params', type='const GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glTexParameterIiv', param, override, other_params=[pname])

    def test_tex_sampler_parameter_value_count_accepts_tex_parameter_family(self) -> None:
        param = Parameter(name='params', type='const GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT,
            pname_param='pname', tex_sampler_family=TexOrSamplerParameterFamily.TEX_PARAMETER)

        # Should not raise.
        _validate_pointer_override('glTexParameterIiv', param, override, other_params=[pname])

    def test_tex_sampler_parameter_value_count_accepts_sampler_parameter_family(self) -> None:
        param = Parameter(name='param', type='const GLint *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT,
            pname_param='pname',
            tex_sampler_family=TexOrSamplerParameterFamily.SAMPLER_PARAMETER)

        # Should not raise.
        _validate_pointer_override('glSamplerParameterIiv', param, override, other_params=[pname])

    def test_patch_parameter_value_count_rejects_tex_sampler_family(self) -> None:
        # glPatchParameterfv has no sibling family at all (see
        # tex_sampler_parameter_metadata.py's own header comment) - a PATCH_PARAMETER_VALUE_COUNT
        # override must never set tex_sampler_family.
        param = Parameter(name='values', type='const GLfloat *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT,
            pname_param='pname', tex_sampler_family=TexOrSamplerParameterFamily.TEX_PARAMETER)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glPatchParameterfv', param, override, other_params=[pname])

    def test_patch_parameter_value_count_accepts_no_tex_sampler_family(self) -> None:
        param = Parameter(name='values', type='const GLfloat *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        # Should not raise.
        _validate_pointer_override('glPatchParameterfv', param, override, other_params=[pname])

    def test_material_parameter_value_count_rejects_tex_sampler_family(self) -> None:
        # glMaterialfv has no sibling family at all (like glPatchParameterfv) - a
        # MATERIAL_PARAMETER_VALUE_COUNT override must never set tex_sampler_family.
        param = Parameter(name='params', type='const GLfloat *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT,
            pname_param='pname', tex_sampler_family=TexOrSamplerParameterFamily.TEX_PARAMETER)

        with self.assertRaises(ValueError):
            _validate_pointer_override('glMaterialfv', param, override, other_params=[pname])

    def test_material_parameter_value_count_accepts_no_tex_sampler_family(self) -> None:
        param = Parameter(name='params', type='const GLfloat *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        # Should not raise.
        _validate_pointer_override('glMaterialfv', param, override, other_params=[pname])

    def test_point_parameter_value_count_rejects_out_direction(self) -> None:
        # glPointParameterfv is a setter, like glPatchParameterfv/glMaterialfv - IN only.
        param = Parameter(name='params', type='const GLfloat *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.OUT,
            length_source_kind=LengthSourceKind.POINT_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        with self.assertRaises(ValueError):
            _validate_pointer_override('glPointParameterfv', param, override, other_params=[pname])

    def test_point_parameter_value_count_accepts_in_direction(self) -> None:
        param = Parameter(name='params', type='const GLfloat *')
        pname = Parameter(name='pname', type='GLenum')
        override = PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.POINT_PARAMETER_VALUE_COUNT,
            pname_param='pname')

        # Should not raise.
        _validate_pointer_override('glPointParameterfv', param, override, other_params=[pname])


class PointerNeedsRawOffsetCaptureTests(unittest.TestCase):
    """Unit tests for pointer_needs_raw_offset_capture() (generator_helpers.py) - which
    pointer-shaped parameters get a companion m_<param>RawPointer field so the player can
    reconstruct the app's original bit pattern even when a buffer-object guard suppressed the
    normal client-memory capture. See that predicate's own header comment for the full
    rationale."""

    def test_none_override_is_not_raw_offset_capturable(self) -> None:
        self.assertFalse(pointer_needs_raw_offset_capture(None))

    def test_unpack_pbo_guard_needs_raw_offset_capture(self) -> None:
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value='imageSize', unpack_pbo_guard=True)
        self.assertTrue(pointer_needs_raw_offset_capture(override))

    def test_query_buffer_guard_needs_raw_offset_capture(self) -> None:
        override = PointerParamOverride(
            direction=PointerDirection.OUT, length_source_kind=LengthSourceKind.FIXED_COUNT,
            length_source_value='1', query_buffer_guard=True)
        self.assertTrue(pointer_needs_raw_offset_capture(override))

    def test_pixel_transfer_size_needs_raw_offset_capture(self) -> None:
        # PIXEL_TRANSFER_SIZE is unconditionally routed through GuardUnpackPixelsPointer/
        # GuardPackPixelsPointer (recorderWrappersAuto.cpp.mako) even without either guard
        # flag explicitly set - see that LengthSourceKind's own comment.
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.PIXEL_TRANSFER_SIZE,
            pixel_format_param='format', pixel_type_param='type', pixel_width_param='width')
        self.assertTrue(pointer_needs_raw_offset_capture(override))

    def test_plain_sibling_param_does_not_need_raw_offset_capture(self) -> None:
        # An ordinary SIBLING_PARAM array (e.g. glGenTextures' `textures`) has no buffer-object
        # duality at all - no guard ever runs for it, so no companion field is needed.
        override = PointerParamOverride(
            direction=PointerDirection.OUT, length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value='n')
        self.assertFalse(pointer_needs_raw_offset_capture(override))

    def test_opaque_bit_pattern_does_not_need_raw_offset_capture(self) -> None:
        # OPAQUE_BIT_PATTERN (glVertexAttribPointer's `pointer`, glDrawElements' `indices`)
        # already captures its own raw bit pattern unconditionally via its own, separate
        # mechanism (commandsAuto.h.mako's m_<param>(reinterpret_cast<uintptr_t>(...)) - see
        # that LengthSourceKind's own comment) - it must not ALSO be treated as needing this
        # predicate's companion field (it has no ArrayArgument<T>/m_<param>.HasValue at all).
        override = PointerParamOverride(
            direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN)
        self.assertFalse(pointer_needs_raw_offset_capture(override))


class IsParamCapturableTests(unittest.TestCase):
    def test_missing_override_leaves_pointer_param_uncapturable_with_no_error(self) -> None:
        # A pointer-shaped param with no POINTER_PARAM_OVERRIDES entry must simply stay
        # uncapturable - never an error. This is what keeps e.g. glGetIntegerv disabled.
        param = Parameter(name='params', type='GLint *')
        self.assertFalse(_is_param_capturable('glGetIntegerv', param, other_params=[]))

    def test_scalar_param_is_always_capturable(self) -> None:
        param = Parameter(name='cap', type='GLenum')
        self.assertTrue(_is_param_capturable('glEnable', param, other_params=[]))

    def test_mapped_handle_param_is_always_capturable(self) -> None:
        param = Parameter(name='hglrc', type='HGLRC')
        self.assertTrue(_is_param_capturable('wglDeleteContext', param, other_params=[]))

    def test_array_param_with_valid_override_is_capturable(self) -> None:
        param = Parameter(name='textures', type='GLuint *')
        sibling = Parameter(name='n', type='GLsizei')
        self.assertTrue(_is_param_capturable('glGenTextures', param, other_params=[sibling]))

    def test_opaque_bit_pattern_array_param_is_capturable_without_an_array_of_arrays_entry(
        self,
    ) -> None:
        # `indices` is structurally is_array_of_arrays_eligible (pointer_depth == 2, base type
        # 'void' - same shape as glShaderSource's `string`), but has no
        # ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry at all - it must still be capturable via its
        # OPAQUE_BIT_PATTERN_ARRAY POINTER_PARAM_OVERRIDES entry instead, checked first - see
        # that LengthSourceKind's own comment (pointer_metadata.py) for why ArrayOfArrays<T>
        # would be actively wrong here.
        param = Parameter(name='indices', type='const void * const *')
        sibling = Parameter(name='drawcount', type='GLsizei')
        self.assertTrue(
            _is_param_capturable(
                'glMultiDrawElementsBaseVertex', param, other_params=[sibling]))

    def test_array_param_with_broken_override_raises(self) -> None:
        # glGenTextures's real override names sibling 'n'; a command reusing the same param
        # name without that sibling present is a genuine table/signature mismatch, not a
        # silent-disable situation.
        param = Parameter(name='textures', type='GLuint *')
        with self.assertRaises(ValueError):
            _is_param_capturable('glGenTextures', param, other_params=[])


class EndToEndEnabledGateTests(unittest.TestCase):
    """Confirms each of the 8 newly-supported commands actually becomes enabled=True from
    real registry data, mirroring test_type_facts.py's pattern of loading a real registry and
    asserting on a named command."""

    def _get_command(self, api: Api, name: str):
        registry = load_registry(api)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
        return get_commands_from_xml_data({api: [raw]})[api][0]

    def test_gl_texture_and_buffer_commands_are_enabled(self) -> None:
        for name in (
            'glGenTextures',
            'glDeleteTextures',
            'glGenBuffers',
            'glBufferData',
            'glBufferSubData',
            'glGetUniformLocation',
            'glBindAttribLocation',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_arb_ext_aliases_of_already_enabled_commands_are_enabled(self) -> None:
        # gl.xml's <alias name="..."/> marks each of these ARB/EXT-suffixed pre-promotion
        # entry points as a byte-for-byte functional duplicate of an already-enabled core
        # command (glDeleteBuffersARB/glGenBuffersARB/glBufferDataARB/
        # glVertexAttribPointerARB/glCompressedTexImage2DARB -> glDeleteBuffers/glGenBuffers/
        # glBufferData/glVertexAttribPointer/glCompressedTexImage2D; glGenFramebuffersEXT ->
        # glGenFramebuffers) - see _pointer_override_for_command_or_alias's own comment
        # (intermediates_creator.py) for why this makes them enabled from the SAME
        # POINTER_PARAM_OVERRIDES entry as their alias target, with no new table row of their
        # own. This was the concrete "playback skipped unsupported OpenGL command:
        # glDeleteBuffersARB/glMapBufferARB/..." gap.
        for name in (
            'glDeleteBuffersARB',
            'glGenBuffersARB',
            'glBufferDataARB',
            'glVertexAttribPointerARB',
            'glCompressedTexImage2DARB',
            'glGenFramebuffersEXT',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_glmapbufferarb_stays_disabled_pending_its_alias_target(self) -> None:
        # glMapBufferARB's alias, glMapBuffer, is itself deliberately NOT enabled at this
        # (Python, pointer_metadata.py-driven) layer - its return value is a live mapped
        # pointer, which pointer_metadata.py's PointerParamOverride/LengthSourceKind vocabulary
        # has no way to express for a *return* value at all (only for parameters - see
        # OPAQUE_BIT_PATTERN's own scoping). The alias fallback only ever inherits an override
        # that already exists for the alias target - it cannot make a command whose own alias
        # is disabled newly enabled, so this one correctly stays disabled=False here.
        #
        # This does NOT mean glMapBuffer/glMapBufferARB have no capture support at all: both
        # gained real, hand-written support via CustomWrapper_glMapBuffer/glMapBufferARB
        # (OpenGL2/recorder/mapTrackingWrappers.cpp), which bypasses this enabled flag entirely
        # by intercepting the recorder's per-name wrapper dispatch directly - see
        # docs/development/opengl2-buffer-map-tracking-plan.md's Decision 3 (this is exactly
        # the same mechanism glMapBufferRange itself already uses - also enabled=False here for
        # the identical "return value is a live pointer" reason - and the same reason
        # glUnmapBuffer's own generated wrapper, despite being enabled=True here, is unused
        # dead code in practice: Decision 3's dispatch-level override intercepts its name too).
        # This test only asserts what this Python layer alone would produce; it says nothing
        # about whether the command is actually captured end-to-end.
        self.assertFalse(self._get_command(Api.GL, 'glMapBuffer').enabled)
        self.assertFalse(self._get_command(Api.GL, 'glMapBufferARB').enabled)

    def test_wgl_create_context_attribs_arb_is_enabled(self) -> None:
        self.assertTrue(
            self._get_command(Api.WGL, 'wglCreateContextAttribsARB').enabled
        )

    def test_wgl_create_context_attribs_arb_attrib_list_uses_key_value_list(self) -> None:
        # Must scan (key, value) pairs, terminating only on a zero KEY - see
        # LengthSourceKind.KEY_VALUE_LIST's own comment for why plain NULL_TERMINATED would
        # silently truncate a list containing a legal zero VALUE (e.g.
        # WGL_CONTEXT_FLAGS_ARB = 0) before the real terminator.
        override = POINTER_PARAM_OVERRIDES[('wglCreateContextAttribsARB', 'attribList')]
        self.assertEqual(override.length_source_kind, LengthSourceKind.KEY_VALUE_LIST)

    def test_opaque_bit_pattern_commands_are_enabled(self) -> None:
        # See docs/development/opengl2-rendering-pipeline-pointer-support-plan.md's
        # offset-vs-pointer decision - the pointer's bit pattern is captured, never
        # dereferenced.
        self.assertTrue(self._get_command(Api.GL, 'glVertexAttribPointer').enabled)

    def test_element_draws_capture_client_indices(self) -> None:
        # `indices` is client memory unless a GL_ELEMENT_ARRAY_BUFFER is bound; both forms are
        # captured (bytes plus raw pointer), so every single element draw is enabled.
        from generator_helpers import pointer_override_for, pointer_needs_raw_offset_capture
        from pointer_metadata import LengthSourceKind
        for name in (
            'glDrawElements', 'glDrawElementsBaseVertex', 'glDrawElementsInstanced',
            'glDrawElementsInstancedBaseVertex', 'glDrawElementsInstancedBaseInstance',
            'glDrawElementsInstancedBaseVertexBaseInstance', 'glDrawRangeElements',
            'glDrawRangeElementsBaseVertex',
        ):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                override = pointer_override_for(cmd, 'indices')
                self.assertEqual(override.length_source_kind,
                                 LengthSourceKind.ELEMENT_INDICES_BYTE_SIZE)
                self.assertEqual(override.length_source_value, 'count')
                self.assertEqual(override.index_type_param, 'type')
                self.assertTrue(pointer_needs_raw_offset_capture(override))

    def test_multi_draw_elements_keep_opaque_index_pointers(self) -> None:
        from generator_helpers import pointer_override_for
        from pointer_metadata import LengthSourceKind
        cmd = self._get_command(Api.GL, 'glMultiDrawElementsBaseVertex')
        self.assertEqual(pointer_override_for(cmd, 'indices').length_source_kind,
                         LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY)

    def test_legacy_client_vertex_array_pointers_are_enabled(self) -> None:
        # glVertexPointer/glColorPointer/glNormalPointer/glTexCoordPointer/
        # glSecondaryColorPointer's `pointer` share glVertexAttribPointer's exact
        # OPAQUE_BIT_PATTERN shape - see pointer_metadata.py's own comment on these entries.
        # This was part of the concrete "playback skipped unsupported OpenGL command" gap
        # (black-screen replay).
        for name in (
            'glVertexPointer',
            'glColorPointer',
            'glNormalPointer',
            'glTexCoordPointer',
            'glSecondaryColorPointer',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_draw_elements_base_vertex_is_enabled(self) -> None:
        # `basevertex` is a plain GLint scalar needing no override.
        self.assertTrue(self._get_command(Api.GL, 'glDrawElementsBaseVertex').enabled)

    def test_material_and_point_parameter_commands_are_enabled(self) -> None:
        # glMaterialfv/glPointParameterfv's value count depends on the `pname` enum's value -
        # see LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT/POINT_PARAMETER_VALUE_COUNT's own
        # comments. This was part of the concrete "playback skipped unsupported OpenGL
        # command: glMaterialfv/glPointParameterfv" gap (black-screen replay, fixed-function
        # lighting/point-sprite state never replayed).
        for name in ('glMaterialfv', 'glPointParameterfv'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_named_buffer_sub_data_commands_are_enabled(self) -> None:
        # glNamedBufferSubDataEXT aliases glNamedBufferSubData (gl.xml: <alias name=
        # "glNamedBufferSubData"/>) - one table entry on the core name enables both via the
        # alias fallback. Same SIBLING_PARAM('size') shape as glBufferSubData's own entry, just
        # DSA-addressed. Part of the DSA buffer/texture content-upload gap (buffers allocated
        # and bound correctly but never filled with real content -> visual corruption on
        # replay).
        for name in ('glNamedBufferSubData', 'glNamedBufferSubDataEXT'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_fixed_function_matrix_commands_are_enabled(self) -> None:
        for name in (
            'glLoadMatrixf', 'glLoadMatrixd', 'glMultMatrixf', 'glMultMatrixd',
            'glLoadTransposeMatrixf', 'glLoadTransposeMatrixfARB',
            'glMultTransposeMatrixd', 'glMultTransposeMatrixdARB',
            'glMatrixLoadfEXT', 'glMatrixMultTransposedEXT',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_buffer_storage_commands_are_enabled(self) -> None:
        # Immutable-storage allocation - the EXT names alias the core names in gl.xml and are
        # enabled through the alias fallback.
        for name in ('glBufferStorage', 'glBufferStorageEXT', 'glNamedBufferStorage',
                     'glNamedBufferStorageEXT'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_compressed_texture_sub_image_2d_ext_is_enabled(self) -> None:
        # glCompressedTextureSubImage2DEXT (GL_EXT_direct_state_access) has no gl.xml <alias> to
        # any core command - needs its own explicit table entry, unlike glNamedBufferSubDataEXT
        # above. Same len="imageSize" SIBLING_PARAM shape as glCompressedTexSubImage2D, with the
        # pointer param named `bits` instead of `data`.
        self.assertTrue(
            self._get_command(Api.GL, 'glCompressedTextureSubImage2DEXT').enabled)

    def test_multi_draw_elements_base_vertex_is_enabled(self) -> None:
        # See pointer_metadata.py's LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY - `indices` must
        # not be silently routed through ARRAY_OF_ARRAYS_PARAM_OVERRIDES/ArrayOfArrays<T>
        # despite matching its structural `const void* const*` shape.
        self.assertTrue(
            self._get_command(Api.GL, 'glMultiDrawElementsBaseVertex').enabled)

    def test_multi_draw_elements_base_vertex_indices_uses_opaque_bit_pattern_array(self) -> None:
        override = POINTER_PARAM_OVERRIDES[('glMultiDrawElementsBaseVertex', 'indices')]
        self.assertEqual(override.length_source_kind, LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY)
        self.assertEqual(override.length_source_value, 'drawcount')

    def test_vector_uniform_setters_are_enabled(self) -> None:
        for name in (
            'glUniform1fv', 'glUniform2fv', 'glUniform3fv', 'glUniform4fv',
            'glUniform1iv', 'glUniform2iv', 'glUniform3iv', 'glUniform4iv',
            'glUniform1uiv', 'glUniform2uiv', 'glUniform3uiv', 'glUniform4uiv',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_matrix_uniform_setters_are_enabled(self) -> None:
        for name in ('glUniformMatrix2fv', 'glUniformMatrix3fv', 'glUniformMatrix4fv'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_uniform_location_param_is_a_plain_scalar_not_an_object_name(self) -> None:
        # 'location' is a GLint from glGetUniformLocation, never a glGen*/glCreate*
        # namespace - see pointer_metadata.py's module comment on this command set.
        from generator_helpers import object_name_override_for
        self.assertIsNone(object_name_override_for('glUniform4fv', 'location', None))

    def test_query_and_fixed_size_getter_commands_are_enabled(self) -> None:
        # See modern-opengl-backend-architecture.md's "Reduce replay warning noise" decision.
        for name in (
            'glGetShaderiv',
            'glGetProgramiv',
            'glGetFramebufferAttachmentParameteriv',
            'glBindFragDataLocation',
            'glGetUniformBlockIndex',
            'glGenQueries',
            'glDeleteQueries',
            'glReadPixels',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_pointer_param_commands_are_enabled(self) -> None:
        # Regression coverage: every command with a newly added POINTER_PARAM_OVERRIDES entry
        # (directly, or via consumed_pointer_params()'s own array-of-arrays absorption - see
        # array_of_arrays_metadata.py) now enables. Each individual entry's own comment in
        # pointer_metadata.py documents exactly why it is
        # safe; this test only pins down the end result across the whole round at once, rather
        # than one assertion per command scattered across many small test methods.
        for name in (
            'glDeleteBuffers',
            'glCompressedTexImage1D', 'glCompressedTexImage2D', 'glCompressedTexImage3D',
            'glCompressedTexSubImage1D', 'glCompressedTexSubImage2D', 'glCompressedTexSubImage3D',
            'glGetActiveUniform', 'glGetActiveAttrib', 'glGetTransformFeedbackVarying',
            'glGetActiveUniformBlockName', 'glGetActiveUniformName',
            'glGetProgramInfoLog', 'glGetProgramPipelineInfoLog', 'glGetShaderInfoLog',
            'glGetQueryiv', 'glGetQueryObjecti64v', 'glGetQueryObjectui64v', 'glGetQueryObjectuiv',
            'glGetProgramStageiv', 'glGetProgramPipelineiv', 'glGetVertexAttribiv',
            'glViewportIndexedfv',
            'glTexParameterfv', 'glTexParameteriv', 'glSamplerParameterfv',
            'glPatchParameterfv',
            'glUniform4dv', 'glUniformMatrix4dv',
            'glProgramUniform4dv', 'glProgramUniformMatrix4dv', 'glProgramUniformMatrix4fv',
            'glUniformSubroutinesuiv',
            'glVertexAttribIPointer', 'glVertexAttribLPointer',
            'glDrawElementsIndirect', 'glDrawElementsInstancedBaseVertexBaseInstance',
            'glDrawBuffers',
            'glGetSubroutineIndex', 'glGetSubroutineUniformLocation', 'glGetAttribLocation',
            'glGenTransformFeedbacks', 'glDeleteTransformFeedbacks',
            'glInvalidateFramebuffer',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_renderbuffer_sampler_program_pipeline_gen_delete_commands_are_enabled(self) -> None:
        # Renderbuffer/Sampler/ProgramPipeline - same
        # SIBLING_PARAM array-output-creation/array-input-destruction shape as
        # glGenQueries/glDeleteQueries above.
        for name in (
            'glGenRenderbuffers', 'glDeleteRenderbuffers',
            'glGenSamplers', 'glDeleteSamplers',
            'glGenProgramPipelines', 'glDeleteProgramPipelines',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_gen_samplers_length_source_is_count_not_n(self) -> None:
        # glGenSamplers/glDeleteSamplers name their element-count sibling param `count`, not
        # `n` like every other Gen*/Delete* pair this backend enables - a real, easy-to-miss
        # divergence a hardcoded 'n' assumption would silently get wrong.
        for cmd_name, param_name in (
            ('glGenSamplers', 'samplers'), ('glDeleteSamplers', 'samplers'),
        ):
            with self.subTest(command=cmd_name):
                override = POINTER_PARAM_OVERRIDES[(cmd_name, param_name)]
                self.assertEqual(override.length_source_kind, LengthSourceKind.SIBLING_PARAM)
                self.assertEqual(override.length_source_value, 'count')

    def test_state_dependent_getters_discard_output(self) -> None:
        # glGetIntegerv's element count depends on `pname` and on implementation state. The
        # output is never captured, so no count is needed.
        from generator_helpers import pointer_override_for
        from pointer_metadata import LengthSourceKind, PointerDirection
        for name, param_name in (
            ('glGetIntegerv', 'data'), ('glGetFloatv', 'data'), ('glGetBooleani_v', 'data'),
            ('glGetUniformfv', 'params'),
        ):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                override = pointer_override_for(cmd, param_name)
                self.assertEqual(override.direction, PointerDirection.OUT)
                self.assertEqual(override.length_source_kind, LengthSourceKind.DISCARDED_OUTPUT)

    def test_discarded_list_outputs_are_sized_from_their_own_parameters(self) -> None:
        # A list result (GL_COMPRESSED_TEXTURE_FORMATS, ...) has a live length; the player
        # queries it with the command's own arguments before the call.
        from generator_helpers import pointer_override_for
        from pointer_length_derivation import (
            _DISCARDED_OUTPUT_LIST_SIZERS, discarded_output_list_sizer)
        from pointer_metadata import LengthSourceKind
        names = ['glGetIntegerv', 'glGetFloatv', 'glGetBooleanv', 'glGetInteger64v',
                 *_DISCARDED_OUTPUT_LIST_SIZERS]
        for name in names:
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                self.assertTrue(any(
                    pointer_override_for(cmd, p.name) is not None and
                    pointer_override_for(cmd, p.name).length_source_kind is
                    LengthSourceKind.DISCARDED_OUTPUT for p in cmd.params))
                sizer = discarded_output_list_sizer(name)
                self.assertIsNotNone(sizer)
                scalars = {p.name for p in cmd.params if p.type_facts.is_scalar}
                self.assertLessEqual(set(sizer[1]), scalars)
        self.assertIsNone(discarded_output_list_sizer('glGetTexParameteriv'))

    def test_query_object_getters_are_not_discarded(self) -> None:
        # With a GL_QUERY_BUFFER bound, `params` is an offset the driver writes GPU-side, so
        # the discarded-output fallback must not apply. Hand-audited entries in
        # POINTER_PARAM_OVERRIDES (glGetQueryObjectui64v, ...) are unaffected.
        from pointer_length_derivation import derive_discarded_output_override
        cmd = self._get_command(Api.GL, 'glGetQueryObjectiv')
        self.assertFalse(cmd.enabled)
        params = next(p for p in cmd.params if p.name == 'params')
        others = [p for p in cmd.params if p.name != 'params']
        self.assertIsNone(
            derive_discarded_output_override('glGetQueryObjectiv', params, others))

    def test_pointer_queries_discard_their_result(self) -> None:
        # A glGet*Pointerv result is an address in the recording process.
        from generator_helpers import pointer_override_for
        for name, param_name in (('glGetBufferPointerv', 'params'),
                                 ('glGetNamedBufferPointervEXT', 'params'),
                                 ('glGetVertexAttribPointerv', 'pointer')):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                override = pointer_override_for(cmd, param_name)
                assert override is not None
                self.assertEqual(override.length_source_kind, LengthSourceKind.DISCARDED_OUTPUT)

    def test_getter_with_unrecognized_compsize_stays_disabled(self) -> None:
        from pointer_length_derivation import derive_discarded_output_override
        cmd = self._get_command(Api.GL, 'glGetIntegerv')
        pname, data = cmd.params
        self.assertIsNone(derive_discarded_output_override(
            'glGetIntegerv', replace(data, len='COMPSIZE(foo,bar)'), [pname]))
        self.assertIsNone(derive_discarded_output_override(
            'glSetIntegerv', data, [pname]))

    def test_get_string_commands_are_enabled_without_return_member(self) -> None:
        from generator_helpers import has_return_member
        for name in ('glGetString', 'glGetStringi'):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                self.assertFalse(has_return_member(cmd))
        self.assertTrue(has_return_member(self._get_command(Api.GL, 'glGenLists')))

    def test_pname_value_count_setters_are_enabled(self) -> None:
        from generator_helpers import pointer_override_for
        from pointer_metadata import LengthSourceKind
        for name in ('glLightfv', 'glLightModelfv', 'glFogfv', 'glTexEnvfv', 'glTexGenfv'):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                override = pointer_override_for(cmd, 'params')
                self.assertEqual(override.length_source_kind, LengthSourceKind.PNAME_VALUE_COUNT)
                self.assertEqual(override.pname_param, 'pname')

    def test_generic_rule_enables_commands_without_hand_entries(self) -> None:
        for name in (
            'glBindTextures', 'glBindSamplers', 'glGetProgramResourceiv', 'glNamedBufferData',
            'glTextureSubImage1D', 'glTextureSubImage2D', 'glTextureSubImage3D',
            'glClearTexImage', 'glClearTexSubImage', 'glClearNamedBufferData',
            'glCompressedTextureSubImage2D', 'glProgramBinary', 'glCreateTransformFeedbacks',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_generic_rule_exceptions_stay_disabled(self) -> None:
        for name in (
            'glSelectBuffer', 'glFeedbackBuffer',           # driver retains the pointer
            'glCullParameterfvEXT',                         # IN array declared without const
            'glGenFencesNV', 'glDeleteFencesNV',            # names without object-name mapping
            'glGetAttachedShaders',                         # OUT existing-object names
            'glGetTexImage',                                # OUT void*, pack-buffer offset
        ):
            with self.subTest(command=name):
                self.assertFalse(self._get_command(Api.GL, name).enabled)

    def test_debug_commands_stay_disabled(self) -> None:
        # glDebugMessageCallback takes an application callback function pointer - structurally
        # uncapturable, never a "just add an override" gap. glDebugMessageControl is left
        # disabled alongside it deliberately (a debug-message filter has no bearing on replay
        # pixel fidelity), even though its `ids` array parameter is structurally no different
        # from glDeleteTextures' own IN SIBLING_PARAM shape - see the "Reduce replay warning
        # noise" decision.
        for name in ('glDebugMessageCallback', 'glDebugMessageControl'):
            with self.subTest(command=name):
                self.assertFalse(self._get_command(Api.GL, name).enabled)

    def test_wgl_pixel_format_commands_are_enabled(self) -> None:
        for name in (
            'DescribePixelFormat',
            'wglGetPixelFormatAttribivARB',
            'ChoosePixelFormat',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.WGL, name).enabled)

    def test_wgl_extensions_string_commands_are_enabled(self) -> None:
        from generator_helpers import has_return_member
        for name in ('wglGetExtensionsStringARB', 'wglGetExtensionsStringEXT'):
            with self.subTest(command=name):
                cmd = self._get_command(Api.WGL, name)
                self.assertTrue(cmd.enabled)
                self.assertFalse(has_return_member(cmd))

    def test_wgl_choose_pixel_format_arb_is_enabled(self) -> None:
        self.assertTrue(self._get_command(Api.WGL, 'wglChoosePixelFormatARB').enabled)

    def test_gl_call_lists_uses_list_name_array_byte_size(self) -> None:
        from pointer_metadata import LengthSourceKind, POINTER_PARAM_OVERRIDES
        self.assertTrue(self._get_command(Api.GL, 'glCallLists').enabled)
        override = POINTER_PARAM_OVERRIDES[('glCallLists', 'lists')]
        self.assertEqual(
            override.length_source_kind, LengthSourceKind.LIST_NAME_ARRAY_BYTE_SIZE)
        self.assertEqual(override.length_source_value, 'n')
        self.assertEqual(override.list_type_param, 'type')

    def test_bitmap_and_polygon_stipple_use_bitmap_byte_size(self) -> None:
        from pointer_metadata import LengthSourceKind, POINTER_PARAM_OVERRIDES
        for name, param_name, dims in (('glBitmap', 'bitmap', ('width', 'height')),
                                       ('glPolygonStipple', 'mask', (None, None))):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)
                override = POINTER_PARAM_OVERRIDES[(name, param_name)]
                self.assertEqual(override.length_source_kind, LengthSourceKind.BITMAP_BYTE_SIZE)
                self.assertEqual((override.pixel_width_param, override.pixel_height_param), dims)
                self.assertTrue(override.unpack_pbo_guard)

    def test_named_buffer_sub_data_commands_are_enabled(self) -> None:
        # glNamedBufferSubDataEXT aliases glNamedBufferSubData (gl.xml's own <alias> tag) - one
        # POINTER_PARAM_OVERRIDES entry on the core name enables both via
        # _pointer_override_for_command_or_alias's alias fallback. Part of the same replay-log
        # gap analysis as glMaterialfv/glPointParameterfv above (DSA buffer-content upload never
        # replayed, contributing to visual corruption once the vertex-array-pointer family was
        # separately fixed).
        for name in ('glNamedBufferSubData', 'glNamedBufferSubDataEXT'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_compressed_texture_sub_image_2d_ext_is_enabled(self) -> None:
        # glCompressedTextureSubImage2DEXT (GL_EXT_direct_state_access) has no <alias> in gl.xml
        # to any core command - needs its own POINTER_PARAM_OVERRIDES entry, not just an alias
        # fallback. Same len="imageSize" sibling shape as glCompressedTexSubImage2D, just DSA-
        # addressed and named `bits` instead of `data`.
        self.assertTrue(
            self._get_command(Api.GL, 'glCompressedTextureSubImage2DEXT').enabled)


if __name__ == '__main__':
    unittest.main()
