#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

arguments_table = {}
enums_table = {}
functions_table = {}
constants_table = []

def Argument(**kwargs):
  if kwargs['name'] in arguments_table and kwargs['version']:
    arguments_table[kwargs['name'] + '_V' + str(kwargs['version'])] = kwargs
  else:
    arguments_table[kwargs['name']] = kwargs

def Enum(**kwargs):
  enums_table[kwargs['name']] = kwargs

def Function(**kwargs):
  if kwargs['name'] in functions_table and kwargs['version']:
    functions_table[kwargs['name'] + '_V' + str(kwargs['version'])] = kwargs
  else:
    functions_table[kwargs['name']] = kwargs

def Constant(**kwargs):
  constants_table.append(kwargs)

def ArgDef(**kwargs):
  return kwargs

def RetDef(**kwargs):
  return kwargs

def VarDef(**kwargs):
  return kwargs

def get_arguments():
  return arguments_table

def get_enums():
  return enums_table

def get_functions():
  return functions_table

def get_constants():
  return constants_table


Constant(name='ZES_DIAG_FIRST_TEST_INDEX',value='0x0')

Constant(name='ZES_DIAG_LAST_TEST_INDEX',value='0xFFFFFFFF')

Constant(name='ZES_ENGINE_ACTIVITY_EXT_NAME',value='"ZES_extension_engine_activity"')

Constant(name='ZES_FAN_TEMP_SPEED_PAIR_COUNT',value='32')

Constant(name='ZES_FIRMWARE_SECURITY_VERSION_EXP_NAME',value='"ZES_experimental_firmware_security_version"')

Constant(name='ZES_MAX_EXTENSION_NAME',value='256')

Constant(name='ZES_MAX_FABRIC_LINK_TYPE_SIZE',value='256')

Constant(name='ZES_MAX_FABRIC_PORT_MODEL_SIZE',value='256')

Constant(name='ZES_MAX_RAS_ERROR_CATEGORY_COUNT',value='7')

Constant(name='ZES_MAX_UUID_SIZE',value='16')

Constant(name='ZES_MEMORY_BANDWIDTH_COUNTER_BITS_EXP_PROPERTIES_NAME',value='"ZES_extension_mem_bandwidth_counter_bits_properties"')

Constant(name='ZES_MEM_PAGE_OFFLINE_STATE_EXP_NAME',value='"ZES_extension_mem_state"')

Constant(name='ZES_POWER_DOMAIN_PROPERTIES_EXP_NAME',value='"ZES_extension_power_domain_properties"')

Constant(name='ZES_POWER_LIMITS_EXT_NAME',value='"ZES_extension_power_limits"')

Constant(name='ZES_RAS_GET_STATE_EXP_NAME',value='"ZES_extension_ras_state"')

Constant(name='ZES_SCHED_WATCHDOG_DISABLE',value='(~(0ULL))')

Constant(name='ZES_STRING_PROPERTY_SIZE',value='64')

Constant(name='ZES_SYSMAN_DEVICE_MAPPING_EXP_NAME',value='"ZES_experimental_sysman_device_mapping"')

Constant(name='ZES_VIRTUAL_FUNCTION_MANAGEMENT_EXP_NAME',value='"ZES_experimental_virtual_function_management"')

Constant(name='ZET_API_TRACING_EXP_NAME',value='"ZET_experimental_api_tracing"')

Constant(name='ZET_CONCURRENT_METRIC_GROUPS_EXP_NAME',value='"ZET_experimental_concurrent_metric_groups"')

Constant(name='ZET_EXPORT_METRICS_DATA_EXP_NAME',value='"ZET_experimental_metric_export_data"')

Constant(name='ZET_GLOBAL_METRICS_TIMESTAMPS_EXP_NAME',value='"ZET_experimental_global_metric_timestamps"')

Constant(name='ZET_MAX_METRIC_COMPONENT',value='256')

Constant(name='ZET_MAX_METRIC_DESCRIPTION',value='256')

Constant(name='ZET_MAX_METRIC_EXPORT_DATA_ELEMENT_DESCRIPTION_EXP',value='256')

Constant(name='ZET_MAX_METRIC_EXPORT_DATA_ELEMENT_NAME_EXP',value='256')

Constant(name='ZET_MAX_METRIC_GROUP_DESCRIPTION',value='256')

Constant(name='ZET_MAX_METRIC_GROUP_NAME',value='256')

Constant(name='ZET_MAX_METRIC_NAME',value='256')

Constant(name='ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP',value='128')

Constant(name='ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP',value='128')

Constant(name='ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP',value='128')

Constant(name='ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP',value='128')

Constant(name='ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP',value='128')

Constant(name='ZET_MAX_METRIC_RESULT_UNITS',value='256')

Constant(name='ZET_MAX_PROGRAMMABLE_METRICS_ELEMENT_DESCRIPTION_EXP',value='256')

Constant(name='ZET_MAX_PROGRAMMABLE_METRICS_ELEMENT_NAME_EXP',value='256')

Constant(name='ZET_METRICS_TRACER_EXP_NAME',value='"ZET_experimental_metric_tracer"')

Constant(name='ZET_MULTI_METRICS_EXP_NAME',value='"ZET_experimental_calculate_multiple_metrics"')

Constant(name='ZET_PROGRAMMABLE_METRICS_EXP_NAME',value='"ZET_experimental_programmable_metrics"')

Constant(name='ZE_APICALL',value='__cdecl',condition='defined(_WIN32)')

Constant(name='ZE_APIEXPORT',value='__declspec(dllexport)',condition='defined(_WIN32)')

Constant(name='ZE_APIEXPORT',value='__attribute__ ((visibility ("default")))',condition='__GNUC__ >= 4')

Constant(name='ZE_API_VERSION_CURRENT_M',value='ZE_MAKE_VERSION( 1, 11 )')

Constant(name='ZE_BANDWIDTH_PROPERTIES_EXP_NAME',value='"ZE_experimental_bandwidth_properties"')

Constant(name='ZE_BFLOAT16_CONVERSIONS_EXT_NAME',value='"ZE_extension_bfloat16_conversions"')

Constant(name='ZE_BINDLESS_IMAGE_EXP_NAME',value='"ZE_experimental_bindless_image"')

Constant(name='ZE_BIT( _i )',value='( 1 << _i )')

Constant(name='ZE_CACHE_RESERVATION_EXT_NAME',value='"ZE_extension_cache_reservation"')

Constant(name='ZE_COMMAND_LIST_CLONE_EXP_NAME',value='"ZE_experimental_command_list_clone"')

Constant(name='ZE_CONTEXT_POWER_SAVING_HINT_EXP_NAME',value='"ZE_experimental_power_saving_hint"')

Constant(name='ZE_DEVICE_IP_VERSION_EXT_NAME',value='"ZE_extension_device_ip_version"')

Constant(name='ZE_DEVICE_LUID_EXT_NAME',value='"ZE_extension_device_luid"')

Constant(name='ZE_DEVICE_MEMORY_PROPERTIES_EXT_NAME',value='"ZE_extension_device_memory_properties"')

Constant(name='ZE_DLLEXPORT',value='__declspec(dllexport)',condition='defined(_WIN32)')

Constant(name='ZE_DLLEXPORT',value='__attribute__ ((visibility ("default")))',condition='__GNUC__ >= 4')

Constant(name='ZE_EU_COUNT_EXT_NAME',value='"ZE_extension_eu_count"')

Constant(name='ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME',value='"ZE_experimental_event_pool_counter_based"')

Constant(name='ZE_EVENT_QUERY_KERNEL_TIMESTAMPS_EXT_NAME',value='"ZE_extension_event_query_kernel_timestamps"')

Constant(name='ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME',value='"ZE_experimental_event_query_timestamps"')

Constant(name='ZE_FABRIC_EXP_NAME',value='"ZE_experimental_fabric"')

Constant(name='ZE_FLOAT_ATOMICS_EXT_NAME',value='"ZE_extension_float_atomics"')

Constant(name='ZE_GET_KERNEL_BINARY_EXP_NAME',value='"ZE_extension_kernel_binary_exp"')

Constant(name='ZE_GLOBAL_OFFSET_EXP_NAME',value='"ZE_experimental_global_offset"')

Constant(name='ZE_IMAGE_COPY_EXT_NAME',value='"ZE_extension_image_copy"')

Constant(name='ZE_IMAGE_MEMORY_PROPERTIES_EXP_NAME',value='"ZE_experimental_image_memory_properties"')

Constant(name='ZE_IMAGE_QUERY_ALLOC_PROPERTIES_EXT_NAME',value='"ZE_extension_image_query_alloc_properties"')

Constant(name='ZE_IMAGE_VIEW_EXP_NAME',value='"ZE_experimental_image_view"')

Constant(name='ZE_IMAGE_VIEW_EXT_NAME',value='"ZE_extension_image_view"')

Constant(name='ZE_IMAGE_VIEW_PLANAR_EXP_NAME',value='"ZE_experimental_image_view_planar"')

Constant(name='ZE_IMAGE_VIEW_PLANAR_EXT_NAME',value='"ZE_extension_image_view_planar"')

Constant(name='ZE_IMMEDIATE_COMMAND_LIST_APPEND_EXP_NAME',value='"ZE_experimental_immediate_command_list_append"')

Constant(name='ZE_KERNEL_MAX_GROUP_SIZE_PROPERTIES_EXT_NAME',value='"ZE_extension_kernel_max_group_size_properties"')

Constant(name='ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME',value='"ZE_experimental_scheduling_hints"')

Constant(name='ZE_LINKAGE_INSPECTION_EXT_NAME',value='"ZE_extension_linkage_inspection"')

Constant(name='ZE_LINKONCE_ODR_EXT_NAME',value='"ZE_extension_linkonce_odr"')

Constant(name='ZE_MAJOR_VERSION( _ver )',value='( _ver >> 16 )')

Constant(name='ZE_MAKE_VERSION( _major, _minor )',value='(( _major << 16 )|( _minor & 0x0000ffff))')

Constant(name='ZE_MAX_DEVICE_LUID_SIZE_EXT',value='8')

Constant(name='ZE_MAX_DEVICE_NAME',value='256')

Constant(name='ZE_MAX_DEVICE_UUID_SIZE',value='16')

Constant(name='ZE_MAX_DRIVER_UUID_SIZE',value='16')

Constant(name='ZE_MAX_EXTENSION_NAME',value='256')

Constant(name='ZE_MAX_FABRIC_EDGE_MODEL_EXP_SIZE',value='256')

Constant(name='ZE_MAX_IPC_HANDLE_SIZE',value='64')

Constant(name='ZE_MAX_KERNEL_UUID_SIZE',value='16')

Constant(name='ZE_MAX_METRIC_GROUP_NAME_PREFIX',value='64')

Constant(name='ZE_MAX_MODULE_UUID_SIZE',value='16')

Constant(name='ZE_MAX_NATIVE_KERNEL_UUID_SIZE',value='16')

Constant(name='ZE_MAX_UUID_SIZE',value='16')

Constant(name='ZE_MEMORY_COMPRESSION_HINTS_EXT_NAME',value='"ZE_extension_memory_compression_hints"')

Constant(name='ZE_MEMORY_FREE_POLICIES_EXT_NAME',value='"ZE_extension_memory_free_policies"')

Constant(name='ZE_MINOR_VERSION( _ver )',value='( _ver & 0x0000ffff )')

Constant(name='ZE_MODULE_PROGRAM_EXP_NAME',value='"ZE_experimental_module_program"')

Constant(name='ZE_MUTABLE_COMMAND_LIST_EXP_NAME',value='"ZE_experimental_mutable_command_list"')

Constant(name='ZE_PCI_PROPERTIES_EXT_NAME',value='"ZE_extension_pci_properties"')

Constant(name='ZE_RAYTRACING_EXT_NAME',value='"ZE_extension_raytracing"')

Constant(name='ZE_RELAXED_ALLOCATION_LIMITS_EXP_NAME',value='"ZE_experimental_relaxed_allocation_limits"')

Constant(name='ZE_RTAS_BUILDER_EXP_NAME',value='"ZE_experimental_rtas_builder"')

Constant(name='ZE_SRGB_EXT_NAME',value='"ZE_extension_srgb"')

Constant(name='ZE_SUBGROUPSIZE_COUNT',value='8')

Constant(name='ZE_SUBGROUPS_EXT_NAME',value='"ZE_extension_subgroups"')

Constant(name='ZE_SUB_ALLOCATIONS_EXP_NAME',value='"ZE_experimental_sub_allocations"')

