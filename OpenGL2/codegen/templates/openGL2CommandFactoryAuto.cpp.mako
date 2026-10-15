// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Generated CreateCommand() body; the class declaration is hand-written (see
// player/include/openGL2CommandFactory.h). Mirrors
// Vulkan/codegen/templates/vulkanCommandFactoryAuto.cpp.mako's identical split. Common
// command ids (ID_INIT_START/ID_INIT_END/ID_FRAME_END/ID_MARKER_UINT64) are deliberately not
// handled here - player/streamPlayer.cpp's shared CommonCommandFactory already covers those
// for every backend (see docs/development/opengl2-player-skeleton-plan.md's Decision 2).

#include "openGL2CommandFactory.h"
#include "commandIdsAuto.h"
#include "commandRunnersAuto.h"
#include "commandRunnersCustom.h"
#include "mappedDataMetaRunner.h"
#include "unsupportedCommandRunner.h"

namespace gits::opengl2 {

stream::CommandRunner* OpenGL2CommandFactory::CreateCommand(unsigned id) {
  switch (static_cast<CommandId>(id)) {
  // Hand-written cases; not part of the per-command loop below - see
  // player/include/unsupportedCommandRunner.h and player/include/mappedDataMetaRunner.h.
  case CommandId::ID_META_UNSUPPORTED_COMMAND:
    return new UnsupportedCommandRunner();
  case CommandId::ID_META_MAPPED_DATA:
    return new MappedDataMetaRunner();
% for name, runner_class in custom_player_runners.items():
  // Hand-written case, overriding the generated (but unused for this one name - see
  // custom_player_runners.py) ${name}Runner below - see player/include/commandRunnersCustom.h.
  case CommandId::ID_${name.upper()}:
    return new ${runner_class}();
% endfor
% for cmd in commands:
% if cmd.enabled and cmd.name not in custom_player_runners:
  case CommandId::ID_${cmd.name.upper()}:
    return new ${cmd.name}Runner();
% endif
% endfor
  default:
    return nullptr;
  }
}

} // namespace gits::opengl2
