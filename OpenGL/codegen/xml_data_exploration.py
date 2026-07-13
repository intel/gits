# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# This file exists to research the OpenGL registry files (XML) and decide what
# kind of data structures and processing logic are needed in the metagenerator.

import sys
from collections import Counter, defaultdict
from dataclasses import dataclass

from generate_gl import strip_suffix
from generator import Api, EnumValue, Token

# ---------------------------------------------------------------------------
# Helpers for find_duplicate_enum_values
# ---------------------------------------------------------------------------

def _build_alias_clusters(enums: list[EnumValue]) -> list[list[EnumValue]]:
    """Group enums into clusters (keep aliases together with their targets)."""
    # Aliases in [gl|wgl|egl|glx].xml are one-directional (never mutual) and
    # don't form chains.
    clusters: dict[str, list[EnumValue]] = defaultdict(list)
    for e in enums:
        if e.alias:
            clusters[e.alias].append(e)
        else:
            clusters[e.name].append(e)
    return list(clusters.values())


def _canonical(cluster: list[EnumValue]) -> EnumValue:
    """Pick the non-alias member of a cluster as its representative, or the first."""
    return next((e for e in cluster if e.alias is None), cluster[0])


def _build_suffix_candidates(alias_clusters: list[list[EnumValue]]) -> list[list[EnumValue]]:
    """Merge alias clusters that share a generic (suffix-stripped) name."""
    merged: dict[str, list[EnumValue]] = defaultdict(list)
    for cluster in alias_clusters:
        merged[strip_suffix(_canonical(cluster).name)].extend(cluster)
    return list(merged.values())


# ---------------------------------------------------------------------------
# Result type
# ---------------------------------------------------------------------------

SENTINEL_GROUP = ''  # Used for enums that belong to no group.


@dataclass
class BucketAnalysis:
    """Analysis of a single (normalized_value, group) bucket with 2+ enums."""

    group: str                            # Group name, or SENTINEL_GROUP if none.
    normalized_value: int                 # Integer form of the value (hex/dec unified).
    raw_value: str                        # Original value string, for display.
    all_enums: list[EnumValue]            # All enums in this bucket.

    # Each inner list is one "name candidate" - enums considered the same thing.
    alias_candidates: list[list[EnumValue]]   # After alias-chain collapse.
    suffix_candidates: list[list[EnumValue]]  # After alias + suffix collapse.

    @property
    def resolved_by_alias(self) -> bool:
        """Alias collapse alone reduced this bucket to one candidate."""
        return len(self.alias_candidates) == 1

    @property
    def needs_suffix_collapse(self) -> bool:
        """Suffix collapse was needed on top of alias collapse."""
        return len(self.alias_candidates) > 1 and len(self.suffix_candidates) == 1

    @property
    def is_genuine_duplicate(self) -> bool:
        """Neither alias nor suffix collapse could resolve this bucket."""
        return len(self.suffix_candidates) > 1


# ---------------------------------------------------------------------------
# Core research function
# ---------------------------------------------------------------------------

def find_duplicate_enum_values(enums: list[EnumValue]) -> list[BucketAnalysis]:
    """Find every (value, group) bucket with 2+ enums and classify how/whether
    the ambiguity can be resolved by alias collapse and/or suffix collapse.
    """
    buckets: dict[tuple[int, str], list[EnumValue]] = defaultdict(list)

    for e in enums:
        if e.value.startswith('EGL_CAST'):
            continue  # Drop them, they won't be typed as GLenum anyway.
        elif e.value == '"GLX"':
            continue  # GLX registry XML admits this is abuse of the enum mechanism.

        try:
            norm = int(e.value, 0) # 0 means auto-detect base.
        except (ValueError, TypeError):
            sys.exit(f"Error: unexpected non-integer enum value {e.name} = {e.value}")

        if not e.groups:
            buckets[(norm, SENTINEL_GROUP)].append(e)
        else:
            for group in e.groups:
                buckets[(norm, group)].append(e)

    results: list[BucketAnalysis] = []
    for (norm_val, group), bucket_enums in buckets.items():
        if len(bucket_enums) < 2:
            continue

        alias_candidates = _build_alias_clusters(bucket_enums)
        suffix_candidates = _build_suffix_candidates(alias_candidates)

        results.append(BucketAnalysis(
            group=group,
            normalized_value=norm_val,
            raw_value=bucket_enums[0].value,
            all_enums=bucket_enums,
            alias_candidates=alias_candidates,
            suffix_candidates=suffix_candidates,
        ))

    return results


