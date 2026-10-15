# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import unittest
from pathlib import Path
from typing import Any

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_interceptor import (
    _make_wgl_swap_multiple_buffers,  # pyright: ignore[reportPrivateUsage]
)
from intermediates import (
    Api,
    TypeFacts,
)
from intermediates_creator import (
    FILE_TO_API,
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


def raw_command(name: str) -> dict[str, Any]:
    return {
        'command_attribs': {'name': name},
        'return_value': {'type': 'void', 'attribs': {}},
        'params': [],
        'alias_attribs': {},
    }


class TypeFactsTests(unittest.TestCase):
    def test_registry_declarations_preserve_type_facts(self) -> None:
        cases = (
            (
                Api.EGL,
                'eglCreateWaylandBufferFromImageWL',
                'return_value',
                'struct wl_buffer *',
                TypeFacts(base_type='struct wl_buffer', pointer_depth=1),
            ),
            (
                Api.WGL,
                'wglChoosePixelFormatARB',
                'pfAttribFList',
                'const FLOAT *',
                TypeFacts(base_type='FLOAT', pointer_depth=1),
            ),
            (
                Api.GLX,
                'glXAssociateDMPbufferSGIX',
                'dpy',
                'Display *',
                TypeFacts(base_type='Display', pointer_depth=1),
            ),
            (
                Api.GL,
                'glCompileShaderIncludeARB',
                'path',
                'const GLchar *const *',
                TypeFacts(base_type='GLchar', pointer_depth=2),
            ),
            (
                Api.GL,
                'glColorPointer',
                'pointer',
                'const void *',
                TypeFacts(base_type='void', pointer_depth=1),
            ),
            (
                Api.EGL,
                'eglCreatePixmapSurfaceHI',
                'pixmap',
                'struct EGLClientPixmapHI *',
                TypeFacts(base_type='struct EGLClientPixmapHI', pointer_depth=1),
            ),
        )

        for api, command_name, parameter_name, expected_type, expected_type_facts in cases:
            with self.subTest(command=command_name, parameter=parameter_name):
                registry = load_registry(api)
                raw_commands = extract_commands_from_registry(registry)
                raw = next(
                    command for command in raw_commands
                    if command['command_attribs']['name'] == command_name
                )
                raw_parameter = (
                    raw['return_value']
                    if parameter_name == 'return_value'
                    else next(param for param in raw['params'] if param['name'] == parameter_name)
                )

                self.assertEqual(raw_parameter['type'], expected_type)

                command = get_commands_from_xml_data({api: [raw]})[api][0]
                parameter = (
                    command.return_value
                    if parameter_name == 'return_value'
                    else next(param for param in command.params if param.name == parameter_name)
                )
                self.assertEqual(parameter.type, expected_type)
                self.assertEqual(parameter.type_facts, expected_type_facts)

    def test_parameter_len_expression_is_preserved(self) -> None:
        expected_type = 'const GLubyte *'
        expected_len = 'COMPSIZE(format,type)'

        raw = raw_command('glExample')
        raw['params'] = [{
            'name': 'data',
            'type': expected_type,
            'attribs': {'len': expected_len},
        }]

        command = get_commands_from_xml_data({Api.GL: [raw]})[Api.GL][0]

        self.assertEqual(command.params[0].len, expected_len)
        self.assertEqual(command.params[0].type, expected_type)
        self.assertEqual(
            command.params[0].type_facts,
            TypeFacts(base_type='GLubyte', pointer_depth=1),
        )

    def test_is_scalar_rejects_pointer_typedefs_with_no_star(self) -> None:
        # GLDEBUGPROC and friends are function-pointer typedefs but, unlike e.g.
        # "const void *", have zero '*' in their own declared type text - the pointer is
        # baked into the typedef. is_scalar must catch these via base_type, not just
        # pointer_depth, or a naive "pointer_depth == 0" rule would wrongly auto-enable
        # glDebugMessageCallback as if `callback` were a plain scalar.
        registry = load_registry(Api.GL)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(
            command for command in raw_commands
            if command['command_attribs']['name'] == 'glDebugMessageCallback'
        )
        command = get_commands_from_xml_data({Api.GL: [raw]})[Api.GL][0]
        callback_param = next(p for p in command.params if p.name == 'callback')

        self.assertEqual(callback_param.type_facts, TypeFacts(base_type='GLDEBUGPROC', pointer_depth=0))
        self.assertFalse(callback_param.type_facts.is_scalar)

    def test_is_scalar_accepts_plain_gl_value_types(self) -> None:
        for base_type in ('GLenum', 'GLfloat', 'GLuint', 'GLsizei', 'void'):
            with self.subTest(base_type=base_type):
                self.assertTrue(TypeFacts(base_type=base_type, pointer_depth=0).is_scalar)

    def test_is_scalar_rejects_real_pointers(self) -> None:
        self.assertFalse(TypeFacts(base_type='GLenum', pointer_depth=1).is_scalar)
        self.assertFalse(TypeFacts(base_type='GLsync', pointer_depth=0).is_scalar)
        self.assertFalse(TypeFacts(base_type='void', pointer_depth=1).is_scalar)

    def test_is_scalar_accepts_wgl_win32_value_types(self) -> None:
        for base_type in ('BOOL', 'INT', 'INT32', 'INT64', 'UINT', 'DWORD', 'FLOAT', 'VOID'):
            with self.subTest(base_type=base_type):
                self.assertTrue(TypeFacts(base_type=base_type, pointer_depth=0).is_scalar)

    def test_is_scalar_rejects_wgl_handle_typedefs(self) -> None:
        # HDC/HGLRC/the DECLARE_HANDLE()-based types and PROC are all pointer typedefs (see
        # glTypes.h) with zero '*' in wgl.xml's own declared type text, same shape of trap as
        # GLDEBUGPROC for GL.
        for base_type in ('HDC', 'HGLRC', 'HANDLE', 'PROC', 'LPVOID', 'LPCSTR', 'HPBUFFERARB'):
            with self.subTest(base_type=base_type):
                self.assertFalse(TypeFacts(base_type=base_type, pointer_depth=0).is_scalar)

    def test_is_scalar_accepts_glx_x11_resource_ids(self) -> None:
        # GLXContextID/GLXDrawable/Font are all `typedef XID ...`, and XID itself is
        # `unsigned long` on X11 - a resource number, not a pointer.
        for base_type in ('int', 'unsigned int', 'Bool', 'GLXContextID', 'GLXDrawable', 'Font'):
            with self.subTest(base_type=base_type):
                self.assertTrue(TypeFacts(base_type=base_type, pointer_depth=0).is_scalar)

    def test_is_scalar_rejects_glx_context_pointer(self) -> None:
        # GLXContext is `typedef struct __GLXcontextRec *GLXContext`, unlike its
        # XID-based siblings above.
        self.assertFalse(TypeFacts(base_type='GLXContext', pointer_depth=0).is_scalar)

    def test_is_scalar_accepts_egl_value_types(self) -> None:
        for base_type in ('EGLBoolean', 'EGLenum', 'EGLint', 'EGLAttrib', 'EGLAttribKHR',
                           'EGLTime', 'EGLTimeKHR', 'EGLTimeNV', 'EGLuint64NV', 'EGLuint64KHR',
                           'EGLnsecsANDROID', 'EGLNativeFileDescriptorKHR'):
            with self.subTest(base_type=base_type):
                self.assertTrue(TypeFacts(base_type=base_type, pointer_depth=0).is_scalar)

    def test_is_scalar_rejects_egl_handle_typedefs(self) -> None:
        # All `typedef void *...` in egl.xml's <types> block, despite zero '*' of their own.
        for base_type in ('EGLDisplay', 'EGLContext', 'EGLSurface', 'EGLConfig', 'EGLImage',
                           'EGLSyncKHR', 'EGLStreamKHR', 'EGLClientBuffer'):
            with self.subTest(base_type=base_type):
                self.assertFalse(TypeFacts(base_type=base_type, pointer_depth=0).is_scalar)

    def test_enabled_gate_restricted_to_intercepted_apis(self) -> None:
        # Zero-pointer commands from GL/WGL (the only APIs the Windows interceptor forwards
        # today - see _INTERCEPTED_APIS) become enabled; equally scalar-shaped commands from
        # GLX/EGL do not, since nothing would ever construct their typed Command yet.
        cases = (
            (Api.WGL, 'wglSwapIntervalEXT', True),
            (Api.GLX, 'glXSwapIntervalSGI', True),
            (Api.EGL, 'eglGetError', True),
        )
        for api, command_name, expected_is_scalar in cases:
            with self.subTest(api=api, command=command_name):
                registry = load_registry(api)
                raw_commands = extract_commands_from_registry(registry)
                raw = next(
                    command for command in raw_commands
                    if command['command_attribs']['name'] == command_name
                )
                command = get_commands_from_xml_data({api: [raw]})[api][0]

                is_scalar = (command.return_value.type_facts.is_scalar
                             and all(p.type_facts.is_scalar for p in command.params))
                self.assertEqual(is_scalar, expected_is_scalar)
                self.assertEqual(command.enabled, expected_is_scalar and api == Api.WGL)

    def test_is_opaque_handle_accepts_hdc(self) -> None:
        facts = TypeFacts(base_type='HDC', pointer_depth=0)
        self.assertTrue(facts.is_opaque_handle)
        # Deliberately not scalar - kept as a separate classification (see
        # _OPAQUE_HANDLE_BASE_TYPES): capturable, but not replay-safe like a real scalar.
        self.assertFalse(facts.is_scalar)
        # And not a mapped handle either - HGLRC is (see test_is_mapped_handle_accepts_hglrc),
        # HDC deliberately isn't, since GITS never intercepts its creation.
        self.assertFalse(facts.is_mapped_handle)

    def test_is_opaque_handle_rejects_pointers_and_other_handles(self) -> None:
        # A real pointer-to-HDC (e.g. an out-parameter) is not itself an opaque handle value.
        self.assertFalse(TypeFacts(base_type='HDC', pointer_depth=1).is_opaque_handle)
        # HGLRC moved to is_mapped_handle (see below) - it is no longer is_opaque_handle.
        self.assertFalse(TypeFacts(base_type='HGLRC', pointer_depth=0).is_opaque_handle)
        # Only HDC is covered for now - other Win32 handle types stay excluded until a
        # specific command needs them (see _OPAQUE_HANDLE_BASE_TYPES).
        for base_type in ('HANDLE', 'PROC', 'HPBUFFERARB', 'GLXContext', 'EGLDisplay'):
            with self.subTest(base_type=base_type):
                self.assertFalse(TypeFacts(base_type=base_type, pointer_depth=0).is_opaque_handle)
        # A plain scalar is not an opaque handle either - the classifications don't overlap.
        self.assertFalse(TypeFacts(base_type='GLenum', pointer_depth=0).is_opaque_handle)

    def test_is_mapped_handle_accepts_hglrc(self) -> None:
        facts = TypeFacts(base_type='HGLRC', pointer_depth=0)
        self.assertTrue(facts.is_mapped_handle)
        self.assertFalse(facts.is_scalar)
        self.assertFalse(facts.is_opaque_handle)

    def test_is_mapped_handle_accepts_glsync(self) -> None:
        # GLsync is the second _MAPPED_HANDLE_BASE_TYPES entry (see that set's own comment for
        # why it earns the same real GITSKey mapping HGLRC does, via SyncHandleMapService
        # instead of ContextHandleMapService) - structurally identical to HGLRC: an opaque,
        # GITS-owned-lifecycle pointer typedef with zero '*' of its own in gl.xml.
        facts = TypeFacts(base_type='GLsync', pointer_depth=0)
        self.assertTrue(facts.is_mapped_handle)
        self.assertFalse(facts.is_scalar)
        self.assertFalse(facts.is_opaque_handle)

    def test_is_mapped_handle_rejects_pointers_and_other_types(self) -> None:
        self.assertFalse(TypeFacts(base_type='HGLRC', pointer_depth=1).is_mapped_handle)
        self.assertFalse(TypeFacts(base_type='GLsync', pointer_depth=1).is_mapped_handle)
        for base_type in ('HDC', 'HANDLE', 'GLenum', 'GLXContext', 'EGLContext'):
            with self.subTest(base_type=base_type):
                self.assertFalse(TypeFacts(base_type=base_type, pointer_depth=0).is_mapped_handle)

    def test_is_array_element_eligible_accepts_pointer_to_scalar_base_types(self) -> None:
        for base_type in ('GLuint', 'GLfloat', 'GLchar', 'void', 'int'):
            with self.subTest(base_type=base_type):
                self.assertTrue(
                    TypeFacts(base_type=base_type, pointer_depth=1).is_array_element_eligible
                )

    def test_is_array_element_eligible_rejects_non_pointer_depth_one(self) -> None:
        # pointer_depth == 0: this is a plain scalar, not a pointer to one - is_scalar covers
        # it instead (the two properties are mutually exclusive).
        self.assertFalse(TypeFacts(base_type='GLuint', pointer_depth=0).is_array_element_eligible)
        # pointer_depth == 2: double indirection (e.g. glShaderSource's `const GLchar* const*`)
        # is explicitly deferred (see the scale_pointer plan's Decision 1) - not a shape this
        # increment's ArrayArgument<T> (single indirection only) can represent.
        self.assertFalse(TypeFacts(base_type='GLchar', pointer_depth=2).is_array_element_eligible)

    def test_is_array_element_eligible_rejects_non_scalar_bases(self) -> None:
        # A pointer to a non-scalar base (an opaque handle or an already-excluded pointer
        # typedef) is not memcpy-safe purely from its element type - see
        # is_array_element_eligible's docstring. GLsync/HGLRC*/HDC* all stay excluded.
        # PIXELFORMATDESCRIPTOR* is deliberately *not* in this list - see the dedicated
        # acceptance test below (opengl2-player-skeleton-plan.md's Decision 9).
        for base_type in ('GLsync', 'HGLRC', 'HDC'):
            with self.subTest(base_type=base_type):
                self.assertFalse(
                    TypeFacts(base_type=base_type, pointer_depth=1).is_array_element_eligible
                )

    def test_is_array_element_eligible_accepts_pointer_to_fixed_size_struct(self) -> None:
        # PIXELFORMATDESCRIPTOR is a fixed-size POD struct (not a scalar - pointer_depth == 0
        # never applies to it in this backend's command set, so it does not join is_scalar's
        # own allowlist), but ArrayArgument<T>/argumentCoders.h's memcpy-based coders are fully
        # generic over T - see intermediates.py's _FIXED_SIZE_STRUCT_BASE_TYPES and
        # docs/development/opengl2-player-skeleton-plan.md's Decision 9
        # (SetPixelFormat/wglSetPixelFormat's `ppfd`).
        self.assertTrue(
            TypeFacts(base_type='PIXELFORMATDESCRIPTOR', pointer_depth=1).is_array_element_eligible
        )
        self.assertFalse(
            TypeFacts(base_type='PIXELFORMATDESCRIPTOR', pointer_depth=0).is_scalar
        )

    def test_is_array_element_eligible_mutually_exclusive_with_other_classifications(self) -> None:
        facts = TypeFacts(base_type='GLuint', pointer_depth=1)
        self.assertTrue(facts.is_array_element_eligible)
        self.assertFalse(facts.is_scalar)
        self.assertFalse(facts.is_opaque_handle)
        self.assertFalse(facts.is_mapped_handle)

    def test_is_array_of_arrays_eligible_accepts_pointer_depth_two_scalar_bases(self) -> None:
        # glShaderSource's `const GLchar *const *` - see array_of_arrays_metadata.py.
        for base_type in ('GLchar', 'GLuint', 'GLfloat', 'int'):
            with self.subTest(base_type=base_type):
                self.assertTrue(
                    TypeFacts(base_type=base_type, pointer_depth=2).is_array_of_arrays_eligible
                )

    def test_is_array_of_arrays_eligible_rejects_other_pointer_depths(self) -> None:
        self.assertFalse(TypeFacts(base_type='GLchar', pointer_depth=0).is_array_of_arrays_eligible)
        self.assertFalse(TypeFacts(base_type='GLchar', pointer_depth=1).is_array_of_arrays_eligible)
        self.assertFalse(TypeFacts(base_type='GLchar', pointer_depth=3).is_array_of_arrays_eligible)

    def test_is_array_of_arrays_eligible_rejects_non_scalar_bases(self) -> None:
        for base_type in ('GLsync', 'HGLRC', 'HDC'):
            with self.subTest(base_type=base_type):
                self.assertFalse(
                    TypeFacts(base_type=base_type, pointer_depth=2).is_array_of_arrays_eligible
                )

    def test_is_array_of_arrays_eligible_mutually_exclusive_with_is_array_element_eligible(self) -> None:
        facts = TypeFacts(base_type='GLchar', pointer_depth=2)
        self.assertTrue(facts.is_array_of_arrays_eligible)
        self.assertFalse(facts.is_array_element_eligible)
        self.assertFalse(facts.is_scalar)

    def test_enabled_gate_accepts_wgl_context_lifecycle_commands(self) -> None:
        # These are exactly the commands "Model context topology before broad object state"
        # names as the first state service to build - previously blocked by HDC/HGLRC being
        # pointer typedefs, now unblocked via is_opaque_handle (HDC)/is_mapped_handle (HGLRC)
        # without any pointer/array coder support.
        enabled_cases = (
            'wglCreateContext',        # HDC -> HGLRC
            'wglMakeCurrent',          # (HDC, HGLRC) -> BOOL
            'wglDeleteContext',        # HGLRC -> BOOL
            'wglGetCurrentContext',    # () -> HGLRC
            'wglGetCurrentDC',         # () -> HDC
            'wglShareLists',           # (HGLRC, HGLRC) -> BOOL
            # (HDC, HGLRC, const int *attribList) - attribList now has a NULL_TERMINATED
            # POINTER_PARAM_OVERRIDES entry (see pointer_metadata.py), so this moved here from
            # disabled_cases below once array/pointer coder support (scale_pointer) landed.
            'wglCreateContextAttribsARB',
        )
        # Still disabled: has a genuine pointer-to-data parameter that neither opaque-handle
        # nor array/pointer coder support addresses (a PIXELFORMATDESCRIPTOR struct pointer -
        # not a memcpy-safe scalar element type, so is_array_element_eligible is false and no
        # POINTER_PARAM_OVERRIDES entry could apply regardless).
        disabled_cases = (
            'ChoosePixelFormat',           # (HDC, const PIXELFORMATDESCRIPTOR *)
        )

        registry = load_registry(Api.WGL)
        raw_commands = extract_commands_from_registry(registry)
        raw_by_name = {raw['command_attribs']['name']: raw for raw in raw_commands}

        for command_name in enabled_cases:
            with self.subTest(command=command_name, expected_enabled=True):
                command = get_commands_from_xml_data({Api.WGL: [raw_by_name[command_name]]})[Api.WGL][0]
                self.assertTrue(command.enabled)

        for command_name in disabled_cases:
            with self.subTest(command=command_name, expected_enabled=False):
                command = get_commands_from_xml_data({Api.WGL: [raw_by_name[command_name]]})[Api.WGL][0]
                self.assertFalse(command.enabled)

    def test_synthetic_wgl_parameters(self) -> None:
        command = _make_wgl_swap_multiple_buffers()

        self.assertEqual(
            [(param.name, param.type_facts) for param in command.params],
            [
                ('n', TypeFacts(base_type='UINT', pointer_depth=0)),
                ('pBuffers', TypeFacts(base_type='WGLSWAP', pointer_depth=1)),
            ],
        )
        self.assertEqual(command.params[1].type, 'const WGLSWAP*')

if __name__ == '__main__':
    unittest.main()