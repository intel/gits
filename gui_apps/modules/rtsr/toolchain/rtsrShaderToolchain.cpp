// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/toolchain/rtsrShaderToolchain.h"

#include <dxil_spirv_c.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <SPIRV/GlslangToSpv.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

#if defined(RTSR_HAS_DXC)
#include <windows.h>
#include <ole2.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <wrl/client.h>
#endif

namespace gits::rtsr {
namespace {

void Require(bool condition, const char* diagnostic) {
  if (!condition) {
    throw std::runtime_error(diagnostic);
  }
}

uint32_t ReadWord(const std::vector<uint8_t>& bytes, size_t offset) {
  Require(offset <= bytes.size() && bytes.size() - offset >= sizeof(uint32_t),
          "Truncated shader container.");
  uint32_t word{};
  std::memcpy(&word, bytes.data() + offset, sizeof(word));
  return word;
}

void ValidateContainer(const std::vector<uint8_t>& bytes, bool needsDxil) {
  Require(bytes.size() >= 32 && ReadWord(bytes, 0) == 0x43425844,
          "Expected a DXBC shader container.");
  Require(ReadWord(bytes, 24) == bytes.size(), "Invalid shader container size.");
  const size_t chunks = ReadWord(bytes, 28);
  Require(chunks <= (bytes.size() - 32) / 4, "Truncated shader chunk table.");
  bool hasDxil = false;
  for (size_t i = 0; i < chunks; ++i) {
    const size_t offset = ReadWord(bytes, 32 + i * 4);
    Require(offset >= 32 + chunks * 4 && offset <= bytes.size() - 8,
            "Invalid shader chunk offset.");
    const size_t size = ReadWord(bytes, offset + 4);
    Require(size <= bytes.size() - offset - 8, "Truncated shader chunk.");
    if (needsDxil && ReadWord(bytes, offset) == 0x4C495844) {
      Require(size >= 24, "Truncated DXIL program header.");
      const size_t program = offset + 8;
      Require(size % 4 == 0 && ReadWord(bytes, program + 4) == size / 4,
              "Invalid DXIL program size.");
      Require(ReadWord(bytes, program + 8) == 0x4C495844, "Invalid DXIL program header.");
      const size_t bitcodeOffset = ReadWord(bytes, program + 16);
      const size_t bitcodeSize = ReadWord(bytes, program + 20);
      Require(bitcodeOffset % 4 == 0 && bitcodeOffset >= 16 && bitcodeOffset <= size - 8 &&
                  bitcodeSize >= 4 && bitcodeSize <= size - 8 - bitcodeOffset,
              "Invalid DXIL bitcode range.");
      Require(ReadWord(bytes, program + 8 + bitcodeOffset) == 0xDEC04342,
              "Invalid DXIL bitcode signature.");
      hasDxil = true;
    }
  }
  Require(!needsDxil || hasDxil, "The shader container has no DXIL program.");
}

std::vector<uint32_t> SpirvWords(const void* data, size_t size) {
  Require(data != nullptr && size >= 20 && size % sizeof(uint32_t) == 0,
          "Invalid SPIR-V bytecode size.");
  std::vector<uint32_t> words(size / sizeof(uint32_t));
  std::memcpy(words.data(), data, size);
  Require(words[4] == 0, "Invalid SPIR-V schema.");
  // SPIRV-Cross checks the magic, version, ID bound and instruction lengths,
  // including byte swapping valid big-endian modules before parsing them.
  return words;
}

struct ShaderSemantic {
  bool Input{};
  uint32_t Location{};
  uint32_t Component{};
  std::string Name;
};

ShaderSource ToGlsl(std::vector<uint32_t> words, const std::string& entrypoint) {
  spirv_cross::CompilerGLSL compiler(std::move(words));
  const auto entries = compiler.get_entry_points_and_stages();
  auto selected = std::find_if(entries.begin(), entries.end(),
                               [&](const auto& entry) { return entry.name == entrypoint; });
  if (selected == entries.end() && entries.size() == 1) {
    selected = entries.begin();
  }
  Require(selected != entries.end(), "Could not identify the SPIR-V entrypoint.");
  compiler.set_entry_point(selected->name, selected->execution_model);

  auto options = compiler.get_common_options();
  options.es = false;
  options.version = 450;
  options.vulkan_semantics = true;
  compiler.set_common_options(options);

  ShaderSource result{SourceKind::Glsl, compiler.compile(),
                      "Reconstructed GLSL; not the original shader source."};
  // GLSL source always exposes main even when the SPIR-V entrypoint carried a
  // different name; the replacement stage is patched to this compiled name.
  result.EntrypointName = "main";
  Require(!result.Text.empty(), "SPIRV-Cross returned empty GLSL.");
  return result;
}

ShaderSource ToHlsl(std::vector<uint32_t> words,
                    const std::string& entrypoint,
                    size_t model,
                    const std::vector<ShaderSemantic>& semantics = {}) {
  spirv_cross::CompilerHLSL compiler(std::move(words));
  const auto entries = compiler.get_entry_points_and_stages();
  auto selected = std::find_if(entries.begin(), entries.end(),
                               [&](const auto& entry) { return entry.name == entrypoint; });
  if (selected == entries.end() && entries.size() == 1) {
    selected = entries.begin();
  }
  Require(selected != entries.end(), "Could not identify the shader entrypoint.");
  switch (selected->execution_model) {
  case spv::ExecutionModelVertex:
  case spv::ExecutionModelTessellationControl:
  case spv::ExecutionModelTessellationEvaluation:
  case spv::ExecutionModelGeometry:
  case spv::ExecutionModelFragment:
  case spv::ExecutionModelGLCompute:
    break;
  default:
    throw std::runtime_error("This shader stage is outside the source viewer's scope.");
  }
  compiler.set_entry_point(selected->name, selected->execution_model);

  // dxil-spirv preserves each DXIL signature register as SPIR-V
  // Location/Component, but SPIRV-Cross otherwise renames it to TEXCOORD<n>.
  // Feed the original semantic back before emitting HLSL so recompiling a
  // single stage preserves its linkage to the untouched neighbouring stage.
  const auto resources = compiler.get_shader_resources();
  const auto applySemantics = [&compiler, &semantics](const auto& stageResources, bool input) {
    for (const auto& resource : stageResources) {
      if (!compiler.has_decoration(resource.id, spv::DecorationLocation)) {
        continue;
      }
      const uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
      const uint32_t component =
          compiler.has_decoration(resource.id, spv::DecorationComponent)
              ? compiler.get_decoration(resource.id, spv::DecorationComponent)
              : 0;
      const auto semantic =
          std::find_if(semantics.begin(), semantics.end(), [&](const ShaderSemantic& candidate) {
            return candidate.Input == input && candidate.Location == location &&
                   candidate.Component == component;
          });
      if (semantic != semantics.end()) {
        compiler.set_decoration_string(resource.id, spv::DecorationUserSemantic, semantic->Name);
      }
    }
  };
  applySemantics(resources.stage_inputs, true);
  applySemantics(resources.stage_outputs, false);

  Require(model <= std::numeric_limits<uint32_t>::max(), "Invalid shader model.");
  auto options = compiler.get_hlsl_options();
  options.shader_model = model == 0 ? 60 : static_cast<uint32_t>(model);
  options.use_entry_point_name = true;
  options.user_semantic = !semantics.empty();
  compiler.set_hlsl_options(options);
  ShaderSource result{SourceKind::Hlsl, compiler.compile(),
                      "Reconstructed HLSL; not the original shader source."};
  result.EntrypointName = selected->name;
  Require(!result.Text.empty(), "SPIRV-Cross returned empty HLSL.");
  if (model == 0) {
    result.Diagnostic += " Shader model was not provided; targeting 6.0.";
  }
  return result;
}

// The upstream callback is thread-local, but the diagnostic belongs to this
// request. Clear the callback before its userdata leaves scope.
class DxilConversion {
public:
  DxilConversion() {
    dxil_spv_begin_thread_allocator_context();
    dxil_spv_set_thread_log_callback(Log, &m_Diagnostic);
  }

