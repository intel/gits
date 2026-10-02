// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rayTracingReplayService.h"
#include "configurator.h"
#include "playerManager.h"
#include "vulkanHelpers.h"

namespace gits {
namespace vulkan {

thread_local std::vector<uint8_t> RayTracingReplayService::tl_ShaderGroupHandles;

namespace {
// Generated with:
//
// glslangValidator.exe -V -H -o GITS_shader.h --vn code PatchSBT.comp
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
// layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
//
// struct ShaderGroupHandle {
//   uvec4 Value[2];
// };
//
// layout(buffer_reference,
//        std430,
//        buffer_reference_align = 16) readonly buffer PushConstantAddressOfHandles {
//   ShaderGroupHandle Handles[];
// };
//
// layout(buffer_reference, std430, buffer_reference_align = 32) buffer DeviceAddress {
//   ShaderGroupHandle HandleToReplace;
// };
//
// layout(push_constant) uniform PushConstants {
//   PushConstantAddressOfHandles OriginalHandles;
//   PushConstantAddressOfHandles NewHandles;
//   uvec2 BaseAddress;
//   uint Stride;
//   uint Size;
// };
//
// uvec2 calculateAddress(uvec2 base, uint offset) {
//   uint carry;
//   base.x = uaddCarry(base.x, offset, carry);
//   base.y += carry;
//   return base;
// }
//
// void main() {
//   uint offset = gl_GlobalInvocationID.x * Stride;
//   if (offset >= Size) {
//     return;
//   }
//
//   uvec2 address = calculateAddress(BaseAddress, offset);
//   uint mapIndex = gl_GlobalInvocationID.y;
//
//   ShaderGroupHandle handleToReplace = DeviceAddress(address).HandleToReplace;
//   if (handleToReplace == OriginalHandles.Handles[mapIndex]) {
//     DeviceAddress(address).HandleToReplace = NewHandles.Handles[mapIndex];
//   }
// }

std::vector<uint32_t> getPatchShaderBindingTableShaderSourceCode() {
  std::vector<uint32_t> code = {
      0x07230203, 0x00010000, 0x0008000b, 0x0000008c, 0x00000000, 0x00020011, 0x00000001,
      0x00020011, 0x000014e3, 0x0009000a, 0x5f565053, 0x5f52484b, 0x73796870, 0x6c616369,
      0x6f74735f, 0x65676172, 0x6675625f, 0x00726566, 0x0006000b, 0x00000001, 0x4c534c47,
      0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x000014e4, 0x00000001, 0x0006000f,
      0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x00000025, 0x00060010, 0x00000004,
      0x00000011, 0x00000020, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2,
      0x00070004, 0x455f4c47, 0x625f5458, 0x65666675, 0x65725f72, 0x65726566, 0x0065636e,
      0x00090004, 0x455f4c47, 0x625f5458, 0x65666675, 0x65725f72, 0x65726566, 0x5f65636e,
      0x63657675, 0x00000032, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00090005,
      0x0000000d, 0x636c6163, 0x74616c75, 0x64644165, 0x73736572, 0x32757628, 0x3b31753b,
      0x00000000, 0x00040005, 0x0000000b, 0x65736162, 0x00000000, 0x00040005, 0x0000000c,
      0x7366666f, 0x00007465, 0x00040005, 0x00000013, 0x72726163, 0x00000079, 0x00040005,
      0x00000014, 0x54736552, 0x00657079, 0x00040005, 0x00000022, 0x7366666f, 0x00007465,
      0x00080005, 0x00000025, 0x475f6c67, 0x61626f6c, 0x766e496c, 0x7461636f, 0x496e6f69,
      0x00000044, 0x00060005, 0x0000002a, 0x68737550, 0x736e6f43, 0x746e6174, 0x00000073,
      0x00070006, 0x0000002a, 0x00000000, 0x6769724f, 0x6c616e69, 0x646e6148, 0x0073656c,
      0x00060006, 0x0000002a, 0x00000001, 0x4877654e, 0x6c646e61, 0x00007365, 0x00060006,
      0x0000002a, 0x00000002, 0x65736142, 0x72646441, 0x00737365, 0x00050006, 0x0000002a,
      0x00000003, 0x69727453, 0x00006564, 0x00050006, 0x0000002a, 0x00000004, 0x657a6953,
      0x00000000, 0x00070005, 0x0000002e, 0x64616853, 0x72477265, 0x4870756f, 0x6c646e61,
      0x00000065, 0x00050006, 0x0000002e, 0x00000000, 0x756c6156, 0x00000065, 0x000a0005,
      0x00000030, 0x68737550, 0x736e6f43, 0x746e6174, 0x72646441, 0x4f737365, 0x6e614866,
      0x73656c64, 0x00000000, 0x00050006, 0x00000030, 0x00000000, 0x646e6148, 0x0073656c,
      0x00030005, 0x00000032, 0x00000000, 0x00040005, 0x00000042, 0x72646461, 0x00737365,
      0x00040005, 0x00000044, 0x61726170, 0x0000006d, 0x00040005, 0x00000048, 0x61726170,
      0x0000006d, 0x00050005, 0x0000004b, 0x4970616d, 0x7865646e, 0x00000000, 0x00070005,
      0x0000004e, 0x64616853, 0x72477265, 0x4870756f, 0x6c646e61, 0x00000065, 0x00050006,
      0x0000004e, 0x00000000, 0x756c6156, 0x00000065, 0x00060005, 0x00000050, 0x646e6168,
      0x6f54656c, 0x6c706552, 0x00656361, 0x00070005, 0x00000054, 0x64616853, 0x72477265,
      0x4870756f, 0x6c646e61, 0x00000065, 0x00050006, 0x00000054, 0x00000000, 0x756c6156,
      0x00000065, 0x00060005, 0x00000055, 0x69766544, 0x64416563, 0x73657264, 0x00000073,
      0x00070006, 0x00000055, 0x00000000, 0x646e6148, 0x6f54656c, 0x6c706552, 0x00656361,
      0x00040047, 0x00000025, 0x0000000b, 0x0000001c, 0x00030047, 0x0000002a, 0x00000002,
      0x00050048, 0x0000002a, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000002a,
      0x00000001, 0x00000023, 0x00000008, 0x00050048, 0x0000002a, 0x00000002, 0x00000023,
      0x00000010, 0x00050048, 0x0000002a, 0x00000003, 0x00000023, 0x00000018, 0x00050048,
      0x0000002a, 0x00000004, 0x00000023, 0x0000001c, 0x00040047, 0x0000002d, 0x00000006,
      0x00000010, 0x00050048, 0x0000002e, 0x00000000, 0x00000023, 0x00000000, 0x00040047,
      0x0000002f, 0x00000006, 0x00000020, 0x00030047, 0x00000030, 0x00000002, 0x00040048,
      0x00000030, 0x00000000, 0x00000018, 0x00050048, 0x00000030, 0x00000000, 0x00000023,
      0x00000000, 0x00040047, 0x00000053, 0x00000006, 0x00000010, 0x00050048, 0x00000054,
      0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x00000055, 0x00000002, 0x00050048,
      0x00000055, 0x00000000, 0x00000023, 0x00000000, 0x00040047, 0x0000008b, 0x0000000b,
      0x00000019, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015,
      0x00000006, 0x00000020, 0x00000000, 0x00040017, 0x00000007, 0x00000006, 0x00000002,
      0x00040020, 0x00000008, 0x00000007, 0x00000007, 0x00040020, 0x00000009, 0x00000007,
      0x00000006, 0x00050021, 0x0000000a, 0x00000007, 0x00000008, 0x00000009, 0x0004002b,
      0x00000006, 0x0000000f, 0x00000000, 0x0004001e, 0x00000014, 0x00000006, 0x00000006,
      0x0004002b, 0x00000006, 0x0000001a, 0x00000001, 0x00040017, 0x00000023, 0x00000006,
      0x00000003, 0x00040020, 0x00000024, 0x00000001, 0x00000023, 0x0004003b, 0x00000024,
      0x00000025, 0x00000001, 0x00040020, 0x00000026, 0x00000001, 0x00000006, 0x00030027,
      0x00000029, 0x000014e5, 0x0007001e, 0x0000002a, 0x00000029, 0x00000029, 0x00000007,
      0x00000006, 0x00000006, 0x00040017, 0x0000002b, 0x00000006, 0x00000004, 0x0004002b,
      0x00000006, 0x0000002c, 0x00000002, 0x0004001c, 0x0000002d, 0x0000002b, 0x0000002c,
      0x0003001e, 0x0000002e, 0x0000002d, 0x0003001d, 0x0000002f, 0x0000002e, 0x0003001e,
      0x00000030, 0x0000002f, 0x00040020, 0x00000029, 0x000014e5, 0x00000030, 0x00040020,
      0x00000031, 0x00000009, 0x0000002a, 0x0004003b, 0x00000031, 0x00000032, 0x00000009,
      0x00040015, 0x00000033, 0x00000020, 0x00000001, 0x0004002b, 0x00000033, 0x00000034,
      0x00000003, 0x00040020, 0x00000035, 0x00000009, 0x00000006, 0x0004002b, 0x00000033,
      0x0000003a, 0x00000004, 0x00020014, 0x0000003d, 0x0004002b, 0x00000033, 0x00000043,
      0x00000002, 0x00040020, 0x00000045, 0x00000009, 0x00000007, 0x0003001e, 0x0000004e,
      0x0000002d, 0x00040020, 0x0000004f, 0x00000007, 0x0000004e, 0x00030027, 0x00000052,
      0x000014e5, 0x0004001c, 0x00000053, 0x0000002b, 0x0000002c, 0x0003001e, 0x00000054,
      0x00000053, 0x0003001e, 0x00000055, 0x00000054, 0x00040020, 0x00000052, 0x000014e5,
      0x00000055, 0x0004002b, 0x00000033, 0x00000057, 0x00000000, 0x00040020, 0x00000058,
      0x000014e5, 0x00000054, 0x00040020, 0x0000005c, 0x00000007, 0x0000002d, 0x00040020,
      0x0000005f, 0x00000007, 0x0000002b, 0x0004002b, 0x00000033, 0x00000062, 0x00000001,
      0x00040020, 0x00000065, 0x00000009, 0x00000029, 0x00040020, 0x00000069, 0x000014e5,
      0x0000002e, 0x00040017, 0x00000070, 0x0000003d, 0x00000004, 0x00040020, 0x00000083,
      0x000014e5, 0x00000053, 0x00040020, 0x00000086, 0x000014e5, 0x0000002b, 0x0004002b,
      0x00000006, 0x0000008a, 0x00000020, 0x0006002c, 0x00000023, 0x0000008b, 0x0000008a,
      0x0000001a, 0x0000001a, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
      0x000200f8, 0x00000005, 0x0004003b, 0x00000009, 0x00000022, 0x00000007, 0x0004003b,
      0x00000008, 0x00000042, 0x00000007, 0x0004003b, 0x00000008, 0x00000044, 0x00000007,
      0x0004003b, 0x00000009, 0x00000048, 0x00000007, 0x0004003b, 0x00000009, 0x0000004b,
      0x00000007, 0x0004003b, 0x0000004f, 0x00000050, 0x00000007, 0x00050041, 0x00000026,
      0x00000027, 0x00000025, 0x0000000f, 0x0004003d, 0x00000006, 0x00000028, 0x00000027,
      0x00050041, 0x00000035, 0x00000036, 0x00000032, 0x00000034, 0x0004003d, 0x00000006,
      0x00000037, 0x00000036, 0x00050084, 0x00000006, 0x00000038, 0x00000028, 0x00000037,
      0x0003003e, 0x00000022, 0x00000038, 0x0004003d, 0x00000006, 0x00000039, 0x00000022,
      0x00050041, 0x00000035, 0x0000003b, 0x00000032, 0x0000003a, 0x0004003d, 0x00000006,
      0x0000003c, 0x0000003b, 0x000500ae, 0x0000003d, 0x0000003e, 0x00000039, 0x0000003c,
      0x000300f7, 0x00000040, 0x00000000, 0x000400fa, 0x0000003e, 0x0000003f, 0x00000040,
      0x000200f8, 0x0000003f, 0x000100fd, 0x000200f8, 0x00000040, 0x00050041, 0x00000045,
      0x00000046, 0x00000032, 0x00000043, 0x0004003d, 0x00000007, 0x00000047, 0x00000046,
      0x0003003e, 0x00000044, 0x00000047, 0x0004003d, 0x00000006, 0x00000049, 0x00000022,
      0x0003003e, 0x00000048, 0x00000049, 0x00060039, 0x00000007, 0x0000004a, 0x0000000d,
      0x00000044, 0x00000048, 0x0003003e, 0x00000042, 0x0000004a, 0x00050041, 0x00000026,
      0x0000004c, 0x00000025, 0x0000001a, 0x0004003d, 0x00000006, 0x0000004d, 0x0000004c,
      0x0003003e, 0x0000004b, 0x0000004d, 0x0004003d, 0x00000007, 0x00000051, 0x00000042,
      0x0004007c, 0x00000052, 0x00000056, 0x00000051, 0x00050041, 0x00000058, 0x00000059,
      0x00000056, 0x00000057, 0x0006003d, 0x00000054, 0x0000005a, 0x00000059, 0x00000002,
      0x00000020, 0x00050051, 0x00000053, 0x0000005b, 0x0000005a, 0x00000000, 0x00050041,
      0x0000005c, 0x0000005d, 0x00000050, 0x00000057, 0x00050051, 0x0000002b, 0x0000005e,
      0x0000005b, 0x00000000, 0x00050041, 0x0000005f, 0x00000060, 0x0000005d, 0x00000057,
      0x0003003e, 0x00000060, 0x0000005e, 0x00050051, 0x0000002b, 0x00000061, 0x0000005b,
      0x00000001, 0x00050041, 0x0000005f, 0x00000063, 0x0000005d, 0x00000062, 0x0003003e,
      0x00000063, 0x00000061, 0x0004003d, 0x0000004e, 0x00000064, 0x00000050, 0x00050041,
      0x00000065, 0x00000066, 0x00000032, 0x00000057, 0x0004003d, 0x00000029, 0x00000067,
      0x00000066, 0x0004003d, 0x00000006, 0x00000068, 0x0000004b, 0x00060041, 0x00000069,
      0x0000006a, 0x00000067, 0x00000057, 0x00000068, 0x0006003d, 0x0000002e, 0x0000006b,
      0x0000006a, 0x00000002, 0x00000010, 0x00050051, 0x0000002d, 0x0000006c, 0x00000064,
      0x00000000, 0x00050051, 0x0000002d, 0x0000006d, 0x0000006b, 0x00000000, 0x00050051,
      0x0000002b, 0x0000006e, 0x0000006c, 0x00000000, 0x00050051, 0x0000002b, 0x0000006f,
      0x0000006d, 0x00000000, 0x000500aa, 0x00000070, 0x00000071, 0x0000006e, 0x0000006f,
      0x0004009b, 0x0000003d, 0x00000072, 0x00000071, 0x00050051, 0x0000002b, 0x00000073,
      0x0000006c, 0x00000001, 0x00050051, 0x0000002b, 0x00000074, 0x0000006d, 0x00000001,
      0x000500aa, 0x00000070, 0x00000075, 0x00000073, 0x00000074, 0x0004009b, 0x0000003d,
      0x00000076, 0x00000075, 0x000500a7, 0x0000003d, 0x00000077, 0x00000072, 0x00000076,
      0x000300f7, 0x00000079, 0x00000000, 0x000400fa, 0x00000077, 0x00000078, 0x00000079,
      0x000200f8, 0x00000078, 0x0004003d, 0x00000007, 0x0000007a, 0x00000042, 0x0004007c,
      0x00000052, 0x0000007b, 0x0000007a, 0x00050041, 0x00000065, 0x0000007c, 0x00000032,
      0x00000062, 0x0004003d, 0x00000029, 0x0000007d, 0x0000007c, 0x0004003d, 0x00000006,
      0x0000007e, 0x0000004b, 0x00060041, 0x00000069, 0x0000007f, 0x0000007d, 0x00000057,
      0x0000007e, 0x0006003d, 0x0000002e, 0x00000080, 0x0000007f, 0x00000002, 0x00000010,
      0x00050041, 0x00000058, 0x00000081, 0x0000007b, 0x00000057, 0x00050051, 0x0000002d,
      0x00000082, 0x00000080, 0x00000000, 0x00050041, 0x00000083, 0x00000084, 0x00000081,
      0x00000057, 0x00050051, 0x0000002b, 0x00000085, 0x00000082, 0x00000000, 0x00050041,
      0x00000086, 0x00000087, 0x00000084, 0x00000057, 0x0005003e, 0x00000087, 0x00000085,
      0x00000002, 0x00000010, 0x00050051, 0x0000002b, 0x00000088, 0x00000082, 0x00000001,
      0x00050041, 0x00000086, 0x00000089, 0x00000084, 0x00000062, 0x0005003e, 0x00000089,
      0x00000088, 0x00000002, 0x00000010, 0x000200f9, 0x00000079, 0x000200f8, 0x00000079,
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

VkDeviceAddress getBufferDeviceAddress(const VkDeviceLevelDispatchTable& dt,
                                       VkDevice device,
                                       VkBuffer buffer) {
  auto vkGetBufferDeviceAddress =
      dt.vkGetBufferDeviceAddress ? dt.vkGetBufferDeviceAddress : dt.vkGetBufferDeviceAddressKHR;

  VkBufferDeviceAddressInfo addressInfo = {
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType    sType;
      nullptr,                                      // const void*        pNext;
      buffer                                        // VkBuffer           buffer;
  };
  return vkGetBufferDeviceAddress(device, &addressInfo);
}

} // namespace

void RayTracingReplayService::OnPreCreateDevice(vkCreateDeviceCommand& command) {
  // Don't use capture/replay features (if available) together with shader group handles patching
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles ||
      !command.m_pCreateInfo.Value || !command.m_pCreateInfo.Value->pNext) {
    return;
  }
  auto* rayTracingPipelineFeatures =
      (VkPhysicalDeviceRayTracingPipelineFeaturesKHR*)getPNextStructure(
          command.m_pCreateInfo.Value->pNext,
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);

  if (rayTracingPipelineFeatures != nullptr) {
    rayTracingPipelineFeatures->rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
  }
}

void RayTracingReplayService::OnPostCreateDevice(vkCreateDeviceCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  auto device = *command.m_pDevice.Value;
  auto& deviceData = m_DevicesData[device];

  deviceData.m_PhysicalDevice = command.m_physicalDevice.Value;
  deviceData.m_ComputePipeline = VK_NULL_HANDLE;
  deviceData.m_Layout = VK_NULL_HANDLE;
}

void RayTracingReplayService::OnPreDestroyDevice(vkDestroyDeviceCommand& command) {
  auto device = command.m_device.Value;
  auto& deviceData = m_DevicesData[device];

  if (deviceData.m_ComputePipeline != VK_NULL_HANDLE) {
    auto& dt = m_Manager.GetDeviceDispatchTable(device);
    dt.vkDestroyPipeline(device, deviceData.m_ComputePipeline, nullptr);
    dt.vkDestroyPipelineLayout(device, deviceData.m_Layout, nullptr);
  }

  for (auto& cmdPoolPair : deviceData.m_CommandBuffers) {
    auto commandPool = cmdPoolPair;

    for (auto cmdBuf : cmdPoolPair.second) {
      m_CommandBuffersData.erase(cmdBuf);
    }
  }

  m_DevicesData.erase(device);
}

void RayTracingReplayService::OnPostAllocateCommandBuffers(
    vkAllocateCommandBuffersCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles ||
      !command.m_pAllocateInfo.Value || !command.m_pCommandBuffers.Value) {
    return;
  }