# ---------------------------------------------------------------------------
# Printer (thin wrapper for research output)
# ---------------------------------------------------------------------------

def print_duplicate_analysis(
    buckets: list[BucketAnalysis],
    *,
    show_alias_resolved: bool = False,
    show_suffix_resolved: bool = False,
    show_genuine: bool = True,
) -> None:
    """Print a summary and optionally detailed listings from find_duplicate_enum_values."""
    alias_resolved = [b for b in buckets if b.resolved_by_alias]
    suffix_needed  = [b for b in buckets if b.needs_suffix_collapse]
    genuine        = [b for b in buckets if b.is_genuine_duplicate]

    print('=== Duplicate Enum Value Analysis ===')
    print(f'  Total ambiguous buckets:      {len(buckets)}')
    print(f'  Resolved by alias alone:      {len(alias_resolved)}')
    print(f'  Needed suffix collapse too:   {len(suffix_needed)}')
    print(f'  Genuine duplicates remaining: {len(genuine)}')

    def _fmt_bucket(b: BucketAnalysis) -> str:
        group_label = b.group or '<no group>'
        names = ', '.join(e.name for e in b.all_enums)
        return f'  [{group_label}] {hex(b.normalized_value)}: {names}'

    if show_alias_resolved and alias_resolved:
        print('\n--- Resolved by alias ---')
        for b in alias_resolved:
            print(_fmt_bucket(b))

    if show_suffix_resolved and suffix_needed:
        print('\n--- Required suffix collapse (alias alone was insufficient) ---')
        for b in suffix_needed:
            print(_fmt_bucket(b))
            for cluster in b.alias_candidates:
                print(f'      alias cluster: [{", ".join(e.name for e in cluster)}]')

    if show_genuine and genuine:
        print('\n--- Genuine duplicates (unresolved by either strategy) ---')
        for b in genuine:
            print(_fmt_bucket(b))
            for cluster in b.suffix_candidates:
                print(f'      remaining candidate: [{", ".join(e.name for e in cluster)}]')


# ---------------------------------------------------------------------------
# Enum inspection
# ---------------------------------------------------------------------------

def inspect_enums(label: str, enums: list[EnumValue]) -> None:
    """Print stats and assumption checks for enums from a single XML file."""
    print(f'\n=== {label} ({len(enums)} enums) ===')

    # Show distinct types and their counts.
    type_counts = Counter(e.type.name for e in enums)
    print(f'  Types: {dict(type_counts)}')

    # Check the API attribute usage.
    with_api = [(e.name, e.api) for e in enums if e.api]
    if with_api:
        print(f'  Enums with api attribute ({len(with_api)}):')
        for name, api in with_api:
            print(f'    {name}: {api!r}')
    else:
        print('  No enums with api attribute.')

    # Check if mutual aliases exist.
    aliases = {e.name: e.alias for e in enums if e.alias}
    mutual = [(a, b) for a, b in aliases.items() if aliases.get(b) == a]
    print(f'  Mutual aliases: {len(mutual)}' + (f' {mutual}' if mutual else ''))

    # Check if alias chains exist.
    name_to_enum = {e.name: e for e in enums}
    chains = [e.name for e in enums
              if e.alias and e.alias in name_to_enum and name_to_enum[e.alias].alias]
    print(f'  Alias chains (enums whose alias target is itself an alias): {len(chains)}'
          + (f': {chains}' if chains else ''))

    # Check if unresolved aliases exist.
    name_set = {e.name for e in enums}
    unresolved = [(name, target) for name, target in aliases.items() if target not in name_set]
    if unresolved:
        print(f'  Aliases pointing outside this file ({len(unresolved)}):')
        for name, target in unresolved:
            print(f'    {name} -> {target}')
    else:
        print('  All aliases resolve within this file.')

    # Group statistics.
    groupless = sum(1 for e in enums if not e.groups)
    distinct_groups = {g for e in enums for g in e.groups}
    print(f'  Enums without groups: {groupless}, distinct groups: {len(distinct_groups)}')