  ~DxilConversion() {
    dxil_spv_converter_free(m_Converter);
    dxil_spv_parsed_blob_free(m_Parsed);
    dxil_spv_set_thread_log_callback(nullptr, nullptr);
    dxil_spv_end_thread_allocator_context();
  }

  DxilConversion(const DxilConversion&) = delete;
  DxilConversion& operator=(const DxilConversion&) = delete;

  ShaderSource Run(const SessionStore::ShaderBlob& blob) {
    Check(dxil_spv_parse_dxil_blob(blob.Bytecode.data(), blob.Bytecode.size(), &m_Parsed),
          "DXIL parsing failed");
    Check(dxil_spv_create_converter(m_Parsed, &m_Converter), "DXIL converter creation failed");
    dxil_spv_converter_set_stage_input_remapper(m_Converter, RemapInput, this);
    dxil_spv_converter_set_stage_output_remapper(m_Converter, RemapOutput, this);

    unsigned entryIndex = 0;
    unsigned entryCount = 0;
    Check(dxil_spv_parsed_blob_get_num_entry_points(m_Parsed, &entryCount),
          "DXIL entrypoint enumeration failed");
    if (dxil_spv_parsed_blob_get_entry_index_by_name(m_Parsed, blob.EntrypointName.c_str(),
                                                     &entryIndex) != DXIL_SPV_SUCCESS) {
      Require(entryCount == 1, "Could not identify the DXIL entrypoint.");
      entryIndex = 0;
    }
    const char* entry = nullptr;
    Check(dxil_spv_parsed_blob_get_entry_point_name(m_Parsed, entryIndex, &entry),
          "DXIL entrypoint lookup failed");
    const auto stage = dxil_spv_parsed_blob_get_shader_stage_for_entry(m_Parsed, entry);
    Require(stage >= DXIL_SPV_STAGE_VERTEX && stage <= DXIL_SPV_STAGE_COMPUTE,
            "This DXIL stage is outside the source viewer's scope.");
    dxil_spv_converter_set_entry_point(m_Converter, entry);
    Check(dxil_spv_converter_run(m_Converter), "DXIL to SPIR-V conversion failed");
    dxil_spv_compiled_spirv compiled{};
    Check(dxil_spv_converter_get_compiled_spirv(m_Converter, &compiled),
          "DXIL converter returned no SPIR-V");
    const char* compiledEntry = nullptr;
    Check(dxil_spv_converter_get_compiled_entry_point(m_Converter, &compiledEntry),
          "DXIL converter returned no entrypoint");
    auto result = ToHlsl(SpirvWords(compiled.data, compiled.size),
                         compiledEntry ? compiledEntry : "", blob.D3DShaderVersion, m_Semantics);
    if (!m_Diagnostic.empty()) {
      result.Diagnostic += "\n" + m_Diagnostic;
    }
    return result;
  }

private:
  static dxil_spv_bool RemapInput(void* userdata,
                                  const dxil_spv_d3d_shader_stage_io* d3d,
                                  dxil_spv_vulkan_shader_stage_io* spirv) noexcept {
    return static_cast<DxilConversion*>(userdata)->RememberSemantic(true, d3d, spirv);
  }

  static dxil_spv_bool RemapOutput(void* userdata,
                                   const dxil_spv_d3d_shader_stage_io* d3d,
                                   dxil_spv_vulkan_shader_stage_io* spirv) noexcept {
    return static_cast<DxilConversion*>(userdata)->RememberSemantic(false, d3d, spirv);
  }

  dxil_spv_bool RememberSemantic(bool input,
                                 const dxil_spv_d3d_shader_stage_io* d3d,
                                 const dxil_spv_vulkan_shader_stage_io* spirv) noexcept {
    if (d3d == nullptr || d3d->semantic == nullptr || spirv == nullptr) {
      return DXIL_SPV_FALSE;
    }
    try {
      m_Semantics.push_back({input, spirv->location, spirv->component,
                             std::string(d3d->semantic) + std::to_string(d3d->semantic_index)});
      return DXIL_SPV_TRUE;
    } catch (...) {
      return DXIL_SPV_FALSE;
    }
  }

  static void Log(void* userdata, dxil_spv_log_level level, const char* message) noexcept {
    if (level == DXIL_SPV_LOG_LEVEL_DEBUG || message == nullptr) {
      return;
    }
    try {
      static_cast<std::string*>(userdata)->append(message);
    } catch (...) {
      // Never unwind through the C diagnostic callback on allocation failure.
    }
  }

  void Check(dxil_spv_result result, const char* operation) const {
    if (result != DXIL_SPV_SUCCESS) {
      throw std::runtime_error(std::string(operation) + " (" + std::to_string(result) + ").\n" +
                               m_Diagnostic);
    }
  }

  std::string m_Diagnostic;
  std::vector<ShaderSemantic> m_Semantics;
  dxil_spv_parsed_blob m_Parsed{};
  dxil_spv_converter m_Converter{};
};

#if defined(RTSR_HAS_DXC)
class DxcLibrary {
public:
  explicit DxcLibrary(const std::filesystem::path& playerPath) {
    Require(!playerPath.empty(), "DXC requires a configured GITS player path.");
    const auto directory = std::filesystem::absolute(playerPath).parent_path() / "D3D12";
    const auto dxilPath = directory / "dxil.dll";
    const auto compilerPath = directory / "dxcompiler.dll";

    // dxcompiler loads the signing/validation library by module name at the
    // point Compile is called, rather than as a static dependency. Preload it
    // from the same trusted directory so Studio's working directory cannot
    // decide whether the resulting DXIL is signed.
    HMODULE dxil =
        LoadLibraryExW(dxilPath.c_str(), nullptr,
                       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!dxil) {
      const DWORD error = GetLastError();
      throw std::runtime_error("Could not load " + dxilPath.string() + " (Windows error " +
                               std::to_string(error) + ").");
    }

    HMODULE compiler =
        LoadLibraryExW(compilerPath.c_str(), nullptr,
                       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!compiler) {
      const DWORD error = GetLastError();
      FreeLibrary(dxil);
      throw std::runtime_error("Could not load " + compilerPath.string() + " (Windows error " +
                               std::to_string(error) + ").");
    }

    m_Dxil = dxil;
    m_Compiler = compiler;
  }

  ~DxcLibrary() {
    FreeLibrary(m_Compiler);
    FreeLibrary(m_Dxil);
  }

  DxcLibrary(const DxcLibrary&) = delete;
  DxcLibrary& operator=(const DxcLibrary&) = delete;