  const auto device = command.m_device.Value;
  auto& deviceData = m_DevicesData[device];

  for (uint32_t i = 0; i < command.m_pAllocateInfo.Value->commandBufferCount; ++i) {
    auto cmdBuf = command.m_pCommandBuffers.Value[i];
    if (cmdBuf == VK_NULL_HANDLE) {
      continue;
    }

    m_CommandBuffersData[cmdBuf].m_Device = device;
    deviceData.m_CommandBuffers[command.m_pAllocateInfo.Value->commandPool].insert(cmdBuf);
  }
}

void RayTracingReplayService::OnPreFreeCommandBuffers(vkFreeCommandBuffersCommand& command) {
  for (uint32_t i = 0; i < command.m_commandBufferCount.Value; ++i) {
    m_CommandBuffersData.erase(command.m_pCommandBuffers.Value[i]);
  }
}

void RayTracingReplayService::OnPreDestroyCommandPool(vkDestroyCommandPoolCommand& command) {
  auto& deviceData = m_DevicesData[command.m_device.Value];
  const auto commandPool = command.m_commandPool.Value;

  for (auto cmdBuf : deviceData.m_CommandBuffers[commandPool]) {
    m_CommandBuffersData.erase(cmdBuf);
  }
  deviceData.m_CommandBuffers.erase(commandPool);
}

void RayTracingReplayService::OnPostBindPipeline(vkCmdBindPipelineCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  auto& cmdBufData = m_CommandBuffersData[command.m_commandBuffer.Value];

  cmdBufData.m_BindPoint = command.m_pipelineBindPoint.Value;
  cmdBufData.m_Pipeline = command.m_pipeline.Value;
  cmdBufData.m_PipelineKey = command.m_pipeline.Key;
}

void RayTracingReplayService::OnPostPushConstants(vkCmdPushConstantsCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  auto& cmdBufData = m_CommandBuffersData[command.m_commandBuffer.Value];

  const auto layout = command.m_layout.Value;
  const auto stageFlags = command.m_stageFlags.Value;
  const auto size = command.m_size.Value;
  const auto offset = command.m_offset.Value;
  const auto* pSrc = command.m_pValues.Value;

  cmdBufData.m_PushConstants = {
      layout,     // VkPipelineLayout m_Layout;
      stageFlags, // VkShaderStageFlags m_StageFlags;
      offset,     // uint32_t m_Offset;
      size,       // uint32_t m_Size;
      {}          // std::vector<uint8_t> m_Data;
  };
  cmdBufData.m_PushConstants.m_Data.resize(size);
  memcpy(cmdBufData.m_PushConstants.m_Data.data(), pSrc, size);
}

void RayTracingReplayService::OnPreCreateRayTracingPipelines(
    vkCreateRayTracingPipelinesKHRCommand& command) {

  // Stop using per pipeline capture/replay features when SBT patching is enabled
  if (Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    const auto device = command.m_device.Value;
    auto& deviceData = m_DevicesData[device];
    if (deviceData.m_ComputePipeline == VK_NULL_HANDLE) {
      deviceData.m_Layout = CreatePipelineLayout(device);
      deviceData.m_ComputePipeline = CreateComputePipeline(device, deviceData.m_Layout);
    }

    for (uint32_t ci = 0; ci < command.m_createInfoCount.Value; ci++) {
      auto* pCreateInfo = &command.m_pCreateInfos.Value[ci];
      pCreateInfo->flags &=
          ~VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR;

      for (uint32_t g = 0; g < pCreateInfo->groupCount; ++g) {
        auto* pGroup = const_cast<VkRayTracingShaderGroupCreateInfoKHR*>(&pCreateInfo->pGroups[g]);
        pGroup->pShaderGroupCaptureReplayHandle = nullptr;
      }
    }

    return;
  }

  if (!command.m_pCreateInfos.CaptureReplayHandleSize ||
      command.m_pCreateInfos.CaptureReplayHandlesData.empty()) {
    return;
  }

  // Assign capture/replay handles when capture/replay feature is used (and patching is disabled)
  uint8_t* ptr = command.m_pCreateInfos.CaptureReplayHandlesData.data();

  for (uint32_t i = 0; i < command.m_createInfoCount.Value; ++i) {
    auto& createInfo = command.m_pCreateInfos.Value[i];

    for (uint32_t g = 0; g < createInfo.groupCount; ++g) {
      auto& group = const_cast<VkRayTracingShaderGroupCreateInfoKHR&>(createInfo.pGroups[g]);
      group.pShaderGroupCaptureReplayHandle = ptr;
      ptr += command.m_pCreateInfos.CaptureReplayHandleSize;
    }
  }
}

void RayTracingReplayService::OnPostCreateRayTracingPipelines(
    vkCreateRayTracingPipelinesKHRCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles ||
      !command.m_pCreateInfos.Value) {
    return;
  }

