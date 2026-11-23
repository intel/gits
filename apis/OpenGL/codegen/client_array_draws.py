#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Draw commands that can read vertex arrays from client memory, and which of their parameters
# bound the vertices read. ClientArraysLayer (clientArraysLayerAuto.cpp.mako) hooks each one and
# records a ClientArraysMetaCommand ahead of it; see
# docs/development/modern-opengl-backend-architecture.md's client-memory vertex array decision.
#
# Classified by parameter names, not by a hand list: every non-indirect GL draw uses the same
# names for the same meaning - `first` (`start` in glDrawArraysInstancedEXT)/`count` for array
# draws, `count`/`type`/`indices` for element draws, plus optional `basevertex`,
# `instancecount`/`primcount` and `baseinstance`. glMultiDraw* take the same names as arrays of
# `drawcount` (`primcount` in the EXT forms) sub-draws, with only `basevertex` optional. A
# command with any other parameter is not matched, so a new shape is never silently
# mis-bounded.

import re
from dataclasses import dataclass

from intermediates import Command

_DRAW_NAME_RE = re.compile(r'^gl(Multi)?Draw(?:Range)?(?:Arrays|Elements)(?!Indirect)')

_FIRST_PARAMS = ('first', 'start')
_ELEMENTS_PARAMS = frozenset({'mode', 'count', 'type', 'indices'})
_RANGE_PARAMS = frozenset({'start', 'end'})
_INSTANCE_COUNT_PARAMS = ('instancecount', 'primcount')
_OPTIONAL_PARAMS = frozenset({'basevertex', 'baseinstance', *_INSTANCE_COUNT_PARAMS})
_DRAW_COUNT_PARAMS = ('drawcount', 'primcount')
_MULTI_OPTIONAL_PARAMS = frozenset({'basevertex'})


@dataclass(frozen=True)
class ClientArrayDraw:
    """Parameter names of one hooked draw; None where the draw has no such parameter.
    `draw_count_param` is set only for multi-draws, whose count/first/indices/basevertex
    parameters are arrays of that many sub-draws."""
    is_elements: bool
    first_param: str | None
    base_vertex_param: str | None
    instance_count_param: str | None
    base_instance_param: str | None
    draw_count_param: str | None = None


def client_array_draw_for(cmd: Command) -> ClientArrayDraw | None:
    match = _DRAW_NAME_RE.match(cmd.name)
    if not match:
        return None
    params = {p.name: p for p in cmd.params}
    names = set(params)
    draw_count_param = None
    if match.group(1):
        draw_count_param = next((n for n in _DRAW_COUNT_PARAMS if n in names), None)
        if draw_count_param is None or params['count'].type_facts.pointer_depth != 1:
            return None
        names.discard(draw_count_param)
        optional = _MULTI_OPTIONAL_PARAMS
    else:
        optional = _OPTIONAL_PARAMS
    first_param = None
    if _ELEMENTS_PARAMS <= names:
        is_elements = True
        required = _ELEMENTS_PARAMS | (_RANGE_PARAMS & names)
    else:
        first_param = next((n for n in _FIRST_PARAMS if n in names), None)
        if first_param is None or not {'mode', 'count'} <= names:
            return None
        is_elements = False
        required = {'mode', 'count', first_param}
    if not names - required <= optional:
        return None
    if sum(name in names for name in _INSTANCE_COUNT_PARAMS) > 1:
        return None
    return ClientArrayDraw(
        is_elements=is_elements,
        first_param=first_param,
        base_vertex_param='basevertex' if 'basevertex' in names else None,
        instance_count_param=next((n for n in _INSTANCE_COUNT_PARAMS if n in names), None),
        base_instance_param='baseinstance' if 'baseinstance' in names else None,
        draw_count_param=draw_count_param,
    )


# Commands that set an array pointer ClientArraysService may later capture. The wrapper reports
# each call, enabled or not, so a draw only queries array slots ever set from client memory (see
# ClientArraysService::NoteArrayPointer). Setters for arrays the service never captures (NV
# vertex programs, IBM lists, tangents, weights, ...) are not listed.
_FIXED_SETTER_RE = re.compile(
    r'^gl(Vertex|Normal|Color|SecondaryColor|FogCoord|Index|EdgeFlag|TexCoord)Pointer(?:EXT)?$')
_ATTRIB_SETTER_RE = re.compile(r'^glVertexAttrib([IL]?)Pointer(?:ARB|EXT)?$')
_ATTRIB_SETTER_FLAGS = {
    '': '0',
    'I': 'CLIENT_ARRAY_FLAG_INTEGER',
    'L': 'CLIENT_ARRAY_FLAG_LONG',
}


@dataclass(frozen=True)
class ClientArraySetter:
    """How a setter call maps to array slots: `kind` is a ClientArrayKind enumerator, or
    'Interleaved' for glInterleavedArrays. `slot_expr` is a C++ expression over the command's
    parameters, or '-1' for the current client active texture unit. `flags` is the
    CLIENT_ARRAY_FLAG_* the setter itself implies (the I and L attribute setters), for contexts
    whose array state cannot be queried for it."""
    kind: str
    slot_expr: str
    flags: str = '0'


def client_array_setter_for(cmd: Command) -> ClientArraySetter | None:
    names = {p.name for p in cmd.params}
    if 'pointer' not in names:
        return None
    fixed = _FIXED_SETTER_RE.match(cmd.name)
    if fixed:
        slot_expr = '-1' if fixed.group(1) == 'TexCoord' else '0'
        return ClientArraySetter(kind=fixed.group(1), slot_expr=slot_expr)
    attrib = _ATTRIB_SETTER_RE.match(cmd.name)
    if attrib and 'index' in names:
        return ClientArraySetter(kind='VertexAttrib', slot_expr='static_cast<int64_t>(index)',
                                 flags=_ATTRIB_SETTER_FLAGS[attrib.group(1)])
    if cmd.name == 'glMultiTexCoordPointerEXT' and 'texunit' in names:
        return ClientArraySetter(
            kind='TexCoord', slot_expr='static_cast<int64_t>(texunit) - GL_TEXTURE0')
    if cmd.name == 'glInterleavedArrays':
        return ClientArraySetter(kind='Interleaved', slot_expr='-1')
    return None


def client_array_setters(commands: list[Command]) -> dict[str, ClientArraySetter]:
    """Every setter among `commands`, by name, enabled or not."""
    setters = {}
    for cmd in commands:
        setter = client_array_setter_for(cmd)
        if setter is not None:
            setters[cmd.name] = setter
    return setters


def client_array_draws(commands: list[Command]) -> dict[str, ClientArrayDraw]:
    """Hooked draws among `commands`, by name. Only enabled commands reach Pre layers."""
    draws = {}
    for cmd in commands:
        if not cmd.enabled:
            continue
        draw = client_array_draw_for(cmd)
        if draw is not None:
            draws[cmd.name] = draw
    return draws