  DxcCreateInstanceProc CreateInstance() const {
    const auto create =
        reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(m_Compiler, "DxcCreateInstance"));
    Require(create != nullptr, "DXC does not export DxcCreateInstance.");
    return create;
  }

private:
  HMODULE m_Dxil{};
  HMODULE m_Compiler{};
};

std::wstring Utf8ToWide(const std::string& text) {
  Require(!text.empty(), "The shader entrypoint is empty.");
  Require(text.size() <= static_cast<size_t>(std::numeric_limits<int>::max()),
          "The shader entrypoint is too long.");
  const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0);
  Require(length > 0, "The shader entrypoint is not valid UTF-8.");
  std::wstring result(static_cast<size_t>(length), L'\0');
  Require(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                              static_cast<int>(text.size()), result.data(), length) == length,
          "The shader entrypoint could not be converted to UTF-16.");
  return result;
}

std::wstring TargetProfile(RTSR_SHADER_STAGE stage, size_t model) {
  Require(model / 10 == 6, "A valid DXIL shader model is required for replacement.");

  const wchar_t* prefix{};
  switch (stage) {
  case RTSR_SHADER_STAGE_VERTEX:
    prefix = L"vs_";
    break;
  case RTSR_SHADER_STAGE_TESSELLATION_HULL:
    prefix = L"hs_";
    break;
  case RTSR_SHADER_STAGE_TESSELLATION_DOMAIN:
    prefix = L"ds_";
    break;
  case RTSR_SHADER_STAGE_GEOMETRY:
    prefix = L"gs_";
    break;
  case RTSR_SHADER_STAGE_PIXEL:
    prefix = L"ps_";
    break;
  case RTSR_SHADER_STAGE_COMPUTE:
    prefix = L"cs_";
    break;
  default:
    throw std::runtime_error("This shader stage cannot be compiled for a D3D12 pipeline.");
  }

  return std::wstring(prefix) + std::to_wstring(model / 10) + L"_" + std::to_wstring(model % 10);
}

std::string DxcDiagnostics(IDxcResult* result) {
  if (result == nullptr) {
    return {};
  }
  Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
  if (FAILED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr)) || !errors ||
      errors->GetStringLength() == 0) {
    return {};
  }
  return std::string(errors->GetStringPointer(), errors->GetStringLength());
}

void CheckDxcResult(IDxcOperationResult* result, const char* operation) {
  Require(result != nullptr, "DXC returned no operation result.");
  HRESULT status = E_FAIL;
  Require(SUCCEEDED(result->GetStatus(&status)), "Could not read the DXC result status.");
  if (SUCCEEDED(status)) {
    return;
  }

  std::string diagnostic =
      std::string(operation) + " failed (HRESULT " + std::to_string(status) + ").";
  Microsoft::WRL::ComPtr<IDxcBlobEncoding> errors;
  if (SUCCEEDED(result->GetErrorBuffer(&errors)) && errors && errors->GetBufferSize() != 0) {
    diagnostic += "\n";
    diagnostic.append(static_cast<const char*>(errors->GetBufferPointer()),
                      errors->GetBufferSize());
    while (!diagnostic.empty() && diagnostic.back() == '\0') {
      diagnostic.pop_back();
    }
  }
  throw std::runtime_error(diagnostic);
}

struct SignatureParameter {
  std::string Semantic;
  uint32_t SemanticIndex{};
  uint32_t Register{};
  uint32_t Stream{};
  D3D_NAME SystemValue{D3D_NAME_UNDEFINED};
  D3D_REGISTER_COMPONENT_TYPE ComponentType{D3D_REGISTER_COMPONENT_UNKNOWN};
  D3D_MIN_PRECISION MinPrecision{D3D_MIN_PRECISION_DEFAULT};
  uint8_t Mask{};
};

struct ShaderSignature {
  std::vector<SignatureParameter> Inputs;
  std::vector<SignatureParameter> Outputs;
  std::vector<SignatureParameter> PatchConstants;
};

std::string Uppercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
    return static_cast<char>(std::toupper(character));
  });
  return text;
}

SignatureParameter CopySignatureParameter(const D3D12_SIGNATURE_PARAMETER_DESC& parameter) {
  SignatureParameter result;
  result.Semantic = Uppercase(parameter.SemanticName != nullptr ? parameter.SemanticName : "");
  result.SemanticIndex = parameter.SemanticIndex;
  result.Register = parameter.Register;
  result.Stream = parameter.Stream;
  result.SystemValue = parameter.SystemValueType;
  result.ComponentType = parameter.ComponentType;
  result.MinPrecision = parameter.MinPrecision;
  result.Mask = parameter.Mask;
  return result;
}

ShaderSignature ReflectSignature(DxcLibrary& library, const std::vector<uint8_t>& bytecode) {
  Require(!bytecode.empty() &&
              bytecode.size() <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()),
          "The shader cannot be reflected because its bytecode size is invalid.");

  Microsoft::WRL::ComPtr<IDxcUtils> utils;
  Require(SUCCEEDED(library.CreateInstance()(CLSID_DxcUtils, IID_PPV_ARGS(&utils))),
          "Could not create the DXC utilities for shader reflection.");
  Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
  Require(SUCCEEDED(utils->CreateBlobFromPinned(
              bytecode.data(), static_cast<uint32_t>(bytecode.size()), DXC_CP_ACP, &blob)),
          "Could not prepare the shader for reflection.");

  Microsoft::WRL::ComPtr<IDxcContainerReflection> container;
  Require(
      SUCCEEDED(library.CreateInstance()(CLSID_DxcContainerReflection, IID_PPV_ARGS(&container))) &&
          SUCCEEDED(container->Load(blob.Get())),
      "Could not open the DXIL container for reflection.");

  constexpr uint32_t DXIL_PART = 0x4C495844; // "DXIL"
  uint32_t part{};
  Require(SUCCEEDED(container->FindFirstPartKind(DXIL_PART, &part)),
          "The shader container has no reflectable DXIL part.");

  Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
  Require(SUCCEEDED(container->GetPartReflection(part, IID_PPV_ARGS(&reflection))) && reflection,
          "Could not reflect the DXIL program.");

  D3D12_SHADER_DESC desc{};
  Require(SUCCEEDED(reflection->GetDesc(&desc)), "Could not read the DXIL shader description.");

  ShaderSignature signature;
  signature.Inputs.reserve(desc.InputParameters);
  signature.Outputs.reserve(desc.OutputParameters);
  signature.PatchConstants.reserve(desc.PatchConstantParameters);

  for (uint32_t i = 0; i < desc.InputParameters; ++i) {
    D3D12_SIGNATURE_PARAMETER_DESC parameter{};
    Require(SUCCEEDED(reflection->GetInputParameterDesc(i, &parameter)),
            "Could not read a DXIL input signature parameter.");
    signature.Inputs.push_back(CopySignatureParameter(parameter));
  }
  for (uint32_t i = 0; i < desc.OutputParameters; ++i) {
    D3D12_SIGNATURE_PARAMETER_DESC parameter{};
    Require(SUCCEEDED(reflection->GetOutputParameterDesc(i, &parameter)),
            "Could not read a DXIL output signature parameter.");
    signature.Outputs.push_back(CopySignatureParameter(parameter));
  }
  for (uint32_t i = 0; i < desc.PatchConstantParameters; ++i) {
    D3D12_SIGNATURE_PARAMETER_DESC parameter{};
    Require(SUCCEEDED(reflection->GetPatchConstantParameterDesc(i, &parameter)),
            "Could not read a DXIL patch-constant signature parameter.");
    signature.PatchConstants.push_back(CopySignatureParameter(parameter));
  }
  return signature;
}

struct DeclarationOrder {
  uint32_t Register{std::numeric_limits<uint32_t>::max()};
  uint32_t Component{std::numeric_limits<uint32_t>::max()};
  size_t Original{};
};