  auto& groupCountsMap = m_DevicesData[command.m_device.Value].m_GroupCounts;

  // Prepare data for SBT patching
  for (uint32_t ci = 0; ci < command.m_createInfoCount.Value; ci++) {
    auto pipeline = command.m_pPipelines.Value[ci];
    if (pipeline == VK_NULL_HANDLE) {
      continue;
    }

    auto* pCreateInfo = &command.m_pCreateInfos.Value[ci];
    auto totalGroupCount = pCreateInfo->groupCount;

    if (pCreateInfo->pLibraryInfo && pCreateInfo->pLibraryInfo->pLibraries) {
      for (uint32_t l = 0; l < pCreateInfo->pLibraryInfo->libraryCount; l++) {
        auto library = pCreateInfo->pLibraryInfo->pLibraries[l];
        auto it = groupCountsMap.find(library);
        if (it == groupCountsMap.end()) {
          continue;
        }

        totalGroupCount += it->second;
      }
    }

    groupCountsMap[pipeline] = totalGroupCount;
    auto totalSize = s_ShaderGroupHandleSize * totalGroupCount;
    m_PipelinesData[command.m_pPipelines.Keys[ci]] = {
        totalGroupCount,                    // uint32_t m_TotalGroupCount;
        std::vector<uint8_t>(totalSize),    // std::vector<uint8_t> m_OriginalHandles;
        std::vector<uint8_t>(totalSize),    // std::vector<uint8_t> m_ReplayHandles;
        std::vector<bool>(totalGroupCount), // std::vector<bool> m_LoadedGroups;
        false,                              // bool m_PatchingRequired;
        false,                              // bool m_AlreadyProcessed;
        {
            VK_NULL_HANDLE, // VkDeviceMemory m_Memory;
            VK_NULL_HANDLE, // VkBuffer m_Buffer;
            0,              // VkDeviceSize m_Size;
            0               // VkDeviceAddress m_DeviceAddress;
        }};
  }
}

void RayTracingReplayService::OnPostDestroyPipeline(vkDestroyPipelineCommand& command) {
  auto device = command.m_device.Value;
  auto& pipeline = command.m_pipeline;
  auto& dt = m_Manager.GetDeviceDispatchTable(device);

  m_DevicesData[device].m_GroupCounts.erase(pipeline.Value);

  auto& bufferData = m_PipelinesData[pipeline.Key].m_Buffer;
  if (bufferData.m_Buffer != VK_NULL_HANDLE) {
    dt.vkDestroyBuffer(device, bufferData.m_Buffer, nullptr);
  }
  if (bufferData.m_Memory != VK_NULL_HANDLE) {
    dt.vkFreeMemory(device, bufferData.m_Memory, nullptr);
  }
  m_PipelinesData.erase(pipeline.Key);
}

void RayTracingReplayService::OnPreGetShaderGroupHandles(
    vkGetRayTracingShaderGroupHandlesKHRCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    tl_ShaderGroupHandles.resize(command.m_dataSize.Value);
    memcpy(tl_ShaderGroupHandles.data(), command.m_pData.Value, command.m_dataSize.Value);
    return;
  }

