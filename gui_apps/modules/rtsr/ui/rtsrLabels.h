// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "baseLabels.h"

namespace gits::gui {

// Text shown by this module's panel. It lives in the module rather than in the
// host application's labels.h, so that adding a module does not mean editing the
// launcher.
struct RtsrLabels {
  static constexpr const char* ENABLE_CHECKBOX = "Enable real-time shader replacement module";
  static constexpr const char* DISABLED_HINT =
      "Enable the module to attach to the next stream playback or launched target. Only DirectX "
      "12 and Vulkan runs are supported.";
  static constexpr const char* PORTS_TAKEN =
      "The RTSR ports are already in use. Close the other RTSR server, RTSREditor or a second "
      "GITS GUI, and enable the module again.";

  static constexpr const char* WAITING_FOR_CLIENT =
      "Listening. Load a DirectX 12 or Vulkan stream, or launch such a target, and RTSR attaches "
      "to it.";
  static constexpr const char* SESSION_ENDED =
      "The run has ended. Its harvested data is kept until the next run starts.";
  static constexpr const char* SESSION_ATTACHED_ELSEWHERE =
      "RTSR is still attached to a process started in the other mode. A run launched in this "
      "mode cannot be instrumented until that process exits.";
  static constexpr const char* SESSION_APPLICATION = "Application";
  static constexpr const char* SESSION_PATH = "Path";
  static constexpr const char* SESSION_API = "API";
  static constexpr const char* SESSION_CONTENTS = "Harvested";

  static constexpr const char* FILTER_HINT = "Filter by name or id";
  static constexpr const char* UNNAMED_PIPELINE = "(unnamed)";
  static constexpr const char* NO_SELECTION = "Select a pipeline to see its shaders.";

  static constexpr const char* COLUMN_ID = "Id";
  static constexpr const char* COLUMN_NAME = "Name";
  static constexpr const char* COLUMN_TYPE = "Type";
  static constexpr const char* COLUMN_STAGES = "Stages";

  static constexpr const char* COLUMN_STAGE = "Stage";
  static constexpr const char* COLUMN_HASH = "Bytecode hash";
  static constexpr const char* COLUMN_ENTRYPOINT = "Entrypoint";
  static constexpr const char* COLUMN_BYTECODE = "Bytecode";
  static constexpr const char* COLUMN_MODEL = "Model";
  static constexpr const char* COLUMN_BYTES = "Bytes";

  static constexpr const char* STATE_HASH = "State hash";
  static constexpr const char* UNKNOWN = "Unknown";
  static constexpr const char* NOT_AVAILABLE = "-";

  static constexpr const char* PIPELINES_FILTER_GFX = "Gfx";
  static constexpr const char* PIPELINES_FILTER_GFX_HINT = "Filter Graphics Pipelines";
  static constexpr const char* PIPELINES_FILTER_CMP = "Cmp";
  static constexpr const char* PIPELINES_FILTER_CMP_HINT = "Filter Compute Pipelines";
  static constexpr const char* PIPELINES_FILTER_RAY = "Ray";
  static constexpr const char* PIPELINES_FILTER_RAY_HINT = "Filter RayTracing Pipelines";

  // Tabs.
  static constexpr const char* TAB_PIPELINES = ICON_MDI_PIPE " Pipelines";
  static constexpr const char* TAB_PERFORMANCE = ICON_MDI_SPEEDOMETER " Performance";
  static constexpr const char* TAB_PIPELINE_STATE = ICON_MDI_PIPE_VALVE " Pipeline state";
  static constexpr const char* SOURCE_PENDING = "Preparing shader source...";
  static constexpr const char* SOURCE_FAILED = "Shader source is unavailable.";
  static constexpr const char* SOURCE_HLSL = "Reconstructed HLSL (read-only)";
  static constexpr const char* SOURCE_HLSL_EDITABLE = "Reconstructed HLSL (editable)";
  static constexpr const char* SOURCE_GLSL = "Reconstructed GLSL (read-only)";
  static constexpr const char* SOURCE_GLSL_EDITABLE = "Reconstructed GLSL (editable)";
  static constexpr const char* SOURCE_DISASSEMBLY = "Disassembly (read-only)";
  static constexpr const char* COMPILE_AND_APPLY = "Compile and apply";
  static constexpr const char* COMPILE_AND_APPLY_HINT =
      "Compile this draft and replace the selected stage in the current pipeline";
  static constexpr const char* DISCARD_EDITS = "Discard edits";
  static constexpr const char* DISCARD_EDITS_HINT =
      "Restore the reconstructed source in the editor; this does not change the running pipeline";
  static constexpr const char* RESTORE_PIPELINE = "Restore original pipeline";
  static constexpr const char* RESTORE_PIPELINE_HINT =
      "Remove every replacement currently installed for this pipeline";
  static constexpr const char* APPLY_PENDING = "Compiling and applying the replacement...";
  static constexpr const char* APPLY_SUCCEEDED = "Replacement applied.";
  static constexpr const char* APPLY_FAILED = "Replacement failed.";
  static constexpr const char* RESET_PENDING = "Restoring the original pipeline...";
  static constexpr const char* RESET_SUCCEEDED = "Original pipeline restored.";
  static constexpr const char* RESET_FAILED = "The original pipeline could not be restored.";
  static constexpr const char* OPERATION_CANCELLED = "The operation was cancelled.";
  static constexpr const char* REPLACEMENT_ACTIVE = "A replacement is active for this pipeline.";

