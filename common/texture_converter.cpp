// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "texture_converter.h"
#include "exception.h"
#include "platform.h"
#include "log.h"
#include <map>
#include <algorithm>

namespace gits {
static const std::map<texel_type, std::string> texel_type_string = {
    {texel_type::A8, "A8"},
    {texel_type::R8, "R8"},
    {texel_type::R8snorm, "R8snorm"},
    {texel_type::R8ui, "R8ui"},
    {texel_type::R8i, "R8i"},
    {texel_type::R16, "R16"},
    {texel_type::R16snorm, "R16snorm"},
    {texel_type::R16ui, "R16ui"},
    {texel_type::R16i, "R16i"},
    {texel_type::R16f, "R16f"},
    {texel_type::RG8, "RG8"},
    {texel_type::RG8snorm, "RG8snorm"},
    {texel_type::RG8ui, "RG8ui"},
    {texel_type::RG8i, "RG8i"},
    {texel_type::B5G6R5, "B5G6R5"},
    {texel_type::A1BGR5, "A1BGR5"},
    {texel_type::A1RGB5, "A1RGB5"},
    {texel_type::ABGR4, "ABGR4"},
    {texel_type::BGR5A1, "BGR5A1"},
    {texel_type::R32ui, "R32ui"},
    {texel_type::R32i, "R32i"},
    {texel_type::R32f, "R32f"},
    {texel_type::RG16, "RG16"},
    {texel_type::RG16snorm, "RG16snorm"},
    {texel_type::RG16ui, "RG16ui"},
    {texel_type::RG16i, "RG16i"},
    {texel_type::RG16f, "RG16f"},
    {texel_type::BGR8, "BGR8"},
    {texel_type::BGR8i, "BGR8i"},
    {texel_type::BGR8ui, "BGR8ui"},
    {texel_type::BGR8snorm, "BGR8snorm"},
    {texel_type::RGB8, "RGB8"},
    {texel_type::RGB8i, "RGB8i"},
    {texel_type::RGB8ui, "RGB8ui"},
    {texel_type::RGB8snorm, "RGB8snorm"},
    {texel_type::RGBA8, "RGBA8"},
    {texel_type::RGBA8snorm, "RGBA8snorm"},
    {texel_type::RGBA8ui, "RGBA8ui"},
    {texel_type::RGBA8i, "RGBA8i"},
    {texel_type::BGRA8, "BGRA8"},
    {texel_type::BGRA8i, "BGRA8i"},
    {texel_type::BGRA8snorm, "BGRA8snorm"},
    {texel_type::BGRA8ui, "BGRA8ui"},
    {texel_type::ABGR8, "ABGR8"},
    {texel_type::ABGR8i, "ABGR8i"},
    {texel_type::ABGR8snorm, "ABGR8snorm"},
    {texel_type::ABGR8ui, "ABGR8ui"},
    {texel_type::RGB10A2, "RGB10A2"},
    {texel_type::RGB10A2ui, "RGB10A2ui"},
    {texel_type::BGR10A2, "BGR10A2"},
    {texel_type::RG11B10f, "RG11B10f"},
    {texel_type::B10GR11f, "B10GR11f"},
    {texel_type::RG32ui, "RG32ui"},
    {texel_type::RG32i, "RG32i"},
    {texel_type::RG32f, "RG32f"},
    {texel_type::RGBA16, "RGBA16"},
    {texel_type::RGBA16snorm, "RGBA16snorm"},
    {texel_type::RGBA16ui, "RGBA16ui"},
    {texel_type::RGBA16i, "RGBA16i"},
    {texel_type::RGBA16f, "RGBA16f"},
    {texel_type::RGBA32ui, "RGBA32ui"},
    {texel_type::RGBA32i, "RGBA32i"},
    {texel_type::RGBA32f, "RGBA32f"},
    {texel_type::X8D24, "X8D24"},
    {texel_type::D24, "D24"},
    {texel_type::D32fS8ui, "D32fS8ui"}};

const char* get_texel_format_string(texel_type val) {
  if (auto it = texel_type_string.find(val); it != texel_type_string.end()) {
    return it->second.c_str();
  }
  throw std::runtime_error("cannot find string value for a given texel type");
}
int get_supported_texels_count() {
  return static_cast<int>(texel_type_string.size());
}

namespace texture_converter {

template <typename T,
          bool FP,
          bool Signed,
          int Bits,
          bool Normalized = false,
          int64_t Min = 0,
          int64_t Max = 0,
          int One = 1,
          int64_t MaxUint = Max,
          typename MinMaxT = T>
struct component_format {
  using type = T;
  static const bool floating_point = FP;
  static const bool signed_value = Signed;
  static const int bit_count = Bits;
  static const bool normalized = Normalized;
  static const int64_t min_value = Min;
  static const int64_t max_value = Max;
  static const int one_value = One;
  static const int64_t max_uint_value = MaxUint;
  using min_max_type = MinMaxT;
};

using unorm_8 = component_format<uint8_t, false, false, 8, true, 0, 255, 255>;
using snorm_8 = component_format<int8_t, false, true, 8, true, -128, 127, 127>;
using uint_8 = component_format<uint8_t, false, false, 8, false, 0, 255>;
using int_8 = component_format<int8_t, false, true, 8, false, -128, 127>;
using unorm_16 = component_format<uint16_t, false, false, 16, true, 0, 65535, 65535>;
using snorm_16 = component_format<int16_t, false, true, 16, true, -32768, 32767, 32767>;
using uint_16 = component_format<uint16_t, false, false, 16, false, 0, 65535>;
using int_16 = component_format<int16_t, false, true, 16, false, -32768, 32767>;
using unorm_24 = component_format<uint32_t, false, false, 24, true, 0, 16777215, 16777215>;
using uint_32 = component_format<uint32_t, false, false, 32, false, 0, 4294967295>;
using int_32 = component_format<int32_t, false, true, 32, false, -2147483648LL, 2147483647>;
using float_32 = component_format<float, true, true, 32, false, INT64_MIN, INT64_MAX>;
using float_16 = component_format<uint16_t, true, true, 16, false, -65504, 65504, 1, 65504, float>;
using float_11 = component_format<uint32_t, true, false, 11, false, 0, 65024, 0x3c0, 2047>;
using float_10 = component_format<uint32_t, true, false, 10, false, 0, 64512, 0x1e0, 1023>;
using unorm_2 = component_format<uint32_t, false, false, 2, true, 0, 3, 3>;
using unorm_10 = component_format<uint32_t, false, false, 10, true, 0, 1023, 1023>;
using uint_2 = component_format<uint32_t, false, false, 2, false, 0, 3>;
using uint_10 = component_format<uint32_t, false, false, 10, false, 0, 1023>;

template <typename InT,
          bool InFP,
          bool InSgn,
          int InBits,
          bool InNorm,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutFP,
          bool OutSgn,
          int OutBits,
          bool OutNorm,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter {
  static OutT convert(InT input) {
    return static_cast<OutT>(input);
  }
};

//conversions between normalized types:

template <int V1, int V2>
const int static_abs_of_sub() {
  return V1 >= V2 ? V1 - V2 : V2 - V1;
}

template <typename T>
const T& static_test(const T& value) {
  return value;
}

#ifdef GITS_PLATFORM_WINDOWS
#define STATIC_TEST(x) static_test(x)
#else
#define STATIC_TEST(x) x
#endif

//unorm -> unorm
template <typename InT,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           false,
                           InBits,
                           true,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           false,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    if (STATIC_TEST(InBits > OutBits)) {
      return static_cast<OutT>(input >> static_abs_of_sub<InBits, OutBits>());
    } else if (STATIC_TEST(InBits < OutBits)) {
      return static_cast<OutT>(input << static_abs_of_sub<OutBits, InBits>());
    } else {
      return static_cast<OutT>(input);
    }
  }
};

//unorm -> snorm
template <typename InT,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           false,
                           InBits,
                           true,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           true,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    if (STATIC_TEST(InBits > OutBits)) {
      return static_cast<OutT>(input >> (static_abs_of_sub<InBits, OutBits>() + 1));
    } else if (STATIC_TEST(InBits < OutBits)) {
      return static_cast<OutT>(input << (static_abs_of_sub<OutBits, InBits>() - 1));
    } else {
      return static_cast<OutT>(input >> 1);
    }
  }
};

#ifdef GITS_PLATFORM_WINDOWS
//Disables 'potential divide by 0' warning
__pragma(warning(push)) __pragma(warning(disable : 4723))
#endif

    //snorm -> snorm
    template <typename InT,
              int InBits,
              int64_t InMin,
              int64_t InMax,
              typename OutT,
              int OutBits,
              int64_t OutMin,
              int64_t OutMax>
    struct component_converter<InT,
                               false,
                               true,
                               InBits,
                               true,
                               InMin,
                               InMax,
                               OutT,
                               false,
                               true,
                               OutBits,
                               true,
                               OutMin,
                               OutMax> {
  static OutT convert(InT input) {
    if (STATIC_TEST(InBits > OutBits)) {
      const InT divisor = (InMax + 1) / (OutMax + 1);
      return static_cast<OutT>(input / divisor);
    } else if (STATIC_TEST(InBits < OutBits)) {
      return static_cast<OutT>(input << static_abs_of_sub<OutBits, InBits>());
    } else {
      return static_cast<OutT>(input);
    }
  }
};