  PipelineData* state;
  {
    auto it = m_PipelinesData.find(command.m_pipeline.Key);
    if ((it == m_PipelinesData.end()) || it->second.m_AlreadyProcessed) {
      return;
    }
    state = &it->second;
  }

  CopyHandles(command, state->m_TotalGroupCount,
              state->m_OriginalHandles.data() +
                  command.m_firstGroup.Value * s_ShaderGroupHandleSize);
}

void RayTracingReplayService::OnPostGetShaderGroupHandles(
    vkGetRayTracingShaderGroupHandlesKHRCommand& command) {
  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }

  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    if (memcmp(tl_ShaderGroupHandles.data(), command.m_pData.Value, command.m_dataSize.Value) !=
        0) {
      LOG_WARNING << "Shader group handles for pipeline O" << command.m_pipeline.Key
                  << " changed between stream recording and replay. This may lead to rendering "
                     "glitches or replay crash when SBT patching is disabled.";
    }
    return;
  }

  PipelineData* pipelineData;
  {
    auto it = m_PipelinesData.find(command.m_pipeline.Key);
    if ((it == m_PipelinesData.end()) || it->second.m_AlreadyProcessed) {
      return;
    }
    pipelineData = &it->second;
  }

  if (!CopyHandles(command, pipelineData->m_TotalGroupCount,
                   pipelineData->m_ReplayHandles.data() +
                       command.m_firstGroup.Value * s_ShaderGroupHandleSize)) {
    return;
  }

  for (uint32_t i = command.m_firstGroup.Value;
       i < command.m_firstGroup.Value + command.m_groupCount.Value; ++i) {
    pipelineData->m_LoadedGroups[i] = true;
  }

  for (auto groupLoaded : pipelineData->m_LoadedGroups) {
    if (!groupLoaded) {
      // Not all group handles were loaded so far, so don't prepare patching data yet.
      return;
    }
  }

  // All shader group handles are now loaded.
  // GITS can now prepare data for SBT patching.
  pipelineData->m_AlreadyProcessed = true;

  if (memcmp(pipelineData->m_OriginalHandles.data(), pipelineData->m_ReplayHandles.data(),
             pipelineData->m_OriginalHandles.size()) == 0) {
    return;
  }

  pipelineData->m_PatchingRequired = true;

  auto device = command.m_device.Value;
  auto size = pipelineData->m_OriginalHandles.size();
  auto& deviceData = m_DevicesData[device];
  pipelineData->m_Buffer = CreateBuffer(device, deviceData.m_PhysicalDevice, size * 2);
  const auto& dt = m_Manager.GetDeviceDispatchTable(device);

  uint8_t* mappedMemoryPtr = nullptr;
  {
    VkResult result = dt.vkMapMemory(device, pipelineData->m_Buffer.m_Memory, 0, VK_WHOLE_SIZE, 0,
                                     (void**)&mappedMemoryPtr);
    if ((result != VK_SUCCESS) || !mappedMemoryPtr) {
      throw std::runtime_error("Could not map a memory for shader group handles!");
    }
  }

  // Buffer used to patch shader group handles contains original and current handles

  // Copy original shader group handles passed from a recorder
  memcpy(mappedMemoryPtr, pipelineData->m_OriginalHandles.data(),
         pipelineData->m_OriginalHandles.size());
  // Copy current shader group handles acquired from a driver
  memcpy(mappedMemoryPtr + pipelineData->m_OriginalHandles.size(),
         pipelineData->m_ReplayHandles.data(), pipelineData->m_ReplayHandles.size());

  VkMappedMemoryRange range = {
      VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType sType;
      nullptr,                               // const void* pNext;
      pipelineData->m_Buffer.m_Memory,       // VkDeviceMemory memory;
      0,                                     // VkDeviceSize offset;
      VK_WHOLE_SIZE                          // VkDeviceSize size;
  };
  dt.vkFlushMappedMemoryRanges(device, 1, &range);
  dt.vkUnmapMemory(device, pipelineData->m_Buffer.m_Memory);
}