def inspect_cross_file_enums(labeled_enums: dict[str, list[EnumValue]]) -> None:
    # TODO: adjust logic to match new input type (dict instead of list of tuples).
    """Print cross-file assumption checks for a collection of enum lists."""
    print('\n=== Cross-file enum inspection ===')

    # Check for same-named enums across files and whether their values match.
    by_name: dict[str, list[tuple[str, EnumValue]]] = defaultdict(list)
    for label, enums in labeled_enums.items():
        for e in enums:
            by_name[e.name].append((label, e))

    multi = {name: entries for name, entries in by_name.items() if len(entries) > 1}
    same_value = sum(1 for entries in multi.values() if len({int(e.value, 0) for _, e in entries}) == 1)
    conflicts  = [(name, entries) for name, entries in multi.items()
                  if len({int(e.value, 0) for _, e in entries}) > 1]

    print(f'  Names in multiple files: {len(multi)} ({same_value} same value, {len(conflicts)} conflicts)')
    for name, entries in conflicts:
        print(f'  CONFLICT {name}:')
        for label, e in entries:
            print(f'    [{label}] {e.value}')

    # Cross-file alias check omitted: per-registry inspect_enums already reports
    # any alias whose target is absent from the local file.


# ---------------------------------------------------------------------------
# Function (command) inspection helpers
# ---------------------------------------------------------------------------

# Function name prefixes used in each registry.
_API_PREFIX: dict[Api, str] = {
    Api.GL:  'gl',
    Api.WGL: 'wgl',
    Api.EGL: 'egl',
    Api.GLX: 'glX',  # This is why we can't just lowercase the Api.
}


def _classify_len(len_str: str, param_names: set[str]) -> str:
    """Categorize a len annotation.

    Categories: 'integer', 'param-name', 'COMPSIZE', 'other', 'empty'.
    """
    if not len_str:
        print('WARNING: empty `len`, indicating erroneous XML data')
        return 'empty'
    if len_str.isdigit():
        return 'integer'
    if len_str in param_names:
        return 'param-name'
    if len_str.startswith('COMPSIZE'):
        return 'COMPSIZE'
    return 'other'


# ---------------------------------------------------------------------------
# Function (command) inspection
# ---------------------------------------------------------------------------

def inspect_functions(label: str, tokens: list[Token]) -> None:
    """Print stats and assumption checks for functions from a single registry.

    Checks:
    - Alias structure: count, mutual aliases (expect 0), alias chains (expect 0),
      locally-unresolved aliases (targets not in this registry, expect 0).
    - Return-type integrity: tokens whose return_value.type is empty (expect 0).
    - Name prefix: each name should start with the registry's canonical prefix
      (gl/wgl/glX/egl). Violations are informational; WGL legitimately includes
      a handful of unprefixed GDI functions.
    """
    print(f'\n=== {label} ({len(tokens)} functions) ===')

    # Alias structure.
    name_to_token: dict[str, Token] = {t.name: t for t in tokens}
    with_alias = [t for t in tokens if t.alias is not None]
    print(f'  Functions with alias: {len(with_alias)}')

    mutual = [
        t.name for t in with_alias
        if t.alias in name_to_token and name_to_token[t.alias].alias == t.name
    ]
    print(f'  Mutual aliases: {len(mutual)}' + (f': {mutual}' if mutual else ''))

    chains = [
        t.name for t in with_alias
        if t.alias in name_to_token and name_to_token[t.alias].alias is not None
    ]
    print(f'  Alias chains (target is itself an alias): {len(chains)}'
          + (f': {chains}' if chains else ''))

    unresolved = [t.name for t in with_alias if t.alias not in name_to_token]
    if unresolved:
        print(f'  Aliases pointing outside this registry ({len(unresolved)}): {unresolved}')
    else:
        print('  All aliases resolve within this registry.')

    # Return-type integrity.
    empty_return = [t.name for t in tokens if not t.return_value.type]
    if empty_return:
        print(f'  WARNING: Empty return type ({len(empty_return)}): {empty_return}')
    else:
        print('  All return types present.')

    # Name prefix.
    # Note: WGL includes six legacy unprefixed GDI pixel-format functions
    # (e.g. ChoosePixelFormat), so violations aren't necessarily errors.
    expected_prefix = _API_PREFIX.get(Api(label), '')
    if expected_prefix:
        wrong_prefix = [t.name for t in tokens if not t.name.startswith(expected_prefix)]
        if wrong_prefix:
            print(f'  Names without expected prefix {expected_prefix!r} ({len(wrong_prefix)}): {wrong_prefix}')
        else:
            print(f'  All names have expected prefix {expected_prefix!r}.')


