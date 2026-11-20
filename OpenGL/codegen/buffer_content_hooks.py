#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Commands that change, or may let the GPU change, a buffer's contents. The wrapper reports each
# call, enabled or not, to BufferShadowService, which keeps recorder-side copies of element
# buffer contents so draws never read indices back from the driver; see
# docs/development/modern-opengl-backend-architecture.md's client-memory vertex array decision.
#
# Families are matched by name and by the parameter names each needs, so a command with an
# unexpected signature is left unhooked instead of mis-hooked. Anything that lets the GPU (or
# external memory) write a buffer marks it permanently unknown, so a copy is never trusted after
# a write the recorder cannot see.

import re
from dataclasses import dataclass

from intermediates import Command


@dataclass(frozen=True)
class BufferContentHook:
    """`call` is the BufferShadowService member call, over the command's parameters.

    Calls that can let the GPU write a buffer the application holds persistently mapped take the
    command name first, so the recorder can name the command it rejects.

    `guard`, when set, is a C++ condition the call runs under: sizes and offsets are signed
    (GLsizeiptr, GLintptr), and a negative one is a call the driver rejects without touching the
    buffer, which must not reach the unsigned `call` arguments.

    `before_call` runs the call before the command publishes mapped writes and reserves its key,
    for commands that have the GPU write the buffer themselves: starting to track the GPU writes
    of a persistent mapping publishes the pages written meanwhile, whole, which must replay before
    the GPU write and not overwrite it. Hooks of commands that only bind a buffer run after the
    call, as the GPU writes it through the binding only in later commands.

    `prepare`, when set, is another BufferShadowService member call, under `guard`, made before
    the command publishes mapped writes and reserves its key, while `call` still runs after the
    call.
    """
    call: str
    guard: str | None = None
    before_call: bool = False
    prepare: str | None = None


def _command(command: str) -> str:
    return f'"{command}"'


def _ref(names: set[str], target: str, buffer: str) -> str | None:
    if target in names:
        return f'BufferRef::ByTarget({target})'
    if buffer in names:
        return f'BufferRef::ByName({buffer})'
    return None


def _size(expr: str) -> str:
    return f'static_cast<uint64_t>({expr})'


def _non_negative(*exprs: str) -> str:
    return ' && '.join(f'{expr} >= 0' for expr in exprs)


def _data(command: str, names: set[str]) -> BufferContentHook | None:
    ref = _ref(names, 'target', 'buffer')
    if ref is None or not {'size', 'data'} <= names:
        return None
    return BufferContentHook(call=f'OnData({_command(command)}, {ref}, {_size("size")}, data)',
                             guard=_non_negative('size'))


def _sub_data(command: str, names: set[str]) -> BufferContentHook | None:
    ref = _ref(names, 'target', 'buffer')
    if ref is None or not {'offset', 'size', 'data'} <= names:
        return None
    arguments = f'{ref}, {_size("offset")}, {_size("size")}, data'
    return BufferContentHook(call=f'OnSubData({arguments})', guard=_non_negative('offset', 'size'),
                             prepare=f'BeforeSubData({arguments})')


def _gpu_write(target: str, buffer: str, hook: str = 'OnGpuWrite'):
    def build(command: str, names: set[str]) -> str | None:
        ref = _ref(names, target, buffer)
        return None if ref is None else f'{hook}({_command(command)}, {ref})'
    return build


def _before_call(build):
    def wrapped(command: str, names: set[str]) -> BufferContentHook | None:
        call = build(command, names)
        return None if call is None else BufferContentHook(call=call, before_call=True)
    return wrapped


def _undefined(command: str, names: set[str]) -> str | None:
    ref = _ref(names, 'target', 'buffer')
    return None if ref is None else f'OnUndefined({ref})'


def _bind(command: str, names: set[str]) -> str | None:
    if not {'target', 'buffer'} <= names:
        return None
    return f'OnBind({_command(command)}, target, buffer)'