void RayTracingReplayService::OnPreTraceRays(vkCmdTraceRaysKHRCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  const auto cmdBuf = command.m_commandBuffer.Value;
  const auto& cmdBufData = m_CommandBuffersData[cmdBuf];
  const auto& pipelineData = m_PipelinesData[cmdBufData.m_PipelineKey];

  if (!pipelineData.m_PatchingRequired) {
    return;
  }

  const VkStridedDeviceAddressRegionKHR* pRaygenSBT = command.m_pRaygenShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pMissSBT = command.m_pMissShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pHitSBT = command.m_pHitShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pCallableSBT = command.m_pCallableShaderBindingTable.Value;

  const auto originalHandlesAddress = pipelineData.m_Buffer.m_DeviceAddress;
  const auto newHandlesAddress = originalHandlesAddress + pipelineData.m_OriginalHandles.size();

  PatchSBT(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, originalHandlesAddress,
           newHandlesAddress, pipelineData.m_TotalGroupCount);
}

void RayTracingReplayService::OnPostTraceRays(vkCmdTraceRaysKHRCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  const auto cmdBuf = command.m_commandBuffer.Value;
  const auto& cmdBufData = m_CommandBuffersData[cmdBuf];
  const auto& pipelineData = m_PipelinesData[cmdBufData.m_PipelineKey];

  if (!pipelineData.m_PatchingRequired) {
    return;
  }

  const VkStridedDeviceAddressRegionKHR* pRaygenSBT = command.m_pRaygenShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pMissSBT = command.m_pMissShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pHitSBT = command.m_pHitShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pCallableSBT = command.m_pCallableShaderBindingTable.Value;

  const auto originalHandlesAddress = pipelineData.m_Buffer.m_DeviceAddress;
  const auto newHandlesAddress = originalHandlesAddress + pipelineData.m_OriginalHandles.size();

  PatchSBT(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, newHandlesAddress,
           originalHandlesAddress, pipelineData.m_TotalGroupCount);
}

