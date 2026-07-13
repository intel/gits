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
from generator import EnumValue

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
# Per-file inspection
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


# ---------------------------------------------------------------------------
# Cross-file inspection
# ---------------------------------------------------------------------------

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

    # Check if cross-file aliases exist.
    all_names = {e.name for enums in labeled_enums.values() for e in enums}
    name_to_file = {e.name: label for label, enums in labeled_enums.items() for e in enums}

    cross_aliases: list[tuple[str, str, str]] = []
    for label, enums in labeled_enums.items():
        file_names = {e.name for e in enums}
        for e in enums:
            if e.alias and e.alias not in file_names and e.alias in all_names:
                cross_aliases.append((label, e.name, e.alias))

    if cross_aliases:
        print(f'  Cross-file aliases ({len(cross_aliases)}):')
        for src_label, name, target in cross_aliases:
            print(f'    [{src_label}] {name} -> {target} (in {name_to_file[target]})')
    else:
        print('  No cross-file aliases.')