Enum(name='ze_api_version_t',
var1=VarDef(name='ZE_API_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
var2=VarDef(name='ZE_API_VERSION_1_1',value='ZE_MAKE_VERSION( 1, 1 )'),
var3=VarDef(name='ZE_API_VERSION_1_2',value='ZE_MAKE_VERSION( 1, 2 )'),
var4=VarDef(name='ZE_API_VERSION_1_3',value='ZE_MAKE_VERSION( 1, 3 )'),
var5=VarDef(name='ZE_API_VERSION_1_4',value='ZE_MAKE_VERSION( 1, 4 )'),
var6=VarDef(name='ZE_API_VERSION_1_5',value='ZE_MAKE_VERSION( 1, 5 )'),
var7=VarDef(name='ZE_API_VERSION_1_6',value='ZE_MAKE_VERSION( 1, 6 )'),
var8=VarDef(name='ZE_API_VERSION_1_7',value='ZE_MAKE_VERSION( 1, 7 )'),
var9=VarDef(name='ZE_API_VERSION_1_8',value='ZE_MAKE_VERSION( 1, 8 )'),
var10=VarDef(name='ZE_API_VERSION_1_9',value='ZE_MAKE_VERSION( 1, 9 )'),
var11=VarDef(name='ZE_API_VERSION_1_10',value='ZE_MAKE_VERSION( 1, 10 )'),
var12=VarDef(name='ZE_API_VERSION_1_11',value='ZE_MAKE_VERSION( 1, 11 )'),
# var13=VarDef(name='ZE_API_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 11 )'),
)

Enum(name='ze_bandwidth_unit_t',
var1=VarDef(name='ZE_BANDWIDTH_UNIT_UNKNOWN',value='0'),
var2=VarDef(name='ZE_BANDWIDTH_UNIT_BYTES_PER_NANOSEC',value='1'),
var3=VarDef(name='ZE_BANDWIDTH_UNIT_BYTES_PER_CLOCK',value='2'),
)

Enum(name='ze_bfloat16_conversions_ext_version_t',
var1=VarDef(name='ZE_BFLOAT16_CONVERSIONS_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_BFLOAT16_CONVERSIONS_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_bindless_image_exp_version_t',
var1=VarDef(name='ZE_BINDLESS_IMAGE_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_BINDLESS_IMAGE_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_cache_config_flags_t',bitFields=True,
var1=VarDef(name='ZE_CACHE_CONFIG_FLAG_LARGE_SLM',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_CACHE_CONFIG_FLAG_LARGE_DATA',value='ZE_BIT(1)'),
)

Enum(name='ze_cache_ext_region_t',
var1=VarDef(name='ZE_CACHE_EXT_REGION_DEFAULT',value='0'),
var2=VarDef(name='ZE_CACHE_EXT_REGION_RESERVED',value='1'),
var3=VarDef(name='ZE_CACHE_EXT_REGION_NON_RESERVED',value='2'),
)

Enum(name='ze_cache_reservation_ext_version_t',
var1=VarDef(name='ZE_CACHE_RESERVATION_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_CACHE_RESERVATION_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_calculate_multiple_metrics_exp_version_t',
var1=VarDef(name='ZE_CALCULATE_MULTIPLE_METRICS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_CALCULATE_MULTIPLE_METRICS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_command_list_clone_exp_version_t',
var1=VarDef(name='ZE_COMMAND_LIST_CLONE_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_COMMAND_LIST_CLONE_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_command_list_flags_t',bitFields=True,
var1=VarDef(name='ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_COMMAND_LIST_FLAG_IN_ORDER',value='ZE_BIT(3)'),
var5=VarDef(name='ZE_COMMAND_LIST_FLAG_EXP_CLONEABLE',value='ZE_BIT(4)'),
)

Enum(name='ze_command_queue_flags_t',bitFields=True,
var1=VarDef(name='ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_COMMAND_QUEUE_FLAG_IN_ORDER',value='ZE_BIT(1)'),
)

Enum(name='ze_command_queue_group_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS',value='ZE_BIT(3)'),
)

Enum(name='ze_command_queue_mode_t',
var1=VarDef(name='ZE_COMMAND_QUEUE_MODE_DEFAULT',value='0'),
var2=VarDef(name='ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS',value='1'),
var3=VarDef(name='ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS',value='2'),
)

Enum(name='ze_command_queue_priority_t',
var1=VarDef(name='ZE_COMMAND_QUEUE_PRIORITY_NORMAL',value='0'),
var2=VarDef(name='ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW',value='1'),
var3=VarDef(name='ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH',value='2'),
)

Enum(name='ze_context_flags_t',bitFields=True,
var1=VarDef(name='ZE_CONTEXT_FLAG_TBD',value='ZE_BIT(0)'),
)

Enum(name='ze_device_cache_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_CACHE_PROPERTY_FLAG_USER_CONTROL',value='ZE_BIT(0)'),
)

Enum(name='ze_device_fp_atomic_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_LOAD_STORE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_ADD',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_MIN_MAX',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_LOAD_STORE',value='ZE_BIT(16)'),
var5=VarDef(name='ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_ADD',value='ZE_BIT(17)'),
var6=VarDef(name='ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_MIN_MAX',value='ZE_BIT(18)'),
)

Enum(name='ze_device_fp_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_FP_FLAG_DENORM',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DEVICE_FP_FLAG_INF_NAN',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_DEVICE_FP_FLAG_ROUND_TO_NEAREST',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_DEVICE_FP_FLAG_ROUND_TO_ZERO',value='ZE_BIT(3)'),
var5=VarDef(name='ZE_DEVICE_FP_FLAG_ROUND_TO_INF',value='ZE_BIT(4)'),
var6=VarDef(name='ZE_DEVICE_FP_FLAG_FMA',value='ZE_BIT(5)'),
var7=VarDef(name='ZE_DEVICE_FP_FLAG_ROUNDED_DIVIDE_SQRT',value='ZE_BIT(6)'),
var8=VarDef(name='ZE_DEVICE_FP_FLAG_SOFT_FLOAT',value='ZE_BIT(7)'),
)

Enum(name='ze_device_ip_version_version_t',
var1=VarDef(name='ZE_DEVICE_IP_VERSION_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_DEVICE_IP_VERSION_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_device_luid_ext_version_t',
var1=VarDef(name='ZE_DEVICE_LUID_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_DEVICE_LUID_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_device_mem_alloc_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT',value='ZE_BIT(2)'),
)

Enum(name='ze_device_memory_ext_type_t',
var1=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_HBM',value='0'),
var2=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_HBM2',value='1'),
var3=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_DDR',value='2'),
var4=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_DDR2',value='3'),
var5=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_DDR3',value='4'),
var6=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_DDR4',value='5'),
var7=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_DDR5',value='6'),
var8=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR',value='7'),
var9=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR3',value='8'),
var10=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR4',value='9'),
var11=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_LPDDR5',value='10'),
var12=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_SRAM',value='11'),
var13=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_L1',value='12'),
var14=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_L3',value='13'),
var15=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GRF',value='14'),
var16=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_SLM',value='15'),
var17=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GDDR4',value='16'),
var18=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GDDR5',value='17'),
var19=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GDDR5X',value='18'),
var20=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GDDR6',value='19'),
var21=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GDDR6X',value='20'),
var22=VarDef(name='ZE_DEVICE_MEMORY_EXT_TYPE_GDDR7',value='21'),
)

Enum(name='ze_device_memory_properties_ext_version_t',
var1=VarDef(name='ZE_DEVICE_MEMORY_PROPERTIES_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_DEVICE_MEMORY_PROPERTIES_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_device_memory_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_MEMORY_PROPERTY_FLAG_TBD',value='ZE_BIT(0)'),
)

Enum(name='ze_device_module_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_MODULE_FLAG_FP16',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DEVICE_MODULE_FLAG_FP64',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_DEVICE_MODULE_FLAG_INT64_ATOMICS',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_DEVICE_MODULE_FLAG_DP4A',value='ZE_BIT(3)'),
)

Enum(name='ze_device_p2p_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_P2P_PROPERTY_FLAG_ACCESS',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS',value='ZE_BIT(1)'),
)

Enum(name='ze_device_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_PROPERTY_FLAG_INTEGRATED',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_DEVICE_PROPERTY_FLAG_ECC',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING',value='ZE_BIT(3)'),
)

Enum(name='ze_device_raytracing_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_DEVICE_RAYTRACING_EXT_FLAG_RAYQUERY',value='ZE_BIT(0)'),
)

Enum(name='ze_device_type_t',
var1=VarDef(name='ZE_DEVICE_TYPE_GPU',value='1'),
var2=VarDef(name='ZE_DEVICE_TYPE_CPU',value='2'),
var3=VarDef(name='ZE_DEVICE_TYPE_FPGA',value='3'),
var4=VarDef(name='ZE_DEVICE_TYPE_MCA',value='4'),
var5=VarDef(name='ZE_DEVICE_TYPE_VPU',value='5'),
)

Enum(name='ze_driver_memory_free_policy_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_DEFER_FREE',value='ZE_BIT(1)'),
)

Enum(name='ze_eu_count_ext_version_t',
var1=VarDef(name='ZE_EU_COUNT_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_EU_COUNT_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_event_pool_counter_based_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_IMMEDIATE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_NON_IMMEDIATE',value='ZE_BIT(1)'),
)

Enum(name='ze_event_pool_counter_based_exp_version_t',
var1=VarDef(name='ZE_EVENT_POOL_COUNTER_BASED_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_EVENT_POOL_COUNTER_BASED_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_event_pool_flags_t',bitFields=True,
var1=VarDef(name='ZE_EVENT_POOL_FLAG_HOST_VISIBLE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_EVENT_POOL_FLAG_IPC',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP',value='ZE_BIT(3)'),
)

Enum(name='ze_event_query_kernel_timestamps_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_EVENT_QUERY_KERNEL_TIMESTAMPS_EXT_FLAG_KERNEL',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_EVENT_QUERY_KERNEL_TIMESTAMPS_EXT_FLAG_SYNCHRONIZED',value='ZE_BIT(1)'),
)

Enum(name='ze_event_query_kernel_timestamps_ext_version_t',
var1=VarDef(name='ZE_EVENT_QUERY_KERNEL_TIMESTAMPS_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_EVENT_QUERY_KERNEL_TIMESTAMPS_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_event_query_timestamps_exp_version_t',
var1=VarDef(name='ZE_EVENT_QUERY_TIMESTAMPS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_EVENT_QUERY_TIMESTAMPS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_event_scope_flags_t',bitFields=True,
var1=VarDef(name='ZE_EVENT_SCOPE_FLAG_SUBDEVICE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_EVENT_SCOPE_FLAG_DEVICE',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_EVENT_SCOPE_FLAG_HOST',value='ZE_BIT(2)'),
)

Enum(name='ze_external_memory_type_flags_t',bitFields=True,
var1=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT',value='ZE_BIT(3)'),
var5=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE',value='ZE_BIT(4)'),
var6=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE_KMT',value='ZE_BIT(5)'),
var7=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP',value='ZE_BIT(6)'),
var8=VarDef(name='ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE',value='ZE_BIT(7)'),
)

Enum(name='ze_fabric_edge_exp_duplexity_t',
var1=VarDef(name='ZE_FABRIC_EDGE_EXP_DUPLEXITY_UNKNOWN',value='0'),
var2=VarDef(name='ZE_FABRIC_EDGE_EXP_DUPLEXITY_HALF_DUPLEX',value='1'),
var3=VarDef(name='ZE_FABRIC_EDGE_EXP_DUPLEXITY_FULL_DUPLEX',value='2'),
)

Enum(name='ze_fabric_vertex_exp_type_t',
var1=VarDef(name='ZE_FABRIC_VERTEX_EXP_TYPE_UNKNOWN',value='0'),
var2=VarDef(name='ZE_FABRIC_VERTEX_EXP_TYPE_DEVICE',value='1'),
var3=VarDef(name='ZE_FABRIC_VERTEX_EXP_TYPE_SUBDEVICE',value='2'),
var4=VarDef(name='ZE_FABRIC_VERTEX_EXP_TYPE_SWITCH',value='3'),
)

Enum(name='ze_fence_flags_t',bitFields=True,
var1=VarDef(name='ZE_FENCE_FLAG_SIGNALED',value='ZE_BIT(0)'),
)

Enum(name='ze_float_atomics_ext_version_t',
var1=VarDef(name='ZE_FLOAT_ATOMICS_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_FLOAT_ATOMICS_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_global_offset_exp_version_t',
var1=VarDef(name='ZE_GLOBAL_OFFSET_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_GLOBAL_OFFSET_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_host_mem_alloc_flags_t',bitFields=True,
var1=VarDef(name='ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_HOST_MEM_ALLOC_FLAG_BIAS_WRITE_COMBINED',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_HOST_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT',value='ZE_BIT(3)'),
)

Enum(name='ze_image_bindless_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_IMAGE_BINDLESS_EXP_FLAG_BINDLESS',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_IMAGE_BINDLESS_EXP_FLAG_SAMPLED_IMAGE',value='ZE_BIT(1)'),
)

Enum(name='ze_image_copy_ext_version_t',
var1=VarDef(name='ZE_IMAGE_COPY_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_COPY_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_image_flags_t',bitFields=True,
var1=VarDef(name='ZE_IMAGE_FLAG_KERNEL_WRITE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_IMAGE_FLAG_BIAS_UNCACHED',value='ZE_BIT(1)'),
)

Enum(name='ze_image_format_layout_t',
var1=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_8',value='0'),
var2=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_16',value='1'),
var3=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_32',value='2'),
var4=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_8_8',value='3'),
var5=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8',value='4'),
var6=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_16_16',value='5'),
var7=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16',value='6'),
var8=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_32_32',value='7'),
var9=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32',value='8'),
var10=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2',value='9'),
var11=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_11_11_10',value='10'),
var12=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_5_6_5',value='11'),
var13=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1',value='12'),
var14=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4',value='13'),
var15=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_Y8',value='14'),
var16=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_NV12',value='15'),
var17=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_YUYV',value='16'),
var18=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_VYUY',value='17'),
var19=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_YVYU',value='18'),
var20=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_UYVY',value='19'),
var21=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_AYUV',value='20'),
var22=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_P010',value='21'),
var23=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_Y410',value='22'),
var24=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_P012',value='23'),
var25=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_Y16',value='24'),
var26=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_P016',value='25'),
var27=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_Y216',value='26'),
var28=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_P216',value='27'),
var29=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_P8',value='28'),
var30=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_YUY2',value='29'),
var31=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_A8P8',value='30'),
var32=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_IA44',value='31'),
var33=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_AI44',value='32'),
var34=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_Y416',value='33'),
var35=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_Y210',value='34'),
var36=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_I420',value='35'),
var37=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_YV12',value='36'),
var38=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_400P',value='37'),
var39=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_422H',value='38'),
var40=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_422V',value='39'),
var41=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_444P',value='40'),
var42=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_RGBP',value='41'),
var43=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_BRGP',value='42'),
var44=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_8_8_8',value='43'),
var45=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_16_16_16',value='44'),
var46=VarDef(name='ZE_IMAGE_FORMAT_LAYOUT_32_32_32',value='45'),
)

Enum(name='ze_image_format_swizzle_t',
var1=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_R',value='0'),
var2=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_G',value='1'),
var3=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_B',value='2'),
var4=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_A',value='3'),
var5=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_0',value='4'),
var6=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_1',value='5'),
var7=VarDef(name='ZE_IMAGE_FORMAT_SWIZZLE_X',value='6'),
)

Enum(name='ze_image_format_type_t',
var1=VarDef(name='ZE_IMAGE_FORMAT_TYPE_UINT',value='0'),
var2=VarDef(name='ZE_IMAGE_FORMAT_TYPE_SINT',value='1'),
var3=VarDef(name='ZE_IMAGE_FORMAT_TYPE_UNORM',value='2'),
var4=VarDef(name='ZE_IMAGE_FORMAT_TYPE_SNORM',value='3'),
var5=VarDef(name='ZE_IMAGE_FORMAT_TYPE_FLOAT',value='4'),
)

Enum(name='ze_image_memory_properties_exp_version_t',
var1=VarDef(name='ZE_IMAGE_MEMORY_PROPERTIES_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_MEMORY_PROPERTIES_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_image_query_alloc_properties_ext_version_t',
var1=VarDef(name='ZE_IMAGE_QUERY_ALLOC_PROPERTIES_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_QUERY_ALLOC_PROPERTIES_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_image_sampler_filter_flags_t',bitFields=True,
var1=VarDef(name='ZE_IMAGE_SAMPLER_FILTER_FLAG_POINT',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_IMAGE_SAMPLER_FILTER_FLAG_LINEAR',value='ZE_BIT(1)'),
)

Enum(name='ze_image_type_t',
var1=VarDef(name='ZE_IMAGE_TYPE_1D',value='0'),
var2=VarDef(name='ZE_IMAGE_TYPE_1DARRAY',value='1'),
var3=VarDef(name='ZE_IMAGE_TYPE_2D',value='2'),
var4=VarDef(name='ZE_IMAGE_TYPE_2DARRAY',value='3'),
var5=VarDef(name='ZE_IMAGE_TYPE_3D',value='4'),
var6=VarDef(name='ZE_IMAGE_TYPE_BUFFER',value='5'),
)

Enum(name='ze_image_view_exp_version_t',
var1=VarDef(name='ZE_IMAGE_VIEW_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_VIEW_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_image_view_ext_version_t',
var1=VarDef(name='ZE_IMAGE_VIEW_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_VIEW_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_image_view_planar_exp_version_t',
var1=VarDef(name='ZE_IMAGE_VIEW_PLANAR_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_VIEW_PLANAR_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_image_view_planar_ext_version_t',
var1=VarDef(name='ZE_IMAGE_VIEW_PLANAR_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMAGE_VIEW_PLANAR_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_immediate_command_list_append_exp_version_t',
var1=VarDef(name='ZE_IMMEDIATE_COMMAND_LIST_APPEND_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_IMMEDIATE_COMMAND_LIST_APPEND_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_init_driver_type_flags_t',bitFields=True,
var1=VarDef(name='ZE_INIT_DRIVER_TYPE_FLAG_GPU',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_INIT_DRIVER_TYPE_FLAG_NPU',value='ZE_BIT(1)'),
)

Enum(name='ze_init_flags_t',bitFields=True,
var1=VarDef(name='ZE_INIT_FLAG_GPU_ONLY',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_INIT_FLAG_VPU_ONLY',value='ZE_BIT(1)'),
)

Enum(name='ze_ipc_memory_flags_t',bitFields=True,
var1=VarDef(name='ZE_IPC_MEMORY_FLAG_BIAS_CACHED',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED',value='ZE_BIT(1)'),
)

Enum(name='ze_ipc_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_IPC_PROPERTY_FLAG_MEMORY',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_IPC_PROPERTY_FLAG_EVENT_POOL',value='ZE_BIT(1)'),
)

Enum(name='ze_kernel_flags_t',bitFields=True,
var1=VarDef(name='ZE_KERNEL_FLAG_FORCE_RESIDENCY',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY',value='ZE_BIT(1)'),
)

Enum(name='ze_kernel_indirect_access_flags_t',bitFields=True,
var1=VarDef(name='ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED',value='ZE_BIT(2)'),
)

Enum(name='ze_kernel_max_group_size_properties_ext_version_t',
var1=VarDef(name='ZE_KERNEL_MAX_GROUP_SIZE_PROPERTIES_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_KERNEL_MAX_GROUP_SIZE_PROPERTIES_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_latency_unit_t',
var1=VarDef(name='ZE_LATENCY_UNIT_UNKNOWN',value='0'),
var2=VarDef(name='ZE_LATENCY_UNIT_NANOSEC',value='1'),
var3=VarDef(name='ZE_LATENCY_UNIT_CLOCK',value='2'),
var4=VarDef(name='ZE_LATENCY_UNIT_HOP',value='3'),
)

Enum(name='ze_linkage_inspection_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_LINKAGE_INSPECTION_EXT_FLAG_IMPORTS',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_LINKAGE_INSPECTION_EXT_FLAG_UNRESOLVABLE_IMPORTS',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_LINKAGE_INSPECTION_EXT_FLAG_EXPORTS',value='ZE_BIT(2)'),
)

Enum(name='ze_linkage_inspection_ext_version_t',
var1=VarDef(name='ZE_LINKAGE_INSPECTION_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_LINKAGE_INSPECTION_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_linkonce_odr_ext_version_t',
var1=VarDef(name='ZE_LINKONCE_ODR_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_LINKONCE_ODR_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_memory_access_attribute_t',
var1=VarDef(name='ZE_MEMORY_ACCESS_ATTRIBUTE_NONE',value='0'),
var2=VarDef(name='ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE',value='1'),
var3=VarDef(name='ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY',value='2'),
)

Enum(name='ze_memory_access_cap_flags_t',bitFields=True,
var1=VarDef(name='ZE_MEMORY_ACCESS_CAP_FLAG_RW',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC',value='ZE_BIT(3)'),
)

Enum(name='ze_memory_advice_t',
var1=VarDef(name='ZE_MEMORY_ADVICE_SET_READ_MOSTLY',value='0'),
var2=VarDef(name='ZE_MEMORY_ADVICE_CLEAR_READ_MOSTLY',value='1'),
var3=VarDef(name='ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION',value='2'),
var4=VarDef(name='ZE_MEMORY_ADVICE_CLEAR_PREFERRED_LOCATION',value='3'),
var5=VarDef(name='ZE_MEMORY_ADVICE_SET_NON_ATOMIC_MOSTLY',value='4'),
var6=VarDef(name='ZE_MEMORY_ADVICE_CLEAR_NON_ATOMIC_MOSTLY',value='5'),
var7=VarDef(name='ZE_MEMORY_ADVICE_BIAS_CACHED',value='6'),
var8=VarDef(name='ZE_MEMORY_ADVICE_BIAS_UNCACHED',value='7'),
var9=VarDef(name='ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION',value='8'),
var10=VarDef(name='ZE_MEMORY_ADVICE_CLEAR_SYSTEM_MEMORY_PREFERRED_LOCATION',value='9'),
)

Enum(name='ze_memory_atomic_attr_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_ATOMICS',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_HOST_ATOMICS',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_HOST_ATOMICS',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_DEVICE_ATOMICS',value='ZE_BIT(3)'),
var5=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_DEVICE_ATOMICS',value='ZE_BIT(4)'),
var6=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_SYSTEM_ATOMICS',value='ZE_BIT(5)'),
var7=VarDef(name='ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_SYSTEM_ATOMICS',value='ZE_BIT(6)'),
)

Enum(name='ze_memory_compression_hints_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_MEMORY_COMPRESSION_HINTS_EXT_FLAG_COMPRESSED',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_MEMORY_COMPRESSION_HINTS_EXT_FLAG_UNCOMPRESSED',value='ZE_BIT(1)'),
)

Enum(name='ze_memory_compression_hints_ext_version_t',
var1=VarDef(name='ZE_MEMORY_COMPRESSION_HINTS_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_MEMORY_COMPRESSION_HINTS_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_memory_free_policies_ext_version_t',
var1=VarDef(name='ZE_MEMORY_FREE_POLICIES_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_MEMORY_FREE_POLICIES_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_memory_type_t',
var1=VarDef(name='ZE_MEMORY_TYPE_UNKNOWN',value='0'),
var2=VarDef(name='ZE_MEMORY_TYPE_HOST',value='1'),
var3=VarDef(name='ZE_MEMORY_TYPE_DEVICE',value='2'),
var4=VarDef(name='ZE_MEMORY_TYPE_SHARED',value='3'),
)

Enum(name='ze_metric_global_timestamps_exp_version_t',
var1=VarDef(name='ZE_METRIC_GLOBAL_TIMESTAMPS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_METRIC_GLOBAL_TIMESTAMPS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_module_format_t',
var1=VarDef(name='ZE_MODULE_FORMAT_IL_SPIRV',value='0'),
var2=VarDef(name='ZE_MODULE_FORMAT_NATIVE',value='1'),
)

Enum(name='ze_module_program_exp_version_t',
var1=VarDef(name='ZE_MODULE_PROGRAM_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_MODULE_PROGRAM_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_module_property_flags_t',bitFields=True,
var1=VarDef(name='ZE_MODULE_PROPERTY_FLAG_IMPORTS',value='ZE_BIT(0)'),
)

Enum(name='ze_mutable_command_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET',value='ZE_BIT(3)'),
var5=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT',value='ZE_BIT(4)'),
var6=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS',value='ZE_BIT(5)'),
var7=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION',value='ZE_BIT(6)'),
var8=VarDef(name='ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS',value='ZE_BIT(7)'),
)

Enum(name='ze_mutable_command_list_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_MUTABLE_COMMAND_LIST_EXP_FLAG_RESERVED',value='ZE_BIT(0)'),
)

Enum(name='ze_mutable_command_list_exp_version_t',
var1=VarDef(name='ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
var2=VarDef(name='ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_1_1',value='ZE_MAKE_VERSION( 1, 1 )'),
# var3=VarDef(name='ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 1 )'),
)

Enum(name='ze_pci_properties_ext_version_t',
var1=VarDef(name='ZE_PCI_PROPERTIES_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_PCI_PROPERTIES_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_physical_mem_flags_t',bitFields=True,
var1=VarDef(name='ZE_PHYSICAL_MEM_FLAG_ALLOCATE_ON_DEVICE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_PHYSICAL_MEM_FLAG_ALLOCATE_ON_HOST',value='ZE_BIT(1)'),
)

Enum(name='ze_power_saving_hint_exp_version_t',
var1=VarDef(name='ZE_POWER_SAVING_HINT_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_POWER_SAVING_HINT_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_power_saving_hint_type_t',
var1=VarDef(name='ZE_POWER_SAVING_HINT_TYPE_MIN',value='0'),
var2=VarDef(name='ZE_POWER_SAVING_HINT_TYPE_MAX',value='100'),
)

Enum(name='ze_raytracing_ext_version_t',
var1=VarDef(name='ZE_RAYTRACING_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_RAYTRACING_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_raytracing_mem_alloc_ext_flags_t',bitFields=True,
var1=VarDef(name='ZE_RAYTRACING_MEM_ALLOC_EXT_FLAG_TBD',value='ZE_BIT(0)'),
)

Enum(name='ze_relaxed_allocation_limits_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RELAXED_ALLOCATION_LIMITS_EXP_FLAG_MAX_SIZE',value='ZE_BIT(0)'),
)

Enum(name='ze_relaxed_allocation_limits_exp_version_t',
var1=VarDef(name='ZE_RELAXED_ALLOCATION_LIMITS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_RELAXED_ALLOCATION_LIMITS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_result_t',
var1=VarDef(name='ZE_RESULT_SUCCESS',value='0'),
var2=VarDef(name='ZE_RESULT_NOT_READY',value='1'),
var3=VarDef(name='ZE_RESULT_ERROR_DEVICE_LOST',value='0x70000001'),
var4=VarDef(name='ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY',value='0x70000002'),
var5=VarDef(name='ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY',value='0x70000003'),
var6=VarDef(name='ZE_RESULT_ERROR_MODULE_BUILD_FAILURE',value='0x70000004'),
var7=VarDef(name='ZE_RESULT_ERROR_MODULE_LINK_FAILURE',value='0x70000005'),
var8=VarDef(name='ZE_RESULT_ERROR_DEVICE_REQUIRES_RESET',value='0x70000006'),
var9=VarDef(name='ZE_RESULT_ERROR_DEVICE_IN_LOW_POWER_STATE',value='0x70000007'),
var10=VarDef(name='ZE_RESULT_EXP_ERROR_DEVICE_IS_NOT_VERTEX',value='0x7ff00001'),
var11=VarDef(name='ZE_RESULT_EXP_ERROR_VERTEX_IS_NOT_DEVICE',value='0x7ff00002'),
var12=VarDef(name='ZE_RESULT_EXP_ERROR_REMOTE_DEVICE',value='0x7ff00003'),
var13=VarDef(name='ZE_RESULT_EXP_ERROR_OPERANDS_INCOMPATIBLE',value='0x7ff00004'),
var14=VarDef(name='ZE_RESULT_EXP_RTAS_BUILD_RETRY',value='0x7ff00005'),
var15=VarDef(name='ZE_RESULT_EXP_RTAS_BUILD_DEFERRED',value='0x7ff00006'),
var16=VarDef(name='ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS',value='0x70010000'),
var17=VarDef(name='ZE_RESULT_ERROR_NOT_AVAILABLE',value='0x70010001'),
var18=VarDef(name='ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE',value='0x70020000'),
var19=VarDef(name='ZE_RESULT_WARNING_DROPPED_DATA',value='0x70020001'),
var20=VarDef(name='ZE_RESULT_ERROR_UNINITIALIZED',value='0x78000001'),
var21=VarDef(name='ZE_RESULT_ERROR_UNSUPPORTED_VERSION',value='0x78000002'),
var22=VarDef(name='ZE_RESULT_ERROR_UNSUPPORTED_FEATURE',value='0x78000003'),
var23=VarDef(name='ZE_RESULT_ERROR_INVALID_ARGUMENT',value='0x78000004'),
var24=VarDef(name='ZE_RESULT_ERROR_INVALID_NULL_HANDLE',value='0x78000005'),
var25=VarDef(name='ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE',value='0x78000006'),
var26=VarDef(name='ZE_RESULT_ERROR_INVALID_NULL_POINTER',value='0x78000007'),
var27=VarDef(name='ZE_RESULT_ERROR_INVALID_SIZE',value='0x78000008'),
var28=VarDef(name='ZE_RESULT_ERROR_UNSUPPORTED_SIZE',value='0x78000009'),
var29=VarDef(name='ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT',value='0x7800000a'),
var30=VarDef(name='ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT',value='0x7800000b'),
var31=VarDef(name='ZE_RESULT_ERROR_INVALID_ENUMERATION',value='0x7800000c'),
var32=VarDef(name='ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION',value='0x7800000d'),
var33=VarDef(name='ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT',value='0x7800000e'),
var34=VarDef(name='ZE_RESULT_ERROR_INVALID_NATIVE_BINARY',value='0x7800000f'),
var35=VarDef(name='ZE_RESULT_ERROR_INVALID_GLOBAL_NAME',value='0x78000010'),
var36=VarDef(name='ZE_RESULT_ERROR_INVALID_KERNEL_NAME',value='0x78000011'),
var37=VarDef(name='ZE_RESULT_ERROR_INVALID_FUNCTION_NAME',value='0x78000012'),
var38=VarDef(name='ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION',value='0x78000013'),
var39=VarDef(name='ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION',value='0x78000014'),
var40=VarDef(name='ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX',value='0x78000015'),
var41=VarDef(name='ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE',value='0x78000016'),
var42=VarDef(name='ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE',value='0x78000017'),
var43=VarDef(name='ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED',value='0x78000018'),
var44=VarDef(name='ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE',value='0x78000019'),
var45=VarDef(name='ZE_RESULT_ERROR_OVERLAPPING_REGIONS',value='0x7800001a'),
var46=VarDef(name='ZE_RESULT_WARNING_ACTION_REQUIRED',value='0x7800001b'),
var47=VarDef(name='ZE_RESULT_ERROR_INVALID_KERNEL_HANDLE',value='0x7800001c'),
var48=VarDef(name='ZE_RESULT_ERROR_UNKNOWN',value='0x7ffffffe'),
)

Enum(name='ze_rtas_builder_build_op_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RTAS_BUILDER_BUILD_OP_EXP_FLAG_COMPACT',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_RTAS_BUILDER_BUILD_OP_EXP_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION',value='ZE_BIT(1)'),
)

Enum(name='ze_rtas_builder_build_quality_hint_exp_t',
var1=VarDef(name='ZE_RTAS_BUILDER_BUILD_QUALITY_HINT_EXP_LOW',value='0'),
var2=VarDef(name='ZE_RTAS_BUILDER_BUILD_QUALITY_HINT_EXP_MEDIUM',value='1'),
var3=VarDef(name='ZE_RTAS_BUILDER_BUILD_QUALITY_HINT_EXP_HIGH',value='2'),
)

Enum(name='ze_rtas_builder_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RTAS_BUILDER_EXP_FLAG_RESERVED',value='ZE_BIT(0)'),
)

Enum(name='ze_rtas_builder_exp_version_t',
var1=VarDef(name='ZE_RTAS_BUILDER_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_RTAS_BUILDER_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_rtas_builder_geometry_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RTAS_BUILDER_GEOMETRY_EXP_FLAG_NON_OPAQUE',value='ZE_BIT(0)'),
)

Enum(name='ze_rtas_builder_geometry_type_exp_t',
var1=VarDef(name='ZE_RTAS_BUILDER_GEOMETRY_TYPE_EXP_TRIANGLES',value='0'),
var2=VarDef(name='ZE_RTAS_BUILDER_GEOMETRY_TYPE_EXP_QUADS',value='1'),
var3=VarDef(name='ZE_RTAS_BUILDER_GEOMETRY_TYPE_EXP_PROCEDURAL',value='2'),
var4=VarDef(name='ZE_RTAS_BUILDER_GEOMETRY_TYPE_EXP_INSTANCE',value='3'),
)

Enum(name='ze_rtas_builder_input_data_format_exp_t',
var1=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_FLOAT3',value='0'),
var2=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_FLOAT3X4_COLUMN_MAJOR',value='1'),
var3=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_FLOAT3X4_ALIGNED_COLUMN_MAJOR',value='2'),
var4=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_FLOAT3X4_ROW_MAJOR',value='3'),
var5=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_AABB',value='4'),
var6=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_TRIANGLE_INDICES_UINT32',value='5'),
var7=VarDef(name='ZE_RTAS_BUILDER_INPUT_DATA_FORMAT_EXP_QUAD_INDICES_UINT32',value='6'),
)

Enum(name='ze_rtas_builder_instance_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RTAS_BUILDER_INSTANCE_EXP_FLAG_TRIANGLE_CULL_DISABLE',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_RTAS_BUILDER_INSTANCE_EXP_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_RTAS_BUILDER_INSTANCE_EXP_FLAG_TRIANGLE_FORCE_OPAQUE',value='ZE_BIT(2)'),
var4=VarDef(name='ZE_RTAS_BUILDER_INSTANCE_EXP_FLAG_TRIANGLE_FORCE_NON_OPAQUE',value='ZE_BIT(3)'),
)

Enum(name='ze_rtas_device_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RTAS_DEVICE_EXP_FLAG_RESERVED',value='ZE_BIT(0)'),
)

Enum(name='ze_rtas_format_exp_t',
var1=VarDef(name='ZE_RTAS_FORMAT_EXP_INVALID',value='0'),
)

Enum(name='ze_rtas_parallel_operation_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_RTAS_PARALLEL_OPERATION_EXP_FLAG_RESERVED',value='ZE_BIT(0)'),
)

Enum(name='ze_sampler_address_mode_t',
var1=VarDef(name='ZE_SAMPLER_ADDRESS_MODE_NONE',value='0'),
var2=VarDef(name='ZE_SAMPLER_ADDRESS_MODE_REPEAT',value='1'),
var3=VarDef(name='ZE_SAMPLER_ADDRESS_MODE_CLAMP',value='2'),
var4=VarDef(name='ZE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER',value='3'),
var5=VarDef(name='ZE_SAMPLER_ADDRESS_MODE_MIRROR',value='4'),
)

Enum(name='ze_sampler_filter_mode_t',
var1=VarDef(name='ZE_SAMPLER_FILTER_MODE_NEAREST',value='0'),
var2=VarDef(name='ZE_SAMPLER_FILTER_MODE_LINEAR',value='1'),
)

Enum(name='ze_scheduling_hint_exp_flags_t',bitFields=True,
var1=VarDef(name='ZE_SCHEDULING_HINT_EXP_FLAG_OLDEST_FIRST',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_SCHEDULING_HINT_EXP_FLAG_ROUND_ROBIN',value='ZE_BIT(1)'),
var3=VarDef(name='ZE_SCHEDULING_HINT_EXP_FLAG_STALL_BASED_ROUND_ROBIN',value='ZE_BIT(2)'),
)

Enum(name='ze_scheduling_hints_exp_version_t',
var1=VarDef(name='ZE_SCHEDULING_HINTS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_SCHEDULING_HINTS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_srgb_ext_version_t',
var1=VarDef(name='ZE_SRGB_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_SRGB_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_structure_type_t',
var1=VarDef(name='ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES',value='0x1',struct='ze_driver_properties_t'),
var2=VarDef(name='ZE_STRUCTURE_TYPE_DRIVER_IPC_PROPERTIES',value='0x2',struct='ze_driver_ipc_properties_t'),
var3=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES',value='0x3',struct='ze_device_properties_t'),
var4=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES',value='0x4',struct='ze_device_compute_properties_t'),
var5=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES',value='0x5',struct='ze_device_module_properties_t'),
var6=VarDef(name='ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES',value='0x6',struct='ze_command_queue_group_properties_t'),
var7=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES',value='0x7',struct='ze_device_memory_properties_t'),
var8=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES',value='0x8',struct='ze_device_memory_access_properties_t'),
var9=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES',value='0x9',struct='ze_device_cache_properties_t'),
var10=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES',value='0xa',struct='ze_device_image_properties_t'),
var11=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES',value='0xb',struct='ze_device_p2p_properties_t'),
var12=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_EXTERNAL_MEMORY_PROPERTIES',value='0xc',struct='ze_device_external_memory_properties_t'),
var13=VarDef(name='ZE_STRUCTURE_TYPE_CONTEXT_DESC',value='0xd',struct='ze_context_desc_t'),
var14=VarDef(name='ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC',value='0xe',struct='ze_command_queue_desc_t'),
var15=VarDef(name='ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC',value='0xf',struct='ze_command_list_desc_t'),
var16=VarDef(name='ZE_STRUCTURE_TYPE_EVENT_POOL_DESC',value='0x10',struct='ze_event_pool_desc_t'),
var17=VarDef(name='ZE_STRUCTURE_TYPE_EVENT_DESC',value='0x11',struct='ze_event_desc_t'),
var18=VarDef(name='ZE_STRUCTURE_TYPE_FENCE_DESC',value='0x12',struct='ze_fence_desc_t'),
var19=VarDef(name='ZE_STRUCTURE_TYPE_IMAGE_DESC',value='0x13',struct='ze_image_desc_t'),
var20=VarDef(name='ZE_STRUCTURE_TYPE_IMAGE_PROPERTIES',value='0x14',struct='ze_image_properties_t'),
var21=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC',value='0x15',struct='ze_device_mem_alloc_desc_t'),
var22=VarDef(name='ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC',value='0x16',struct='ze_host_mem_alloc_desc_t'),
var23=VarDef(name='ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES',value='0x17',struct='ze_memory_allocation_properties_t'),
var24=VarDef(name='ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC',value='0x18',struct='ze_external_memory_export_desc_t'),
var25=VarDef(name='ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD',value='0x19',struct='ze_external_memory_import_fd_t'),
var26=VarDef(name='ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD',value='0x1a',struct='ze_external_memory_export_fd_t'),
var27=VarDef(name='ZE_STRUCTURE_TYPE_MODULE_DESC',value='0x1b',struct='ze_module_desc_t'),
var28=VarDef(name='ZE_STRUCTURE_TYPE_MODULE_PROPERTIES',value='0x1c',struct='ze_module_properties_t'),
var29=VarDef(name='ZE_STRUCTURE_TYPE_KERNEL_DESC',value='0x1d',struct='ze_kernel_desc_t'),
var30=VarDef(name='ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES',value='0x1e',struct='ze_kernel_properties_t'),
var31=VarDef(name='ZE_STRUCTURE_TYPE_SAMPLER_DESC',value='0x1f',struct='ze_sampler_desc_t'),
var32=VarDef(name='ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC',value='0x20',struct='ze_physical_mem_desc_t'),
var33=VarDef(name='ZE_STRUCTURE_TYPE_KERNEL_PREFERRED_GROUP_SIZE_PROPERTIES',value='0x21',struct='ze_kernel_preferred_group_size_properties_t'),
var34=VarDef(name='ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32',value='0x22',struct='ze_external_memory_import_win32_handle_t'),
var35=VarDef(name='ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32',value='0x23',struct='ze_external_memory_export_win32_handle_t'),
var36=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_RAYTRACING_EXT_PROPERTIES',value='0x00010001',struct='ze_device_raytracing_ext_properties_t'),
var37=VarDef(name='ZE_STRUCTURE_TYPE_RAYTRACING_MEM_ALLOC_EXT_DESC',value='0x10002',struct='ze_raytracing_mem_alloc_ext_desc_t'),
var38=VarDef(name='ZE_STRUCTURE_TYPE_FLOAT_ATOMIC_EXT_PROPERTIES',value='0x10003',struct='ze_float_atomic_ext_properties_t'),
var39=VarDef(name='ZE_STRUCTURE_TYPE_CACHE_RESERVATION_EXT_DESC',value='0x10004',struct='ze_cache_reservation_ext_desc_t'),
var40=VarDef(name='ZE_STRUCTURE_TYPE_EU_COUNT_EXT',value='0x10005',struct='ze_eu_count_ext_t'),
var41=VarDef(name='ZE_STRUCTURE_TYPE_SRGB_EXT_DESC',value='0x10006',struct='ze_srgb_ext_desc_t'),
var42=VarDef(name='ZE_STRUCTURE_TYPE_LINKAGE_INSPECTION_EXT_DESC',value='0x10007',struct='ze_linkage_inspection_ext_desc_t'),
var43=VarDef(name='ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES',value='0x10008',struct='ze_pci_ext_properties_t'),
var44=VarDef(name='ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES',value='0x10009',struct='ze_driver_memory_free_ext_properties_t'),
var45=VarDef(name='ZE_STRUCTURE_TYPE_MEMORY_FREE_EXT_DESC',value='0x1000a',struct='ze_memory_free_ext_desc_t'),
var46=VarDef(name='ZE_STRUCTURE_TYPE_MEMORY_COMPRESSION_HINTS_EXT_DESC',value='0x1000b',struct='ze_memory_compression_hints_ext_desc_t'),
var47=VarDef(name='ZE_STRUCTURE_TYPE_IMAGE_ALLOCATION_EXT_PROPERTIES',value='0x1000c',struct='ze_image_allocation_ext_properties_t'),
var48=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_LUID_EXT_PROPERTIES',value='0x1000d',struct='ze_device_luid_ext_properties_t'),
var49=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_MEMORY_EXT_PROPERTIES',value='0x1000e',struct='ze_device_memory_ext_properties_t'),
var50=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_IP_VERSION_EXT',value='0x1000f',struct='ze_device_ip_version_ext_t'),
var51=VarDef(name='ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC',value='0x10010',struct='ze_image_view_planar_ext_desc_t'),
var52=VarDef(name='ZE_STRUCTURE_TYPE_EVENT_QUERY_KERNEL_TIMESTAMPS_EXT_PROPERTIES',value='0x10011',struct='ze_event_query_kernel_timestamps_ext_properties_t'),
# var53=VarDef(name='ZE_STRUCTURE_TYPE_EVENT_QUERY_KERNEL_TIMESTAMPS_RESULTS_EXT_PROPERTIES',value='0x10012',struct='ze_event_query_kernel_timestamps_results_ext_properties_t'),
# var54=VarDef(name='ZE_STRUCTURE_TYPE_KERNEL_MAX_GROUP_SIZE_EXT_PROPERTIES',value='0x10013',struct='ze_kernel_max_group_size_ext_properties_t'),
var53=VarDef(name='ZE_STRUCTURE_TYPE_RELAXED_ALLOCATION_LIMITS_EXP_DESC',value='0x00020001',struct='ze_relaxed_allocation_limits_exp_desc_t'),
# var56=VarDef(name='ZE_STRUCTURE_TYPE_MODULE_PROGRAM_EXP_DESC',value='0x00020002',struct='ze_module_program_exp_desc_t'),
var54=VarDef(name='ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_PROPERTIES',value='0x00020003',struct='ze_scheduling_hint_exp_properties_t'),
var55=VarDef(name='ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_DESC',value='0x00020004',struct='ze_scheduling_hint_exp_desc_t'),
var56=VarDef(name='ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXP_DESC',value='0x00020005',struct='ze_image_view_planar_exp_desc_t'),
var57=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2',value='0x00020006',struct='ze_device_properties_t'),
var58=VarDef(name='ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES',value='0x00020007',struct='ze_image_memory_properties_exp_t'),
var59=VarDef(name='ZE_STRUCTURE_TYPE_POWER_SAVING_HINT_EXP_DESC',value='0x00020008',struct='ze_context_power_saving_hint_exp_desc_t'),
var60=VarDef(name='ZE_STRUCTURE_TYPE_COPY_BANDWIDTH_EXP_PROPERTIES',value='0x00020009',struct='ze_copy_bandwidth_exp_properties_t'),
var61=VarDef(name='ZE_STRUCTURE_TYPE_DEVICE_P2P_BANDWIDTH_EXP_PROPERTIES',value='0x0002000A',struct='ze_device_p2p_bandwidth_exp_properties_t'),
var62=VarDef(name='ZE_STRUCTURE_TYPE_FABRIC_VERTEX_EXP_PROPERTIES',value='0x0002000B',struct='ze_fabric_vertex_exp_properties_t'),
var63=VarDef(name='ZE_STRUCTURE_TYPE_FABRIC_EDGE_EXP_PROPERTIES',value='0x0002000C',struct='ze_fabric_edge_exp_properties_t'),
# var67=VarDef(name='ZE_STRUCTURE_TYPE_MEMORY_SUB_ALLOCATIONS_EXP_PROPERTIES',value='0x0002000D',struct='ze_memory_sub_allocations_exp_properties_t'),
var64=VarDef(name='ZE_STRUCTURE_TYPE_RTAS_BUILDER_EXP_DESC',value='0x0002000E',struct='ze_rtas_builder_exp_desc_t'),
# var69=VarDef(name='ZE_STRUCTURE_TYPE_RTAS_BUILDER_BUILD_OP_EXP_DESC',value='0x0002000F',struct='ze_rtas_builder_build_op_exp_desc_t'),
var65=VarDef(name='ZE_STRUCTURE_TYPE_RTAS_BUILDER_EXP_PROPERTIES',value='0x00020010',struct='ze_rtas_builder_exp_properties_t'),
var66=VarDef(name='ZE_STRUCTURE_TYPE_RTAS_PARALLEL_OPERATION_EXP_PROPERTIES',value='0x00020011',struct='ze_rtas_parallel_operation_exp_properties_t'),
var67=VarDef(name='ZE_STRUCTURE_TYPE_RTAS_DEVICE_EXP_PROPERTIES',value='0x00020012',struct='ze_rtas_device_exp_properties_t'),
# var73=VarDef(name='ZE_STRUCTURE_TYPE_RTAS_GEOMETRY_AABBS_EXP_CB_PARAMS',value='0x00020013',struct='ze_rtas_geometry_aabbs_exp_cb_params_t'),
var68=VarDef(name='ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC',value='0x00020014',struct='ze_event_pool_counter_based_exp_desc_t'),
var69=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_PROPERTIES',value='0x00020015',struct='ze_mutable_command_list_exp_properties_t'),
var70=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC',value='0x00020016',struct='ze_mutable_command_list_exp_desc_t'),
var71=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC',value='0x00020017',struct='ze_mutable_command_id_exp_desc_t'),
var72=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_COMMANDS_EXP_DESC',value='0x00020018',struct='ze_mutable_commands_exp_desc_t'),
var73=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC',value='0x00020019',struct='ze_mutable_kernel_argument_exp_desc_t'),
# var80=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC',value='0x0002001A',struct='ze_mutable_group_count_exp_desc_t'),
var74=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC',value='0x0002001B',struct='ze_mutable_group_size_exp_desc_t'),
var75=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_GLOBAL_OFFSET_EXP_DESC',value='0x0002001C',struct='ze_mutable_global_offset_exp_desc_t'),
var76=VarDef(name='ZE_STRUCTURE_TYPE_PITCHED_ALLOC_DEVICE_EXP_PROPERTIES',value='0x0002001D',struct='ze_device_pitched_alloc_exp_properties_t'),
var77=VarDef(name='ZE_STRUCTURE_TYPE_BINDLESS_IMAGE_EXP_DESC',value='0x0002001E',struct='ze_image_bindless_exp_desc_t'),
var78=VarDef(name='ZE_STRUCTURE_TYPE_PITCHED_IMAGE_EXP_DESC',value='0x0002001F',struct='ze_image_pitched_exp_desc_t'),
# var79=VarDef(name='ZE_STRUCTURE_TYPE_MUTABLE_GRAPH_ARGUMENT_EXP_DESC',value='0x00020020',struct='ze_mutable_graph_argument_exp_desc_t'),
var79=VarDef(name='ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC',value='0x00020021',struct='ze_init_driver_type_desc_t'),
)

Enum(name='zel_structure_type_t',
var1=VarDef(name='ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC',value='0x1',struct='zel_tracer_desc_t'),
)

Enum(name='ze_sub_allocations_exp_version_t',
var1=VarDef(name='ZE_SUB_ALLOCATIONS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_SUB_ALLOCATIONS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='ze_subgroup_ext_version_t',
var1=VarDef(name='ZE_SUBGROUP_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZE_SUBGROUP_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_control_state_t',
var1=VarDef(name='ZES_CONTROL_STATE_STATE_UNSET',value='0'),
var2=VarDef(name='ZES_CONTROL_STATE_STATE_ACTIVE',value='2'),
var3=VarDef(name='ZES_CONTROL_STATE_STATE_DISABLED',value='3'),
)

Enum(name='zes_device_action_t',
var1=VarDef(name='ZES_DEVICE_ACTION_NONE',value='0'),
var2=VarDef(name='ZES_DEVICE_ACTION_WARM_CARD_RESET',value='1'),
var3=VarDef(name='ZES_DEVICE_ACTION_COLD_CARD_RESET',value='2'),
var4=VarDef(name='ZES_DEVICE_ACTION_COLD_SYSTEM_REBOOT',value='3'),
)

Enum(name='zes_device_ecc_state_t',
var1=VarDef(name='ZES_DEVICE_ECC_STATE_UNAVAILABLE',value='0'),
var2=VarDef(name='ZES_DEVICE_ECC_STATE_ENABLED',value='1'),
var3=VarDef(name='ZES_DEVICE_ECC_STATE_DISABLED',value='2'),
)

Enum(name='zes_device_property_flags_t',bitFields=True,
var1=VarDef(name='ZES_DEVICE_PROPERTY_FLAG_INTEGRATED',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_DEVICE_PROPERTY_FLAG_SUBDEVICE',value='ZE_BIT(1)'),
var3=VarDef(name='ZES_DEVICE_PROPERTY_FLAG_ECC',value='ZE_BIT(2)'),
var4=VarDef(name='ZES_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING',value='ZE_BIT(3)'),
)

Enum(name='zes_device_type_t',
var1=VarDef(name='ZES_DEVICE_TYPE_GPU',value='1'),
var2=VarDef(name='ZES_DEVICE_TYPE_CPU',value='2'),
var3=VarDef(name='ZES_DEVICE_TYPE_FPGA',value='3'),
var4=VarDef(name='ZES_DEVICE_TYPE_MCA',value='4'),
var5=VarDef(name='ZES_DEVICE_TYPE_VPU',value='5'),
)

Enum(name='zes_diag_result_t',
var1=VarDef(name='ZES_DIAG_RESULT_NO_ERRORS',value='0'),
var2=VarDef(name='ZES_DIAG_RESULT_ABORT',value='1'),
var3=VarDef(name='ZES_DIAG_RESULT_FAIL_CANT_REPAIR',value='2'),
var4=VarDef(name='ZES_DIAG_RESULT_REBOOT_FOR_REPAIR',value='3'),
)

Enum(name='zes_engine_activity_ext_version_t',
var1=VarDef(name='ZES_ENGINE_ACTIVITY_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_ENGINE_ACTIVITY_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_engine_group_t',
var1=VarDef(name='ZES_ENGINE_GROUP_ALL',value='0'),
var2=VarDef(name='ZES_ENGINE_GROUP_COMPUTE_ALL',value='1'),
var3=VarDef(name='ZES_ENGINE_GROUP_MEDIA_ALL',value='2'),
var4=VarDef(name='ZES_ENGINE_GROUP_COPY_ALL',value='3'),
var5=VarDef(name='ZES_ENGINE_GROUP_COMPUTE_SINGLE',value='4'),
var6=VarDef(name='ZES_ENGINE_GROUP_RENDER_SINGLE',value='5'),
var7=VarDef(name='ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE',value='6'),
var8=VarDef(name='ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE',value='7'),
var9=VarDef(name='ZES_ENGINE_GROUP_COPY_SINGLE',value='8'),
var10=VarDef(name='ZES_ENGINE_GROUP_MEDIA_ENHANCEMENT_SINGLE',value='9'),
var11=VarDef(name='ZES_ENGINE_GROUP_3D_SINGLE',value='10'),
var12=VarDef(name='ZES_ENGINE_GROUP_3D_RENDER_COMPUTE_ALL',value='11'),
var13=VarDef(name='ZES_ENGINE_GROUP_RENDER_ALL',value='12'),
var14=VarDef(name='ZES_ENGINE_GROUP_3D_ALL',value='13'),
var15=VarDef(name='ZES_ENGINE_GROUP_MEDIA_CODEC_SINGLE',value='14'),
)

Enum(name='zes_engine_type_flags_t',bitFields=True,
var1=VarDef(name='ZES_ENGINE_TYPE_FLAG_OTHER',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_ENGINE_TYPE_FLAG_COMPUTE',value='ZE_BIT(1)'),
var3=VarDef(name='ZES_ENGINE_TYPE_FLAG_3D',value='ZE_BIT(2)'),
var4=VarDef(name='ZES_ENGINE_TYPE_FLAG_MEDIA',value='ZE_BIT(3)'),
var5=VarDef(name='ZES_ENGINE_TYPE_FLAG_DMA',value='ZE_BIT(4)'),
var6=VarDef(name='ZES_ENGINE_TYPE_FLAG_RENDER',value='ZE_BIT(5)'),
)

Enum(name='zes_event_type_flags_t',bitFields=True,
var1=VarDef(name='ZES_EVENT_TYPE_FLAG_DEVICE_DETACH',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH',value='ZE_BIT(1)'),
var3=VarDef(name='ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_ENTER',value='ZE_BIT(2)'),
var4=VarDef(name='ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_EXIT',value='ZE_BIT(3)'),
var5=VarDef(name='ZES_EVENT_TYPE_FLAG_FREQ_THROTTLED',value='ZE_BIT(4)'),
var6=VarDef(name='ZES_EVENT_TYPE_FLAG_ENERGY_THRESHOLD_CROSSED',value='ZE_BIT(5)'),
var7=VarDef(name='ZES_EVENT_TYPE_FLAG_TEMP_CRITICAL',value='ZE_BIT(6)'),
var8=VarDef(name='ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD1',value='ZE_BIT(7)'),
var9=VarDef(name='ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD2',value='ZE_BIT(8)'),
var10=VarDef(name='ZES_EVENT_TYPE_FLAG_MEM_HEALTH',value='ZE_BIT(9)'),
var11=VarDef(name='ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH',value='ZE_BIT(10)'),
var12=VarDef(name='ZES_EVENT_TYPE_FLAG_PCI_LINK_HEALTH',value='ZE_BIT(11)'),
var13=VarDef(name='ZES_EVENT_TYPE_FLAG_RAS_CORRECTABLE_ERRORS',value='ZE_BIT(12)'),
var14=VarDef(name='ZES_EVENT_TYPE_FLAG_RAS_UNCORRECTABLE_ERRORS',value='ZE_BIT(13)'),
var15=VarDef(name='ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED',value='ZE_BIT(14)'),
var16=VarDef(name='ZES_EVENT_TYPE_FLAG_SURVIVABILITY_MODE_DETECTED',value='ZE_BIT(15)'),
)

Enum(name='zes_fabric_port_failure_flags_t',bitFields=True,
var1=VarDef(name='ZES_FABRIC_PORT_FAILURE_FLAG_FAILED',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT',value='ZE_BIT(1)'),
var3=VarDef(name='ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING',value='ZE_BIT(2)'),
)

Enum(name='zes_fabric_port_qual_issue_flags_t',bitFields=True,
var1=VarDef(name='ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED',value='ZE_BIT(1)'),
)

Enum(name='zes_fabric_port_status_t',
var1=VarDef(name='ZES_FABRIC_PORT_STATUS_UNKNOWN',value='0'),
var2=VarDef(name='ZES_FABRIC_PORT_STATUS_HEALTHY',value='1'),
var3=VarDef(name='ZES_FABRIC_PORT_STATUS_DEGRADED',value='2'),
var4=VarDef(name='ZES_FABRIC_PORT_STATUS_FAILED',value='3'),
var5=VarDef(name='ZES_FABRIC_PORT_STATUS_DISABLED',value='4'),
)

Enum(name='zes_fan_speed_mode_t',
var1=VarDef(name='ZES_FAN_SPEED_MODE_DEFAULT',value='0'),
var2=VarDef(name='ZES_FAN_SPEED_MODE_FIXED',value='1'),
var3=VarDef(name='ZES_FAN_SPEED_MODE_TABLE',value='2'),
)

Enum(name='zes_fan_speed_units_t',
var1=VarDef(name='ZES_FAN_SPEED_UNITS_RPM',value='0'),
var2=VarDef(name='ZES_FAN_SPEED_UNITS_PERCENT',value='1'),
)

Enum(name='zes_firmware_security_exp_version_t',
var1=VarDef(name='ZES_FIRMWARE_SECURITY_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_FIRMWARE_SECURITY_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_freq_domain_t',
var1=VarDef(name='ZES_FREQ_DOMAIN_GPU',value='0'),
var2=VarDef(name='ZES_FREQ_DOMAIN_MEMORY',value='1'),
var3=VarDef(name='ZES_FREQ_DOMAIN_MEDIA',value='2'),
)

Enum(name='zes_freq_throttle_reason_flags_t',bitFields=True,
var1=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_BURST_PWR_CAP',value='ZE_BIT(1)'),
var3=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_CURRENT_LIMIT',value='ZE_BIT(2)'),
var4=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_THERMAL_LIMIT',value='ZE_BIT(3)'),
var5=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_PSU_ALERT',value='ZE_BIT(4)'),
var6=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_SW_RANGE',value='ZE_BIT(5)'),
var7=VarDef(name='ZES_FREQ_THROTTLE_REASON_FLAG_HW_RANGE',value='ZE_BIT(6)'),
)

Enum(name='zes_init_flags_t',bitFields=True,
var1=VarDef(name='ZES_INIT_FLAG_PLACEHOLDER',value='ZE_BIT(0)'),
)

Enum(name='zes_limit_unit_t',
var1=VarDef(name='ZES_LIMIT_UNIT_UNKNOWN',value='0'),
var2=VarDef(name='ZES_LIMIT_UNIT_CURRENT',value='1'),
var3=VarDef(name='ZES_LIMIT_UNIT_POWER',value='2'),
)

Enum(name='zes_mem_bandwidth_counter_bits_exp_version_t',
var1=VarDef(name='ZES_MEM_BANDWIDTH_COUNTER_BITS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_MEM_BANDWIDTH_COUNTER_BITS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_mem_health_t',
var1=VarDef(name='ZES_MEM_HEALTH_UNKNOWN',value='0'),
var2=VarDef(name='ZES_MEM_HEALTH_OK',value='1'),
var3=VarDef(name='ZES_MEM_HEALTH_DEGRADED',value='2'),
var4=VarDef(name='ZES_MEM_HEALTH_CRITICAL',value='3'),
var5=VarDef(name='ZES_MEM_HEALTH_REPLACE',value='4'),
)

Enum(name='zes_mem_loc_t',
var1=VarDef(name='ZES_MEM_LOC_SYSTEM',value='0'),
var2=VarDef(name='ZES_MEM_LOC_DEVICE',value='1'),
)

Enum(name='zes_mem_page_offline_state_exp_version_t',
var1=VarDef(name='ZES_MEM_PAGE_OFFLINE_STATE_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_MEM_PAGE_OFFLINE_STATE_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_mem_type_t',
var1=VarDef(name='ZES_MEM_TYPE_HBM',value='0'),
var2=VarDef(name='ZES_MEM_TYPE_DDR',value='1'),
var3=VarDef(name='ZES_MEM_TYPE_DDR3',value='2'),
var4=VarDef(name='ZES_MEM_TYPE_DDR4',value='3'),
var5=VarDef(name='ZES_MEM_TYPE_DDR5',value='4'),
var6=VarDef(name='ZES_MEM_TYPE_LPDDR',value='5'),
var7=VarDef(name='ZES_MEM_TYPE_LPDDR3',value='6'),
var8=VarDef(name='ZES_MEM_TYPE_LPDDR4',value='7'),
var9=VarDef(name='ZES_MEM_TYPE_LPDDR5',value='8'),
var10=VarDef(name='ZES_MEM_TYPE_SRAM',value='9'),
var11=VarDef(name='ZES_MEM_TYPE_L1',value='10'),
var12=VarDef(name='ZES_MEM_TYPE_L3',value='11'),
var13=VarDef(name='ZES_MEM_TYPE_GRF',value='12'),
var14=VarDef(name='ZES_MEM_TYPE_SLM',value='13'),
var15=VarDef(name='ZES_MEM_TYPE_GDDR4',value='14'),
var16=VarDef(name='ZES_MEM_TYPE_GDDR5',value='15'),
var17=VarDef(name='ZES_MEM_TYPE_GDDR5X',value='16'),
var18=VarDef(name='ZES_MEM_TYPE_GDDR6',value='17'),
var19=VarDef(name='ZES_MEM_TYPE_GDDR6X',value='18'),
var20=VarDef(name='ZES_MEM_TYPE_GDDR7',value='19'),
)

Enum(name='zes_oc_mode_t',
var1=VarDef(name='ZES_OC_MODE_OFF',value='0'),
var2=VarDef(name='ZES_OC_MODE_OVERRIDE',value='1'),
var3=VarDef(name='ZES_OC_MODE_INTERPOLATIVE',value='2'),
var4=VarDef(name='ZES_OC_MODE_FIXED',value='3'),
)

Enum(name='zes_overclock_control_t',
var1=VarDef(name='ZES_OVERCLOCK_CONTROL_VF',value='1'),
var2=VarDef(name='ZES_OVERCLOCK_CONTROL_FREQ_OFFSET',value='2'),
var3=VarDef(name='ZES_OVERCLOCK_CONTROL_VMAX_OFFSET',value='4'),
var4=VarDef(name='ZES_OVERCLOCK_CONTROL_FREQ',value='8'),
var5=VarDef(name='ZES_OVERCLOCK_CONTROL_VOLT_LIMIT',value='16'),
var6=VarDef(name='ZES_OVERCLOCK_CONTROL_POWER_SUSTAINED_LIMIT',value='32'),
var7=VarDef(name='ZES_OVERCLOCK_CONTROL_POWER_BURST_LIMIT',value='64'),
var8=VarDef(name='ZES_OVERCLOCK_CONTROL_POWER_PEAK_LIMIT',value='128'),
var9=VarDef(name='ZES_OVERCLOCK_CONTROL_ICCMAX_LIMIT',value='256'),
var10=VarDef(name='ZES_OVERCLOCK_CONTROL_TEMP_LIMIT',value='512'),
var11=VarDef(name='ZES_OVERCLOCK_CONTROL_ITD_DISABLE',value='1024'),
var12=VarDef(name='ZES_OVERCLOCK_CONTROL_ACM_DISABLE',value='2048'),
)

Enum(name='zes_overclock_domain_t',
var1=VarDef(name='ZES_OVERCLOCK_DOMAIN_CARD',value='1'),
var2=VarDef(name='ZES_OVERCLOCK_DOMAIN_PACKAGE',value='2'),
var3=VarDef(name='ZES_OVERCLOCK_DOMAIN_GPU_ALL',value='4'),
var4=VarDef(name='ZES_OVERCLOCK_DOMAIN_GPU_RENDER_COMPUTE',value='8'),
var5=VarDef(name='ZES_OVERCLOCK_DOMAIN_GPU_RENDER',value='16'),
var6=VarDef(name='ZES_OVERCLOCK_DOMAIN_GPU_COMPUTE',value='32'),
var7=VarDef(name='ZES_OVERCLOCK_DOMAIN_GPU_MEDIA',value='64'),
var8=VarDef(name='ZES_OVERCLOCK_DOMAIN_VRAM',value='128'),
var9=VarDef(name='ZES_OVERCLOCK_DOMAIN_ADM',value='256'),
)

Enum(name='zes_overclock_mode_t',
var1=VarDef(name='ZES_OVERCLOCK_MODE_MODE_OFF',value='0'),
var2=VarDef(name='ZES_OVERCLOCK_MODE_MODE_STOCK',value='2'),
var3=VarDef(name='ZES_OVERCLOCK_MODE_MODE_ON',value='3'),
var4=VarDef(name='ZES_OVERCLOCK_MODE_MODE_UNAVAILABLE',value='4'),
var5=VarDef(name='ZES_OVERCLOCK_MODE_MODE_DISABLED',value='5'),
)

Enum(name='zes_pci_bar_type_t',
var1=VarDef(name='ZES_PCI_BAR_TYPE_MMIO',value='0'),
var2=VarDef(name='ZES_PCI_BAR_TYPE_ROM',value='1'),
var3=VarDef(name='ZES_PCI_BAR_TYPE_MEM',value='2'),
)

Enum(name='zes_pci_link_qual_issue_flags_t',bitFields=True,
var1=VarDef(name='ZES_PCI_LINK_QUAL_ISSUE_FLAG_REPLAYS',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_PCI_LINK_QUAL_ISSUE_FLAG_SPEED',value='ZE_BIT(1)'),
)

Enum(name='zes_pci_link_stab_issue_flags_t',bitFields=True,
var1=VarDef(name='ZES_PCI_LINK_STAB_ISSUE_FLAG_RETRAINING',value='ZE_BIT(0)'),
)

Enum(name='zes_pci_link_status_t',
var1=VarDef(name='ZES_PCI_LINK_STATUS_UNKNOWN',value='0'),
var2=VarDef(name='ZES_PCI_LINK_STATUS_GOOD',value='1'),
var3=VarDef(name='ZES_PCI_LINK_STATUS_QUALITY_ISSUES',value='2'),
var4=VarDef(name='ZES_PCI_LINK_STATUS_STABILITY_ISSUES',value='3'),
)

Enum(name='zes_pending_action_t',
var1=VarDef(name='ZES_PENDING_ACTION_PENDING_NONE',value='0'),
var2=VarDef(name='ZES_PENDING_ACTION_PENDING_IMMINENT',value='1'),
var3=VarDef(name='ZES_PENDING_ACTION_PENDING_COLD_RESET',value='2'),
var4=VarDef(name='ZES_PENDING_ACTION_PENDING_WARM_RESET',value='3'),
)

Enum(name='zes_power_domain_properties_exp_version_t',
var1=VarDef(name='ZES_POWER_DOMAIN_PROPERTIES_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_POWER_DOMAIN_PROPERTIES_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_power_domain_t',
var1=VarDef(name='ZES_POWER_DOMAIN_UNKNOWN',value='0'),
var2=VarDef(name='ZES_POWER_DOMAIN_CARD',value='1'),
var3=VarDef(name='ZES_POWER_DOMAIN_PACKAGE',value='2'),
var4=VarDef(name='ZES_POWER_DOMAIN_STACK',value='3'),
var5=VarDef(name='ZES_POWER_DOMAIN_MEMORY',value='4'),
var6=VarDef(name='ZES_POWER_DOMAIN_GPU',value='5'),
)

Enum(name='zes_power_level_t',
var1=VarDef(name='ZES_POWER_LEVEL_UNKNOWN',value='0'),
var2=VarDef(name='ZES_POWER_LEVEL_SUSTAINED',value='1'),
var3=VarDef(name='ZES_POWER_LEVEL_BURST',value='2'),
var4=VarDef(name='ZES_POWER_LEVEL_PEAK',value='3'),
var5=VarDef(name='ZES_POWER_LEVEL_INSTANTANEOUS',value='4'),
)

Enum(name='zes_power_limits_ext_version_t',
var1=VarDef(name='ZES_POWER_LIMITS_EXT_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_POWER_LIMITS_EXT_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_power_source_t',
var1=VarDef(name='ZES_POWER_SOURCE_ANY',value='0'),
var2=VarDef(name='ZES_POWER_SOURCE_MAINS',value='1'),
var3=VarDef(name='ZES_POWER_SOURCE_BATTERY',value='2'),
)

Enum(name='zes_psu_voltage_status_t',
var1=VarDef(name='ZES_PSU_VOLTAGE_STATUS_UNKNOWN',value='0'),
var2=VarDef(name='ZES_PSU_VOLTAGE_STATUS_NORMAL',value='1'),
var3=VarDef(name='ZES_PSU_VOLTAGE_STATUS_OVER',value='2'),
var4=VarDef(name='ZES_PSU_VOLTAGE_STATUS_UNDER',value='3'),
)

Enum(name='zes_ras_error_cat_t',
var1=VarDef(name='ZES_RAS_ERROR_CAT_RESET',value='0'),
var2=VarDef(name='ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS',value='1'),
var3=VarDef(name='ZES_RAS_ERROR_CAT_DRIVER_ERRORS',value='2'),
var4=VarDef(name='ZES_RAS_ERROR_CAT_COMPUTE_ERRORS',value='3'),
var5=VarDef(name='ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS',value='4'),
var6=VarDef(name='ZES_RAS_ERROR_CAT_CACHE_ERRORS',value='5'),
var7=VarDef(name='ZES_RAS_ERROR_CAT_DISPLAY_ERRORS',value='6'),
)

Enum(name='zes_ras_error_category_exp_t',
var1=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_RESET',value='0'),
var2=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_PROGRAMMING_ERRORS',value='1'),
var3=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_DRIVER_ERRORS',value='2'),
var4=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_COMPUTE_ERRORS',value='3'),
var5=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_NON_COMPUTE_ERRORS',value='4'),
var6=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_CACHE_ERRORS',value='5'),
var7=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_DISPLAY_ERRORS',value='6'),
var8=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_MEMORY_ERRORS',value='7'),
var9=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_SCALE_ERRORS',value='8'),
var10=VarDef(name='ZES_RAS_ERROR_CATEGORY_EXP_L3FABRIC_ERRORS',value='9'),
)

Enum(name='zes_ras_error_type_t',
var1=VarDef(name='ZES_RAS_ERROR_TYPE_CORRECTABLE',value='0'),
var2=VarDef(name='ZES_RAS_ERROR_TYPE_UNCORRECTABLE',value='1'),
)

Enum(name='zes_ras_state_exp_version_t',
var1=VarDef(name='ZES_RAS_STATE_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_RAS_STATE_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_repair_status_t',
var1=VarDef(name='ZES_REPAIR_STATUS_UNSUPPORTED',value='0'),
var2=VarDef(name='ZES_REPAIR_STATUS_NOT_PERFORMED',value='1'),
var3=VarDef(name='ZES_REPAIR_STATUS_PERFORMED',value='2'),
)

Enum(name='zes_reset_reason_flags_t',bitFields=True,
var1=VarDef(name='ZES_RESET_REASON_FLAG_WEDGED',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_RESET_REASON_FLAG_REPAIR',value='ZE_BIT(1)'),
)

Enum(name='zes_reset_type_t',
var1=VarDef(name='ZES_RESET_TYPE_WARM',value='0'),
var2=VarDef(name='ZES_RESET_TYPE_COLD',value='1'),
var3=VarDef(name='ZES_RESET_TYPE_FLR',value='2'),
)

Enum(name='zes_sched_mode_t',
var1=VarDef(name='ZES_SCHED_MODE_TIMEOUT',value='0'),
var2=VarDef(name='ZES_SCHED_MODE_TIMESLICE',value='1'),
var3=VarDef(name='ZES_SCHED_MODE_EXCLUSIVE',value='2'),
var4=VarDef(name='ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG',value='3'),
)

Enum(name='zes_standby_promo_mode_t',
var1=VarDef(name='ZES_STANDBY_PROMO_MODE_DEFAULT',value='0'),
var2=VarDef(name='ZES_STANDBY_PROMO_MODE_NEVER',value='1'),
)

Enum(name='zes_standby_type_t',
var1=VarDef(name='ZES_STANDBY_TYPE_GLOBAL',value='0'),
)

Enum(name='zes_structure_type_t',
var1=VarDef(name='ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES',value='0x1',struct='zes_device_properties_t'),
var2=VarDef(name='ZES_STRUCTURE_TYPE_PCI_PROPERTIES',value='0x2',struct='zes_pci_properties_t'),
var3=VarDef(name='ZES_STRUCTURE_TYPE_PCI_BAR_PROPERTIES',value='0x3',struct='zes_pci_bar_properties_t'),
var4=VarDef(name='ZES_STRUCTURE_TYPE_DIAG_PROPERTIES',value='0x4',struct='zes_diag_properties_t'),
var5=VarDef(name='ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES',value='0x5',struct='zes_engine_properties_t'),
var6=VarDef(name='ZES_STRUCTURE_TYPE_FABRIC_PORT_PROPERTIES',value='0x6',struct='zes_fabric_port_properties_t'),
var7=VarDef(name='ZES_STRUCTURE_TYPE_FAN_PROPERTIES',value='0x7',struct='zes_fan_properties_t'),
var8=VarDef(name='ZES_STRUCTURE_TYPE_FIRMWARE_PROPERTIES',value='0x8',struct='zes_firmware_properties_t'),
var9=VarDef(name='ZES_STRUCTURE_TYPE_FREQ_PROPERTIES',value='0x9',struct='zes_freq_properties_t'),
var10=VarDef(name='ZES_STRUCTURE_TYPE_LED_PROPERTIES',value='0xa',struct='zes_led_properties_t'),
var11=VarDef(name='ZES_STRUCTURE_TYPE_MEM_PROPERTIES',value='0xb',struct='zes_mem_properties_t'),
var12=VarDef(name='ZES_STRUCTURE_TYPE_PERF_PROPERTIES',value='0xc',struct='zes_perf_properties_t'),
var13=VarDef(name='ZES_STRUCTURE_TYPE_POWER_PROPERTIES',value='0xd',struct='zes_power_properties_t'),
var14=VarDef(name='ZES_STRUCTURE_TYPE_PSU_PROPERTIES',value='0xe',struct='zes_psu_properties_t'),
var15=VarDef(name='ZES_STRUCTURE_TYPE_RAS_PROPERTIES',value='0xf',struct='zes_ras_properties_t'),
var16=VarDef(name='ZES_STRUCTURE_TYPE_SCHED_PROPERTIES',value='0x10',struct='zes_sched_properties_t'),
var17=VarDef(name='ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES',value='0x11',struct='zes_sched_timeout_properties_t'),
var18=VarDef(name='ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES',value='0x12',struct='zes_sched_timeslice_properties_t'),
var19=VarDef(name='ZES_STRUCTURE_TYPE_STANDBY_PROPERTIES',value='0x13',struct='zes_standby_properties_t'),
var20=VarDef(name='ZES_STRUCTURE_TYPE_TEMP_PROPERTIES',value='0x14',struct='zes_temp_properties_t'),
var21=VarDef(name='ZES_STRUCTURE_TYPE_DEVICE_STATE',value='0x15',struct='zes_device_state_t'),
var22=VarDef(name='ZES_STRUCTURE_TYPE_PROCESS_STATE',value='0x16',struct='zes_process_state_t'),
var23=VarDef(name='ZES_STRUCTURE_TYPE_PCI_STATE',value='0x17',struct='zes_pci_state_t'),
var24=VarDef(name='ZES_STRUCTURE_TYPE_FABRIC_PORT_CONFIG',value='0x18',struct='zes_fabric_port_config_t'),
var25=VarDef(name='ZES_STRUCTURE_TYPE_FABRIC_PORT_STATE',value='0x19',struct='zes_fabric_port_state_t'),
var26=VarDef(name='ZES_STRUCTURE_TYPE_FAN_CONFIG',value='0x1a',struct='zes_fan_config_t'),
var27=VarDef(name='ZES_STRUCTURE_TYPE_FREQ_STATE',value='0x1b',struct='zes_freq_state_t'),
var28=VarDef(name='ZES_STRUCTURE_TYPE_OC_CAPABILITIES',value='0x1c',struct='zes_oc_capabilities_t'),
var29=VarDef(name='ZES_STRUCTURE_TYPE_LED_STATE',value='0x1d',struct='zes_led_state_t'),
var30=VarDef(name='ZES_STRUCTURE_TYPE_MEM_STATE',value='0x1e',struct='zes_mem_state_t'),
var31=VarDef(name='ZES_STRUCTURE_TYPE_PSU_STATE',value='0x1f',struct='zes_psu_state_t'),
var32=VarDef(name='ZES_STRUCTURE_TYPE_BASE_STATE',value='0x20',struct='zes_base_state_t'),
var33=VarDef(name='ZES_STRUCTURE_TYPE_RAS_CONFIG',value='0x21',struct='zes_ras_config_t'),
var34=VarDef(name='ZES_STRUCTURE_TYPE_RAS_STATE',value='0x22',struct='zes_ras_state_t'),
var35=VarDef(name='ZES_STRUCTURE_TYPE_TEMP_CONFIG',value='0x23',struct='zes_temp_config_t'),
var36=VarDef(name='ZES_STRUCTURE_TYPE_PCI_BAR_PROPERTIES_1_2',value='0x24',struct='zes_pci_bar_properties_1_2_t'),
var37=VarDef(name='ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC',value='0x25',struct='zes_device_ecc_desc_t'),
var38=VarDef(name='ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES',value='0x26',struct='zes_device_ecc_properties_t'),
var39=VarDef(name='ZES_STRUCTURE_TYPE_POWER_LIMIT_EXT_DESC',value='0x27',struct='zes_power_limit_ext_desc_t'),
# var40=VarDef(name='ZES_STRUCTURE_TYPE_POWER_EXT_PROPERTIES',value='0x28',struct='zes_power_ext_properties_t'),
var40=VarDef(name='ZES_STRUCTURE_TYPE_OVERCLOCK_PROPERTIES',value='0x29',struct='zes_overclock_properties_t'),
var41=VarDef(name='ZES_STRUCTURE_TYPE_FABRIC_PORT_ERROR_COUNTERS',value='0x2a',struct='zes_fabric_port_error_counters_t'),
var42=VarDef(name='ZES_STRUCTURE_TYPE_ENGINE_EXT_PROPERTIES',value='0x2b',struct='zes_engine_ext_properties_t'),
var43=VarDef(name='ZES_STRUCTURE_TYPE_RESET_PROPERTIES',value='0x2c',struct='zes_reset_properties_t'),
var44=VarDef(name='ZES_STRUCTURE_TYPE_DEVICE_EXT_PROPERTIES',value='0x2d',struct='zes_device_ext_properties_t'),
var45=VarDef(name='ZES_STRUCTURE_TYPE_DEVICE_UUID',value='0x2e',struct='zes_uuid_t'),
var46=VarDef(name='ZES_STRUCTURE_TYPE_POWER_DOMAIN_EXP_PROPERTIES',value='0x00020001',struct='zes_power_domain_exp_properties_t'),
var47=VarDef(name='ZES_STRUCTURE_TYPE_MEM_BANDWIDTH_COUNTER_BITS_EXP_PROPERTIES',value='0x00020002',struct='zes_mem_bandwidth_counter_bits_exp_properties_t'),
var48=VarDef(name='ZES_STRUCTURE_TYPE_MEMORY_PAGE_OFFLINE_STATE_EXP',value='0x00020003',struct='zes_mem_page_offline_state_exp_t'),
var49=VarDef(name='ZES_STRUCTURE_TYPE_SUBDEVICE_EXP_PROPERTIES',value='0x00020004',struct='zes_subdevice_exp_properties_t'),
var50=VarDef(name='ZES_STRUCTURE_TYPE_VF_EXP_PROPERTIES',value='0x00020005',struct='zes_vf_exp_properties_t'),
var51=VarDef(name='ZES_STRUCTURE_TYPE_VF_UTIL_MEM_EXP',value='0x00020006',struct='zes_vf_util_mem_exp_t'),
var52=VarDef(name='ZES_STRUCTURE_TYPE_VF_UTIL_ENGINE_EXP',value='0x00020007',struct='zes_vf_util_engine_exp_t'),
var53=VarDef(name='ZES_STRUCTURE_TYPE_VF_EXP_CAPABILITIES',value='0x00020008',struct='zes_vf_exp_capabilities_t'),
var54=VarDef(name='ZES_STRUCTURE_TYPE_VF_UTIL_MEM_EXP2',value='0x00020009',struct='zes_vf_util_mem_exp2_t'),
var55=VarDef(name='ZES_STRUCTURE_TYPE_VF_UTIL_ENGINE_EXP2',value='0x00020010',struct='zes_vf_util_engine_exp2_t'),
)

Enum(name='zes_sysman_device_mapping_exp_version_t',
var1=VarDef(name='ZES_SYSMAN_DEVICE_MAPPING_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZES_SYSMAN_DEVICE_MAPPING_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zes_temp_sensors_t',
var1=VarDef(name='ZES_TEMP_SENSORS_GLOBAL',value='0'),
var2=VarDef(name='ZES_TEMP_SENSORS_GPU',value='1'),
var3=VarDef(name='ZES_TEMP_SENSORS_MEMORY',value='2'),
var4=VarDef(name='ZES_TEMP_SENSORS_GLOBAL_MIN',value='3'),
var5=VarDef(name='ZES_TEMP_SENSORS_GPU_MIN',value='4'),
var6=VarDef(name='ZES_TEMP_SENSORS_MEMORY_MIN',value='5'),
var7=VarDef(name='ZES_TEMP_SENSORS_GPU_BOARD',value='6'),
var8=VarDef(name='ZES_TEMP_SENSORS_GPU_BOARD_MIN',value='7'),
var9=VarDef(name='ZES_TEMP_SENSORS_VOLTAGE_REGULATOR',value='8'),
)

Enum(name='zes_vf_array_type_t',
var1=VarDef(name='ZES_VF_ARRAY_TYPE_USER_VF_ARRAY',value='0'),
var2=VarDef(name='ZES_VF_ARRAY_TYPE_DEFAULT_VF_ARRAY',value='1'),
var3=VarDef(name='ZES_VF_ARRAY_TYPE_LIVE_VF_ARRAY',value='2'),
)

Enum(name='zes_vf_info_mem_type_exp_flags_t',bitFields=True,
var1=VarDef(name='ZES_VF_INFO_MEM_TYPE_EXP_FLAG_MEM_TYPE_SYSTEM',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_VF_INFO_MEM_TYPE_EXP_FLAG_MEM_TYPE_DEVICE',value='ZE_BIT(1)'),
)

Enum(name='zes_vf_info_util_exp_flags_t',bitFields=True,
var1=VarDef(name='ZES_VF_INFO_UTIL_EXP_FLAG_INFO_NONE',value='ZE_BIT(0)'),
var2=VarDef(name='ZES_VF_INFO_UTIL_EXP_FLAG_INFO_MEM_CPU',value='ZE_BIT(1)'),
var3=VarDef(name='ZES_VF_INFO_UTIL_EXP_FLAG_INFO_MEM_GPU',value='ZE_BIT(2)'),
var4=VarDef(name='ZES_VF_INFO_UTIL_EXP_FLAG_INFO_ENGINE',value='ZE_BIT(3)'),
)

Enum(name='zes_vf_management_exp_version_t',
var1=VarDef(name='ZES_VF_MANAGEMENT_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
var2=VarDef(name='ZES_VF_MANAGEMENT_EXP_VERSION_1_1',value='ZE_MAKE_VERSION( 1, 1 )'),
var3=VarDef(name='ZES_VF_MANAGEMENT_EXP_VERSION_1_2',value='ZE_MAKE_VERSION( 1, 2 )'),
# var4=VarDef(name='ZES_VF_MANAGEMENT_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 2 )'),
)

Enum(name='zes_vf_program_type_t',
var1=VarDef(name='ZES_VF_PROGRAM_TYPE_VF_ARBITRARY',value='0'),
var2=VarDef(name='ZES_VF_PROGRAM_TYPE_VF_FREQ_FIXED',value='1'),
var3=VarDef(name='ZES_VF_PROGRAM_TYPE_VF_VOLT_FIXED',value='2'),
)

Enum(name='zes_vf_type_t',
var1=VarDef(name='ZES_VF_TYPE_VOLT',value='0'),
var2=VarDef(name='ZES_VF_TYPE_FREQ',value='1'),
)

Enum(name='zet_api_tracing_exp_version_t',
var1=VarDef(name='ZET_API_TRACING_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZET_API_TRACING_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zet_concurrent_metric_groups_exp_version_t',
var1=VarDef(name='ZET_CONCURRENT_METRIC_GROUPS_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZET_CONCURRENT_METRIC_GROUPS_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zet_debug_detach_reason_t',
var1=VarDef(name='ZET_DEBUG_DETACH_REASON_INVALID',value='0'),
var2=VarDef(name='ZET_DEBUG_DETACH_REASON_HOST_EXIT',value='1'),
)

Enum(name='zet_debug_event_flags_t',bitFields=True,
var1=VarDef(name='ZET_DEBUG_EVENT_FLAG_NEED_ACK',value='ZE_BIT(0)'),
)

Enum(name='zet_debug_event_type_t',
var1=VarDef(name='ZET_DEBUG_EVENT_TYPE_INVALID',value='0'),
var2=VarDef(name='ZET_DEBUG_EVENT_TYPE_DETACHED',value='1'),
var3=VarDef(name='ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY',value='2'),
var4=VarDef(name='ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT',value='3'),
var5=VarDef(name='ZET_DEBUG_EVENT_TYPE_MODULE_LOAD',value='4'),
var6=VarDef(name='ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD',value='5'),
var7=VarDef(name='ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED',value='6'),
var8=VarDef(name='ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE',value='7'),
var9=VarDef(name='ZET_DEBUG_EVENT_TYPE_PAGE_FAULT',value='8'),
)

Enum(name='zet_debug_memory_space_type_t',
var1=VarDef(name='ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT',value='0'),
var2=VarDef(name='ZET_DEBUG_MEMORY_SPACE_TYPE_SLM',value='1'),
var3=VarDef(name='ZET_DEBUG_MEMORY_SPACE_TYPE_ELF',value='2'),
)

Enum(name='zet_debug_page_fault_reason_t',
var1=VarDef(name='ZET_DEBUG_PAGE_FAULT_REASON_INVALID',value='0'),
var2=VarDef(name='ZET_DEBUG_PAGE_FAULT_REASON_MAPPING_ERROR',value='1'),
var3=VarDef(name='ZET_DEBUG_PAGE_FAULT_REASON_PERMISSION_ERROR',value='2'),
)

Enum(name='zet_debug_regset_flags_t',bitFields=True,
var1=VarDef(name='ZET_DEBUG_REGSET_FLAG_READABLE',value='ZE_BIT(0)'),
var2=VarDef(name='ZET_DEBUG_REGSET_FLAG_WRITEABLE',value='ZE_BIT(1)'),
)

Enum(name='zet_device_debug_property_flags_t',bitFields=True,
var1=VarDef(name='ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH',value='ZE_BIT(0)'),
)

Enum(name='zet_export_metric_data_exp_version_t',
var1=VarDef(name='ZET_EXPORT_METRIC_DATA_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZET_EXPORT_METRIC_DATA_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zet_metric_group_calculation_type_t',
var1=VarDef(name='ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES',value='0'),
var2=VarDef(name='ZET_METRIC_GROUP_CALCULATION_TYPE_MAX_METRIC_VALUES',value='1'),
)

Enum(name='zet_metric_group_sampling_type_flags_t',bitFields=True,
var1=VarDef(name='ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED',value='ZE_BIT(0)'),
var2=VarDef(name='ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED',value='ZE_BIT(1)'),
var3=VarDef(name='ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED',value='ZE_BIT(2)'),
)

Enum(name='zet_metric_group_type_exp_flags_t',bitFields=True,
var1=VarDef(name='ZET_METRIC_GROUP_TYPE_EXP_FLAG_EXPORT_DMA_BUF',value='ZE_BIT(0)'),
var2=VarDef(name='ZET_METRIC_GROUP_TYPE_EXP_FLAG_USER_CREATED',value='ZE_BIT(1)'),
var3=VarDef(name='ZET_METRIC_GROUP_TYPE_EXP_FLAG_OTHER',value='ZE_BIT(2)'),
)

Enum(name='zet_metric_programmable_exp_version_t',
var1=VarDef(name='ZET_METRIC_PROGRAMMABLE_EXP_VERSION_1_1',value='ZE_MAKE_VERSION( 1, 1 )'),
# var2=VarDef(name='ZET_METRIC_PROGRAMMABLE_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 1 )'),
)

Enum(name='zet_metric_programmable_param_type_exp_t',
var1=VarDef(name='ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_DISAGGREGATION',value='0'),
var2=VarDef(name='ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_LATENCY',value='1'),
var3=VarDef(name='ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_UTILIZATION',value='2'),
var4=VarDef(name='ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_AVERAGE',value='3'),
var5=VarDef(name='ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_RATE',value='4'),
var6=VarDef(name='ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_BYTES',value='5'),
)

Enum(name='zet_metric_query_pool_type_t',
var1=VarDef(name='ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE',value='0'),
var2=VarDef(name='ZET_METRIC_QUERY_POOL_TYPE_EXECUTION',value='1'),
)

Enum(name='zet_metric_tracer_exp_version_t',
var1=VarDef(name='ZET_METRIC_TRACER_EXP_VERSION_1_0',value='ZE_MAKE_VERSION( 1, 0 )'),
# var2=VarDef(name='ZET_METRIC_TRACER_EXP_VERSION_CURRENT',value='ZE_MAKE_VERSION( 1, 0 )'),
)

Enum(name='zet_metric_type_t',
var1=VarDef(name='ZET_METRIC_TYPE_DURATION',value='0'),
var2=VarDef(name='ZET_METRIC_TYPE_EVENT',value='1'),
var3=VarDef(name='ZET_METRIC_TYPE_EVENT_WITH_RANGE',value='2'),
var4=VarDef(name='ZET_METRIC_TYPE_THROUGHPUT',value='3'),
var5=VarDef(name='ZET_METRIC_TYPE_TIMESTAMP',value='4'),
var6=VarDef(name='ZET_METRIC_TYPE_FLAG',value='5'),
var7=VarDef(name='ZET_METRIC_TYPE_RATIO',value='6'),
var8=VarDef(name='ZET_METRIC_TYPE_RAW',value='7'),
var9=VarDef(name='ZET_METRIC_TYPE_IP_EXP',value='8'),
var10=VarDef(name='ZET_METRIC_TYPE_IP',value='9'),
var11=VarDef(name='ZET_METRIC_TYPE_EVENT_EXP_TIMESTAMP',value='10'),
var12=VarDef(name='ZET_METRIC_TYPE_EVENT_EXP_START',value='11'),
var13=VarDef(name='ZET_METRIC_TYPE_EVENT_EXP_END',value='12'),
var14=VarDef(name='ZET_METRIC_TYPE_EXPORT_DMA_BUF',value='0x7ffffffd'),
var15=VarDef(name='ZET_METRIC_TYPE_EVENT_EXP_MONOTONIC_WRAPS_VALUE',value='0x7ffffffe'),
)

Enum(name='zet_module_debug_info_format_t',
var1=VarDef(name='ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF',value='0'),
)

Enum(name='zet_profile_flags_t',bitFields=True,
var1=VarDef(name='ZET_PROFILE_FLAG_REGISTER_REALLOCATION',value='ZE_BIT(0)'),
var2=VarDef(name='ZET_PROFILE_FLAG_FREE_REGISTER_INFO',value='ZE_BIT(1)'),
)

Enum(name='zet_profile_token_type_t',
var1=VarDef(name='ZET_PROFILE_TOKEN_TYPE_FREE_REGISTER',value='0'),
)

Enum(name='zet_structure_type_t',
var1=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES',value='0x1',struct='zet_metric_group_properties_t'),
var2=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_PROPERTIES',value='0x2',struct='zet_metric_properties_t'),
var3=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC',value='0x3',struct='zet_metric_streamer_desc_t'),
var4=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_QUERY_POOL_DESC',value='0x4',struct='zet_metric_query_pool_desc_t'),
var5=VarDef(name='ZET_STRUCTURE_TYPE_PROFILE_PROPERTIES',value='0x5',struct='zet_profile_properties_t'),
var6=VarDef(name='ZET_STRUCTURE_TYPE_DEVICE_DEBUG_PROPERTIES',value='0x6',struct='zet_device_debug_properties_t'),
var7=VarDef(name='ZET_STRUCTURE_TYPE_DEBUG_MEMORY_SPACE_DESC',value='0x7',struct='zet_debug_memory_space_desc_t'),
var8=VarDef(name='ZET_STRUCTURE_TYPE_DEBUG_REGSET_PROPERTIES',value='0x8',struct='zet_debug_regset_properties_t'),
var9=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_GLOBAL_TIMESTAMPS_RESOLUTION_EXP',value='0x9',struct='zet_metric_global_timestamps_resolution_exp_t'),
var10=VarDef(name='ZET_STRUCTURE_TYPE_TRACER_EXP_DESC',value='0x00010001',struct='zet_tracer_exp_desc_t'),
var11=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_CALCULATE_EXP_DESC',value='0x00010002',struct='zet_metric_calculate_exp_desc_t'),
var12=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_EXP_PROPERTIES',value='0x00010003',struct='zet_metric_programmable_exp_properties_t'),
var13=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_PARAM_INFO_EXP',value='0x00010004',struct='zet_metric_programmable_param_info_exp_t'),
var14=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_PARAM_VALUE_INFO_EXP',value='0x00010005',struct='zet_metric_programmable_param_value_info_exp_t'),
var15=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_GROUP_TYPE_EXP',value='0x00010006',struct='zet_metric_group_type_exp_t'),
var16=VarDef(name='ZET_STRUCTURE_TYPE_EXPORT_DMA_EXP_PROPERTIES',value='0x00010007',struct='zet_export_dma_buf_exp_properties_t'),
var17=VarDef(name='ZET_STRUCTURE_TYPE_METRIC_TRACER_EXP_DESC',value='0x00010008',struct='zet_metric_tracer_exp_desc_t'),
)

Enum(name='zet_value_info_type_exp_t',
var1=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_UINT32',value='0'),
var2=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_UINT64',value='1'),
var3=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_FLOAT32',value='2'),
var4=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_FLOAT64',value='3'),
var5=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_BOOL8',value='4'),
var6=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_UINT8',value='5'),
var7=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_UINT16',value='6'),
var8=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_UINT64_RANGE',value='7'),
var9=VarDef(name='ZET_VALUE_INFO_TYPE_EXP_FLOAT64_RANGE',value='8'),
)

Enum(name='zet_value_type_t',
var1=VarDef(name='ZET_VALUE_TYPE_UINT32',value='0'),
var2=VarDef(name='ZET_VALUE_TYPE_UINT64',value='1'),
var3=VarDef(name='ZET_VALUE_TYPE_FLOAT32',value='2'),
var4=VarDef(name='ZET_VALUE_TYPE_FLOAT64',value='3'),
var5=VarDef(name='ZET_VALUE_TYPE_BOOL8',value='4'),
var6=VarDef(name='ZET_VALUE_TYPE_STRING',value='5'),
var7=VarDef(name='ZET_VALUE_TYPE_UINT8',value='6'),
var8=VarDef(name='ZET_VALUE_TYPE_UINT16',value='7'),
)

Enum(name='ze_gits_recording_info_t',bitFields=True,
var1=VarDef(name='ZE_GITS_RECORDING_DEFAULT',value='ZE_BIT(0)'),
var2=VarDef(name='ZE_GITS_SWITCH_NOMENCLATURE_COUNTING',value='ZE_BIT(1)'),
)

Argument(name='ze_driver_handle_t',obj=True)

Argument(name='ze_device_handle_t',obj=True)

Argument(name='ze_context_handle_t',obj=True)

Argument(name='ze_command_queue_handle_t',obj=True)

Argument(name='ze_command_list_handle_t',obj=True)

Argument(name='ze_fence_handle_t',obj=True)

Argument(name='ze_event_pool_handle_t',obj=True)

Argument(name='ze_event_handle_t',obj=True)

Argument(name='ze_image_handle_t',obj=True)

Argument(name='ze_module_handle_t',obj=True)

Argument(name='ze_module_build_log_handle_t',obj=True)

Argument(name='ze_kernel_handle_t',obj=True)

Argument(name='ze_sampler_handle_t',obj=True)

Argument(name='ze_physical_mem_handle_t',obj=True)

Argument(name='ze_fabric_vertex_handle_t',obj=True)

Argument(name='ze_fabric_edge_handle_t',obj=True)

Argument(name='zel_tracer_handle_t',obj=True)

Argument(name='zel_tracer_desc_t',
var1=VarDef(name='stype',type='zel_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTracer'),
var3=VarDef(name='pUserData',type='const void*',tag='in',wrapType='COutArgument'),
)

Argument(name='ze_ipc_mem_handle_t',
var1=VarDef(name='data[ZE_MAX_IPC_HANDLE_SIZE]',type='char',tag='out'),
)

Argument(name='ze_ipc_event_pool_handle_t',
var1=VarDef(name='data[ZE_MAX_IPC_HANDLE_SIZE]',type='char',tag='out'),
)

Argument(name='ze_uuid_t',
var1=VarDef(name='id[ZE_MAX_UUID_SIZE]',type='uint8_t',tag='out'),
)

Argument(name='ze_base_cb_params_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
)

Argument(name='ze_base_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
)

Argument(name='ze_base_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
)

Argument(name='ze_init_driver_type_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_init_driver_type_flags_t',tag='in'),
)

Argument(name='ze_driver_uuid_t',
var1=VarDef(name='id[ZE_MAX_DRIVER_UUID_SIZE]',type='uint8_t',tag='out'),
)

Argument(name='ze_driver_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='uuid',type='ze_driver_uuid_t',tag='out'),
var4=VarDef(name='driverVersion',type='uint32_t',tag='out'),
)

Argument(name='ze_driver_ipc_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_ipc_property_flags_t',tag='out'),
)

Argument(name='ze_driver_extension_properties_t',
var1=VarDef(name='name[ZE_MAX_EXTENSION_NAME]',type='char',tag='out'),
var2=VarDef(name='version',type='uint32_t',tag='out'),
)

Argument(name='ze_device_uuid_t',
var1=VarDef(name='id[ZE_MAX_DEVICE_UUID_SIZE]',type='uint8_t',tag='out'),
)

Argument(name='ze_device_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='type',type='ze_device_type_t',tag='out'),
var4=VarDef(name='vendorId',type='uint32_t',tag='out'),
var5=VarDef(name='deviceId',type='uint32_t',tag='out'),
var6=VarDef(name='flags',type='ze_device_property_flags_t',tag='out'),
var7=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var8=VarDef(name='coreClockRate',type='uint32_t',tag='out'),
var9=VarDef(name='maxMemAllocSize',type='uint64_t',tag='out'),
var10=VarDef(name='maxHardwareContexts',type='uint32_t',tag='out'),
var11=VarDef(name='maxCommandQueuePriority',type='uint32_t',tag='out'),
var12=VarDef(name='numThreadsPerEU',type='uint32_t',tag='out'),
var13=VarDef(name='physicalEUSimdWidth',type='uint32_t',tag='out'),
var14=VarDef(name='numEUsPerSubslice',type='uint32_t',tag='out'),
var15=VarDef(name='numSubslicesPerSlice',type='uint32_t',tag='out'),
var16=VarDef(name='numSlices',type='uint32_t',tag='out'),
var17=VarDef(name='timerResolution',type='uint64_t',tag='out'),
var18=VarDef(name='timestampValidBits',type='uint32_t',tag='out'),
var19=VarDef(name='kernelTimestampValidBits',type='uint32_t',tag='out'),
var20=VarDef(name='uuid',type='ze_device_uuid_t',tag='out'),
var21=VarDef(name='name[ZE_MAX_DEVICE_NAME]',type='char',tag='out'),
)

Argument(name='ze_device_thread_t',
var1=VarDef(name='slice',type='uint32_t',tag='inout'),
var2=VarDef(name='subslice',type='uint32_t',tag='inout'),
var3=VarDef(name='eu',type='uint32_t',tag='inout'),
var4=VarDef(name='thread',type='uint32_t',tag='inout'),
)

Argument(name='ze_device_compute_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='maxTotalGroupSize',type='uint32_t',tag='out'),
var4=VarDef(name='maxGroupSizeX',type='uint32_t',tag='out'),
var5=VarDef(name='maxGroupSizeY',type='uint32_t',tag='out'),
var6=VarDef(name='maxGroupSizeZ',type='uint32_t',tag='out'),
var7=VarDef(name='maxGroupCountX',type='uint32_t',tag='out'),
var8=VarDef(name='maxGroupCountY',type='uint32_t',tag='out'),
var9=VarDef(name='maxGroupCountZ',type='uint32_t',tag='out'),
var10=VarDef(name='maxSharedLocalMemory',type='uint32_t',tag='out'),
var11=VarDef(name='numSubGroupSizes',type='uint32_t',tag='out'),
var12=VarDef(name='subGroupSizes[ZE_SUBGROUPSIZE_COUNT]',type='uint32_t',tag='out'),
)

Argument(name='ze_native_kernel_uuid_t',
var1=VarDef(name='id[ZE_MAX_NATIVE_KERNEL_UUID_SIZE]',type='uint8_t',tag='out'),
)

Argument(name='ze_device_module_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='spirvVersionSupported',type='uint32_t',tag='out'),
var4=VarDef(name='flags',type='ze_device_module_flags_t',tag='out'),
var5=VarDef(name='fp16flags',type='ze_device_fp_flags_t',tag='out'),
var6=VarDef(name='fp32flags',type='ze_device_fp_flags_t',tag='out'),
var7=VarDef(name='fp64flags',type='ze_device_fp_flags_t',tag='out'),
var8=VarDef(name='maxArgumentsSize',type='uint32_t',tag='out'),
var9=VarDef(name='printfBufferSize',type='uint32_t',tag='out'),
var10=VarDef(name='nativeKernelSupported',type='ze_native_kernel_uuid_t',tag='out'),
)

Argument(name='ze_command_queue_group_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_command_queue_group_property_flags_t',tag='out'),
var4=VarDef(name='maxMemoryFillPatternSize',type='size_t',tag='out'),
var5=VarDef(name='numQueues',type='uint32_t',tag='out'),
)

Argument(name='ze_device_memory_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_device_memory_property_flags_t',tag='out'),
var4=VarDef(name='maxClockRate',type='uint32_t',tag='out'),
var5=VarDef(name='maxBusWidth',type='uint32_t',tag='out'),
var6=VarDef(name='totalSize',type='uint64_t',tag='out'),
var7=VarDef(name='name[ZE_MAX_DEVICE_NAME]',type='char',tag='out'),
)

Argument(name='ze_device_memory_access_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='hostAllocCapabilities',type='ze_memory_access_cap_flags_t',tag='out'),
var4=VarDef(name='deviceAllocCapabilities',type='ze_memory_access_cap_flags_t',tag='out'),
var5=VarDef(name='sharedSingleDeviceAllocCapabilities',type='ze_memory_access_cap_flags_t',tag='out'),
var6=VarDef(name='sharedCrossDeviceAllocCapabilities',type='ze_memory_access_cap_flags_t',tag='out'),
var7=VarDef(name='sharedSystemAllocCapabilities',type='ze_memory_access_cap_flags_t',tag='out'),
)

Argument(name='ze_device_cache_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_device_cache_property_flags_t',tag='out'),
var4=VarDef(name='cacheSize',type='size_t',tag='out'),
)

Argument(name='ze_device_image_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='maxImageDims1D',type='uint32_t',tag='out'),
var4=VarDef(name='maxImageDims2D',type='uint32_t',tag='out'),
var5=VarDef(name='maxImageDims3D',type='uint32_t',tag='out'),
var6=VarDef(name='maxImageBufferSize',type='uint64_t',tag='out'),
var7=VarDef(name='maxImageArraySlices',type='uint32_t',tag='out'),
var8=VarDef(name='maxSamplers',type='uint32_t',tag='out'),
var9=VarDef(name='maxReadImageArgs',type='uint32_t',tag='out'),
var10=VarDef(name='maxWriteImageArgs',type='uint32_t',tag='out'),
)

Argument(name='ze_device_external_memory_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='memoryAllocationImportTypes',type='ze_external_memory_type_flags_t',tag='out'),
var4=VarDef(name='memoryAllocationExportTypes',type='ze_external_memory_type_flags_t',tag='out'),
var5=VarDef(name='imageImportTypes',type='ze_external_memory_type_flags_t',tag='out'),
var6=VarDef(name='imageExportTypes',type='ze_external_memory_type_flags_t',tag='out'),
)

Argument(name='ze_device_p2p_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_device_p2p_property_flags_t',tag='out'),
)

Argument(name='ze_context_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_context_flags_t',tag='in'),
)

Argument(name='ze_command_queue_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='ordinal',type='uint32_t',tag='in'),
var4=VarDef(name='index',type='uint32_t',tag='in'),
var5=VarDef(name='flags',type='ze_command_queue_flags_t',tag='in'),
var6=VarDef(name='mode',type='ze_command_queue_mode_t',tag='in'),
var7=VarDef(name='priority',type='ze_command_queue_priority_t',tag='in'),
)

Argument(name='ze_command_list_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='commandQueueGroupOrdinal',type='uint32_t',tag='in'),
var4=VarDef(name='flags',type='ze_command_list_flags_t',tag='in'),
)

Argument(name='ze_copy_region_t',
var1=VarDef(name='originX',type='uint32_t',tag='in'),
var2=VarDef(name='originY',type='uint32_t',tag='in'),
var3=VarDef(name='originZ',type='uint32_t',tag='in'),
var4=VarDef(name='width',type='uint32_t',tag='in'),
var5=VarDef(name='height',type='uint32_t',tag='in'),
var6=VarDef(name='depth',type='uint32_t',tag='in'),
)

Argument(name='ze_image_region_t',
var1=VarDef(name='originX',type='uint32_t',tag='in'),
var2=VarDef(name='originY',type='uint32_t',tag='in'),
var3=VarDef(name='originZ',type='uint32_t',tag='in'),
var4=VarDef(name='width',type='uint32_t',tag='in'),
var5=VarDef(name='height',type='uint32_t',tag='in'),
var6=VarDef(name='depth',type='uint32_t',tag='in'),
)

Argument(name='ze_event_pool_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_event_pool_flags_t',tag='in'),
var4=VarDef(name='count',type='uint32_t',tag='in'),
)

Argument(name='ze_event_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='index',type='uint32_t',tag='in'),
var4=VarDef(name='signal',type='ze_event_scope_flags_t',tag='in'),
var5=VarDef(name='wait',type='ze_event_scope_flags_t',tag='in'),
)

Argument(name='ze_kernel_timestamp_data_t',
var1=VarDef(name='kernelStart',type='uint64_t',tag='out'),
var2=VarDef(name='kernelEnd',type='uint64_t',tag='out'),
)

Argument(name='ze_kernel_timestamp_result_t',
var1=VarDef(name='global',type='ze_kernel_timestamp_data_t',tag='out'),
var2=VarDef(name='context',type='ze_kernel_timestamp_data_t',tag='out'),
)

Argument(name='ze_fence_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_fence_flags_t',tag='in'),
)

Argument(name='ze_image_format_t',
var1=VarDef(name='layout',type='ze_image_format_layout_t',tag='in'),
var2=VarDef(name='type',type='ze_image_format_type_t',tag='in'),
var3=VarDef(name='x',type='ze_image_format_swizzle_t',tag='in'),
var4=VarDef(name='y',type='ze_image_format_swizzle_t',tag='in'),
var5=VarDef(name='z',type='ze_image_format_swizzle_t',tag='in'),
var6=VarDef(name='w',type='ze_image_format_swizzle_t',tag='in'),
)

Argument(name='ze_image_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_image_flags_t',tag='in'),
var4=VarDef(name='type',type='ze_image_type_t',tag='in'),
var5=VarDef(name='format',type='ze_image_format_t',tag='in'),
var6=VarDef(name='width',type='uint64_t',tag='in'),
var7=VarDef(name='height',type='uint32_t',tag='in'),
var8=VarDef(name='depth',type='uint32_t',tag='in'),
var9=VarDef(name='arraylevels',type='uint32_t',tag='in'),
var10=VarDef(name='miplevels',type='uint32_t',tag='in'),
)

Argument(name='ze_image_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='samplerFilterFlags',type='ze_image_sampler_filter_flags_t',tag='out'),
)

Argument(name='ze_device_mem_alloc_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_device_mem_alloc_flags_t',tag='in'),
var4=VarDef(name='ordinal',type='uint32_t',tag='in'),
)

Argument(name='ze_host_mem_alloc_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_host_mem_alloc_flags_t',tag='in'),
)

Argument(name='ze_memory_allocation_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='type',type='ze_memory_type_t',tag='out'),
var4=VarDef(name='id',type='uint64_t',tag='out'),
var5=VarDef(name='pageSize',type='uint64_t',tag='out'),
)

Argument(name='ze_external_memory_export_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_external_memory_type_flags_t',tag='in'),
)

Argument(name='ze_external_memory_import_fd_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_external_memory_type_flags_t',tag='in'),
var4=VarDef(name='fd',type='int',tag='in'),
)

Argument(name='ze_external_memory_export_fd_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_external_memory_type_flags_t',tag='in'),
var4=VarDef(name='fd',type='int',tag='out'),
)

Argument(name='ze_external_memory_import_win32_handle_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_external_memory_type_flags_t',tag='in'),
var4=VarDef(name='handle',type='void*',tag='in',wrapType='CMappedHandle'),
var5=VarDef(name='name',type='const void*',tag='in'),
)

Argument(name='ze_external_memory_export_win32_handle_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_external_memory_type_flags_t',tag='in'),
var4=VarDef(name='handle',type='void*',tag='out'),
)

Argument(name='ze_module_constants_t',custom=True,
var1=VarDef(name='numConstants',type='uint32_t',tag='in'),
var2=VarDef(name='pConstantIds',type='const uint32_t*',tag='in'),
var3=VarDef(name='pConstantValues',type='const void**',tag='in'),
)

Argument(name='ze_module_constants_t',custom=True,version=1,
var1=VarDef(name='numConstants',type='uint32_t',tag='in'),
var2=VarDef(name='pConstantIds',type='const uint32_t*',tag='in'),
var3=VarDef(name='pConstantValues',type='const void**',tag='in'),
)

Argument(name='ze_module_desc_t',custom=True,
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='format',type='ze_module_format_t',tag='in'),
var4=VarDef(name='inputSize',type='size_t',tag='in'),
var5=VarDef(name='pInputModule',type='const uint8_t*',tag='in'),
var6=VarDef(name='pBuildFlags',type='const char*',tag='in'),
var7=VarDef(name='pConstants',type='const ze_module_constants_t*',tag='in'),
)

Argument(name='ze_module_desc_t',custom=True,version=1,
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='format',type='ze_module_format_t',tag='in'),
var4=VarDef(name='inputSize',type='size_t',tag='in'),
var5=VarDef(name='pInputModule',type='const uint8_t*',tag='in',wrapType='CProgramSource'),
var6=VarDef(name='pBuildFlags',type='const char*',tag='in'),
var7=VarDef(name='pConstants',type='const ze_module_constants_t*',tag='in'),
)

Argument(name='ze_module_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_module_property_flags_t',tag='out'),
)

Argument(name='ze_kernel_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_kernel_flags_t',tag='in'),
var4=VarDef(name='pKernelName',type='const char*',tag='in'),
)

Argument(name='ze_kernel_uuid_t',
var1=VarDef(name='kid[ZE_MAX_KERNEL_UUID_SIZE]',type='uint8_t',tag='out'),
var2=VarDef(name='mid[ZE_MAX_MODULE_UUID_SIZE]',type='uint8_t',tag='out'),
)

Argument(name='ze_kernel_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='numKernelArgs',type='uint32_t',tag='out'),
var4=VarDef(name='requiredGroupSizeX',type='uint32_t',tag='out'),
var5=VarDef(name='requiredGroupSizeY',type='uint32_t',tag='out'),
var6=VarDef(name='requiredGroupSizeZ',type='uint32_t',tag='out'),
var7=VarDef(name='requiredNumSubGroups',type='uint32_t',tag='out'),
var8=VarDef(name='requiredSubgroupSize',type='uint32_t',tag='out'),
var9=VarDef(name='maxSubgroupSize',type='uint32_t',tag='out'),
var10=VarDef(name='maxNumSubgroups',type='uint32_t',tag='out'),
var11=VarDef(name='localMemSize',type='uint32_t',tag='out'),
var12=VarDef(name='privateMemSize',type='uint32_t',tag='out'),
var13=VarDef(name='spillMemSize',type='uint32_t',tag='out'),
var14=VarDef(name='uuid',type='ze_kernel_uuid_t',tag='out'),
)

Argument(name='ze_kernel_preferred_group_size_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='preferredMultiple',type='uint32_t',tag='out'),
)

Argument(name='ze_group_count_t',
var1=VarDef(name='groupCountX',type='uint32_t',tag='in'),
var2=VarDef(name='groupCountY',type='uint32_t',tag='in'),
var3=VarDef(name='groupCountZ',type='uint32_t',tag='in'),
)

# TODO
# Argument(name='ze_module_program_exp_desc_t',custom=True,
# var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
# var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
# var3=VarDef(name='count',type='uint32_t',tag='in'),
# var4=VarDef(name='inputSizes',type='const size_t*',tag='in'),
# var5=VarDef(name='pInputModules',type='const uint8_t**',tag='in'),
# var6=VarDef(name='pBuildFlags',type='const char**',tag='in'),
# var7=VarDef(name='pConstants',type='const ze_module_constants_t**',tag='in'),
# )

Argument(name='ze_device_raytracing_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_device_raytracing_ext_flags_t',tag='out'),
var4=VarDef(name='maxBVHLevels',type='uint32_t',tag='out'),
)

Argument(name='ze_raytracing_mem_alloc_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_raytracing_mem_alloc_ext_flags_t',tag='in'),
)

Argument(name='ze_sampler_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='addressMode',type='ze_sampler_address_mode_t',tag='in'),
var4=VarDef(name='filterMode',type='ze_sampler_filter_mode_t',tag='in'),
var5=VarDef(name='isNormalized',type='ze_bool_t',tag='in'),
)

Argument(name='ze_physical_mem_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_physical_mem_flags_t',tag='in'),
var4=VarDef(name='size',type='size_t',tag='in'),
)

Argument(name='ze_float_atomic_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='fp16Flags',type='ze_device_fp_atomic_ext_flags_t',tag='out'),
var4=VarDef(name='fp32Flags',type='ze_device_fp_atomic_ext_flags_t',tag='out'),
var5=VarDef(name='fp64Flags',type='ze_device_fp_atomic_ext_flags_t',tag='out'),
)

Argument(name='ze_mutable_command_id_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_mutable_command_exp_flags_t',tag='in'),
)

Argument(name='ze_mutable_command_list_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='mutableCommandListFlags',type='ze_mutable_command_list_exp_flags_t',tag='out'),
var4=VarDef(name='mutableCommandFlags',type='ze_mutable_command_exp_flags_t',tag='out'),
)

Argument(name='ze_mutable_command_list_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_mutable_command_list_exp_flags_t',tag='in'),
)

Argument(name='ze_mutable_commands_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='uint32_t',tag='in'),
)

Argument(name='ze_mutable_kernel_argument_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='commandId',type='uint64_t',tag='in'),
var4=VarDef(name='argIndex',type='uint32_t',tag='in'),
var5=VarDef(name='argSize',type='size_t',tag='in'),
var6=VarDef(name='pArgValue',type='const void*',tag='in',wrapType='CKernelArgValue',wrapParams='argSize, {name}'),
)

# Argument(name='ze_mutable_group_count_exp_desc_t',
# var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
# var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
# var3=VarDef(name='commandId',type='uint64_t',tag='in'),
# var4=VarDef(name='pGroupCount',type='const ze_group_count_t*',tag='in'),
# )

Argument(name='ze_mutable_group_size_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='commandId',type='uint64_t',tag='in'),
var4=VarDef(name='groupSizeX',type='uint32_t',tag='in'),
var5=VarDef(name='groupSizeY',type='uint32_t',tag='in'),
var6=VarDef(name='groupSizeZ',type='uint32_t',tag='in'),
)

Argument(name='ze_mutable_global_offset_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='commandId',type='uint64_t',tag='in'),
var4=VarDef(name='offsetX',type='uint32_t',tag='in'),
var5=VarDef(name='offsetY',type='uint32_t',tag='in'),
var6=VarDef(name='offsetZ',type='uint32_t',tag='in'),
)

# Argument(name='ze_mutable_graph_argument_exp_desc_t',
# var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
# var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
# var3=VarDef(name='commandId',type='uint64_t',tag='in'),
# var4=VarDef(name='argIndex',type='uint32_t',tag='in'),
# var5=VarDef(name='pArgValue',type='const void*',tag='in'),
# )

Argument(name='ze_relaxed_allocation_limits_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_relaxed_allocation_limits_exp_flags_t',tag='in'),
)

Argument(name='ze_cache_reservation_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='maxCacheReservationSize',type='size_t',tag='out'),
)

Argument(name='ze_image_memory_properties_exp_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='size',type='uint64_t',tag='out'),
var4=VarDef(name='rowPitch',type='uint64_t',tag='out'),
var5=VarDef(name='slicePitch',type='uint64_t',tag='out'),
)

Argument(name='ze_image_view_planar_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='planeIndex',type='uint32_t',tag='in'),
)

Argument(name='ze_image_view_planar_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='planeIndex',type='uint32_t',tag='in'),
)

Argument(name='ze_scheduling_hint_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='schedulingHintFlags',type='ze_scheduling_hint_exp_flags_t',tag='out'),
)

Argument(name='ze_scheduling_hint_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_scheduling_hint_exp_flags_t',tag='in'),
)

Argument(name='ze_context_power_saving_hint_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='hint',type='uint32_t',tag='in'),
)

Argument(name='ze_eu_count_ext_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='numTotalEUs',type='uint32_t',tag='out'),
)

Argument(name='ze_pci_address_ext_t',
var1=VarDef(name='domain',type='uint32_t',tag='out'),
var2=VarDef(name='bus',type='uint32_t',tag='out'),
var3=VarDef(name='device',type='uint32_t',tag='out'),
var4=VarDef(name='function',type='uint32_t',tag='out'),
)

Argument(name='ze_pci_speed_ext_t',
var1=VarDef(name='genVersion',type='int32_t',tag='out'),
var2=VarDef(name='width',type='int32_t',tag='out'),
var3=VarDef(name='maxBandwidth',type='int64_t',tag='out'),
)

Argument(name='ze_pci_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='address',type='ze_pci_address_ext_t',tag='out'),
var4=VarDef(name='maxSpeed',type='ze_pci_speed_ext_t',tag='out'),
)

Argument(name='ze_srgb_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='sRGB',type='ze_bool_t',tag='in'),
)

Argument(name='ze_image_allocation_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='id',type='uint64_t',tag='out'),
)

Argument(name='ze_linkage_inspection_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_linkage_inspection_ext_flags_t',tag='in'),
)

Argument(name='ze_memory_compression_hints_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_memory_compression_hints_ext_flags_t',tag='in'),
)

Argument(name='ze_driver_memory_free_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='freePolicies',type='ze_driver_memory_free_policy_ext_flags_t',tag='out'),
)

Argument(name='ze_memory_free_ext_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='freePolicy',type='ze_driver_memory_free_policy_ext_flags_t',tag='in'),
)

Argument(name='ze_device_p2p_bandwidth_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='logicalBandwidth',type='uint32_t',tag='out'),
var4=VarDef(name='physicalBandwidth',type='uint32_t',tag='out'),
var5=VarDef(name='bandwidthUnit',type='ze_bandwidth_unit_t',tag='out'),
var6=VarDef(name='logicalLatency',type='uint32_t',tag='out'),
var7=VarDef(name='physicalLatency',type='uint32_t',tag='out'),
var8=VarDef(name='latencyUnit',type='ze_latency_unit_t',tag='out'),
)

Argument(name='ze_copy_bandwidth_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='copyBandwidth',type='uint32_t',tag='out'),
var4=VarDef(name='copyBandwidthUnit',type='ze_bandwidth_unit_t',tag='out'),
)

Argument(name='ze_device_luid_ext_t',
var1=VarDef(name='id[ZE_MAX_DEVICE_LUID_SIZE_EXT]',type='uint8_t',tag='out'),
)

Argument(name='ze_device_luid_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='luid',type='ze_device_luid_ext_t',tag='out'),
var4=VarDef(name='nodeMask',type='uint32_t',tag='out'),
)

Argument(name='ze_fabric_vertex_pci_exp_address_t',
var1=VarDef(name='domain',type='uint32_t',tag='out'),
var2=VarDef(name='bus',type='uint32_t',tag='out'),
var3=VarDef(name='device',type='uint32_t',tag='out'),
var4=VarDef(name='function',type='uint32_t',tag='out'),
)

Argument(name='ze_fabric_vertex_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='uuid',type='ze_uuid_t',tag='out'),
var4=VarDef(name='type',type='ze_fabric_vertex_exp_type_t',tag='out'),
var5=VarDef(name='remote',type='ze_bool_t',tag='out'),
var6=VarDef(name='address',type='ze_fabric_vertex_pci_exp_address_t',tag='out'),
)

Argument(name='ze_fabric_edge_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='uuid',type='ze_uuid_t',tag='out'),
var4=VarDef(name='model[ZE_MAX_FABRIC_EDGE_MODEL_EXP_SIZE]',type='char',tag='out'),
var5=VarDef(name='bandwidth',type='uint32_t',tag='out'),
var6=VarDef(name='bandwidthUnit',type='ze_bandwidth_unit_t',tag='out'),
var7=VarDef(name='latency',type='uint32_t',tag='out'),
var8=VarDef(name='latencyUnit',type='ze_latency_unit_t',tag='out'),
var9=VarDef(name='duplexity',type='ze_fabric_edge_exp_duplexity_t',tag='out'),
)

Argument(name='ze_device_memory_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='type',type='ze_device_memory_ext_type_t',tag='out'),
var4=VarDef(name='physicalSize',type='uint64_t',tag='out'),
var5=VarDef(name='readBandwidth',type='uint32_t',tag='out'),
var6=VarDef(name='writeBandwidth',type='uint32_t',tag='out'),
var7=VarDef(name='bandwidthUnit',type='ze_bandwidth_unit_t',tag='out'),
)

Argument(name='ze_device_ip_version_ext_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='ipVersion',type='uint32_t',tag='out'),
)

Argument(name='ze_kernel_max_group_size_properties_ext_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='maxGroupSize',type='uint32_t',tag='out'),
)

# Argument(name='ze_sub_allocation_t',
# var1=VarDef(name='base',type='void*',tag='inout'),
# var2=VarDef(name='size',type='size_t',tag='inout'),
# )

# Argument(name='ze_memory_sub_allocations_exp_properties_t',
# var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
# var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
# var3=VarDef(name='pCount',type='uint32_t*',tag='inout',wrapType='Cuint32_t::CSArray'),
# var4=VarDef(name='pSubAllocations',type='ze_sub_allocation_t*',tag='inout',range='0,*pCount',optional=True),
# )

Argument(name='ze_event_query_kernel_timestamps_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_event_query_kernel_timestamps_ext_flags_t',tag='out'),
)

Argument(name='ze_synchronized_timestamp_data_ext_t',
var1=VarDef(name='kernelStart',type='uint64_t',tag='out'),
var2=VarDef(name='kernelEnd',type='uint64_t',tag='out'),
)

Argument(name='ze_synchronized_timestamp_result_ext_t',
var1=VarDef(name='global',type='ze_synchronized_timestamp_data_ext_t',tag='out'),
var2=VarDef(name='context',type='ze_synchronized_timestamp_data_ext_t',tag='out'),
)

# TODO
Argument(name='ze_event_query_kernel_timestamps_results_ext_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='pKernelTimestampsBuffer',type='ze_kernel_timestamp_result_t*',tag='inout',wrapType='COutArgument'),
var4=VarDef(name='pSynchronizedTimestampsBuffer',type='ze_synchronized_timestamp_result_ext_t*',tag='inout',wrapType='COutArgument'),
)

Argument(name='ze_rtas_builder_exp_handle_t',obj=True)

Argument(name='ze_rtas_parallel_operation_exp_handle_t',obj=True)

Argument(name='ze_rtas_builder_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='builderVersion',type='ze_rtas_builder_exp_version_t',tag='in'),
)

Argument(name='ze_rtas_builder_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_rtas_builder_exp_flags_t',tag='out'),
var4=VarDef(name='rtasBufferSizeBytesExpected',type='size_t',tag='out'),
var5=VarDef(name='rtasBufferSizeBytesMaxRequired',type='size_t',tag='out'),
var6=VarDef(name='scratchBufferSizeBytes',type='size_t',tag='out'),
)

Argument(name='ze_rtas_parallel_operation_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_rtas_parallel_operation_exp_flags_t',tag='out'),
var4=VarDef(name='maxConcurrency',type='uint32_t',tag='out'),
)

Argument(name='ze_rtas_device_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_rtas_device_exp_flags_t',tag='out'),
var4=VarDef(name='rtasFormat',type='ze_rtas_format_exp_t',tag='out'),
var5=VarDef(name='rtasBufferAlignment',type='uint32_t',tag='out'),
)

Argument(name='ze_rtas_float3_exp_t',
var1=VarDef(name='x',type='float',tag='in'),
var2=VarDef(name='y',type='float',tag='in'),
var3=VarDef(name='z',type='float',tag='in'),
)

Argument(name='ze_rtas_transform_float3x4_column_major_exp_t',
var1=VarDef(name='vx_x',type='float',tag='in'),
var2=VarDef(name='vx_y',type='float',tag='in'),
var3=VarDef(name='vx_z',type='float',tag='in'),
var4=VarDef(name='vy_x',type='float',tag='in'),
var5=VarDef(name='vy_y',type='float',tag='in'),
var6=VarDef(name='vy_z',type='float',tag='in'),
var7=VarDef(name='vz_x',type='float',tag='in'),
var8=VarDef(name='vz_y',type='float',tag='in'),
var9=VarDef(name='vz_z',type='float',tag='in'),
var10=VarDef(name='p_x',type='float',tag='in'),
var11=VarDef(name='p_y',type='float',tag='in'),
var12=VarDef(name='p_z',type='float',tag='in'),
)

Argument(name='ze_rtas_transform_float3x4_aligned_column_major_exp_t',
var1=VarDef(name='vx_x',type='float',tag='in'),
var2=VarDef(name='vx_y',type='float',tag='in'),
var3=VarDef(name='vx_z',type='float',tag='in'),
var4=VarDef(name='pad0',type='float',tag='in'),
var5=VarDef(name='vy_x',type='float',tag='in'),
var6=VarDef(name='vy_y',type='float',tag='in'),
var7=VarDef(name='vy_z',type='float',tag='in'),
var8=VarDef(name='pad1',type='float',tag='in'),
var9=VarDef(name='vz_x',type='float',tag='in'),
var10=VarDef(name='vz_y',type='float',tag='in'),
var11=VarDef(name='vz_z',type='float',tag='in'),
var12=VarDef(name='pad2',type='float',tag='in'),
var13=VarDef(name='p_x',type='float',tag='in'),
var14=VarDef(name='p_y',type='float',tag='in'),
var15=VarDef(name='p_z',type='float',tag='in'),
var16=VarDef(name='pad3',type='float',tag='in'),
)

Argument(name='ze_rtas_transform_float3x4_row_major_exp_t',
var1=VarDef(name='vx_x',type='float',tag='in'),
var2=VarDef(name='vy_x',type='float',tag='in'),
var3=VarDef(name='vz_x',type='float',tag='in'),
var4=VarDef(name='p_x',type='float',tag='in'),
var5=VarDef(name='vx_y',type='float',tag='in'),
var6=VarDef(name='vy_y',type='float',tag='in'),
var7=VarDef(name='vz_y',type='float',tag='in'),
var8=VarDef(name='p_y',type='float',tag='in'),
var9=VarDef(name='vx_z',type='float',tag='in'),
var10=VarDef(name='vy_z',type='float',tag='in'),
var11=VarDef(name='vz_z',type='float',tag='in'),
var12=VarDef(name='p_z',type='float',tag='in'),
)

Argument(name='ze_rtas_aabb_exp_t',
var1=VarDef(name='lower',type='ze_rtas_float3_exp_t',tag='in'),
var2=VarDef(name='upper',type='ze_rtas_float3_exp_t',tag='in'),
)

Argument(name='ze_rtas_triangle_indices_uint32_exp_t',
var1=VarDef(name='v0',type='uint32_t',tag='in'),
var2=VarDef(name='v1',type='uint32_t',tag='in'),
var3=VarDef(name='v2',type='uint32_t',tag='in'),
)

Argument(name='ze_rtas_quad_indices_uint32_exp_t',
var1=VarDef(name='v0',type='uint32_t',tag='in'),
var2=VarDef(name='v1',type='uint32_t',tag='in'),
var3=VarDef(name='v2',type='uint32_t',tag='in'),
var4=VarDef(name='v3',type='uint32_t',tag='in'),
)

Argument(name='ze_rtas_builder_geometry_info_exp_t',
var1=VarDef(name='geometryType',type='ze_rtas_builder_packed_geometry_type_exp_t',tag='in'),
)

# TODO
Argument(name='ze_rtas_builder_triangles_geometry_info_exp_t',
var1=VarDef(name='geometryType',type='ze_rtas_builder_packed_geometry_type_exp_t',tag='in'),
var2=VarDef(name='geometryFlags',type='ze_rtas_builder_packed_geometry_exp_flags_t',tag='in'),
var3=VarDef(name='geometryMask',type='uint8_t',tag='in'),
var4=VarDef(name='triangleFormat',type='ze_rtas_builder_packed_input_data_format_exp_t',tag='in'),
var5=VarDef(name='vertexFormat',type='ze_rtas_builder_packed_input_data_format_exp_t',tag='in'),
var6=VarDef(name='triangleCount',type='uint32_t',tag='in'),
var7=VarDef(name='vertexCount',type='uint32_t',tag='in'),
var8=VarDef(name='triangleStride',type='uint32_t',tag='in'),
var9=VarDef(name='vertexStride',type='uint32_t',tag='in'),
var10=VarDef(name='pTriangleBuffer',type='void*',tag='in'),
var11=VarDef(name='pVertexBuffer',type='void*',tag='in'),
)

Argument(name='ze_rtas_builder_quads_geometry_info_exp_t',
var1=VarDef(name='geometryType',type='ze_rtas_builder_packed_geometry_type_exp_t',tag='in'),
var2=VarDef(name='geometryFlags',type='ze_rtas_builder_packed_geometry_exp_flags_t',tag='in'),
var3=VarDef(name='geometryMask',type='uint8_t',tag='in'),
var4=VarDef(name='quadFormat',type='ze_rtas_builder_packed_input_data_format_exp_t',tag='in'),
var5=VarDef(name='vertexFormat',type='ze_rtas_builder_packed_input_data_format_exp_t',tag='in'),
var6=VarDef(name='quadCount',type='uint32_t',tag='in'),
var7=VarDef(name='vertexCount',type='uint32_t',tag='in'),
var8=VarDef(name='quadStride',type='uint32_t',tag='in'),
var9=VarDef(name='vertexStride',type='uint32_t',tag='in'),
var10=VarDef(name='pQuadBuffer',type='void*',tag='in'),
var11=VarDef(name='pVertexBuffer',type='void*',tag='in'),
)

# Argument(name='ze_rtas_geometry_aabbs_exp_cb_params_t',
# var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
# var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
# var3=VarDef(name='primID',type='uint32_t',tag='in'),
# var4=VarDef(name='primIDCount',type='uint32_t',tag='in'),
# var5=VarDef(name='pGeomUserPtr',type='void*',tag='in'),
# var6=VarDef(name='pBuildUserPtr',type='void*',tag='in'),
# var7=VarDef(name='pBoundsOut',type='ze_rtas_aabb_exp_t*',tag='out'),
# )

# Argument(name='ze_rtas_builder_procedural_geometry_info_exp_t',
# var1=VarDef(name='geometryType',type='ze_rtas_builder_packed_geometry_type_exp_t',tag='in'),
# var2=VarDef(name='geometryFlags',type='ze_rtas_builder_packed_geometry_exp_flags_t',tag='in'),
# var3=VarDef(name='geometryMask',type='uint8_t',tag='in'),
# var4=VarDef(name='reserved',type='uint8_t',tag='in'),
# var5=VarDef(name='primCount',type='uint32_t',tag='in'),
# var6=VarDef(name='pfnGetBoundsCb',type='ze_rtas_geometry_aabbs_cb_exp_t',tag='in'),
# var7=VarDef(name='pGeomUserPtr',type='void*',tag='in'),
# )

# Argument(name='ze_rtas_builder_instance_geometry_info_exp_t',
# var1=VarDef(name='geometryType',type='ze_rtas_builder_packed_geometry_type_exp_t',tag='in'),
# var2=VarDef(name='instanceFlags',type='ze_rtas_builder_packed_instance_exp_flags_t',tag='in'),
# var3=VarDef(name='geometryMask',type='uint8_t',tag='in'),
# var4=VarDef(name='transformFormat',type='ze_rtas_builder_packed_input_data_format_exp_t',tag='in'),
# var5=VarDef(name='instanceUserID',type='uint32_t',tag='in'),
# var6=VarDef(name='pTransform',type='void*',tag='in'),
# var7=VarDef(name='pBounds',type='ze_rtas_aabb_exp_t*',tag='in'),
# var8=VarDef(name='pAccelerationStructure',type='void*',tag='in'),
# )

Argument(name='ze_rtas_builder_build_op_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='rtasFormat',type='ze_rtas_format_exp_t',tag='in'),
var4=VarDef(name='buildQuality',type='ze_rtas_builder_build_quality_hint_exp_t',tag='in'),
var5=VarDef(name='buildFlags',type='ze_rtas_builder_build_op_exp_flags_t',tag='in'),
var6=VarDef(name='ppGeometries',type='const ze_rtas_builder_geometry_info_exp_t**',tag='in',wrapType='COutArgument'),
var7=VarDef(name='numGeometries',type='uint32_t',tag='in'),
)

Argument(name='ze_event_pool_counter_based_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_event_pool_counter_based_exp_flags_t',tag='in'),
)

Argument(name='ze_image_bindless_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='flags',type='ze_image_bindless_exp_flags_t',tag='in'),
)

Argument(name='ze_image_pitched_exp_desc_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructCore'),
var3=VarDef(name='ptr',type='void*',tag='in',wrapType='CUSMPtr'),
)

Argument(name='ze_device_pitched_alloc_exp_properties_t',
var1=VarDef(name='stype',type='ze_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructCore'),
var3=VarDef(name='maxImageLinearWidth',type='size_t',tag='out'),
var4=VarDef(name='maxImageLinearHeight',type='size_t',tag='out'),
)

Argument(name='zet_driver_handle_t',obj=True,alias='ze_driver_handle_t')

Argument(name='zet_device_handle_t',obj=True,alias='ze_device_handle_t')

Argument(name='zet_context_handle_t',obj=True,alias='ze_context_handle_t')

Argument(name='zet_command_list_handle_t',obj=True,alias='ze_command_list_handle_t')

Argument(name='zet_module_handle_t',obj=True,alias='ze_module_handle_t')

Argument(name='zet_kernel_handle_t',obj=True,alias='ze_kernel_handle_t')

Argument(name='zet_metric_group_handle_t',obj=True)

Argument(name='zet_metric_handle_t',obj=True)

Argument(name='zet_metric_streamer_handle_t',obj=True)

Argument(name='zet_metric_query_pool_handle_t',obj=True)

Argument(name='zet_metric_query_handle_t',obj=True)

Argument(name='zet_tracer_exp_handle_t',obj=True)

Argument(name='zet_debug_session_handle_t',obj=True)

Argument(name='zet_base_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
)

Argument(name='zet_base_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
)

Argument(name='zet_value_t',union=True,
var1=VarDef(name='ui32',type='uint32_t',tag='out'),
var2=VarDef(name='ui64',type='uint64_t',tag='out'),
var3=VarDef(name='fp32',type='float',tag='out'),
var4=VarDef(name='fp64',type='double',tag='out'),
var5=VarDef(name='b8',type='ze_bool_t',tag='out'),
)

Argument(name='zet_typed_value_t',
var1=VarDef(name='type',type='zet_value_type_t',tag='out'),
var2=VarDef(name='value',type='zet_value_t',tag='out'),
)

Argument(name='zet_device_debug_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='flags',type='zet_device_debug_property_flags_t',tag='out'),
)

Argument(name='zet_debug_config_t',
var1=VarDef(name='pid',type='uint32_t',tag='in'),
)

Argument(name='zet_debug_event_info_detached_t',
var1=VarDef(name='reason',type='zet_debug_detach_reason_t',tag='out'),
)

Argument(name='zet_debug_event_info_module_t',
var1=VarDef(name='format',type='zet_module_debug_info_format_t',tag='out'),
var2=VarDef(name='moduleBegin',type='uint64_t',tag='out'),
var3=VarDef(name='moduleEnd',type='uint64_t',tag='out'),
var4=VarDef(name='load',type='uint64_t',tag='out'),
)

Argument(name='zet_debug_event_info_thread_stopped_t',
var1=VarDef(name='thread',type='ze_device_thread_t',tag='out'),
)

Argument(name='zet_debug_event_info_page_fault_t',
var1=VarDef(name='address',type='uint64_t',tag='out'),
var2=VarDef(name='mask',type='uint64_t',tag='out'),
var3=VarDef(name='reason',type='zet_debug_page_fault_reason_t',tag='out'),
)

Argument(name='zet_debug_event_info_t',union=True,
var1=VarDef(name='detached',type='zet_debug_event_info_detached_t',tag='out'),
var2=VarDef(name='module',type='zet_debug_event_info_module_t',tag='out'),
var3=VarDef(name='thread',type='zet_debug_event_info_thread_stopped_t',tag='out'),
var4=VarDef(name='page_fault',type='zet_debug_event_info_page_fault_t',tag='out'),
)

Argument(name='zet_debug_event_t',
var1=VarDef(name='type',type='zet_debug_event_type_t',tag='out'),
var2=VarDef(name='flags',type='zet_debug_event_flags_t',tag='out'),
var3=VarDef(name='info',type='zet_debug_event_info_t',tag='out'),
)

Argument(name='zet_debug_memory_space_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='type',type='zet_debug_memory_space_type_t',tag='in'),
var4=VarDef(name='address',type='uint64_t',tag='in'),
)

Argument(name='zet_debug_regset_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='type',type='uint32_t',tag='out'),
var4=VarDef(name='version',type='uint32_t',tag='out'),
var5=VarDef(name='generalFlags',type='zet_debug_regset_flags_t',tag='out'),
var6=VarDef(name='deviceFlags',type='uint32_t',tag='out'),
var7=VarDef(name='count',type='uint32_t',tag='out'),
var8=VarDef(name='bitSize',type='uint32_t',tag='out'),
var9=VarDef(name='byteSize',type='uint32_t',tag='out'),
)

Argument(name='zet_metric_group_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='name[ZET_MAX_METRIC_GROUP_NAME]',type='char',tag='out'),
var4=VarDef(name='description[ZET_MAX_METRIC_GROUP_DESCRIPTION]',type='char',tag='out'),
var5=VarDef(name='samplingType',type='zet_metric_group_sampling_type_flags_t',tag='out'),
var6=VarDef(name='domain',type='uint32_t',tag='out'),
var7=VarDef(name='metricCount',type='uint32_t',tag='out'),
)

Argument(name='zet_metric_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='name[ZET_MAX_METRIC_NAME]',type='char',tag='out'),
var4=VarDef(name='description[ZET_MAX_METRIC_DESCRIPTION]',type='char',tag='out'),
var5=VarDef(name='component[ZET_MAX_METRIC_COMPONENT]',type='char',tag='out'),
var6=VarDef(name='tierNumber',type='uint32_t',tag='out'),
var7=VarDef(name='metricType',type='zet_metric_type_t',tag='out'),
var8=VarDef(name='resultType',type='zet_value_type_t',tag='out'),
var9=VarDef(name='resultUnits[ZET_MAX_METRIC_RESULT_UNITS]',type='char',tag='out'),
)

Argument(name='zet_metric_streamer_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='notifyEveryNReports',type='uint32_t',tag='inout'),
var4=VarDef(name='samplingPeriod',type='uint32_t',tag='inout'),
)

Argument(name='zet_metric_query_pool_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='type',type='zet_metric_query_pool_type_t',tag='in'),
var4=VarDef(name='count',type='uint32_t',tag='in'),
)

Argument(name='zet_profile_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='flags',type='zet_profile_flags_t',tag='out'),
var4=VarDef(name='numTokens',type='uint32_t',tag='out'),
)

Argument(name='zet_profile_free_register_token_t',
var1=VarDef(name='type',type='zet_profile_token_type_t',tag='out'),
var2=VarDef(name='size',type='uint32_t',tag='out'),
var3=VarDef(name='count',type='uint32_t',tag='out'),
)

Argument(name='zet_profile_register_sequence_t',
var1=VarDef(name='start',type='uint32_t',tag='out'),
var2=VarDef(name='count',type='uint32_t',tag='out'),
)

Argument(name='zet_tracer_exp_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='pUserData',type='void*',tag='in'),
)

Argument(name='zet_metric_tracer_exp_handle_t',obj=True)

Argument(name='zet_metric_decoder_exp_handle_t',obj=True)

Argument(name='zet_metric_tracer_exp_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='notifyEveryNBytes',type='uint32_t',tag='inout'),
)

Argument(name='zet_metric_entry_exp_t',
var1=VarDef(name='value',type='zet_value_t',tag='out'),
var2=VarDef(name='timeStamp',type='uint64_t',tag='out'),
var3=VarDef(name='metricIndex',type='uint32_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
)

Argument(name='zet_metric_group_type_exp_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='type',type='zet_metric_group_type_exp_flags_t',tag='out'),
)

Argument(name='zet_export_dma_buf_exp_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='fd',type='int',tag='out'),
var4=VarDef(name='size',type='size_t',tag='out'),
)

Argument(name='zet_metric_global_timestamps_resolution_exp_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='timerResolution',type='uint64_t',tag='out'),
var4=VarDef(name='timestampValidBits',type='uint64_t',tag='out'),
)

Argument(name='zet_metric_calculate_exp_desc_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructTools'),
var3=VarDef(name='rawReportSkipCount',type='uint32_t',tag='in'),
)

Argument(name='zet_metric_programmable_exp_handle_t',obj=True)

Argument(name='zet_metric_programmable_exp_properties_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='name[ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP]',type='char',tag='out'),
var4=VarDef(name='description[ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP]',type='char',tag='out'),
var5=VarDef(name='component[ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP]',type='char',tag='out'),
var6=VarDef(name='tierNumber',type='uint32_t',tag='out'),
var7=VarDef(name='domain',type='uint32_t',tag='out'),
var8=VarDef(name='parameterCount',type='uint32_t',tag='out'),
var9=VarDef(name='samplingType',type='zet_metric_group_sampling_type_flags_t',tag='out'),
var10=VarDef(name='sourceId',type='uint32_t',tag='out'),
)

Argument(name='zet_value_uint64_range_exp_t',
var1=VarDef(name='ui64Min',type='uint64_t',tag='out'),
var2=VarDef(name='ui64Max',type='uint64_t',tag='out'),
)

Argument(name='zet_value_fp64_range_exp_t',
var1=VarDef(name='fp64Min',type='double',tag='out'),
var2=VarDef(name='fp64Max',type='double',tag='out'),
)

Argument(name='zet_value_info_exp_t',union=True,
var1=VarDef(name='ui32',type='uint32_t',tag='out'),
var2=VarDef(name='ui64',type='uint64_t',tag='out'),
var3=VarDef(name='fp32',type='float',tag='out'),
var4=VarDef(name='fp64',type='double',tag='out'),
var5=VarDef(name='b8',type='ze_bool_t',tag='out'),
var6=VarDef(name='ui8',type='uint8_t',tag='out'),
var7=VarDef(name='ui16',type='uint16_t',tag='out'),
var8=VarDef(name='ui64Range',type='zet_value_uint64_range_exp_t',tag='out'),
var9=VarDef(name='fp64Range',type='zet_value_fp64_range_exp_t',tag='out'),
)

Argument(name='zet_metric_programmable_param_info_exp_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='type',type='zet_metric_programmable_param_type_exp_t',tag='out'),
var4=VarDef(name='name[ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP]',type='char',tag='out'),
var5=VarDef(name='valueInfoType',type='zet_value_info_type_exp_t',tag='out'),
var6=VarDef(name='defaultValue',type='zet_value_t',tag='out'),
var7=VarDef(name='valueInfoCount',type='uint32_t',tag='out'),
)

Argument(name='zet_metric_programmable_param_value_info_exp_t',
var1=VarDef(name='stype',type='zet_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructTools'),
var3=VarDef(name='valueInfo',type='zet_value_info_exp_t',tag='out'),
var4=VarDef(name='description[ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP]',type='char',tag='out'),
)

Argument(name='zet_metric_programmable_param_value_exp_t',
var1=VarDef(name='value',type='zet_value_t',tag='in'),
)

Argument(name='zes_driver_handle_t',obj=True,alias='ze_driver_handle_t')

Argument(name='zes_device_handle_t',obj=True,alias='ze_device_handle_t')

Argument(name='zes_sched_handle_t',obj=True)

Argument(name='zes_perf_handle_t',obj=True)

Argument(name='zes_pwr_handle_t',obj=True)

Argument(name='zes_freq_handle_t',obj=True)

Argument(name='zes_engine_handle_t',obj=True)

Argument(name='zes_standby_handle_t',obj=True)

Argument(name='zes_firmware_handle_t',obj=True)

Argument(name='zes_mem_handle_t',obj=True)

Argument(name='zes_fabric_port_handle_t',obj=True)

Argument(name='zes_temp_handle_t',obj=True)

Argument(name='zes_psu_handle_t',obj=True)

Argument(name='zes_fan_handle_t',obj=True)

Argument(name='zes_led_handle_t',obj=True)

Argument(name='zes_ras_handle_t',obj=True)

Argument(name='zes_diag_handle_t',obj=True)

Argument(name='zes_overclock_handle_t',obj=True)

Argument(name='zes_vf_handle_t',obj=True)

Argument(name='zes_base_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
)

Argument(name='zes_base_desc_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
)

Argument(name='zes_base_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
)

Argument(name='zes_base_config_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
)

Argument(name='zes_base_capability_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
)

Argument(name='zes_driver_extension_properties_t',
var1=VarDef(name='name[ZES_MAX_EXTENSION_NAME]',type='char',tag='out'),
var2=VarDef(name='version',type='uint32_t',tag='out'),
)

Argument(name='zes_device_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='reset',type='zes_reset_reason_flags_t',tag='out'),
var4=VarDef(name='repaired',type='zes_repair_status_t',tag='out'),
)

Argument(name='zes_reset_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='force',type='ze_bool_t',tag='in'),
var4=VarDef(name='resetType',type='zes_reset_type_t',tag='in'),
)

Argument(name='zes_uuid_t',
var1=VarDef(name='id[ZES_MAX_UUID_SIZE]',type='uint8_t',tag='out'),
)

Argument(name='zes_device_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='core',type='ze_device_properties_t',tag='out'),
var4=VarDef(name='numSubdevices',type='uint32_t',tag='out'),
var5=VarDef(name='serialNumber[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var6=VarDef(name='boardNumber[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var7=VarDef(name='brandName[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var8=VarDef(name='modelName[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var9=VarDef(name='vendorName[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var10=VarDef(name='driverVersion[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
)

Argument(name='zes_device_ext_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='uuid',type='zes_uuid_t',tag='out'),
var4=VarDef(name='type',type='zes_device_type_t',tag='out'),
var5=VarDef(name='flags',type='zes_device_property_flags_t',tag='out'),
)

Argument(name='zes_process_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='processId',type='uint32_t',tag='out'),
var4=VarDef(name='memSize',type='uint64_t',tag='out'),
var5=VarDef(name='sharedSize',type='uint64_t',tag='out'),
var6=VarDef(name='engines',type='zes_engine_type_flags_t',tag='out'),
)

Argument(name='zes_pci_address_t',
var1=VarDef(name='domain',type='uint32_t',tag='out'),
var2=VarDef(name='bus',type='uint32_t',tag='out'),
var3=VarDef(name='device',type='uint32_t',tag='out'),
var4=VarDef(name='function',type='uint32_t',tag='out'),
)

Argument(name='zes_pci_speed_t',
var1=VarDef(name='gen',type='int32_t',tag='out'),
var2=VarDef(name='width',type='int32_t',tag='out'),
var3=VarDef(name='maxBandwidth',type='int64_t',tag='out'),
)

Argument(name='zes_pci_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='address',type='zes_pci_address_t',tag='out'),
var4=VarDef(name='maxSpeed',type='zes_pci_speed_t',tag='out'),
var5=VarDef(name='haveBandwidthCounters',type='ze_bool_t',tag='out'),
var6=VarDef(name='havePacketCounters',type='ze_bool_t',tag='out'),
var7=VarDef(name='haveReplayCounters',type='ze_bool_t',tag='out'),
)

Argument(name='zes_pci_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='status',type='zes_pci_link_status_t',tag='out'),
var4=VarDef(name='qualityIssues',type='zes_pci_link_qual_issue_flags_t',tag='out'),
var5=VarDef(name='stabilityIssues',type='zes_pci_link_stab_issue_flags_t',tag='out'),
var6=VarDef(name='speed',type='zes_pci_speed_t',tag='out'),
)

Argument(name='zes_pci_bar_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_pci_bar_type_t',tag='out'),
var4=VarDef(name='index',type='uint32_t',tag='out'),
var5=VarDef(name='base',type='uint64_t',tag='out'),
var6=VarDef(name='size',type='uint64_t',tag='out'),
)

Argument(name='zes_pci_bar_properties_1_2_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_pci_bar_type_t',tag='out'),
var4=VarDef(name='index',type='uint32_t',tag='out'),
var5=VarDef(name='base',type='uint64_t',tag='out'),
var6=VarDef(name='size',type='uint64_t',tag='out'),
var7=VarDef(name='resizableBarSupported',type='ze_bool_t',tag='out'),
var8=VarDef(name='resizableBarEnabled',type='ze_bool_t',tag='out'),
)

Argument(name='zes_pci_stats_t',
var1=VarDef(name='timestamp',type='uint64_t',tag='out'),
var2=VarDef(name='replayCounter',type='uint64_t',tag='out'),
var3=VarDef(name='packetCounter',type='uint64_t',tag='out'),
var4=VarDef(name='rxCounter',type='uint64_t',tag='out'),
var5=VarDef(name='txCounter',type='uint64_t',tag='out'),
var6=VarDef(name='speed',type='zes_pci_speed_t',tag='out'),
)

Argument(name='zes_overclock_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='domainType',type='zes_overclock_domain_t',tag='out'),
var4=VarDef(name='AvailableControls',type='uint32_t',tag='out'),
var5=VarDef(name='VFProgramType',type='zes_vf_program_type_t',tag='out'),
var6=VarDef(name='NumberOfVFPoints',type='uint32_t',tag='out'),
)

Argument(name='zes_control_property_t',
var1=VarDef(name='MinValue',type='double',tag='out'),
var2=VarDef(name='MaxValue',type='double',tag='out'),
var3=VarDef(name='StepValue',type='double',tag='out'),
var4=VarDef(name='RefValue',type='double',tag='out'),
var5=VarDef(name='DefaultValue',type='double',tag='out'),
)

Argument(name='zes_vf_property_t',
var1=VarDef(name='MinFreq',type='double',tag='out'),
var2=VarDef(name='MaxFreq',type='double',tag='out'),
var3=VarDef(name='StepFreq',type='double',tag='out'),
var4=VarDef(name='MinVolt',type='double',tag='out'),
var5=VarDef(name='MaxVolt',type='double',tag='out'),
var6=VarDef(name='StepVolt',type='double',tag='out'),
)

Argument(name='zes_diag_test_t',
var1=VarDef(name='index',type='uint32_t',tag='out'),
var2=VarDef(name='name[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
)

Argument(name='zes_diag_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='name[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var6=VarDef(name='haveTests',type='ze_bool_t',tag='out'),
)

Argument(name='zes_device_ecc_desc_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='state',type='zes_device_ecc_state_t',tag='out'),
)

Argument(name='zes_device_ecc_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='currentState',type='zes_device_ecc_state_t',tag='out'),
var4=VarDef(name='pendingState',type='zes_device_ecc_state_t',tag='out'),
var5=VarDef(name='pendingAction',type='zes_device_action_t',tag='out'),
)

Argument(name='zes_engine_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_engine_group_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
)

Argument(name='zes_engine_stats_t',
var1=VarDef(name='activeTime',type='uint64_t',tag='out'),
var2=VarDef(name='timestamp',type='uint64_t',tag='out'),
)

Argument(name='zes_fabric_port_id_t',
var1=VarDef(name='fabricId',type='uint32_t',tag='out'),
var2=VarDef(name='attachId',type='uint32_t',tag='out'),
var3=VarDef(name='portNumber',type='uint8_t',tag='out'),
)

Argument(name='zes_fabric_port_speed_t',
var1=VarDef(name='bitRate',type='int64_t',tag='out'),
var2=VarDef(name='width',type='int32_t',tag='out'),
)

Argument(name='zes_fabric_port_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='model[ZES_MAX_FABRIC_PORT_MODEL_SIZE]',type='char',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var6=VarDef(name='portId',type='zes_fabric_port_id_t',tag='out'),
var7=VarDef(name='maxRxSpeed',type='zes_fabric_port_speed_t',tag='out'),
var8=VarDef(name='maxTxSpeed',type='zes_fabric_port_speed_t',tag='out'),
)

Argument(name='zes_fabric_link_type_t',
var1=VarDef(name='desc[ZES_MAX_FABRIC_LINK_TYPE_SIZE]',type='char',tag='out'),
)

Argument(name='zes_fabric_port_config_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='enabled',type='ze_bool_t',tag='inout'),
var4=VarDef(name='beaconing',type='ze_bool_t',tag='inout'),
)

Argument(name='zes_fabric_port_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='status',type='zes_fabric_port_status_t',tag='out'),
var4=VarDef(name='qualityIssues',type='zes_fabric_port_qual_issue_flags_t',tag='out'),
var5=VarDef(name='failureReasons',type='zes_fabric_port_failure_flags_t',tag='out'),
var6=VarDef(name='remotePortId',type='zes_fabric_port_id_t',tag='out'),
var7=VarDef(name='rxSpeed',type='zes_fabric_port_speed_t',tag='out'),
var8=VarDef(name='txSpeed',type='zes_fabric_port_speed_t',tag='out'),
)

Argument(name='zes_fabric_port_throughput_t',
var1=VarDef(name='timestamp',type='uint64_t',tag='out'),
var2=VarDef(name='rxCounter',type='uint64_t',tag='out'),
var3=VarDef(name='txCounter',type='uint64_t',tag='out'),
)

Argument(name='zes_fabric_port_error_counters_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='linkFailureCount',type='uint64_t',tag='out'),
var4=VarDef(name='fwCommErrorCount',type='uint64_t',tag='out'),
var5=VarDef(name='fwErrorCount',type='uint64_t',tag='out'),
var6=VarDef(name='linkDegradeCount',type='uint64_t',tag='out'),
)

Argument(name='zes_fan_speed_t',
var1=VarDef(name='speed',type='int32_t',tag='inout'),
var2=VarDef(name='units',type='zes_fan_speed_units_t',tag='inout'),
)

Argument(name='zes_fan_temp_speed_t',
var1=VarDef(name='temperature',type='uint32_t',tag='inout'),
var2=VarDef(name='speed',type='zes_fan_speed_t',tag='inout'),
)

Argument(name='zes_fan_speed_table_t',
var1=VarDef(name='numPoints',type='int32_t',tag='inout'),
var2=VarDef(name='table[ZES_FAN_TEMP_SPEED_PAIR_COUNT]',type='zes_fan_temp_speed_t',tag='inout'),
)

Argument(name='zes_fan_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='canControl',type='ze_bool_t',tag='out'),
var6=VarDef(name='supportedModes',type='uint32_t',tag='out'),
var7=VarDef(name='supportedUnits',type='uint32_t',tag='out'),
var8=VarDef(name='maxRPM',type='int32_t',tag='out'),
var9=VarDef(name='maxPoints',type='int32_t',tag='out'),
)

Argument(name='zes_fan_config_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='mode',type='zes_fan_speed_mode_t',tag='inout'),
var4=VarDef(name='speedFixed',type='zes_fan_speed_t',tag='inout'),
var5=VarDef(name='speedTable',type='zes_fan_speed_table_t',tag='out'),
)

Argument(name='zes_firmware_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='canControl',type='ze_bool_t',tag='out'),
var6=VarDef(name='name[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
var7=VarDef(name='version[ZES_STRING_PROPERTY_SIZE]',type='char',tag='out'),
)

Argument(name='zes_freq_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_freq_domain_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var6=VarDef(name='canControl',type='ze_bool_t',tag='out'),
var7=VarDef(name='isThrottleEventSupported',type='ze_bool_t',tag='out'),
var8=VarDef(name='min',type='double',tag='out'),
var9=VarDef(name='max',type='double',tag='out'),
)

Argument(name='zes_freq_range_t',
var1=VarDef(name='min',type='double',tag='inout'),
var2=VarDef(name='max',type='double',tag='inout'),
)

Argument(name='zes_freq_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='currentVoltage',type='double',tag='out'),
var4=VarDef(name='request',type='double',tag='out'),
var5=VarDef(name='tdp',type='double',tag='out'),
var6=VarDef(name='efficient',type='double',tag='out'),
var7=VarDef(name='actual',type='double',tag='out'),
var8=VarDef(name='throttleReasons',type='zes_freq_throttle_reason_flags_t',tag='out'),
)

Argument(name='zes_freq_throttle_time_t',
var1=VarDef(name='throttleTime',type='uint64_t',tag='out'),
var2=VarDef(name='timestamp',type='uint64_t',tag='out'),
)

Argument(name='zes_oc_capabilities_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='isOcSupported',type='ze_bool_t',tag='out'),
var4=VarDef(name='maxFactoryDefaultFrequency',type='double',tag='out'),
var5=VarDef(name='maxFactoryDefaultVoltage',type='double',tag='out'),
var6=VarDef(name='maxOcFrequency',type='double',tag='out'),
var7=VarDef(name='minOcVoltageOffset',type='double',tag='out'),
var8=VarDef(name='maxOcVoltageOffset',type='double',tag='out'),
var9=VarDef(name='maxOcVoltage',type='double',tag='out'),
var10=VarDef(name='isTjMaxSupported',type='ze_bool_t',tag='out'),
var11=VarDef(name='isIccMaxSupported',type='ze_bool_t',tag='out'),
var12=VarDef(name='isHighVoltModeCapable',type='ze_bool_t',tag='out'),
var13=VarDef(name='isHighVoltModeEnabled',type='ze_bool_t',tag='out'),
var14=VarDef(name='isExtendedModeSupported',type='ze_bool_t',tag='out'),
var15=VarDef(name='isFixedModeSupported',type='ze_bool_t',tag='out'),
)

Argument(name='zes_led_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='canControl',type='ze_bool_t',tag='out'),
var6=VarDef(name='haveRGB',type='ze_bool_t',tag='out'),
)

Argument(name='zes_led_color_t',
var1=VarDef(name='red',type='double',tag='inout'),
var2=VarDef(name='green',type='double',tag='inout'),
var3=VarDef(name='blue',type='double',tag='inout'),
)

Argument(name='zes_led_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='isOn',type='ze_bool_t',tag='out'),
var4=VarDef(name='color',type='zes_led_color_t',tag='out'),
)

Argument(name='zes_mem_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_mem_type_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var6=VarDef(name='location',type='zes_mem_loc_t',tag='out'),
var7=VarDef(name='physicalSize',type='uint64_t',tag='out'),
var8=VarDef(name='busWidth',type='int32_t',tag='out'),
var9=VarDef(name='numChannels',type='int32_t',tag='out'),
)

Argument(name='zes_mem_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='health',type='zes_mem_health_t',tag='out'),
var4=VarDef(name='free',type='uint64_t',tag='out'),
var5=VarDef(name='size',type='uint64_t',tag='out'),
)

Argument(name='zes_mem_bandwidth_t',
var1=VarDef(name='readCounter',type='uint64_t',tag='out'),
var2=VarDef(name='writeCounter',type='uint64_t',tag='out'),
var3=VarDef(name='maxBandwidth',type='uint64_t',tag='out'),
var4=VarDef(name='timestamp',type='uint64_t',tag='out'),
)

Argument(name='zes_mem_ext_bandwidth_t',
var1=VarDef(name='memoryTimestampValidBits',type='uint32_t',tag='out'),
)

Argument(name='zes_perf_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='engines',type='zes_engine_type_flags_t',tag='out'),
)

Argument(name='zes_power_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='canControl',type='ze_bool_t',tag='out'),
var6=VarDef(name='isEnergyThresholdSupported',type='ze_bool_t',tag='out'),
var7=VarDef(name='defaultLimit',type='int32_t',tag='out'),
var8=VarDef(name='minLimit',type='int32_t',tag='out'),
var9=VarDef(name='maxLimit',type='int32_t',tag='out'),
)

Argument(name='zes_power_energy_counter_t',
var1=VarDef(name='energy',type='uint64_t',tag='out'),
var2=VarDef(name='timestamp',type='uint64_t',tag='out'),
)

Argument(name='zes_power_sustained_limit_t',
var1=VarDef(name='enabled',type='ze_bool_t',tag='inout'),
var2=VarDef(name='power',type='int32_t',tag='inout'),
var3=VarDef(name='interval',type='int32_t',tag='inout'),
)

Argument(name='zes_power_burst_limit_t',
var1=VarDef(name='enabled',type='ze_bool_t',tag='inout'),
var2=VarDef(name='power',type='int32_t',tag='inout'),
)

Argument(name='zes_power_peak_limit_t',
var1=VarDef(name='powerAC',type='int32_t',tag='inout'),
var2=VarDef(name='powerDC',type='int32_t',tag='inout'),
)

Argument(name='zes_energy_threshold_t',
var1=VarDef(name='enable',type='ze_bool_t',tag='inout'),
var2=VarDef(name='threshold',type='double',tag='inout'),
var3=VarDef(name='processId',type='uint32_t',tag='inout'),
)

Argument(name='zes_psu_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='haveFan',type='ze_bool_t',tag='out'),
var6=VarDef(name='ampLimit',type='int32_t',tag='out'),
)

Argument(name='zes_psu_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='voltStatus',type='zes_psu_voltage_status_t',tag='out'),
var4=VarDef(name='fanFailed',type='ze_bool_t',tag='out'),
var5=VarDef(name='temperature',type='int32_t',tag='out'),
var6=VarDef(name='current',type='int32_t',tag='out'),
)

Argument(name='zes_ras_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_ras_error_type_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
)

Argument(name='zes_ras_state_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='category[ZES_MAX_RAS_ERROR_CATEGORY_COUNT]',type='uint64_t',tag='in'),
)

Argument(name='zes_ras_config_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='totalThreshold',type='uint64_t',tag='inout'),
var4=VarDef(name='detailedThresholds',type='zes_ras_state_t',tag='inout'),
)

Argument(name='zes_sched_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var4=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var5=VarDef(name='canControl',type='ze_bool_t',tag='out'),
var6=VarDef(name='engines',type='zes_engine_type_flags_t',tag='out'),
var7=VarDef(name='supportedModes',type='uint32_t',tag='out'),
)

Argument(name='zes_sched_timeout_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='watchdogTimeout',type='uint64_t',tag='inout'),
)

Argument(name='zes_sched_timeslice_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='interval',type='uint64_t',tag='inout'),
var4=VarDef(name='yieldTimeout',type='uint64_t',tag='inout'),
)

Argument(name='zes_standby_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_standby_type_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
)

Argument(name='zes_temp_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_temp_sensors_t',tag='out'),
var4=VarDef(name='onSubdevice',type='ze_bool_t',tag='out'),
var5=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var6=VarDef(name='maxTemperature',type='double',tag='out'),
var7=VarDef(name='isCriticalTempSupported',type='ze_bool_t',tag='out'),
var8=VarDef(name='isThreshold1Supported',type='ze_bool_t',tag='out'),
var9=VarDef(name='isThreshold2Supported',type='ze_bool_t',tag='out'),
)

Argument(name='zes_temp_threshold_t',
var1=VarDef(name='enableLowToHigh',type='ze_bool_t',tag='inout'),
var2=VarDef(name='enableHighToLow',type='ze_bool_t',tag='inout'),
var3=VarDef(name='threshold',type='double',tag='inout'),
)

Argument(name='zes_temp_config_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='enableCritical',type='ze_bool_t',tag='inout'),
var4=VarDef(name='threshold1',type='zes_temp_threshold_t',tag='inout'),
var5=VarDef(name='threshold2',type='zes_temp_threshold_t',tag='inout'),
)

Argument(name='zes_power_limit_ext_desc_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='level',type='zes_power_level_t',tag='inout'),
var4=VarDef(name='source',type='zes_power_source_t',tag='out'),
var5=VarDef(name='limitUnit',type='zes_limit_unit_t',tag='out'),
var6=VarDef(name='enabledStateLocked',type='ze_bool_t',tag='out'),
var7=VarDef(name='enabled',type='ze_bool_t',tag='inout'),
var8=VarDef(name='intervalValueLocked',type='ze_bool_t',tag='out'),
var9=VarDef(name='interval',type='int32_t',tag='inout'),
var10=VarDef(name='limitValueLocked',type='ze_bool_t',tag='out'),
var11=VarDef(name='limit',type='int32_t',tag='inout'),
)

# Argument(name='zes_power_ext_properties_t',
# var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
# var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
# var3=VarDef(name='domain',type='zes_power_domain_t',tag='out'),
# var4=VarDef(name='defaultLimit',type='zes_power_limit_ext_desc_t*',tag='out'),
# )

Argument(name='zes_engine_ext_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='countOfVirtualFunctionInstance',type='uint32_t',tag='out'),
)

Argument(name='zes_ras_state_exp_t',
var1=VarDef(name='category',type='zes_ras_error_category_exp_t',tag='out'),
var2=VarDef(name='errorCounter',type='uint64_t',tag='out'),
)

Argument(name='zes_mem_page_offline_state_exp_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='memoryPageOffline',type='uint32_t',tag='out'),
var4=VarDef(name='maxMemoryPageOffline',type='uint32_t',tag='out'),
)

# Deprecated
Argument(name='zes_mem_timestamp_bits_exp_t',
var1=VarDef(name='memoryTimestampValidBits',type='uint32_t',tag='out'),
)

Argument(name='zes_mem_bandwidth_counter_bits_exp_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='validBitsCount',type='uint32_t',tag='out'),
)

Argument(name='zes_power_domain_exp_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='powerDomain',type='zes_power_domain_t',tag='out'),
)

Argument(name='zes_subdevice_exp_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='subdeviceId',type='uint32_t',tag='out'),
var4=VarDef(name='uuid',type='zes_uuid_t',tag='out'),
)

Argument(name='zes_vf_exp_properties_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='address',type='zes_pci_address_t',tag='out'),
var4=VarDef(name='uuid',type='zes_uuid_t',tag='out'),
var5=VarDef(name='flags',type='zes_vf_info_util_exp_flags_t',tag='out'),
)

Argument(name='zes_vf_util_mem_exp_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='memTypeFlags',type='zes_vf_info_mem_type_exp_flags_t',tag='out'),
var4=VarDef(name='free',type='uint64_t',tag='out'),
var5=VarDef(name='size',type='uint64_t',tag='out'),
var6=VarDef(name='timestamp',type='uint64_t',tag='out'),
)

Argument(name='zes_vf_util_engine_exp_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='type',type='zes_engine_group_t',tag='out'),
var4=VarDef(name='activeCounterValue',type='uint64_t',tag='out'),
var5=VarDef(name='samplingCounterValue',type='uint64_t',tag='out'),
var6=VarDef(name='timestamp',type='uint64_t',tag='out'),
)

Argument(name='zes_vf_exp_capabilities_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='void*',tag='inout',wrapType='CExtensionStructSysman'),
var3=VarDef(name='address',type='zes_pci_address_t',tag='out'),
var4=VarDef(name='vfDeviceMemSize',type='uint32_t',tag='out'),
var5=VarDef(name='vfID',type='uint32_t',tag='out'),
)

Argument(name='zes_vf_util_mem_exp2_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='vfMemLocation',type='zes_mem_loc_t',tag='out'),
var4=VarDef(name='vfMemUtilized',type='uint64_t',tag='out'),
)

Argument(name='zes_vf_util_engine_exp2_t',
var1=VarDef(name='stype',type='zes_structure_type_t',tag='in'),
var2=VarDef(name='pNext',type='const void*',tag='in',wrapType='CExtensionStructSysman'),
var3=VarDef(name='vfEngineType',type='zes_engine_group_t',tag='out'),
var4=VarDef(name='activeCounterValue',type='uint64_t',tag='out'),
var5=VarDef(name='samplingCounterValue',type='uint64_t',tag='out'),
)

Function(name='zeCommandListAppendBarrier',component='ze_command_list',enabled=True,ddi_pos=6,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg3=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg4=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendEventReset',component='ze_command_list',enabled=True,ddi_pos=20,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
)

Function(name='zeCommandListAppendImageCopy',component='ze_command_list',enabled=True,ddi_pos=12,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hDstImage',type='ze_image_handle_t',tag='in'),
arg3=ArgDef(name='hSrcImage',type='ze_image_handle_t',tag='in'),
arg4=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg5=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg6=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendImageCopyFromMemory',component='ze_command_list',enabled=True,recWrap=True,ddi_pos=15,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hDstImage',type='ze_image_handle_t',tag='in'),
arg3=ArgDef(name='srcptr',type='const void*',tag='in',wrapType='CUSMPtr',wrapParams='{name}, hDstImage'),
arg4=ArgDef(name='pDstRegion',type='const ze_image_region_t*',tag='in',optional=True),
arg5=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg6=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg7=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendImageCopyFromMemoryExt',component='ze_command_list',enabled=False,api_version='1.3',ddi_pos=27,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hDstImage',type='ze_image_handle_t',tag='in',wrapType='CUSMPtr',wrapParams='{name}, hDstImage'),
arg3=ArgDef(name='srcptr',type='const void*',tag='in',wrapType='CUSMPtr'),
arg4=ArgDef(name='pDstRegion',type='const ze_image_region_t*',tag='in',optional=True),
arg5=ArgDef(name='srcRowPitch',type='uint32_t',tag='in'),
arg6=ArgDef(name='srcSlicePitch',type='uint32_t',tag='in'),
arg7=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg8=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg9=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendImageCopyRegion',component='ze_command_list',enabled=True,ddi_pos=13,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hDstImage',type='ze_image_handle_t',tag='in'),
arg3=ArgDef(name='hSrcImage',type='ze_image_handle_t',tag='in'),
arg4=ArgDef(name='pDstRegion',type='const ze_image_region_t*',tag='in',optional=True),
arg5=ArgDef(name='pSrcRegion',type='const ze_image_region_t*',tag='in',optional=True),
arg6=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg8=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendImageCopyToMemory',component='ze_command_list',enabled=True,ddi_pos=14,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='void*',tag='in',wrapType='CUSMPtr',wrapParams='{name}, hSrcImage'),
arg3=ArgDef(name='hSrcImage',type='ze_image_handle_t',tag='in'),
arg4=ArgDef(name='pSrcRegion',type='const ze_image_region_t*',tag='in',optional=True),
arg5=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg6=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg7=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendImageCopyToMemoryExt',component='ze_command_list',enabled=False,api_version='1.3',ddi_pos=26,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='void*',tag='in',wrapType='CUSMPtr',wrapParams='{name}, hSrcImage'),
arg3=ArgDef(name='hSrcImage',type='ze_image_handle_t',tag='in'),
arg4=ArgDef(name='pSrcRegion',type='const ze_image_region_t*',tag='in',optional=True),
arg5=ArgDef(name='destRowPitch',type='uint32_t',tag='in'),
arg6=ArgDef(name='destSlicePitch',type='uint32_t',tag='in'),
arg7=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg8=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg9=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendLaunchCooperativeKernel',component='ze_command_list',enabled=True,stateTrack=True,recWrap=True,recExecWrap=True,runWrap=True,nomenclatureModifier=True,ddi_pos=23,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg3=ArgDef(name='pLaunchFuncArgs',type='const ze_group_count_t*',tag='in'),
arg4=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg5=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg6=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendLaunchKernel',component='ze_command_list',enabled=True,stateTrack=True,recWrap=True,recExecWrap=True,runWrap=True,nomenclatureModifier=True,ddi_pos=22,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg3=ArgDef(name='pLaunchFuncArgs',type='const ze_group_count_t*',tag='in'),
arg4=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg5=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg6=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendLaunchKernelIndirect',component='ze_command_list',enabled=True,stateTrack=True,recWrap=True,recExecWrap=True,runWrap=True,nomenclatureModifier=True,ddi_pos=24,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg3=ArgDef(name='pLaunchArgumentsBuffer',type='const ze_group_count_t*',tag='in'),
arg4=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg5=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg6=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendLaunchMultipleKernelsIndirect',component='ze_command_list',enabled=True,stateTrack=True,recWrap=True,recExecWrap=True,runWrap=True,nomenclatureModifier=True,ddi_pos=25,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='numKernels',type='uint32_t',tag='in'),
arg3=ArgDef(name='phKernels',type='ze_kernel_handle_t*',tag='in',range='0,numKernels'),
arg4=ArgDef(name='pCountBuffer',type='const uint32_t*',tag='in'),
arg5=ArgDef(name='pLaunchArgumentsBuffer',type='const ze_group_count_t*',tag='in',range='0,numKernels'),
arg6=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg8=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemAdvise',component='ze_command_list',enabled=True,ddi_pos=17,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='const void*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='advice',type='ze_memory_advice_t',tag='in'),
)

Function(name='zeCommandListAppendMemoryCopy',component='ze_command_list',enabled=True,recWrap=True,stateTrack=True,ddi_pos=8,unprotectLogic=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='void*',tag='in',wrapType='CUSMPtr',wrapParams='size, {name}'),
arg3=ArgDef(name='srcptr',type='const void*',tag='in',wrapType='CUSMPtr',wrapParams='size, {name}, dstptr'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg6=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg7=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemoryCopyFromContext',component='ze_command_list',enabled=True,recWrap=True,ddi_pos=11,unprotectLogic=True,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='void*',tag='in',wrapType='CUSMPtr',wrapParams='size, {name}'),
arg3=ArgDef(name='hContextSrc',type='ze_context_handle_t',tag='in'),
arg4=ArgDef(name='srcptr',type='const void*',tag='in',wrapType='CUSMPtr',wrapParams='size, {name}'),
arg5=ArgDef(name='size',type='size_t',tag='in'),
arg6=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg8=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemoryCopyRegion',component='ze_command_list',enabled=True,ddi_pos=10,unprotectLogic=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='void*',tag='in'),
arg3=ArgDef(name='dstRegion',type='const ze_copy_region_t*',tag='in'),
arg4=ArgDef(name='dstPitch',type='uint32_t',tag='in'),
arg5=ArgDef(name='dstSlicePitch',type='uint32_t',tag='in'),
arg6=ArgDef(name='srcptr',type='const void*',tag='in'),
arg7=ArgDef(name='srcRegion',type='const ze_copy_region_t*',tag='in'),
arg8=ArgDef(name='srcPitch',type='uint32_t',tag='in'),
arg9=ArgDef(name='srcSlicePitch',type='uint32_t',tag='in'),
arg10=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg11=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg12=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemoryCopyRegion',component='ze_command_list',enabled=True,recWrap=True,version=1,ddi_pos=10,unprotectLogic=True,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='void*',tag='in',wrapType='CUSMPtr',wrapParams='GetSizeFromCopyRegion(dstRegion), {name}'),
arg3=ArgDef(name='dstRegion',type='const ze_copy_region_t*',tag='in'),
arg4=ArgDef(name='dstPitch',type='uint32_t',tag='in'),
arg5=ArgDef(name='dstSlicePitch',type='uint32_t',tag='in'),
arg6=ArgDef(name='srcptr',type='const void*',tag='in',wrapType='CUSMPtr',wrapParams='GetSizeFromCopyRegion(srcRegion), {name}'),
arg7=ArgDef(name='srcRegion',type='const ze_copy_region_t*',tag='in'),
arg8=ArgDef(name='srcPitch',type='uint32_t',tag='in'),
arg9=ArgDef(name='srcSlicePitch',type='uint32_t',tag='in'),
arg10=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg11=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg12=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemoryFill',component='ze_command_list',enabled=True,ddi_pos=9,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='void*',tag='in'),
arg3=ArgDef(name='pattern',type='const void*',tag='in'),
arg4=ArgDef(name='pattern_size',type='size_t',tag='in'),
arg5=ArgDef(name='size',type='size_t',tag='in'),
arg6=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg8=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemoryFill',component='ze_command_list',enabled=True,version=1,ddi_pos=9,stateTrack=True,recExecWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='void*',tag='in',wrapType='CUSMPtr'),
arg3=ArgDef(name='pattern',type='const void*',tag='in',wrapType='CBinaryData',wrapParams='pattern_size, {name}'),
arg4=ArgDef(name='pattern_size',type='size_t',tag='in'),
arg5=ArgDef(name='size',type='size_t',tag='in'),
arg6=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg8=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendMemoryPrefetch',component='ze_command_list',enabled=True,ddi_pos=16,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeCommandListAppendMemoryRangesBarrier',component='ze_command_list',enabled=True,ddi_pos=7,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='numRanges',type='uint32_t',tag='in'),
arg3=ArgDef(name='pRangeSizes',type='const size_t*',tag='in',range='0,numRanges'),
arg4=ArgDef(name='pRanges',type='const void**',tag='in',range='0,numRanges',wrapType='CBufferArray'),
arg5=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg6=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg7=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendQueryKernelTimestamps',component='ze_command_list',enabled=True,runWrap=True,ddi_pos=21,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='numEvents',type='uint32_t',tag='in'),
arg3=ArgDef(name='phEvents',type='ze_event_handle_t*',tag='in',range='0,numEvents'),
arg4=ArgDef(name='dstptr',type='void*',tag='inout'),
arg5=ArgDef(name='pOffsets',type='const size_t*',tag='in',range='0,numEvents',optional=True),
arg6=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg8=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListAppendSignalEvent',component='ze_command_list',enabled=True,ddi_pos=18,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
)

Function(name='zeCommandListAppendWaitOnEvents',component='ze_command_list',enabled=True,ddi_pos=19,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='numEvents',type='uint32_t',tag='in'),
arg3=ArgDef(name='phEvents',type='ze_event_handle_t*',tag='in',range='0,numEvents'),
)

Function(name='zeCommandListAppendWriteGlobalTimestamp',component='ze_command_list',enabled=True,ddi_pos=5,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='uint64_t*',tag='inout'),
arg3=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg4=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg5=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListClose',component='ze_command_list',enabled=True,ddi_pos=3,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
)

Function(name='zeCommandListCreate',component='ze_command_list',enabled=True,stateTrack=True,runWrap=True,recWrap=True,nomenclatureModifier=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_command_list_desc_t*',tag='in'),
arg4=ArgDef(name='phCommandList',type='ze_command_list_handle_t*',tag='out'),
)

Function(name='zeCommandListCreateCloneExp',component='ze_command_list_exp',enabled=False,api_version='1.9',ddi_pos=0,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='phClonedCommandList',type='ze_command_list_handle_t*',tag='out'),
)

Function(name='zeCommandListCreateImmediate',component='ze_command_list',enabled=True,stateTrack=True,runWrap=True,recWrap=True,nomenclatureModifier=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='altdesc',type='const ze_command_queue_desc_t*',tag='in'),
arg4=ArgDef(name='phCommandList',type='ze_command_list_handle_t*',tag='out'),
)

Function(name='zeCommandListDestroy',component='ze_command_list',enabled=True,stateTrack=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in',release=True),
)

Function(name='zeCommandListGetContextHandle',component='ze_command_list',enabled=True,api_version='1.9',ddi_pos=30,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='phContext',type='ze_context_handle_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeCommandListGetDeviceHandle',component='ze_command_list',enabled=True,api_version='1.9',ddi_pos=29,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='phDevice',type='ze_device_handle_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeCommandListGetNextCommandIdExp',component='ze_command_list_exp',enabled=True,api_version='1.9',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_mutable_command_id_exp_desc_t*',tag='in'),
arg3=ArgDef(name='pCommandId',type='uint64_t*',tag='out',wrapType='Cuint64_t::CSArray'),
)

Function(name='zeCommandListGetNextCommandIdWithKernelsExp',component='ze_command_list_exp',enabled=True,api_version='1.10',ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_mutable_command_id_exp_desc_t*',tag='in'),
arg3=ArgDef(name='numKernels',type='uint32_t',tag='in',optional=True),
arg4=ArgDef(name='phKernels',type='ze_kernel_handle_t*',tag='in',range='0,numKernels',optional=True),
arg5=ArgDef(name='pCommandId',type='uint64_t*',tag='out',wrapType='Cuint64_t::CSArray'),
)

Function(name='zeCommandListGetOrdinal',component='ze_command_list',enabled=True,api_version='1.9',ddi_pos=31,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='pOrdinal',type='uint32_t*',tag='out',wrapType='Cuint32_t::CSArray'),
)

Function(name='zeCommandListHostSynchronize',component='ze_command_list',enabled=True,api_version='1.6',ddi_pos=28,protectLogic=True,stateTrack=True,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint64_t',tag='in'),
)

Function(name='zeCommandListImmediateAppendCommandListsExp',component='ze_command_list_exp',enabled=True,api_version='1.9',ddi_pos=3,stateTrack=True,recExecWrap=True,recWrap=True,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandListImmediate',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='numCommandLists',type='uint32_t',tag='in'),
arg3=ArgDef(name='phCommandLists',type='ze_command_list_handle_t*',tag='in',range='0,numCommandLists'),
arg4=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg5=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg6=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListImmediateGetIndex',component='ze_command_list',enabled=True,api_version='1.9',ddi_pos=32,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandListImmediate',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='pIndex',type='uint32_t*',tag='out',wrapType='Cuint32_t::CSArray'),
)

Function(name='zeCommandListIsImmediate',component='ze_command_list',enabled=True,api_version='1.9',ddi_pos=33,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='pIsImmediate',type='ze_bool_t*',tag='out',wrapType='Cuint8_t::CSArray'),
)

Function(name='zeCommandListReset',component='ze_command_list',enabled=True,stateTrack=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
)

Function(name='zeCommandListUpdateMutableCommandKernelsExp',component='ze_command_list_exp',enabled=False,api_version='1.10',ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='numKernels',type='uint32_t',tag='in'),
arg3=ArgDef(name='pCommandId',type='uint64_t*',tag='in',range='0,numKernels'),
arg4=ArgDef(name='phKernels',type='ze_kernel_handle_t*',tag='in',range='0,numKernels'),
)

Function(name='zeCommandListUpdateMutableCommandSignalEventExp',component='ze_command_list_exp',enabled=False,api_version='1.10',ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='commandId',type='uint64_t',tag='in'),
arg3=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
)

Function(name='zeCommandListUpdateMutableCommandWaitEventsExp',component='ze_command_list_exp',enabled=False,api_version='1.9',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='commandId',type='uint64_t',tag='in'),
arg3=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg4=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zeCommandListUpdateMutableCommandsExp',component='ze_command_list_exp',enabled=False,api_version='1.10',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='ze_command_list_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_mutable_commands_exp_desc_t*',tag='in'),
)

Function(name='zeCommandQueueCreate',component='ze_command_queue',enabled=True,stateTrack=True,runWrap=True,recWrap=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_command_queue_desc_t*',tag='in'),
arg4=ArgDef(name='phCommandQueue',type='ze_command_queue_handle_t*',tag='out'),
)

Function(name='zeCommandQueueDestroy',component='ze_command_queue',enabled=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandQueue',type='ze_command_queue_handle_t',tag='in',release=True),
)

Function(name='zeCommandQueueExecuteCommandLists',component='ze_command_queue',enabled=True,stateTrack=True,recExecWrap=True,recWrap=True,runWrap=True,nomenclatureModifier=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandQueue',type='ze_command_queue_handle_t',tag='in'),
arg2=ArgDef(name='numCommandLists',type='uint32_t',tag='in'),
arg3=ArgDef(name='phCommandLists',type='ze_command_list_handle_t*',tag='in',range='0,numCommandLists'),
arg4=ArgDef(name='hFence',type='ze_fence_handle_t',tag='in',optional=True),
)

Function(name='zeCommandQueueGetIndex',component='ze_command_queue',enabled=True,api_version='1.9',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandQueue',type='ze_command_queue_handle_t',tag='in'),
arg2=ArgDef(name='pIndex',type='uint32_t*',tag='out',wrapType='Cuint32_t::CSArray'),
)

Function(name='zeCommandQueueGetOrdinal',component='ze_command_queue',enabled=True,api_version='1.9',ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandQueue',type='ze_command_queue_handle_t',tag='in'),
arg2=ArgDef(name='pOrdinal',type='uint32_t*',tag='out',wrapType='Cuint32_t::CSArray'),
)

Function(name='zeCommandQueueSynchronize',component='ze_command_queue',enabled=True,ddi_pos=3,stateTrack=True,protectLogic=True,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandQueue',type='ze_command_queue_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint64_t',tag='in'),
)

Function(name='zeContextCreate',component='ze_context',enabled=True,stateTrack=True,ddi_pos=0,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_context_desc_t*',tag='in'),
arg3=ArgDef(name='phContext',type='ze_context_handle_t*',tag='out'),
)

Function(name='zeContextCreateEx',component='ze_context',enabled=True,stateTrack=True,api_version='1.1',ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_context_desc_t*',tag='in'),
arg3=ArgDef(name='numDevices',type='uint32_t',tag='in',optional=True),
arg4=ArgDef(name='phDevices',type='ze_device_handle_t*',tag='in',range='0, numDevices',optional=True),
arg5=ArgDef(name='phContext',type='ze_context_handle_t*',tag='out'),
)

Function(name='zeContextDestroy',component='ze_context',enabled=True,stateTrack=True,ddi_pos=1,runWrap=True,recExecWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in',release=True),
)

Function(name='zeContextEvictImage',component='ze_context',enabled=True,ddi_pos=7,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
)

Function(name='zeContextEvictMemory',component='ze_context',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='void*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeContextEvictMemory',component='ze_context',enabled=True,ddi_pos=5,version=1,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='void*',tag='in',wrapType='CUSMPtr'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeContextGetStatus',component='ze_context',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
)

Function(name='zeContextMakeImageResident',component='ze_context',enabled=True,ddi_pos=6,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
)

Function(name='zeContextMakeMemoryResident',component='ze_context',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='void*',tag='in',),
arg4=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeContextMakeMemoryResident',component='ze_context',enabled=True,ddi_pos=4,version=1,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='void*',tag='in',wrapType='CUSMPtr'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeContextSystemBarrier',component='ze_context',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
)

Function(name='zeDeviceCanAccessPeer',component='ze_device',enabled=True,skipRun=True,ddi_pos=12,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='hPeerDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='value',type='ze_bool_t*',tag='out'),
)

Function(name='zeDeviceGet',component='ze_device',enabled=True,stateTrack=True,recWrap=True,ddi_pos=0,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phDevices',type='ze_device_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeDeviceGetCacheProperties',component='ze_device',enabled=True,recWrap=True,ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pCacheProperties',type='ze_device_cache_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeDeviceGetCommandQueueGroupProperties',component='ze_device',enabled=True,recWrap=True,stateTrack=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pCommandQueueGroupProperties',type='ze_command_queue_group_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeDeviceGetComputeProperties',component='ze_device',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pComputeProperties',type='ze_device_compute_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetExternalMemoryProperties',component='ze_device',enabled=True,ddi_pos=10,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pExternalMemoryProperties',type='ze_device_external_memory_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetFabricVertexExp',component='ze_device_exp',enabled=True,api_version='1.4',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='phVertex',type='ze_fabric_vertex_handle_t*',tag='out'),
)

Function(name='zeDeviceGetGlobalTimestamps',component='ze_device',enabled=True,api_version='1.1',skipRun=True,ddi_pos=14,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='hostTimestamp',type='uint64_t*',tag='out'),
arg3=ArgDef(name='deviceTimestamp',type='uint64_t*',tag='out'),
)

Function(name='zeDeviceGetImageProperties',component='ze_device',enabled=True,ddi_pos=9,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pImageProperties',type='ze_device_image_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetMemoryAccessProperties',component='ze_device',enabled=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pMemAccessProperties',type='ze_device_memory_access_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetMemoryProperties',component='ze_device',enabled=True,recWrap=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pMemProperties',type='ze_device_memory_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeDeviceGetModuleProperties',component='ze_device',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pModuleProperties',type='ze_device_module_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetP2PProperties',component='ze_device',enabled=True,ddi_pos=11,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='hPeerDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='pP2PProperties',type='ze_device_p2p_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetProperties',component='ze_device',enabled=True,stateTrack=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pDeviceProperties',type='ze_device_properties_t*',tag='inout'),
)

Function(name='zeDeviceGetRootDevice',component='ze_device',enabled=True,api_version='1.7',ddi_pos=18,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='phRootDevice',type='ze_device_handle_t*',tag='inout'),
)

Function(name='zeDeviceGetStatus',component='ze_device',enabled=True,ddi_pos=13,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
)

Function(name='zeDeviceGetSubDevices',component='ze_device',enabled=True,recWrap=True,runWrap=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phSubdevices',type='ze_device_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

# DEPRECATED
Function(name='zeDeviceReserveCache',component='ze_deprecated',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='cacheLevel',type='size_t',tag='in'),
arg3=ArgDef(name='cacheReservationSize',type='size_t',tag='in'),
)

# DEPRECATED
Function(name='zeDeviceSetCacheAdvice',component='ze_deprecated',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='void*',tag='in'),
arg3=ArgDef(name='regionSize',type='size_t',tag='in'),
arg4=ArgDef(name='cacheRegion',type='ze_cache_ext_region_t',tag='in'),
)

Function(name='zeDevicePciGetPropertiesExt',component='ze_device',enabled=True,api_version='1.3',ddi_pos=17,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='pPciProperties',type='ze_pci_ext_properties_t*',tag='inout'),
)

Function(name='zeDeviceReserveCacheExt',component='ze_device',enabled=True,api_version='1.2',ddi_pos=15,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='cacheLevel',type='size_t',tag='in'),
arg3=ArgDef(name='cacheReservationSize',type='size_t',tag='in'),
)

Function(name='zeDeviceSetCacheAdviceExt',component='ze_device',enabled=True,api_version='1.2',ddi_pos=16,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='void*',tag='in'),
arg3=ArgDef(name='regionSize',type='size_t',tag='in'),
arg4=ArgDef(name='cacheRegion',type='ze_cache_ext_region_t',tag='in'),
)

