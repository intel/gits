// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "stateRestorer.h"

#include "commandSerializersAuto.h"
#include "liveGl.h"

#include <algorithm>
#include <array>

namespace gits::opengl {

namespace {

struct VersionedEnum {
  GLenum Value;
  int Major;
  int Minor;
};

constexpr VersionedEnum COMPATIBILITY_CAPABILITIES[] = {
    {GL_ALPHA_TEST, 1, 0},
    {GL_AUTO_NORMAL, 1, 0},
    {GL_COLOR_MATERIAL, 1, 0},
    {GL_FOG, 1, 0},
    {GL_LIGHTING, 1, 0},
    {GL_LINE_STIPPLE, 1, 0},
    {GL_POLYGON_STIPPLE, 1, 0},
    {GL_NORMALIZE, 1, 0},
    {GL_POINT_SMOOTH, 1, 0},
    {GL_MAP1_COLOR_4, 1, 0},
    {GL_MAP1_INDEX, 1, 0},
    {GL_MAP1_NORMAL, 1, 0},
    {GL_MAP1_TEXTURE_COORD_1, 1, 0},
    {GL_MAP1_TEXTURE_COORD_2, 1, 0},
    {GL_MAP1_TEXTURE_COORD_3, 1, 0},
    {GL_MAP1_TEXTURE_COORD_4, 1, 0},
    {GL_MAP1_VERTEX_3, 1, 0},
    {GL_MAP1_VERTEX_4, 1, 0},
    {GL_MAP2_COLOR_4, 1, 0},
    {GL_MAP2_INDEX, 1, 0},
    {GL_MAP2_NORMAL, 1, 0},
    {GL_MAP2_TEXTURE_COORD_1, 1, 0},
    {GL_MAP2_TEXTURE_COORD_2, 1, 0},
    {GL_MAP2_TEXTURE_COORD_3, 1, 0},
    {GL_MAP2_TEXTURE_COORD_4, 1, 0},
    {GL_MAP2_VERTEX_3, 1, 0},
    {GL_MAP2_VERTEX_4, 1, 0},
    {GL_INDEX_LOGIC_OP, 1, 1},
    {GL_RESCALE_NORMAL, 1, 2},
    {GL_COLOR_SUM, 1, 4},
    {GL_POINT_SPRITE, 2, 0},
    {GL_VERTEX_PROGRAM_TWO_SIDE, 2, 0},
};

constexpr VersionedEnum COMPATIBILITY_HINTS[] = {
    {GL_PERSPECTIVE_CORRECTION_HINT, 1, 0},
    {GL_POINT_SMOOTH_HINT, 1, 0},
    {GL_FOG_HINT, 1, 0},
    {GL_GENERATE_MIPMAP_HINT, 1, 4},
};

constexpr VersionedEnum UNIT_TEXTURE_TARGETS[] = {
    {GL_TEXTURE_1D, 1, 0},       {GL_TEXTURE_2D, 1, 0},        {GL_TEXTURE_3D, 1, 2},
    {GL_TEXTURE_CUBE_MAP, 1, 3}, {GL_TEXTURE_RECTANGLE, 3, 1},
};

struct VectorParameter {
  GLenum Pname;
  size_t Count;
};

constexpr VectorParameter LIGHT_PARAMETERS[] = {
    {GL_AMBIENT, 4},
    {GL_DIFFUSE, 4},
    {GL_SPECULAR, 4},
    {GL_POSITION, 4},
    {GL_SPOT_DIRECTION, 3},
    {GL_SPOT_EXPONENT, 1},
    {GL_SPOT_CUTOFF, 1},
    {GL_CONSTANT_ATTENUATION, 1},
    {GL_LINEAR_ATTENUATION, 1},
    {GL_QUADRATIC_ATTENUATION, 1},
};

constexpr VectorParameter MATERIAL_PARAMETERS[] = {
    {GL_AMBIENT, 4}, {GL_DIFFUSE, 4}, {GL_SPECULAR, 4}, {GL_EMISSION, 4}, {GL_SHININESS, 1},
};

constexpr GLenum TEXTURE_COMBINE_PARAMETERS[] = {
    GL_COMBINE_RGB,  GL_COMBINE_ALPHA,  GL_SRC0_RGB,       GL_SRC1_RGB,       GL_SRC2_RGB,
    GL_SRC0_ALPHA,   GL_SRC1_ALPHA,     GL_SRC2_ALPHA,     GL_OPERAND0_RGB,   GL_OPERAND1_RGB,
    GL_OPERAND2_RGB, GL_OPERAND0_ALPHA, GL_OPERAND1_ALPHA, GL_OPERAND2_ALPHA,
};

struct TextureCoordinate {
  GLenum Coordinate;
  GLenum Generation;
};

constexpr TextureCoordinate TEXTURE_COORDINATES[] = {
    {GL_S, GL_TEXTURE_GEN_S},
    {GL_T, GL_TEXTURE_GEN_T},
    {GL_R, GL_TEXTURE_GEN_R},
    {GL_Q, GL_TEXTURE_GEN_Q},
};

std::vector<GLfloat> GetFloats(GLenum pname, size_t count) {
  std::vector<GLfloat> values(16);
  GL_FN(glGetFloatv)(pname, values.data());
  values.resize(count);
  return values;
}

GLint GetArbProgram(GLenum target, GLenum pname) {
  GLint value = 0;
  GL_FN(glGetProgramivARB)(target, pname, &value);
  return value;
}

bool IsZero(const std::vector<GLfloat>& values) {
  return std::all_of(values.begin(), values.end(), [](GLfloat value) { return value == 0.0f; });
}

} // namespace

void StateRestorer::RestoreCompatibilityState() {
  for (const VersionedEnum& capability : COMPATIBILITY_CAPABILITIES) {
    if (m_Version.AtLeast(capability.Major, capability.Minor)) {
      EmitCapability(capability.Value, GL_FN(glIsEnabled)(capability.Value) != GL_FALSE);
    }
  }
  const GLint lights = GetInteger(GL_MAX_LIGHTS);
  for (GLint i = 0; i < lights; ++i) {
    const GLenum light = GL_LIGHT0 + static_cast<GLenum>(i);
    EmitCapability(light, GL_FN(glIsEnabled)(light) != GL_FALSE);
  }
  if (HasExtension("GL_EXT_stencil_two_side")) {
    EmitCapability(GL_STENCIL_TEST_TWO_SIDE_EXT,
                   GL_FN(glIsEnabled)(GL_STENCIL_TEST_TWO_SIDE_EXT) != GL_FALSE);
  }
  for (GLenum target : ArbProgramTargets()) {
    EmitCapability(target, GL_FN(glIsEnabled)(target) != GL_FALSE);
  }
  for (const VersionedEnum& hint : COMPATIBILITY_HINTS) {
    if (m_Version.AtLeast(hint.Major, hint.Minor)) {
      glHintCommand command;
      command.m_target.Value = hint.Value;
      command.m_mode.Value = GetInteger(hint.Value);
      Emit<glHintCommandSerializer>(command);
    }
  }

  {
    glShadeModelCommand shadeModel;
    shadeModel.m_mode.Value = GetInteger(GL_SHADE_MODEL);
    Emit<glShadeModelCommandSerializer>(shadeModel);
    glAlphaFuncCommand alphaFunc;
    alphaFunc.m_func.Value = GetInteger(GL_ALPHA_TEST_FUNC);
    alphaFunc.m_ref.Value = GetFloat(GL_ALPHA_TEST_REF);
    Emit<glAlphaFuncCommandSerializer>(alphaFunc);
    glLineStippleCommand lineStipple;
    lineStipple.m_factor.Value = GetInteger(GL_LINE_STIPPLE_REPEAT);
    lineStipple.m_pattern.Value = static_cast<GLushort>(GetInteger(GL_LINE_STIPPLE_PATTERN));
    Emit<glLineStippleCommandSerializer>(lineStipple);
    glPolygonStippleCommand polygonStipple;
    polygonStipple.m_mask.Data.resize(32 * 32 / 8);
    polygonStipple.m_mask.HasValue = true;
    BeginReadback();
    GL_FN(glGetPolygonStipple)(polygonStipple.m_mask.Data.data());
    EndReadback();
    Emit<glPolygonStippleCommandSerializer>(polygonStipple);
    const std::vector<GLfloat> accum = GetFloats(GL_ACCUM_CLEAR_VALUE, 4);
    glClearAccumCommand clearAccum;
    clearAccum.m_red.Value = accum[0];
    clearAccum.m_green.Value = accum[1];
    clearAccum.m_blue.Value = accum[2];
    clearAccum.m_alpha.Value = accum[3];
    Emit<glClearAccumCommandSerializer>(clearAccum);
  }
  if (m_Version.AtLeast(1, 4)) {
    for (GLenum pname : {GL_POINT_SIZE_MIN, GL_POINT_SIZE_MAX}) {
      glPointParameterfCommand command;
      command.m_pname.Value = pname;
      command.m_param.Value = GetFloat(pname);
      Emit<glPointParameterfCommandSerializer>(command);
    }
    glPointParameterfvCommand attenuation;
    attenuation.m_pname.Value = GL_POINT_DISTANCE_ATTENUATION;
    attenuation.m_params.Data = GetFloats(GL_POINT_DISTANCE_ATTENUATION, 3);
    attenuation.m_params.HasValue = true;
    Emit<glPointParameterfvCommandSerializer>(attenuation);
  }

  {
    glLightModelfvCommand ambient;
    ambient.m_pname.Value = GL_LIGHT_MODEL_AMBIENT;
    ambient.m_params.Data = GetFloats(GL_LIGHT_MODEL_AMBIENT, 4);
    ambient.m_params.HasValue = true;
    Emit<glLightModelfvCommandSerializer>(ambient);
    std::vector<GLenum> lightModel{GL_LIGHT_MODEL_LOCAL_VIEWER, GL_LIGHT_MODEL_TWO_SIDE};
    if (m_Version.AtLeast(1, 2)) {
      lightModel.push_back(GL_LIGHT_MODEL_COLOR_CONTROL);
    }
    for (GLenum pname : lightModel) {
      glLightModeliCommand command;
      command.m_pname.Value = pname;
      command.m_param.Value = GetInteger(pname);
      Emit<glLightModeliCommandSerializer>(command);
    }
  }
  // With GL_COLOR_MATERIAL enabled, glColorMaterial and glColor overwrite the material colors
  // they track, so the material is written between them and the current color last.
  {
    glColorMaterialCommand colorMaterial;
    colorMaterial.m_face.Value = GetInteger(GL_COLOR_MATERIAL_FACE);
    colorMaterial.m_mode.Value = GetInteger(GL_COLOR_MATERIAL_PARAMETER);
    Emit<glColorMaterialCommandSerializer>(colorMaterial);
    for (GLenum face : {GL_FRONT, GL_BACK}) {
      for (const VectorParameter& parameter : MATERIAL_PARAMETERS) {
        glMaterialfvCommand command;
        command.m_face.Value = face;
        command.m_pname.Value = parameter.Pname;
        command.m_params.Data.resize(4);
        command.m_params.HasValue = true;
        GL_FN(glGetMaterialfv)(face, parameter.Pname, command.m_params.Data.data());
        command.m_params.Data.resize(parameter.Count);
        Emit<glMaterialfvCommandSerializer>(command);
      }
    }
  }
  {
    glFogiCommand mode;
    mode.m_pname.Value = GL_FOG_MODE;
    mode.m_param.Value = GetInteger(GL_FOG_MODE);
    Emit<glFogiCommandSerializer>(mode);
    for (GLenum pname : {GL_FOG_DENSITY, GL_FOG_START, GL_FOG_END}) {
      glFogfCommand command;
      command.m_pname.Value = pname;
      command.m_param.Value = GetFloat(pname);
      Emit<glFogfCommandSerializer>(command);
    }
    glFogfvCommand color;
    color.m_pname.Value = GL_FOG_COLOR;
    color.m_params.Data = GetFloats(GL_FOG_COLOR, 4);
    color.m_params.HasValue = true;
    Emit<glFogfvCommandSerializer>(color);
    if (m_Version.AtLeast(1, 4)) {
      glFogiCommand source;
      source.m_pname.Value = GL_FOG_COORD_SRC;
      source.m_param.Value = GetInteger(GL_FOG_COORD_SRC);
      Emit<glFogiCommandSerializer>(source);
    }
  }
  {
    glColor4fvCommand color;
    color.m_v.Data = GetFloats(GL_CURRENT_COLOR, 4);
    color.m_v.HasValue = true;
    Emit<glColor4fvCommandSerializer>(color);
    glNormal3fvCommand normal;
    normal.m_v.Data = GetFloats(GL_CURRENT_NORMAL, 3);
    normal.m_v.HasValue = true;
    Emit<glNormal3fvCommandSerializer>(normal);
    if (m_Version.AtLeast(1, 4)) {
      glSecondaryColor3fvCommand secondaryColor;
      secondaryColor.m_v.Data = GetFloats(GL_CURRENT_SECONDARY_COLOR, 3);
      secondaryColor.m_v.HasValue = true;
      Emit<glSecondaryColor3fvCommandSerializer>(secondaryColor);
      glFogCoordfCommand fogCoord;
      fogCoord.m_coord.Value = GetFloat(GL_CURRENT_FOG_COORD);
      Emit<glFogCoordfCommandSerializer>(fogCoord);
    }
    GLboolean edgeFlag = GL_TRUE;
    GL_FN(glGetBooleanv)(GL_EDGE_FLAG, &edgeFlag);
    glEdgeFlagCommand edge;
    edge.m_flag.Value = edgeFlag;
    Emit<glEdgeFlagCommandSerializer>(edge);
  }

  for (GLenum target : ArbProgramTargets()) {
    const GLint parameters = GetArbProgram(target, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB);
    for (GLint i = 0; i < parameters; ++i) {
      glProgramEnvParameter4fvARBCommand command;
      command.m_target.Value = target;
      command.m_index.Value = static_cast<GLuint>(i);
      command.m_params.Data.resize(4);
      command.m_params.HasValue = true;
      GL_FN(glGetProgramEnvParameterfvARB)
      (target, command.m_index.Value, command.m_params.Data.data());
      if (!IsZero(command.m_params.Data)) {
        Emit<glProgramEnvParameter4fvARBCommandSerializer>(command);
      }
    }
  }

  RestoreFixedFunctionTransforms();
}

// Light positions and directions, clip planes and eye-linear texture coordinate planes are kept
// in eye coordinates. They are written under an identity modelview, which leaves them unchanged,
// before the matrices themselves.
void StateRestorer::RestoreFixedFunctionTransforms() {
  const GLint activeTexture = GetInteger(GL_ACTIVE_TEXTURE);
  const GLint matrixMode = GetInteger(GL_MATRIX_MODE);
  {
    glMatrixModeCommand mode;
    mode.m_mode.Value = GL_MODELVIEW;
    Emit<glMatrixModeCommandSerializer>(mode);
    glLoadIdentityCommand identity;
    Emit<glLoadIdentityCommandSerializer>(identity);
  }
  const GLint lights = GetInteger(GL_MAX_LIGHTS);
  for (GLint i = 0; i < lights; ++i) {
    const GLenum light = GL_LIGHT0 + static_cast<GLenum>(i);
    for (const VectorParameter& parameter : LIGHT_PARAMETERS) {
      glLightfvCommand command;
      command.m_light.Value = light;
      command.m_pname.Value = parameter.Pname;
      command.m_params.Data.resize(4);
      command.m_params.HasValue = true;
      GL_FN(glGetLightfv)(light, parameter.Pname, command.m_params.Data.data());
      command.m_params.Data.resize(parameter.Count);
      Emit<glLightfvCommandSerializer>(command);
    }
  }
  const GLint clipPlanes = GetInteger(GL_MAX_CLIP_PLANES);
  for (GLint i = 0; i < clipPlanes; ++i) {
    glClipPlaneCommand command;
    command.m_plane.Value = GL_CLIP_PLANE0 + static_cast<GLenum>(i);
    command.m_equation.Data.resize(4);
    command.m_equation.HasValue = true;
    GL_FN(glGetClipPlane)(command.m_plane.Value, command.m_equation.Data.data());
    Emit<glClipPlaneCommandSerializer>(command);
  }

  const bool multitexture = m_Version.AtLeast(1, 3);
  const GLint environmentUnits = multitexture ? GetInteger(GL_MAX_TEXTURE_UNITS) : 1;
  const GLint coordinateUnits =
      m_Version.AtLeast(2, 0) ? GetInteger(GL_MAX_TEXTURE_COORDS) : environmentUnits;
  for (GLint unit = 0; unit < std::max(environmentUnits, coordinateUnits); ++unit) {
    if (multitexture) {
      GL_FN(glActiveTexture)(GL_TEXTURE0 + static_cast<GLenum>(unit));
      glActiveTextureCommand active;
      active.m_texture.Value = GL_TEXTURE0 + static_cast<GLenum>(unit);
      Emit<glActiveTextureCommandSerializer>(active);
    }
    RestoreTextureUnit(unit, unit < coordinateUnits, unit < environmentUnits);
  }
  if (multitexture) {
    GL_FN(glActiveTexture)(static_cast<GLenum>(activeTexture));
  }

  for (const auto& [mode, matrix] : {std::pair{GL_PROJECTION, GL_PROJECTION_MATRIX},
                                     std::pair{GL_MODELVIEW, GL_MODELVIEW_MATRIX}}) {
    glMatrixModeCommand matrixModeCommand;
    matrixModeCommand.m_mode.Value = mode;
    Emit<glMatrixModeCommandSerializer>(matrixModeCommand);
    glLoadMatrixfCommand load;
    load.m_m.Data = GetFloats(matrix, 16);
    load.m_m.HasValue = true;
    Emit<glLoadMatrixfCommandSerializer>(load);
  }
  glMatrixModeCommand mode;
  mode.m_mode.Value = static_cast<GLenum>(matrixMode);
  Emit<glMatrixModeCommandSerializer>(mode);
}

// State of the active texture unit: coordinate generation, the current coordinates and the
// texture matrix for the units with coordinates, the environment and the enabled targets for
// the fixed-function units.
void StateRestorer::RestoreTextureUnit(GLint unit, bool coordinates, bool environment) {
  if (coordinates) {
    for (const TextureCoordinate& coordinate : TEXTURE_COORDINATES) {
      glTexGeniCommand mode;
      mode.m_coord.Value = coordinate.Coordinate;
      mode.m_pname.Value = GL_TEXTURE_GEN_MODE;
      GL_FN(glGetTexGeniv)(coordinate.Coordinate, GL_TEXTURE_GEN_MODE, &mode.m_param.Value);
      Emit<glTexGeniCommandSerializer>(mode);
      for (GLenum plane : {GL_OBJECT_PLANE, GL_EYE_PLANE}) {
        glTexGendvCommand command;
        command.m_coord.Value = coordinate.Coordinate;
        command.m_pname.Value = plane;
        command.m_params.Data.resize(4);
        command.m_params.HasValue = true;
        GL_FN(glGetTexGendv)(coordinate.Coordinate, plane, command.m_params.Data.data());
        Emit<glTexGendvCommandSerializer>(command);
      }
      EmitCapability(coordinate.Generation, GL_FN(glIsEnabled)(coordinate.Generation) != GL_FALSE);
    }
    if (m_Version.AtLeast(1, 3)) {
      glMultiTexCoord4fvCommand current;
      current.m_target.Value = GL_TEXTURE0 + static_cast<GLenum>(unit);
      current.m_v.Data = GetFloats(GL_CURRENT_TEXTURE_COORDS, 4);
      current.m_v.HasValue = true;
      Emit<glMultiTexCoord4fvCommandSerializer>(current);
    }
    glMatrixModeCommand mode;
    mode.m_mode.Value = GL_TEXTURE;
    Emit<glMatrixModeCommandSerializer>(mode);
    glLoadMatrixfCommand load;
    load.m_m.Data = GetFloats(GL_TEXTURE_MATRIX, 16);
    load.m_m.HasValue = true;
    Emit<glLoadMatrixfCommandSerializer>(load);
  }
  if (!environment) {
    return;
  }

  std::vector<GLenum> integerParameters{GL_TEXTURE_ENV_MODE};
  std::vector<GLenum> floatParameters;
  if (m_Version.AtLeast(1, 3)) {
    integerParameters.insert(integerParameters.end(), std::begin(TEXTURE_COMBINE_PARAMETERS),
                             std::end(TEXTURE_COMBINE_PARAMETERS));
    floatParameters = {GL_RGB_SCALE, GL_ALPHA_SCALE};
  }
  for (GLenum pname : integerParameters) {
    glTexEnviCommand command;
    command.m_target.Value = GL_TEXTURE_ENV;
    command.m_pname.Value = pname;
    GL_FN(glGetTexEnviv)(GL_TEXTURE_ENV, pname, &command.m_param.Value);
    Emit<glTexEnviCommandSerializer>(command);
  }
  for (GLenum pname : floatParameters) {
    glTexEnvfCommand command;
    command.m_target.Value = GL_TEXTURE_ENV;
    command.m_pname.Value = pname;
    GL_FN(glGetTexEnvfv)(GL_TEXTURE_ENV, pname, &command.m_param.Value);
    Emit<glTexEnvfCommandSerializer>(command);
  }
  glTexEnvfvCommand color;
  color.m_target.Value = GL_TEXTURE_ENV;
  color.m_pname.Value = GL_TEXTURE_ENV_COLOR;
  color.m_params.Data.resize(4);
  color.m_params.HasValue = true;
  GL_FN(glGetTexEnvfv)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color.m_params.Data.data());
  Emit<glTexEnvfvCommandSerializer>(color);
  if (m_Version.AtLeast(1, 4)) {
    glTexEnvfCommand lodBias;
    lodBias.m_target.Value = GL_TEXTURE_FILTER_CONTROL;
    lodBias.m_pname.Value = GL_TEXTURE_LOD_BIAS;
    GL_FN(glGetTexEnvfv)(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, &lodBias.m_param.Value);
    Emit<glTexEnvfCommandSerializer>(lodBias);
  }
  if (m_Version.AtLeast(2, 0)) {
    glTexEnviCommand coordReplace;
    coordReplace.m_target.Value = GL_POINT_SPRITE;
    coordReplace.m_pname.Value = GL_COORD_REPLACE;
    GL_FN(glGetTexEnviv)(GL_POINT_SPRITE, GL_COORD_REPLACE, &coordReplace.m_param.Value);
    Emit<glTexEnviCommandSerializer>(coordReplace);
  }
  for (const VersionedEnum& target : UNIT_TEXTURE_TARGETS) {
    if (m_Version.AtLeast(target.Major, target.Minor)) {
      EmitCapability(target.Value, GL_FN(glIsEnabled)(target.Value) != GL_FALSE);
    }
  }
}

