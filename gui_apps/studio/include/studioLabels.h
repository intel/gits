// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "baseLabels.h"

namespace gits::gui {

namespace Labels {
static constexpr const char* LIVE_MODE = ICON_MDI_APPLICATION " Live Mode";
static constexpr const char* STREAM_MODE = ICON_MDI_FILE_DOCUMENT " Stream Mode";

static constexpr const char* BLANK_CONTENT_AREA = "No panel selected on the right side.";

static constexpr const char* LAUNCH_WORKLOAD = "Launch Target";

static constexpr const char* NO_TRACEFILE = "No tracefile present.";

namespace Menu {
static constexpr const char* ANALYSIS = "Analysis";
static constexpr const char* ANALYSIS_FULL = "Full Analysis";
static constexpr const char* ANALYSIS_SCREENSHOTS = "Screenshot Stream";
static constexpr const char* ANALYSIS_TRACE = "Trace Stream";
static constexpr const char* VIEW_METADATA = "View Metadata";
static constexpr const char* VIEW_TRACE = "View Trace";
}; // namespace Menu

namespace NavBar {
static constexpr const char* LOAD_STREAM = "Prepare Playback";
static constexpr const char* RESET_STREAM = "Restart Playback";
static constexpr const char* STOP_PLAYBACK = ICON_MDI_STOP " Stop";
static constexpr const char* START_PLAYBACK = ICON_MDI_PLAY " Start";
static constexpr const char* PAUSE_PLAYBACK = ICON_MDI_PAUSE "Pause";
static constexpr const char* PLAY_UNTIL = ICON_MDI_SKIP_NEXT " Play Until";
static constexpr const char* STEP_PLAYBACK = ICON_MDI_STEP_FORWARD " Step";
static constexpr const char* SECTION_LABEL = "Section";
static constexpr const char* MARK_START = ICON_MDI_RAY_START " Mark Start";
static constexpr const char* MARK_END = ICON_MDI_RAY_END " Mark End";
}; // namespace NavBar

namespace Status {
static constexpr const char* PLAYBACK_STOPPING = "Stopping player; waiting for process exit.";
static constexpr const char* NO_STREAM_ERROR =
    "Select an existing stream and player executable before loading.";
static constexpr const char* PIPE_ALREADY_OWNED =
    "Cannot own the status pipe. Close other Studio/player sessions first.";

static constexpr const char* LOADING_IPC = "Loading player; waiting for IPC connection.";
static constexpr const char* UNSUPPORTED_STREAM =
    "Remote control supports newer PlayStream streams only.";
static constexpr const char* NON_INTERACTIVE_MODE =
    "Remote control requires interactive mode in the playback configuration.";

static constexpr const char* PLAYER_FAILED_PREFIX = " Player launch failed : ";

static constexpr const char* SESSION_ENDED = "Player session ended. Load/Reset to relaunch.";
static constexpr const char* PLAYER_FAILED =
    "Player failed to launch or exited with an error. See the log.";

static constexpr const char* COMMAND_FAILED =
    "Command failed; reconnect before retrying. No new player has been launched.";
static constexpr const char* WAITING_CONNECTION =
    "Waiting for IPC connection; process exit has not been confirmed.";

static constexpr const char* PAUSED = "Paused";
static constexpr const char* RUNNING = "Running";
static constexpr const char* DONE = "Playback finished; waiting for process exit.";
}; // namespace Status

} // namespace Labels
} // namespace gits::gui