Function(name='zeDriverGet',component='ze_driver',enabled=True,stateTrack=True,recWrap=True,ddi_pos=0,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg2=ArgDef(name='phDrivers',type='ze_driver_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeDriverGetApiVersion',component='ze_driver',enabled=True,skipRun=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='version',type='ze_api_version_t*',tag='out'),
)

Function(name='zeDriverGetExtensionFunctionAddress',component='ze_driver',enabled=True,api_version='1.1',recExecWrap=True,skipRun=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='name',type='const char*',tag='in'),
arg3=ArgDef(name='ppFunctionAddress',type='void**',tag='out'),
)

Function(name='zeDriverGetExtensionProperties',component='ze_driver',enabled=True,recWrap=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pExtensionProperties',type='ze_driver_extension_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeDriverGetIpcProperties',component='ze_driver',enabled=False,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='pIpcProperties',type='ze_driver_ipc_properties_t*',tag='inout'),
)

Function(name='zeDriverGetLastErrorDescription',component='ze_driver',enabled=True,api_version='1.6',runWrap=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='ppString',type='const char**',tag='inout',wrapType='COutArgument'),
)

Function(name='zeDriverGetProperties',component='ze_driver',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='pDriverProperties',type='ze_driver_properties_t*',tag='inout'),
)