namespace {

struct ClientArray {
  GLenum Array;
  GLenum Buffer;
  GLenum Size;
  GLenum Type;
  GLenum Stride;
  GLenum Pointer;
  int Major;
  int Minor;
};

constexpr ClientArray CLIENT_ARRAYS[] = {
    {GL_VERTEX_ARRAY, GL_VERTEX_ARRAY_BUFFER_BINDING, GL_VERTEX_ARRAY_SIZE, GL_VERTEX_ARRAY_TYPE,
     GL_VERTEX_ARRAY_STRIDE, GL_VERTEX_ARRAY_POINTER, 1, 1},
    {GL_NORMAL_ARRAY, GL_NORMAL_ARRAY_BUFFER_BINDING, GL_NONE, GL_NORMAL_ARRAY_TYPE,
     GL_NORMAL_ARRAY_STRIDE, GL_NORMAL_ARRAY_POINTER, 1, 1},
    {GL_COLOR_ARRAY, GL_COLOR_ARRAY_BUFFER_BINDING, GL_COLOR_ARRAY_SIZE, GL_COLOR_ARRAY_TYPE,
     GL_COLOR_ARRAY_STRIDE, GL_COLOR_ARRAY_POINTER, 1, 1},
    {GL_SECONDARY_COLOR_ARRAY, GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING,
     GL_SECONDARY_COLOR_ARRAY_SIZE, GL_SECONDARY_COLOR_ARRAY_TYPE, GL_SECONDARY_COLOR_ARRAY_STRIDE,
     GL_SECONDARY_COLOR_ARRAY_POINTER, 1, 4},
    {GL_FOG_COORD_ARRAY, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, 1, 4},
    {GL_EDGE_FLAG_ARRAY, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, 1, 1},
    {GL_INDEX_ARRAY, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, 1, 1},
};

GLint ClientTextureUnits(bool shaders) {
  return GetInteger(shaders ? GL_MAX_TEXTURE_COORDS : GL_MAX_TEXTURE_UNITS);
}

} // namespace