std::string SemanticFromDeclaration(const std::string& line) {
  const size_t colon = line.rfind(':');
  const size_t semicolon = line.find(';', colon == std::string::npos ? 0 : colon);
  if (colon == std::string::npos || semicolon == std::string::npos) {
    return {};
  }
  const size_t begin = line.find_first_not_of(" \t", colon + 1);
  if (begin == std::string::npos || begin >= semicolon) {
    return {};
  }
  const size_t end = line.find_last_not_of(" \t", semicolon - 1);
  return Uppercase(line.substr(begin, end - begin + 1));
}

DeclarationOrder OrderForSemantic(const std::string& semantic,
                                  const std::vector<SignatureParameter>& signature,
                                  size_t original) {
  for (const auto& parameter : signature) {
    const std::string indexed = parameter.Semantic + std::to_string(parameter.SemanticIndex);
    if (semantic != indexed && !(parameter.SemanticIndex == 0 && semantic == parameter.Semantic)) {
      continue;
    }
    uint32_t component = 0;
    while (component < 4 && (parameter.Mask & (1u << component)) == 0) {
      ++component;
    }
    return {parameter.Register, component, original};
  }
  return {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), original};
}

void ReorderInterfaceStruct(std::string& source,
                            const char* name,
                            const std::vector<SignatureParameter>& signature) {
  const size_t declaration = source.find(std::string("struct ") + name);
  if (declaration == std::string::npos) {
    return;
  }
  const size_t open = source.find('{', declaration);
  const size_t bodyBegin = source.find('\n', open);
  const size_t close = source.find("};", bodyBegin);
  if (open == std::string::npos || bodyBegin == std::string::npos || close == std::string::npos) {
    return;
  }

  struct Line {
    std::string Text;
    DeclarationOrder Order;
  };
  std::vector<std::string> lines;
  std::vector<Line> declarations;
  std::istringstream input(source.substr(bodyBegin + 1, close - bodyBegin - 1));
  for (std::string line; std::getline(input, line);) {
    const std::string semantic = SemanticFromDeclaration(line);
    lines.push_back(line);
    if (!semantic.empty()) {
      declarations.push_back({line, OrderForSemantic(semantic, signature, declarations.size())});
    }
  }

  std::stable_sort(
      declarations.begin(), declarations.end(), [](const Line& left, const Line& right) {
        return std::tie(left.Order.Register, left.Order.Component, left.Order.Original) <
               std::tie(right.Order.Register, right.Order.Component, right.Order.Original);
      });

  size_t nextDeclaration = 0;
  std::ostringstream reordered;
  for (const std::string& line : lines) {
    if (SemanticFromDeclaration(line).empty()) {
      reordered << line << '\n';
    } else {
      reordered << declarations[nextDeclaration++].Text << '\n';
    }
  }
  source.replace(bodyBegin + 1, close - bodyBegin - 1, reordered.str());
}

std::string ReorderInterfaceSource(std::string source, const ShaderSignature& signature) {
  ReorderInterfaceStruct(source, "SPIRV_Cross_Input", signature.Inputs);
  ReorderInterfaceStruct(source, "SPIRV_Cross_Output", signature.Outputs);
  return source;
}

bool SameSignatureSection(std::vector<SignatureParameter> expected,
                          std::vector<SignatureParameter> actual,
                          const char* section,
                          std::string& difference) {
  const auto order = [](const SignatureParameter& parameter) {
    return std::tie(parameter.Semantic, parameter.SemanticIndex, parameter.Stream);
  };
  std::sort(expected.begin(), expected.end(),
            [&](const auto& left, const auto& right) { return order(left) < order(right); });
  std::sort(actual.begin(), actual.end(),
            [&](const auto& left, const auto& right) { return order(left) < order(right); });

  if (expected.size() != actual.size()) {
    difference = std::string(section) + " signature parameter count changed from " +
                 std::to_string(expected.size()) + " to " + std::to_string(actual.size()) + ".";
    return false;
  }

  for (size_t i = 0; i < expected.size(); ++i) {
    const auto& before = expected[i];
    const auto& after = actual[i];
    const std::string expectedName = before.Semantic + std::to_string(before.SemanticIndex);
    const std::string actualName = after.Semantic + std::to_string(after.SemanticIndex);
    if (expectedName != actualName) {
      difference = std::string(section) + " signature expected " + expectedName + ", got " +
                   actualName + ".";
      return false;
    }
    if (before.Register != after.Register) {
      difference = std::string(section) + " signature " + expectedName + " moved from register " +
                   std::to_string(before.Register) + " to " + std::to_string(after.Register) + ".";
      return false;
    }
    if (before.Stream != after.Stream || before.SystemValue != after.SystemValue ||
        before.ComponentType != after.ComponentType || before.MinPrecision != after.MinPrecision ||
        before.Mask != after.Mask) {
      difference = std::string(section) + " signature " + expectedName +
                   " changed its stream, type, precision, system value or component mask.";
      return false;
    }
  }
  return true;
}

bool SameSignature(const ShaderSignature& expected,
                   const ShaderSignature& actual,
                   std::string& difference) {
  return SameSignatureSection(expected.Inputs, actual.Inputs, "Input", difference) &&
         SameSignatureSection(expected.Outputs, actual.Outputs, "Output", difference) &&
         SameSignatureSection(expected.PatchConstants, actual.PatchConstants, "Patch constant",
                              difference);
}

ShaderCompileResult CompileCandidate(DxcLibrary& library,
                                     const ShaderCompileInput& input,
                                     const std::wstring& entrypoint,
                                     const std::wstring& profile,
                                     bool optimizedPacking) {
  Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
  Require(SUCCEEDED(library.CreateInstance()(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))),
          "Could not create the DXC compiler.");

  const DxcBuffer source{input.Source.data(), input.Source.size(), DXC_CP_UTF8};
  std::vector<const wchar_t*> arguments = {L"-E",  entrypoint.c_str(), L"-T", profile.c_str(),
                                           L"-Zi", L"-Qembed_debug"};
  if (optimizedPacking) {
    arguments.push_back(L"-pack-optimized");
  }

  Microsoft::WRL::ComPtr<IDxcResult> result;
  Require(SUCCEEDED(compiler->Compile(&source, arguments.data(),
                                      static_cast<uint32_t>(arguments.size()), nullptr,
                                      IID_PPV_ARGS(&result))) &&
              result,
          "DXC compilation could not be started.");
  const std::string diagnostics = DxcDiagnostics(result.Get());
  CheckDxcResult(result.Get(), "HLSL compilation");

  Microsoft::WRL::ComPtr<IDxcBlob> object;
  Require(SUCCEEDED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), nullptr)) && object &&
              object->GetBufferPointer() != nullptr && object->GetBufferSize() != 0,
          "DXC returned no shader bytecode.");

  ShaderCompileResult compiled;
  compiled.Bytecode.resize(object->GetBufferSize());
  std::memcpy(compiled.Bytecode.data(), object->GetBufferPointer(), object->GetBufferSize());
  compiled.EntrypointName = input.EntrypointName;
  compiled.Diagnostic = diagnostics;
  return compiled;
}
#endif