Function(name='zeDriverRTASFormatCompatibilityCheckExp',component='ze_driver_exp',enabled=True,api_version='1.7',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='rtasFormatA',type='ze_rtas_format_exp_t',tag='in'),
arg3=ArgDef(name='rtasFormatB',type='ze_rtas_format_exp_t',tag='in'),
)

Function(name='zeEventCreate',component='ze_event',enabled=True,stateTrack=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEventPool',type='ze_event_pool_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_event_desc_t*',tag='in'),
arg3=ArgDef(name='phEvent',type='ze_event_handle_t*',tag='out'),
)

Function(name='zeEventDestroy',component='ze_event',enabled=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in',release=True),
)

Function(name='zeEventGetEventPool',component='ze_event',enabled=True,api_version='1.9',ddi_pos=8,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeEventGetSignalScope',component='ze_event',enabled=True,api_version='1.9',ddi_pos=9,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='pSignalScope',type='ze_event_scope_flags_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeEventGetWaitScope',component='ze_event',enabled=True,api_version='1.9',ddi_pos=10,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='pWaitScope',type='ze_event_scope_flags_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeEventHostReset',component='ze_event',enabled=True,ddi_pos=5,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
)

Function(name='zeEventHostSignal',component='ze_event',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
)

Function(name='zeEventHostSynchronize',component='ze_event',enabled=True,ddi_pos=3,protectLogic=True,stateTrack=True,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint64_t',tag='in'),
)