// Fixed-function arrays of the bound vertex array. Like generic attributes, those sourced from
// client memory are restored by the draws that use them.
void StateRestorer::RestoreClientArrays() {
  const auto restore = [this](const ClientArray& array) {
    if (GL_FN(glIsEnabled)(array.Array)) {
      glEnableClientStateCommand command;
      command.m_array.Value = array.Array;
      Emit<glEnableClientStateCommandSerializer>(command);
    }
    const GLuint buffer = array.Buffer != GL_NONE ? GetName(array.Buffer) : 0;
    if (buffer == 0) {
      return;
    }
    void* pointer = nullptr;
    GL_FN(glGetPointerv)(array.Pointer, &pointer);
    glBindBufferCommand bind;
    bind.m_target.Value = GL_ARRAY_BUFFER;
    bind.m_buffer.Key = KeyOf(GLObjectKind::Buffer, buffer);
    Emit<glBindBufferCommandSerializer>(bind);
    const GLint size = array.Size != GL_NONE ? GetInteger(array.Size) : 0;
    const auto type = static_cast<GLenum>(GetInteger(array.Type));
    const GLint stride = GetInteger(array.Stride);
    const auto offset = reinterpret_cast<uintptr_t>(pointer);
    switch (array.Array) {
    case GL_VERTEX_ARRAY: {
      glVertexPointerCommand command;
      command.m_size.Value = size;
      command.m_type.Value = type;
      command.m_stride.Value = stride;
      command.m_pointer.Value = offset;
      Emit<glVertexPointerCommandSerializer>(command);
      break;
    }
    case GL_NORMAL_ARRAY: {
      glNormalPointerCommand command;
      command.m_type.Value = type;
      command.m_stride.Value = stride;
      command.m_pointer.Value = offset;
      Emit<glNormalPointerCommandSerializer>(command);
      break;
    }
    case GL_COLOR_ARRAY: {
      glColorPointerCommand command;
      command.m_size.Value = size;
      command.m_type.Value = type;
      command.m_stride.Value = stride;
      command.m_pointer.Value = offset;
      Emit<glColorPointerCommandSerializer>(command);
      break;
    }
    case GL_SECONDARY_COLOR_ARRAY: {
      glSecondaryColorPointerCommand command;
      command.m_size.Value = size;
      command.m_type.Value = type;
      command.m_stride.Value = stride;
      command.m_pointer.Value = offset;
      Emit<glSecondaryColorPointerCommandSerializer>(command);
      break;
    }
    default:
      break;
    }
  };
  for (const ClientArray& array : CLIENT_ARRAYS) {
    if (m_Version.AtLeast(array.Major, array.Minor)) {
      restore(array);
    }
  }

  if (!m_Version.AtLeast(1, 3)) {
    return;
  }
  const GLint clientActiveTexture = GetInteger(GL_CLIENT_ACTIVE_TEXTURE);
  // Texture coordinate state queries fail while the active texture unit has no texture
  // coordinates (from GL_MAX_TEXTURE_COORDS on), so each unit is read with itself active.
  const GLint activeTexture = GetInteger(GL_ACTIVE_TEXTURE);
  const GLint units = ClientTextureUnits(m_Version.AtLeast(2, 0));
  for (GLint unit = 0; unit < units; ++unit) {
    const GLenum texture = GL_TEXTURE0 + static_cast<GLenum>(unit);
    GL_FN(glActiveTexture)(texture);
    GL_FN(glClientActiveTexture)(texture);
    const bool enabled = GL_FN(glIsEnabled)(GL_TEXTURE_COORD_ARRAY) != GL_FALSE;
    const GLuint buffer = GetName(GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING);
    if (!enabled && buffer == 0) {
      continue;
    }
    glClientActiveTextureCommand active;
    active.m_texture.Value = texture;
    Emit<glClientActiveTextureCommandSerializer>(active);
    if (enabled) {
      glEnableClientStateCommand command;
      command.m_array.Value = GL_TEXTURE_COORD_ARRAY;
      Emit<glEnableClientStateCommandSerializer>(command);
    }
    if (buffer != 0) {
      void* pointer = nullptr;
      GL_FN(glGetPointerv)(GL_TEXTURE_COORD_ARRAY_POINTER, &pointer);
      glBindBufferCommand bind;
      bind.m_target.Value = GL_ARRAY_BUFFER;
      bind.m_buffer.Key = KeyOf(GLObjectKind::Buffer, buffer);
      Emit<glBindBufferCommandSerializer>(bind);
      glTexCoordPointerCommand command;
      command.m_size.Value = GetInteger(GL_TEXTURE_COORD_ARRAY_SIZE);
      command.m_type.Value = static_cast<GLenum>(GetInteger(GL_TEXTURE_COORD_ARRAY_TYPE));
      command.m_stride.Value = GetInteger(GL_TEXTURE_COORD_ARRAY_STRIDE);
      command.m_pointer.Value = reinterpret_cast<uintptr_t>(pointer);
      Emit<glTexCoordPointerCommandSerializer>(command);
    }
  }
  GL_FN(glActiveTexture)(static_cast<GLenum>(activeTexture));
  GL_FN(glClientActiveTexture)(static_cast<GLenum>(clientActiveTexture));
  glClientActiveTextureCommand active;
  active.m_texture.Value = static_cast<GLenum>(clientActiveTexture);
  Emit<glClientActiveTextureCommandSerializer>(active);
}

