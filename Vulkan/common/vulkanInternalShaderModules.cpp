// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanInternalShaderModules.cpp
*
* @brief Declaration of SPIR-V data for custom shader modules.
*
*/

#include "vulkanInternalShaderModules.h"

namespace gits {
namespace Vulkan {

// Generated with:
//
// glslangValidator.exe -V -H -o GITS_shader.h --vn code PrepareDeviceAddressesForPatching.comp
//
// Compiled from the following source code:
//
// --------------------------------------------------------------------------------------------
//
// #version 450
//
// #extension GL_EXT_buffer_reference : require
// #extension GL_EXT_buffer_reference_uvec2 : require
//
// layout(local_size_x = 1, local_size_y = 1) in;
//
// struct InputDataStruct {
//   uvec2 address;
//   uint offset;
//   uint padding;
// };
//
// struct OutputDataStruct {
//   uvec2 dVA;
//   uvec2 REF;
// };
//
// layout(buffer_reference,
//        std430,
//        buffer_reference_align = 16) readonly buffer PushConstantAddressOfInputData {
//   InputDataStruct inputData[];
// };
//
// layout(buffer_reference,
//        std430,
//        buffer_reference_align = 16) writeonly buffer PushConstantAddressOfOutputData {
//   OutputDataStruct outputData[];
// };
//
// layout(buffer_reference, std430, buffer_reference_align = 8) readonly buffer DeviceAddress {
//   uvec2 value;
// };
//
// layout(push_constant) uniform PushConstants {
//   PushConstantAddressOfInputData AddressOfInputData;
//   PushConstantAddressOfOutputData AddressOfOutputData;
// };
//
// uvec2 uadd_64_32(uvec2 address, uint offset) {
//   uint carry;
//   address.x = uaddCarry(address.x, offset, carry);
//   address.y += carry;
//   return address;
// }
//
// void main() {
//   InputDataStruct inputData = AddressOfInputData.inputData[gl_GlobalInvocationID.x];
//
//   uvec2 iVA = inputData.address;
//   uvec2 address = DeviceAddress(iVA).value;
//
//   uvec2 dVA = uadd_64_32(address, inputData.offset);
//   uvec2 REF = DeviceAddress(dVA).value;
//
//   AddressOfOutputData.outputData[gl_GlobalInvocationID.x] = OutputDataStruct(dVA, REF);
// }

std::vector<uint32_t> getPrepareDeviceAddressesForPatchingShaderModuleSource() {
  std::vector<uint32_t> code = {
      0x07230203, 0x00010000, 0x0008000b, 0x0000006d, 0x00000000, 0x00020011, 0x00000001,
      0x00020011, 0x000014e3, 0x0009000a, 0x5f565053, 0x5f52484b, 0x73796870, 0x6c616369,
      0x6f74735f, 0x65676172, 0x6675625f, 0x00726566, 0x0006000b, 0x00000001, 0x4c534c47,
      0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x000014e4, 0x00000001, 0x0006000f,
      0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x00000037, 0x00060010, 0x00000004,
      0x00000011, 0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2,
      0x00070004, 0x455f4c47, 0x625f5458, 0x65666675, 0x65725f72, 0x65726566, 0x0065636e,
      0x00090004, 0x455f4c47, 0x625f5458, 0x65666675, 0x65725f72, 0x65726566, 0x5f65636e,
      0x63657675, 0x00000032, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00070005,
      0x0000000d, 0x64646175, 0x5f34365f, 0x76283233, 0x753b3275, 0x00003b31, 0x00040005,
      0x0000000b, 0x72646461, 0x00737365, 0x00040005, 0x0000000c, 0x7366666f, 0x00007465,
      0x00040005, 0x00000013, 0x72726163, 0x00000079, 0x00040005, 0x00000014, 0x54736552,
      0x00657079, 0x00060005, 0x00000022, 0x75706e49, 0x74614474, 0x72745361, 0x00746375,
      0x00050006, 0x00000022, 0x00000000, 0x72646461, 0x00737365, 0x00050006, 0x00000022,
      0x00000001, 0x7366666f, 0x00007465, 0x00050006, 0x00000022, 0x00000002, 0x64646170,
      0x00676e69, 0x00050005, 0x00000024, 0x75706e69, 0x74614474, 0x00000061, 0x00060005,
      0x00000027, 0x68737550, 0x736e6f43, 0x746e6174, 0x00000073, 0x00080006, 0x00000027,
      0x00000000, 0x72646441, 0x4f737365, 0x706e4966, 0x61447475, 0x00006174, 0x00080006,
      0x00000027, 0x00000001, 0x72646441, 0x4f737365, 0x74754f66, 0x44747570, 0x00617461,
      0x00060005, 0x00000028, 0x75706e49, 0x74614474, 0x72745361, 0x00746375, 0x00050006,
      0x00000028, 0x00000000, 0x72646461, 0x00737365, 0x00050006, 0x00000028, 0x00000001,
      0x7366666f, 0x00007465, 0x00050006, 0x00000028, 0x00000002, 0x64646170, 0x00676e69,
      0x000a0005, 0x0000002a, 0x68737550, 0x736e6f43, 0x746e6174, 0x72646441, 0x4f737365,
      0x706e4966, 0x61447475, 0x00006174, 0x00060006, 0x0000002a, 0x00000000, 0x75706e69,
      0x74614474, 0x00000061, 0x00070005, 0x0000002b, 0x7074754f, 0x61447475, 0x74536174,
      0x74637572, 0x00000000, 0x00040006, 0x0000002b, 0x00000000, 0x00415664, 0x00040006,
      0x0000002b, 0x00000001, 0x00464552, 0x000a0005, 0x0000002d, 0x68737550, 0x736e6f43,
      0x746e6174, 0x72646441, 0x4f737365, 0x74754f66, 0x44747570, 0x00617461, 0x00060006,
      0x0000002d, 0x00000000, 0x7074756f, 0x61447475, 0x00006174, 0x00030005, 0x0000002f,
      0x00000000, 0x00080005, 0x00000037, 0x475f6c67, 0x61626f6c, 0x766e496c, 0x7461636f,
      0x496e6f69, 0x00000044, 0x00030005, 0x00000046, 0x00415669, 0x00040005, 0x00000049,
      0x72646461, 0x00737365, 0x00060005, 0x0000004c, 0x69766544, 0x64416563, 0x73657264,
      0x00000073, 0x00050006, 0x0000004c, 0x00000000, 0x756c6176, 0x00000065, 0x00030005,
      0x00000051, 0x00415664, 0x00040005, 0x00000052, 0x61726170, 0x0000006d, 0x00040005,
      0x00000054, 0x61726170, 0x0000006d, 0x00030005, 0x00000058, 0x00464552, 0x00070005,
      0x00000064, 0x7074754f, 0x61447475, 0x74536174, 0x74637572, 0x00000000, 0x00040006,
      0x00000064, 0x00000000, 0x00415664, 0x00040006, 0x00000064, 0x00000001, 0x00464552,
      0x00050048, 0x00000027, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000027,
      0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x00000027, 0x00000002, 0x00050048,
      0x00000028, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000028, 0x00000001,
      0x00000023, 0x00000008, 0x00050048, 0x00000028, 0x00000002, 0x00000023, 0x0000000c,
      0x00040047, 0x00000029, 0x00000006, 0x00000010, 0x00040048, 0x0000002a, 0x00000000,
      0x00000018, 0x00050048, 0x0000002a, 0x00000000, 0x00000023, 0x00000000, 0x00030047,
      0x0000002a, 0x00000002, 0x00050048, 0x0000002b, 0x00000000, 0x00000023, 0x00000000,
      0x00050048, 0x0000002b, 0x00000001, 0x00000023, 0x00000008, 0x00040047, 0x0000002c,
      0x00000006, 0x00000010, 0x00040048, 0x0000002d, 0x00000000, 0x00000019, 0x00050048,
      0x0000002d, 0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x0000002d, 0x00000002,
      0x00040047, 0x00000037, 0x0000000b, 0x0000001c, 0x00040048, 0x0000004c, 0x00000000,
      0x00000018, 0x00050048, 0x0000004c, 0x00000000, 0x00000023, 0x00000000, 0x00030047,
      0x0000004c, 0x00000002, 0x00040047, 0x0000006c, 0x0000000b, 0x00000019, 0x00020013,
      0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015, 0x00000006, 0x00000020,
      0x00000000, 0x00040017, 0x00000007, 0x00000006, 0x00000002, 0x00040020, 0x00000008,
      0x00000007, 0x00000007, 0x00040020, 0x00000009, 0x00000007, 0x00000006, 0x00050021,
      0x0000000a, 0x00000007, 0x00000008, 0x00000009, 0x0004002b, 0x00000006, 0x0000000f,
      0x00000000, 0x0004001e, 0x00000014, 0x00000006, 0x00000006, 0x0004002b, 0x00000006,
      0x0000001a, 0x00000001, 0x0005001e, 0x00000022, 0x00000007, 0x00000006, 0x00000006,
      0x00040020, 0x00000023, 0x00000007, 0x00000022, 0x00030027, 0x00000025, 0x000014e5,
      0x00030027, 0x00000026, 0x000014e5, 0x0004001e, 0x00000027, 0x00000025, 0x00000026,
      0x0005001e, 0x00000028, 0x00000007, 0x00000006, 0x00000006, 0x0003001d, 0x00000029,
      0x00000028, 0x0003001e, 0x0000002a, 0x00000029, 0x00040020, 0x00000025, 0x000014e5,
      0x0000002a, 0x0004001e, 0x0000002b, 0x00000007, 0x00000007, 0x0003001d, 0x0000002c,
      0x0000002b, 0x0003001e, 0x0000002d, 0x0000002c, 0x00040020, 0x00000026, 0x000014e5,
      0x0000002d, 0x00040020, 0x0000002e, 0x00000009, 0x00000027, 0x0004003b, 0x0000002e,
      0x0000002f, 0x00000009, 0x00040015, 0x00000030, 0x00000020, 0x00000001, 0x0004002b,
      0x00000030, 0x00000031, 0x00000000, 0x00040020, 0x00000032, 0x00000009, 0x00000025,
      0x00040017, 0x00000035, 0x00000006, 0x00000003, 0x00040020, 0x00000036, 0x00000001,
      0x00000035, 0x0004003b, 0x00000036, 0x00000037, 0x00000001, 0x00040020, 0x00000038,
      0x00000001, 0x00000006, 0x00040020, 0x0000003b, 0x000014e5, 0x00000028, 0x0004002b,
      0x00000030, 0x00000041, 0x00000001, 0x0004002b, 0x00000030, 0x00000044, 0x00000002,
      0x00030027, 0x0000004b, 0x000014e5, 0x0003001e, 0x0000004c, 0x00000007, 0x00040020,
      0x0000004b, 0x000014e5, 0x0000004c, 0x00040020, 0x0000004e, 0x000014e5, 0x00000007,
      0x00040020, 0x0000005d, 0x00000009, 0x00000026, 0x0004001e, 0x00000064, 0x00000007,
      0x00000007, 0x00040020, 0x00000066, 0x000014e5, 0x0000002b, 0x0006002c, 0x00000035,
      0x0000006c, 0x0000001a, 0x0000001a, 0x0000001a, 0x00050036, 0x00000002, 0x00000004,
      0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x00000023, 0x00000024,
      0x00000007, 0x0004003b, 0x00000008, 0x00000046, 0x00000007, 0x0004003b, 0x00000008,
      0x00000049, 0x00000007, 0x0004003b, 0x00000008, 0x00000051, 0x00000007, 0x0004003b,
      0x00000008, 0x00000052, 0x00000007, 0x0004003b, 0x00000009, 0x00000054, 0x00000007,
      0x0004003b, 0x00000008, 0x00000058, 0x00000007, 0x00050041, 0x00000032, 0x00000033,
      0x0000002f, 0x00000031, 0x0004003d, 0x00000025, 0x00000034, 0x00000033, 0x00050041,
      0x00000038, 0x00000039, 0x00000037, 0x0000000f, 0x0004003d, 0x00000006, 0x0000003a,
      0x00000039, 0x00060041, 0x0000003b, 0x0000003c, 0x00000034, 0x00000031, 0x0000003a,
      0x0006003d, 0x00000028, 0x0000003d, 0x0000003c, 0x00000002, 0x00000010, 0x00050051,
      0x00000007, 0x0000003e, 0x0000003d, 0x00000000, 0x00050041, 0x00000008, 0x0000003f,
      0x00000024, 0x00000031, 0x0003003e, 0x0000003f, 0x0000003e, 0x00050051, 0x00000006,
      0x00000040, 0x0000003d, 0x00000001, 0x00050041, 0x00000009, 0x00000042, 0x00000024,
      0x00000041, 0x0003003e, 0x00000042, 0x00000040, 0x00050051, 0x00000006, 0x00000043,
      0x0000003d, 0x00000002, 0x00050041, 0x00000009, 0x00000045, 0x00000024, 0x00000044,
      0x0003003e, 0x00000045, 0x00000043, 0x00050041, 0x00000008, 0x00000047, 0x00000024,
      0x00000031, 0x0004003d, 0x00000007, 0x00000048, 0x00000047, 0x0003003e, 0x00000046,
      0x00000048, 0x0004003d, 0x00000007, 0x0000004a, 0x00000046, 0x0004007c, 0x0000004b,
      0x0000004d, 0x0000004a, 0x00050041, 0x0000004e, 0x0000004f, 0x0000004d, 0x00000031,
      0x0006003d, 0x00000007, 0x00000050, 0x0000004f, 0x00000002, 0x00000008, 0x0003003e,
      0x00000049, 0x00000050, 0x0004003d, 0x00000007, 0x00000053, 0x00000049, 0x0003003e,
      0x00000052, 0x00000053, 0x00050041, 0x00000009, 0x00000055, 0x00000024, 0x00000041,
      0x0004003d, 0x00000006, 0x00000056, 0x00000055, 0x0003003e, 0x00000054, 0x00000056,
      0x00060039, 0x00000007, 0x00000057, 0x0000000d, 0x00000052, 0x00000054, 0x0003003e,
      0x00000051, 0x00000057, 0x0004003d, 0x00000007, 0x00000059, 0x00000051, 0x0004007c,
      0x0000004b, 0x0000005a, 0x00000059, 0x00050041, 0x0000004e, 0x0000005b, 0x0000005a,
      0x00000031, 0x0006003d, 0x00000007, 0x0000005c, 0x0000005b, 0x00000002, 0x00000008,
      0x0003003e, 0x00000058, 0x0000005c, 0x00050041, 0x0000005d, 0x0000005e, 0x0000002f,
      0x00000041, 0x0004003d, 0x00000026, 0x0000005f, 0x0000005e, 0x00050041, 0x00000038,
      0x00000060, 0x00000037, 0x0000000f, 0x0004003d, 0x00000006, 0x00000061, 0x00000060,
      0x0004003d, 0x00000007, 0x00000062, 0x00000051, 0x0004003d, 0x00000007, 0x00000063,
      0x00000058, 0x00050050, 0x00000064, 0x00000065, 0x00000062, 0x00000063, 0x00060041,
      0x00000066, 0x00000067, 0x0000005f, 0x00000031, 0x00000061, 0x00050051, 0x00000007,
      0x00000068, 0x00000065, 0x00000000, 0x00050041, 0x0000004e, 0x00000069, 0x00000067,
      0x00000031, 0x0005003e, 0x00000069, 0x00000068, 0x00000002, 0x00000000, 0x00050051,
      0x00000007, 0x0000006a, 0x00000065, 0x00000001, 0x00050041, 0x0000004e, 0x0000006b,
      0x00000067, 0x00000041, 0x0005003e, 0x0000006b, 0x0000006a, 0x00000002, 0x00000008,
      0x000100fd, 0x00010038, 0x00050036, 0x00000007, 0x0000000d, 0x00000000, 0x0000000a,
      0x00030037, 0x00000008, 0x0000000b, 0x00030037, 0x00000009, 0x0000000c, 0x000200f8,
      0x0000000e, 0x0004003b, 0x00000009, 0x00000013, 0x00000007, 0x00050041, 0x00000009,
      0x00000010, 0x0000000b, 0x0000000f, 0x0004003d, 0x00000006, 0x00000011, 0x00000010,
      0x0004003d, 0x00000006, 0x00000012, 0x0000000c, 0x00050095, 0x00000014, 0x00000015,
      0x00000011, 0x00000012, 0x00050051, 0x00000006, 0x00000016, 0x00000015, 0x00000001,
      0x0003003e, 0x00000013, 0x00000016, 0x00050051, 0x00000006, 0x00000017, 0x00000015,
      0x00000000, 0x00050041, 0x00000009, 0x00000018, 0x0000000b, 0x0000000f, 0x0003003e,
      0x00000018, 0x00000017, 0x0004003d, 0x00000006, 0x00000019, 0x00000013, 0x00050041,
      0x00000009, 0x0000001b, 0x0000000b, 0x0000001a, 0x0004003d, 0x00000006, 0x0000001c,
      0x0000001b, 0x00050080, 0x00000006, 0x0000001d, 0x0000001c, 0x00000019, 0x00050041,
      0x00000009, 0x0000001e, 0x0000000b, 0x0000001a, 0x0003003e, 0x0000001e, 0x0000001d,
      0x0004003d, 0x00000007, 0x0000001f, 0x0000000b, 0x000200fe, 0x0000001f, 0x00010038};
  return code;
}

// Generated with:
//
// glslangValidator.exe -V -H -o GITS_shader.h --vn code PatchDeviceAddresses.comp
//
// Compiled from the following source code:
//
// --------------------------------------------------------------------------------------------
//
// #version 450
//
// #extension GL_EXT_buffer_reference : require
// #extension GL_EXT_buffer_reference_uvec2 : require
//
// layout(local_size_x = 1, local_size_y = 1) in;
//
// struct DeviceAddressPatchGITS {
//   uvec2 originalValue;
//   uvec2 newValue;
// };
//
// layout(buffer_reference,
//        std430,
//        buffer_reference_align = 16) readonly buffer PushConstantAddressOfLocations {
//   uvec2 locations[];
// };
//
// layout(buffer_reference,
//        std430,
//        buffer_reference_align = 16) readonly buffer PushConstantAddressOfPatchesMap {
//   DeviceAddressPatchGITS patches[];
// };
//
// layout(buffer_reference, std430, buffer_reference_align = 8) buffer DeviceAddress {
//   uvec2 value;
// };
//
// layout(push_constant) uniform PushConstants {
//   PushConstantAddressOfLocations AddressOfLocations;
//   PushConstantAddressOfPatchesMap AddressOfPatchesMap;
//   uint NumMapElements;
// };
//
// void main() {
//   uvec2 location = AddressOfLocations.locations[gl_GlobalInvocationID.x];
//   uvec2 storedValue = DeviceAddress(location).value;
//
//   for (uint i = 0; i < NumMapElements; ++i) {
//     DeviceAddressPatchGITS addressPatch = AddressOfPatchesMap.patches[i];
//
//     if (all(equal(storedValue, addressPatch.originalValue))) {
//       DeviceAddress(location).value = addressPatch.newValue;
//       return;
//     }
//   }
// }

std::vector<uint32_t> getPatchDeviceAddressesShaderModuleSource() {
  std::vector<uint32_t> code = {
      0x07230203, 0x00010000, 0x0008000b, 0x00000059, 0x00000000, 0x00020011, 0x00000001,
      0x00020011, 0x000014e3, 0x0009000a, 0x5f565053, 0x5f52484b, 0x73796870, 0x6c616369,
      0x6f74735f, 0x65676172, 0x6675625f, 0x00726566, 0x0006000b, 0x00000001, 0x4c534c47,
      0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x000014e4, 0x00000001, 0x0006000f,
      0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x0000001b, 0x00060010, 0x00000004,
      0x00000011, 0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2,
      0x00070004, 0x455f4c47, 0x625f5458, 0x65666675, 0x65725f72, 0x65726566, 0x0065636e,
      0x00090004, 0x455f4c47, 0x625f5458, 0x65666675, 0x65725f72, 0x65726566, 0x5f65636e,
      0x63657675, 0x00000032, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00050005,
      0x00000009, 0x61636f6c, 0x6e6f6974, 0x00000000, 0x00060005, 0x0000000c, 0x68737550,
      0x736e6f43, 0x746e6174, 0x00000073, 0x00080006, 0x0000000c, 0x00000000, 0x72646441,
      0x4f737365, 0x636f4c66, 0x6f697461, 0x0000736e, 0x00080006, 0x0000000c, 0x00000001,
      0x72646441, 0x4f737365, 0x74615066, 0x73656863, 0x0070614d, 0x00070006, 0x0000000c,
      0x00000002, 0x4d6d754e, 0x6c457061, 0x6e656d65, 0x00007374, 0x000a0005, 0x0000000e,
      0x68737550, 0x736e6f43, 0x746e6174, 0x72646441, 0x4f737365, 0x636f4c66, 0x6f697461,
      0x0000736e, 0x00060006, 0x0000000e, 0x00000000, 0x61636f6c, 0x6e6f6974, 0x00000073,
      0x00080005, 0x0000000f, 0x69766544, 0x64416563, 0x73657264, 0x74615073, 0x49476863,
      0x00005354, 0x00070006, 0x0000000f, 0x00000000, 0x6769726f, 0x6c616e69, 0x756c6156,
      0x00000065, 0x00060006, 0x0000000f, 0x00000001, 0x5677656e, 0x65756c61, 0x00000000,
      0x000a0005, 0x00000011, 0x68737550, 0x736e6f43, 0x746e6174, 0x72646441, 0x4f737365,
      0x74615066, 0x73656863, 0x0070614d, 0x00050006, 0x00000011, 0x00000000, 0x63746170,
      0x00736568, 0x00030005, 0x00000013, 0x00000000, 0x00080005, 0x0000001b, 0x475f6c67,
      0x61626f6c, 0x766e496c, 0x7461636f, 0x496e6f69, 0x00000044, 0x00050005, 0x00000023,
      0x726f7473, 0x61566465, 0x0065756c, 0x00060005, 0x00000026, 0x69766544, 0x64416563,
      0x73657264, 0x00000073, 0x00050006, 0x00000026, 0x00000000, 0x756c6176, 0x00000065,
      0x00030005, 0x0000002b, 0x00000069, 0x00080005, 0x00000038, 0x69766544, 0x64416563,
      0x73657264, 0x74615073, 0x49476863, 0x00005354, 0x00070006, 0x00000038, 0x00000000,
      0x6769726f, 0x6c616e69, 0x756c6156, 0x00000065, 0x00060006, 0x00000038, 0x00000001,
      0x5677656e, 0x65756c61, 0x00000000, 0x00060005, 0x0000003a, 0x72646461, 0x50737365,
      0x68637461, 0x00000000, 0x00050048, 0x0000000c, 0x00000000, 0x00000023, 0x00000000,
      0x00050048, 0x0000000c, 0x00000001, 0x00000023, 0x00000008, 0x00050048, 0x0000000c,
      0x00000002, 0x00000023, 0x00000010, 0x00030047, 0x0000000c, 0x00000002, 0x00040047,
      0x0000000d, 0x00000006, 0x00000008, 0x00040048, 0x0000000e, 0x00000000, 0x00000018,
      0x00050048, 0x0000000e, 0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x0000000e,
      0x00000002, 0x00050048, 0x0000000f, 0x00000000, 0x00000023, 0x00000000, 0x00050048,
      0x0000000f, 0x00000001, 0x00000023, 0x00000008, 0x00040047, 0x00000010, 0x00000006,
      0x00000010, 0x00040048, 0x00000011, 0x00000000, 0x00000018, 0x00050048, 0x00000011,
      0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x00000011, 0x00000002, 0x00040047,
      0x0000001b, 0x0000000b, 0x0000001c, 0x00050048, 0x00000026, 0x00000000, 0x00000023,
      0x00000000, 0x00030047, 0x00000026, 0x00000002, 0x00040047, 0x00000058, 0x0000000b,
      0x00000019, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015,
      0x00000006, 0x00000020, 0x00000000, 0x00040017, 0x00000007, 0x00000006, 0x00000002,
      0x00040020, 0x00000008, 0x00000007, 0x00000007, 0x00030027, 0x0000000a, 0x000014e5,
      0x00030027, 0x0000000b, 0x000014e5, 0x0005001e, 0x0000000c, 0x0000000a, 0x0000000b,
      0x00000006, 0x0003001d, 0x0000000d, 0x00000007, 0x0003001e, 0x0000000e, 0x0000000d,
      0x00040020, 0x0000000a, 0x000014e5, 0x0000000e, 0x0004001e, 0x0000000f, 0x00000007,
      0x00000007, 0x0003001d, 0x00000010, 0x0000000f, 0x0003001e, 0x00000011, 0x00000010,
      0x00040020, 0x0000000b, 0x000014e5, 0x00000011, 0x00040020, 0x00000012, 0x00000009,
      0x0000000c, 0x0004003b, 0x00000012, 0x00000013, 0x00000009, 0x00040015, 0x00000014,
      0x00000020, 0x00000001, 0x0004002b, 0x00000014, 0x00000015, 0x00000000, 0x00040020,
      0x00000016, 0x00000009, 0x0000000a, 0x00040017, 0x00000019, 0x00000006, 0x00000003,
      0x00040020, 0x0000001a, 0x00000001, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b,
      0x00000001, 0x0004002b, 0x00000006, 0x0000001c, 0x00000000, 0x00040020, 0x0000001d,
      0x00000001, 0x00000006, 0x00040020, 0x00000020, 0x000014e5, 0x00000007, 0x00030027,
      0x00000025, 0x000014e5, 0x0003001e, 0x00000026, 0x00000007, 0x00040020, 0x00000025,
      0x000014e5, 0x00000026, 0x00040020, 0x0000002a, 0x00000007, 0x00000006, 0x0004002b,
      0x00000014, 0x00000032, 0x00000002, 0x00040020, 0x00000033, 0x00000009, 0x00000006,
      0x00020014, 0x00000036, 0x0004001e, 0x00000038, 0x00000007, 0x00000007, 0x00040020,
      0x00000039, 0x00000007, 0x00000038, 0x0004002b, 0x00000014, 0x0000003b, 0x00000001,
      0x00040020, 0x0000003c, 0x00000009, 0x0000000b, 0x00040020, 0x00000040, 0x000014e5,
      0x0000000f, 0x00040017, 0x0000004a, 0x00000036, 0x00000002, 0x0004002b, 0x00000006,
      0x00000057, 0x00000001, 0x0006002c, 0x00000019, 0x00000058, 0x00000057, 0x00000057,
      0x00000057, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
      0x00000005, 0x0004003b, 0x00000008, 0x00000009, 0x00000007, 0x0004003b, 0x00000008,
      0x00000023, 0x00000007, 0x0004003b, 0x0000002a, 0x0000002b, 0x00000007, 0x0004003b,
      0x00000039, 0x0000003a, 0x00000007, 0x00050041, 0x00000016, 0x00000017, 0x00000013,
      0x00000015, 0x0004003d, 0x0000000a, 0x00000018, 0x00000017, 0x00050041, 0x0000001d,
      0x0000001e, 0x0000001b, 0x0000001c, 0x0004003d, 0x00000006, 0x0000001f, 0x0000001e,
      0x00060041, 0x00000020, 0x00000021, 0x00000018, 0x00000015, 0x0000001f, 0x0006003d,
      0x00000007, 0x00000022, 0x00000021, 0x00000002, 0x00000008, 0x0003003e, 0x00000009,
      0x00000022, 0x0004003d, 0x00000007, 0x00000024, 0x00000009, 0x0004007c, 0x00000025,
      0x00000027, 0x00000024, 0x00050041, 0x00000020, 0x00000028, 0x00000027, 0x00000015,
      0x0006003d, 0x00000007, 0x00000029, 0x00000028, 0x00000002, 0x00000008, 0x0003003e,
      0x00000023, 0x00000029, 0x0003003e, 0x0000002b, 0x0000001c, 0x000200f9, 0x0000002c,
      0x000200f8, 0x0000002c, 0x000400f6, 0x0000002e, 0x0000002f, 0x00000000, 0x000200f9,
      0x00000030, 0x000200f8, 0x00000030, 0x0004003d, 0x00000006, 0x00000031, 0x0000002b,
      0x00050041, 0x00000033, 0x00000034, 0x00000013, 0x00000032, 0x0004003d, 0x00000006,
      0x00000035, 0x00000034, 0x000500b0, 0x00000036, 0x00000037, 0x00000031, 0x00000035,
      0x000400fa, 0x00000037, 0x0000002d, 0x0000002e, 0x000200f8, 0x0000002d, 0x00050041,
      0x0000003c, 0x0000003d, 0x00000013, 0x0000003b, 0x0004003d, 0x0000000b, 0x0000003e,
      0x0000003d, 0x0004003d, 0x00000006, 0x0000003f, 0x0000002b, 0x00060041, 0x00000040,
      0x00000041, 0x0000003e, 0x00000015, 0x0000003f, 0x0006003d, 0x0000000f, 0x00000042,
      0x00000041, 0x00000002, 0x00000010, 0x00050051, 0x00000007, 0x00000043, 0x00000042,
      0x00000000, 0x00050041, 0x00000008, 0x00000044, 0x0000003a, 0x00000015, 0x0003003e,
      0x00000044, 0x00000043, 0x00050051, 0x00000007, 0x00000045, 0x00000042, 0x00000001,
      0x00050041, 0x00000008, 0x00000046, 0x0000003a, 0x0000003b, 0x0003003e, 0x00000046,
      0x00000045, 0x0004003d, 0x00000007, 0x00000047, 0x00000023, 0x00050041, 0x00000008,
      0x00000048, 0x0000003a, 0x00000015, 0x0004003d, 0x00000007, 0x00000049, 0x00000048,
      0x000500aa, 0x0000004a, 0x0000004b, 0x00000047, 0x00000049, 0x0004009b, 0x00000036,
      0x0000004c, 0x0000004b, 0x000300f7, 0x0000004e, 0x00000000, 0x000400fa, 0x0000004c,
      0x0000004d, 0x0000004e, 0x000200f8, 0x0000004d, 0x0004003d, 0x00000007, 0x0000004f,
      0x00000009, 0x0004007c, 0x00000025, 0x00000050, 0x0000004f, 0x00050041, 0x00000008,
      0x00000051, 0x0000003a, 0x0000003b, 0x0004003d, 0x00000007, 0x00000052, 0x00000051,
      0x00050041, 0x00000020, 0x00000053, 0x00000050, 0x00000015, 0x0005003e, 0x00000053,
      0x00000052, 0x00000002, 0x00000008, 0x000100fd, 0x000200f8, 0x0000004e, 0x000200f9,
      0x0000002f, 0x000200f8, 0x0000002f, 0x0004003d, 0x00000006, 0x00000055, 0x0000002b,
      0x00050080, 0x00000006, 0x00000056, 0x00000055, 0x0000003b, 0x0003003e, 0x0000002b,
      0x00000056, 0x000200f9, 0x0000002c, 0x000200f8, 0x0000002e, 0x000100fd, 0x00010038};
  return code;
}

} // namespace Vulkan
} // namespace gits