Function(name='zeEventPoolCloseIpcHandle',component='ze_event_pool',enabled=False,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEventPool',type='ze_event_pool_handle_t',tag='in',release=True),
)

Function(name='zeEventPoolCreate',component='ze_event_pool',enabled=True,stateTrack=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_event_pool_desc_t*',tag='in'),
arg3=ArgDef(name='numDevices',type='uint32_t',tag='in',optional=True),
arg4=ArgDef(name='phDevices',type='ze_device_handle_t*',tag='in',range='0,numDevices',optional=True),
arg5=ArgDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='out'),
)

Function(name='zeEventPoolDestroy',component='ze_event_pool',enabled=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEventPool',type='ze_event_pool_handle_t',tag='in',release=True),
)

Function(name='zeEventPoolGetContextHandle',component='ze_event_pool',enabled=True,api_version='1.9',ddi_pos=6,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEventPool',type='ze_event_pool_handle_t',tag='in'),
arg2=ArgDef(name='phContext',type='ze_context_handle_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeEventPoolGetFlags',component='ze_event_pool',enabled=True,api_version='1.9',ddi_pos=7,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEventPool',type='ze_event_pool_handle_t',tag='in'),
arg2=ArgDef(name='pFlags',type='ze_event_pool_flags_t*',tag='out',wrapType='COutArgument'),
)

Function(name='zeEventPoolGetIpcHandle',component='ze_event_pool',enabled=False,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEventPool',type='ze_event_pool_handle_t',tag='in'),
arg2=ArgDef(name='phIpc',type='ze_ipc_event_pool_handle_t*',tag='out'),
)

Function(name='zeEventPoolOpenIpcHandle',component='ze_event_pool',enabled=False,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hIpc',type='ze_ipc_event_pool_handle_t',tag='in'),
arg3=ArgDef(name='phEventPool',type='ze_event_pool_handle_t*',tag='out'),
)

Function(name='zeEventPoolPutIpcHandle',component='ze_event_pool',enabled=False,api_version='1.6',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hIpc',type='ze_ipc_event_pool_handle_t',tag='in'),
)

Function(name='zeEventQueryKernelTimestamp',component='ze_event',enabled=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='dstptr',type='ze_kernel_timestamp_result_t*',tag='inout'),
)

