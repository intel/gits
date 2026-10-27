# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_helpers import pointer_needs_pixel_transfer_unknown_flag, pointer_override_for
from intermediates import Api, Command, CmdType, Parameter, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    _is_param_capturable,  # pyright: ignore[reportPrivateUsage]
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from pixel_transfer_metadata import derive_pixel_transfer_override
from pointer_metadata import LengthSourceKind, PointerDirection, POINTER_PARAM_OVERRIDES
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


def _format_param() -> Parameter:
    return Parameter(name='format', type='GLenum', group='PixelFormat')


def _type_param() -> Parameter:
    return Parameter(name='type', type='GLenum', group='PixelType')


def _dim_param(name: str) -> Parameter:
    return Parameter(name=name, type='GLsizei')


class DerivePixelTransferOverrideUnitTests(unittest.TestCase):
    """Direct unit tests against derive_pixel_transfer_override() - no registry I/O, so every
    accept/reject boundary from that function's own header comment is pinned down precisely,
    independent of what the live gl.xml happens to contain today."""

    def test_2d_in_direction_is_derived(self) -> None:
        # glTexImage2D's own shape: const void*, COMPSIZE(format,type,width,height).
        param = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        override = derive_pixel_transfer_override(param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)
        self.assertEqual(override.pixel_format_param, 'format')
        self.assertEqual(override.pixel_type_param, 'type')
        self.assertEqual(override.pixel_width_param, 'width')
        self.assertEqual(override.pixel_height_param, 'height')
        self.assertIsNone(override.pixel_depth_param)

    def test_1d_shape_is_derived_with_no_height_or_depth(self) -> None:
        param = Parameter(name='pixels', type='const void *', len='COMPSIZE(format,type,width)')
        other = [_format_param(), _type_param(), _dim_param('width')]

        override = derive_pixel_transfer_override(param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.pixel_width_param, 'width')
        self.assertIsNone(override.pixel_height_param)
        self.assertIsNone(override.pixel_depth_param)

    def test_3d_shape_is_derived_with_depth(self) -> None:
        # glTexSubImage3D's own shape: const void*, COMPSIZE(format,type,width,height,depth).
        param = Parameter(
            name='pixels', type='const void *',
            len='COMPSIZE(format,type,width,height,depth)')
        other = [
            _format_param(), _type_param(),
            _dim_param('width'), _dim_param('height'), _dim_param('depth'),
        ]

        override = derive_pixel_transfer_override(param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.pixel_width_param, 'width')
        self.assertEqual(override.pixel_height_param, 'height')
        self.assertEqual(override.pixel_depth_param, 'depth')

    def test_out_direction_plain_void_pointer_is_derived(self) -> None:
        # glReadPixels' own shape: plain void*, not const.
        param = Parameter(
            name='pixels', type='void *', len='COMPSIZE(format,type,width,height)')
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        override = derive_pixel_transfer_override(param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.OUT)

    def test_non_compsize_len_is_not_derived(self) -> None:
        # glCompressedTexImage2D's own shape: len="imageSize", a plain sibling reference, not
        # COMPSIZE(...) at all - must NOT be derived by this fallback (see
        # pointer_metadata.py's own SIBLING_PARAM entries for these commands instead).
        param = Parameter(name='data', type='const void *', len='imageSize')
        other = [_dim_param('imageSize')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_missing_len_is_recovered_from_siblings(self) -> None:
        # glTextureSubImage2D's `pixels`: gl.xml omits len=, the siblings still give the shape.
        param = Parameter(name='pixels', type='const void *', len=None)
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        override = derive_pixel_transfer_override(param, other)
        self.assertEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)
        self.assertEqual(override.pixel_width_param, 'width')
        self.assertEqual(override.pixel_height_param, 'height')
        self.assertIsNone(override.pixel_depth_param)

    def test_missing_len_on_out_pointer_is_not_derived(self) -> None:
        param = Parameter(name='pixels', type='void *', len=None)
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_wrong_leading_names_are_not_derived(self) -> None:
        # COMPSIZE(target,level,format,type) - glGetTexImage's own shape. Starts with
        # target/level, not format/type - must not be derived.
        param = Parameter(
            name='pixels', type='void *', len='COMPSIZE(target,level,format,type)')
        other = [
            Parameter(name='target', type='GLenum'), Parameter(name='level', type='GLint'),
            _format_param(), _type_param(),
        ]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_pname_only_compsize_is_not_derived(self) -> None:
        # COMPSIZE(pname) - glTexParameterfv's own shape (a completely different formula - see
        # pointer_metadata.py's TEX_SAMPLER_PARAMETER_VALUE_COUNT).
        param = Parameter(name='params', type='const GLfloat *', len='COMPSIZE(pname)')
        other = [Parameter(name='pname', type='GLenum')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_clear_buffer_shaped_compsize_is_not_derived(self) -> None:
        # COMPSIZE(buffer) - glClearBufferfv's own shape.
        param = Parameter(name='value', type='const GLfloat *', len='COMPSIZE(buffer)')
        other = [Parameter(name='buffer', type='GLenum')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_dimensionless_compsize_is_one_texel(self) -> None:
        # COMPSIZE(format,type) - glClearTexImage's shape: a single clear value, not an image.
        param = Parameter(name='data', type='const void *', len='COMPSIZE(format,type)')
        other = [_format_param(), _type_param()]

        override = derive_pixel_transfer_override(param, other)
        self.assertEqual(override.length_source_kind, LengthSourceKind.TEXEL_BYTE_SIZE)
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertFalse(override.unpack_pbo_guard)

    def test_extra_trailing_dimension_is_not_derived(self) -> None:
        # COMPSIZE(format,type,width,height,depth,size4d) - glTexImage4DSGIS's own shape: a
        # genuine fifth dimension no version of ComputePixelTransferByteSize() computes.
        param = Parameter(
            name='pixels', type='const void *',
            len='COMPSIZE(format,type,width,height,depth,size4d)')
        other = [
            _format_param(), _type_param(), _dim_param('width'), _dim_param('height'),
            _dim_param('depth'), _dim_param('size4d'),
        ]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_third_name_other_than_width_is_not_derived(self) -> None:
        # COMPSIZE(format,type,count) - glColorSubTable's own shape: a 1-D update whose third
        # name is `count`, not `width`.
        param = Parameter(name='data', type='const void *', len='COMPSIZE(format,type,count)')
        other = [_format_param(), _type_param(), _dim_param('count')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_missing_format_sibling_is_not_derived(self) -> None:
        param = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        other = [_type_param(), _dim_param('width'), _dim_param('height')]  # no 'format'

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_wrong_group_on_format_is_not_derived(self) -> None:
        # `format` exists but is not gl.xml's PixelFormat-group enum - a same-named, different-
        # meaning parameter, not a real pixel-transfer format.
        param = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        other = [
            Parameter(name='format', type='GLenum', group='InternalFormat'), _type_param(),
            _dim_param('width'), _dim_param('height'),
        ]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_wrong_group_on_type_is_not_derived(self) -> None:
        param = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        other = [
            _format_param(), Parameter(name='type', type='GLenum', group='DataType'),
            _dim_param('width'), _dim_param('height'),
        ]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_non_scalar_dimension_sibling_is_not_derived(self) -> None:
        param = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        other = [
            _format_param(), _type_param(),
            Parameter(name='width', type='GLsizei *'),  # not scalar
            _dim_param('height'),
        ]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_double_indirection_is_not_derived(self) -> None:
        # Structurally matches every other check, but is a `const void **`, not a single-
        # indirection `const void *` - must not be derived (this module never touches
        # is_array_of_arrays_eligible-shaped params).
        param = Parameter(
            name='pixels', type='const void **', len='COMPSIZE(format,type,width,height)')
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_non_void_base_type_is_not_derived(self) -> None:
        param = Parameter(
            name='pixels', type='const GLubyte *', len='COMPSIZE(format,type,width,height)')
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))

    def test_unexpected_cv_qualification_is_not_derived(self) -> None:
        # Neither of the two exact spellings this module accepts ('const void *' / 'void *') -
        # falls back to None rather than guessing a direction.
        param = Parameter(
            name='pixels', type='const void * const', len='COMPSIZE(format,type,width,height)')
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]

        self.assertIsNone(derive_pixel_transfer_override(param, other))