void ValidateDxil([[maybe_unused]] const SessionStore::ShaderBlob& blob,
                  [[maybe_unused]] const std::filesystem::path& playerPath) {
#if defined(RTSR_HAS_DXC)
  DxcLibrary library(playerPath);
  Microsoft::WRL::ComPtr<IDxcUtils> utils;
  Require(SUCCEEDED(library.CreateInstance()(CLSID_DxcUtils, IID_PPV_ARGS(&utils))),
          "Could not create the DXC utilities.");
  Microsoft::WRL::ComPtr<IDxcValidator> validator;
  Require(SUCCEEDED(library.CreateInstance()(CLSID_DxcValidator, IID_PPV_ARGS(&validator))),
          "Could not create the DXIL validator.");
  Require(blob.Bytecode.size() <= std::numeric_limits<UINT32>::max(),
          "The shader exceeds DXC's input size limit.");
  Microsoft::WRL::ComPtr<IDxcBlobEncoding> input;
  Require(SUCCEEDED(utils->CreateBlobFromPinned(
              blob.Bytecode.data(), static_cast<UINT32>(blob.Bytecode.size()), DXC_CP_ACP, &input)),
          "Could not prepare the shader for DXIL validation.");

  // Validate the container and its inner module. No InPlaceEdit: the input is
  // borrowed from the request and must remain unchanged for the converter.
  Microsoft::WRL::ComPtr<IDxcOperationResult> result;
  Require(SUCCEEDED(validator->Validate(input.Get(), DxcValidatorFlags_Default, &result)),
          "DXIL validation call failed.");
  CheckDxcResult(result.Get(), "DXIL validation");
#else
  throw std::runtime_error("DXIL validation requires DXC, which is unavailable in this build.");
#endif
}

ShaderSource Disassemble([[maybe_unused]] const SessionStore::ShaderBlob& blob,
                         [[maybe_unused]] const std::filesystem::path& playerPath) {
#if defined(RTSR_HAS_DXC)
  DxcLibrary library(playerPath);
  Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
  Require(SUCCEEDED(library.CreateInstance()(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))),
          "Could not create the DXC disassembler.");
  const DxcBuffer buffer{blob.Bytecode.data(), blob.Bytecode.size(), 0};
  Microsoft::WRL::ComPtr<IDxcResult> result;
  Require(SUCCEEDED(compiler->Disassemble(&buffer, IID_PPV_ARGS(&result))) && result,
          "DXC disassembly call failed.");
  CheckDxcResult(result.Get(), "DXC disassembly");
  Microsoft::WRL::ComPtr<IDxcBlob> text;
  Require(SUCCEEDED(result->GetResult(&text)) && text, "DXC returned no disassembly.");
  Require(text->GetBufferPointer() != nullptr && text->GetBufferSize() != 0,
          "DXC returned empty disassembly.");
  std::string source(static_cast<const char*>(text->GetBufferPointer()), text->GetBufferSize());
  while (!source.empty() && source.back() == '\0') {
    source.pop_back();
  }
  Require(!source.empty(), "DXC returned empty disassembly.");
  return {SourceKind::Disassembly, std::move(source),
          "DXBC disassembly; HLSL reconstruction is not supported for DXBC."};
#else
  return {SourceKind::Disassembly, {}, "DXC disassembly is unavailable in this build."};
#endif
}

} // namespace

ShaderSource Decompile(const SessionStore::ShaderBlob& blob,
                       const std::filesystem::path& playerPath) {
  try {
    Require(!blob.Bytecode.empty(), "The shader bytecode is empty.");
    ShaderSource source;
    switch (blob.BytecodeType) {
    case RTSR_BYTECODE_TYPE_DXIL: {
      ValidateContainer(blob.Bytecode, true);
      ValidateDxil(blob, playerPath);
      DxilConversion conversion;
      source = conversion.Run(blob);
      break;
    }
    case RTSR_BYTECODE_TYPE_SPIRV:
      source = ToGlsl(SpirvWords(blob.Bytecode.data(), blob.Bytecode.size()), blob.EntrypointName);
      break;
    case RTSR_BYTECODE_TYPE_DXBC:
      ValidateContainer(blob.Bytecode, false);
      source = Disassemble(blob, playerPath);
      break;
    default:
      throw std::runtime_error("Unsupported shader bytecode type.");
    }
    if (source.Text.empty() && source.Diagnostic.empty()) {
      source.Diagnostic = "The toolchain returned no shader source.";
    }
    return source;
  } catch (const std::exception& error) {
    return {SourceKind::Disassembly, {}, error.what()};
  } catch (...) {
    return {SourceKind::Disassembly, {}, "Unknown shader conversion failure."};
  }
}

namespace {

struct VulkanEnvironment {
  glslang::EShTargetClientVersion Client;
  glslang::EShTargetLanguageVersion Spirv;
  spv_target_env Validator;
};

VulkanEnvironment EnvironmentFor(RTSR_API api) {
  switch (api) {
  case RTSR_API_VULKAN_1_0:
    return {glslang::EShTargetVulkan_1_0, glslang::EShTargetSpv_1_0, SPV_ENV_VULKAN_1_0};
  case RTSR_API_VULKAN_1_1:
    return {glslang::EShTargetVulkan_1_1, glslang::EShTargetSpv_1_3, SPV_ENV_VULKAN_1_1};
  case RTSR_API_VULKAN_1_2:
    return {glslang::EShTargetVulkan_1_2, glslang::EShTargetSpv_1_5, SPV_ENV_VULKAN_1_2};
  case RTSR_API_VULKAN_1_3:
    return {glslang::EShTargetVulkan_1_3, glslang::EShTargetSpv_1_6, SPV_ENV_VULKAN_1_3};
  default:
    throw std::runtime_error("A Vulkan API version is required for GLSL compilation.");
  }
}

EShLanguage GlslangStage(RTSR_SHADER_STAGE stage) {
  switch (stage) {
  case RTSR_SHADER_STAGE_VERTEX:
    return EShLangVertex;
  case RTSR_SHADER_STAGE_TESSELLATION_CONTROL:
    return EShLangTessControl;
  case RTSR_SHADER_STAGE_TESSELLATION_EVALUATION:
    return EShLangTessEvaluation;
  case RTSR_SHADER_STAGE_GEOMETRY:
    return EShLangGeometry;
  case RTSR_SHADER_STAGE_FRAGMENT:
    return EShLangFragment;
  case RTSR_SHADER_STAGE_COMPUTE:
    return EShLangCompute;
  default:
    throw std::runtime_error("This shader stage is outside Vulkan GLSL replacement scope.");
  }
}

class GlslangProcess {
public:
  GlslangProcess() {
    Require(glslang::InitializeProcess(), "Could not initialize glslang.");
  }
  ~GlslangProcess() {
    glslang::FinalizeProcess();
  }
};

GlslangProcess& Glslang() {
  static GlslangProcess process;
  return process;
}

class RuntimeDescriptorArrayPreserver : public glslang::TIntermTraverser {
public:
  RuntimeDescriptorArrayPreserver(const std::vector<uint32_t>& words, std::string_view entrypoint) {
    spirv_cross::Compiler compiler(words);
    const auto entries = compiler.get_entry_points_and_stages();
    const auto selected = std::find_if(entries.begin(), entries.end(),
                                       [&](const auto& entry) { return entry.name == entrypoint; });
    Require(selected != entries.end(), "Could not find the selected SPIR-V entrypoint.");
    compiler.set_entry_point(selected->name, selected->execution_model);
    for (const auto id : compiler.get_active_interface_variables()) {
      if (!compiler.has_decoration(id, spv::DecorationBinding)) {
        continue;
      }
      const auto& type = compiler.get_type_from_variable(id);
      if (!type.array.empty() && type.array_size_literal.back() && type.array.back() == 0) {
        const uint32_t set = compiler.has_decoration(id, spv::DecorationDescriptorSet)
                                 ? compiler.get_decoration(id, spv::DecorationDescriptorSet)
                                 : 0;
        m_Bindings.emplace(set, compiler.get_decoration(id, spv::DecorationBinding));
      }
    }
  }