//snorm -> unorm
template <typename InT,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           true,
                           InBits,
                           true,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           false,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    input = std::max(input, static_cast<InT>(0));
    if (STATIC_TEST(InBits > OutBits)) {
      const InT divisor = ((InMax + 1) / (OutMax + 1)) / 2;
      return static_cast<OutT>(input / divisor);
    } else if (STATIC_TEST(InBits < OutBits)) {
      return static_cast<OutT>(input << (static_abs_of_sub<OutBits, InBits>() + 1));
    } else {
      return static_cast<OutT>(input * 2);
    }
  }
};

#ifdef GITS_PLATFORM_WINDOWS
__pragma(warning(pop))
#endif

// conversions between integer types or integer and normalized ones:

#ifdef GITS_PLATFORM_WINDOWS
    //Disables 'truncation of constant value' warning, when static_cast of negative valued template parameter OutMin is performed.
    __pragma(warning(push)) __pragma(warning(disable : 4309))
#endif

    //uint/int -> uint/int
    template <typename InT,
              bool InSgn,
              int InBits,
              int64_t InMin,
              int64_t InMax,
              typename OutT,
              bool OutSgn,
              int OutBits,
              int64_t OutMin,
              int64_t OutMax>
    struct component_converter<InT,
                               false,
                               InSgn,
                               InBits,
                               false,
                               InMin,
                               InMax,
                               OutT,
                               false,
                               OutSgn,
                               OutBits,
                               false,
                               OutMin,
                               OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(
        std::max(std::min(input, static_cast<InT>(OutMax)), static_cast<InT>(OutMin)));
  }
};

//unorm/snorm -> uint/int
template <typename InT,
          bool InSgn,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           InSgn,
                           InBits,
                           true,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(
        std::max(std::min(input, static_cast<InT>(OutMax)), static_cast<InT>(OutMin)));
  }
};

//uint/int -> unorm/snorm
template <typename InT,
          bool InSgn,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           InSgn,
                           InBits,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(
        std::max(std::min(input, static_cast<InT>(OutMax)), static_cast<InT>(OutMin)));
  }
};

#ifdef GITS_PLATFORM_WINDOWS
__pragma(warning(pop))
#endif

    // conversions between normalized and floating point types:

    //floating point -> unorm/snorm
    template <typename InT,
              int InBits,
              int64_t InMin,
              int64_t InMax,
              typename OutT,
              bool OutSgn,
              int OutBits,
              int64_t OutMin,
              int64_t OutMax>
    struct component_converter<InT,
                               true,
                               true,
                               InBits,
                               false,
                               InMin,
                               InMax,
                               OutT,
                               false,
                               OutSgn,
                               OutBits,
                               true,
                               OutMin,
                               OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(
        std::max(std::min(static_cast<InT>(OutMax) * input, static_cast<InT>(OutMax)),
                 static_cast<InT>(OutMin)));
  }
};

//unorm/snorm -> floating point
template <typename InT,
          bool InSgn,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           InSgn,
                           InBits,
                           true,
                           InMin,
                           InMax,
                           OutT,
                           true,
                           true,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(input) / static_cast<OutT>(InMax);
  }
};

// conversions between integer and floating point types:

//floating point -> uint/int
template <typename InT,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           true,
                           true,
                           InBits,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(
        std::max(std::min(input, static_cast<InT>(OutMax)), static_cast<InT>(OutMin)));
  }
};

//uint/int -> floating point
template <typename InT,
          bool InSgn,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          typename OutT,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           InSgn,
                           InBits,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           true,
                           true,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(InT input) {
    return static_cast<OutT>(input);
  }
};

//conversions between normalized, floating point and half floating point types:

const int first_set_bit_indices[] = {-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                                     4,  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

int find_first_set_bit_in_10bit_word(uint32_t value) {
  uint32_t higher_half = value & (1023 - 31);
  if (higher_half) {
    return first_set_bit_indices[higher_half >> 5] + 5;
  } else {
    return first_set_bit_indices[value];
  }
}

float convert_half_to_single_float(uint16_t value) {
  uint32_t f16 = static_cast<uint32_t>(value);
  uint32_t exponent = (f16 >> 10) & 31;
  uint32_t sign = f16 >> 15;
  uint32_t fraction = f16 & 1023;
  uint32_t f32;
  if (exponent == 0) {
    //subnormal
    int exponent_offset = 9 - find_first_set_bit_in_10bit_word(fraction);
    f32 = (sign << 31) | ((127 - 15 - exponent_offset) << 23) |
          ((fraction << (14 + exponent_offset)) & 0x7FFFFF);
  } else if (exponent == 31) {
    if (fraction == 0) {
      //infinity
      f32 = (sign << 31) | (255 << 23);
    } else {
      //NaN
      f32 = (sign << 31) | (255 << 23) | (fraction << 13);
    }
  } else {
    //normalized
    f32 = (sign << 31) | ((exponent + 127 - 15) << 23) | (fraction << 13);
  }
  return *((float*)&f32);
}

uint16_t convert_single_to_half_float(float value) {
  uint32_t f32 = *((uint32_t*)&value);
  uint32_t exponent = (f32 >> 23) & 255;
  uint32_t sign = f32 >> 31;
  uint32_t fraction = f32 & 0x7FFFFF;
  uint16_t f16;
  if (exponent == 0) {
    //denormal or zero, let's treat it as zero
    f16 = static_cast<uint16_t>(sign << 15);
  } else if (exponent == 255) {
    if (fraction == 0) {
      //infinity
      f16 = static_cast<uint16_t>((sign << 15) | (31 << 10));
    } else {
      //NaN
      f16 = static_cast<uint16_t>((sign << 15) | 32767);
    }
  } else {
    //normalized
    uint32_t exp5 =
        static_cast<uint32_t>(std::max(std::min(static_cast<int>(exponent) - 127 + 15, 31), 0));
    f16 = static_cast<uint16_t>((sign << 15) | (exp5 << 10) | (fraction >> 13));
  }
  return f16;
}

//half-float -> unorm/snorm
template <int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<uint16_t,
                           true,
                           true,
                           16,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(uint16_t input) {
    float value = convert_half_to_single_float(input);
    return static_cast<OutT>(
        std::max(std::min(static_cast<float>(OutMax) * value, static_cast<float>(OutMax)),
                 static_cast<float>(OutMin)));
  }
};

//unorm/snorm -> half-float
template <typename InT,
          bool InSgn,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           InSgn,
                           InBits,
                           true,
                           InMin,
                           InMax,
                           uint16_t,
                           true,
                           true,
                           16,
                           false,
                           OutMin,
                           OutMax> {
  static uint16_t convert(InT input) {
    float value = static_cast<float>(input) / static_cast<float>(InMax);
    return convert_single_to_half_float(value);
  }
};

//half-float -> float
template <int64_t InMin, int64_t InMax, int64_t OutMin, int64_t OutMax>
struct component_converter<uint16_t,
                           true,
                           true,
                           16,
                           false,
                           InMin,
                           InMax,
                           float,
                           true,
                           true,
                           32,
                           false,
                           OutMin,
                           OutMax> {
  static float convert(uint16_t input) {
    return convert_half_to_single_float(input);
  }
};

//float -> half-float
template <int64_t InMin, int64_t InMax, int64_t OutMin, int64_t OutMax>
struct component_converter<float,
                           true,
                           true,
                           32,
                           false,
                           InMin,
                           InMax,
                           uint16_t,
                           true,
                           true,
                           16,
                           false,
                           OutMin,
                           OutMax> {
  static uint16_t convert(float input) {
    return convert_single_to_half_float(input);
  }
};

//half-float -> uint/int
template <int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<uint16_t,
                           true,
                           true,
                           16,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(uint16_t input) {
    float value = convert_half_to_single_float(input);
    return static_cast<OutT>(
        std::max(std::min(value, static_cast<float>(OutMax)), static_cast<float>(OutMin)));
  }
};

//uint/int -> half-float
template <typename InT,
          bool InSgn,
          int InBits,
          int64_t InMin,
          int64_t InMax,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<InT,
                           false,
                           InSgn,
                           InBits,
                           false,
                           InMin,
                           InMax,
                           uint16_t,
                           true,
                           true,
                           16,
                           false,
                           OutMin,
                           OutMax> {
  static uint16_t convert(InT input) {
    return convert_single_to_half_float(input);
  }
};

// conversions between normalized types and unsigned 11-bit or 10-bit floating point types

template <int FractionBits, int FractionMask, int ExpBits, int ExpMask, int ExpHalf, int Mask>
float convert_unsigned_nbit_to_single_float(uint32_t value) {
  uint32_t fin = static_cast<uint32_t>(value) & Mask;
  uint32_t exponent = (fin >> FractionBits) & ExpMask;
  uint32_t fraction = fin & FractionMask;
  uint32_t f32;
  if (exponent > 0 && exponent < ExpMask) {
    //normalized
    f32 = ((exponent + 127 - ExpHalf) << 23) | (fraction << (23 - FractionBits));
  } else if (exponent == 0) {
    if (fraction == 0) {
      return 0.0f;
    } else {
      // 2^-14 * (fraction / 2^FractionBits)
      return fraction * (0.0000610352f / static_cast<float>(FractionMask + 1));
    }
  } else {
    if (fraction == 0) {
      //infinity
      f32 = (1u << 31) | (255u << 23);
    } else {
      //NaN
      f32 = (1u << 31) | (255u << 23) | (fraction << 13);
    }
  }
  return *((float*)&f32);
}

