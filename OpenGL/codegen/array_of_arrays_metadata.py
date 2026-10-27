#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Small, hand-maintained table classifying doubly-indirect (`const T* const*`) pointer
# parameters - precedent: pointer_metadata.py's POINTER_PARAM_OVERRIDES, handle_lifecycle.py's
# HANDLE_CREATING_COMMANDS. See docs/development/opengl-rendering-pipeline-pointer-support-plan.md
# for the full design rationale (the GL-flavored analogue of Vulkan's ArrayOfArrays<T>,
# Vulkan/common/layer_interface/arguments.h).
#
# A doubly-indirect array (gl.xml's only example is glShaderSource's `string`) always comes
# with two *sibling* parameters that give it shape: an outer count (a plain by-value integer,
# exactly like pointer_metadata.py's SIBLING_PARAM) and a per-element length array (itself a
# single-indirection pointer parameter - glShaderSource's `length`). That second sibling is
# fully absorbed into the doubly-indirect parameter's own Command member (arguments.h's
# ArrayOfArrays<T> stores each element's resolved length itself - see that type's comment) and
# therefore gets **no Command member of its own** - see consumed_pointer_params()/
# consumed_param_owner() below, used by intermediates_creator.py's _is_param_capturable (so the
# consumed parameter doesn't fail the enabled gate for lack of its own override) and by
# commandsAuto.h.mako/commandRunnersAuto.cpp.mako (so no member is declared/read for it).

from dataclasses import dataclass


@dataclass(frozen=True, kw_only=True)
class ArrayOfArraysOverride:
    """Classifies a `const T* const*`-shaped parameter (TypeFacts.is_array_of_arrays_eligible)
    together with the two sibling parameters that give it shape.

    outer_count_param: name of the plain by-value integer parameter giving the outer array's
        element count (e.g. glShaderSource's `count`) - same role as pointer_metadata.py's
        SIBLING_PARAM length_source_value, just spelled out explicitly here since this
        override's own dataclass has no length_source_kind field to hang it off of.
    element_length_param: name of the sibling single-indirection pointer parameter giving each
        element's length (e.g. glShaderSource's `length`) - absorbed into this parameter's own
        wrapper (arguments.h's ArrayOfArrays<T>) and given no Command member of its own; see
        consumed_pointer_params() below. None when gl.xml genuinely gives the doubly-indirect
        parameter no length-array sibling at all (glCreateShaderProgramv's `strings`,
        glTransformFeedbackVaryings' `varyings` - both take only an outer `count`, no `length`
        parameter exists anywhere in either command's own signature) - the real driver call
        itself has no lengths to pass either in that case, so arguments.h's ArrayOfArrays<T>
        constructor falls back to scanning each element for its own null terminator instead of
        trusting a per-element length (see that constructor's own comment) rather than this
        being a gap in what this backend tracks.
    """
    outer_count_param: str
    element_length_param: str | None = None


# (command_name, param_name) -> classification, param_name always the *doubly*-indirect
# parameter (never the absorbed element_length_param sibling - that one is looked up via
# consumed_pointer_params()/consumed_param_owner(), not this dict, directly).
ARRAY_OF_ARRAYS_PARAM_OVERRIDES: dict[tuple[str, str], ArrayOfArraysOverride] = {
    ('glShaderSource', 'string'): ArrayOfArraysOverride(
        outer_count_param='count', element_length_param='length'),
    ('glShaderSourceARB', 'string'): ArrayOfArraysOverride(
        outer_count_param='count', element_length_param='length'),

    # glCreateShaderProgramv(type, count, strings)/glTransformFeedbackVaryings(program, count,
    # varyings, bufferMode): both `strings`/`varyings` are `const GLchar *const*` with
    # len="count" - same doubly-indirect shape as glShaderSource's `string`, but gl.xml gives
    # neither command an element-length-array sibling at all (verified: each has only 3-4
    # params total, none named `length`) - element_length_param stays None; see that field's
    # own comment above for why ArrayOfArrays<T> still handles this correctly (null-terminated
    # element scan).
    ('glCreateShaderProgramv', 'strings'): ArrayOfArraysOverride(outer_count_param='count'),
    ('glTransformFeedbackVaryings', 'varyings'): ArrayOfArraysOverride(outer_count_param='count'),

    # Same null-terminated string array shape as glCreateShaderProgramv's `strings`.
    ('glGetUniformIndices', 'uniformNames'): ArrayOfArraysOverride(outer_count_param='uniformCount'),
}


def array_of_arrays_override_for(command_name: str, param_name: str) -> ArrayOfArraysOverride | None:
    """Thin wrapper around ARRAY_OF_ARRAYS_PARAM_OVERRIDES.get(...), for use directly from
    Mako templates and intermediates_creator.py - mirrors pointer_metadata.py's
    pointer_override_for()/object_name_lifecycle.py's object_name_override_for() naming."""
    return ARRAY_OF_ARRAYS_PARAM_OVERRIDES.get((command_name, param_name))


def consumed_pointer_params(command_name: str) -> frozenset[str]:
    """Parameter names on `command_name` that are absorbed into a sibling parameter's
    ArrayOfArrays<T> wrapper and therefore get no Command member of their own, even though
    they are themselves TypeFacts.is_array_element_eligible pointers that would otherwise need
    their own POINTER_PARAM_OVERRIDES entry to be considered capturable - see
    intermediates_creator.py's _is_param_capturable. Omits entries whose element_length_param
    is None (glCreateShaderProgramv/glTransformFeedbackVaryings above) - there is no sibling
    parameter to exclude from Command members in that case; the doubly-indirect parameter
    itself absorbs nothing."""
    return frozenset(
        override.element_length_param
        for (cmd, _param_name), override in ARRAY_OF_ARRAYS_PARAM_OVERRIDES.items()
        if cmd == command_name and override.element_length_param is not None
    )


def consumed_param_owner(command_name: str, param_name: str) -> str | None:
    """The doubly-indirect parameter name that absorbed `param_name` (e.g.
    consumed_param_owner('glShaderSource', 'length') == 'string'), or None if `param_name`
    isn't consumed on this command. Used by commandRunnersAuto.cpp.mako to find, at replay
    time, which ArrayOfArrays<T> member actually holds the per-element lengths this call
    argument needs (there is no `command.m_length` - see consumed_pointer_params())."""
    for (cmd, owner_name), override in ARRAY_OF_ARRAYS_PARAM_OVERRIDES.items():
        if cmd == command_name and override.element_length_param == param_name:
            return owner_name
    return None
