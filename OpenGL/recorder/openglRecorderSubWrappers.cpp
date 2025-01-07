// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openglRecorderSubWrappers.h
*
* @brief Automatically generated declarations of OpenGL library simple function call wrappers.
*
*/

#include "openglRecorderSubWrappers.h"

void gits::OpenGL::RestoreDisplayListState(CRecorder& recorder) {
  GLenum clientStateValues[] = {
      GL_COLOR_ARRAY,  GL_EDGE_FLAG_ARRAY,       GL_FOG_COORD_ARRAY,     GL_INDEX_ARRAY,
      GL_NORMAL_ARRAY, GL_SECONDARY_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_VERTEX_ARRAY};
  unsigned int clientStateValuesSize = sizeof(clientStateValues) / sizeof(GLenum);
  CScheduler& scheduler = recorder.Scheduler();
  for (unsigned i = 0; i < clientStateValuesSize; i++) {
    recorder.Schedule(new CglDisableClientState(clientStateValues[i]), true);
  }

  for (unsigned i = 0; i < clientStateValuesSize; i++) {
    std::shared_ptr<CVariableClientCapability> variableClientCapability(
        new CVariableClientCapability(clientStateValues[i]));
    variableClientCapability->Get();
    variableClientCapability->Schedule(scheduler, CVariableClientCapability(clientStateValues[i]));
  }

  std::shared_ptr<CVariableColorPointer> variableColorPointer(new CVariableColorPointer());
  variableColorPointer->Get();
  variableColorPointer->Schedule(scheduler, CVariableColorPointer());

  std::shared_ptr<CVariableSecondaryColorPointer> variableSecondaryColorPointer(
      new CVariableSecondaryColorPointer());
  variableSecondaryColorPointer->Get();
  variableSecondaryColorPointer->Schedule(scheduler, CVariableSecondaryColorPointer());

  std::shared_ptr<CVariableVertexPointer> variableVertexPointer(new CVariableVertexPointer());
  variableVertexPointer->Get();
  variableVertexPointer->Schedule(scheduler, CVariableVertexPointer());

  std::shared_ptr<CVariableNormalPointer> variableNormalPointer(new CVariableNormalPointer());
  variableNormalPointer->Get();
  variableNormalPointer->Schedule(scheduler, CVariableNormalPointer());

  std::shared_ptr<CVariableTexCoordInfo> variableTexCoordInfo(new CVariableTexCoordInfo());
  variableTexCoordInfo->Get();
  variableTexCoordInfo->Schedule(scheduler, CVariableTexCoordInfo());

  std::shared_ptr<CVariableVertexAttribInfo> variableVertexAttribPointer(
      new CVariableVertexAttribInfo());
  variableVertexAttribPointer->Get();
  variableVertexAttribPointer->Schedule(scheduler, CVariableVertexAttribInfo());
}