//11-bit float -> unorm/snorm
template <int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<uint32_t,
                           true,
                           false,
                           11,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(uint32_t input) {
    float value = convert_unsigned_nbit_to_single_float<6, 63, 5, 31, 15, 2047>(input);
    return static_cast<OutT>(
        std::max(std::min(static_cast<float>(OutMax) * value, static_cast<float>(OutMax)),
                 static_cast<float>(OutMin)));
  }
};

//11-bit float -> uint/int
template <int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<uint32_t,
                           true,
                           false,
                           11,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(uint32_t input) {
    float value = convert_unsigned_nbit_to_single_float<6, 63, 5, 31, 15, 2047>(input);
    return static_cast<OutT>(
        std::max(std::min(value, static_cast<float>(OutMax)), static_cast<float>(OutMin)));
  }
};

//11-bit float -> single float
template <int64_t InMin, int64_t InMax, int64_t OutMin, int64_t OutMax>
struct component_converter<uint32_t,
                           true,
                           false,
                           11,
                           false,
                           InMin,
                           InMax,
                           float,
                           true,
                           true,
                           32,
                           false,
                           OutMin,
                           OutMax> {
  static float convert(uint32_t input) {
    return convert_unsigned_nbit_to_single_float<6, 63, 5, 31, 15, 2047>(input);
  }
};

//10-bit float -> unorm/snorm
template <int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<uint32_t,
                           true,
                           false,
                           10,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           true,
                           OutMin,
                           OutMax> {
  static OutT convert(uint32_t input) {
    float value = convert_unsigned_nbit_to_single_float<5, 31, 5, 31, 15, 1023>(input);
    return static_cast<OutT>(
        std::max(std::min(static_cast<float>(OutMax) * value, static_cast<float>(OutMax)),
                 static_cast<float>(OutMin)));
  }
};

//10-bit float -> uint/int
template <int64_t InMin,
          int64_t InMax,
          typename OutT,
          bool OutSgn,
          int OutBits,
          int64_t OutMin,
          int64_t OutMax>
struct component_converter<uint32_t,
                           true,
                           false,
                           10,
                           false,
                           InMin,
                           InMax,
                           OutT,
                           false,
                           OutSgn,
                           OutBits,
                           false,
                           OutMin,
                           OutMax> {
  static OutT convert(uint32_t input) {
    float value = convert_unsigned_nbit_to_single_float<5, 31, 5, 31, 15, 1023>(input);
    return static_cast<OutT>(
        std::max(std::min(value, static_cast<float>(OutMax)), static_cast<float>(OutMin)));
  }
};

//10-bit float -> single float
template <int64_t InMin, int64_t InMax, int64_t OutMin, int64_t OutMax>
struct component_converter<uint32_t,
                           true,
                           false,
                           10,
                           false,
                           InMin,
                           InMax,
                           float,
                           true,
                           true,
                           32,
                           false,
                           OutMin,
                           OutMax> {
  static float convert(uint32_t input) {
    return convert_unsigned_nbit_to_single_float<5, 31, 5, 31, 15, 1023>(input);
  }
};

template <typename In, typename Out>
typename Out::type convert_component(typename In::type input) {
  return component_converter<typename In::type, In::floating_point, In::signed_value, In::bit_count,
                             In::normalized, In::min_value, In::max_value, typename Out::type,
                             Out::floating_point, Out::signed_value, Out::bit_count,
                             Out::normalized, Out::min_value, Out::max_value>::convert(input);
}

template <typename T,
          int R,
          int G,
          int B,
          int A,
          int Size,
          bool IsPacked = false,
          typename PackedT = uint_32,
          typename T2 = T,
          typename T3 = T,
          typename T4 = T>
struct texel_descriptor {
  using comp_fmt = T;
  static const int r = R;
  static const int g = G;
  static const int b = B;
  static const int a = A;
  static const int size_in_bytes = Size;
  static const bool is_packed = IsPacked;
  using packed_fmt = PackedT;
  using comp2_fmt = T2;
  using comp3_fmt = T3;
  using comp4_fmt = T4;
};

const int NA = -1;

using a_8unorm = texel_descriptor<unorm_8, NA, NA, NA, 0, 1>;
using r_8unorm = texel_descriptor<unorm_8, 0, NA, NA, NA, 1>;
using r_8snorm = texel_descriptor<snorm_8, 0, NA, NA, NA, 1>;
using r_8ui = texel_descriptor<uint_8, 0, NA, NA, NA, 1>;
using r_8i = texel_descriptor<int_8, 0, NA, NA, NA, 1>;

using r_16unorm = texel_descriptor<unorm_16, 0, NA, NA, NA, 2>;
using r_16snorm = texel_descriptor<snorm_16, 0, NA, NA, NA, 2>;
using r_16ui = texel_descriptor<uint_16, 0, NA, NA, NA, 2>;
using r_16i = texel_descriptor<int_16, 0, NA, NA, NA, 2>;
using r_16f = texel_descriptor<float_16, 0, NA, NA, NA, 2>;

using x8d24_unorm = texel_descriptor<unorm_24, 0, 0, 0, NA, 4, true>;
using d24_unorm = texel_descriptor<unorm_24, 0, NA, NA, NA, 4>;

using rg_8unorm = texel_descriptor<unorm_8, 0, 1, NA, NA, 2>;
using rg_8snorm = texel_descriptor<snorm_8, 0, 1, NA, NA, 2>;
using rg_8ui = texel_descriptor<uint_8, 0, 1, NA, NA, 2>;
using rg_8i = texel_descriptor<int_8, 0, 1, NA, NA, 2>;

using r_32ui = texel_descriptor<uint_32, 0, NA, NA, NA, 4>;
using r_32i = texel_descriptor<int_32, 0, NA, NA, NA, 4>;
using r_32f = texel_descriptor<float_32, 0, NA, NA, NA, 4>;

using rg_16unorm = texel_descriptor<unorm_16, 0, 1, NA, NA, 4>;
using rg_16snorm = texel_descriptor<snorm_16, 0, 1, NA, NA, 4>;
using rg_16ui = texel_descriptor<uint_16, 0, 1, NA, NA, 4>;
using rg_16i = texel_descriptor<int_16, 0, 1, NA, NA, 4>;
using rg_16f = texel_descriptor<float_16, 0, 1, NA, NA, 4>;

using rgb_8unorm = texel_descriptor<unorm_8, 0, 1, 2, NA, 3>;
using rgb_8snorm = texel_descriptor<snorm_8, 0, 1, 2, NA, 3>;
using rgb_8ui = texel_descriptor<uint_8, 0, 1, 2, NA, 3>;
using rgb_8i = texel_descriptor<int_8, 0, 1, 2, NA, 3>;
using bgr_8unorm = texel_descriptor<unorm_8, 2, 1, 0, NA, 3>;
using bgr_8snorm = texel_descriptor<snorm_8, 2, 1, 0, NA, 3>;
using bgr_8i = texel_descriptor<int_8, 2, 1, 0, NA, 3>;
using bgr_8ui = texel_descriptor<uint_8, 2, 1, 0, NA, 3>;

using rgba_8unorm = texel_descriptor<unorm_8, 0, 1, 2, 3, 4>;
using rgba_8snorm = texel_descriptor<snorm_8, 0, 1, 2, 3, 4>;
using rgba_8ui = texel_descriptor<uint_8, 0, 1, 2, 3, 4>;
using rgba_8i = texel_descriptor<int_8, 0, 1, 2, 3, 4>;
using bgra_8unorm = texel_descriptor<unorm_8, 2, 1, 0, 3, 4>;
using bgra_8i = texel_descriptor<int_8, 2, 1, 0, 3, 4>;
using bgra_8snorm = texel_descriptor<snorm_8, 2, 1, 0, 3, 4>;
using bgra_8ui = texel_descriptor<uint_8, 2, 1, 0, 3, 4>;
using abgr_8unorm = texel_descriptor<unorm_8, 3, 2, 1, 0, 4>;
using abgr_8i = texel_descriptor<int_8, 3, 2, 1, 0, 4>;
using abgr_8snorm = texel_descriptor<snorm_8, 3, 2, 1, 0, 4>;
using abgr_8ui = texel_descriptor<uint_8, 3, 2, 1, 0, 4>;

using rg11b10_f = texel_descriptor<float_11, 0, 11, 22, NA, 4, true, uint_32, float_11, float_10>;
using b10gr11_f = texel_descriptor<float_11, 21, 10, 0, NA, 4, true, uint_32, float_11, float_10>;
using rgb10a2_unorm =
    texel_descriptor<unorm_10, 0, 10, 20, 30, 4, true, uint_32, unorm_10, unorm_10, unorm_2>;
using rgb10a2_ui =
    texel_descriptor<uint_10, 0, 10, 20, 30, 4, true, uint_32, uint_10, uint_10, uint_2>;
using bgr10a2_unorm =
    texel_descriptor<unorm_10, 20, 10, 0, 30, 4, true, uint_32, unorm_10, unorm_10, unorm_2>;