Function(name='zeEventQueryKernelTimestampsExt',component='ze_event',enabled=False,api_version='1.6',ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg4=ArgDef(name='pResults',type='ze_event_query_kernel_timestamps_results_ext_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeEventQueryStatus',component='ze_event',enabled=True,ddi_pos=4,unprotectLogic=True,stateTrack=True,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
)

Function(name='zeEventQueryTimestampsExp',component='ze_event_exp',enabled=True,api_version='1.2',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEvent',type='ze_event_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg4=ArgDef(name='pTimestamps',type='ze_kernel_timestamp_result_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeFabricEdgeGetExp',component='ze_fabric_edge_exp',enabled=True,api_version='1.4',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVertexA',type='ze_fabric_vertex_handle_t',tag='in'),
arg2=ArgDef(name='hVertexB',type='ze_fabric_vertex_handle_t',tag='in'),
arg3=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg4=ArgDef(name='phEdges',type='ze_fabric_edge_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeFabricEdgeGetPropertiesExp',component='ze_fabric_edge_exp',enabled=True,api_version='1.4',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEdge',type='ze_fabric_edge_handle_t',tag='in'),
arg2=ArgDef(name='pEdgeProperties',type='ze_fabric_edge_exp_properties_t*',tag='inout'),
)

Function(name='zeFabricEdgeGetVerticesExp',component='ze_fabric_edge_exp',enabled=False,api_version='1.4',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEdge',type='ze_fabric_edge_handle_t',tag='in'),
arg2=ArgDef(name='phVertexA',type='ze_fabric_vertex_handle_t*',tag='out'),
arg3=ArgDef(name='phVertexB',type='ze_fabric_vertex_handle_t*',tag='out'),
)

Function(name='zeFabricVertexGetDeviceExp',component='ze_fabric_vertex_exp',enabled=False,api_version='1.4',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVertex',type='ze_fabric_vertex_handle_t',tag='in'),
arg2=ArgDef(name='phDevice',type='ze_device_handle_t*',tag='out'),
)

Function(name='zeFabricVertexGetExp',component='ze_fabric_vertex_exp',enabled=True,api_version='1.4',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phVertices',type='ze_fabric_vertex_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeFabricVertexGetPropertiesExp',component='ze_fabric_vertex_exp',enabled=True,api_version='1.4',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVertex',type='ze_fabric_vertex_handle_t',tag='in'),
arg2=ArgDef(name='pVertexProperties',type='ze_fabric_vertex_exp_properties_t*',tag='inout'),
)

Function(name='zeFabricVertexGetSubVerticesExp',component='ze_fabric_vertex_exp',enabled=True,api_version='1.4',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVertex',type='ze_fabric_vertex_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phSubvertices',type='ze_fabric_vertex_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zeFenceCreate',component='ze_fence',enabled=True,stateTrack=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandQueue',type='ze_command_queue_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_fence_desc_t*',tag='in'),
arg3=ArgDef(name='phFence',type='ze_fence_handle_t*',tag='out'),
)

Function(name='zeFenceDestroy',component='ze_fence',enabled=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFence',type='ze_fence_handle_t',tag='in',release=True),
)

Function(name='zeFenceHostSynchronize',component='ze_fence',enabled=True,ddi_pos=2,stateTrack=True,protectLogic=True,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFence',type='ze_fence_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint64_t',tag='in'),
)

Function(name='zeFenceQueryStatus',component='ze_fence',enabled=True,runWrap=True,ddi_pos=3,stateTrack=True,unprotectLogic=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFence',type='ze_fence_handle_t',tag='in'),
)

Function(name='zeFenceReset',component='ze_fence',enabled=True,stateTrack=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFence',type='ze_fence_handle_t',tag='in'),
)

Function(name='zeImageCreate',component='ze_image',enabled=True,stateTrack=True,recWrap=True,runWrap=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_image_desc_t*',tag='in'),
arg4=ArgDef(name='phImage',type='ze_image_handle_t*',tag='out'),
)

Function(name='zeImageDestroy',component='ze_image',enabled=True,stateTrack=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hImage',type='ze_image_handle_t',tag='in',release=True),
)

Function(name='zeImageGetAllocPropertiesExt',component='ze_image',enabled=True,api_version='1.3',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
arg3=ArgDef(name='pImageAllocProperties',type='ze_image_allocation_ext_properties_t*',tag='inout'),
)

Function(name='zeImageGetDeviceOffsetExp',component='ze_image_exp',enabled=False,api_version='1.9',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
arg2=ArgDef(name='pDeviceOffset',type='uint64_t*',tag='out',wrapType='Cuint64_t::CSArray'),
)

Function(name='zeImageGetMemoryPropertiesExp',component='ze_image_exp',enabled=True,api_version='1.2',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
arg2=ArgDef(name='pMemoryProperties',type='ze_image_memory_properties_exp_t*',tag='inout'),
)

Function(name='zeImageGetProperties',component='ze_image',enabled=True,skipRun=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_image_desc_t*',tag='in'),
arg3=ArgDef(name='pImageProperties',type='ze_image_properties_t*',tag='out'),
)

# DEPRECATED
Function(name='zeImageViewCreateExp',component='ze_image_exp',enabled=True,stateTrack=True,api_version='1.2',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_image_desc_t*',tag='in'),
arg4=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
arg5=ArgDef(name='phImageView',type='ze_image_handle_t*',tag='out'),
)

Function(name='zeImageViewCreateExt',component='ze_image',enabled=True,api_version='1.5',stateTrack=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_image_desc_t*',tag='in'),
arg4=ArgDef(name='hImage',type='ze_image_handle_t',tag='in'),
arg5=ArgDef(name='phImageView',type='ze_image_handle_t*',tag='out'),
)

Function(name='zeInit',component='ze_global',enabled=True,recWrap=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='flags',type='ze_init_flags_t',tag='in'),
)

Function(name='zeInitDrivers',component='ze_global',enabled=True,api_version='1.10',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg2=ArgDef(name='phDrivers',type='ze_driver_handle_t*',tag='inout',range='0,*pCount',optional=True),
arg3=ArgDef(name='desc',type='ze_init_driver_type_desc_t*',tag='in'),
)

Function(name='zeKernelCreate',component='ze_kernel',enabled=True,stateTrack=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const ze_kernel_desc_t*',tag='in'),
arg3=ArgDef(name='phKernel',type='ze_kernel_handle_t*',tag='out'),
)

Function(name='zeKernelDestroy',component='ze_kernel',enabled=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in',release=True),
)

Function(name='zeKernelGetIndirectAccess',component='ze_kernel',enabled=True,skipRun=True,ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='pFlags',type='ze_kernel_indirect_access_flags_t*',tag='out'),
)

Function(name='zeKernelGetName',component='ze_kernel',enabled=True,ddi_pos=11,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='pSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pName',type='char*',tag='inout',optional=True),
)

Function(name='zeKernelGetProperties',component='ze_kernel',enabled=True,ddi_pos=10,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='pKernelProperties',type='ze_kernel_properties_t*',tag='inout'),
)

Function(name='zeKernelGetSourceAttributes',component='ze_kernel',enabled=True,skipRun=True,ddi_pos=9,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='pSize',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pString',type='char**',tag='inout',optional=True,wrapType='COutArgument'),
)

Function(name='zeKernelSchedulingHintExp',component='ze_kernel_exp',enabled=True,api_version='1.2',stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='pHint',type='ze_scheduling_hint_exp_desc_t*',tag='in'),
)

Function(name='zeKernelSetArgumentValue',component='ze_kernel',enabled=True,stateTrack=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='argIndex',type='uint32_t',tag='in'),
arg3=ArgDef(name='argSize',type='size_t',tag='in'),
arg4=ArgDef(name='pArgValue',type='const void*',tag='in',optional=True,wrapType='CKernelArgValue',wrapParams='argSize, {name}'),
)

Function(name='zeKernelSetCacheConfig',component='ze_kernel',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='flags',type='ze_cache_config_flags_t',tag='in'),
)

Function(name='zeKernelSetGlobalOffsetExp',component='ze_kernel_exp',enabled=True,stateTrack=True,api_version='1.1',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='offsetX',type='uint32_t',tag='in'),
arg3=ArgDef(name='offsetY',type='uint32_t',tag='in'),
arg4=ArgDef(name='offsetZ',type='uint32_t',tag='in'),
)

Function(name='zeKernelSetGroupSize',component='ze_kernel',enabled=True,stateTrack=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='groupSizeX',type='uint32_t',tag='in'),
arg3=ArgDef(name='groupSizeY',type='uint32_t',tag='in'),
arg4=ArgDef(name='groupSizeZ',type='uint32_t',tag='in'),
)

Function(name='zeKernelSetIndirectAccess',component='ze_kernel',enabled=True,stateTrack=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='flags',type='ze_kernel_indirect_access_flags_t',tag='in'),
)

Function(name='zeKernelSuggestGroupSize',component='ze_kernel',enabled=True,skipRun=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='globalSizeX',type='uint32_t',tag='in'),
arg3=ArgDef(name='globalSizeY',type='uint32_t',tag='in'),
arg4=ArgDef(name='globalSizeZ',type='uint32_t',tag='in'),
arg5=ArgDef(name='groupSizeX',type='uint32_t*',tag='out'),
arg6=ArgDef(name='groupSizeY',type='uint32_t*',tag='out'),
arg7=ArgDef(name='groupSizeZ',type='uint32_t*',tag='out'),
)

Function(name='zeKernelSuggestMaxCooperativeGroupCount',component='ze_kernel',enabled=True,skipRun=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='ze_kernel_handle_t',tag='in'),
arg2=ArgDef(name='totalGroupCount',type='uint32_t*',tag='out'),
)

Function(name='zeMemAllocDevice',component='ze_mem',enabled=True,stateTrack=True,recWrap=True,runWrap=True,ddi_pos=1,recExecWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='device_desc',type='const ze_device_mem_alloc_desc_t*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='alignment',type='size_t',tag='in'),
arg5=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg6=ArgDef(name='pptr',type='void**',tag='out',wrapType='CMappedPtr::CSMapArray'),
)

Function(name='zeMemAllocHost',component='ze_mem',enabled=True,stateTrack=True,recWrap=True,runWrap=True,ddi_pos=2,recExecWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='host_desc',type='const ze_host_mem_alloc_desc_t*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='alignment',type='size_t',tag='in'),
arg5=ArgDef(name='pptr',type='void**',tag='out',wrapType='CMappedPtr::CSMapArray'),
)

Function(name='zeMemAllocShared',component='ze_mem',enabled=True,stateTrack=True,recWrap=True,runWrap=True,ddi_pos=0,recExecWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='device_desc',type='const ze_device_mem_alloc_desc_t*',tag='in'),
arg3=ArgDef(name='host_desc',type='const ze_host_mem_alloc_desc_t*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='alignment',type='size_t',tag='in'),
arg6=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in',optional=True),
arg7=ArgDef(name='pptr',type='void**',tag='out',wrapType='CMappedPtr::CSMapArray'),
)

Function(name='zeMemCloseIpcHandle',component='ze_mem',enabled=False,ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in',release=True),
)

Function(name='zeMemFree',component='ze_mem',enabled=True,stateTrack=True,recExecWrap=True,ddi_pos=3,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='void*',tag='in',wrapType='CMappedPtr',release=True),
)

Function(name='zeMemFreeExt',component='ze_mem',enabled=True,api_version='1.3',stateTrack=True,recExecWrap=True,ddi_pos=9,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='pMemFreeDesc',type='const ze_memory_free_ext_desc_t*',tag='in'),
arg3=ArgDef(name='ptr',type='void*',tag='in',release=True),
)

Function(name='zeMemFreeExt',component='ze_mem',enabled=True,api_version='1.3',stateTrack=True,recExecWrap=True,ddi_pos=9,version=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='pMemFreeDesc',type='const ze_memory_free_ext_desc_t*',tag='in'),
arg3=ArgDef(name='ptr',type='void*',tag='in',wrapType='CMappedPtr',release=True),
)

Function(name='zeMemGetAddressRange',component='ze_mem',enabled=True,skipRun=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='pBase',type='void**',tag='inout',optional=True),
arg4=ArgDef(name='pSize',type='size_t*',tag='inout',optional=True),
)

Function(name='zeMemGetAllocProperties',component='ze_mem',enabled=True,skipRun=True,ddi_pos=4,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='pMemAllocProperties',type='ze_memory_allocation_properties_t*',tag='inout'),
arg4=ArgDef(name='phDevice',type='ze_device_handle_t*',tag='out',optional=True),
)

Function(name='zeMemGetAtomicAccessAttributeExp',component='ze_mem_exp',enabled=False,api_version='1.7',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='const void*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='pAttr',type='ze_memory_atomic_attr_exp_flags_t*',tag='out'),
)

Function(name='zeMemGetFileDescriptorFromIpcHandleExp',component='ze_mem_exp',enabled=False,api_version='1.6',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ipcHandle',type='ze_ipc_mem_handle_t',tag='in'),
arg3=ArgDef(name='pHandle',type='uint64_t*',tag='out'),
)

Function(name='zeMemGetIpcHandle',component='ze_mem',enabled=False,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='pIpcHandle',type='ze_ipc_mem_handle_t*',tag='out'),
)

Function(name='zeMemGetIpcHandleFromFileDescriptorExp',component='ze_mem_exp',enabled=False,api_version='1.6',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='handle',type='uint64_t',tag='in'),
arg3=ArgDef(name='pIpcHandle',type='ze_ipc_mem_handle_t*',tag='out'),
)

Function(name='zeMemGetPitchFor2dImage',component='ze_mem',enabled=False,api_version='1.9',ddi_pos=11,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='imageWidth',type='size_t',tag='in'),
arg4=ArgDef(name='imageHeight',type='size_t',tag='in'),
arg5=ArgDef(name='elementSizeInBytes',type='unsigned int',tag='in'),
arg6=ArgDef(name='rowPitch',type='size_t *',tag='out'),
)

Function(name='zeMemOpenIpcHandle',component='ze_mem',enabled=False,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='handle',type='ze_ipc_mem_handle_t',tag='in'),
arg4=ArgDef(name='flags',type='ze_ipc_memory_flags_t',tag='in'),
arg5=ArgDef(name='pptr',type='void**',tag='out'),
)

Function(name='zeMemPutIpcHandle',component='ze_mem',enabled=False,api_version='1.6',ddi_pos=10,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='handle',type='ze_ipc_mem_handle_t',tag='in'),
)

Function(name='zeMemSetAtomicAccessAttributeExp',component='ze_mem_exp',enabled=False,api_version='1.7',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='ptr',type='const void*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='attr',type='ze_memory_atomic_attr_exp_flags_t',tag='in'),
)

Function(name='zeModuleBuildLogDestroy',component='ze_module_build_log',enabled=True,ddi_pos=0,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModuleBuildLog',type='ze_module_build_log_handle_t',tag='in',release=True),
)

Function(name='zeModuleBuildLogGetString',component='ze_module_build_log',enabled=True,ddi_pos=1,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModuleBuildLog',type='ze_module_build_log_handle_t',tag='in'),
arg2=ArgDef(name='pSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pBuildLog',type='char*',tag='inout',optional=True),
)

Function(name='zeModuleCreate',component='ze_module',enabled=True,stateTrack=True,runWrap=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_module_desc_t*',tag='in'),
arg4=ArgDef(name='phModule',type='ze_module_handle_t*',tag='out'),
arg5=ArgDef(name='phBuildLog',type='ze_module_build_log_handle_t*',tag='out',optional=True),
)

Function(name='zeModuleCreate',component='ze_module',enabled=True,stateTrack=True,runWrap=True,recWrap=True,version=1,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_module_desc_t*',tag='in',wrapType="Cze_module_desc_t_V1::CSArray"),
arg4=ArgDef(name='phModule',type='ze_module_handle_t*',tag='out'),
arg5=ArgDef(name='phBuildLog',type='ze_module_build_log_handle_t*',tag='out',optional=True),
)

Function(name='zeModuleDestroy',component='ze_module',enabled=True,stateTrack=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in',release=True),
)

Function(name='zeModuleDynamicLink',component='ze_module',enabled=True,stateTrack=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='numModules',type='uint32_t',tag='in'),
arg2=ArgDef(name='phModules',type='ze_module_handle_t*',tag='in',range='0,numModules'),
arg3=ArgDef(name='phLinkLog',type='ze_module_build_log_handle_t*',tag='out',optional=True),
)

Function(name='zeModuleGetFunctionPointer',component='ze_module',enabled=True,skipRun=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='pFunctionName',type='const char*',tag='in'),
arg3=ArgDef(name='pfnFunction',type='void**',tag='out'),
)

Function(name='zeModuleGetFunctionPointer',component='ze_module',enabled=True,version=1,stateTrack=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='pFunctionName',type='const char*',tag='in'),
arg3=ArgDef(name='pfnFunction',type='void**',tag='out',wrapType='CMappedPtr::CSMapArray'),
)

Function(name='zeModuleGetGlobalPointer',component='ze_module',enabled=True,stateTrack=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='pGlobalName',type='const char*',tag='in'),
arg3=ArgDef(name='pSize',type='size_t*',tag='inout',optional=True),
arg4=ArgDef(name='pptr',type='void**',tag='inout',optional=True,wrapType='CMappedPtr::CSMapArray'),
)

Function(name='zeModuleGetKernelNames',component='ze_module',enabled=True,recWrap=True,skipRun=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pNames',type='const char**',tag='inout',range='0,*pCount',optional=True,wrapType='COutArgument'),
)

Function(name='zeModuleGetNativeBinary',component='ze_module',enabled=True,ddi_pos=3,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='pSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pModuleNativeBinary',type='uint8_t*',tag='inout',optional=True),
)

Function(name='zeModuleGetProperties',component='ze_module',enabled=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='ze_module_handle_t',tag='in'),
arg2=ArgDef(name='pModuleProperties',type='ze_module_properties_t*',tag='inout'),
)

Function(name='zeModuleInspectLinkageExt',component='ze_module',enabled=True,api_version='1.3',ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='pInspectDesc',type='ze_linkage_inspection_ext_desc_t*',tag='in'),
arg2=ArgDef(name='numModules',type='uint32_t',tag='in'),
arg3=ArgDef(name='phModules',type='ze_module_handle_t*',tag='in',range='0,numModules'),
arg4=ArgDef(name='phLog',type='ze_module_build_log_handle_t*',tag='out'),
)

Function(name='zePhysicalMemCreate',component='ze_physical_mem',enabled=True,ddi_pos=0,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='ze_physical_mem_desc_t*',tag='in'),
arg4=ArgDef(name='phPhysicalMemory',type='ze_physical_mem_handle_t*',tag='out'),
)

Function(name='zePhysicalMemDestroy',component='ze_physical_mem',enabled=True,ddi_pos=1,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hPhysicalMemory',type='ze_physical_mem_handle_t',tag='in',release=True),
)

Function(name='zeRTASBuilderBuildExp',component='ze_rtas_builder_exp',enabled=False,api_version='1.7',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hBuilder',type='ze_rtas_builder_exp_handle_t',tag='in'),
arg2=ArgDef(name='pBuildOpDescriptor',type='const ze_rtas_builder_build_op_exp_desc_t*',tag='in',wrapType='COutArgument'),
arg3=ArgDef(name='pScratchBuffer',type='void*',tag='in'),
arg4=ArgDef(name='scratchBufferSizeBytes',type='size_t',tag='in'),
arg5=ArgDef(name='pRtasBuffer',type='void*',tag='in'),
arg6=ArgDef(name='rtasBufferSizeBytes',type='size_t',tag='in'),
arg7=ArgDef(name='hParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t',tag='in',optional=True),
arg8=ArgDef(name='pBuildUserPtr',type='void*',tag='in',optional=True),
arg9=ArgDef(name='pBounds',type='ze_rtas_aabb_exp_t*',tag='inout',optional=True),
arg10=ArgDef(name='pRtasBufferSizeBytes',type='size_t*',tag='out',optional=True),
)

Function(name='zeRTASBuilderCreateExp',component='ze_rtas_builder_exp',enabled=False,api_version='1.7',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='pDescriptor',type='const ze_rtas_builder_exp_desc_t*',tag='in'),
arg3=ArgDef(name='phBuilder',type='ze_rtas_builder_exp_handle_t*',tag='out'),
)

Function(name='zeRTASBuilderDestroyExp',component='ze_rtas_builder_exp',enabled=False,api_version='1.7',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hBuilder',type='ze_rtas_builder_exp_handle_t',tag='in',release=True),
)

Function(name='zeRTASBuilderGetBuildPropertiesExp',component='ze_rtas_builder_exp',enabled=False,api_version='1.7',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hBuilder',type='ze_rtas_builder_exp_handle_t',tag='in'),
arg2=ArgDef(name='pBuildOpDescriptor',type='const ze_rtas_builder_build_op_exp_desc_t*',tag='in',wrapType='COutArgument'),
arg3=ArgDef(name='pProperties',type='ze_rtas_builder_exp_properties_t*',tag='inout'),
)

Function(name='zeRTASParallelOperationCreateExp',component='ze_rtas_parallel_operation_exp',enabled=False,api_version='1.7',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='phParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t*',tag='out'),
)

Function(name='zeRTASParallelOperationDestroyExp',component='ze_rtas_parallel_operation_exp',enabled=False,api_version='1.7',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t',tag='in',release=True),
)

Function(name='zeRTASParallelOperationGetPropertiesExp',component='ze_rtas_parallel_operation_exp',enabled=False,api_version='1.7',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='ze_rtas_parallel_operation_exp_properties_t*',tag='inout'),
)

Function(name='zeRTASParallelOperationJoinExp',component='ze_rtas_parallel_operation_exp',enabled=False,api_version='1.7',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hParallelOperation',type='ze_rtas_parallel_operation_exp_handle_t',tag='in'),
)

Function(name='zeSamplerCreate',component='ze_sampler',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='desc',type='const ze_sampler_desc_t*',tag='in'),
arg4=ArgDef(name='phSampler',type='ze_sampler_handle_t*',tag='out'),
)

Function(name='zeSamplerDestroy',component='ze_sampler',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hSampler',type='ze_sampler_handle_t',tag='in',release=True),
)

Function(name='zeVirtualMemFree',component='ze_virtual_mem',enabled=True,ddi_pos=1,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeVirtualMemFree',component='ze_virtual_mem',enabled=True,ddi_pos=1,stateTrack=True,version=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in',wrapType='CUSMPtr'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeVirtualMemGetAccessAttribute',component='ze_virtual_mem',enabled=True,skipRun=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='access',type='ze_memory_access_attribute_t*',tag='out'),
arg5=ArgDef(name='outSize',type='size_t*',tag='out'),
)

Function(name='zeVirtualMemMap',component='ze_virtual_mem',enabled=True,ddi_pos=3,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='hPhysicalMemory',type='ze_physical_mem_handle_t',tag='in'),
arg5=ArgDef(name='offset',type='size_t',tag='in'),
arg6=ArgDef(name='access',type='ze_memory_access_attribute_t',tag='in'),
)

Function(name='zeVirtualMemMap',component='ze_virtual_mem',enabled=True,ddi_pos=3,stateTrack=True,version=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in',wrapType='CUSMPtr'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='hPhysicalMemory',type='ze_physical_mem_handle_t',tag='in'),
arg5=ArgDef(name='offset',type='size_t',tag='in'),
arg6=ArgDef(name='access',type='ze_memory_access_attribute_t',tag='in'),
)

Function(name='zeVirtualMemQueryPageSize',component='ze_virtual_mem',enabled=True,skipRun=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='pagesize',type='size_t*',tag='out'),
)

Function(name='zeVirtualMemReserve',component='ze_virtual_mem',enabled=True,skipRun=True,ddi_pos=0,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='pStart',type='const void*',tag='in',optional=True),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='pptr',type='void**',tag='out'),
)

Function(name='zeVirtualMemReserve',component='ze_virtual_mem',enabled=True,ddi_pos=0,stateTrack=True,version=1,runWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='pStart',type='const void*',tag='in',optional=True,wrapType='Cuintptr_t'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='pptr',type='void**',tag='out',wrapType='CMappedPtr::CSMapArray'),
)

Function(name='zeVirtualMemSetAccessAttribute',component='ze_virtual_mem',enabled=True,ddi_pos=5,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='access',type='ze_memory_access_attribute_t',tag='in'),
)

Function(name='zeVirtualMemSetAccessAttribute',component='ze_virtual_mem',enabled=True,ddi_pos=5,stateTrack=True,version=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in',wrapType='CUSMPtr'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
arg4=ArgDef(name='access',type='ze_memory_access_attribute_t',tag='in'),
)

Function(name='zeVirtualMemUnmap',component='ze_virtual_mem',enabled=True,ddi_pos=4,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zeVirtualMemUnmap',component='ze_virtual_mem',enabled=True,ddi_pos=4,stateTrack=True,version=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='ze_context_handle_t',tag='in'),
arg2=ArgDef(name='ptr',type='const void*',tag='in',wrapType='CUSMPtr'),
arg3=ArgDef(name='size',type='size_t',tag='in'),
)

Function(name='zesDeviceEccAvailable',component='zes_device',enabled=False,api_version='1.4',ddi_pos=25,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pAvailable',type='ze_bool_t*',tag='out'),
)

Function(name='zesDeviceEccConfigurable',component='zes_device',enabled=False,api_version='1.4',ddi_pos=26,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pConfigurable',type='ze_bool_t*',tag='out'),
)

Function(name='zesDeviceEnumActiveVFExp',component='zes_device_exp',enabled=True,api_version='1.9',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phVFhandle',type='zes_vf_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumDiagnosticTestSuites',component='zes_device',enabled=True,ddi_pos=8,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phDiagnostics',type='zes_diag_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumEnabledVFExp',component='zes_device_exp',enabled=False,api_version='1.10',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phVFhandle',type='zes_vf_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumEngineGroups',component='zes_device',enabled=True,ddi_pos=9,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phEngine',type='zes_engine_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumFabricPorts',component='zes_device',enabled=True,ddi_pos=11,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phPort',type='zes_fabric_port_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumFans',component='zes_device',enabled=True,ddi_pos=12,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phFan',type='zes_fan_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumFirmwares',component='zes_device',enabled=True,ddi_pos=13,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phFirmware',type='zes_firmware_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumFrequencyDomains',component='zes_device',enabled=True,ddi_pos=14,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phFrequency',type='zes_freq_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumLeds',component='zes_device',enabled=True,ddi_pos=15,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phLed',type='zes_led_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumMemoryModules',component='zes_device',enabled=True,ddi_pos=16,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phMemory',type='zes_mem_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumOverclockDomains',component='zes_device',enabled=True,api_version='1.5',ddi_pos=35,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phDomainHandle',type='zes_overclock_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumPerformanceFactorDomains',component='zes_device',enabled=True,ddi_pos=17,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phPerf',type='zes_perf_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumPowerDomains',component='zes_device',enabled=True,ddi_pos=18,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phPower',type='zes_pwr_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumPsus',component='zes_device',enabled=True,ddi_pos=20,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phPsu',type='zes_psu_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumRasErrorSets',component='zes_device',enabled=True,ddi_pos=21,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phRas',type='zes_ras_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumSchedulers',component='zes_device',enabled=True,ddi_pos=22,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phScheduler',type='zes_sched_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumStandbyDomains',component='zes_device',enabled=True,ddi_pos=23,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phStandby',type='zes_standby_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEnumTemperatureSensors',component='zes_device',enabled=True,ddi_pos=24,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phTemperature',type='zes_temp_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceEventRegister',component='zes_device',enabled=True,ddi_pos=10,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='events',type='zes_event_type_flags_t',tag='in'),
)

Function(name='zesDeviceGet',component='zes_device',enabled=True,api_version='1.5',ddi_pos=29,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='zes_driver_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phDevices',type='zes_device_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceGetCardPowerDomain',component='zes_device',enabled=True,ddi_pos=19,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='phPower',type='zes_pwr_handle_t*',tag='inout'),
)

Function(name='zesDeviceGetEccState',component='zes_device',enabled=False,api_version='1.4',ddi_pos=27,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_device_ecc_properties_t*',tag='out'),
)

Function(name='zesDeviceGetOverclockControls',component='zes_device',enabled=False,api_version='1.5',ddi_pos=32,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='domainType',type='zes_overclock_domain_t',tag='in'),
arg3=ArgDef(name='pAvailableControls',type='uint32_t*',tag='inout'),
)

Function(name='zesDeviceGetOverclockDomains',component='zes_device',enabled=False,api_version='1.5',ddi_pos=31,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pOverclockDomains',type='uint32_t*',tag='inout'),
)

Function(name='zesDeviceGetProperties',component='zes_device',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_device_properties_t*',tag='inout'),
)