class PointerOverrideForFallbackTests(unittest.TestCase):
    """generator_helpers.py's pointer_override_for() must consult this fallback, and an
    explicit POINTER_PARAM_OVERRIDES entry must still take priority over it."""

    def _make_command(self, params: list[Parameter]) -> Command:
        return Command(
            name='glFakeTexImageForTest', api=Api.GL, enabled=True,
            command_type=CmdType.PARAM, return_value=ReturnValue(type='void'), params=params)

    def test_fallback_is_used_when_no_explicit_entry_exists(self) -> None:
        pixels = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        cmd = self._make_command(
            [_format_param(), _type_param(), _dim_param('width'), _dim_param('height'), pixels])
        self.assertNotIn((cmd.name, 'pixels'), POINTER_PARAM_OVERRIDES)

        override = pointer_override_for(cmd, 'pixels')

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)

    def test_unknown_param_name_returns_none(self) -> None:
        cmd = self._make_command([_format_param()])
        self.assertIsNone(pointer_override_for(cmd, 'does_not_exist'))

    def test_fallback_returns_none_for_a_plain_scalar_param(self) -> None:
        cmd = self._make_command([_format_param()])
        self.assertIsNone(pointer_override_for(cmd, 'format'))


class EndToEndPixelTransferTests(unittest.TestCase):
    """Confirms real, live gl.xml commands actually get auto-derived (or correctly do NOT),
    mirroring test_pointer_metadata.py's EndToEndEnabledGateTests pattern."""

    def _get_command(self, api: Api, name: str) -> Command:
        registry = load_registry(api)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
        return get_commands_from_xml_data({api: [raw]})[api][0]

    def test_teximage2d_pixels_has_no_explicit_table_entry(self) -> None:
        # See pointer_metadata.py's own comment: this manual entry was removed once this
        # fallback covered it - pinning that removal down as a regression check.
        self.assertNotIn(('glTexImage2D', 'pixels'), POINTER_PARAM_OVERRIDES)

    def test_readpixels_pixels_has_no_explicit_table_entry(self) -> None:
        self.assertNotIn(('glReadPixels', 'pixels'), POINTER_PARAM_OVERRIDES)

    def test_teximage2d_is_enabled_and_uses_pixel_transfer_size(self) -> None:
        cmd = self._get_command(Api.GL, 'glTexImage2D')
        self.assertTrue(cmd.enabled)
        param = next(p for p in cmd.params if p.name == 'pixels')
        override = pointer_override_for(cmd, 'pixels')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)
        self.assertIsNone(override.pixel_depth_param)
        self.assertTrue(_is_param_capturable(cmd.name, param, [p for p in cmd.params if p is not param]))

    def test_texsubimage3d_is_enabled_and_uses_depth(self) -> None:
        cmd = self._get_command(Api.GL, 'glTexSubImage3D')
        self.assertTrue(cmd.enabled)
        override = pointer_override_for(cmd, 'pixels')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)
        self.assertEqual(override.pixel_depth_param, 'depth')

    def test_readpixels_is_enabled_and_out_direction(self) -> None:
        cmd = self._get_command(Api.GL, 'glReadPixels')
        self.assertTrue(cmd.enabled)
        override = pointer_override_for(cmd, 'pixels')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.OUT)
        self.assertEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)

    def test_compressed_teximage_stays_on_sibling_param_not_this_fallback(self) -> None:
        # glCompressedTexImage2D's `data` is enabled via its own explicit SIBLING_PARAM
        # ('imageSize') entry - this fallback must never fire for it (len="imageSize" is not
        # COMPSIZE(...) at all).
        cmd = self._get_command(Api.GL, 'glCompressedTexImage2D')
        self.assertTrue(cmd.enabled)
        override = pointer_override_for(cmd, 'data')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_source_kind, LengthSourceKind.SIBLING_PARAM)
        self.assertEqual(override.length_source_value, 'imageSize')

    def test_gettteximage_stays_disabled(self) -> None:
        # glGetTexImage's COMPSIZE(target,level,format,type) does not start with
        # (format, type, ...) - must stay disabled, not silently mis-derived.
        cmd = self._get_command(Api.GL, 'glGetTexImage')
        param = next(p for p in cmd.params if p.name == 'pixels')
        self.assertIsNone(pointer_override_for(cmd, 'pixels'))
        self.assertFalse(
            _is_param_capturable(cmd.name, param, [p for p in cmd.params if p is not param]))