using rg_32ui = texel_descriptor<uint_32, 0, 1, NA, NA, 8>;
using rg_32i = texel_descriptor<int_32, 0, 1, NA, NA, 8>;
using rg_32f = texel_descriptor<float_32, 0, 1, NA, NA, 8>;

using rgba_16unorm = texel_descriptor<unorm_16, 0, 1, 2, 3, 8>;
using rgba_16snorm = texel_descriptor<snorm_16, 0, 1, 2, 3, 8>;
using rgba_16ui = texel_descriptor<uint_16, 0, 1, 2, 3, 8>;
using rgba_16i = texel_descriptor<int_16, 0, 1, 2, 3, 8>;
using rgba_16f = texel_descriptor<float_16, 0, 1, 2, 3, 8>;

using rgba_32ui = texel_descriptor<uint_32, 0, 1, 2, 3, 16>;
using rgba_32i = texel_descriptor<int_32, 0, 1, 2, 3, 16>;
using rgba_32f = texel_descriptor<float_32, 0, 1, 2, 3, 16>;

#ifdef GITS_PLATFORM_WINDOWS
// We use push/pop to set the scope where the following pragmas are in effect.
__pragma(warning(push))
    // Disables the "truncation of constant value" warning.
    __pragma(warning(disable : 4309))
    // Disables the "shift count negative or too big, undefined behavior" warning.
    __pragma(warning(disable : 4293))
    // Disables the "unknown pragma" warning caused by GCC pragmas.
    __pragma(warning(disable : 4068))
#endif

#pragma GCC diagnostic push
// GCC <= 4.9 doesn't recognize the following -Wnoshift-count-negative pragma,
// but newer GCCs do. To avoid warnings in old GCC, but have that pragma work
// in new GCC, we disable the "unknown option after '#pragma GCC diagnostic'
// kind" warning.
#pragma GCC diagnostic ignored "-Wpragmas"
// Disables the "left shift count is negative" warning.
#pragma GCC diagnostic ignored "-Wshift-count-negative"

    // GCC 4.9 can't silence negative shift warnings with a pragma directive, so we
    // use these shift guards instead.
    // TODO: remove them when we update to GCC >= 5, as above pragma is enough.
    template <int X>
    int shift_guard() {
  return X;
}

template <>
int shift_guard<NA>() {
  return 0;
}

template <bool InPacked,
          typename InPackT,
          typename InFmt,
          int InComp,
          bool OutPacked,
          typename OutPackT,
          typename OutFmt,
          int OutComp,
          int Default = 0>
void convert_texel_component(const typename InFmt::type* ptr_in, typename OutFmt::type* ptr_out) {
  if (STATIC_TEST(OutComp != NA)) {
    if (STATIC_TEST(InPacked)) {
      auto in_comp =
          InComp != NA
              ? static_cast<typename InFmt::type>(
                    (*(reinterpret_cast<const typename InPackT::type*>(ptr_in)) >> InComp) &
                    InFmt::max_uint_value)
              : static_cast<typename InFmt::type>(Default);
      if (STATIC_TEST(OutPacked)) {
        auto packed_out = reinterpret_cast<typename OutPackT::type*>(ptr_out);
        auto out_comp = convert_component<InFmt, OutFmt>(in_comp);
        *packed_out |= static_cast<typename OutPackT::type>(out_comp) << shift_guard<OutComp>();
      } else {
        ptr_out[OutComp] = convert_component<InFmt, OutFmt>(in_comp);
      }
    } else {
      auto in_comp = InComp != NA ? ptr_in[InComp] : static_cast<typename InFmt::type>(Default);

      if (STATIC_TEST(OutPacked)) {
        auto packed_out = reinterpret_cast<typename OutPackT::type*>(ptr_out);
        auto out_comp = convert_component<InFmt, OutFmt>(in_comp);
        *packed_out |= static_cast<typename OutPackT::type>(out_comp) << shift_guard<OutComp>();
      } else {
        ptr_out[OutComp] = convert_component<InFmt, OutFmt>(in_comp);
      }
    }
  }
}

#pragma GCC diagnostic pop

#ifdef GITS_PLATFORM_WINDOWS
__pragma(warning(pop))
#endif

    template <typename In, typename Out>
    void convert_texel(const uint8_t* ptr_input, uint8_t* ptr_output) {
  auto ptr_in = reinterpret_cast<const typename In::comp_fmt::type*>(ptr_input);
  auto ptr_out = reinterpret_cast<typename Out::comp_fmt::type*>(ptr_output);

  if (STATIC_TEST(Out::is_packed)) {
    auto ptr_packed_out = reinterpret_cast<typename In::packed_fmt::type*>(ptr_output);
    *ptr_packed_out = 0;
  }

  convert_texel_component<In::is_packed, typename In::packed_fmt, typename In::comp_fmt, In::r,
                          Out::is_packed, typename Out::packed_fmt, typename Out::comp_fmt, Out::r>(
      ptr_in, ptr_out);
  convert_texel_component<In::is_packed, typename In::packed_fmt, typename In::comp2_fmt, In::g,
                          Out::is_packed, typename Out::packed_fmt, typename Out::comp2_fmt,
                          Out::g>(ptr_in, ptr_out);
  convert_texel_component<In::is_packed, typename In::packed_fmt, typename In::comp3_fmt, In::b,
                          Out::is_packed, typename Out::packed_fmt, typename Out::comp3_fmt,
                          Out::b>(ptr_in, ptr_out);
  convert_texel_component<In::is_packed, typename In::packed_fmt, typename In::comp4_fmt, In::a,
                          Out::is_packed, typename Out::packed_fmt, typename Out::comp4_fmt, Out::a,
                          Out::comp4_fmt::one_value>(ptr_in, ptr_out);
}

template <typename In, typename Out>
void convert(const std::vector<uint8_t>& input_data,
             std::vector<uint8_t>& output_data,
             int width,
             int height) {
  auto input_texel_size = In::size_in_bytes;
  auto input_image_size = input_texel_size * width * height;
  auto output_texel_size = Out::size_in_bytes;
  auto output_image_size = output_texel_size * width * height;

  if (input_data.size() < (size_t)input_image_size) {
    Log(ERR) << "Invalid input buffer/file size: " << input_data.size()
             << " bytes as a source of image: " << width << "x" << height
             << " with texel size: " << input_texel_size << " bytes.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (output_data.size() < (size_t)output_image_size) {
    Log(ERR) << "Invalid output buffer/file size: " << output_data.size()
             << " as a destination for image: " << width << "x" << height
             << " with texel size: " << output_texel_size << " bytes.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  for (int i = 0, j = 0; i < input_image_size; i += input_texel_size, j += output_texel_size) {
    convert_texel<In, Out>(&input_data[i], &output_data[j]);
  }
}

using conversion_type = std::pair<texel_type, texel_type>;
using conversion_function = void (*)(const std::vector<uint8_t>& input_data,
                                     std::vector<uint8_t>& output_data,
                                     int width,
                                     int height);