Function(name='zesDeviceGetState',component='zes_device',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_device_state_t*',tag='inout'),
)

Function(name='zesDeviceGetSubDevicePropertiesExp',component='zes_device_exp',enabled=True,api_version='1.9',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pSubdeviceProps',type='zes_subdevice_exp_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDevicePciGetBars',component='zes_device',enabled=True,ddi_pos=6,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pProperties',type='zes_pci_bar_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDevicePciGetProperties',component='zes_device',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_pci_properties_t*',tag='inout'),
)

Function(name='zesDevicePciGetState',component='zes_device',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_pci_state_t*',tag='inout'),
)

Function(name='zesDevicePciGetStats',component='zes_device',enabled=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pStats',type='zes_pci_stats_t*',tag='inout'),
)

Function(name='zesDeviceProcessesGetState',component='zes_device',enabled=True,ddi_pos=3,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pProcesses',type='zes_process_state_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDeviceReadOverclockState',component='zes_device',enabled=False,api_version='1.5',ddi_pos=34,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pOverclockMode',type='zes_overclock_mode_t*',tag='out'),
arg3=ArgDef(name='pWaiverSetting',type='ze_bool_t*',tag='out'),
arg4=ArgDef(name='pOverclockState',type='ze_bool_t*',tag='out'),
arg5=ArgDef(name='pPendingAction',type='zes_pending_action_t*',tag='out'),
arg6=ArgDef(name='pPendingReset',type='ze_bool_t*',tag='out'),
)

Function(name='zesDeviceReset',component='zes_device',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='force',type='ze_bool_t',tag='in'),
)

Function(name='zesDeviceResetExt',component='zes_device',enabled=False,api_version='1.7',ddi_pos=36,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_reset_properties_t*',tag='in'),
)

Function(name='zesDeviceResetOverclockSettings',component='zes_device',enabled=True,api_version='1.5',ddi_pos=33,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='onShippedState',type='ze_bool_t',tag='in'),
)

Function(name='zesDeviceSetEccState',component='zes_device',enabled=False,api_version='1.4',ddi_pos=28,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='newState',type='const zes_device_ecc_desc_t*',tag='in'),
arg3=ArgDef(name='pState',type='zes_device_ecc_properties_t*',tag='out'),
)

Function(name='zesDeviceSetOverclockWaiver',component='zes_device',enabled=True,api_version='1.5',ddi_pos=30,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
)

Function(name='zesDiagnosticsGetProperties',component='zes_diagnostics',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDiagnostics',type='zes_diag_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_diag_properties_t*',tag='inout'),
)

Function(name='zesDiagnosticsGetTests',component='zes_diagnostics',enabled=True,ddi_pos=1,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDiagnostics',type='zes_diag_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pTests',type='zes_diag_test_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDiagnosticsRunTests',component='zes_diagnostics',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDiagnostics',type='zes_diag_handle_t',tag='in'),
arg2=ArgDef(name='startIndex',type='uint32_t',tag='in'),
arg3=ArgDef(name='endIndex',type='uint32_t',tag='in'),
arg4=ArgDef(name='pResult',type='zes_diag_result_t*',tag='inout'),
)

Function(name='zesDriverEventListen',component='zes_driver',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint32_t',tag='in'),
arg3=ArgDef(name='count',type='uint32_t',tag='in'),
arg4=ArgDef(name='phDevices',type='zes_device_handle_t*',tag='in',range='0,count'),
arg5=ArgDef(name='pNumDeviceEvents',type='uint32_t*',tag='inout'),
arg6=ArgDef(name='pEvents',type='zes_event_type_flags_t*',tag='inout'),
)

Function(name='zesDriverEventListenEx',component='zes_driver',enabled=True,api_version='1.1',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint64_t',tag='in'),
arg3=ArgDef(name='count',type='uint32_t',tag='in'),
arg4=ArgDef(name='phDevices',type='zes_device_handle_t*',tag='in',range='0,count'),
arg5=ArgDef(name='pNumDeviceEvents',type='uint32_t*',tag='inout'),
arg6=ArgDef(name='pEvents',type='zes_event_type_flags_t*',tag='inout'),
)

Function(name='zesDriverGet',component='zes_driver',enabled=True,api_version='1.5',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg2=ArgDef(name='phDrivers',type='zes_driver_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesDriverGetDeviceByUuidExp',component='zes_driver_exp',enabled=False,api_version='1.9',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='zes_driver_handle_t',tag='in'),
arg2=ArgDef(name='uuid',type='zes_uuid_t',tag='in'),
arg3=ArgDef(name='phDevice',type='zes_device_handle_t*',tag='out'),
arg4=ArgDef(name='onSubdevice',type='ze_bool_t*',tag='out'),
arg5=ArgDef(name='subdeviceId',type='uint32_t*',tag='out'),
)

Function(name='zesDriverGetExtensionFunctionAddress',component='zes_driver',enabled=True,api_version='1.8',ddi_pos=4,recExecWrap=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='zes_driver_handle_t',tag='in'),
arg2=ArgDef(name='name',type='const char*',tag='in'),
arg3=ArgDef(name='ppFunctionAddress',type='void**',tag='out'),
)

Function(name='zesDriverGetExtensionProperties',component='zes_driver',enabled=True,api_version='1.8',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='zes_driver_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pExtensionProperties',type='zes_driver_extension_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesEngineGetActivity',component='zes_engine',enabled=True,api_version='1.7',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEngine',type='zes_engine_handle_t',tag='in'),
arg2=ArgDef(name='pStats',type='zes_engine_stats_t*',tag='inout'),
)

Function(name='zesEngineGetActivityExt',component='zes_engine',enabled=True,api_version='1.7',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEngine',type='zes_engine_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pStats',type='zes_engine_stats_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesEngineGetProperties',component='zes_engine',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hEngine',type='zes_engine_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_engine_properties_t*',tag='inout'),
)

Function(name='zesFabricPortGetConfig',component='zes_fabric_port',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='zes_fabric_port_config_t*',tag='inout'),
)

Function(name='zesFabricPortGetFabricErrorCounters',component='zes_fabric_port',enabled=True,api_version='1.7',ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pErrors',type='zes_fabric_port_error_counters_t*',tag='inout'),
)

Function(name='zesFabricPortGetLinkType',component='zes_fabric_port',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pLinkType',type='zes_fabric_link_type_t*',tag='inout'),
)

Function(name='zesFabricPortGetMultiPortThroughput',component='zes_fabric_port',enabled=False,api_version='1.7',ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zes_device_handle_t',tag='in'),
arg2=ArgDef(name='numPorts',type='uint32_t',tag='in'),
arg3=ArgDef(name='phPort',type='zes_fabric_port_handle_t*',tag='in',range='0,numPorts'),
arg4=ArgDef(name='pThroughput',type='zes_fabric_port_throughput_t**',tag='out',range='0,numPorts',wrapType='COutArgument'),
)

Function(name='zesFabricPortGetProperties',component='zes_fabric_port',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_fabric_port_properties_t*',tag='inout'),
)

Function(name='zesFabricPortGetState',component='zes_fabric_port',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_fabric_port_state_t*',tag='inout'),
)

Function(name='zesFabricPortGetThroughput',component='zes_fabric_port',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pThroughput',type='zes_fabric_port_throughput_t*',tag='inout'),
)

Function(name='zesFabricPortSetConfig',component='zes_fabric_port',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPort',type='zes_fabric_port_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='const zes_fabric_port_config_t*',tag='in'),
)

Function(name='zesFanGetConfig',component='zes_fan',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFan',type='zes_fan_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='zes_fan_config_t*',tag='inout'),
)

Function(name='zesFanGetProperties',component='zes_fan',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFan',type='zes_fan_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_fan_properties_t*',tag='inout'),
)

Function(name='zesFanGetState',component='zes_fan',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFan',type='zes_fan_handle_t',tag='in'),
arg2=ArgDef(name='units',type='zes_fan_speed_units_t',tag='in'),
arg3=ArgDef(name='pSpeed',type='int32_t*',tag='inout'),
)

Function(name='zesFanSetDefaultMode',component='zes_fan',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFan',type='zes_fan_handle_t',tag='in'),
)

Function(name='zesFanSetFixedSpeedMode',component='zes_fan',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFan',type='zes_fan_handle_t',tag='in'),
arg2=ArgDef(name='speed',type='const zes_fan_speed_t*',tag='in'),
)

Function(name='zesFanSetSpeedTableMode',component='zes_fan',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFan',type='zes_fan_handle_t',tag='in'),
arg2=ArgDef(name='speedTable',type='const zes_fan_speed_table_t*',tag='in'),
)

Function(name='zesFirmwareFlash',component='zes_firmware',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFirmware',type='zes_firmware_handle_t',tag='in'),
arg2=ArgDef(name='pImage',type='void*',tag='in'),
arg3=ArgDef(name='size',type='uint32_t',tag='in'),
)

Function(name='zesFirmwareGetConsoleLogs',component='zes_firmware',enabled=False,api_version='1.9',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFirmware',type='zes_firmware_handle_t',tag='in'),
arg2=ArgDef(name='pSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pFirmwareLog',type='char*',tag='inout',optional=True),
)

Function(name='zesFirmwareGetFlashProgress',component='zes_firmware',enabled=True,api_version='1.8',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFirmware',type='zes_firmware_handle_t',tag='in'),
arg2=ArgDef(name='pCompletionPercent',type='uint32_t*',tag='inout'),
)

Function(name='zesFirmwareGetProperties',component='zes_firmware',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFirmware',type='zes_firmware_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_firmware_properties_t*',tag='inout'),
)

Function(name='zesFirmwareGetSecurityVersionExp',component='zes_firmware_exp',enabled=False,api_version='1.9',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFirmware',type='zes_firmware_handle_t',tag='in'),
arg2=ArgDef(name='pVersion',type='char*',tag='inout'),
)

Function(name='zesFirmwareSetSecurityVersionExp',component='zes_firmware_exp',enabled=False,api_version='1.9',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFirmware',type='zes_firmware_handle_t',tag='in'),
)

Function(name='zesFrequencyGetAvailableClocks',component='zes_frequency',enabled=True,ddi_pos=1,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phFrequency',type='double*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesFrequencyGetProperties',component='zes_frequency',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_freq_properties_t*',tag='inout'),
)

Function(name='zesFrequencyGetRange',component='zes_frequency',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pLimits',type='zes_freq_range_t*',tag='inout'),
)

Function(name='zesFrequencyGetState',component='zes_frequency',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_freq_state_t*',tag='inout'),
)

Function(name='zesFrequencyGetThrottleTime',component='zes_frequency',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pThrottleTime',type='zes_freq_throttle_time_t*',tag='inout'),
)

Function(name='zesFrequencyOcGetCapabilities',component='zes_frequency',enabled=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pOcCapabilities',type='zes_oc_capabilities_t*',tag='inout'),
)

Function(name='zesFrequencyOcGetFrequencyTarget',component='zes_frequency',enabled=True,ddi_pos=7,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pCurrentOcFrequency',type='double*',tag='out'),
)

Function(name='zesFrequencyOcGetIccMax',component='zes_frequency',enabled=True,ddi_pos=13,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pOcIccMax',type='double*',tag='inout'),
)

Function(name='zesFrequencyOcGetMode',component='zes_frequency',enabled=True,ddi_pos=12,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pCurrentOcMode',type='zes_oc_mode_t*',tag='out'),
)

Function(name='zesFrequencyOcGetTjMax',component='zes_frequency',enabled=True,ddi_pos=15,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pOcTjMax',type='double*',tag='inout'),
)

Function(name='zesFrequencyOcGetVoltageTarget',component='zes_frequency',enabled=True,ddi_pos=9,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pCurrentVoltageTarget',type='double*',tag='out'),
arg3=ArgDef(name='pCurrentVoltageOffset',type='double*',tag='out'),
)

Function(name='zesFrequencyOcSetFrequencyTarget',component='zes_frequency',enabled=True,ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='CurrentOcFrequency',type='double',tag='in'),
)

Function(name='zesFrequencyOcSetIccMax',component='zes_frequency',enabled=True,ddi_pos=14,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='ocIccMax',type='double',tag='in'),
)

Function(name='zesFrequencyOcSetMode',component='zes_frequency',enabled=True,ddi_pos=11,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='CurrentOcMode',type='zes_oc_mode_t',tag='in'),
)

Function(name='zesFrequencyOcSetTjMax',component='zes_frequency',enabled=True,ddi_pos=16,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='ocTjMax',type='double',tag='in'),
)

Function(name='zesFrequencyOcSetVoltageTarget',component='zes_frequency',enabled=True,ddi_pos=10,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='CurrentVoltageTarget',type='double',tag='in'),
arg3=ArgDef(name='CurrentVoltageOffset',type='double',tag='in'),
)

Function(name='zesFrequencySetRange',component='zes_frequency',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hFrequency',type='zes_freq_handle_t',tag='in'),
arg2=ArgDef(name='pLimits',type='const zes_freq_range_t*',tag='in'),
)

Function(name='zesInit',component='zes_global',enabled=True,recWrap=True,api_version='1.5',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='flags',type='zes_init_flags_t',tag='in'),
)

Function(name='zesLedGetProperties',component='zes_led',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hLed',type='zes_led_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_led_properties_t*',tag='inout'),
)

Function(name='zesLedGetState',component='zes_led',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hLed',type='zes_led_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_led_state_t*',tag='inout'),
)

Function(name='zesLedSetColor',component='zes_led',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hLed',type='zes_led_handle_t',tag='in'),
arg2=ArgDef(name='pColor',type='const zes_led_color_t*',tag='in'),
)

Function(name='zesLedSetState',component='zes_led',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hLed',type='zes_led_handle_t',tag='in'),
arg2=ArgDef(name='enable',type='ze_bool_t',tag='in'),
)

Function(name='zesMemoryGetBandwidth',component='zes_memory',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMemory',type='zes_mem_handle_t',tag='in'),
arg2=ArgDef(name='pBandwidth',type='zes_mem_bandwidth_t*',tag='inout'),
)

Function(name='zesMemoryGetProperties',component='zes_memory',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMemory',type='zes_mem_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_mem_properties_t*',tag='inout'),
)

Function(name='zesMemoryGetState',component='zes_memory',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMemory',type='zes_mem_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_mem_state_t*',tag='inout'),
)

Function(name='zesOverclockGetControlCurrentValue',component='zes_overclock',enabled=True,api_version='1.5',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='DomainControl',type='zes_overclock_control_t',tag='in'),
arg3=ArgDef(name='pValue',type='double*',tag='inout'),
)

Function(name='zesOverclockGetControlPendingValue',component='zes_overclock',enabled=False,api_version='1.5',ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='DomainControl',type='zes_overclock_control_t',tag='in'),
arg3=ArgDef(name='pValue',type='double*',tag='out'),
)

Function(name='zesOverclockGetControlState',component='zes_overclock',enabled=False,api_version='1.5',ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='DomainControl',type='zes_overclock_control_t',tag='in'),
arg3=ArgDef(name='pControlState',type='zes_control_state_t*',tag='out'),
arg4=ArgDef(name='pPendingAction',type='zes_pending_action_t*',tag='out'),
)

Function(name='zesOverclockGetDomainControlProperties',component='zes_overclock',enabled=True,api_version='1.5',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='DomainControl',type='zes_overclock_control_t',tag='in'),
arg3=ArgDef(name='pControlProperties',type='zes_control_property_t*',tag='inout'),
)

Function(name='zesOverclockGetDomainProperties',component='zes_overclock',enabled=True,api_version='1.5',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='pDomainProperties',type='zes_overclock_properties_t*',tag='inout'),
)

Function(name='zesOverclockGetDomainVFProperties',component='zes_overclock',enabled=True,api_version='1.5',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='pVFProperties',type='zes_vf_property_t*',tag='inout'),
)

Function(name='zesOverclockGetVFPointValues',component='zes_overclock',enabled=False,api_version='1.5',ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='VFType',type='zes_vf_type_t',tag='in'),
arg3=ArgDef(name='VFArrayType',type='zes_vf_array_type_t',tag='in'),
arg4=ArgDef(name='PointIndex',type='uint32_t',tag='in'),
arg5=ArgDef(name='PointValue',type='uint32_t*',tag='out'),
)

Function(name='zesOverclockSetControlUserValue',component='zes_overclock',enabled=False,api_version='1.5',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='DomainControl',type='zes_overclock_control_t',tag='in'),
arg3=ArgDef(name='pValue',type='double',tag='in'),
arg4=ArgDef(name='pPendingAction',type='zes_pending_action_t*',tag='out'),
)

Function(name='zesOverclockSetVFPointValues',component='zes_overclock',enabled=True,api_version='1.5',ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDomainHandle',type='zes_overclock_handle_t',tag='in'),
arg2=ArgDef(name='VFType',type='zes_vf_type_t',tag='in'),
arg3=ArgDef(name='PointIndex',type='uint32_t',tag='in'),
arg4=ArgDef(name='PointValue',type='uint32_t',tag='in'),
)

Function(name='zesPerformanceFactorGetConfig',component='zes_performance_factor',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPerf',type='zes_perf_handle_t',tag='in'),
arg2=ArgDef(name='pFactor',type='double*',tag='inout'),
)

Function(name='zesPerformanceFactorGetProperties',component='zes_performance_factor',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPerf',type='zes_perf_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_perf_properties_t*',tag='inout'),
)

Function(name='zesPerformanceFactorSetConfig',component='zes_performance_factor',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPerf',type='zes_perf_handle_t',tag='in'),
arg2=ArgDef(name='factor',type='double',tag='in'),
)

Function(name='zesPowerGetEnergyCounter',component='zes_power',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pEnergy',type='zes_power_energy_counter_t*',tag='inout'),
)

Function(name='zesPowerGetEnergyThreshold',component='zes_power',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pThreshold',type='zes_energy_threshold_t*',tag='inout'),
)

Function(name='zesPowerGetLimits',component='zes_power',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pSustained',type='zes_power_sustained_limit_t*',tag='inout',optional=True),
arg3=ArgDef(name='pBurst',type='zes_power_burst_limit_t*',tag='inout',optional=True),
arg4=ArgDef(name='pPeak',type='zes_power_peak_limit_t*',tag='inout',optional=True),
)

Function(name='zesPowerGetLimitsExt',component='zes_power',enabled=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pSustained',type='zes_power_limit_ext_desc_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesPowerGetProperties',component='zes_power',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_power_properties_t*',tag='inout'),
)

Function(name='zesPowerSetEnergyThreshold',component='zes_power',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='threshold',type='double',tag='in'),
)

Function(name='zesPowerSetLimits',component='zes_power',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pSustained',type='const zes_power_sustained_limit_t*',tag='in',optional=True),
arg3=ArgDef(name='pBurst',type='const zes_power_burst_limit_t*',tag='in',optional=True),
arg4=ArgDef(name='pPeak',type='const zes_power_peak_limit_t*',tag='in',optional=True),
)

Function(name='zesPowerSetLimitsExt',component='zes_power',enabled=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPower',type='zes_pwr_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='in'),
arg3=ArgDef(name='pSustained',type='zes_power_limit_ext_desc_t*',tag='in',range='0,*pCount',optional=True),
)

Function(name='zesPsuGetProperties',component='zes_psu',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPsu',type='zes_psu_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_psu_properties_t*',tag='inout'),
)

Function(name='zesPsuGetState',component='zes_psu',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hPsu',type='zes_psu_handle_t',tag='in'),
arg2=ArgDef(name='pState',type='zes_psu_state_t*',tag='inout'),
)

Function(name='zesRasClearStateExp',component='zes_ras_exp',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hRas',type='zes_ras_handle_t',tag='in'),
arg2=ArgDef(name='category',type='zes_ras_error_category_exp_t',tag='in'),
)

Function(name='zesRasGetConfig',component='zes_ras',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hRas',type='zes_ras_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='zes_ras_config_t*',tag='inout'),
)

Function(name='zesRasGetProperties',component='zes_ras',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hRas',type='zes_ras_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_ras_properties_t*',tag='inout'),
)

Function(name='zesRasGetState',component='zes_ras',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hRas',type='zes_ras_handle_t',tag='in'),
arg2=ArgDef(name='clear',type='ze_bool_t',tag='in'),
arg3=ArgDef(name='pState',type='zes_ras_state_t*',tag='inout'),
)

Function(name='zesRasGetStateExp',component='zes_ras_exp',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hRas',type='zes_ras_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pState',type='zes_ras_state_exp_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesRasSetConfig',component='zes_ras',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hRas',type='zes_ras_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='const zes_ras_config_t*',tag='in'),
)

Function(name='zesSchedulerGetCurrentMode',component='zes_scheduler',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='pMode',type='zes_sched_mode_t*',tag='inout'),
)

Function(name='zesSchedulerGetProperties',component='zes_scheduler',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_sched_properties_t*',tag='inout'),
)

Function(name='zesSchedulerGetTimeoutModeProperties',component='zes_scheduler',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='getDefaults',type='ze_bool_t',tag='in'),
arg3=ArgDef(name='pConfig',type='zes_sched_timeout_properties_t*',tag='inout'),
)

Function(name='zesSchedulerGetTimesliceModeProperties',component='zes_scheduler',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='getDefaults',type='ze_bool_t',tag='in'),
arg3=ArgDef(name='pConfig',type='zes_sched_timeslice_properties_t*',tag='inout'),
)

Function(name='zesSchedulerSetComputeUnitDebugMode',component='zes_scheduler',enabled=True,ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='pNeedReload',type='ze_bool_t*',tag='inout'),
)

Function(name='zesSchedulerSetExclusiveMode',component='zes_scheduler',enabled=True,ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='pNeedReload',type='ze_bool_t*',tag='inout'),
)

Function(name='zesSchedulerSetTimeoutMode',component='zes_scheduler',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_sched_timeout_properties_t*',tag='in'),
arg3=ArgDef(name='pNeedReload',type='ze_bool_t*',tag='inout'),
)

Function(name='zesSchedulerSetTimesliceMode',component='zes_scheduler',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hScheduler',type='zes_sched_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_sched_timeslice_properties_t*',tag='in'),
arg3=ArgDef(name='pNeedReload',type='ze_bool_t*',tag='inout'),
)

Function(name='zesStandbyGetMode',component='zes_standby',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hStandby',type='zes_standby_handle_t',tag='in'),
arg2=ArgDef(name='pMode',type='zes_standby_promo_mode_t*',tag='inout'),
)

Function(name='zesStandbyGetProperties',component='zes_standby',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hStandby',type='zes_standby_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_standby_properties_t*',tag='inout'),
)

Function(name='zesStandbySetMode',component='zes_standby',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hStandby',type='zes_standby_handle_t',tag='in'),
arg2=ArgDef(name='mode',type='zes_standby_promo_mode_t',tag='in'),
)

Function(name='zesTemperatureGetConfig',component='zes_temperature',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTemperature',type='zes_temp_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='zes_temp_config_t*',tag='inout'),
)

Function(name='zesTemperatureGetProperties',component='zes_temperature',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTemperature',type='zes_temp_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_temp_properties_t*',tag='inout'),
)

Function(name='zesTemperatureGetState',component='zes_temperature',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTemperature',type='zes_temp_handle_t',tag='in'),
arg2=ArgDef(name='pTemperature',type='double*',tag='inout'),
)

Function(name='zesTemperatureSetConfig',component='zes_temperature',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTemperature',type='zes_temp_handle_t',tag='in'),
arg2=ArgDef(name='pConfig',type='const zes_temp_config_t*',tag='in'),
)

Function(name='zesVFManagementGetVFCapabilitiesExp',component='zes_vf_management_exp',enabled=False,api_version='1.10',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='pCapability',type='zes_vf_exp_capabilities_t*',tag='inout'),
)

Function(name='zesVFManagementGetVFEngineUtilizationExp',component='zes_vf_management_exp',enabled=False,api_version='1.9',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pEngineUtil',type='zes_vf_util_engine_exp_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesVFManagementGetVFEngineUtilizationExp2',component='zes_vf_management_exp',enabled=False,api_version='1.10',ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pEngineUtil',type='zes_vf_util_engine_exp2_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesVFManagementGetVFMemoryUtilizationExp',component='zes_vf_management_exp',enabled=False,api_version='1.9',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pMemUtil',type='zes_vf_util_mem_exp_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesVFManagementGetVFMemoryUtilizationExp2',component='zes_vf_management_exp',enabled=False,api_version='1.10',ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pMemUtil',type='zes_vf_util_mem_exp2_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zesVFManagementGetVFPropertiesExp',component='zes_vf_management_exp',enabled=False,api_version='1.9',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zes_vf_exp_properties_t*',tag='inout'),
)

Function(name='zesVFManagementSetVFTelemetryModeExp',component='zes_vf_management_exp',enabled=False,api_version='1.9',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='flags',type='zes_vf_info_util_exp_flags_t',tag='in'),
arg3=ArgDef(name='enable',type='ze_bool_t',tag='in'),
)

Function(name='zesVFManagementSetVFTelemetrySamplingIntervalExp',component='zes_vf_management_exp',enabled=False,api_version='1.9',ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hVFhandle',type='zes_vf_handle_t',tag='in'),
arg2=ArgDef(name='flag',type='zes_vf_info_util_exp_flags_t',tag='in'),
arg3=ArgDef(name='samplingInterval',type='uint64_t',tag='in'),
)

Function(name='zetCommandListAppendMetricMemoryBarrier',component='zet_command_list',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='zet_command_list_handle_t',tag='in'),
)

Function(name='zetCommandListAppendMetricQueryBegin',component='zet_command_list',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='zet_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hMetricQuery',type='zet_metric_query_handle_t',tag='in'),
)

Function(name='zetCommandListAppendMetricQueryEnd',component='zet_command_list',enabled=True,ddi_pos=2,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='zet_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hMetricQuery',type='zet_metric_query_handle_t',tag='in'),
arg3=ArgDef(name='hSignalEvent',type='ze_event_handle_t',tag='in',optional=True),
arg4=ArgDef(name='numWaitEvents',type='uint32_t',tag='in',optional=True),
arg5=ArgDef(name='phWaitEvents',type='ze_event_handle_t*',tag='in',range='0,numWaitEvents',optional=True),
)

Function(name='zetCommandListAppendMetricStreamerMarker',component='zet_command_list',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hCommandList',type='zet_command_list_handle_t',tag='in'),
arg2=ArgDef(name='hMetricStreamer',type='zet_metric_streamer_handle_t',tag='in'),
arg3=ArgDef(name='value',type='uint32_t',tag='in'),
)

Function(name='zetContextActivateMetricGroups',component='zet_context',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='zet_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg3=ArgDef(name='count',type='uint32_t',tag='in'),
arg4=ArgDef(name='phMetricGroups',type='zet_metric_group_handle_t*',tag='in',range='0,count',optional=True),
)

Function(name='zetDebugAcknowledgeEvent',component='zet_debug',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='event',type='const zet_debug_event_t*',tag='in'),
)

Function(name='zetDebugAttach',component='zet_debug',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='config',type='const zet_debug_config_t*',tag='in'),
arg3=ArgDef(name='phDebug',type='zet_debug_session_handle_t*',tag='out'),
)

Function(name='zetDebugDetach',component='zet_debug',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in',release=True),
)

Function(name='zetDebugGetRegisterSetProperties',component='zet_debug',enabled=True,ddi_pos=8,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pRegisterSetProperties',type='zet_debug_regset_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zetDebugGetThreadRegisterSetProperties',component='zet_debug',enabled=True,api_version='1.5',ddi_pos=11,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
arg3=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg4=ArgDef(name='pRegisterSetProperties',type='zet_debug_regset_properties_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zetDebugInterrupt',component='zet_debug',enabled=True,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
)

Function(name='zetDebugReadEvent',component='zet_debug',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='timeout',type='uint64_t',tag='in'),
arg3=ArgDef(name='event',type='zet_debug_event_t*',tag='inout'),
)

Function(name='zetDebugReadMemory',component='zet_debug',enabled=True,ddi_pos=6,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
arg3=ArgDef(name='desc',type='const zet_debug_memory_space_desc_t*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='buffer',type='void*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetDebugReadRegisters',component='zet_debug',enabled=True,ddi_pos=9,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
arg3=ArgDef(name='type',type='uint32_t',tag='in'),
arg4=ArgDef(name='start',type='uint32_t',tag='in'),
arg5=ArgDef(name='count',type='uint32_t',tag='in'),
arg6=ArgDef(name='pRegisterValues',type='void*',tag='inout',wrapType='COutArgument',optional=True),
)

Function(name='zetDebugResume',component='zet_debug',enabled=True,ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
)

Function(name='zetDebugWriteMemory',component='zet_debug',enabled=True,ddi_pos=7,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
arg3=ArgDef(name='desc',type='const zet_debug_memory_space_desc_t*',tag='in'),
arg4=ArgDef(name='size',type='size_t',tag='in'),
arg5=ArgDef(name='buffer',type='const void*',tag='in',wrapType='COutArgument'),
)

Function(name='zetDebugWriteRegisters',component='zet_debug',enabled=True,ddi_pos=10,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDebug',type='zet_debug_session_handle_t',tag='in'),
arg2=ArgDef(name='thread',type='ze_device_thread_t',tag='in'),
arg3=ArgDef(name='type',type='uint32_t',tag='in'),
arg4=ArgDef(name='start',type='uint32_t',tag='in'),
arg5=ArgDef(name='count',type='uint32_t',tag='in'),
arg6=ArgDef(name='pRegisterValues',type='void*',tag='inout',wrapType='COutArgument',optional=True),
)

Function(name='zetDeviceCreateMetricGroupsFromMetricsExp',component='zet_device_exp',enabled=False,api_version='1.10',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='metricCount',type='uint32_t',tag='in'),
arg3=ArgDef(name='phMetrics',type='zet_metric_handle_t *',tag='in'),
arg4=ArgDef(name='pMetricGroupNamePrefix',type='const char *',tag='in'),
arg5=ArgDef(name='pDescription',type='const char *',tag='in'),
arg6=ArgDef(name='pMetricGroupCount',type='uint32_t *',tag='inout'),
arg7=ArgDef(name='phMetricGroup',type='zet_metric_group_handle_t*',tag='inout',range='0,*pMetricGroupCount',optional=True),
)

Function(name='zetDeviceGetConcurrentMetricGroupsExp',component='zet_device_exp',enabled=False,api_version='1.10',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='metricGroupCount',type='uint32_t',tag='in'),
arg3=ArgDef(name='phMetricGroups',type='zet_metric_group_handle_t *',tag='inout'),
arg4=ArgDef(name='pMetricGroupsCountPerConcurrentGroup',type='uint32_t *',tag='inout',optional=True),
arg5=ArgDef(name='pConcurrentGroupCount',type='uint32_t *',tag='out'),
)

Function(name='zetDeviceGetDebugProperties',component='zet_device',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='pDebugProperties',type='zet_device_debug_properties_t*',tag='inout'),
)

Function(name='zetKernelGetProfileInfo',component='zet_kernel',enabled=True,ddi_pos=0,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hKernel',type='zet_kernel_handle_t',tag='in'),
arg2=ArgDef(name='pProfileProperties',type='zet_profile_properties_t*',tag='out'),
)

Function(name='zetMetricCreateFromProgrammableExp',component='zet_metric_exp',enabled=False,api_version='1.11',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricProgrammable',type='zet_metric_programmable_exp_handle_t',tag='in'),
arg2=ArgDef(name='pParameterValues',type='zet_metric_programmable_param_value_exp_t*',tag='in'),
arg3=ArgDef(name='parameterCount',type='uint32_t',tag='in'),
arg4=ArgDef(name='pName',type='const char*',tag='in'),
arg5=ArgDef(name='pDescription',type='const char*',tag='in'),
arg6=ArgDef(name='pMetricHandleCount',type='uint32_t*',tag='inout'),
arg7=ArgDef(name='phMetricHandles',type='zet_metric_handle_t*',tag='inout',range='0,*pMetricHandleCount',optional=True),
)

Function(name='zetMetricCreateFromProgrammableExp2',component='zet_metric_exp',enabled=False,api_version='1.11',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricProgrammable',type='zet_metric_programmable_exp_handle_t',tag='in'),
arg2=ArgDef(name='parameterCount',type='uint32_t',tag='in'),
arg3=ArgDef(name='pParameterValues',type='zet_metric_programmable_param_value_exp_t*',tag='in'),
arg4=ArgDef(name='pName',type='const char*',tag='in'),
arg5=ArgDef(name='pDescription',type='const char*',tag='in'),
arg6=ArgDef(name='pMetricHandleCount',type='uint32_t*',tag='inout'),
arg7=ArgDef(name='phMetricHandles',type='zet_metric_handle_t*',tag='inout',range='0,*pMetricHandleCount',optional=True),
)

Function(name='zetMetricDecoderCreateExp',component='zet_metric_decoder_exp',enabled=False,api_version='1.10',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricTracer',type='zet_metric_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='phMetricDecoder',type='zet_metric_decoder_exp_handle_t*',tag='out'),
)

Function(name='zetMetricDecoderDestroyExp',component='zet_metric_decoder_exp',enabled=False,api_version='1.10',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='phMetricDecoder',type='zet_metric_decoder_exp_handle_t',tag='in'),
)

Function(name='zetMetricDecoderGetDecodableMetricsExp',component='zet_metric_decoder_exp',enabled=False,api_version='1.10',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricDecoder',type='zet_metric_decoder_exp_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phMetrics',type='zet_metric_handle_t*',tag='inout',range='0,*pCount'),
)

Function(name='zetMetricDestroyExp',component='zet_metric_exp',enabled=False,api_version='1.9',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetric',type='zet_metric_handle_t',tag='in'),
)

Function(name='zetMetricGet',component='zet_metric',enabled=True,ddi_pos=0,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phMetrics',type='zet_metric_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zetMetricGetProperties',component='zet_metric',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetric',type='zet_metric_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zet_metric_properties_t*',tag='inout'),
)

Function(name='zetMetricGroupAddMetricExp',component='zet_metric_group_exp',enabled=False,api_version='1.9',ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='hMetric',type='zet_metric_handle_t',tag='in'),
arg3=ArgDef(name='pErrorStringSize',type='size_t *',tag='inout',optional=True),
arg4=ArgDef(name='pErrorString',type='char*',tag='inout',range='0,*pErrorStringSize',optional=True),
)

Function(name='zetMetricGroupCalculateMetricExportDataExp',component='zet_metric_group_exp',enabled=False,api_version='1.6',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDriver',type='ze_driver_handle_t',tag='in'),
arg2=ArgDef(name='type',type='zet_metric_group_calculation_type_t',tag='in'),
arg3=ArgDef(name='exportDataSize',type='size_t',tag='in'),
arg4=ArgDef(name='pExportData',type='const uint8_t*',tag='in',range='0,exportDataSize'),
arg5=ArgDef(name='pCalculateDescriptor',type='zet_metric_calculate_exp_desc_t*',tag='in'),
arg6=ArgDef(name='pSetCount',type='uint32_t*',tag='inout'),
arg7=ArgDef(name='pTotalMetricValueCount',type='uint32_t*',tag='inout'),
arg8=ArgDef(name='pMetricCounts',type='uint32_t*',tag='inout',range='0,*pSetCount',optional=True),
arg9=ArgDef(name='pMetricValues',type='zet_typed_value_t*',tag='inout',range='0,*pTotalMetricValueCount',optional=True),
)

Function(name='zetMetricGroupCalculateMetricValues',component='zet_metric_group',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='type',type='zet_metric_group_calculation_type_t',tag='in'),
arg3=ArgDef(name='rawDataSize',type='size_t',tag='in'),
arg4=ArgDef(name='pRawData',type='const uint8_t*',tag='in',range='0,rawDataSize'),
arg5=ArgDef(name='pMetricValueCount',type='uint32_t*',tag='inout'),
arg6=ArgDef(name='pMetricValues',type='zet_typed_value_t*',tag='inout',range='0,*pMetricValueCount',optional=True),
)

Function(name='zetMetricGroupCalculateMultipleMetricValuesExp',component='zet_metric_group_exp',enabled=True,api_version='1.2',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='type',type='zet_metric_group_calculation_type_t',tag='in'),
arg3=ArgDef(name='rawDataSize',type='size_t',tag='in'),
arg4=ArgDef(name='pRawData',type='const uint8_t*',tag='in',range='0,rawDataSize'),
arg5=ArgDef(name='pSetCount',type='uint32_t*',tag='inout'),
arg6=ArgDef(name='pTotalMetricValueCount',type='uint32_t*',tag='inout'),
arg7=ArgDef(name='pMetricCounts',type='uint32_t*',tag='inout',range='0,*pSetCount',optional=True),
arg8=ArgDef(name='pMetricValues',type='zet_typed_value_t*',tag='inout',range='0,*pTotalMetricValueCount',optional=True),
)

Function(name='zetMetricGroupCloseExp',component='zet_metric_group_exp',enabled=False,api_version='1.9',ddi_pos=6,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
)

Function(name='zetMetricGroupCreateExp',component='zet_metric_group_exp',enabled=False,api_version='1.11',ddi_pos=8,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='pName',type='const char*',tag='in'),
arg3=ArgDef(name='pDescription',type='const char*',tag='in'),
arg4=ArgDef(name='samplingType',type='zet_metric_group_sampling_type_flags_t',tag='in'),
arg5=ArgDef(name='phMetricGroup',type='zet_metric_group_handle_t*',tag='inout'),
)

Function(name='zetMetricGroupDestroyExp',component='zet_metric_group_exp',enabled=False,api_version='1.9',ddi_pos=7,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
)

Function(name='zetMetricGroupGet',component='zet_metric_group',enabled=True,ddi_pos=0,recWrap=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phMetricGroups',type='zet_metric_group_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zetMetricGroupGetExportDataExp',component='zet_metric_group_exp',enabled=False,api_version='1.6',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='pRawData',type='const uint8_t*',tag='in'),
arg3=ArgDef(name='rawDataSize',type='size_t',tag='in'),
arg4=ArgDef(name='pExportDataSize',type='size_t*',tag='inout'),
arg5=ArgDef(name='pExportData',type='uint8_t *',tag='inout',range='0,*pExportDataSize',optional=True),
)

Function(name='zetMetricGroupGetGlobalTimestampsExp',component='zet_metric_group_exp',enabled=False,api_version='1.5',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='synchronizedWithHost',type='ze_bool_t',tag='in'),
arg3=ArgDef(name='globalTimestamp',type='uint64_t*',tag='out'),
arg4=ArgDef(name='metricTimestamp',type='uint64_t*',tag='out'),
)

Function(name='zetMetricGroupGetProperties',component='zet_metric_group',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zet_metric_group_properties_t*',tag='inout'),
)

Function(name='zetMetricGroupRemoveMetricExp',component='zet_metric_group_exp',enabled=False,api_version='1.9',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg2=ArgDef(name='hMetric',type='zet_metric_handle_t',tag='in'),
)

