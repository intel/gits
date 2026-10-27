// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "clientArraysLayerAuto.h"

#include "captureManager.h"
#include "clientArraysService.h"

namespace gits::opengl {

% for name, draw in client_array_draws.items():
void ClientArraysLayer::Pre(${name}Command& command) {
  ClientArraysDraw draw{};
% if draw.draw_count_param:
  draw.DrawCount = command.m_${draw.draw_count_param}.Value;
  draw.Counts = command.m_count.HasValue ? command.m_count.Data.data() : nullptr;
% if draw.is_elements:
  draw.IsElements = true;
  draw.IndexType = command.m_type.Value;
  draw.IndicesOffsets = command.m_indices.HasValue ? command.m_indices.Data.data() : nullptr;
% else:
  draw.Firsts = command.m_${draw.first_param}.HasValue ? command.m_${draw.first_param}.Data.data() : nullptr;
% endif
% if draw.base_vertex_param:
  draw.BaseVertices = command.m_${draw.base_vertex_param}.HasValue
                          ? command.m_${draw.base_vertex_param}.Data.data()
                          : nullptr;
% endif
% else:
  draw.Count = command.m_count.Value;
% if draw.is_elements:
  draw.IsElements = true;
  draw.IndexType = command.m_type.Value;
  draw.Indices = &command.m_indices;
  draw.IndicesOffset = command.m_indicesRawPointer.Value;
% else:
  draw.First = command.m_${draw.first_param}.Value;
% endif
% if draw.base_vertex_param:
  draw.BaseVertex = command.m_${draw.base_vertex_param}.Value;
% endif
% if draw.instance_count_param:
  draw.InstanceCount = command.m_${draw.instance_count_param}.Value;
% endif
% if draw.base_instance_param:
  draw.BaseInstance = command.m_${draw.base_instance_param}.Value;
% endif
% endif
  ClientArraysService::Get().Capture(command.m_ThreadId, draw,
                                     CaptureManager::Get().TakePendingPreHookKey());
  Pre(static_cast<Command&>(command));
}

% endfor
} // namespace gits::opengl