class PointerNeedsPixelTransferUnknownFlagTests(unittest.TestCase):
    """Direct unit tests against pointer_needs_pixel_transfer_unknown_flag() - no registry I/O,
    no C++ source parsing."""

    def test_none_override_is_false(self) -> None:
        self.assertFalse(pointer_needs_pixel_transfer_unknown_flag(None))

    def test_pixel_transfer_size_override_is_true(self) -> None:
        param = Parameter(
            name='pixels', type='const void *', len='COMPSIZE(format,type,width,height)')
        other = [_format_param(), _type_param(), _dim_param('width'), _dim_param('height')]
        override = derive_pixel_transfer_override(param, other)
        self.assertIsNotNone(override)
        self.assertTrue(pointer_needs_pixel_transfer_unknown_flag(override))

    def test_non_pixel_transfer_override_is_false(self) -> None:
        # glBufferData-shaped SIBLING_PARAM override - not PIXEL_TRANSFER_SIZE, must not get
        # the flag.
        override = POINTER_PARAM_OVERRIDES.get(('glBufferData', 'data'))
        self.assertIsNotNone(override)
        assert override is not None
        self.assertNotEqual(override.length_source_kind, LengthSourceKind.PIXEL_TRANSFER_SIZE)
        self.assertFalse(pointer_needs_pixel_transfer_unknown_flag(override))