Function(name='zetMetricProgrammableGetExp',component='zet_metric_programmable_exp',enabled=False,api_version='1.9',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg2=ArgDef(name='pCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='phMetricProgrammables',type='zet_metric_programmable_exp_handle_t*',tag='inout',range='0,*pCount',optional=True),
)

Function(name='zetMetricProgrammableGetParamInfoExp',component='zet_metric_programmable_exp',enabled=False,api_version='1.9',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricProgrammable',type='zet_metric_programmable_exp_handle_t',tag='in'),
arg2=ArgDef(name='pParameterCount',type='uint32_t*',tag='inout'),
arg3=ArgDef(name='pParameterInfo',type='zet_metric_programmable_param_info_exp_t*',tag='inout',range='0,*pParameterCount'),
)

Function(name='zetMetricProgrammableGetParamValueInfoExp',component='zet_metric_programmable_exp',enabled=False,api_version='1.9',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricProgrammable',type='zet_metric_programmable_exp_handle_t',tag='in'),
arg2=ArgDef(name='parameterOrdinal',type='uint32_t',tag='in'),
arg3=ArgDef(name='pValueInfoCount',type='uint32_t*',tag='inout'),
arg4=ArgDef(name='pValueInfo',type='zet_metric_programmable_param_value_info_exp_t*',tag='inout',range='0,*pValueInfoCount'),
)

Function(name='zetMetricProgrammableGetPropertiesExp',component='zet_metric_programmable_exp',enabled=False,api_version='1.9',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricProgrammable',type='zet_metric_programmable_exp_handle_t',tag='in'),
arg2=ArgDef(name='pProperties',type='zet_metric_programmable_exp_properties_t*',tag='inout'),
)

Function(name='zetMetricQueryCreate',component='zet_metric_query',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricQueryPool',type='zet_metric_query_pool_handle_t',tag='in'),
arg2=ArgDef(name='index',type='uint32_t',tag='in'),
arg3=ArgDef(name='phMetricQuery',type='zet_metric_query_handle_t*',tag='out'),
)

Function(name='zetMetricQueryDestroy',component='zet_metric_query',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricQuery',type='zet_metric_query_handle_t',tag='in',release=True),
)

Function(name='zetMetricQueryGetData',component='zet_metric_query',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricQuery',type='zet_metric_query_handle_t',tag='in'),
arg2=ArgDef(name='pRawDataSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pRawData',type='uint8_t*',tag='inout',range='0,*pRawDataSize',optional=True),
)

Function(name='zetMetricQueryPoolCreate',component='zet_metric_query_pool',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='zet_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg3=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg4=ArgDef(name='desc',type='const zet_metric_query_pool_desc_t*',tag='in'),
arg5=ArgDef(name='phMetricQueryPool',type='zet_metric_query_pool_handle_t*',tag='out'),
)

Function(name='zetMetricQueryPoolDestroy',component='zet_metric_query_pool',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricQueryPool',type='zet_metric_query_pool_handle_t',tag='in',release=True),
)

Function(name='zetMetricQueryReset',component='zet_metric_query',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricQuery',type='zet_metric_query_handle_t',tag='in'),
)

Function(name='zetMetricStreamerClose',component='zet_metric_streamer',enabled=True,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricStreamer',type='zet_metric_streamer_handle_t',tag='in',release=True),
)

Function(name='zetMetricStreamerOpen',component='zet_metric_streamer',enabled=True,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='zet_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg3=ArgDef(name='hMetricGroup',type='zet_metric_group_handle_t',tag='in'),
arg4=ArgDef(name='desc',type='zet_metric_streamer_desc_t*',tag='inout'),
arg5=ArgDef(name='hNotificationEvent',type='ze_event_handle_t',tag='in',optional=True),
arg6=ArgDef(name='phMetricStreamer',type='zet_metric_streamer_handle_t*',tag='out'),
)

Function(name='zetMetricStreamerReadData',component='zet_metric_streamer',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricStreamer',type='zet_metric_streamer_handle_t',tag='in'),
arg2=ArgDef(name='maxReportCount',type='uint32_t',tag='in'),
arg3=ArgDef(name='pRawDataSize',type='size_t*',tag='inout'),
arg4=ArgDef(name='pRawData',type='uint8_t*',tag='inout',range='0,*pRawDataSize',optional=True),
)

Function(name='zetMetricTracerCreateExp',component='zet_metric_tracer_exp',enabled=False,api_version='1.10',ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='zet_context_handle_t',tag='in'),
arg2=ArgDef(name='hDevice',type='zet_device_handle_t',tag='in'),
arg3=ArgDef(name='metricGroupCount',type='uint32_t',tag='in'),
arg4=ArgDef(name='phMetricGroups',type='zet_metric_group_handle_t*',tag='in',range='0,metricGroupCount '),
arg5=ArgDef(name='desc',type='zet_metric_tracer_exp_desc_t*',tag='inout'),
arg6=ArgDef(name='hNotificationEvent',type='ze_event_handle_t',tag='in',optional=True),
arg7=ArgDef(name='phMetricTracer',type='zet_metric_tracer_exp_handle_t*',tag='out'),
)

Function(name='zetMetricTracerDecodeExp',component='zet_metric_tracer_exp',enabled=False,api_version='1.10',ddi_pos=5,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='phMetricDecoder',type='zet_metric_decoder_exp_handle_t',tag='in'),
arg2=ArgDef(name='pRawDataSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pRawData',type='uint8_t*',tag='inout',range='0,*pRawDataSize',optional=True),
arg4=ArgDef(name='metricsCount',type='uint32_t',tag='in'),
arg5=ArgDef(name='phMetrics',type='zet_metric_handle_t*',tag='in',range='0,metricsCount'),
arg6=ArgDef(name='pSetCount',type='uint32_t*',tag='inout'),
arg7=ArgDef(name='pMetricEntriesCountPerSet',type='uint32_t*',tag='inout',range='0,*pSetCount',optional=True),
arg8=ArgDef(name='pMetricEntriesCount',type='uint32_t*',tag='inout'),
arg9=ArgDef(name='pMetricEntries',type='zet_metric_entry_exp_t*',tag='inout',range='0,*pMetricEntriesCount',optional=True),
)

Function(name='zetMetricTracerDestroyExp',component='zet_metric_tracer_exp',enabled=False,api_version='1.10',ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricTracer',type='zet_metric_tracer_exp_handle_t',tag='in'),
)

Function(name='zetMetricTracerDisableExp',component='zet_metric_tracer_exp',enabled=False,api_version='1.10',ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricTracer',type='zet_metric_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='synchronous',type='ze_bool_t',tag='in'),
)

Function(name='zetMetricTracerEnableExp',component='zet_metric_tracer_exp',enabled=False,api_version='1.10',ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricTracer',type='zet_metric_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='synchronous',type='ze_bool_t',tag='in'),
)

Function(name='zetMetricTracerReadDataExp',component='zet_metric_tracer_exp',enabled=False,api_version='1.10',ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hMetricTracer',type='zet_metric_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='pRawDataSize',type='size_t*',tag='inout'),
arg3=ArgDef(name='pRawData',type='uint8_t*',tag='inout',range='0,*pRawDataSize',optional=True),
)

Function(name='zetModuleGetDebugInfo',component='zet_module',enabled=False,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hModule',type='zet_module_handle_t',tag='in'),
arg2=ArgDef(name='format',type='zet_module_debug_info_format_t',tag='in'),
arg3=ArgDef(name='pSize',type='size_t*',tag='inout'),
arg4=ArgDef(name='pDebugInfo',type='uint8_t*',tag='inout',optional=True),
)

Function(name='zetTracerExpCreate',component='zet_tracer_exp',enabled=False,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hContext',type='zet_context_handle_t',tag='in'),
arg2=ArgDef(name='desc',type='const zet_tracer_exp_desc_t*',tag='in'),
arg3=ArgDef(name='phTracer',type='zet_tracer_exp_handle_t*',tag='out'),
)

Function(name='zetTracerExpDestroy',component='zet_tracer_exp',enabled=False,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zet_tracer_exp_handle_t',tag='in',release=True),
)

Function(name='zetTracerExpSetEnabled',component='zet_tracer_exp',enabled=False,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zet_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='enable',type='ze_bool_t',tag='in'),
)

Function(name='zetTracerExpSetEpilogues',component='zet_tracer_exp',enabled=True,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zet_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='pCoreCbs',type='zet_core_callbacks_t*',tag='in'),
)

Function(name='zetTracerExpSetPrologues',component='zet_tracer_exp',enabled=True,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zet_tracer_exp_handle_t',tag='in'),
arg2=ArgDef(name='pCoreCbs',type='zet_core_callbacks_t*',tag='in'),
)

Function(name='zelTracerCreate',component='zel_tracer',enabled=False,ddi_pos=0,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='desc',type='const zel_tracer_desc_t*',tag='in'),
arg2=ArgDef(name='phTracer',type='zel_tracer_handle_t*',tag='out'),
)

Function(name='zelTracerDestroy',component='zel_tracer',enabled=False,ddi_pos=1,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in',release=True),
)

Function(name='zelTracerSetEnabled',component='zel_tracer',enabled=False,ddi_pos=4,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='enable',type='ze_bool_t',tag='in'),
)

Function(name='zelTracerSetEpilogues',component='zel_tracer',enabled=False,ddi_pos=3,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='pCoreCbs',type='zel_core_callbacks_t*',tag='in'),
)

Function(name='zelTracerSetPrologues',component='zel_tracer',enabled=False,ddi_pos=2,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='pCoreCbs',type='zel_core_callbacks_t*',tag='in'),
)

Function(name='zelLoaderGetVersions',component='zel',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='num_elems',type='size_t*',tag='in'),
arg2=ArgDef(name='versions',type='zel_component_version_t *',tag='inout',wrapType='COutArgument'),
)

Function(name='zelSetDriverTeardown',component='zel',enabled=False,recExecWrap=True,
retV=RetDef(type='ze_result_t'),
)

Function(name='zelEnableTracingLayer',component='zel',enabled=False,
retV=RetDef(type='ze_result_t'),
)

Function(name='zelDisableTracingLayer',component='zel',enabled=False,
retV=RetDef(type='ze_result_t'),
)

Function(name='zelLoaderTranslateHandle',component='zel',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='handleType',type='zel_handle_type_t',tag='in'),
arg2=ArgDef(name='handleIn',type='void *',tag='in',wrapType='COutArgument'),
arg3=ArgDef(name='handleOut',type='void **',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetCommandListProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_command_list_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetCommandListExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_command_list_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetCommandQueueProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_command_queue_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetContextProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_context_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetDeviceProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_device_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetDeviceExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_device_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetDriverProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_driver_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetDriverExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_driver_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetEventProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_event_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetEventExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_event_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetEventPoolProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_event_pool_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetFabricEdgeExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_fabric_edge_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetFabricVertexExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_fabric_vertex_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetFenceProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_fence_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetGlobalProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_global_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetImageProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_image_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetImageExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_image_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetKernelProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_kernel_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetKernelExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_kernel_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetMemProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_mem_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetMemExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_mem_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetModuleProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_module_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetModuleBuildLogProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_module_build_log_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetPhysicalMemProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_physical_mem_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetRtasBuilderExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_rtas_builder_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetRtasParallelOperationExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_rtas_parallel_operation_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetSamplerProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_sampler_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGetVirtualMemProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='ze_virtual_mem_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetDeviceProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_device_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetDeviceExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_device_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetDiagnosticsProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_diagnostics_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetDriverProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_driver_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetDriverExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_driver_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetEngineProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_engine_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetFabricPortProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_fabric_port_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetFanProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_fan_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetFirmwareProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_firmware_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetFirmwareExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_firmware_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetFrequencyProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_frequency_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetGlobalProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_global_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetLedProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_led_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetMemoryProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_memory_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetOverclockProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_overclock_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetPerformanceFactorProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_performance_factor_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetPowerProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_power_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetPsuProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_psu_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetRasProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_ras_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetRasExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_ras_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetSchedulerProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_scheduler_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetStandbyProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_standby_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetTemperatureProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_temperature_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zesGetVfManagementExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zes_vf_management_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetCommandListProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_command_list_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetContextProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_context_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetDebugProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_debug_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetDeviceProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_device_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetDeviceExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_device_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetKernelProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_kernel_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricDecoderExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_decoder_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricGroupProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_group_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricGroupExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_group_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricProgrammableExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_programmable_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricQueryProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_query_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricQueryPoolProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_query_pool_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricStreamerProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_streamer_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetMetricTracerExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_metric_tracer_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetModuleProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_module_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zetGetTracerExpProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zet_tracer_exp_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zelGetTracerApiProcAddrTable',component='ze_dditable',enabled=True,skipRun=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='version',type='ze_api_version_t',tag='in'),
arg2=ArgDef(name='pDdiTable',type='zel_tracer_dditable_t*',tag='inout',wrapType='COutArgument'),
)

Function(name='zeGitsIndirectAllocationOffsets',component='ze_gits_extension',extension=True,enabled=True,stateTrack=True,recWrap=True,
retV=RetDef(type='void'),
arg1=ArgDef(name='pAlloc',type='void*',tag='in',wrapType='CMappedPtr'),
arg2=ArgDef(name='numOffsets',type='uint32_t',tag='in'),
arg3=ArgDef(name='pOffsets',type='size_t*',tag='in',range='0,numOffsets'),
)

Function(name='zeGitsStopRecording',component='ze_gits_extension',extension=True,enabled=True,recWrap=True,skipRun=True,recExecWrap=True,log=False,
retV=RetDef(type='void'),
arg1=ArgDef(name='properties',type='ze_gits_recording_info_t',tag='in'),
)

Function(name='zeGitsStartRecording',component='ze_gits_extension',extension=True,enabled=True,recWrap=True,skipRun=True,recExecWrap=True,log=False,
retV=RetDef(type='void'),
arg1=ArgDef(name='properties',type='ze_gits_recording_info_t',tag='in'),
)

Function(name='zeGitsOriginalQueueFamilyInfo',component='ze_gits_extension',extension=True,enabled=True,log=False,stateTrack=True,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hDevice',type='ze_device_handle_t',tag='in'),
arg2=ArgDef(name='count',type='uint32_t',tag='in'),
arg3=ArgDef(name='cqGroupProperties',type='const ze_command_queue_group_properties_t*',tag='in',wrapParams='count, {name}')
)

Function(name='zelTracerInitRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnInitCb',type='ze_pfnInitCb_t',tag='in'),
)

Function(name='zelTracerDriverGetRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetCb',type='ze_pfnDriverGetCb_t',tag='in'),
)

Function(name='zelTracerInitDriversRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnInitDriversCb',type='ze_pfnInitDriversCb_t',tag='in'),
)

Function(name='zelTracerDriverGetApiVersionRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetApiVersionCb',type='ze_pfnDriverGetApiVersionCb_t',tag='in'),
)

Function(name='zelTracerDriverGetPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesCb',type='ze_pfnDriverGetPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDriverGetIpcPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetIpcPropertiesCb',type='ze_pfnDriverGetIpcPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDriverGetExtensionPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetExtensionPropertiesCb',type='ze_pfnDriverGetExtensionPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDriverGetExtensionFunctionAddressRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetExtensionFunctionAddressCb',type='ze_pfnDriverGetExtensionFunctionAddressCb_t',tag='in'),
)

Function(name='zelTracerDriverGetLastErrorDescriptionRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetLastErrorDescriptionCb',type='ze_pfnDriverGetLastErrorDescriptionCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetCb',type='ze_pfnDeviceGetCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetRootDeviceRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetRootDeviceCb',type='ze_pfnDeviceGetRootDeviceCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetSubDevicesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetSubDevicesCb',type='ze_pfnDeviceGetSubDevicesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesCb',type='ze_pfnDeviceGetPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetComputePropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetComputePropertiesCb',type='ze_pfnDeviceGetComputePropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetModulePropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetModulePropertiesCb',type='ze_pfnDeviceGetModulePropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetCommandQueueGroupPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetCommandQueueGroupPropertiesCb',type='ze_pfnDeviceGetCommandQueueGroupPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetMemoryPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetMemoryPropertiesCb',type='ze_pfnDeviceGetMemoryPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetMemoryAccessPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetMemoryAccessPropertiesCb',type='ze_pfnDeviceGetMemoryAccessPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetCachePropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetCachePropertiesCb',type='ze_pfnDeviceGetCachePropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetImagePropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetImagePropertiesCb',type='ze_pfnDeviceGetImagePropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetExternalMemoryPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetExternalMemoryPropertiesCb',type='ze_pfnDeviceGetExternalMemoryPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetP2PPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetP2PPropertiesCb',type='ze_pfnDeviceGetP2PPropertiesCb_t',tag='in'),
)

Function(name='zelTracerDeviceCanAccessPeerRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCanAccessPeerCb',type='ze_pfnDeviceCanAccessPeerCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetStatusRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetStatusCb',type='ze_pfnDeviceGetStatusCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetGlobalTimestampsRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetGlobalTimestampsCb',type='ze_pfnDeviceGetGlobalTimestampsCb_t',tag='in'),
)

Function(name='zelTracerContextCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnContextCreateCb_t',tag='in'),
)

Function(name='zelTracerContextCreateExRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateExCb',type='ze_pfnContextCreateExCb_t',tag='in'),
)

Function(name='zelTracerContextDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnContextDestroyCb_t',tag='in'),
)

Function(name='zelTracerContextGetStatusRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetStatusCb',type='ze_pfnContextGetStatusCb_t',tag='in'),
)

Function(name='zelTracerCommandQueueCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnCommandQueueCreateCb_t',tag='in'),
)

Function(name='zelTracerCommandQueueDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnCommandQueueDestroyCb_t',tag='in'),
)

Function(name='zelTracerCommandQueueExecuteCommandListsRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnExecuteCommandListsCb',type='ze_pfnCommandQueueExecuteCommandListsCb_t',tag='in'),
)

Function(name='zelTracerCommandQueueSynchronizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSynchronizeCb',type='ze_pfnCommandQueueSynchronizeCb_t',tag='in'),
)

Function(name='zelTracerCommandQueueGetOrdinalRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetOrdinalCb',type='ze_pfnCommandQueueGetOrdinalCb_t',tag='in'),
)

Function(name='zelTracerCommandQueueGetIndexRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetIndexCb',type='ze_pfnCommandQueueGetIndexCb_t',tag='in'),
)

Function(name='zelTracerCommandListCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnCommandListCreateCb_t',tag='in'),
)

Function(name='zelTracerCommandListCreateImmediateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateImmediateCb',type='ze_pfnCommandListCreateImmediateCb_t',tag='in'),
)

Function(name='zelTracerCommandListDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnCommandListDestroyCb_t',tag='in'),
)

Function(name='zelTracerCommandListCloseRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCloseCb',type='ze_pfnCommandListCloseCb_t',tag='in'),
)

Function(name='zelTracerCommandListResetRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnResetCb',type='ze_pfnCommandListResetCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendWriteGlobalTimestampRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendWriteGlobalTimestampCb',type='ze_pfnCommandListAppendWriteGlobalTimestampCb_t',tag='in'),
)

Function(name='zelTracerCommandListHostSynchronizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnHostSynchronizeCb',type='ze_pfnCommandListHostSynchronizeCb_t',tag='in'),
)

Function(name='zelTracerCommandListGetDeviceHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetDeviceHandleCb',type='ze_pfnCommandListGetDeviceHandleCb_t',tag='in'),
)

Function(name='zelTracerCommandListGetContextHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetContextHandleCb',type='ze_pfnCommandListGetContextHandleCb_t',tag='in'),
)

Function(name='zelTracerCommandListGetOrdinalRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetOrdinalCb',type='ze_pfnCommandListGetOrdinalCb_t',tag='in'),
)

Function(name='zelTracerCommandListImmediateGetIndexRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnImmediateGetIndexCb',type='ze_pfnCommandListImmediateGetIndexCb_t',tag='in'),
)

Function(name='zelTracerCommandListIsImmediateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnIsImmediateCb',type='ze_pfnCommandListIsImmediateCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendBarrierRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendBarrierCb',type='ze_pfnCommandListAppendBarrierCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemoryRangesBarrierRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemoryRangesBarrierCb',type='ze_pfnCommandListAppendMemoryRangesBarrierCb_t',tag='in'),
)

Function(name='zelTracerContextSystemBarrierRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSystemBarrierCb',type='ze_pfnContextSystemBarrierCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemoryCopyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemoryCopyCb',type='ze_pfnCommandListAppendMemoryCopyCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemoryFillRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemoryFillCb',type='ze_pfnCommandListAppendMemoryFillCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemoryCopyRegionRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemoryCopyRegionCb',type='ze_pfnCommandListAppendMemoryCopyRegionCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemoryCopyFromContextRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemoryCopyFromContextCb',type='ze_pfnCommandListAppendMemoryCopyFromContextCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendImageCopyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendImageCopyCb',type='ze_pfnCommandListAppendImageCopyCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendImageCopyRegionRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendImageCopyRegionCb',type='ze_pfnCommandListAppendImageCopyRegionCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendImageCopyToMemoryRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendImageCopyToMemoryCb',type='ze_pfnCommandListAppendImageCopyToMemoryCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendImageCopyFromMemoryRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendImageCopyFromMemoryCb',type='ze_pfnCommandListAppendImageCopyFromMemoryCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemoryPrefetchRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemoryPrefetchCb',type='ze_pfnCommandListAppendMemoryPrefetchCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendMemAdviseRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendMemAdviseCb',type='ze_pfnCommandListAppendMemAdviseCb_t',tag='in'),
)

Function(name='zelTracerEventPoolCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnEventPoolCreateCb_t',tag='in'),
)

Function(name='zelTracerEventPoolDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnEventPoolDestroyCb_t',tag='in'),
)

Function(name='zelTracerEventCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnEventCreateCb_t',tag='in'),
)

Function(name='zelTracerEventDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnEventDestroyCb_t',tag='in'),
)

Function(name='zelTracerEventPoolGetIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetIpcHandleCb',type='ze_pfnEventPoolGetIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerEventPoolPutIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnPutIpcHandleCb',type='ze_pfnEventPoolPutIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerEventPoolOpenIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnOpenIpcHandleCb',type='ze_pfnEventPoolOpenIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerEventPoolCloseIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCloseIpcHandleCb',type='ze_pfnEventPoolCloseIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendSignalEventRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendSignalEventCb',type='ze_pfnCommandListAppendSignalEventCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendWaitOnEventsRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendWaitOnEventsCb',type='ze_pfnCommandListAppendWaitOnEventsCb_t',tag='in'),
)

Function(name='zelTracerEventHostSignalRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnHostSignalCb',type='ze_pfnEventHostSignalCb_t',tag='in'),
)

Function(name='zelTracerEventHostSynchronizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnHostSynchronizeCb',type='ze_pfnEventHostSynchronizeCb_t',tag='in'),
)

Function(name='zelTracerEventQueryStatusRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnQueryStatusCb',type='ze_pfnEventQueryStatusCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendEventResetRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendEventResetCb',type='ze_pfnCommandListAppendEventResetCb_t',tag='in'),
)

Function(name='zelTracerEventHostResetRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnHostResetCb',type='ze_pfnEventHostResetCb_t',tag='in'),
)

Function(name='zelTracerEventQueryKernelTimestampRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnQueryKernelTimestampCb',type='ze_pfnEventQueryKernelTimestampCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendQueryKernelTimestampsRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendQueryKernelTimestampsCb',type='ze_pfnCommandListAppendQueryKernelTimestampsCb_t',tag='in'),
)

Function(name='zelTracerEventGetEventPoolRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetEventPoolCb',type='ze_pfnEventGetEventPoolCb_t',tag='in'),
)

Function(name='zelTracerEventGetSignalScopeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetSignalScopeCb',type='ze_pfnEventGetSignalScopeCb_t',tag='in'),
)

Function(name='zelTracerEventGetWaitScopeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetWaitScopeCb',type='ze_pfnEventGetWaitScopeCb_t',tag='in'),
)

Function(name='zelTracerEventPoolGetContextHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetContextHandleCb',type='ze_pfnEventPoolGetContextHandleCb_t',tag='in'),
)

Function(name='zelTracerEventPoolGetFlagsRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetFlagsCb',type='ze_pfnEventPoolGetFlagsCb_t',tag='in'),
)

Function(name='zelTracerFenceCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnFenceCreateCb_t',tag='in'),
)

Function(name='zelTracerFenceDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnFenceDestroyCb_t',tag='in'),
)

Function(name='zelTracerFenceHostSynchronizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnHostSynchronizeCb',type='ze_pfnFenceHostSynchronizeCb_t',tag='in'),
)

Function(name='zelTracerFenceQueryStatusRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnQueryStatusCb',type='ze_pfnFenceQueryStatusCb_t',tag='in'),
)

Function(name='zelTracerFenceResetRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnResetCb',type='ze_pfnFenceResetCb_t',tag='in'),
)

Function(name='zelTracerImageGetPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesCb',type='ze_pfnImageGetPropertiesCb_t',tag='in'),
)

Function(name='zelTracerImageCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnImageCreateCb_t',tag='in'),
)

Function(name='zelTracerImageDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnImageDestroyCb_t',tag='in'),
)

Function(name='zelTracerMemAllocSharedRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAllocSharedCb',type='ze_pfnMemAllocSharedCb_t',tag='in'),
)

Function(name='zelTracerMemAllocDeviceRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAllocDeviceCb',type='ze_pfnMemAllocDeviceCb_t',tag='in'),
)

Function(name='zelTracerMemAllocHostRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAllocHostCb',type='ze_pfnMemAllocHostCb_t',tag='in'),
)

Function(name='zelTracerMemFreeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnFreeCb',type='ze_pfnMemFreeCb_t',tag='in'),
)

Function(name='zelTracerMemGetAllocPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetAllocPropertiesCb',type='ze_pfnMemGetAllocPropertiesCb_t',tag='in'),
)

Function(name='zelTracerMemGetAddressRangeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetAddressRangeCb',type='ze_pfnMemGetAddressRangeCb_t',tag='in'),
)

Function(name='zelTracerMemGetIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetIpcHandleCb',type='ze_pfnMemGetIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerMemGetIpcHandleFromFileDescriptorExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetIpcHandleFromFileDescriptorExpCb',type='ze_pfnMemGetIpcHandleFromFileDescriptorExpCb_t',tag='in'),
)

Function(name='zelTracerMemGetFileDescriptorFromIpcHandleExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetFileDescriptorFromIpcHandleExpCb',type='ze_pfnMemGetFileDescriptorFromIpcHandleExpCb_t',tag='in'),
)

Function(name='zelTracerMemPutIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnPutIpcHandleCb',type='ze_pfnMemPutIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerMemOpenIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnOpenIpcHandleCb',type='ze_pfnMemOpenIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerMemCloseIpcHandleRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCloseIpcHandleCb',type='ze_pfnMemCloseIpcHandleCb_t',tag='in'),
)

Function(name='zelTracerMemSetAtomicAccessAttributeExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetAtomicAccessAttributeExpCb',type='ze_pfnMemSetAtomicAccessAttributeExpCb_t',tag='in'),
)

Function(name='zelTracerMemGetAtomicAccessAttributeExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetAtomicAccessAttributeExpCb',type='ze_pfnMemGetAtomicAccessAttributeExpCb_t',tag='in'),
)

Function(name='zelTracerModuleCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnModuleCreateCb_t',tag='in'),
)

Function(name='zelTracerModuleDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnModuleDestroyCb_t',tag='in'),
)

Function(name='zelTracerModuleDynamicLinkRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDynamicLinkCb',type='ze_pfnModuleDynamicLinkCb_t',tag='in'),
)

Function(name='zelTracerModuleBuildLogDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnModuleBuildLogDestroyCb_t',tag='in'),
)

Function(name='zelTracerModuleBuildLogGetStringRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetStringCb',type='ze_pfnModuleBuildLogGetStringCb_t',tag='in'),
)

Function(name='zelTracerModuleGetNativeBinaryRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetNativeBinaryCb',type='ze_pfnModuleGetNativeBinaryCb_t',tag='in'),
)

Function(name='zelTracerModuleGetGlobalPointerRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetGlobalPointerCb',type='ze_pfnModuleGetGlobalPointerCb_t',tag='in'),
)

Function(name='zelTracerModuleGetKernelNamesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetKernelNamesCb',type='ze_pfnModuleGetKernelNamesCb_t',tag='in'),
)

Function(name='zelTracerModuleGetPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesCb',type='ze_pfnModuleGetPropertiesCb_t',tag='in'),
)

Function(name='zelTracerKernelCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnKernelCreateCb_t',tag='in'),
)

Function(name='zelTracerKernelDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnKernelDestroyCb_t',tag='in'),
)

Function(name='zelTracerModuleGetFunctionPointerRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetFunctionPointerCb',type='ze_pfnModuleGetFunctionPointerCb_t',tag='in'),
)

Function(name='zelTracerKernelSetGroupSizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetGroupSizeCb',type='ze_pfnKernelSetGroupSizeCb_t',tag='in'),
)

Function(name='zelTracerKernelSuggestGroupSizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSuggestGroupSizeCb',type='ze_pfnKernelSuggestGroupSizeCb_t',tag='in'),
)

Function(name='zelTracerKernelSuggestMaxCooperativeGroupCountRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSuggestMaxCooperativeGroupCountCb',type='ze_pfnKernelSuggestMaxCooperativeGroupCountCb_t',tag='in'),
)

Function(name='zelTracerKernelSetArgumentValueRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetArgumentValueCb',type='ze_pfnKernelSetArgumentValueCb_t',tag='in'),
)

Function(name='zelTracerKernelSetIndirectAccessRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetIndirectAccessCb',type='ze_pfnKernelSetIndirectAccessCb_t',tag='in'),
)

Function(name='zelTracerKernelGetIndirectAccessRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetIndirectAccessCb',type='ze_pfnKernelGetIndirectAccessCb_t',tag='in'),
)

Function(name='zelTracerKernelGetSourceAttributesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetSourceAttributesCb',type='ze_pfnKernelGetSourceAttributesCb_t',tag='in'),
)

Function(name='zelTracerKernelSetCacheConfigRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetCacheConfigCb',type='ze_pfnKernelSetCacheConfigCb_t',tag='in'),
)

Function(name='zelTracerKernelGetPropertiesRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesCb',type='ze_pfnKernelGetPropertiesCb_t',tag='in'),
)

Function(name='zelTracerKernelGetNameRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetNameCb',type='ze_pfnKernelGetNameCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendLaunchKernelRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendLaunchKernelCb',type='ze_pfnCommandListAppendLaunchKernelCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendLaunchCooperativeKernelRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendLaunchCooperativeKernelCb',type='ze_pfnCommandListAppendLaunchCooperativeKernelCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendLaunchKernelIndirectRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendLaunchKernelIndirectCb',type='ze_pfnCommandListAppendLaunchKernelIndirectCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendLaunchMultipleKernelsIndirectRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendLaunchMultipleKernelsIndirectCb',type='ze_pfnCommandListAppendLaunchMultipleKernelsIndirectCb_t',tag='in'),
)

Function(name='zelTracerContextMakeMemoryResidentRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnMakeMemoryResidentCb',type='ze_pfnContextMakeMemoryResidentCb_t',tag='in'),
)

Function(name='zelTracerContextEvictMemoryRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnEvictMemoryCb',type='ze_pfnContextEvictMemoryCb_t',tag='in'),
)

Function(name='zelTracerContextMakeImageResidentRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnMakeImageResidentCb',type='ze_pfnContextMakeImageResidentCb_t',tag='in'),
)

Function(name='zelTracerContextEvictImageRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnEvictImageCb',type='ze_pfnContextEvictImageCb_t',tag='in'),
)

Function(name='zelTracerSamplerCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnSamplerCreateCb_t',tag='in'),
)

Function(name='zelTracerSamplerDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnSamplerDestroyCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemReserveRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnReserveCb',type='ze_pfnVirtualMemReserveCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemFreeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnFreeCb',type='ze_pfnVirtualMemFreeCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemQueryPageSizeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnQueryPageSizeCb',type='ze_pfnVirtualMemQueryPageSizeCb_t',tag='in'),
)

Function(name='zelTracerPhysicalMemCreateRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCb',type='ze_pfnPhysicalMemCreateCb_t',tag='in'),
)

Function(name='zelTracerPhysicalMemDestroyRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyCb',type='ze_pfnPhysicalMemDestroyCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemMapRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnMapCb',type='ze_pfnVirtualMemMapCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemUnmapRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnUnmapCb',type='ze_pfnVirtualMemUnmapCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemSetAccessAttributeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetAccessAttributeCb',type='ze_pfnVirtualMemSetAccessAttributeCb_t',tag='in'),
)

Function(name='zelTracerVirtualMemGetAccessAttributeRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetAccessAttributeCb',type='ze_pfnVirtualMemGetAccessAttributeCb_t',tag='in'),
)

Function(name='zelTracerKernelSetGlobalOffsetExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetGlobalOffsetExpCb',type='ze_pfnKernelSetGlobalOffsetExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListGetNextCommandIdExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetNextCommandIdExpCb',type='ze_pfnCommandListGetNextCommandIdExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListGetNextCommandIdWithKernelsExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetNextCommandIdWithKernelsExpCb',type='ze_pfnCommandListGetNextCommandIdWithKernelsExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListUpdateMutableCommandsExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnUpdateMutableCommandsExpCb',type='ze_pfnCommandListUpdateMutableCommandsExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListUpdateMutableCommandSignalEventExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnUpdateMutableCommandSignalEventExpCb',type='ze_pfnCommandListUpdateMutableCommandSignalEventExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListUpdateMutableCommandWaitEventsExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnUpdateMutableCommandWaitEventsExpCb',type='ze_pfnCommandListUpdateMutableCommandWaitEventsExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListUpdateMutableCommandKernelsExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnUpdateMutableCommandKernelsExpCb',type='ze_pfnCommandListUpdateMutableCommandKernelsExpCb_t',tag='in'),
)

Function(name='zelTracerDeviceReserveCacheExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnReserveCacheExtCb',type='ze_pfnDeviceReserveCacheExtCb_t',tag='in'),
)

Function(name='zelTracerDeviceSetCacheAdviceExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSetCacheAdviceExtCb',type='ze_pfnDeviceSetCacheAdviceExtCb_t',tag='in'),
)

Function(name='zelTracerEventQueryTimestampsExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnQueryTimestampsExpCb',type='ze_pfnEventQueryTimestampsExpCb_t',tag='in'),
)

Function(name='zelTracerImageGetMemoryPropertiesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetMemoryPropertiesExpCb',type='ze_pfnImageGetMemoryPropertiesExpCb_t',tag='in'),
)

Function(name='zelTracerImageViewCreateExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnViewCreateExtCb',type='ze_pfnImageViewCreateExtCb_t',tag='in'),
)

Function(name='zelTracerImageViewCreateExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnViewCreateExpCb',type='ze_pfnImageViewCreateExpCb_t',tag='in'),
)

Function(name='zelTracerKernelSchedulingHintExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnSchedulingHintExpCb',type='ze_pfnKernelSchedulingHintExpCb_t',tag='in'),
)

Function(name='zelTracerDevicePciGetPropertiesExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnPciGetPropertiesExtCb',type='ze_pfnDevicePciGetPropertiesExtCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendImageCopyToMemoryExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendImageCopyToMemoryExtCb',type='ze_pfnCommandListAppendImageCopyToMemoryExtCb_t',tag='in'),
)

Function(name='zelTracerCommandListAppendImageCopyFromMemoryExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnAppendImageCopyFromMemoryExtCb',type='ze_pfnCommandListAppendImageCopyFromMemoryExtCb_t',tag='in'),
)

Function(name='zelTracerImageGetAllocPropertiesExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetAllocPropertiesExtCb',type='ze_pfnImageGetAllocPropertiesExtCb_t',tag='in'),
)

Function(name='zelTracerModuleInspectLinkageExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnInspectLinkageExtCb',type='ze_pfnModuleInspectLinkageExtCb_t',tag='in'),
)

Function(name='zelTracerMemFreeExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnFreeExtCb',type='ze_pfnMemFreeExtCb_t',tag='in'),
)

Function(name='zelTracerFabricVertexGetExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetExpCb',type='ze_pfnFabricVertexGetExpCb_t',tag='in'),
)

Function(name='zelTracerFabricVertexGetSubVerticesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetSubVerticesExpCb',type='ze_pfnFabricVertexGetSubVerticesExpCb_t',tag='in'),
)

Function(name='zelTracerFabricVertexGetPropertiesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesExpCb',type='ze_pfnFabricVertexGetPropertiesExpCb_t',tag='in'),
)

Function(name='zelTracerFabricVertexGetDeviceExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetDeviceExpCb',type='ze_pfnFabricVertexGetDeviceExpCb_t',tag='in'),
)

Function(name='zelTracerDeviceGetFabricVertexExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetFabricVertexExpCb',type='ze_pfnDeviceGetFabricVertexExpCb_t',tag='in'),
)

Function(name='zelTracerFabricEdgeGetExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetExpCb',type='ze_pfnFabricEdgeGetExpCb_t',tag='in'),
)

Function(name='zelTracerFabricEdgeGetVerticesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetVerticesExpCb',type='ze_pfnFabricEdgeGetVerticesExpCb_t',tag='in'),
)

Function(name='zelTracerFabricEdgeGetPropertiesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesExpCb',type='ze_pfnFabricEdgeGetPropertiesExpCb_t',tag='in'),
)

Function(name='zelTracerEventQueryKernelTimestampsExtRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnQueryKernelTimestampsExtCb',type='ze_pfnEventQueryKernelTimestampsExtCb_t',tag='in'),
)

Function(name='zelTracerRTASBuilderCreateExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateExpCb',type='ze_pfnRTASBuilderCreateExpCb_t',tag='in'),
)

Function(name='zelTracerRTASBuilderGetBuildPropertiesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetBuildPropertiesExpCb',type='ze_pfnRTASBuilderGetBuildPropertiesExpCb_t',tag='in'),
)

Function(name='zelTracerDriverRTASFormatCompatibilityCheckExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnRTASFormatCompatibilityCheckExpCb',type='ze_pfnDriverRTASFormatCompatibilityCheckExpCb_t',tag='in'),
)

Function(name='zelTracerRTASBuilderBuildExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnBuildExpCb',type='ze_pfnRTASBuilderBuildExpCb_t',tag='in'),
)

Function(name='zelTracerRTASBuilderDestroyExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyExpCb',type='ze_pfnRTASBuilderDestroyExpCb_t',tag='in'),
)

Function(name='zelTracerRTASParallelOperationCreateExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateExpCb',type='ze_pfnRTASParallelOperationCreateExpCb_t',tag='in'),
)

Function(name='zelTracerRTASParallelOperationGetPropertiesExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPropertiesExpCb',type='ze_pfnRTASParallelOperationGetPropertiesExpCb_t',tag='in'),
)

Function(name='zelTracerRTASParallelOperationJoinExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnJoinExpCb',type='ze_pfnRTASParallelOperationJoinExpCb_t',tag='in'),
)

Function(name='zelTracerRTASParallelOperationDestroyExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnDestroyExpCb',type='ze_pfnRTASParallelOperationDestroyExpCb_t',tag='in'),
)

Function(name='zelTracerMemGetPitchFor2dImageRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetPitchFor2dImageCb',type='ze_pfnMemGetPitchFor2dImageCb_t',tag='in'),
)

Function(name='zelTracerImageGetDeviceOffsetExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnGetDeviceOffsetExpCb',type='ze_pfnImageGetDeviceOffsetExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListCreateCloneExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnCreateCloneExpCb',type='ze_pfnCommandListCreateCloneExpCb_t',tag='in'),
)

Function(name='zelTracerCommandListImmediateAppendCommandListsExpRegisterCallback',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
arg2=ArgDef(name='callback_type',type='zel_tracer_reg_t',tag='in'),
arg3=ArgDef(name='pfnImmediateAppendCommandListsExpCb',type='ze_pfnCommandListImmediateAppendCommandListsExpCb_t',tag='in'),
)

Function(name='zelTracerResetAllCallbacks',component='zel_tracer_register_cb',enabled=False,
retV=RetDef(type='ze_result_t'),
arg1=ArgDef(name='hTracer',type='zel_tracer_handle_t',tag='in'),
)