void RayTracingReplayService::OnPreTraceRaysIndirect(vkCmdTraceRaysIndirectKHRCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  const auto cmdBuf = command.m_commandBuffer.Value;
  const auto& cmdBufData = m_CommandBuffersData[cmdBuf];
  const auto& pipelineData = m_PipelinesData[cmdBufData.m_PipelineKey];

  if (!pipelineData.m_PatchingRequired) {
    return;
  }

  const VkStridedDeviceAddressRegionKHR* pRaygenSBT = command.m_pRaygenShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pMissSBT = command.m_pMissShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pHitSBT = command.m_pHitShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pCallableSBT = command.m_pCallableShaderBindingTable.Value;

  const auto originalHandlesAddress = pipelineData.m_Buffer.m_DeviceAddress;
  const auto newHandlesAddress = originalHandlesAddress + pipelineData.m_OriginalHandles.size();

  PatchSBT(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, originalHandlesAddress,
           newHandlesAddress, pipelineData.m_TotalGroupCount);
}

void RayTracingReplayService::OnPostTraceRaysIndirect(vkCmdTraceRaysIndirectKHRCommand& command) {
  if (!Configurator::Get().vulkan.player.portability.patchShaderGroupHandles) {
    return;
  }

  const auto cmdBuf = command.m_commandBuffer.Value;
  const auto& cmdBufData = m_CommandBuffersData[cmdBuf];
  const auto& pipelineData = m_PipelinesData[cmdBufData.m_PipelineKey];

  if (!pipelineData.m_PatchingRequired) {
    return;
  }

  const VkStridedDeviceAddressRegionKHR* pRaygenSBT = command.m_pRaygenShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pMissSBT = command.m_pMissShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pHitSBT = command.m_pHitShaderBindingTable.Value;
  const VkStridedDeviceAddressRegionKHR* pCallableSBT = command.m_pCallableShaderBindingTable.Value;

  const auto originalHandlesAddress = pipelineData.m_Buffer.m_DeviceAddress;
  const auto newHandlesAddress = originalHandlesAddress + pipelineData.m_OriginalHandles.size();

  PatchSBT(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, newHandlesAddress,
           originalHandlesAddress, pipelineData.m_TotalGroupCount);
}