  void visitSymbol(glslang::TIntermSymbol* symbol) override {
    auto& type = symbol->getWritableType();
    const auto& qualifier = type.getQualifier();
    if (!type.isUnsizedArray() || !qualifier.hasBinding()) {
      return;
    }
    const uint32_t set = qualifier.hasSet() ? qualifier.layoutSet : 0;
    if (m_Bindings.contains({set, static_cast<uint32_t>(qualifier.layoutBinding)})) {
      // glslang otherwise sizes [] from constant indices during linking. This
      // flag preserves OpTypeRuntimeArray without changing the shader's accesses.
      type.setArrayVariablyIndexed();
    }
  }

private:
  std::set<std::pair<uint32_t, uint32_t>> m_Bindings;
};

class SpirvInterfaceCompiler : public spirv_cross::Compiler {
public:
  using spirv_cross::Compiler::Compiler;

  std::string WorkgroupKey() const {
    const auto& size = get_entry_point().workgroup_size;
    const auto dimension = [&](uint32_t id, uint32_t literal) {
      const auto* constant = id != 0 ? maybe_get<spirv_cross::SPIRConstant>(id) : nullptr;
      if (constant && !constant->specialization) {
        return std::to_string(constant->scalar_u64());
      }
      return id != 0 ? ConstantKey(id) : std::to_string(literal);
    };
    std::string key = "workgroup:";
    if (size.constant != 0) {
      const auto& constant = get_constant(size.constant);
      for (uint32_t axis = 0; axis < 3; ++axis) {
        key += '[' +
               dimension(constant.specialization_constant_id(0, axis), constant.scalar(0, axis)) +
               ']';
      }
    } else {
      key += '[' + dimension(size.id_x, size.x) + ']';
      key += '[' + dimension(size.id_y, size.y) + ']';
      key += '[' + dimension(size.id_z, size.z) + ']';
    }
    return key;
  }

  std::string ConstantKey(uint32_t id, uint32_t depth = 0) const {
    Require(depth < 32, "The SPIR-V constant expression is nested too deeply.");
    std::ostringstream key;
    const auto appendType = [&](spirv_cross::TypeID typeId) {
      const auto& type = get_type(typeId);
      key << static_cast<uint32_t>(type.basetype) << ':' << type.width << ':' << type.vecsize << ':'
          << type.columns;
    };
    if (const auto* operation = maybe_get<spirv_cross::SPIRConstantOp>(id)) {
      key << "op=" << static_cast<uint32_t>(operation->opcode) << ':';
      appendType(operation->basetype);
      for (size_t i = 0; i < operation->arguments.size(); ++i) {
        const bool literal = (operation->opcode == spv::OpCompositeExtract && i >= 1) ||
                             (operation->opcode == spv::OpCompositeInsert && i >= 2) ||
                             (operation->opcode == spv::OpVectorShuffle && i >= 2);
        key << '['
            << (literal ? std::to_string(operation->arguments[i])
                        : ConstantKey(operation->arguments[i], depth + 1))
            << ']';
      }
      return key.str();
    }
    const auto* constant = maybe_get<spirv_cross::SPIRConstant>(id);
    Require(constant != nullptr, "Unsupported SPIR-V array-size constant expression.");
    appendType(constant->constant_type);
    if (has_decoration(id, spv::DecorationSpecId)) {
      key << ":spec=" << get_decoration(id, spv::DecorationSpecId);
    }
    if (!constant->subconstants.empty()) {
      for (const auto component : constant->subconstants) {
        key << '[' << ConstantKey(component, depth + 1) << ']';
      }
    } else {
      const auto& type = get_type(constant->constant_type);
      for (uint32_t column = 0; column < type.columns; ++column) {
        const uint32_t columnId = constant->specialization_constant_id(column);
        if (columnId != 0) {
          key << '[' << ConstantKey(columnId, depth + 1) << ']';
          continue;
        }
        for (uint32_t row = 0; row < type.vecsize; ++row) {
          const uint32_t component = constant->specialization_constant_id(column, row);
          key << '['
              << (component != 0 ? ConstantKey(component, depth + 1)
                                 : std::to_string(constant->scalar_u64(column, row)))
              << ']';
        }
      }
    }
    return key.str();
  }
};

std::string SpirvTypeKey(SpirvInterfaceCompiler& compiler,
                         spirv_cross::TypeID id,
                         uint32_t depth = 0) {
  Require(depth < 32, "The SPIR-V interface type is recursively nested too deeply.");
  const auto& type = compiler.get_type(id);
  std::ostringstream key;
  key << static_cast<uint32_t>(type.basetype) << ':' << type.width << ':' << type.vecsize << ':'
      << type.columns << ':' << static_cast<uint32_t>(type.image.dim) << ':' << type.image.arrayed
      << ':' << type.image.ms << ':' << type.image.sampled << ':'
      << static_cast<uint32_t>(type.image.format) << ':' << type.image.depth << ':'
      << static_cast<uint32_t>(type.image.access);
  if (type.image.type != 0) {
    key << ":sampled=" << SpirvTypeKey(compiler, type.image.type, depth + 1);
  }
  if (!type.array.empty() && type.parent_type != 0) {
    key << ":element=" << SpirvTypeKey(compiler, type.parent_type, depth + 1);
  }
  for (const auto decoration :
       {spv::DecorationArrayStride, spv::DecorationBlock, spv::DecorationBufferBlock}) {
    if (compiler.has_decoration(id, decoration)) {
      key << ":d" << static_cast<uint32_t>(decoration) << '='
          << compiler.get_decoration(id, decoration);
    }
  }
  for (size_t dimension = 0; dimension < type.array.size(); ++dimension) {
    key << ":array["
        << (type.array_size_literal[dimension] ? std::to_string(type.array[dimension])
                                               : compiler.ConstantKey(type.array[dimension]))
        << ']';
  }
  for (uint32_t member = 0; member < type.member_types.size(); ++member) {
    key << "{type=" << SpirvTypeKey(compiler, type.member_types[member], depth + 1);
    for (const auto decoration :
         {spv::DecorationOffset, spv::DecorationMatrixStride, spv::DecorationRowMajor,
          spv::DecorationColMajor, spv::DecorationLocation, spv::DecorationComponent,
          spv::DecorationIndex, spv::DecorationBuiltIn, spv::DecorationFlat,
          spv::DecorationNoPerspective, spv::DecorationCentroid, spv::DecorationSample,
          spv::DecorationPatch, spv::DecorationInvariant}) {
      if (compiler.has_member_decoration(type.self, member, decoration)) {
        key << ":d" << static_cast<uint32_t>(decoration) << '='
            << compiler.get_member_decoration(type.self, member, decoration);
      }
    }
    key << '}';
  }
  return key.str();
}

struct SpirvInterfaceElement {
  std::string Definition;
  bool MayRemove{};
};

using SpirvSignature = std::map<std::string, SpirvInterfaceElement>;

void AddSpirvElement(SpirvSignature& signature,
                     std::string identity,
                     std::string definition,
                     bool mayRemove = false) {
  Require(signature
              .emplace(std::move(identity), SpirvInterfaceElement{std::move(definition), mayRemove})
              .second,
          "Ambiguous SPIR-V interface identity; compatibility cannot be established.");
}

template <typename Resources>
void AppendSpirvResources(SpirvInterfaceCompiler& compiler,
                          const Resources& resources,
                          const char* category,
                          SpirvSignature& signature) {
  for (const auto& resource : resources) {
    const uint32_t set = compiler.has_decoration(resource.id, spv::DecorationDescriptorSet)
                             ? compiler.get_decoration(resource.id, spv::DecorationDescriptorSet)
                             : 0;
    const uint32_t binding = compiler.has_decoration(resource.id, spv::DecorationBinding)
                                 ? compiler.get_decoration(resource.id, spv::DecorationBinding)
                                 : 0;
    std::string definition = std::string(category) + ':' +
                             SpirvTypeKey(compiler, resource.type_id) + ':' +
                             SpirvTypeKey(compiler, resource.base_type_id);
    if (compiler.has_decoration(resource.id, spv::DecorationInputAttachmentIndex)) {
      definition +=
          ":attachment=" +
          std::to_string(compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex));
    }
    AddSpirvElement(signature,
                    "descriptor set " + std::to_string(set) + " binding " + std::to_string(binding),
                    std::move(definition), true);
  }
}