std::map<conversion_type, conversion_function> converter_map = {
    // conversions to BGRA8 unsigned normalized:

    {{texel_type::A8, texel_type::BGRA8}, convert<a_8unorm, bgra_8unorm>},
    {{texel_type::R8, texel_type::BGRA8}, convert<r_8unorm, bgra_8unorm>},
    {{texel_type::R8snorm, texel_type::BGRA8}, convert<r_8snorm, bgra_8unorm>},
    {{texel_type::R8ui, texel_type::BGRA8}, convert<r_8ui, bgra_8unorm>},
    {{texel_type::R8i, texel_type::BGRA8}, convert<r_8i, bgra_8unorm>},

    {{texel_type::R16, texel_type::BGRA8}, convert<r_16unorm, bgra_8unorm>},
    {{texel_type::R16snorm, texel_type::BGRA8}, convert<r_16snorm, bgra_8unorm>},
    {{texel_type::R16ui, texel_type::BGRA8}, convert<r_16ui, bgra_8unorm>},
    {{texel_type::R16i, texel_type::BGRA8}, convert<r_16i, bgra_8unorm>},
    {{texel_type::R16f, texel_type::BGRA8}, convert<r_16f, bgra_8unorm>},

    {{texel_type::RG8, texel_type::BGRA8}, convert<rg_8unorm, bgra_8unorm>},
    {{texel_type::RG8snorm, texel_type::BGRA8}, convert<rg_8snorm, bgra_8unorm>},
    {{texel_type::RG8ui, texel_type::BGRA8}, convert<rg_8ui, bgra_8unorm>},
    {{texel_type::RG8i, texel_type::BGRA8}, convert<rg_8i, bgra_8unorm>},

    {{texel_type::R32ui, texel_type::BGRA8}, convert<r_32ui, bgra_8unorm>},
    {{texel_type::R32i, texel_type::BGRA8}, convert<r_32i, bgra_8unorm>},
    {{texel_type::R32f, texel_type::BGRA8}, convert<r_32f, bgra_8unorm>},

    {{texel_type::RG16, texel_type::BGRA8}, convert<rg_16unorm, bgra_8unorm>},
    {{texel_type::RG16snorm, texel_type::BGRA8}, convert<rg_16snorm, bgra_8unorm>},
    {{texel_type::RG16ui, texel_type::BGRA8}, convert<rg_16ui, bgra_8unorm>},
    {{texel_type::RG16i, texel_type::BGRA8}, convert<rg_16i, bgra_8unorm>},
    {{texel_type::RG16f, texel_type::BGRA8}, convert<rg_16f, bgra_8unorm>},

    {{texel_type::RGB8, texel_type::BGRA8}, convert<rgb_8unorm, bgra_8unorm>},
    {{texel_type::RGB8i, texel_type::BGRA8}, convert<rgb_8i, bgra_8unorm>},
    {{texel_type::RGB8ui, texel_type::BGRA8}, convert<rgb_8ui, bgra_8unorm>},
    {{texel_type::RGB8snorm, texel_type::BGRA8}, convert<rgb_8snorm, bgra_8unorm>},

    {{texel_type::BGR8, texel_type::BGRA8}, convert<bgr_8unorm, bgra_8unorm>},
    {{texel_type::BGR8i, texel_type::BGRA8}, convert<bgr_8i, bgra_8unorm>},
    {{texel_type::BGR8ui, texel_type::BGRA8}, convert<bgr_8ui, bgra_8unorm>},
    {{texel_type::BGR8snorm, texel_type::BGRA8}, convert<bgr_8snorm, bgra_8unorm>},

    {{texel_type::RGBA8, texel_type::BGRA8}, convert<rgba_8unorm, bgra_8unorm>},
    {{texel_type::RGBA8snorm, texel_type::BGRA8}, convert<rgba_8snorm, bgra_8unorm>},
    {{texel_type::RGBA8ui, texel_type::BGRA8}, convert<rgba_8ui, bgra_8unorm>},
    {{texel_type::RGBA8i, texel_type::BGRA8}, convert<rgba_8i, bgra_8unorm>},

    {{texel_type::BGRA8, texel_type::BGRA8}, convert<bgra_8unorm, bgra_8unorm>},
    {{texel_type::BGRA8i, texel_type::BGRA8}, convert<bgra_8i, bgra_8unorm>},
    {{texel_type::BGRA8snorm, texel_type::BGRA8}, convert<bgra_8snorm, bgra_8unorm>},
    {{texel_type::BGRA8ui, texel_type::BGRA8}, convert<bgra_8ui, bgra_8unorm>},

    {{texel_type::ABGR8, texel_type::BGRA8}, convert<abgr_8unorm, bgra_8unorm>},
    {{texel_type::ABGR8i, texel_type::BGRA8}, convert<abgr_8i, bgra_8unorm>},
    {{texel_type::ABGR8snorm, texel_type::BGRA8}, convert<abgr_8snorm, bgra_8unorm>},
    {{texel_type::ABGR8ui, texel_type::BGRA8}, convert<abgr_8ui, bgra_8unorm>},

    {{texel_type::X8D24, texel_type::BGRA8}, convert<x8d24_unorm, bgra_8unorm>},
    {{texel_type::D24, texel_type::BGRA8}, convert<d24_unorm, bgra_8unorm>},

    {{texel_type::RGB10A2, texel_type::BGRA8}, convert<rgb10a2_unorm, bgra_8unorm>},
    {{texel_type::RGB10A2ui, texel_type::BGRA8}, convert<rgb10a2_ui, bgra_8unorm>},
    {{texel_type::BGR10A2, texel_type::BGRA8}, convert<bgr10a2_unorm, bgra_8unorm>},
    {{texel_type::RG11B10f, texel_type::BGRA8}, convert<rg11b10_f, bgra_8unorm>},
    {{texel_type::B10GR11f, texel_type::BGRA8}, convert<b10gr11_f, bgra_8unorm>},

    {{texel_type::RG32ui, texel_type::BGRA8}, convert<rg_32ui, bgra_8unorm>},
    {{texel_type::RG32i, texel_type::BGRA8}, convert<rg_32i, bgra_8unorm>},
    {{texel_type::RG32f, texel_type::BGRA8}, convert<rg_32f, bgra_8unorm>},

    {{texel_type::RGBA16, texel_type::BGRA8}, convert<rgba_16unorm, bgra_8unorm>},
    {{texel_type::RGBA16snorm, texel_type::BGRA8}, convert<rgba_16snorm, bgra_8unorm>},
    {{texel_type::RGBA16ui, texel_type::BGRA8}, convert<rgba_16ui, bgra_8unorm>},
    {{texel_type::RGBA16i, texel_type::BGRA8}, convert<rgba_16i, bgra_8unorm>},
    {{texel_type::RGBA16f, texel_type::BGRA8}, convert<rgba_16f, bgra_8unorm>},

    {{texel_type::RGBA32ui, texel_type::BGRA8}, convert<rgba_32ui, bgra_8unorm>},
    {{texel_type::RGBA32i, texel_type::BGRA8}, convert<rgba_32i, bgra_8unorm>},
    {{texel_type::RGBA32f, texel_type::BGRA8}, convert<rgba_32f, bgra_8unorm>},

    // conversions to RGBA8 unsigned normalized:

    {{texel_type::A8, texel_type::RGBA8}, convert<a_8unorm, rgba_8unorm>},
    {{texel_type::R8, texel_type::RGBA8}, convert<r_8unorm, rgba_8unorm>},
    {{texel_type::R8snorm, texel_type::RGBA8}, convert<r_8snorm, rgba_8unorm>},
    {{texel_type::R8ui, texel_type::RGBA8}, convert<r_8ui, rgba_8unorm>},
    {{texel_type::R8i, texel_type::RGBA8}, convert<r_8i, rgba_8unorm>},

    {{texel_type::R16, texel_type::RGBA8}, convert<r_16unorm, rgba_8unorm>},
    {{texel_type::R16snorm, texel_type::RGBA8}, convert<r_16snorm, rgba_8unorm>},
    {{texel_type::R16ui, texel_type::RGBA8}, convert<r_16ui, rgba_8unorm>},
    {{texel_type::R16i, texel_type::RGBA8}, convert<r_16i, rgba_8unorm>},
    {{texel_type::R16f, texel_type::RGBA8}, convert<r_16f, rgba_8unorm>},

    {{texel_type::RG8, texel_type::RGBA8}, convert<rg_8unorm, rgba_8unorm>},
    {{texel_type::RG8snorm, texel_type::RGBA8}, convert<rg_8snorm, rgba_8unorm>},
    {{texel_type::RG8ui, texel_type::RGBA8}, convert<rg_8ui, rgba_8unorm>},
    {{texel_type::RG8i, texel_type::RGBA8}, convert<rg_8i, rgba_8unorm>},

    {{texel_type::R32ui, texel_type::RGBA8}, convert<r_32ui, rgba_8unorm>},
    {{texel_type::R32i, texel_type::RGBA8}, convert<r_32i, rgba_8unorm>},
    {{texel_type::R32f, texel_type::RGBA8}, convert<r_32f, rgba_8unorm>},

    {{texel_type::RG16, texel_type::RGBA8}, convert<rg_16unorm, rgba_8unorm>},
    {{texel_type::RG16snorm, texel_type::RGBA8}, convert<rg_16snorm, rgba_8unorm>},
    {{texel_type::RG16ui, texel_type::RGBA8}, convert<rg_16ui, rgba_8unorm>},
    {{texel_type::RG16i, texel_type::RGBA8}, convert<rg_16i, rgba_8unorm>},
    {{texel_type::RG16f, texel_type::RGBA8}, convert<rg_16f, rgba_8unorm>},

    {{texel_type::RGB8, texel_type::RGBA8}, convert<rgb_8unorm, rgba_8unorm>},
    {{texel_type::RGB8i, texel_type::RGBA8}, convert<rgb_8i, rgba_8unorm>},
    {{texel_type::RGB8ui, texel_type::RGBA8}, convert<rgb_8ui, rgba_8unorm>},
    {{texel_type::RGB8snorm, texel_type::RGBA8}, convert<rgb_8snorm, rgba_8unorm>},

    {{texel_type::BGR8, texel_type::RGBA8}, convert<bgr_8unorm, rgba_8unorm>},
    {{texel_type::BGR8i, texel_type::RGBA8}, convert<bgr_8i, rgba_8unorm>},
    {{texel_type::BGR8ui, texel_type::RGBA8}, convert<bgr_8ui, rgba_8unorm>},
    {{texel_type::BGR8snorm, texel_type::RGBA8}, convert<bgr_8snorm, rgba_8unorm>},

    {{texel_type::RGBA8, texel_type::RGBA8}, convert<rgba_8unorm, rgba_8unorm>},
    {{texel_type::RGBA8snorm, texel_type::RGBA8}, convert<rgba_8snorm, rgba_8unorm>},
    {{texel_type::RGBA8ui, texel_type::RGBA8}, convert<rgba_8ui, rgba_8unorm>},
    {{texel_type::RGBA8i, texel_type::RGBA8}, convert<rgba_8i, rgba_8unorm>},

    {{texel_type::BGRA8, texel_type::RGBA8}, convert<bgra_8unorm, rgba_8unorm>},
    {{texel_type::BGRA8snorm, texel_type::RGBA8}, convert<bgra_8snorm, rgba_8unorm>},
    {{texel_type::BGRA8ui, texel_type::RGBA8}, convert<bgra_8ui, rgba_8unorm>},
    {{texel_type::BGRA8i, texel_type::RGBA8}, convert<bgra_8i, rgba_8unorm>},

    {{texel_type::ABGR8, texel_type::RGBA8}, convert<abgr_8unorm, rgba_8unorm>},
    {{texel_type::ABGR8snorm, texel_type::RGBA8}, convert<abgr_8snorm, rgba_8unorm>},
    {{texel_type::ABGR8ui, texel_type::RGBA8}, convert<abgr_8ui, rgba_8unorm>},
    {{texel_type::ABGR8i, texel_type::RGBA8}, convert<abgr_8i, rgba_8unorm>},

    {{texel_type::X8D24, texel_type::RGBA8}, convert<x8d24_unorm, rgba_8unorm>},
    {{texel_type::D24, texel_type::RGBA8}, convert<d24_unorm, rgba_8unorm>},

    {{texel_type::RGB10A2, texel_type::RGBA8}, convert<rgb10a2_unorm, rgba_8unorm>},
    {{texel_type::RGB10A2ui, texel_type::RGBA8}, convert<rgb10a2_ui, rgba_8unorm>},
    {{texel_type::BGR10A2, texel_type::RGBA8}, convert<bgr10a2_unorm, rgba_8unorm>},
    {{texel_type::RG11B10f, texel_type::RGBA8}, convert<rg11b10_f, rgba_8unorm>},
    {{texel_type::B10GR11f, texel_type::RGBA8}, convert<b10gr11_f, rgba_8unorm>},

    {{texel_type::RG32ui, texel_type::RGBA8}, convert<rg_32ui, rgba_8unorm>},
    {{texel_type::RG32i, texel_type::RGBA8}, convert<rg_32i, rgba_8unorm>},
    {{texel_type::RG32f, texel_type::RGBA8}, convert<rg_32f, rgba_8unorm>},

    {{texel_type::RGBA16, texel_type::RGBA8}, convert<rgba_16unorm, rgba_8unorm>},
    {{texel_type::RGBA16snorm, texel_type::RGBA8}, convert<rgba_16snorm, rgba_8unorm>},
    {{texel_type::RGBA16ui, texel_type::RGBA8}, convert<rgba_16ui, rgba_8unorm>},
    {{texel_type::RGBA16i, texel_type::RGBA8}, convert<rgba_16i, rgba_8unorm>},
    {{texel_type::RGBA16f, texel_type::RGBA8}, convert<rgba_16f, rgba_8unorm>},

    {{texel_type::RGBA32ui, texel_type::RGBA8}, convert<rgba_32ui, rgba_8unorm>},
    {{texel_type::RGBA32i, texel_type::RGBA8}, convert<rgba_32i, rgba_8unorm>},
    {{texel_type::RGBA32f, texel_type::RGBA8}, convert<rgba_32f, rgba_8unorm>},

    // conversions to RGBA8 unsigned int:

    {{texel_type::A8, texel_type::RGBA8ui}, convert<a_8unorm, rgba_8ui>},
    {{texel_type::R8, texel_type::RGBA8ui}, convert<r_8unorm, rgba_8ui>},
    {{texel_type::R8snorm, texel_type::RGBA8ui}, convert<r_8snorm, rgba_8ui>},
    {{texel_type::R8ui, texel_type::RGBA8ui}, convert<r_8ui, rgba_8ui>},
    {{texel_type::R8i, texel_type::RGBA8ui}, convert<r_8i, rgba_8ui>},

    {{texel_type::R16, texel_type::RGBA8ui}, convert<r_16unorm, rgba_8ui>},
    {{texel_type::R16snorm, texel_type::RGBA8ui}, convert<r_16snorm, rgba_8ui>},
    {{texel_type::R16ui, texel_type::RGBA8ui}, convert<r_16ui, rgba_8ui>},
    {{texel_type::R16i, texel_type::RGBA8ui}, convert<r_16i, rgba_8ui>},
    {{texel_type::R16f, texel_type::RGBA8ui}, convert<r_16f, rgba_8ui>},

    {{texel_type::RG8, texel_type::RGBA8ui}, convert<rg_8unorm, rgba_8ui>},
    {{texel_type::RG8snorm, texel_type::RGBA8ui}, convert<rg_8snorm, rgba_8ui>},
    {{texel_type::RG8ui, texel_type::RGBA8ui}, convert<rg_8ui, rgba_8ui>},
    {{texel_type::RG8i, texel_type::RGBA8ui}, convert<rg_8i, rgba_8ui>},

    {{texel_type::R32ui, texel_type::RGBA8ui}, convert<r_32ui, rgba_8ui>},
    {{texel_type::R32i, texel_type::RGBA8ui}, convert<r_32i, rgba_8ui>},
    {{texel_type::R32f, texel_type::RGBA8ui}, convert<r_32f, rgba_8ui>},

    {{texel_type::RG16, texel_type::RGBA8ui}, convert<rg_16unorm, rgba_8ui>},
    {{texel_type::RG16snorm, texel_type::RGBA8ui}, convert<rg_16snorm, rgba_8ui>},
    {{texel_type::RG16ui, texel_type::RGBA8ui}, convert<rg_16ui, rgba_8ui>},
    {{texel_type::RG16i, texel_type::RGBA8ui}, convert<rg_16i, rgba_8ui>},
    {{texel_type::RG16f, texel_type::RGBA8ui}, convert<rg_16f, rgba_8ui>},

    {{texel_type::RGB8, texel_type::RGBA8ui}, convert<rgb_8unorm, rgba_8ui>},
    {{texel_type::RGB8i, texel_type::RGBA8ui}, convert<rgb_8i, rgba_8ui>},
    {{texel_type::RGB8ui, texel_type::RGBA8ui}, convert<rgb_8ui, rgba_8ui>},
    {{texel_type::RGB8snorm, texel_type::RGBA8ui}, convert<rgb_8snorm, rgba_8ui>},

    {{texel_type::BGR8, texel_type::RGBA8ui}, convert<bgr_8unorm, rgba_8ui>},
    {{texel_type::BGR8i, texel_type::RGBA8ui}, convert<bgr_8i, rgba_8ui>},
    {{texel_type::BGR8ui, texel_type::RGBA8ui}, convert<bgr_8ui, rgba_8ui>},
    {{texel_type::BGR8snorm, texel_type::RGBA8ui}, convert<bgr_8snorm, rgba_8ui>},

    {{texel_type::RGBA8, texel_type::RGBA8ui}, convert<rgba_8unorm, rgba_8ui>},
    {{texel_type::RGBA8snorm, texel_type::RGBA8ui}, convert<rgba_8snorm, rgba_8ui>},
    {{texel_type::RGBA8ui, texel_type::RGBA8ui}, convert<rgba_8ui, rgba_8ui>},
    {{texel_type::RGBA8i, texel_type::RGBA8ui}, convert<rgba_8i, rgba_8ui>},

    {{texel_type::BGRA8, texel_type::RGBA8ui}, convert<bgra_8unorm, rgba_8ui>},
    {{texel_type::BGRA8snorm, texel_type::RGBA8ui}, convert<bgra_8snorm, rgba_8ui>},
    {{texel_type::BGRA8ui, texel_type::RGBA8ui}, convert<bgra_8ui, rgba_8ui>},
    {{texel_type::BGRA8i, texel_type::RGBA8ui}, convert<bgra_8i, rgba_8ui>},

    {{texel_type::ABGR8, texel_type::RGBA8ui}, convert<abgr_8unorm, rgba_8ui>},
    {{texel_type::ABGR8snorm, texel_type::RGBA8ui}, convert<abgr_8snorm, rgba_8ui>},
    {{texel_type::ABGR8ui, texel_type::RGBA8ui}, convert<abgr_8ui, rgba_8ui>},
    {{texel_type::ABGR8i, texel_type::RGBA8ui}, convert<abgr_8i, rgba_8ui>},

    {{texel_type::X8D24, texel_type::RGBA8ui}, convert<x8d24_unorm, rgba_8ui>},
    {{texel_type::D24, texel_type::RGBA8ui}, convert<d24_unorm, rgba_8ui>},

    {{texel_type::RGB10A2, texel_type::RGBA8ui}, convert<rgb10a2_unorm, rgba_8ui>},
    {{texel_type::RGB10A2ui, texel_type::RGBA8ui}, convert<rgb10a2_ui, rgba_8ui>},
    {{texel_type::BGR10A2, texel_type::RGBA8ui}, convert<bgr10a2_unorm, rgba_8ui>},
    {{texel_type::RG11B10f, texel_type::RGBA8ui}, convert<rg11b10_f, rgba_8ui>},
    {{texel_type::B10GR11f, texel_type::RGBA8ui}, convert<rg11b10_f, rgba_8ui>},

    {{texel_type::RG32ui, texel_type::RGBA8ui}, convert<rg_32ui, rgba_8ui>},
    {{texel_type::RG32i, texel_type::RGBA8ui}, convert<rg_32i, rgba_8ui>},
    {{texel_type::RG32f, texel_type::RGBA8ui}, convert<rg_32f, rgba_8ui>},

    {{texel_type::RGBA16, texel_type::RGBA8ui}, convert<rgba_16unorm, rgba_8ui>},
    {{texel_type::RGBA16snorm, texel_type::RGBA8ui}, convert<rgba_16snorm, rgba_8ui>},
    {{texel_type::RGBA16ui, texel_type::RGBA8ui}, convert<rgba_16ui, rgba_8ui>},
    {{texel_type::RGBA16i, texel_type::RGBA8ui}, convert<rgba_16i, rgba_8ui>},
    {{texel_type::RGBA16f, texel_type::RGBA8ui}, convert<rgba_16f, rgba_8ui>},

    {{texel_type::RGBA32ui, texel_type::RGBA8ui}, convert<rgba_32ui, rgba_8ui>},
    {{texel_type::RGBA32i, texel_type::RGBA8ui}, convert<rgba_32i, rgba_8ui>},
    {{texel_type::RGBA32f, texel_type::RGBA8ui}, convert<rgba_32f, rgba_8ui>},

    // conversions used by normalization function:

    {{texel_type::R16f, texel_type::R32f}, convert<r_16f, r_32f>},
    {{texel_type::RG16f, texel_type::RG32f}, convert<rg_16f, rg_32f>},
    {{texel_type::RG11B10f, texel_type::RGBA32f}, convert<rg11b10_f, rgba_32f>},
    {{texel_type::B10GR11f, texel_type::RGBA32f}, convert<b10gr11_f, rgba_32f>},
    {{texel_type::RGBA16f, texel_type::RGBA32f}, convert<rgba_16f, rgba_32f>},
};