VkPipelineLayout RayTracingReplayService::CreatePipelineLayout(VkDevice device) {
  VkPushConstantRange pushConstantsRange = {
      VK_SHADER_STAGE_COMPUTE_BIT, // VkShaderStageFlags stageFlags
      0,                           // uint32_t           offset
      4 * sizeof(VkDeviceAddress)  // uint32_t           size
  };

  VkPipelineLayoutCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // VkStructureType               sType
      nullptr,                                       // const void                  * pNext
      0,                                             // VkPipelineLayoutCreateFlags   flags
      0,                                             // uint32_t                      setLayoutCount
      nullptr,                                       // const VkDescriptorSetLayout * pSetLayouts
      1,                  // uint32_t                      pushConstantRangeCount
      &pushConstantsRange // const VkPushConstantRange   * pPushConstantRanges
  };

  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkResult result = m_Manager.GetDeviceDispatchTable(device).vkCreatePipelineLayout(
      device, &createInfo, nullptr, &layout);
  if ((result != VK_SUCCESS) || (layout == VK_NULL_HANDLE)) {
    throw std::runtime_error("Could not create a pipeline layout used by injected pipelines. "
                             "Retry recording or use capture/replay features instead.");
  }

  return layout;
}

VkPipeline RayTracingReplayService::CreateComputePipeline(VkDevice device,
                                                          VkPipelineLayout layout) {
  const auto& dt = m_Manager.GetDeviceDispatchTable(device);

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  {
    const auto code = getPatchShaderBindingTableShaderSourceCode();
    auto size = code.size() * sizeof(uint32_t);

    VkShaderModuleCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, // VkStructureType             sType
        nullptr,                                     // const void                * pNext
        0,                                           // VkShaderModuleCreateFlags   flags
        size,                                        // size_t                      codeSize
        code.data()                                  // const uint32_t            * pCode
    };

    VkResult result = dt.vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if ((result != VK_SUCCESS) || (shaderModule == VK_NULL_HANDLE)) {
      throw std::runtime_error("Could not create a shader module for an injected pipeline. Retry "
                               "recording or use capture/replay features instead.");
    }
  }

  VkPipeline pipeline = VK_NULL_HANDLE;
  {
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType sType
        nullptr,                     // const void                       * pNext
        0,                           // VkPipelineShaderStageCreateFlags   flags
        VK_SHADER_STAGE_COMPUTE_BIT, // VkShaderStageFlagBits              stage
        shaderModule,                // VkShaderModule                     module
        "main",                      // const char                       * pName
        nullptr                      // const VkSpecializationInfo       * pSpecializationInfo
    };

    VkComputePipelineCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, // VkStructureType                   sType
        nullptr,                                        // const void                      * pNext
        0,                                              // VkPipelineCreateFlags             flags
        shaderStageCreateInfo,                          // VkPipelineShaderStageCreateInfo   stage
        layout,                                         // VkPipelineLayout                  layout
        VK_NULL_HANDLE, // VkPipeline                        basePipelineHandle
        -1              // int32_t                           basePipelineIndex
    };

    VkResult result =
        dt.vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
    if ((result != VK_SUCCESS) || (pipeline == VK_NULL_HANDLE)) {
      throw std::runtime_error(
          "Could not create a compute pipeline for Shader Binding Table patching.");
    }
  }

  dt.vkDestroyShaderModule(device, shaderModule, nullptr);

  return pipeline;
}

RayTracingReplayService::BufferData RayTracingReplayService::CreateBuffer(
    VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size) {
  const auto& dt = m_Manager.GetDeviceDispatchTable(device);

  VkBuffer buffer = VK_NULL_HANDLE;
  {
    VkBufferCreateInfo bufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,      // VkStructureType        sType;
        nullptr,                                   // const void           * pNext;
        0,                                         // VkBufferCreateFlags    flags;
        size,                                      // VkDeviceSize           size;
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // VkBufferUsageFlags     usage;
        VK_SHARING_MODE_EXCLUSIVE,                 // VkSharingMode          sharingMode;
        0,                                         // uint32_t               queueFamilyIndexCount;
        nullptr                                    // const uint32_t       * pQueueFamilyIndices;
    };
    VkResult result = dt.vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    if ((result != VK_SUCCESS) || (buffer == VK_NULL_HANDLE)) {
      throw std::runtime_error("Could not create a buffer for shader group handles!");
    }
  }

  VkMemoryRequirements bufferMemoryRequirements;
  dt.vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);

  VkDeviceMemory memory = VK_NULL_HANDLE;
  {
    VkMemoryAllocateFlagsInfo memoryAllocateFlags = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, // VkStructureType          sType;
        nullptr,                                      // const void             * pNext;
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,        // VkMemoryAllocateFlags    flags;
        1                                             // uint32_t                 deviceMask;
    };

    VkMemoryAllocateInfo memoryAllocateInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // VkStructureType    sType
        &memoryAllocateFlags,                   // const void       * pNext
        bufferMemoryRequirements.size,          // VkDeviceSize       allocationSize
        0                                       // uint32_t           memoryTypeIndex
    };

    VkPhysicalDeviceMemoryProperties memoryProperties{};
    m_Manager.GetInstanceDispatchTable(physicalDevice)
        .vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    // Find appropriate memory type index
    for (uint32_t type = 0; type < memoryProperties.memoryTypeCount; ++type) {
      if (isBitSet(bufferMemoryRequirements.memoryTypeBits, 1 << type) &&
          isBitSet(memoryProperties.memoryTypes[type].propertyFlags,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        memoryAllocateInfo.memoryTypeIndex = type;

        VkResult result = dt.vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);
        if (VK_SUCCESS == result) {
          break;
        }
      }
    }
    if (memory == VK_NULL_HANDLE) {
      throw std::runtime_error("Could not allocate memory for a buffer.");
    }
  }

  // Bind memory to the buffer
  if (VK_SUCCESS != dt.vkBindBufferMemory(device, buffer, memory, 0)) {
    throw std::runtime_error("Could not bind a memory object to a temporary buffer.");
  }

  return BufferData{
      memory,                                    // VkDeviceMemory m_Memory
      buffer,                                    // VkBuffer m_Buffer
      bufferMemoryRequirements.size,             // VkDeviceSize m_Size
      getBufferDeviceAddress(dt, device, buffer) // VkDeviceAddress m_DeviceAddress
  };
}