SpirvSignature SpirvInterface(const std::vector<uint32_t>& words, std::string_view entrypoint) {
  SpirvInterfaceCompiler compiler(words);
  const auto entries = compiler.get_entry_points_and_stages();
  auto selected = std::find_if(entries.begin(), entries.end(),
                               [&](const auto& entry) { return entry.name == entrypoint; });
  Require(selected != entries.end(), "Could not find the selected SPIR-V entrypoint.");
  compiler.set_entry_point(selected->name, selected->execution_model);
  const auto declarations = compiler.get_shader_resources();
  const auto resources = compiler.get_shader_resources(compiler.get_active_interface_variables());
  compiler.update_active_builtins();

  SpirvSignature signature;
  AddSpirvElement(signature, "execution model", std::to_string(selected->execution_model));
  const auto& modes = compiler.get_execution_mode_bitset();
  for (const auto mode : {spv::ExecutionModeInvocations,
                          spv::ExecutionModeSpacingEqual,
                          spv::ExecutionModeSpacingFractionalEven,
                          spv::ExecutionModeSpacingFractionalOdd,
                          spv::ExecutionModeVertexOrderCw,
                          spv::ExecutionModeVertexOrderCcw,
                          spv::ExecutionModePixelCenterInteger,
                          spv::ExecutionModeOriginUpperLeft,
                          spv::ExecutionModeEarlyFragmentTests,
                          spv::ExecutionModePointMode,
                          spv::ExecutionModeDepthReplacing,
                          spv::ExecutionModeDepthGreater,
                          spv::ExecutionModeDepthLess,
                          spv::ExecutionModeDepthUnchanged,
                          spv::ExecutionModeInputPoints,
                          spv::ExecutionModeInputLines,
                          spv::ExecutionModeInputLinesAdjacency,
                          spv::ExecutionModeTriangles,
                          spv::ExecutionModeInputTrianglesAdjacency,
                          spv::ExecutionModeQuads,
                          spv::ExecutionModeIsolines,
                          spv::ExecutionModeOutputVertices,
                          spv::ExecutionModeOutputPoints,
                          spv::ExecutionModeOutputLineStrip,
                          spv::ExecutionModeOutputTriangleStrip}) {
    if (modes.get(mode)) {
      std::string key = "mode:" + std::to_string(mode);
      if (mode == spv::ExecutionModeInvocations || mode == spv::ExecutionModeOutputVertices) {
        key += ':' + std::to_string(compiler.get_execution_mode_argument(mode));
      }
      AddSpirvElement(signature, "execution mode " + std::to_string(mode), std::move(key));
    }
  }
  if (selected->execution_model == spv::ExecutionModelGLCompute) {
    AddSpirvElement(signature, "compute workgroup size", compiler.WorkgroupKey());
  }
  const auto appendStage = [&](const auto& stageResources, const char* category) {
    for (const auto& resource : stageResources) {
      const uint32_t location = compiler.has_decoration(resource.id, spv::DecorationLocation)
                                    ? compiler.get_decoration(resource.id, spv::DecorationLocation)
                                    : 0;
      const uint32_t component =
          compiler.has_decoration(resource.id, spv::DecorationComponent)
              ? compiler.get_decoration(resource.id, spv::DecorationComponent)
              : 0;
      std::string key = std::string(category) + ':' + std::to_string(location) + ':' +
                        std::to_string(component) + ':' + SpirvTypeKey(compiler, resource.type_id);
      for (const auto decoration :
           {spv::DecorationIndex, spv::DecorationFlat, spv::DecorationNoPerspective,
            spv::DecorationCentroid, spv::DecorationSample, spv::DecorationPatch,
            spv::DecorationInvariant}) {
        if (compiler.has_decoration(resource.id, decoration)) {
          key += ":d" + std::to_string(static_cast<uint32_t>(decoration)) + '=' +
                 std::to_string(compiler.get_decoration(resource.id, decoration));
        }
      }
      const uint32_t index = compiler.has_decoration(resource.id, spv::DecorationIndex)
                                 ? compiler.get_decoration(resource.id, spv::DecorationIndex)
                                 : 0;
      AddSpirvElement(signature,
                      std::string(category) + " location " + std::to_string(location) +
                          " component " + std::to_string(component) + " index " +
                          std::to_string(index),
                      std::move(key), std::string_view(category) == "input");
    }
  };
  appendStage(resources.stage_inputs, "input");
  appendStage(declarations.stage_outputs, "output");
  for (const auto& builtin : resources.builtin_inputs) {
    if (compiler.has_active_builtin(builtin.builtin, spv::StorageClassInput)) {
      AddSpirvElement(signature, "input built-in " + std::to_string(builtin.builtin),
                      SpirvTypeKey(compiler, builtin.value_type_id), true);
    }
  }
  for (const auto& builtin : declarations.builtin_outputs) {
    if (compiler.has_active_builtin(builtin.builtin, spv::StorageClassOutput)) {
      AddSpirvElement(signature, "output built-in " + std::to_string(builtin.builtin),
                      SpirvTypeKey(compiler, builtin.value_type_id));
    }
  }

  AppendSpirvResources(compiler, resources.uniform_buffers, "ubo", signature);
  AppendSpirvResources(compiler, resources.storage_buffers, "ssbo", signature);
  AppendSpirvResources(compiler, resources.sampled_images, "sampled-image", signature);
  AppendSpirvResources(compiler, resources.separate_images, "image", signature);
  AppendSpirvResources(compiler, resources.separate_samplers, "sampler", signature);
  AppendSpirvResources(compiler, resources.storage_images, "storage-image", signature);
  AppendSpirvResources(compiler, resources.subpass_inputs, "subpass", signature);
  AppendSpirvResources(compiler, resources.acceleration_structures, "acceleration", signature);

  for (const auto& push : resources.push_constant_buffers) {
    const auto& type = compiler.get_type(push.base_type_id);
    AddSpirvElement(signature, "push constants",
                    std::to_string(compiler.get_declared_struct_size(type)) + ':' +
                        SpirvTypeKey(compiler, push.base_type_id),
                    true);
  }
  for (const auto& constant : compiler.get_specialization_constants()) {
    AddSpirvElement(signature, "specialization constant " + std::to_string(constant.constant_id),
                    compiler.ConstantKey(constant.id), true);
  }
  return signature;
}