conversion_function get_converter(texel_type input_type, texel_type output_type) {
  auto it = converter_map.find(std::make_pair(input_type, output_type));
  if (it != converter_map.end()) {
    return it->second;
  }
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

template <typename CF, int Comp1, int Comp2, int Comp3, int Comp4>
struct min_max_updater {
  static void update(typename CF::type* value,
                     typename CF::min_max_type& min_val,
                     typename CF::min_max_type& max_val) {
    if (std::min({value[Comp1], value[Comp2], value[Comp3], value[Comp4]}) < min_val) {
      min_val = std::min({value[Comp1], value[Comp2], value[Comp3], value[Comp4]});
    }
    if (std::max({value[Comp1], value[Comp2], value[Comp3], value[Comp4]}) > max_val) {
      max_val = std::max({value[Comp1], value[Comp2], value[Comp3], value[Comp4]});
    }
  }
};

template <int Comp1, int Comp2, int Comp3, int Comp4>
struct min_max_updater<float_16, Comp1, Comp2, Comp3, Comp4> {
  static void update(float_16::type* value,
                     float_16::min_max_type& min_val,
                     float_16::min_max_type& max_val) {
    float_16::min_max_type val1 = convert_half_to_single_float(value[Comp1]);
    float_16::min_max_type val2 = convert_half_to_single_float(value[Comp2]);
    float_16::min_max_type val3 = convert_half_to_single_float(value[Comp3]);
    float_16::min_max_type val4 = convert_half_to_single_float(value[Comp4]);
    if (std::min({val1, val2, val3, val4}) < min_val) {
      min_val = std::min({val1, val2, val3, val4});
    }
    if (std::max({val1, val2, val3, val4}) > max_val) {
      max_val = std::max({val1, val2, val3, val4});
    }
  }
};

template <typename CF, int Comp1>
struct min_max_updater<CF, Comp1, NA, NA, NA> {
  static void update(typename CF::type* value,
                     typename CF::min_max_type& min_val,
                     typename CF::min_max_type& max_val) {
    min_val = std::min(value[Comp1], min_val);
    max_val = std::max(value[Comp1], max_val);
  }
};

template <int Comp1>
struct min_max_updater<unorm_24, Comp1, NA, NA, NA> {
  static void update(unorm_24::type* value,
                     unorm_24::min_max_type& min_val,
                     unorm_24::min_max_type& max_val) {
    unorm_24::min_max_type val = value[Comp1] & unorm_24::one_value;
    min_val = std::min(val, min_val);
    max_val = std::max(val, max_val);
  }
};

template <int Comp1>
struct min_max_updater<float_16, Comp1, NA, NA, NA> {
  static void update(float_16::type* value,
                     float_16::min_max_type& min_val,
                     float_16::min_max_type& max_val) {
    float_16::min_max_type val = convert_half_to_single_float(value[Comp1]);
    min_val = std::min(val, min_val);
    max_val = std::max(val, max_val);
  }
};

template <typename CF, int Comp1, int Comp2>
struct min_max_updater<CF, Comp1, Comp2, NA, NA> {
  static void update(typename CF::type* value,
                     typename CF::min_max_type& min_val,
                     typename CF::min_max_type& max_val) {
    if (std::min(value[Comp1], value[Comp2]) < min_val) {
      min_val = std::min(value[Comp1], value[Comp2]);
    }
    if (std::max(value[Comp1], value[Comp2]) > max_val) {
      max_val = std::max(value[Comp1], value[Comp2]);
    }
  }
};

template <int Comp1, int Comp2>
struct min_max_updater<float_16, Comp1, Comp2, NA, NA> {
  static void update(float_16::type* value,
                     float_16::min_max_type& min_val,
                     float_16::min_max_type& max_val) {
    float_16::min_max_type val1 = convert_half_to_single_float(value[Comp1]);
    float_16::min_max_type val2 = convert_half_to_single_float(value[Comp2]);
    if (std::min(val1, val2) < min_val) {
      min_val = std::min(val1, val2);
    }
    if (std::max(val1, val2) > max_val) {
      max_val = std::max(val1, val2);
    }
  }
};

template <typename T>
void find_min_and_max(std::vector<uint8_t>& data,
                      int width,
                      int height,
                      typename T::comp_fmt::min_max_type& min_value,
                      typename T::comp_fmt::min_max_type& max_value) {
  using comp_type = typename T::comp_fmt::type;

  const auto size = T::size_in_bytes * width * height;
  for (auto i = 0; i < size; i += T::size_in_bytes) {
    comp_type* ptr = reinterpret_cast<comp_type*>(&data[i]);
    min_max_updater<typename T::comp_fmt, T::r, T::g, T::b, T::a>::update(ptr, min_value,
                                                                          max_value);
  }
}

template <typename CF, int Comp1, int Comp2, int Comp3, int Comp4>
struct color_scaler {
  static void scale(typename CF::type* value,
                    typename CF::min_max_type min_val,
                    typename CF::min_max_type max_val) {
    value[Comp1] = (value[Comp1] - min_val) / (max_val - min_val);
    value[Comp2] = (value[Comp2] - min_val) / (max_val - min_val);
    value[Comp3] = (value[Comp3] - min_val) / (max_val - min_val);
    value[Comp4] = (value[Comp4] - min_val) / (max_val - min_val);
  }
};

template <int Comp1, int Comp2, int Comp3, int Comp4>
struct color_scaler<float_16, Comp1, Comp2, Comp3, Comp4> {
  static void scale(float_16::type* value,
                    float_16::min_max_type min_val,
                    float_16::min_max_type max_val) {
    float_16::min_max_type val1 = convert_half_to_single_float(value[Comp1]);
    val1 = (val1 - min_val) / (max_val - min_val);
    value[Comp1] = convert_single_to_half_float(val1);

    float_16::min_max_type val2 = convert_half_to_single_float(value[Comp2]);
    val2 = (val2 - min_val) / (max_val - min_val);
    value[Comp2] = convert_single_to_half_float(val2);

    float_16::min_max_type val3 = convert_half_to_single_float(value[Comp3]);
    val3 = (val3 - min_val) / (max_val - min_val);
    value[Comp3] = convert_single_to_half_float(val3);

    float_16::min_max_type val4 = convert_half_to_single_float(value[Comp4]);
    val4 = (val4 - min_val) / (max_val - min_val);
    value[Comp4] = convert_single_to_half_float(val4);
  }
};

template <int Comp1, int Comp2, int Comp3, int Comp4>
struct color_scaler<unorm_8, Comp1, Comp2, Comp3, Comp4> {
  static void scale(typename unorm_8::type* value,
                    typename unorm_8::min_max_type min_val,
                    typename unorm_8::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_8::type>(255.0f * (value[Comp1] - min_val) / (max_val - min_val));
    value[Comp2] =
        static_cast<unorm_8::type>(255.0f * (value[Comp2] - min_val) / (max_val - min_val));
    value[Comp3] =
        static_cast<unorm_8::type>(255.0f * (value[Comp3] - min_val) / (max_val - min_val));
    value[Comp4] =
        static_cast<unorm_8::type>(255.0f * (value[Comp4] - min_val) / (max_val - min_val));
  }
};

template <int Comp1, int Comp2, int Comp3, int Comp4>
struct color_scaler<unorm_24, Comp1, Comp2, Comp3, Comp4> {
  static void scale(typename unorm_24::type* value,
                    typename unorm_24::min_max_type min_val,
                    typename unorm_24::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_24::type>(16777215.0f * (value[Comp1] - min_val) / (max_val - min_val));
    value[Comp2] =
        static_cast<unorm_24::type>(16777215.0f * (value[Comp2] - min_val) / (max_val - min_val));
    value[Comp3] =
        static_cast<unorm_24::type>(16777215.0f * (value[Comp3] - min_val) / (max_val - min_val));
    value[Comp4] =
        static_cast<unorm_24::type>(16777215.0f * (value[Comp4] - min_val) / (max_val - min_val));
  }
};

template <int Comp1, int Comp2, int Comp3, int Comp4>
struct color_scaler<unorm_16, Comp1, Comp2, Comp3, Comp4> {
  static void scale(typename unorm_16::type* value,
                    typename unorm_16::min_max_type min_val,
                    typename unorm_16::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp1] - min_val) / (max_val - min_val));
    value[Comp2] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp2] - min_val) / (max_val - min_val));
    value[Comp3] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp3] - min_val) / (max_val - min_val));
    value[Comp4] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp4] - min_val) / (max_val - min_val));
  }
};