bool RayTracingReplayService::CopyHandles(vkGetRayTracingShaderGroupHandlesKHRCommand& command,
                                          uint32_t totalGroupCount,
                                          uint8_t* target) {
  auto device = command.m_device.Value;
  auto pipeline = command.m_pipeline.Value;
  auto groupCount = command.m_groupCount.Value;
  auto dataSize = command.m_dataSize.Value;
  auto* pData = command.m_pData.Value;

  if (!device || !pipeline || !groupCount || !dataSize || !pData) {
    return false;
  }

  auto copySize = groupCount * s_ShaderGroupHandleSize;
  if (dataSize < copySize) {
    LOG_WARNING << "Buffer size for ray tracing shader group handles is too small!";
  }

  if (command.m_firstGroup.Value + groupCount > totalGroupCount) {
    LOG_WARNING << "App tries to copy shader group handles for non-existent groups!";
    return false;
  }

  memcpy(target, pData, copySize);
  return true;
}

void RayTracingReplayService::PatchSBT(VkCommandBuffer cmdBuf,
                                       const VkStridedDeviceAddressRegionKHR* pRaygenSBT,
                                       const VkStridedDeviceAddressRegionKHR* pMissSBT,
                                       const VkStridedDeviceAddressRegionKHR* pHitSBT,
                                       const VkStridedDeviceAddressRegionKHR* pCallableSBT,
                                       VkDeviceAddress oldHandlesMap,
                                       VkDeviceAddress newHandlesMap,
                                       uint32_t handlesMapEntriesCount) {
  const auto& cmdBufData = m_CommandBuffersData[cmdBuf];
  const auto device = cmdBufData.m_Device;
  const auto& dt = m_Manager.GetDeviceDispatchTable(device);
  const auto& deviceData = m_DevicesData[device];

  // Inject compute - patch shader group handles in SBT with current values
  {
    VkMemoryBarrier barrierPre = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,                      // VkStructureType sType;
        nullptr,                                               // const void* pNext;
        VK_ACCESS_MEMORY_WRITE_BIT,                            // VkAccessFlags srcAccessMask;
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT // VkAccessFlags dstAccessMask;
    };
    dt.vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrierPre, 0, nullptr, 0,
                            nullptr);
    dt.vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, deviceData.m_ComputePipeline);

    auto dispatchComputeShader = [&](const VkStridedDeviceAddressRegionKHR* pSBT) {
      if (pSBT && pSBT->deviceAddress && pSBT->size) {
        auto stride = pSBT->stride > 0 ? pSBT->stride : pSBT->size;
        struct PushConstantsData {
          VkDeviceAddress OldHandlesMap;
          VkDeviceAddress NewHandlesMap;
          VkDeviceAddress SBTBaseAddress;
          uint32_t Stride;
          uint32_t Size;
        } pushConstants = {
            oldHandlesMap,       // VkDeviceAddress OldHandlesMap;
            newHandlesMap,       // VkDeviceAddress NewHandlesMap;
            pSBT->deviceAddress, // VkDeviceAddress SBTBaseAddress;
            (uint32_t)stride,    // uint32_t Stride;
            (uint32_t)pSBT->size // uint32_t Size;
        };
        // 32 is a performance optimization - 32 local invocations of a compute shader are dispatched
        uint32_t divisor = stride * 32;
        uint32_t invocationsCount = (pSBT->size / divisor) + ((pSBT->size % divisor > 0) ? 1 : 0);

        dt.vkCmdPushConstants(cmdBuf, deviceData.m_Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                              sizeof(PushConstantsData), &pushConstants);
        dt.vkCmdDispatch(cmdBuf, invocationsCount, handlesMapEntriesCount, 1);
      }
    };

    dispatchComputeShader(pRaygenSBT);
    dispatchComputeShader(pMissSBT);
    dispatchComputeShader(pHitSBT);
    dispatchComputeShader(pCallableSBT);

    VkMemoryBarrier barrierPost = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,                       // VkStructureType sType;
        nullptr,                                                // const void* pNext;
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // VkAccessFlags srcAccessMask;
        VK_ACCESS_SHADER_READ_BIT                               // VkAccessFlags dstAccessMask;
    };
    dt.vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &barrierPost, 0,
                            nullptr, 0, nullptr);
  }

  // Restore original push constants data
  if (cmdBufData.m_Pipeline != VK_NULL_HANDLE) {
    dt.vkCmdBindPipeline(cmdBuf, cmdBufData.m_BindPoint, cmdBufData.m_Pipeline);
    if (cmdBufData.m_PushConstants.m_Data.size()) {
      auto& pushConstants = cmdBufData.m_PushConstants;
      dt.vkCmdPushConstants(cmdBuf, pushConstants.m_Layout, pushConstants.m_StageFlags,
                            pushConstants.m_Offset, pushConstants.m_Data.size(),
                            pushConstants.m_Data.data());
    }
  }
}

} // namespace vulkan
} // namespace gits