class PixelTransferSizeIsUnknownCppHelperTests(unittest.TestCase):
    """Source-text regression coverage for PixelTransferSizeIsUnknown()/the player-side skip
    logic it feeds - there is no C++ unit test infrastructure in this repo (see AGENTS.md's
    hard build/test constraints), so this parses the actual generated/hand-written C++ text
    the same way
    test_tex_sampler_parameter_metadata.py's CROP_RECT_OES regression test already does."""

    _PIXEL_TRANSFER_SIZE_H = (
        CODEGEN_DIR.parent / 'common' / 'layer_interface' / 'pixelTransferSize.h')
    _PIXEL_TRANSFER_SIZE_CPP = (
        CODEGEN_DIR.parent / 'common' / 'layer_interface' / 'pixelTransferSize.cpp')
    _COMMANDS_AUTO_H_MAKO = CODEGEN_DIR / 'templates' / 'commandsAuto.h.mako'
    _COMMAND_CODERS_AUTO_CPP_MAKO = CODEGEN_DIR / 'templates' / 'commandCodersAuto.cpp.mako'
    _RECORDER_WRAPPERS_AUTO_CPP_MAKO = CODEGEN_DIR / 'templates' / 'recorderWrappersAuto.cpp.mako'
    _COMMAND_RUNNERS_AUTO_CPP_MAKO = CODEGEN_DIR / 'templates' / 'commandRunnersAuto.cpp.mako'

    def test_header_declares_the_predicate(self) -> None:
        source = self._PIXEL_TRANSFER_SIZE_H.read_text(encoding='utf-8')
        self.assertIn('bool PixelTransferSizeIsUnknown(', source)

    def test_implementation_short_circuits_non_positive_dimensions_to_false(self) -> None:
        source = self._PIXEL_TRANSFER_SIZE_CPP.read_text(encoding='utf-8')
        match = re.search(
            r'bool PixelTransferSizeIsUnknown\((.*?)\n\}', source, re.DOTALL)
        self.assertIsNotNone(match, 'PixelTransferSizeIsUnknown() definition not found')
        assert match is not None
        body = match.group(1)
        # The one genuinely-zero-byte case must short-circuit to false BEFORE the delegating
        # call to ComputePixelTransferByteSize() below it - i.e. this exact condition must
        # appear textually before "== 0" in the function body.
        dims_check_pos = body.find('width <= 0 || height <= 0 || depth <= 0')
        self.assertNotEqual(dims_check_pos, -1,
                            'Non-positive width/height/depth short-circuit not found')
        return_false_pos = body.find('return false', dims_check_pos)
        self.assertNotEqual(return_false_pos, -1,
                            'Non-positive width/height/depth case does not return false')
        delegate_pos = body.find('return ComputePixelTransferByteSize(')
        self.assertNotEqual(delegate_pos, -1,
                            'PixelTransferSizeIsUnknown() does not delegate to '
                            'ComputePixelTransferByteSize()')
        self.assertLess(return_false_pos, delegate_pos,
                        'Non-positive-dimension short-circuit must run before the delegating '
                        'ComputePixelTransferByteSize() call')
        self.assertIn('== 0', body[delegate_pos:])

    def test_commands_auto_h_mako_declares_size_unknown_member(self) -> None:
        source = self._COMMANDS_AUTO_H_MAKO.read_text(encoding='utf-8')
        self.assertIn('pointer_needs_pixel_transfer_unknown_flag(member_ptr_override)', source)
        self.assertIn('Argument<bool> m_${p.name}SizeUnknown;', source)

    def test_command_coders_auto_cpp_mako_walks_size_unknown_member(self) -> None:
        source = self._COMMAND_CODERS_AUTO_CPP_MAKO.read_text(encoding='utf-8')
        self.assertIn('command.m_${p.name}SizeUnknown', source)

    def test_recorder_wrappers_auto_cpp_mako_populates_size_unknown_both_directions(
            self) -> None:
        source = self._RECORDER_WRAPPERS_AUTO_CPP_MAKO.read_text(encoding='utf-8')
        # Populated exactly twice: once for the IN-direction case (before the real call, next
        # to the RawPointer companion), once for the OUT-direction case (after Capture()).
        matches = re.findall(
            r'command->m_\$\{p\.name\}SizeUnknown\.Value = PixelTransferSizeIsUnknown\(',
            source)
        self.assertEqual(len(matches), 2,
                         'Expected exactly one IN-direction and one OUT-direction '
                         'm_<param>SizeUnknown population site')

    def test_command_runners_auto_cpp_mako_skips_real_call_when_unknown(self) -> None:
        source = self._COMMAND_RUNNERS_AUTO_CPP_MAKO.read_text(encoding='utf-8')
        self.assertIn('pixel_transfer_size_params', source)
        self.assertIn('SizeUnknown.Value', source)
        self.assertIn('LOG_WARNING', source)
        self.assertIn('degraded fidelity', source)
        # The real driver call itself must be textually inside the "else" arm of the
        # skip-when-unknown branch, not merely logged alongside an unconditional call.
        pixel_transfer_gate_pos = source.find('${cmd.name}PixelTransferSizeUnknown')
        self.assertNotEqual(pixel_transfer_gate_pos, -1)
        real_call_pos = source.find('realFunction(${call_args})', pixel_transfer_gate_pos)
        self.assertNotEqual(real_call_pos, -1)
        else_pos = source.find('} else {', pixel_transfer_gate_pos)
        self.assertNotEqual(else_pos, -1)
        self.assertLess(else_pos, real_call_pos,
                        'Real driver call must be gated behind the size-unknown check\'s '
                        '"else" arm')