template <typename CF, int Comp1>
struct color_scaler<CF, Comp1, NA, NA, NA> {
  static void scale(typename CF::type* value,
                    typename CF::min_max_type min_val,
                    typename CF::min_max_type max_val) {
    value[Comp1] = (value[Comp1] - min_val) / (max_val - min_val);
  }
};

template <int Comp1>
struct color_scaler<float_16, Comp1, NA, NA, NA> {
  static void scale(float_16::type* value,
                    float_16::min_max_type min_val,
                    float_16::min_max_type max_val) {
    float_16::min_max_type val1 = convert_half_to_single_float(value[Comp1]);
    val1 = (val1 - min_val) / (max_val - min_val);
    value[Comp1] = convert_single_to_half_float(val1);
  }
};

template <int Comp1>
struct color_scaler<unorm_8, Comp1, NA, NA, NA> {
  static void scale(typename unorm_8::type* value,
                    typename unorm_8::min_max_type min_val,
                    typename unorm_8::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_8::type>(255.0f * (value[Comp1] - min_val) / (max_val - min_val));
  }
};

template <int Comp1>
struct color_scaler<unorm_24, Comp1, NA, NA, NA> {
  static void scale(typename unorm_24::type* value,
                    typename unorm_24::min_max_type min_val,
                    typename unorm_24::min_max_type max_val) {
    value[Comp1] = static_cast<unorm_24::type>(
        16777215.0f * ((value[Comp1] & unorm_24::one_value) - min_val) / (max_val - min_val));
  }
};