void StateRestorer::ResetClientArrays() {
  for (const ClientArray& array : CLIENT_ARRAYS) {
    if (m_Version.AtLeast(array.Major, array.Minor) && GL_FN(glIsEnabled)(array.Array)) {
      glDisableClientStateCommand command;
      command.m_array.Value = array.Array;
      Emit<glDisableClientStateCommandSerializer>(command);
    }
  }

  if (!m_Version.AtLeast(1, 3)) {
    return;
  }
  const GLint clientActiveTexture = GetInteger(GL_CLIENT_ACTIVE_TEXTURE);
  bool switched = clientActiveTexture != GL_TEXTURE0;
  const GLint units = ClientTextureUnits(m_Version.AtLeast(2, 0));
  for (GLint unit = 0; unit < units; ++unit) {
    const GLenum texture = GL_TEXTURE0 + static_cast<GLenum>(unit);
    GL_FN(glClientActiveTexture)(texture);
    if (!GL_FN(glIsEnabled)(GL_TEXTURE_COORD_ARRAY)) {
      continue;
    }
    glClientActiveTextureCommand active;
    active.m_texture.Value = texture;
    Emit<glClientActiveTextureCommandSerializer>(active);
    glDisableClientStateCommand command;
    command.m_array.Value = GL_TEXTURE_COORD_ARRAY;
    Emit<glDisableClientStateCommandSerializer>(command);
    switched = true;
  }
  GL_FN(glClientActiveTexture)(static_cast<GLenum>(clientActiveTexture));
  if (switched) {
    glClientActiveTextureCommand active;
    active.m_texture.Value = GL_TEXTURE0;
    Emit<glClientActiveTextureCommandSerializer>(active);
  }
}