def inspect_function_params(label: str, tokens: list[Token]) -> None:
    """Inspect argument and annotation quality for functions from a single registry.

    Checks:
    - Signature integrity: warns on every argument with an empty name or type.
    - Annotation coverage: counts and distinct values for group, class_, kind
      across all arguments and return values.
    - len automatability: classifies non-empty len annotations into
      integer / param-name / COMPSIZE / other, with samples of 'other'.
    """
    print(f'\n=== {label} function parameters ===')

    total_args = sum(len(t.args) for t in tokens)
    print(f'  Total arguments: {total_args}')

    # Signature integrity: warn on every bad argument.
    for token in tokens:
        for arg in token.args:
            if not arg.name:
                print(f'  WARNING: {token.name}: argument has empty name (type={arg.type!r})')
            if not arg.type:
                print(f'  WARNING: {token.name}: argument {arg.name!r} has empty type')

    # Annotation coverage over return values and arguments.
    all_annotated = [token.return_value for token in tokens] + [
        arg for token in tokens for arg in token.args
    ]
    groups  = [a.group  for a in all_annotated if a.group  is not None]
    classes = [a.class_ for a in all_annotated if a.class_ is not None]
    kinds   = [k for a in all_annotated for k in a.kinds]
    print(f'  Annotation coverage (return values + arguments):')
    print(f'    group:  {len(groups)} present, {len(set(groups))} distinct')
    print(f'    class_: {len(classes)} present, {len(set(classes))} distinct')
    print(f'    kind:   {len(kinds)} present, {len(set(kinds))} distinct')

    # len automatability breakdown (arguments only).
    len_counts: Counter[str] = Counter()
    other_samples: list[str] = []
    for token in tokens:
        param_names = {arg.name for arg in token.args}
        for arg in token.args:
            if arg.len is not None:
                category = _classify_len(arg.len, param_names)
                len_counts[category] += 1
                if category == 'other' and len(other_samples) < 8:
                    other_samples.append(f'{token.name}: {arg.name} len={arg.len!r}')
    if len_counts:
        print(f'  len annotations: {dict(len_counts)}')
        if other_samples:
            print('  Other len samples (may need manual review):')
            for sample in other_samples:
                print(f'    {sample}')
    else:
        print('  No len annotations.')


def check_function_groups_against_enums(
    labeled_tokens: dict[Api, list[Token]],
    labeled_enums: dict[Api, list[EnumValue]],
) -> None:
    """Verify that every group annotation on a function argument or return value
    names a group that exists in the enum data.

    The documented semantic is that function group annotations restrict allowed
    values to a named enum group, so every group name should match one of the
    groups present on enums in the same registry. Groups that don't match any
    enum group indicate a mismatch worth investigating.
    """
    print('\n=== Function group annotations vs enum groups ===')

    for api in labeled_tokens:
        tokens = labeled_tokens[api]
        enums  = labeled_enums.get(api, [])

        enum_groups: set[str] = {g for e in enums for g in e.groups}

        unknown: list[tuple[str, str, str]] = []  # (function, param, group)
        for token in tokens:
            for annotated in [token.return_value, *token.args]:
                if annotated.group is not None and annotated.group not in enum_groups:
                    unknown.append((token.name, annotated.name, annotated.group))

        if unknown:
            print(f'  [{api}] {len(unknown)} group annotation(s) not found in enum groups:')
            for fn, param, group in unknown:
                print(f'    {fn}: {param} group={group!r}')
        else:
            print(f'  [{api}] All function group annotations match enum groups (or none present).')