template <int Comp1>
struct color_scaler<unorm_16, Comp1, NA, NA, NA> {
  static void scale(typename unorm_16::type* value,
                    typename unorm_16::min_max_type min_val,
                    typename unorm_16::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp1] - min_val) / (max_val - min_val));
  }
};

template <typename CF, int Comp1, int Comp2>
struct color_scaler<CF, Comp1, Comp2, NA, NA> {
  static void scale(typename CF::type* value,
                    typename CF::min_max_type min_val,
                    typename CF::min_max_type max_val) {
    value[Comp1] = (value[Comp1] - min_val) / (max_val - min_val);
    value[Comp2] = (value[Comp2] - min_val) / (max_val - min_val);
  }
};

template <int Comp1, int Comp2>
struct color_scaler<float_16, Comp1, Comp2, NA, NA> {
  static void scale(float_16::type* value,
                    float_16::min_max_type min_val,
                    float_16::min_max_type max_val) {
    float_16::min_max_type val1 = convert_half_to_single_float(value[Comp1]);
    float_16::min_max_type val2 = convert_half_to_single_float(value[Comp2]);
    val1 = (val1 - min_val) / (max_val - min_val);
    val2 = (val2 - min_val) / (max_val - min_val);
    value[Comp1] = convert_single_to_half_float(val1);
    value[Comp2] = convert_single_to_half_float(val2);
  }
};

template <int Comp1, int Comp2>
struct color_scaler<unorm_8, Comp1, Comp2, NA, NA> {
  static void scale(typename unorm_8::type* value,
                    typename unorm_8::min_max_type min_val,
                    typename unorm_8::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_8::type>(255.0f * (value[Comp1] - min_val) / (max_val - min_val));
    value[Comp2] =
        static_cast<unorm_8::type>(255.0f * (value[Comp2] - min_val) / (max_val - min_val));
  }
};

template <int Comp1, int Comp2>
struct color_scaler<unorm_24, Comp1, Comp2, NA, NA> {
  static void scale(typename unorm_24::type* value,
                    typename unorm_24::min_max_type min_val,
                    typename unorm_24::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_24::type>(16777215.0f * (value[Comp1] - min_val) / (max_val - min_val));
    value[Comp2] =
        static_cast<unorm_24::type>(16777215.0f * (value[Comp2] - min_val) / (max_val - min_val));
  }
};

template <int Comp1, int Comp2>
struct color_scaler<unorm_16, Comp1, Comp2, NA, NA> {
  static void scale(typename unorm_16::type* value,
                    typename unorm_16::min_max_type min_val,
                    typename unorm_16::min_max_type max_val) {
    value[Comp1] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp1] - min_val) / (max_val - min_val));
    value[Comp2] =
        static_cast<unorm_16::type>(65535.0f * (value[Comp2] - min_val) / (max_val - min_val));
  }
};

template <typename T>
void scale_colors(std::vector<uint8_t>& data,
                  int width,
                  int height,
                  typename T::comp_fmt::min_max_type min_value,
                  typename T::comp_fmt::min_max_type max_value) {
  using comp_type = typename T::comp_fmt::type;

  const auto size = T::size_in_bytes * width * height;
  for (auto i = 0; i < size; i += T::size_in_bytes) {
    comp_type* ptr = reinterpret_cast<comp_type*>(&data[i]);
    color_scaler<typename T::comp_fmt, T::r, T::g, T::b, T::a>::scale(ptr, min_value, max_value);
  }
}

template <typename T>
void normalize(std::vector<uint8_t>& data, int width, int height) {
  if (STATIC_TEST(T::is_packed)) {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }

  using min_max_t = typename T::comp_fmt::min_max_type;
  min_max_t min_value = static_cast<min_max_t>(T::comp_fmt::max_value);
  min_max_t max_value = static_cast<min_max_t>(T::comp_fmt::min_value);
  find_min_and_max<T>(data, width, height, min_value, max_value);

  if (min_value == max_value) {
    return;
  }
  scale_colors<T>(data, width, height, min_value, max_value);
}

using normalization_function = void (*)(std::vector<uint8_t>& data, int width, int height);

std::map<texel_type, normalization_function> normalizer_map = {
    {texel_type::R8, normalize<r_8unorm>},        {texel_type::R32f, normalize<r_32f>},
    {texel_type::RG32f, normalize<rg_32f>},       {texel_type::RGBA32f, normalize<rgba_32f>},
    {texel_type::R16f, normalize<r_16f>},         {texel_type::RG16f, normalize<rg_16f>},
    {texel_type::RG11B10f, normalize<rg11b10_f>}, {texel_type::B10GR11f, normalize<b10gr11_f>},
    {texel_type::RGBA16f, normalize<rgba_16f>},   {texel_type::R16, normalize<r_16unorm>},
    {texel_type::D24, normalize<d24_unorm>}};

normalization_function get_normalizer(texel_type type) {
  auto it = normalizer_map.find(type);
  if (it != normalizer_map.end()) {
    return it->second;
  }
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

template <typename T>
std::pair<double, double> min_max(std::vector<uint8_t>& data, int width, int height) {
  if (STATIC_TEST(T::is_packed)) {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }

  using min_max_t = typename T::comp_fmt::min_max_type;
  min_max_t min_value = static_cast<min_max_t>(T::comp_fmt::max_value);
  min_max_t max_value = static_cast<min_max_t>(T::comp_fmt::min_value);
  find_min_and_max<T>(data, width, height, min_value, max_value);
  return std::make_pair((double)min_value, (double)max_value);
}

using min_max_function = std::pair<double, double> (*)(std::vector<uint8_t>& data,
                                                       int width,
                                                       int height);

std::map<texel_type, min_max_function> min_max_map = {
    {texel_type::R8, min_max<r_8unorm>},        {texel_type::R32f, min_max<r_32f>},
    {texel_type::RG32f, min_max<rg_32f>},       {texel_type::RGBA32f, min_max<rgba_32f>},
    {texel_type::R16f, min_max<r_16f>},         {texel_type::RG16f, min_max<rg_16f>},
    {texel_type::RG11B10f, min_max<rg11b10_f>}, {texel_type::B10GR11f, min_max<b10gr11_f>},
    {texel_type::RGBA16f, min_max<rgba_16f>},   {texel_type::R16, min_max<r_16unorm>},
    {texel_type::D24, min_max<d24_unorm>}};

min_max_function get_min_max(texel_type type) {
  auto it = min_max_map.find(type);
  if (it != min_max_map.end()) {
    return it->second;
  }
  throw ENotImplemented(EXCEPTION_MESSAGE);
}

} // namespace texture_converter
} // namespace gits

void gits::convert_texture_data(texel_type input_type,
                                const std::vector<uint8_t>& input_data,
                                texel_type output_type,
                                std::vector<uint8_t>& output_data,
                                int width,
                                int height) {
  texture_converter::conversion_function converter =
      texture_converter::get_converter(input_type, output_type);
  converter(input_data, output_data, width, height);
}

void gits::normalize_texture_data(texel_type type,
                                  std::vector<uint8_t>& data,
                                  int width,
                                  int height) {
  auto normalizer = texture_converter::get_normalizer(type);
  normalizer(data, width, height);
}

std::pair<double, double> gits::get_min_max_values(texel_type type,
                                                   std::vector<uint8_t>& data,
                                                   int width,
                                                   int height) {
  auto min_max_func = texture_converter::get_min_max(type);
  return min_max_func(data, width, height);
}