void StateRestorer::RestoreArbPrograms(GITSKey shareGroup) {
  const std::vector<GLenum> targets = ArbProgramTargets();
  for (const auto& [key, program] : m_State.ArbPrograms()) {
    if (program.ShareGroup != shareGroup) {
      continue;
    }
    glGenProgramsARBCommand gen;
    gen.m_n.Value = 1;
    gen.m_programs.Key = {key};
    gen.m_programs.Value = {0};
    gen.m_programs.HasValue = true;
    Emit<glGenProgramsARBCommandSerializer>(gen);
    const GLuint name = NameOf(GLObjectKind::Program, key);
    const GLenum target = program.Target;
    if (name == 0 || std::find(targets.begin(), targets.end(), target) == targets.end()) {
      continue;
    }
    const auto bound = static_cast<GLuint>(GetArbProgram(target, GL_PROGRAM_BINDING_ARB));
    GL_FN(glBindProgramARB)(target, name);
    glBindProgramARBCommand bind;
    bind.m_target.Value = target;
    bind.m_program.Key = key;
    Emit<glBindProgramARBCommandSerializer>(bind);
    const GLint length = GetArbProgram(target, GL_PROGRAM_LENGTH_ARB);
    if (length > 0) {
      glProgramStringARBCommand source;
      source.m_target.Value = target;
      source.m_format.Value = GL_PROGRAM_FORMAT_ASCII_ARB;
      source.m_len.Value = length;
      source.m_string.Data.resize(static_cast<size_t>(length));
      source.m_string.HasValue = true;
      GL_FN(glGetProgramStringARB)(target, GL_PROGRAM_STRING_ARB, source.m_string.Data.data());
      Emit<glProgramStringARBCommandSerializer>(source);
      const GLint parameters = GetArbProgram(target, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB);
      for (GLint i = 0; i < parameters; ++i) {
        glProgramLocalParameter4fvARBCommand command;
        command.m_target.Value = target;
        command.m_index.Value = static_cast<GLuint>(i);
        command.m_params.Data.resize(4);
        command.m_params.HasValue = true;
        GL_FN(glGetProgramLocalParameterfvARB)
        (target, command.m_index.Value, command.m_params.Data.data());
        if (!IsZero(command.m_params.Data)) {
          Emit<glProgramLocalParameter4fvARBCommandSerializer>(command);
        }
      }
    }
    GL_FN(glBindProgramARB)(target, bound);
  }
}

void StateRestorer::RestoreArbProgramBindings() {
  for (GLenum target : ArbProgramTargets()) {
    if (const auto name = static_cast<GLuint>(GetArbProgram(target, GL_PROGRAM_BINDING_ARB))) {
      glBindProgramARBCommand command;
      command.m_target.Value = target;
      command.m_program.Key = ArbProgramKeyOf(name);
      Emit<glBindProgramARBCommandSerializer>(command);
    }
  }
}

} // namespace gits::opengl