std::string SpirvInterfaceDifference(const SpirvSignature& original,
                                     const SpirvSignature& replacement) {
  std::ostringstream diagnostic;
  for (const auto& [identity, element] : replacement) {
    const auto previous = original.find(identity);
    if (previous == original.end()) {
      diagnostic << "\nAdded " << identity << ": new interface requirements are unsupported.";
    } else if (previous->second.Definition != element.Definition) {
      diagnostic << "\nChanged " << identity << ". Original: " << previous->second.Definition
                 << ". Replacement: " << element.Definition << '.';
    }
  }
  for (const auto& [identity, element] : original) {
    if (!element.MayRemove && replacement.find(identity) == replacement.end()) {
      diagnostic << "\nRemoved " << identity << ": required output or execution contract.";
    }
  }
  const std::string details = diagnostic.str();
  return details.empty() ? std::string{}
                         : "Recompiled GLSL changed the Vulkan shader interface." + details;
}

} // namespace

ShaderCompileResult CompileHlslToDxil(const ShaderCompileInput& input,
                                      const std::vector<uint8_t>& originalBytecode,
                                      const std::filesystem::path& playerPath) {
#if defined(RTSR_HAS_DXC)
  try {
    Require(!input.Source.empty(), "The HLSL source is empty.");
    ValidateContainer(originalBytecode, true);
    const std::wstring entrypoint = Utf8ToWide(input.EntrypointName);
    const std::wstring profile = TargetProfile(input.Stage, input.D3DShaderVersion);

    DxcLibrary library(playerPath);
    const ShaderSignature originalSignature = ReflectSignature(library, originalBytecode);
    ShaderCompileInput orderedInput = input;
    orderedInput.Source = ReorderInterfaceSource(input.Source, originalSignature);

    ShaderCompileResult compiled =
        CompileCandidate(library, orderedInput, entrypoint, profile, false);
    ValidateContainer(compiled.Bytecode, true);
    std::string defaultDifference;
    if (!SameSignature(originalSignature, ReflectSignature(library, compiled.Bytecode),
                       defaultDifference)) {
      ShaderCompileResult optimized =
          CompileCandidate(library, orderedInput, entrypoint, profile, true);
      ValidateContainer(optimized.Bytecode, true);
      std::string optimizedDifference;
      if (!SameSignature(originalSignature, ReflectSignature(library, optimized.Bytecode),
                         optimizedDifference)) {
        throw std::runtime_error("Recompiled HLSL changed the shader interface. Default packing: " +
                                 defaultDifference + " Optimized packing: " + optimizedDifference);
      }
      compiled = std::move(optimized);
      if (!compiled.Diagnostic.empty()) {
        compiled.Diagnostic += '\n';
      }
      compiled.Diagnostic +=
          "DXC optimized signature packing was required to preserve the captured interface.";
    }

    const SessionStore::ShaderBlob output{RTSR_BYTECODE_TYPE_DXIL, compiled.EntrypointName,
                                          input.D3DShaderVersion, compiled.Bytecode};
    ValidateDxil(output, playerPath);
    return compiled;
  } catch (const std::exception& error) {
    return {{}, input.EntrypointName, error.what()};
  } catch (...) {
    return {{}, input.EntrypointName, "Unknown HLSL compilation failure."};
  }
#else
  (void)input;
  (void)originalBytecode;
  (void)playerPath;
  return {{}, {}, "HLSL compilation requires DXC, which is unavailable in this build."};
#endif
}

ShaderCompileResult CompileGlslToSpirv(const ShaderCompileInput& input,
                                       const std::vector<uint8_t>& originalBytecode) {
  try {
    Require(!input.Source.empty(), "The GLSL source is empty.");
    Require(input.Source.size() <= static_cast<size_t>(std::numeric_limits<int>::max()),
            "The GLSL source is too large.");
    const VulkanEnvironment environment = EnvironmentFor(input.TargetApi);
    const EShLanguage stage = GlslangStage(input.Stage);
    Glslang();

    const char* source = input.Source.data();
    const int sourceLength = static_cast<int>(input.Source.size());
    glslang::TShader shader(stage);
    shader.setStringsWithLengthsAndNames(&source, &sourceLength, nullptr, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
    shader.setEnvClient(glslang::EShClientVulkan, environment.Client);
    shader.setEnvTarget(glslang::EShTargetSpv, environment.Spirv);
    const char* entrypoint =
        input.EntrypointName.empty() ? RTSR_DEFAULT_ENTRYPOINT_NAME : input.EntrypointName.c_str();
    shader.setSourceEntryPoint(entrypoint);
    shader.setEntryPoint(entrypoint);

    // No debug information is requested. glslang's parser emits scope nodes for
    // compound statements when it is, and translating them to SPIR-V asserts
    // unless non-semantic debug info is also on -- which requires
    // VK_KHR_shader_non_semantic_info before Vulkan 1.3, an extension the
    // captured device need not have enabled.
    constexpr EShMessages MESSAGES = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, MESSAGES)) {
      throw std::runtime_error(std::string("GLSL compilation failed.\n") + shader.getInfoLog() +
                               shader.getInfoDebugLog());
    }

    const auto originalWords = SpirvWords(originalBytecode.data(), originalBytecode.size());
    const std::string_view originalEntrypoint =
        input.OriginalEntrypointName.empty() ? input.EntrypointName : input.OriginalEntrypointName;
    RuntimeDescriptorArrayPreserver runtimeArrays(originalWords, originalEntrypoint);
    auto* parsed = shader.getIntermediate();
    Require(parsed != nullptr && parsed->getTreeRoot() != nullptr,
            "glslang returned no shader syntax tree.");
    parsed->getTreeRoot()->traverse(&runtimeArrays);

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(MESSAGES)) {
      throw std::runtime_error(std::string("GLSL link failed.\n") + program.getInfoLog() +
                               program.getInfoDebugLog());
    }
    auto* intermediate = program.getIntermediate(stage);
    Require(intermediate != nullptr, "glslang returned no shader intermediate.");

    std::vector<uint32_t> words;
    glslang::SpvOptions options;
    glslang::GlslangToSpv(*intermediate, words, &options);
    Require(!words.empty(), "glslang returned no SPIR-V.");

    std::string validationDiagnostic;
    spvtools::SpirvTools validator(environment.Validator);
    validator.SetMessageConsumer([&validationDiagnostic](spv_message_level_t, const char*,
                                                         const spv_position_t& position,
                                                         const char* message) {
      validationDiagnostic += "SPIR-V " + std::to_string(position.line) + ":" +
                              std::to_string(position.column) + ": " +
                              (message != nullptr ? message : "validation error") + "\n";
    });
    Require(validator.Validate(words), validationDiagnostic.empty() ? "SPIR-V validation failed."
                                                                    : validationDiagnostic.c_str());

    const auto originalInterface = SpirvInterface(originalWords, originalEntrypoint);
    const auto replacementInterface = SpirvInterface(words, entrypoint);
    const auto difference = SpirvInterfaceDifference(originalInterface, replacementInterface);
    if (!difference.empty()) {
      throw std::runtime_error(difference);
    }

    ShaderCompileResult result;
    result.Bytecode.resize(words.size() * sizeof(uint32_t));
    std::memcpy(result.Bytecode.data(), words.data(), result.Bytecode.size());
    result.EntrypointName = entrypoint;
    result.Diagnostic = shader.getInfoLog();
    return result;
  } catch (const std::exception& error) {
    return {{}, input.EntrypointName, error.what()};
  } catch (...) {
    return {{}, input.EntrypointName, "Unknown GLSL compilation failure."};
  }
}

} // namespace gits::rtsr