class PixelBufferOffsetFlagCppHelperTests(unittest.TestCase):
    """Source-text regression coverage for the m_<param>IsBufferOffset companion flag and the
    player-side skip-gate bypass it feeds: a PIXEL_TRANSFER_SIZE-shaped call whose pointer was
    actually captured in buffer-offset mode (a pixel unpack/pack buffer object bound) must
    still make the real driver call - using the preserved raw offset - even when the captured
    byte size is otherwise "unknown". There is no C++ unit test infrastructure in this repo
    (see AGENTS.md's hard build/test constraints), so this parses the actual
    generated/hand-written C++ text the same way PixelTransferSizeIsUnknownCppHelperTests
    above does."""

    _CLIENT_MEMORY_CAPTURE_H = (
        CODEGEN_DIR.parent / 'recorder' / 'include' / 'clientMemoryCapture.h')
    _CLIENT_MEMORY_CAPTURE_CPP = CODEGEN_DIR.parent / 'recorder' / 'clientMemoryCapture.cpp'
    _COMMANDS_AUTO_H_MAKO = CODEGEN_DIR / 'templates' / 'commandsAuto.h.mako'
    _COMMAND_CODERS_AUTO_CPP_MAKO = CODEGEN_DIR / 'templates' / 'commandCodersAuto.cpp.mako'
    _RECORDER_WRAPPERS_AUTO_CPP_MAKO = CODEGEN_DIR / 'templates' / 'recorderWrappersAuto.cpp.mako'
    _COMMAND_RUNNERS_AUTO_CPP_MAKO = CODEGEN_DIR / 'templates' / 'commandRunnersAuto.cpp.mako'

    def test_header_declares_both_predicates(self) -> None:
        source = self._CLIENT_MEMORY_CAPTURE_H.read_text(encoding='utf-8')
        self.assertIn('bool UnpackPixelsPointerIsBufferOffset(const void* pixels);', source)
        self.assertIn('bool PackPixelsPointerIsBufferOffset(const void* pixels);', source)

    def test_implementation_shares_guard_pixels_pointers_own_condition(self) -> None:
        source = self._CLIENT_MEMORY_CAPTURE_CPP.read_text(encoding='utf-8')
        # GuardPixelsPointer() itself must be expressed in terms of the same shared predicate
        # the two new query functions use, so the guard's own gating decision and the query
        # functions' answer can never independently drift out of sync.
        guard_match = re.search(
            r'const void\* GuardPixelsPointer\(.*?\n\}', source, re.DOTALL)
        self.assertIsNotNone(guard_match, 'GuardPixelsPointer() definition not found')
        assert guard_match is not None
        self.assertIn('PixelBufferObjectIsBound(', guard_match.group(0))

        unpack_match = re.search(
            r'bool UnpackPixelsPointerIsBufferOffset\(const void\* [^)]*\) \{(.*?)\n\}',
            source, re.DOTALL)
        self.assertIsNotNone(unpack_match, 'UnpackPixelsPointerIsBufferOffset() not found')
        assert unpack_match is not None
        self.assertIn('PixelBufferObjectIsBound(GL_PIXEL_UNPACK_BUFFER_BINDING)',
                      unpack_match.group(1))

        pack_match = re.search(
            r'bool PackPixelsPointerIsBufferOffset\(const void\* [^)]*\) \{(.*?)\n\}',
            source, re.DOTALL)
        self.assertIsNotNone(pack_match, 'PackPixelsPointerIsBufferOffset() not found')
        assert pack_match is not None
        self.assertIn('PixelBufferObjectIsBound(GL_PIXEL_PACK_BUFFER_BINDING)',
                      pack_match.group(1))

    def test_predicate_does_not_gate_on_pixels_nullness(self) -> None:
        """Regression test: a legal PBO offset of 0 is passed by the app as `pixels == nullptr`
        when a pixel-(un)pack buffer is bound. PixelBufferObjectIsBound() must classify that as
        buffer-offset mode purely from the buffer binding, never by first checking
        `pixels != nullptr` - a null-check gate would misreport offset-0 PBO calls as "not
        buffer-offset mode", which (combined with the SizeUnknown skip gate) would incorrectly
        skip a perfectly valid, PBO-backed call instead of letting it run normally."""
        source = self._CLIENT_MEMORY_CAPTURE_CPP.read_text(encoding='utf-8')
        bound_match = re.search(
            r'bool PixelBufferObjectIsBound\((.*?)\) \{(.*?)\n\}', source, re.DOTALL)
        self.assertIsNotNone(bound_match, 'PixelBufferObjectIsBound() not found')
        assert bound_match is not None
        params, body = bound_match.group(1), bound_match.group(2)
        self.assertNotIn('pixels', params,
                         'PixelBufferObjectIsBound() must not take a pixels parameter at all - '
                         'buffer-offset mode must never depend on the pointer value')
        self.assertNotIn('!= nullptr', body,
                         'PixelBufferObjectIsBound() must not null-check anything - a legal '
                         'offset-0 PBO call (pixels == nullptr) must still be classified as '
                         'buffer-offset mode')
        self.assertIn('ContextSupportsPixelBufferObject()', body)
        self.assertIn('QueryBoundBufferQuiet(bindingPname) != 0', body)

        # Both public predicates must delegate to the single-argument form (no pixels forwarded
        # into the bound-check), so a nullptr `pixels` argument to either public function can
        # never suppress "buffer-offset mode" for a genuinely bound buffer.
        unpack_match = re.search(
            r'bool UnpackPixelsPointerIsBufferOffset\(const void\* [^)]*\) \{(.*?)\n\}',
            source, re.DOTALL)
        self.assertIsNotNone(unpack_match)
        assert unpack_match is not None
        self.assertIn('PixelBufferObjectIsBound(GL_PIXEL_UNPACK_BUFFER_BINDING)',
                      unpack_match.group(1))

        pack_match = re.search(
            r'bool PackPixelsPointerIsBufferOffset\(const void\* [^)]*\) \{(.*?)\n\}',
            source, re.DOTALL)
        self.assertIsNotNone(pack_match)
        assert pack_match is not None
        self.assertIn('PixelBufferObjectIsBound(GL_PIXEL_PACK_BUFFER_BINDING)',
                      pack_match.group(1))

    def test_commands_auto_h_mako_declares_is_buffer_offset_member(self) -> None:
        source = self._COMMANDS_AUTO_H_MAKO.read_text(encoding='utf-8')
        self.assertIn('Argument<bool> m_${p.name}IsBufferOffset;', source)
        # Must be declared after m_<param>SizeUnknown, not before - both are gated by the same
        # predicate, but the runner-side skip gate reads SizeUnknown first.
        size_unknown_pos = source.find('Argument<bool> m_${p.name}SizeUnknown;')
        is_buffer_offset_pos = source.find('Argument<bool> m_${p.name}IsBufferOffset;')
        self.assertNotEqual(size_unknown_pos, -1)
        self.assertNotEqual(is_buffer_offset_pos, -1)
        self.assertLess(size_unknown_pos, is_buffer_offset_pos)

    def test_command_coders_auto_cpp_mako_walks_is_buffer_offset_member(self) -> None:
        source = self._COMMAND_CODERS_AUTO_CPP_MAKO.read_text(encoding='utf-8')
        self.assertIn('command.m_${p.name}IsBufferOffset', source)

    def test_recorder_wrappers_auto_cpp_mako_populates_is_buffer_offset_both_directions(
            self) -> None:
        source = self._RECORDER_WRAPPERS_AUTO_CPP_MAKO.read_text(encoding='utf-8')
        self.assertIn(
            'command->m_${p.name}IsBufferOffset.Value = '
            'UnpackPixelsPointerIsBufferOffset(${p.name});', source)
        self.assertIn(
            'command->m_${p.name}IsBufferOffset.Value = '
            'PackPixelsPointerIsBufferOffset(${p.name});', source)

    def test_command_runners_auto_cpp_mako_bypasses_skip_for_buffer_offset_calls(self) -> None:
        source = self._COMMAND_RUNNERS_AUTO_CPP_MAKO.read_text(encoding='utf-8')
        # The per-parameter contribution to the skip gate must be
        # "SizeUnknown && !IsBufferOffset", not SizeUnknown alone - i.e. a buffer-offset call
        # bypasses the skip regardless of whether its size is otherwise unknown. So does a null
        # client pointer of a command that then only allocates storage.
        self.assertIn(
            '(command.m_${p.name}SizeUnknown.Value && '
            '!command.m_${p.name}IsBufferOffset.Value)', source)
        self.assertIn(
            '(command.m_${p.name}SizeUnknown.Value && '
            '!command.m_${p.name}IsBufferOffset.Value && '
            'command.m_${p.name}RawPointer.Value != 0)', source)

    def test_generated_runner_bypasses_skip_for_gltteximage2d(self) -> None:
        # End-to-end check against the actual generated (not template) source for a real
        # enabled command: confirms codegen output, not just template text, reflects the fix.
        generated = CODEGEN_DIR.parent / 'player' / 'commandRunnersAuto.cpp'
        if not generated.exists():
            self.skipTest('generated commandRunnersAuto.cpp not present (codegen not run)')
        source = generated.read_text(encoding='utf-8')
        match = re.search(
            r'glTexImage2DPixelTransferSizeUnknown = (.*?);', source)
        self.assertIsNotNone(match, 'glTexImage2DPixelTransferSizeUnknown not found')
        assert match is not None
        expr = match.group(1)
        self.assertIn('m_pixelsSizeUnknown.Value', expr)
        self.assertIn('!command.m_pixelsIsBufferOffset.Value', expr)
        self.assertIn('command.m_pixelsRawPointer.Value != 0', expr)

    def test_generated_runner_skips_null_pointer_updates_and_readbacks(self) -> None:
        # A null pointer allocates only for glTexImage*; updates, draws and readbacks would
        # read or write through it, so they keep the skip.
        generated = CODEGEN_DIR.parent / 'player' / 'commandRunnersAuto.cpp'
        if not generated.exists():
            self.skipTest('generated commandRunnersAuto.cpp not present (codegen not run)')
        source = generated.read_text(encoding='utf-8')
        for name in ('glTexSubImage2D', 'glDrawPixels', 'glReadPixels'):
            match = re.search(name + r'PixelTransferSizeUnknown = (.*?);', source)
            self.assertIsNotNone(match, name + 'PixelTransferSizeUnknown not found')
            assert match is not None
            self.assertIn('SizeUnknown.Value', match.group(1))
            self.assertNotIn('RawPointer', match.group(1), name)


if __name__ == '__main__':
    unittest.main()