  // Plots.
  static constexpr const char* MOUSE_NAVIGATION = "Mouse navigation";
  static constexpr const char* MOUSE_NAVIGATION_HINT =
      "Pan and zoom freely. Turns off automatic scaling and selection by click.";

  // Performance tab, toolbar.
  static constexpr const char* COLLECT_START = ICON_MDI_RECORD_CIRCLE " Start Live Data Collection";
  static constexpr const char* COLLECT_END = ICON_MDI_PAUSE " Pause Live Data Collection";
  static constexpr const char* COLLECT_DISABLED =
      ICON_MDI_CIRCLE_OFF_OUTLINE " Live data collection unavailable";
  static constexpr const char* COLLECT_HINT =
      "Ask the connected client to time every pipeline it binds";
  static constexpr const char* COLLECT_REFUSED =
      "The client did not accept the request. Pipeline timing is not implemented in the plugin "
      "yet.";
  static constexpr const char* RESET_VIEW = ICON_MDI_REFRESH " Reset view";
  static constexpr const char* RESET_VIEW_HINT =
      "Drop the selection and return both plots to automatic scaling";
  static constexpr const char* PERF_EMPTY =
      "No timings yet. Enable live data collection, then run a stream.";
  static constexpr const char* PERF_NO_FRAME = "Select a frame in the plot above.";

  // Performance tab, frame plot.
  static constexpr const char* FRAME_PLOT = "Frame present-to-present times";
  static constexpr const char* FRAME_AXIS_X = "Frame";
  static constexpr const char* FRAME_AXIS_Y = "Present to present [ms]";
  static constexpr const char* AUTOSCALE_X = "Autoscale X";
  static constexpr const char* AUTOSCALE_X_HINT = "Fit the X axis to the frames received";
  static constexpr const char* FRAME_WINDOW = "# Frames";
  static constexpr const char* FRAME_WINDOW_HINT = "How many of the most recent frames to show";
  static constexpr const char* AUTOSCALE_Y = "Autoscale Y";
  static constexpr const char* AUTOSCALE_Y_HINT = "Fit the Y axis to the longest frame";
  static constexpr const char* AXIS_Y_MAX = "Max [ms]";
  static constexpr const char* AXIS_Y_MAX_HINT =
      "Fixed upper limit, so frames stay comparable as data scrolls";
  static constexpr const char* FRAME_SELECTION = "Frame selection";
  static constexpr const char* FRAME_SELECTION_HINT =
      "Pick a frame, or a run of frames, to break down below";
  static constexpr const char* SELECTION_LENGTH = "Length";
  static constexpr const char* SELECTION_LENGTH_HINT = "How many frames the selection covers";
  static constexpr const char* STICKY_SELECTION = "Sticky selection";
  static constexpr const char* STICKY_SELECTION_HINT =
      "Follow the selected frame as data scrolls, instead of holding the position";
  static constexpr const char* PREVIOUS = "<";
  static constexpr const char* NEXT = ">";
  static constexpr const char* PREVIOUS_FRAME_HINT = "Select the previous frame";
  static constexpr const char* NEXT_FRAME_HINT = "Select the next frame";

  // Performance tab, pipelines plot.
  static constexpr const char* PIPELINE_AXIS_X = "Pipeline";
  static constexpr const char* ACCUMULATE = "Accumulate";
  static constexpr const char* ACCUMULATE_HINT =
      "Sum the bars left to right, to see how few pipelines account for the frame";
  static constexpr const char* SELECTED_PIPELINE = "Selected pipeline";
  static constexpr const char* PREVIOUS_PIPELINE_HINT = "Select the pipeline to the left";
  static constexpr const char* NEXT_PIPELINE_HINT = "Select the pipeline to the right";
  static constexpr const char* PIPELINE_PROPERTY = "Property";
  static constexpr const char* PIPELINE_PROPERTY_HINT = "Which metric the bars show and sort by";
  static constexpr const char* SORT = "Sort";
  static constexpr const char* FIELD_TOTAL_DURATION = "Total duration";
  static constexpr const char* FIELD_MIN_DURATION = "Min duration";
  static constexpr const char* FIELD_MAX_DURATION = "Max duration";
  static constexpr const char* FIELD_OCCURRENCES = "Occurrences";
  static constexpr const char* SORT_DESCENDING = "Descending";
  static constexpr const char* SORT_ASCENDING = "Ascending";

  // Performance tab, details.
  static constexpr const char* DETAILS_SELECTION = "Selection";
  static constexpr const char* DETAILS_PIPELINE = "Selected pipeline";
  static constexpr const char* DETAILS_EXTREMES = "Across selection";
  static constexpr const char* DETAILS_VALUE = "Value";
  static constexpr const char* ROW_FRAMES = "Frames";
  static constexpr const char* ROW_DURATION_CPU = "Duration CPU";
  static constexpr const char* ROW_DURATION_GPU = "Duration GPU";
  static constexpr const char* ROW_PIPELINES = "Pipelines";
  static constexpr const char* ROW_ID = "Id";
  static constexpr const char* ROW_OCCURRENCES = "Occurrences";
  static constexpr const char* ROW_DURATION = "Duration";
  static constexpr const char* ROW_SHARE = "Share of GPU time";
  static constexpr const char* ROW_MIN_DURATION = "Min duration";
  static constexpr const char* ROW_MAX_DURATION = "Max duration";
  static constexpr const char* GO_TO_PIPELINE = ICON_MDI_FORWARD " Go to pipeline";
  static constexpr const char* GO_TO_PIPELINE_HINT = "Show this pipeline on the Shaders tab";
};

} // namespace gits::gui