def _bind_indexed(command: str, names: set[str]) -> str | None:
    if not {'target', 'buffer'} <= names:
        return None
    return f'OnBindIndexed({_command(command)}, target, buffer)'


def _bind_indexed_multi(command: str, names: set[str]) -> str | None:
    if not {'target', 'count', 'buffers'} <= names:
        return None
    return f'OnBindIndexed({_command(command)}, target, count, buffers)'


def _delete(command: str, names: set[str]) -> str | None:
    return 'OnDelete(n, buffers)' if {'n', 'buffers'} <= names else None


_FAMILIES = (
    (re.compile(r'^gl(Named)?Buffer(Data|Storage)(ARB|EXT)?$'), _data),
    (re.compile(r'^gl(Named)?BufferSubData(ARB|EXT)?$'), _sub_data),
    (re.compile(r'^glCopyBufferSubData$'), _gpu_write('writeTarget', 'writeBuffer')),
    (re.compile(r'^gl(CopyNamed|NamedCopy)BufferSubData(EXT)?$'),
     _gpu_write('writeTarget', 'writeBuffer')),
    (re.compile(r'^glClear(Named)?Buffer(Sub)?Data(EXT)?$'), _gpu_write('target', 'buffer')),
    # After these, the GPU or external memory may write the buffer in commands that are not
    # hooked: image stores to a buffer texture, external memory, and shaders through a resident
    # address.
    (re.compile(r'^gl(Tex|Texture|MultiTex)Buffer(Range)?(ARB|EXT|OES)?$'),
     _gpu_write('', 'buffer', 'OnUnhookedGpuWriter')),
    # Transform feedback and query results reach persistent mappings like shader writes do.
    (re.compile(r'^glTransformFeedbackBuffer(Base|Range)$'),
     _gpu_write('', 'buffer', 'OnGpuResultWrite')),
    (re.compile(r'^glGetQueryBufferObject'),
     _before_call(_gpu_write('', 'buffer', 'OnGpuResultWrite'))),
    (re.compile(r'^gl(Named)?BufferStorage(Mem|External)EXT$'),
     _gpu_write('target', 'buffer', 'OnUnhookedGpuWriter')),
    (re.compile(r'^glMake(Named)?BufferResidentNV$'),
     _gpu_write('target', 'buffer', 'OnUnhookedGpuWriter')),
    (re.compile(r'^gl(Named)?BufferPageCommitment(ARB|EXT|MemNV)?$'), _undefined),
    (re.compile(r'^glInvalidateBuffer(Sub)?Data$'), _undefined),
    # Maps MapTrackingService tracks have custom wrappers (mapTrackingWrappers.cpp) that update
    # the copies themselves; these generated wrappers only run for the untracked ones.
    (re.compile(r'^glMap(Named)?Buffer(Range)?(ARB|EXT|OES)?$'), _undefined),
    (re.compile(r'^glBindBuffer(ARB)?$'), _bind),
    (re.compile(r'^glBindBuffer(Base|Range|Offset)(EXT|NV)?$'), _bind_indexed),
    (re.compile(r'^glBindBuffers(Base|Range)$'), _bind_indexed_multi),
    (re.compile(r'^glDeleteBuffers(ARB)?$'), _delete),
)


def buffer_content_hook_for(cmd: Command) -> BufferContentHook | None:
    names = {p.name for p in cmd.params}
    for pattern, build in _FAMILIES:
        if pattern.match(cmd.name):
            hook = build(cmd.name, names)
            if isinstance(hook, str):
                hook = BufferContentHook(call=hook)
            return hook
    return None


def buffer_content_hooks(commands: list[Command]) -> dict[str, BufferContentHook]:
    """Every hooked command among `commands`, by name, enabled or not."""
    hooks = {}
    for cmd in commands:
        hook = buffer_content_hook_for(cmd)
        if hook is not None:
            hooks[cmd.name] = hook
    return hooks
