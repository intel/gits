from generator_helpers import generate_file


def feature_members(structure):
    return [member for member in structure.members
            if member.base_type == 'VkBool32' and not member.is_pointer
            and not member.is_pointer_to_pointer and not member.is_triple_pointer
            and not member.fixed_array_size and member.bitfield is None]


def generate_feature_accessors(context, out_path):
    core_features = next(structure for structure in context['structures']
                         if structure.name == 'VkPhysicalDeviceFeatures')
    chain_features = [structure for structure in context['structures']
                      if 'VkPhysicalDeviceFeatures2' in structure.struct_extends
                      and structure.stype_value and feature_members(structure)]
    generate_file(context | {'core_features': core_features,
                             'chain_features': chain_features,
                             'feature_members': feature_members},
                  'featureAccessorsAuto.h', out_path)