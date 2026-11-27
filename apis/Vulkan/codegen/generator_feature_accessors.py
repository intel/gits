#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from generator_helpers import generate_file


def feature_members(structure):
    return [member for member in structure.members
            if member.base_type == 'VkBool32' and not member.is_pointer
            and not member.is_pointer_to_pointer and not member.is_triple_pointer
            and not member.fixed_array_size and member.bitfield is None]


def feature_identities(structures):
    aliases = {alias: structure.name for structure in structures for alias in structure.aliases}
    members = {(structure.name, member.name): member
               for structure in structures for member in feature_members(structure)}
    parents = {key: key for key in members}

    def find(key):
        while parents[key] != key:
            key = parents[key]
        return key

    for key, member in members.items():
        if '::' not in member.name_alias:
            continue
        structure_name, member_name = member.name_alias.split('::', 1)
        target = (aliases.get(structure_name, structure_name), member_name)
        if target in parents:
            parents[find(key)] = find(target)

    return {key: '.'.join(find(key)) for key in members}


def generate_feature_accessors(context, out_path):
    core_features = next(structure for structure in context['structures']
                         if structure.name == 'VkPhysicalDeviceFeatures')
    chain_features = [structure for structure in context['structures']
                      if 'VkPhysicalDeviceFeatures2' in structure.struct_extends
                      and structure.stype_value and feature_members(structure)]
    structures = [core_features, *chain_features]
    generate_file(context | {'core_features': core_features,
                             'chain_features': chain_features,
                             'feature_members': feature_members,
                             'feature_identities': feature_identities(structures)},
                  'featureAccessorsAuto.h', out_path)