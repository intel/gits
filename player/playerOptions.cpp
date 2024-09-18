// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "gits.h"
#include "playerOptions.h"
#include "log.h"
#include "config.h"
#include "getopt_.h"
#include "pragmas.h"
#include "exception.h"
#include "tools.h"
#include "lua_bindings.h"
#include "ptblLibrary.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace gits {

namespace {
bool ends_with(const std::string& str, const std::string& ending) {
  if (str.size() < ending.size()) {
    return false;
  }

  return str.substr(str.size() - ending.size()) == ending;
}

void fixup_args(std::vector<std::string>& input_args,
                std::vector<char*>& storage,
                int& argc,
                char**& argv) {
  storage.clear();
  for (auto& arg : input_args) {
    storage.push_back(const_cast<char*>(arg.c_str()));
  }

  argc = static_cast<int>(input_args.size());
  argv = &storage[0];
}

/*
  Obsoleted arguments are removed from the command line, with a message.
*/
void obsolete_args_override(int& argc, char**& argv) {
  static std::vector<std::string> lines;
  for (int i = 0; i < argc; ++i) {
    lines.push_back(argv[i]);
  }

  auto matches = [](const std::string& arg, const std::string& ref) {
    return (arg == "-" + ref) || (arg == "--" + ref);
  };
  auto remove_from_lines = [&](const std::string& ref, bool has_arg) -> void {
    size_t i = 0;
    while (i < lines.size()) {
      if (matches(lines[i], ref)) {
        auto last = lines.begin() + i + 1;
        if (last != lines.end() && has_arg) {
          last++;
        }
        lines.erase(lines.begin() + i, last);
        Log(WARN) << "Option " << ref << " is obsoleted and will have no effect";
      } else {
        i++;
      }
    }
  };

  // Options with no effect.
  remove_from_lines("playbackOnOneThreadWA", false);

  static std::vector<char*> args;
  fixup_args(lines, args, argc, argv);
}

int console_columns() {
  const int default_width = 99;
#ifdef GITS_PLATFORM_WINDOWS
  CONSOLE_SCREEN_BUFFER_INFO csbi = {};
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return std::max((int)csbi.dwSize.X, default_width);
  }
#else
  try {
    int width = std::stoi(CommandOutput("echo -n $COLUMNS", false));
    return std::max(width, default_width);
  } catch (...) {
  }
#endif
  return default_width;
}

template <class T, class U>
void set_when_option_present(T& variable, U& option) {
  if (option.Present()) {
    variable = option.Value();
  }
}
} // namespace
template <class T>
void process_lua_handled_option(const T& opt) {
  if (opt.Present()) {
    Log(INFO) << "Option: '" << opt.LongName()
              << "' is no longer present in GITS core. "
                 "Please use coresponding lua script provided in GITS install directory instead.";
    throw std::runtime_error("deprecated option specified");
  }
}

bool configure_player(int argc, char** argv) {
  obsolete_args_override(argc, argv);

  // command line options parser
  CGetOpt options(argc, argv);

  // command line options
  TypedOption<bool> optionHelp(options, OPTION_GROUP_GENERAL, 'h', "help",
                               "Displays this help message. This is equivalent to \"-hh general\""
                               "if -hh is not specified.");

  TypedOption<std::string> optionGroupHelp(
      options, OPTION_GROUP_GENERAL, 0, "hh",
      "Displays description of options is specified group.\n"
      "Possible values for group are:\n\n"
      "  general     - general GITS behavior options.\n\n"
      "  playback    - playback control options (like early stream exit).\n\n"
      "  image       - options for images (png files) gathering.\n\n"
      "  metrics     - options for doing various measurements/statistics.\n\n"
      "  performance - options that can affect GITS playback performance.\n\n"
      "  mutators    - options that modify GITS API stream in some way.\n\n"
      "  workaround  - workarounds needed for stream playback in some cases.\n\n"
      "  internal    - options of interest mostly for GITS dev team.\n\n"
      "  all         - all the options (union of prev. mentioned groups).\n\n");

  TypedOption<bool> optionVersion(options, OPTION_GROUP_GENERAL, 'v', "version",
                                  "Displays gitsPlayer version.");

  TypedOption<bool> optionTrace(
      options, OPTION_GROUP_GENERAL, 0, "trace",
      "[DEPRECATED] Legacy equivalent of the \"--logLevel TRACE\" option.");

  TypedOption<bool> optionTraceGLVerbose(
      options, OPTION_GROUP_GENERAL, 0, "traceGLVerbose",
      "[DEPRECATED] Legacy equivalent of the \"--logLevel TRACEVERBOSE\" option.");

  TypedOption<LogLevel> optionLogLevel(
      options, OPTION_GROUP_GENERAL, 0, "logLevel",
      "Only info with level greater or equal to this will be logged."
      " The levels are: TRACEVERBOSE, TRACE, INFOVERBOSE,  INFO (default), WARNING, ERROR and OFF."
      " With TRACE level API calls are logged. With TRACEVERBOSE additional details are logged,"
      " for example in OGL streams log will contain state like list of bound framebuffers"
      " with attachments, pipelines, programs, shaders, buffers and textures.");

  TypedOption<std::string> optionTraceData(
      options, OPTION_GROUP_GENERAL, 0, "traceData",
      "Avaliable if --logLevel TRACE selected.\n"
      "Enables logging additional info to trace specified by comma separated options.\n"
      "Options:\n"
      " * vkStructs - instead of logging addresses of Vulkan objects, GITS will log the "
      "structures, tables or other data behind them.\n"
      " * frameNumber - dump frame number for every api call.\n");

  TypedOption<bool> optionTraceGitsInternal(options, OPTION_GROUP_GENERAL, 0, "traceGitsInternal",
                                            "Causes --logLevel TRACEVERBOSE to print additional"
                                            " calls generated by Gits internally.");

  TypedOption<bool> optionTraceGLError(
      options, OPTION_GROUP_GENERAL, 0, "traceGLError",
      "If specified, glGetError will be called for each gl call and errors will be logged.");

  TypedOption<bool> optionPrintBuildLogsGL(
      options, OPTION_GROUP_GENERAL, 0, "printBuildLogsGL",
      "Print build statuses and logs from shader compilation and program linking.");

  TypedOption<bool> optionTraceVKStructs(
      options, OPTION_GROUP_GENERAL, 0, "traceVKStructs",
      "[DEPRECATED] Legacy equivalent of the \"--traceData vkStructs\" option.");

  TypedOption<bool> optionNoExceptionHandling(options, OPTION_GROUP_GENERAL, 'x', "noseh",
                                              "If specified, structured exceptions raised during "
                                              "playback will be passed to system unhandled.");

  TypedOption<std::filesystem::path> optionOutputDir(options, OPTION_GROUP_GENERAL, 0, "outputDir",
                                                     "Specifies directory where all the artifacts "
                                                     "will be stored.");

  TypedOption<bool> optionRecorderDiags(
      options, OPTION_GROUP_GENERAL, 0, "recorderDiags",
      "Causes player to output diagnostic info gathered during stream recording "
      "Then, player exits.");

  TypedOption<bool> optionTraceVKShaderHashes(
      options, OPTION_GROUP_GENERAL, 0, "traceVKShaderHashes",
      "Prints information about shader hashes in gits output (log). To match with drawing "
      "commands, use this option together with --logLevel TRACE.");

  TypedOption<bool> optionPrintStateRestoreLogsVk(
      options, OPTION_GROUP_GENERAL, 0, "printStateRestoreLogsVk",
      "Log state restore phases and their duration in Vulkan workloads.");

  TypedOption<bool> optionPrintMemUsageVk(
      options, OPTION_GROUP_GENERAL, 0, "printMemUsageVk",
      "Log memory usage in Vulkan workloads (on vkAllocateMemory, vkFreeMemory, vkMapMemory API "
      "calls).");

  TypedOption<bool> optionExecCmdBuffsBeforeQueueSubmit(
      options, OPTION_GROUP_GENERAL, 0, "execCmdBuffsBeforeQueueSubmit",
      "Executing commandBuffers operation just before QueueSubmit, instead of doing that in the "
      "same way as app/benchmark/game.");

  TypedOption<std::string> optionCaptureVulkanRenderPasses(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanRenderPasses",
      "List of Vulkan render passes, whose attachments will be captured during playback. The "
      "format consists of a hierarchy of numbers:\n"
      "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_range "
      "which can be adapted. User may omit elements, starting from the rightmost (e.g. omit "
      "renderpass number and use the "
      "queue_submit_number/command_buffer_batch_number/command_buffer_number form or even omit "
      "everything except the queue submit number). Additionally "
      "the rightmost number may be replaced with an elemspec (creating a "
      "number/number/number/elemspec form or even a form consisting of just a single "
      "elemspec specifying queues).\n"

      "Example elemspecs:\n"
      " * \"1/2/3/4\" specifies queue submit number 1, command buffer batch number 2, "
      "commandbuffer number 3, render pass number 4,\n"
      " * \"3,5-10\" specifies queue submit number 3 and queue submits from 5 to 10 (it's actually "
      "a normal elemspec),\n"
      " * \"7/2/1-20,30,40-50:2\" specifies queue submit number 7, command buffer batch 2, command "
      "buffers from 1 to 20, command buffer 30 and even numbered command buffers between 40 and 50 "
      "(all ranges are inclusive).");

  TypedOption<std::string> optionCaptureVulkanRenderPassesResources(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanRenderPassesResources",
      "List of Vulkan render passes, whose storage buffers, storage images, destination buffers"
      " and destination textures be captured during playback. The format consists of a hierarchy "
      "of numbers:\n"
      "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_range "
      "which can be adapted. User may omit elements, starting from the rightmost (e.g. omit "
      "renderpass number and use the "
      "queue_submit_number/command_buffer_batch_number/command_buffer_number form or even omit "
      "everything except the queue submit number). Additionally "
      "the rightmost number may be replaced with an elemspec (creating a "
      "number/number/number/elemspec form or even a form consisting of just a single "
      "elemspec specifying queues).\n"

      "Example elemspecs:\n"
      " * \"1/2/3/4\" specifies queue submit number 1, command buffer batch number 2, "
      "commandbuffer number 3, render pass number 4,\n"
      " * \"3,5-10\" specifies queue submit number 3 and queue submits from 5 to 10 (it's actually "
      "a normal elemspec),\n"
      " * \"7/2/1-20,30,40-50:2\" specifies queue submit number 7, command buffer batch 2, command "
      "buffers from 1 to 20, command buffer 30 and even numbered command buffers between 40 and 50 "
      "(all ranges are inclusive).");

  TypedOption<std::string> optionCaptureVulkanDraws(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanDraws",
      "List of Vulkan draws, whose render pass attachments will be captured during playback. The "
      "format consists of a hierarchy of numbers:\n"
      "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_number/"
      "draws_range which can be adapted. User may omit elements, starting from the rightmost (e.g. "
      "omit draw number and use the "
      "queue_submit_number/command_buffer_batch_number/command_buffer_number/render_pass_number "
      "form or even omit everything except the queue submit number). Additionally "
      "the rightmost number may be replaced with an elemspec (creating a "
      "number/number/number/number/elemspec form or even a form consisting of just a single "
      "elemspec specifying queues).\n"

      "Example elemspecs:\n"
      " * \"1/2/3/4\" specifies queue submit number 1, command buffer batch number 2, "
      "commandbuffer number 3, render pass number 4,\n"
      " * \"3,5-10\" specifies queue submit number 3 and queue submits from 5 to 10 (it's actually "
      "a normal elemspec),\n"
      " * \"7/2/1-20,30,40-50:2\" specifies queue submit number 7, command buffer batch 2, command "
      "buffers from 1 to 20, command buffer 30 and even numbered command buffers between 40 and 50 "
      "(all ranges are inclusive).");

  TypedOption<std::string> optionCaptureVulkanResources(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanResources",
      "List of Vulkan draws, blits, dispatches, whose storage buffers, storage images, destination "
      "buffers  and destination textures be captured during playback.\n"
      "The format consists of a hierarchy of numbers:\n"
      "queue_submit_number/command_buffer_batch_number/command_buffer_number "
      "which can be adapted. User may omit elements, starting from the rightmost (e.g. omit "
      "command buffer number and use the "
      "queue_submit_number/command_buffer_batch_number form or even omit "
      "everything except the queue submit number). Additionally "
      "the rightmost number may be replaced with an elemspec (creating a "
      "number/number/number/elemspec form or even a form consisting of just a single "
      "elemspec specifying queues).\n"

      "Example elemspecs:\n"
      " * \"1/2/3/4\" specifies queue submit number 1, command buffer batch number 2, "
      "commandbuffer number 3, render pass number 4,\n"
      " * \"3,5-10\" specifies queue submit number 3 and queue submits from 5 to 10 (it's actually "
      "a normal elemspec),\n"
      " * \"7/2/1-20,30,40-50:2\" specifies queue submit number 7, command buffer batch 2, command "
      "buffers from 1 to 20, command buffer 30 and even numbered command buffers between 40 and 50 "
      "(all ranges are inclusive).");

  TypedOption<BitRange> optionTraceGLBufferHashes(
      options, OPTION_GROUP_GENERAL, 0, "traceGLBufferHashes",
      "Together with --loglevel TRACEVERBOSE causes additional info to be printed "
      "in the form of buffer objects hashes used for specified draw-call execution. "
      "inspecting current VAO state - enabled attributes and element array buffer binding");

  TypedOption<std::filesystem::path> optionOutputTracePath(
      options, OPTION_GROUP_GENERAL, 0, "outputTracePath",
      "Specifies file path where gits traces will be stored.");

  TypedOption<bool> optionForceOrigScreenResolution(
      options, OPTION_GROUP_PLAYBACK, 0, "forceOrigScreenResolution",
      "Forces player to set original screen resolution used "
      "during recording. Useful to replay full screen mode.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionShowWindowBorder(
      options, OPTION_GROUP_PLAYBACK, 0, "showWindowBorder",
      "Forces player to show window border (ignored in fullscreen)",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionFullscreen(
      options, OPTION_GROUP_PLAYBACK, 0, "fullscreen",
      "Attempts to display the window in the exclusive fullscreen mode "
      "for maximum performance. Window covers everything, including OS bars, docks, etc. "
      "Display resolution gets changed if necessary to match the stream dimensions.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<BitRange> optionStopAfterFrames(options, OPTION_GROUP_PLAYBACK, 0, "stopAfterFrames",
                                              "List of frames to stop after "
                                              "during playback. Specified as single string "
                                              "without spaces of following format: \n"
                                              "  * pattern = frame_list | frame_list/Repeat-Count"
                                              "  * frame_list = element | frame_list, element\n"
                                              "  * element = Frame | Begin-End | Begin-End:Step");

  TypedOption<bool> optionInteractive(options, OPTION_GROUP_PLAYBACK, 'i', "",
                                      "Defines that interactive mode should be enabled.",
                                      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionSwapAfterPrepare(options, OPTION_GROUP_PLAYBACK, 0, "swapAfterPrepare",
                                           "If specified, gitsPlayer will issue additional "
                                           "swap after state restoration calls.");

  TypedOption<int> optionEndFrameSleep(options, OPTION_GROUP_PLAYBACK, 0, "endFrameSleep",
                                       "Sleep this many milliseconds after a swap.");

  TypedOption<bool> optionClCaptureReads(
      options, OPTION_GROUP_PLAYBACK, 0, "clCaptureReads",
      "Enables dumping of CL memory objects to directory specified in --outputDir"
      "(implicitly to \"dump\" in stream directory). Option may be used with "
      "--clOmitReadOnlyObjects");

  TypedOption<std::string> optionL0CaptureKernels(
      options, OPTION_GROUP_PLAYBACK, 0, "l0CaptureKernels",
      "special option for capturing every single kernel argument"
      "injects synchronization points and reads for adequate queue mode"
      "reads are injected into gits reserved memory space, gits allocated memory last until sync"
      "the symantics: for kernel dumping: "
      "QueueSubmitNumberRange/CommandListNumberRange/AppendKernelsRange"
      "for command list dumping: QueueSubmitNumberRange/CommandListNumberRange"
      "Numeration of kernels is the order of appending the kernel launch call to a command list"
      "Numeration of command lists is the order in which they were created"
      "Numeration of command queue submit is the queue submit number"
      "Numeration calls must be specified as a single string without spaces"
      "As spec says command queue's command lists execution is happening in a fifo manner"
      "Numeration of Immediate command lists on creation increments number of queue submit and "
      "command list"
      "Examples:"
      " * \"1/2/3-4\" specifies queue submit number 1, command list number 2, kernels 3 and 4"
      " * \"3,5-10/10,5,12-100\" specifies queue submit number 3 and from 5 to 10, dumps command "
      "lists number 5, kernel: 10 and from 12 to 100"
      " * \"+/+/+\" dumps everything , '+' sign indicates choosing all of the possible range"
      " * \"2/5\" dumps 2nd queue submit and every kernel from 5th command list"
      "to check numbering, please look at recorders log and make sure log_level is set to at least "
      "TRACE"
      "option may be used with --l0CaptureAfterSubmit to reduce memory used by GITS, in a cost of "
      "each kernel buffer verificaiton"
      "Buffers will be stored in directory specified in --outputDir"
      "(implicitly to \"dump\" in stream directory)");

  TypedOption<bool> optionL0DumpSpv(options, OPTION_GROUP_PLAYBACK, 0, "l0DumpSpv",
                                    "Option deprecated!");

  TypedOption<bool> optionL0OmitOriginalAddressCheck(
      options, OPTION_GROUP_PLAYBACK, 0, "l0OmitOriginalAddressCheck",
      "Do not throw when expected address is not equal to originally requested");

  TypedOption<bool> optionL0CaptureInputKernels(
      options, OPTION_GROUP_PLAYBACK, 0, "l0CaptureInputKernels",
      "Debug option to dump input kernels. Must be used with l0CaptureKernels");

  TypedOption<bool> optionL0CaptureAfterSubmit(
      options, OPTION_GROUP_PLAYBACK, 0, "l0CaptureAfterSubmit",
      "used with l0CaptureKernels, modifies the way kernel arguments are dumped."
      "Injects immediate command lists, reads and synchronization points after queue submit,"
      "instead of injecting read after every appendKernel call. This way dumping process is after "
      "command queue execution containing N command lists."
      "If kernel argument is overwritten by many kernels, the output buffer will have the value "
      "after last executed kernel that used it."
      "All of other same buffers are skipped. Option reduces allocated memory by GITS");

  TypedOption<BitRange> optionClCaptureKernels(
      options, OPTION_GROUP_PLAYBACK, 0, "clCaptureKernels",
      "Injects clEnqueueRead*() after specified occurence of NDRange call for each "
      "used buffer to allow contents verification. "
      "Number of NDRange calls must be specified as a single string without spaces of following "
      "format:\n"
      " * num1-num2 ex. 1-7 - will dump buffers used in the range of calls\n"
      " * num1,num2,num3 ex. 2,5,7 - will dump buffers used in specified occurence of NRange "
      "calls\n"
      "Approaches may be combined ex. 2-5,14,17 "
      "Buffers will be stored in directory specified in --outputDir "
      "(implicitly to \"dump\" in stream directory). Option may be used with "
      "--clOmitReadOnlyObjects");

  TypedOption<bool> optionClInjectBufferResetAfterCreate(
      options, OPTION_GROUP_PLAYBACK, 0, "clInjectBufferResetAfterCreate",
      "Nullifies Buffer, Image, USM and SVM memory regions  "
      "immediately after their creation to produce deterministic results when verifying buffers.");

  TypedOption<bool> optionL0InjectBufferResetAfterCreate(
      options, OPTION_GROUP_PLAYBACK, 0, "l0InjectBufferResetAfterCreate",
      "Nullifies USM Buffer and Image memory regions immediately"
      " after their creation to produce deterministic results when verifying buffers.");

  TypedOption<bool> optionL0DisableNullIndirectPointersInBuffer(
      options, OPTION_GROUP_PLAYBACK, 0, "l0DisableNullIndirectPointersInBuffer",
      "Disable nullification of indirect pointers inside"
      " buffers. Call this option when you want to disable it."
      " GITS l0_gits_indirect_access_pointers_locations extension required.");

  TypedOption<bool> optionClOmitReadOnlyObjects(
      options, OPTION_GROUP_PLAYBACK, 0, "clOmitReadOnlyObjects",
      "Omits dumping for objects created with CL_MEM_READ_ONLY. Takes effect "
      "only when --clCaptureReads or --clCaptureKernels is used.");

  TypedOption<bool> optionClDumpLayoutOnly(
      options, OPTION_GROUP_PLAYBACK, 0, "clDumpLayoutOnly",
      "Omits dumping injected buffers and images. Only layout.json file "
      "with structure of expected dumps. Takes effect only when --clCaptureKernels is used.");

  TypedOption<bool> optionL0DumpLayoutOnly(
      options, OPTION_GROUP_PLAYBACK, 0, "l0DumpLayoutOnly",
      "Omits dumping injected buffers and images. Only dumps layout.json file "
      "with structure of expected dumps. Takes effect only when --l0CaptureKernels is used.");

  TypedOption<unsigned> optionExitFrame(options, OPTION_GROUP_PLAYBACK, 0, "exitFrame",
                                        "Stop playback after this frame.");

  TypedOption<bool> optionExitOnError(options, OPTION_GROUP_PLAYBACK, 0, "exitOnError",
                                      "Stop playback when API function call returns error value.");

  TypedOption<bool> optionExitOnVkQueueSubmitFail(
      options, OPTION_GROUP_PLAYBACK, 0, "exitOnVkQueueSubmitFail",
      "Stop playback when vkQueueSubmit returns error value.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<std::string> optionForceWindowPos(
      options, OPTION_GROUP_PLAYBACK, 0, "forceWindowPos",
      "Specified coordinates (e.g. 100x100) will be used as position of "
      "all windows created during stream playback. Window position updates will be inhibited.");

  TypedOption<BitRange> optionKeepDraws(options, OPTION_GROUP_PLAYBACK, 0, "keepDraws",
                                        "List of drawcall numbers to be executed by "
                                        "gits during playback. Rest of drawcalls won't be executed."
                                        "Specified as single string without spaces of "
                                        "following format: \n"
                                        "  * pattern = draw_list | draw_list/Repeat-Count"
                                        "  * draw_list = element | draw_list, element\n"
                                        "  * element = Number | Begin-End | Begin-End:Step");

  TypedOption<BitRange> optionKeepFrames(
      options, OPTION_GROUP_PLAYBACK, 0, "keepFrames",
      "List of frames whose draw calls should be played back. "
      "All drawcalls outside specified range will be skipped. Uses the same format "
      "as other frame specifications. Defaults to 'all frames'.");

  TypedOption<GLProfileOpt> optionForceGLProfile(
      options, OPTION_GROUP_PLAYBACK, 0, "forceGLProfile",
      "Forces OpenGL Profile:\n"
      " - COMPAT\n"
      " - CORE\n"
      " - ES\n"
      "This option may cause a crash if API used in stream or shaders does not match selected "
      "profile.");

  TypedOption<GLNativeApiOpt> optionForceGLNativeAPI(
      options, OPTION_GROUP_PLAYBACK, 0, "forceGLNativeAPI",
      "Forces OpenGL native API:"
      " - EGL\n"
      " - GLX\n"
      " - WGL\n"
      "Option useful on OS/driver configurations where multiple native APIs are available. "
      "For instance some intel drivers support EGL and WGL on Windows. "
      "In this case it allows to run for example stream from application using WGL through EGL.");

  TypedOption<unsigned> optionForceGLVersionMajor(
      options, OPTION_GROUP_PLAYBACK, 0, "forceGLVersionMajor",
      "Forces OpenGL version major number.", GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<unsigned> optionForceGLVersionMinor(
      options, OPTION_GROUP_PLAYBACK, 0, "forceGLVersionMinor",
      "Forces OpenGL version minor number.", GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionForceWaylandWindow(
      options, OPTION_GROUP_PLAYBACK, 0, "forceWaylandWindow",
      "Forces using Wayland windows instead of X11.", GITS_PLATFORM_BIT_X11);

  TypedOption<std::filesystem::path> optionEventScript(
      options, OPTION_GROUP_PLAYBACK, 0, "eventScript",
      "Path to lua event script. "
      "Following functions can be provided in script file for event handlers:\n"
      " - all GL / WGL / EGL / GLX functions\n"
      " - gitsFrameBegin(nr)\n"
      " - gitsFrameEnd(nr)\n"
      " - gitsLoopBegin(nr)\n"
      " - gitsLoopEnd(nr)\n"
      " - gitsStateRestoreBegin()\n"
      " - gitsStateRestoreEnd()\n"
      " - gitsProgramStart()\n"
      " - gitsProgramExit()\n"
      " - gitsLogging(log)\n"
      "Following functions exposed by GITS may be used inside lua scripts:\n"
      " - string udtToStr(userdata udt) - casts lua userdata to string.\n"
      " - number udtToInt(userdata udt) - casts user data to int.\n"
      " - userdata getUdt(userdata carray, number index) - extracts userdata from carray's index.\n"
      " - number getInt(userdata carray, number index) - extracts int from carray's index.\n"
      " - number getFloat(userdata carray, number index) - extracts float from carray's index.\n"
      " - void setUdt(userdata carray, number index, number value) - sets userdata at array's "
      "index.\n"
      " - void setInt(userdata carray, number index, number value) - sets int at array's index.\n"
      " - void setFloat(userdata carray, number index, number value) - sets float at array's "
      "index.\n"
      " - userdata nullUdt() - returns null user data \"(void*)0\".\n"
      " - number currTime() - returns time in microseconds from the start of the player.\n"
      " - void msgBox(string title, string message, number type) - Windows exclusive "
      "functions.Displays a message box with arbitral title, message and typ.\n"
      " - userdata allocBytes(number size) - Allocates memory and returns userdata(pointer to "
      "allocated data in C).\n"
      " - void freeBytes(userdata pointer) - Deallocates memory.\n"
      " - string getArgsStr() - returns string with args passed through gitsPlayer option "
      "--scriptArgsStr.\n"
      " - string getOutDir() - returns output directory for artifacts defined with gitsPlayer "
      "option --outputDir.\n"
      " - gitsGlDebugProc() - returns userdata containing pointer to callback function which is "
      "being set by glDebugMessageCallback.");

  TypedOption<std::string> optionScriptArgsString(
      options, OPTION_GROUP_PLAYBACK, 0, "scriptArgsStr",
      "Lua script arguments. String with lua script arguments may be read by lua script.");

  TypedOption<bool> optionFaithfulThreading(
      options, OPTION_GROUP_PLAYBACK, 0, "faithfulThreading",
      "GITS, by default, plays the stream in single thread, synthesizing "
      "necessary GL context switches. This option causes GITS to use as many threads as original "
      "application for playback. This also makes it impossible to create a subcapture from the "
      "stream");

  TypedOption<bool> optionRenderOffscreen(
      options, OPTION_GROUP_PLAYBACK, 0, "renderOffscreen",
      "Forces offscreen rendering:\n"
      "  - in EGL to a pbuffer\n"
      "  - in Vulkan to a swapchain, but without presentation.\n"
      "Not supported for other API types. Use --captureFrames or similar option to get rendering "
      "output.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionCheckCrossPlatformCompatibility(
      options, OPTION_GROUP_PLAYBACK, 0, "checkCrossPlatformCompatibility", "Option deprecated!");

  TypedOption<bool> optionOneVulkanDrawPerCommandBuffer(
      options, OPTION_GROUP_PLAYBACK, 0, "oneVulkanDrawPerCommandBuffer",
      "Force to split command buffers. Each draw will "
      "be executed in separate command buffer. It affects performance.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionOneVulkanRenderPassPerCommandBuffer(
      options, OPTION_GROUP_PLAYBACK, 0, "oneVulkanRenderPassPerCommandBuffer",
      "Force to split command buffers. Each render pass will "
      "be executed in separate command buffer. It affects performance.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<unsigned> optionUseVKPhysicalDeviceIndex(
      options, OPTION_GROUP_PLAYBACK, 0, "useVKPhysicalDeviceIndex",
      "Forces selected physical device index to be used for logical device creation.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<std::string> optionUseVKPhysicalDeviceName(
      options, OPTION_GROUP_PLAYBACK, 0, "useVKPhysicalDeviceName",
      "Forces physical device with a given name to be used for logical device creation.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<DeviceTypeOpt> optionUseVKPhysicalDeviceType(
      options, OPTION_GROUP_PLAYBACK, 0, "useVKPhysicalDeviceType",
      "Forces integrated or discrete physical device to be used for logical device creation.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionCaptureWholeWindow(options, OPTION_GROUP_IMAGE, 0, "captureWholeWindow",
                                             "Causes '--captureFrames' to capture whole "
                                             "current window instead of current viewport.",
                                             GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionDontForceBackBufferGL(
      options, OPTION_GROUP_IMAGE, 0, "dontForceBackBufferGL",
      "When OpenGL API is used this option modifies --captureFrames, "
      "--captureReadPixels, --captureFinishFrame, --captureFlushFrame, --captureBindFboFrame"
      "options by letting player dump currently bound framebuffer "
      "(by default these options force player to dump back buffer contents).");

  TypedOption<BitRange> optionCaptureFrames(
      options, OPTION_GROUP_IMAGE, 0, "captureFrames",
      "List of frames that will be captured by "
      "gits during playback. Specified as single string without spaces of "
      "following format:\n"
      "  * pattern = frame_list | frame_list/Repeat-Count\n"
      "  * frame_list = element | frame_list, ',', element\n"
      "  * element = Frame | Begin-End | Begin-End:Step");

  TypedOption<BitRange> optionTraceSelectedFrames(
      options, OPTION_GROUP_GENERAL, 0, "traceSelectedFrames",
      "List of frames for which trace (log) will be generated "
      "during playback. Specified as a single string without spaces with the following format:\n"
      "  * pattern = frame_list | frame_list/Repeat-Count\n"
      "  * frame_list = element | frame_list, ',', element\n"
      "  * element = Frame | Begin-End | Begin-End:Step");

  TypedOption<bool> optionCaptureFramesHashes(
      options, OPTION_GROUP_IMAGE, 0, "captureFramesHashes",
      "Modifies captureFrames option to just print image hash instead of frame dumping.");

  TypedOption<bool> optionCaptureScreenshot(options, OPTION_GROUP_IMAGE, 0, "captureScreenshot",
                                            "Modifies captureFrames option to capture a WinAPI "
                                            "screenshot instead of buffer read. Forces vsync.",
                                            GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<BitRange> optionCaptureVulkanSubmits(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanSubmits",
      "List of Vulkan queue submits, whose render passes "
      "will be captured by gits during playback. Render targets from all render passes will be "
      "dumped "
      "per command buffer after submission, so each command buffer is executed in a separate queue "
      "submit."
      "Specified as single string without spaces of following format:\n"
      "  * pattern = cmdBuff_list | cmdBuff_list/Repeat-Count\n"
      "  * cmdBuff_list = element | cmdBuff_list, ',', element\n"
      "  * element = Frame | Begin-End | Begin-End:Step");

  TypedOption<VKCaptureGroupTypeOpt> optionCaptureVulkanSubmitsGroupType(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanSubmitsGroupType",
      "Set Group Type for captureVulkanSubmits:\n"
      " * CmdBuffer - default; creating list of textures from renderpasses and dumping (each "
      "texture are dumped only once)\n"
      " * RenderPass - dumping textures per each renderpass (if renderpasses use the same images "
      "for some render targets"
      " then each image may be dumped several times)");

  TypedOption<BitRange> optionCaptureVulkanSubmitsResources(
      options, OPTION_GROUP_IMAGE, 0, "captureVulkanSubmitsResources",
      "List of Vulkan queue submits, whose storage buffers, "
      "storage images, destination buffers and destination textures will be captured by gits "
      "during playback. "
      "Resources will be dumped per command buffer after submission, so each command buffer is "
      "executed "
      "in a separate queue submit."
      "Specified as single string without spaces of following format:\n"
      "  * pattern = cmdBuff_list | cmdBuff_list/Repeat-Count\n"
      "  * cmdBuff_list = element | cmdBuff_list, ',', element\n"
      "  * element = Frame | Begin-End | Begin-End:Step");

  TypedOption<bool> optionSkipNonDeterministicImages(
      options, OPTION_GROUP_IMAGE, 0, "skipNonDeterministicImages",
      "Skip images with potential corruptions. Use only with --captureVulkanSubmits",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<BitRange> optionCaptureDraws(
      options, OPTION_GROUP_IMAGE, 0, "captureDraws",
      "List of drawcall numbers that will be captured by "
      "gits during playback. Specified as single string without spaces of following format:\n"
      "  * pattern = draw_list | draw_list/Repeat-Count\n"
      "  * draw_list = element | draw_list, ',', element\n"
      "  * element = Number | Begin-End | Begin-End:Step");

  TypedOption<bool> optionCaptureDepth(
      options, OPTION_GROUP_IMAGE, 0, "captureDepth",
      "Causes 'captureDraws' to capture "
      "not only color buffer, but also depth buffer. "
      "Depth values are mapped to colors of visible light spectrum, "
      "ranging from red (near plane) to magenta (far plane).");

  TypedOption<bool> optionCaptureStencil(options, OPTION_GROUP_IMAGE, 0, "captureStencil",
                                         "Causes 'captureDraws' to capture "
                                         "not only color buffer, but also stencil buffer.");

  TypedOption<bool> optionCaptureDrawsPre(
      options, OPTION_GROUP_IMAGE, 0, "captureDrawsPre",
      "Causes 'captureDraws' to capture "
      "drawbuffer content not only after specified drawcall, but also before it.");

  TypedOption<BitRange> optionCaptureFinishFrame(options, OPTION_GROUP_IMAGE, 0,
                                                 "captureFinishFrame",
                                                 "Capture framebuffer content on glFinish "
                                                 "(the same numbering as captureFrames option).");

  TypedOption<BitRange> optionCaptureReadPixels(options, OPTION_GROUP_IMAGE, 0, "captureReadPixels",
                                                "Capture framebuffer content on glReadPixels "
                                                "(the same numbering as captureFrames option).");

  TypedOption<BitRange> optionCaptureFlushFrame(options, OPTION_GROUP_IMAGE, 0, "captureFlushFrame",
                                                "Capture framebuffer content on glFlush "
                                                "(the same numbering as captureFrames option).");

  TypedOption<BitRange> optionCaptureBindFboFrame(
      options, OPTION_GROUP_IMAGE, 0, "captureBindFboFrame",
      "Capture framebuffer content on glBindFramebuffer "
      "(the same numbering as captureFrames option).");

  TypedOption<BitRange> optionCapture2DTexs(
      options, OPTION_GROUP_IMAGE, 0, "capture2DTexs",
      "List of glTexImage2D API calls, ordered by execution time,"
      "specified as single string without spaces of  following format: \n"
      "  * texs_list = element | texs_list, element\n"
      "  * element = Number | Begin-End | Begin-End:Step"
      "Captured textures name format is: tex2D_<texture object>_<texture load number>.png");

  TypedOption<BitRange> optionCaptureDraws2DTexs(
      options, OPTION_GROUP_IMAGE, 0, "captureDraws2DTexs",
      "List of drawcalls specified as single string without spaces of "
      "following format: \n"
      "  * draw_list = element | draw_list, ',', element\n"
      "  * element = Number | Begin-End | Begin-End:Step"
      "Captured textures name format is: draw<drawcall number>_unit<unitnumber>_tex<texture load "
      "number>.png");

  TypedOption<bool> optionClCaptureImages(
      options, OPTION_GROUP_IMAGE, 0, "clCaptureImages",
      "Capture OpenCL images after each clEnqueueReadImage call."
      "Assume they all are 2D RGBA8 images.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionClRemoveSourceLengths(
      options, OPTION_GROUP_IMAGE, 0, "clRemoveSourceLengths",
      "Removes source lengths. Makes driver assume strings are null-terminated"
      "in clCreateProgramWithSource",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionL0CaptureImages(options, OPTION_GROUP_IMAGE, 0, "l0CaptureImages",
                                          "Capture L0 images."
                                          "Assume they all are 2D RGBA8 images."
                                          "option have to be used with --l0CaptureKernels <range>",
                                          GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);
  TypedOption<uint32_t> optionL0DisableAddressTranslation(
      options, OPTION_GROUP_PLAYBACK, 0, "l0DisableAddressTranslation",
      "1 - only host addresses"
      "2 - only device addresses"
      "4 - only shared addresses"
      "option value is treated as bitfield and might be used in combination.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionStats(options, OPTION_GROUP_METRICS, 's', "stats",
                                "GITS will not play specified file but "
                                "instead will create some statistics about it.");

  TypedOption<bool> optionBenchmark(
      options, OPTION_GROUP_METRICS, 'b', "benchmark",
      "Enables performance measurement in OpenGL streams on frame base."
      "Results are being stored in benchmark.csv file in CWD or");

  TypedOption<bool> optionEscalatePriority(options, OPTION_GROUP_PERFORMANCE, 0, "realtime",
                                           "If specified, gitsPlayer will request "
                                           "realtime priority from the system.");

  TypedOption<int> optionTokenBurstLimit(
      options, OPTION_GROUP_PERFORMANCE, 0, "tokenBurstLimit",
      "Specifies number of tokens to be loaded "
      "from stream file in one burst. Burst is a group of tokens that have to be all loaded before "
      "execution of first token in a burst will start. This option is mutually exclusive with "
      "loadWholeStreamBeforePlayback option.");

  TypedOption<int> optionTokenBurstNum(
      options, OPTION_GROUP_PERFORMANCE, 0, "tokenBurstNum",
      "Specifies maximum number of 'bursts' loaded, waiting for execution, "
      "at any one time by player");

  TypedOption<bool> optionLoadWholeStreamBeforePlayback(
      options, OPTION_GROUP_PERFORMANCE, 0, "loadWholeStreamBeforePlayback",
      "Forces load of entire stream to memory before playback. May cause memory issues and errors "
      "related to max value being exceeded. This option is mutually exclusive with tokenBurstLimit "
      "option.");

  TypedOption<std::string> optionAffectViewport(
      options, OPTION_GROUP_MUTATORS, 0, "affectViewport",
      "Causes certain other options to affect only a viewports specified with 'width,height' "
      "dimmensions."
      "This option has influence on following options: forceScissor, forceWindowSize.");

  TypedOption<std::string> optionForceDesktopResolution(
      options, OPTION_GROUP_MUTATORS, 0, "forceDesktopResolution",
      "Forces specified desktop resolution before playing stream. Returns to previous resolution "
      "after execution."
      "Accepts 'width,height' argument. E.g. --forceDesktopResolution 1024,768",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionSkipQueries(
      options, OPTION_GROUP_MUTATORS, 0, "skipQueries",
      "Do not call any Query related functions (glBeginQuery, etc).\n"
      "This may corrupt the rendering.");

  TypedOption<float> optionScaleFactor(
      options, OPTION_GROUP_MUTATORS, 0, "scaleFactor",
      "Rendering will be scaled by specified factor.\n"
      "WARNING: This may produce corrupted rendering  in some cases.");

  TypedOption<bool> optionVsyncDisable(
      options, OPTION_GROUP_MUTATORS, 0, "vsyncDisable",
      "Disable vsync on first make-current in the stream. Option Implemented via lua script"
      "Run stream with option --eventScript <GITS_DIR>/Scripts/vSyncDisable.lua to enable option");

  TypedOption<std::string> optionForceWindowSize(
      options, OPTION_GROUP_MUTATORS, 0, "forceWindowSize",
      "Forces windows, viewport and scissor box sizes to specified "
      " values.");

  TypedOption<bool> optionForceInvisibleWindows(
      options, OPTION_GROUP_MUTATORS, 0, "forceInvisibleWindows", "Forces windows to be hidden.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<std::string> optionForceScissor(options, OPTION_GROUP_MUTATORS, 0, "forceScissor",
                                              "Forces scissor x,y,width,height");

  TypedOption<bool> optionLinkGetProgBinary(
      options, OPTION_GROUP_MUTATORS, 0, "linkGetProgBinary",
      "After each link program query resulting program binary "
      "and store it in files for future use.");

  TypedOption<bool> optionLinkUseProgBinary(options, OPTION_GROUP_MUTATORS, 0, "linkUseProgBinary",
                                            "Instead of linking a program source them from "
                                            "binary files.");

  TypedOption<bool> optionUseDebugContext(
      options, OPTION_GROUP_MUTATORS, 0, "useDebugContext",
      "Force creation of debug context and install "
      "ARB_debug_output handlers. Option Implemented via lua script"
      "Run stream with option --eventScript <GITS_DIR>/Scripts/useDebugContext.lua to enable "
      "option",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionForceWireFrame(
      options, OPTION_GROUP_MUTATORS, 0, "forceWireFrame",
      "Forces WireFrame view by injecting glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); call. Option "
      "Implemented via lua script"
      "Run stream with option --eventScript <GITS_DIR>/Scripts/forceWireFrame.lua to enable option",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionForceNoMSAA(
      options, OPTION_GROUP_MUTATORS, 0, "forceNoMSAA",
      "Force pixelformat to not use multisampling. Renderbuffers are forced to 0 sampels. "
      "Multisampled textures are not supported by this option. If multisampled textures are used, "
      "GITS will abort the stream.");

  TypedOption<bool> optionCleanResourcesOnExit(
      options, OPTION_GROUP_MUTATORS, 0, "cleanResourcesOnExit",
      "Deletes unreleased resources at the end of the stream (with additional API functions "
      "calls).\n"
      "- OpenGL - the first 2000 names of the following resources are destroyed: FBOs, RBOs, VAOs, "
      "Textures, Buffers, Shaders, Programs, ARBPrograms, DisplayLists, Queries.\n"
      "- Vulkan - all created resources are destroyed.");

  TypedOption<bool> optionDestroyContextsOnExit(
      options, OPTION_GROUP_MUTATORS, 0, "destroyContextsOnExit",
      "Playing additional API calls in the end of the stream to delete all existing contexts.");

  TypedOption<std::filesystem::path> optionOverrideVKPipelineCache(
      options, OPTION_GROUP_MUTATORS, 0, "overrideVKPipelineCache",
      "Overrides all pipeline cache uses with a single, global pipeline cache object. Initial and "
      "final data is loaded from and stored in a specified file.");

  TypedOption<bool> optionMinimalConfig(
      options, OPTION_GROUP_WORKAROUND, 0, "minimalConfig",
      "When selecting a pixel format in EGL or WGL, this option causes most pixel format "
      "attributes to be removed; only the essential ones are preserved. This allows player to use "
      "pixel formats that are slightly different from the ones used in recorder, but still "
      "compatible with them. This option is useful when a stream recorded on one platform is being "
      "replayed on another platform. Without it, some unimportant attribute mismatch could cause "
      "the driver to return no usable pixel formats.");

  TypedOption<bool> optionShowWindowsWA(options, OPTION_GROUP_WORKAROUND, 0, "showWindowsWA",
                                        "Enables workaround that makes all windows visible "
                                        "all the time by default");

  TypedOption<bool> optionWaitAfterQueueSubmitWA(
      options, OPTION_GROUP_WORKAROUND, 0, "waitAfterQueueSubmitWA",
      "Vulkan specific option. This WA calls vkQueueWaitIdle after vkQueueSubmit.");

  TypedOption<bool> optionStripProfileRequest(options, OPTION_GROUP_WORKAROUND, 0,
                                              "stripProfileRequest",
                                              "Remove profile requests from context creation.");

  TypedOption<std::string> optionSuppressVKDeviceFeatures(
      options, OPTION_GROUP_WORKAROUND, 0, "suppressVKDeviceFeatures",
      "Disable physical device features during logical device creation. "
      "Option accepts list of comma (,) separated names of physical device features (as specified "
      "in the VkPhysicalDeviceFeatures structure definition).",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<std::string> optionSuppressVKExtensions(
      options, OPTION_GROUP_WORKAROUND, 0, "suppressVKExtensions",
      "Disable extensions during instance and/or logical device creation. Option accepts list of "
      "comma (,) separated names of extensions.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<std::string> optionSuppressVKLayers(
      options, OPTION_GROUP_WORKAROUND, 0, "suppressVKLayers",
      "Disable layers during instance and/or logical device creation. Option accepts list of comma "
      "(,) separated names of layers.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionIgnoreVKCrossPlatformIncompatibilitiesWA(
      options, OPTION_GROUP_WORKAROUND, 0, "ignoreVKCrossPlatformIncompatibilitiesWA",
      "Forces stream to continue playback even if platform's properties are not compatible with "
      "recorded parameters.");

  TypedOption<bool> optionForceMultithreadedPipelineCompilation(
      options, OPTION_GROUP_WORKAROUND, 0, "forceMultithreadedPipelineCompilation",
      "Forces pipeline objects to be created concurrently on multiple threads.");

  TypedOption<uint32_t> optionMaxAllowedVkSwapchainRewinds(
      options, OPTION_GROUP_WORKAROUND, 0, "maxAllowedVkSwapchainRewinds",
      "Vulkan specific option. Changes the number of maximum allowed Vulkan swapchain rewinds "
      "(defaults to 100).");

  TypedOption<bool> optionLogFncs(options, OPTION_GROUP_INTERNAL, 0, "logFncs",
                                  "All played tokens are logged to standard output.");

  TypedOption<bool> optionLogLoadedTokens(options, OPTION_GROUP_INTERNAL, 0, "logLoadedTokens",
                                          "Prints token names after they are loaded.");

  TypedOption<bool> optionAubSignaturesCL(
      options, OPTION_GROUP_INTERNAL, 0, "aubSignaturesCL",
      "Append signatures at the end of each mem object. "
      "Currently has to be used with --clInjectBufferResetAfterCreate.");

  TypedOption<bool> optionNullRun(options, OPTION_GROUP_INTERNAL, 0, "nullRun",
                                  "No functions are played back. This option is useful only for "
                                  "characterization of GITS player framework performance.");

  TypedOption<bool> optionWaitForEnter(
      options, OPTION_GROUP_INTERNAL, 0, "waitForEnter",
      "Before and after playing back the stream, wait for ENTER keypress.");

  TypedOption<std::filesystem::path> optionLibOCLPath(
      options, OPTION_GROUP_INTERNAL, 0, "libOCLPath",
      "Use a custom path to the OpenCL shared library.",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionNoOpenCL(options, OPTION_GROUP_INTERNAL, 0, "noOpenCL",
                                   "Do not initialize OpenCL subsystem. This will cause GITS to "
                                   "crash in case OpenCL is actually used by the stream.",
                                   GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<std::filesystem::path> optionLibEGL(options, OPTION_GROUP_INTERNAL, 0, "libEGLPath",
                                                  "Override for library containing EGL functions",
                                                  GITS_PLATFORM_BIT_WINDOWS |
                                                      GITS_PLATFORM_BIT_X11);

  TypedOption<std::filesystem::path> optionLibGLESv1(
      options, OPTION_GROUP_INTERNAL, 0, "libGLESv1Path",
      "Override for library path containing GLES 1 functions",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<std::filesystem::path> optionLibGLESv2(
      options, OPTION_GROUP_INTERNAL, 0, "libGLESv2Path",
      "Override for library path containing GLES 2 functions",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<std::filesystem::path> optionLibGL(
      options, OPTION_GROUP_INTERNAL, 0, "libGLPath",
      "Override for library path containing GL functions",
      GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<std::filesystem::path> optionLibVK(options, OPTION_GROUP_INTERNAL, 0, "libVKPath",
                                                 "Override for Vulkan Loader library path.",
                                                 GITS_PLATFORM_BIT_WINDOWS | GITS_PLATFORM_BIT_X11);

  TypedOption<bool> optionSignStream(
      options, OPTION_GROUP_INTERNAL, 0, "signStream",
      "Create a signature file for stream and exit after this."
      " (Note that all files in the stream folder and its subfolders will be"
      " treated as stream files unless they are on GITS' internal blacklist."
      " If necessary, you can edit the signature file by hand to remove stray"
      " non-stream files from it.)");

  TypedOption<bool> optionVerifyStream(options, OPTION_GROUP_INTERNAL, 0, "verifyStream",
                                       "Verify stream integrity. Exit after this.");

  TypedOption<bool> optionDontVerifyStream(options, OPTION_GROUP_INTERNAL, 0, "dontVerifyStream",
                                           "Just play the stream without verifying its integrity.");

#if defined(GITS_PLATFORM_WINDOWS)
  TypedOption<BitRange> optionRenderDocCaptureFrames(
      options, OPTION_GROUP_PLAYBACK, 0, "renderDocCaptureFrames",
      "Range of frames that will be captured by "
      "RenderDoc during playback. Specified as string of following format:\n"
      "  * --renderDocCaptureFrames frameNumber\n"
      "  * --renderDocCaptureFrames beginFrame-endFrame\n"
      "Captures will be saved in the stream folder. "
      "Option cannot be combined with option 'renderDocCaptureVKSubmits'",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<BitRange> optionRenderDocCaptureVKSubmits(
      options, OPTION_GROUP_PLAYBACK, 0, "renderDocCaptureVKSubmits",
      "Range of queue submits that will be captured by "
      "RenderDoc during playback. Specified as string of following format:\n"
      "  * --renderDocCaptureVKSubmits queueSubmit\n"
      "  * --renderDocCaptureVKSubmits beginQueueSubmit-endQueueSubmit\n"
      "Captures will be saved in the stream folder. "
      "Option cannot be combined with option 'renderDocCaptureFrames'",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<std::filesystem::path> optionRenderDocDllPath(
      options, OPTION_GROUP_PLAYBACK, 0, "renderDocDllPath",
      "By default, RenderDoc is found automatically, "
      "and this option is not necessary. However, if RenderDoc cannot be found, "
      "use this option to specify renderdoc.dll path.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionRenderDocContinuousCapture(
      options, OPTION_GROUP_PLAYBACK, 0, "renderDocContinuousCapture",
      "Option must be combined with "
      "'renderDocCaptureFrames' or 'renderDocCaptureVKSubmits'. Range of frames "
      "or queue submits specified by those options will be dumped into one file "
      "instead of separate files.",
      GITS_PLATFORM_BIT_WINDOWS);

  TypedOption<bool> optionRenderDocEnableUI(
      options, OPTION_GROUP_PLAYBACK, 0, "renderDocEnableUI",
      "Option must be combined with "
      "'renderDocCaptureFrames' or 'renderDocCaptureVKSubmits'. Opens RenderDoc "
      "with recorded captures. Captures are not saved by default but can be "
      "saved via RenderDoc UI.",
      GITS_PLATFORM_BIT_WINDOWS);
#endif

  // parse application command line
  options.Parse();

  process_lua_handled_option(optionUseDebugContext);
  process_lua_handled_option(optionForceWireFrame);
  process_lua_handled_option(optionVsyncDisable);
  process_lua_handled_option(optionPrintBuildLogsGL);
  process_lua_handled_option(optionStripProfileRequest);

  Config cfg = Config::Get();
  if (optionHelp.Value()) {
    cfg.common.player.helpGroup = "general";
  } else if (optionGroupHelp.Present()) {
    cfg.common.player.helpGroup = optionGroupHelp.Value();
  }
  set_when_option_present(cfg.common.player.version, optionVersion);
  set_when_option_present(cfg.common.player.interactive, optionInteractive);
  set_when_option_present(cfg.common.player.stats, optionStats);
  set_when_option_present(cfg.common.player.disableExceptionHandling, optionNoExceptionHandling);
  set_when_option_present(cfg.common.player.escalatePriority, optionEscalatePriority);
  set_when_option_present(cfg.common.player.swapAfterPrepare, optionSwapAfterPrepare);
  set_when_option_present(cfg.opengl.player.skipQueries, optionSkipQueries);
  set_when_option_present(cfg.common.player.waitForEnter, optionWaitForEnter);
  set_when_option_present(cfg.opengl.player.linkGetProgBinary, optionLinkGetProgBinary);
  set_when_option_present(cfg.opengl.player.linkUseProgBinary, optionLinkUseProgBinary);
  set_when_option_present(cfg.common.player.diags, optionRecorderDiags);
  set_when_option_present(cfg.opengl.player.forceNoMSAA, optionForceNoMSAA);
  set_when_option_present(cfg.common.player.cleanResourcesOnExit, optionCleanResourcesOnExit);
  set_when_option_present(cfg.opengl.player.destroyContextsOnExit, optionDestroyContextsOnExit);
  set_when_option_present(cfg.vulkan.player.printStateRestoreLogsVk, optionPrintStateRestoreLogsVk);
  set_when_option_present(cfg.vulkan.player.printMemUsageVk, optionPrintMemUsageVk);
  set_when_option_present(cfg.vulkan.player.execCmdBuffsBeforeQueueSubmit,
                          optionExecCmdBuffsBeforeQueueSubmit);
  set_when_option_present(cfg.vulkan.player.forceMultithreadedPipelineCompilation,
                          optionForceMultithreadedPipelineCompilation);

  if (optionSuppressVKDeviceFeatures.Present()) {
    parse_vector(optionSuppressVKDeviceFeatures.Value(),
                 cfg.vulkan.shared.suppressPhysicalDeviceFeatures);
  }
  if (optionSuppressVKExtensions.Present()) {
    parse_vector(optionSuppressVKExtensions.Value(), cfg.vulkan.shared.suppressExtensions);
  }
  if (optionSuppressVKLayers.Present()) {
    parse_vector(optionSuppressVKLayers.Value(), cfg.vulkan.shared.suppressLayers);
  }
  set_when_option_present(cfg.vulkan.player.waitAfterQueueSubmitWA, optionWaitAfterQueueSubmitWA);

  if (optionOverrideVKPipelineCache.Present()) {
    std::filesystem::path pipelineCachePath = optionOverrideVKPipelineCache.Value();
    cfg.vulkan.player.overrideVKPipelineCache = std::filesystem::absolute(pipelineCachePath);
  }

  set_when_option_present(cfg.opencl.player.noOpenCL, optionNoOpenCL);
  cfg.common.player.applicationPath = options.AppPath();

  set_when_option_present(cfg.opengl.player.traceGLBufferHashes, optionTraceGLBufferHashes);

  if (optionOutputDir.Present()) {
    std::filesystem::path outputPath = optionOutputDir.Value();
    cfg.common.player.outputDir = std::filesystem::absolute(outputPath);
  }

  if (optionOutputTracePath.Present()) {
    std::filesystem::path outputPath = optionOutputTracePath.Value();
    // If output path is just a file name (has no parent path), then create the
    // trace file in output dir path if present, rather than the working
    // directory
    if (!outputPath.has_parent_path() && optionOutputDir.Present()) {
      outputPath = optionOutputDir.Value() / outputPath;
    }
    cfg.common.player.outputTracePath = std::filesystem::absolute(outputPath);
  }

  if (optionScriptArgsString.Present()) {
    cfg.common.shared.scriptArgsStr = optionScriptArgsString.StrValue();
  }
  if (optionEventScript.Present()) {
    std::filesystem::path scriptPath = optionEventScript.Value();
    if (!std::filesystem::exists(scriptPath) || !std::filesystem::is_regular_file(scriptPath)) {
      throw std::runtime_error("could not find file: " + optionEventScript.Value().string());
    }

    lua::CreateAndRegisterEvents(scriptPath.string().c_str());
    cfg.common.shared.useEvents = true;
  }

  if (optionCaptureDepth.Present() || optionCaptureStencil.Present()) {
    throw std::runtime_error("Options 'captureDepth' and 'captureStencil are obsolete due to "
                             "intelligent dumping of draws by 'captureDraws' option");
  }

  set_when_option_present(cfg.opengl.player.captureDrawsPre, optionCaptureDrawsPre);
  set_when_option_present(cfg.common.player.benchmark, optionBenchmark);
  set_when_option_present(cfg.common.player.nullRun, optionNullRun);

  if (optionTrace.Present()) {
    cfg.common.shared.thresholdLogLevel = LogLevel::TRACE;
    Log(WARN) << "The '--trace' argument is deprecated. "
              << "Please use '--logLevel TRACE' instead.";
  }
  if (optionTraceGLVerbose.Present()) {
    cfg.common.shared.thresholdLogLevel = LogLevel::TRACEV;
    Log(WARN) << "The '--traceGLVerbose' argument is deprecated. "
              << "Please use '--logLevel TRACEVERBOSE' instead.";
  }
  if (optionLogLevel.Present()) {
    std::string logLevelString = optionLogLevel.StrValue();
    CEnumParser<LogLevel> logLevelParser;
    std::optional<LogLevel> lvl = logLevelParser.ParseEnum(logLevelString);
    if (lvl) {
      cfg.common.shared.thresholdLogLevel = lvl.value();
    } else {
      Log(WARN) << "Incorrect log level: \"" << logLevelString << "\".";
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  // Log can't use config directly, see log.cpp for info.
  CLog::SetLogLevel(cfg.common.shared.thresholdLogLevel);

  bool traceDataPresent = optionTraceData.Present();
  if (traceDataPresent && cfg.common.shared.thresholdLogLevel != LogLevel::TRACE) {
    throw std::runtime_error("Option 'traceData' requires log level to be exactly 'TRACE'.\n");
  }
  if (traceDataPresent) {
    std::string traceDataOptsInput = optionTraceData.StrValue();
    std::stringstream ssTraceDataOpts(traceDataOptsInput);
    std::string option;
    CEnumParser<TraceData> traceDataParser;
    while (std::getline(ssTraceDataOpts, option, ',')) {
      std::optional<TraceData> opt = traceDataParser.ParseEnum(option);
      if (opt) {
        cfg.common.shared.traceDataOpts.insert(opt.value());
      } else {
        Log(WARN) << "Incorect traceData option: \"" << option << "\".";
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
    }
  }

  if (cfg.common.shared.thresholdLogLevel == LogLevel::TRACEV) {
    for (auto& opt : TraceDataAll) {
      cfg.common.shared.traceDataOpts.insert(opt);
    }
  }

  set_when_option_present(cfg.opengl.player.traceGitsInternal, optionTraceGitsInternal);
  set_when_option_present(cfg.opengl.shared.traceGLError, optionTraceGLError);

  if (optionTraceVKStructs.Present()) {
    cfg.common.shared.traceDataOpts.insert(TraceData::VK_STRUCTS);
    Log(WARN) << "The '--traceVKStructs' argument is deprecated. "
              << "Please use '--traceData vkStructs' instead.";
  }

  if (optionCaptureVulkanRenderPasses.Present()) {
    cfg.vulkan.player.captureVulkanRenderPasses.SetFromString(
        optionCaptureVulkanRenderPasses.StrValue());
  }

  if (optionCaptureVulkanRenderPassesResources.Present()) {
    cfg.vulkan.player.captureVulkanRenderPassesResources.SetFromString(
        optionCaptureVulkanRenderPassesResources.StrValue());
  }

  if (optionCaptureVulkanDraws.Present()) {
    cfg.vulkan.player.captureVulkanDraws.SetFromString(optionCaptureVulkanDraws.StrValue());
  }

  if (optionCaptureVulkanResources.Present()) {
    cfg.vulkan.player.captureVulkanResources.SetFromString(optionCaptureVulkanResources.StrValue());
  }

  set_when_option_present(cfg.vulkan.player.oneVulkanDrawPerCommandBuffer,
                          optionOneVulkanDrawPerCommandBuffer);

  set_when_option_present(cfg.vulkan.player.oneVulkanRenderPassPerCommandBuffer,
                          optionOneVulkanRenderPassPerCommandBuffer);

  if (optionCaptureVulkanRenderPasses.Present() ||
      optionCaptureVulkanRenderPassesResources.Present() ||
      optionOneVulkanDrawPerCommandBuffer.Present() ||
      optionOneVulkanRenderPassPerCommandBuffer.Present() || optionCaptureVulkanDraws.Present() ||
      optionCaptureVulkanResources.Present()) {
    cfg.vulkan.player.execCmdBuffsBeforeQueueSubmit = true;
  }

  if (optionCaptureVulkanDraws.Present() || optionCaptureVulkanResources.Present()) {
    cfg.vulkan.player.oneVulkanDrawPerCommandBuffer = true;
  }

  if (optionCaptureVulkanRenderPasses.Present() ||
      optionCaptureVulkanRenderPassesResources.Present()) {
    cfg.vulkan.player.oneVulkanRenderPassPerCommandBuffer = true;
  }

  set_when_option_present(cfg.vulkan.player.traceVKShaderHashes, optionTraceVKShaderHashes);

  if ((optionUseVKPhysicalDeviceIndex.Present() && optionUseVKPhysicalDeviceName.Present()) ||
      (optionUseVKPhysicalDeviceIndex.Present() && optionUseVKPhysicalDeviceType.Present()) ||
      (optionUseVKPhysicalDeviceName.Present() && optionUseVKPhysicalDeviceType.Present())) {
    throw std::runtime_error(
        "Only one of the 'useVKPhysicalDeviceIndex', 'useVKPhysicalDeviceName' and "
        "'useVKPhysicalDevicType' options can be provided.");
  }
  set_when_option_present(cfg.vulkan.player.vulkanForcedPhysicalDeviceIndex,
                          optionUseVKPhysicalDeviceIndex);
  set_when_option_present(cfg.vulkan.player.vulkanForcedPhysicalDeviceName,
                          optionUseVKPhysicalDeviceName);
  set_when_option_present(cfg.vulkan.player.vulkanForcedPhysicalDeviceType,
                          optionUseVKPhysicalDeviceType);

  set_when_option_present(cfg.opengl.player.forceGLProfile, optionForceGLProfile);
  set_when_option_present(cfg.opengl.player.forceGLNativeAPI, optionForceGLNativeAPI);
  switch (cfg.opengl.player.forceGLNativeAPI) {
  case TForcedGLNativeApi::NO_NTV_API_FORCED:
    break;
  case TForcedGLNativeApi::WGL:
    gits::OpenGL::PtblNtvApi(gits::OpenGL::PtblNativeAPI::Type::WGL);
    break;
  case TForcedGLNativeApi::EGL:
    gits::OpenGL::PtblNtvApi(gits::OpenGL::PtblNativeAPI::Type::EGL);
    break;
  case TForcedGLNativeApi::GLX:
    gits::OpenGL::PtblNtvApi(gits::OpenGL::PtblNativeAPI::Type::GLX);
    break;
  default:
    Log(ERR) << "Invalid native API selected";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (optionForceGLVersionMajor.Present() || optionForceGLVersionMinor.Present()) {
    auto verMajor = optionForceGLVersionMajor.Value();
    auto verMinor = optionForceGLVersionMinor.Value();
    cfg.opengl.shared.forceGLVersion = std::to_string(verMajor) + "." + std::to_string(verMinor);
    cfg.opengl.shared.forceGLVersionMajor = verMajor;
    cfg.opengl.shared.forceGLVersionMinor = verMinor;
  }

  set_when_option_present(cfg.common.player.exitFrame, optionExitFrame);
  set_when_option_present(cfg.common.player.exitOnError, optionExitOnError);
  set_when_option_present(cfg.vulkan.player.exitOnVkQueueSubmitFail, optionExitOnVkQueueSubmitFail);

  if (optionCaptureDrawsPre.Present() && !optionCaptureDraws.Present()) {
    throw std::runtime_error(
        "Option 'captureDrawsPre' requires option 'captureDraws' to be specified");
  }

  set_when_option_present(cfg.common.player.logFncs, optionLogFncs);
  set_when_option_present(cfg.common.player.faithfulThreading, optionFaithfulThreading);
  set_when_option_present(cfg.common.player.logLoadedTokens, optionLogLoadedTokens);
  set_when_option_present(cfg.opengl.player.captureWholeWindow, optionCaptureWholeWindow);
  set_when_option_present(cfg.opengl.player.dontForceBackBufferGL, optionDontForceBackBufferGL);
  set_when_option_present(cfg.common.player.forceOrigScreenResolution,
                          optionForceOrigScreenResolution);
  set_when_option_present(cfg.common.player.showWindowBorder, optionShowWindowBorder);
  if (optionFullscreen.Present()) {
    cfg.common.player.windowMode = WindowMode::EXCLUSIVE_FULLSCREEN;
  }
  set_when_option_present(cfg.common.player.showWindowsWA, optionShowWindowsWA);
  set_when_option_present(cfg.common.player.signStream, optionSignStream);
  set_when_option_present(cfg.common.player.verifyStream, optionVerifyStream);
  set_when_option_present(cfg.common.player.dontVerifyStream, optionDontVerifyStream);
  set_when_option_present(cfg.common.player.renderOffscreen, optionRenderOffscreen);
  set_when_option_present(cfg.vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA,
                          optionIgnoreVKCrossPlatformIncompatibilitiesWA);
  if (optionCheckCrossPlatformCompatibility.Present()) {
    Log(WARN) << "The '--checkCrossPlatformCompatibility' argument is deprecated.";
  }
  set_when_option_present(cfg.opengl.player.scaleFactor, optionScaleFactor);
  set_when_option_present(cfg.vulkan.player.maxAllowedVkSwapchainRewinds,
                          optionMaxAllowedVkSwapchainRewinds);
  set_when_option_present(cfg.common.player.endFrameSleep, optionEndFrameSleep);
  set_when_option_present(cfg.common.player.stopAfterFrames, optionStopAfterFrames);
  set_when_option_present(cfg.common.player.captureFrames, optionCaptureFrames);
  set_when_option_present(cfg.common.player.traceSelectedFrames, optionTraceSelectedFrames);
  set_when_option_present(cfg.vulkan.player.captureVulkanSubmits, optionCaptureVulkanSubmits);
  set_when_option_present(cfg.vulkan.player.captureVulkanSubmitsResources,
                          optionCaptureVulkanSubmitsResources);
  set_when_option_present(cfg.vulkan.player.captureVulkanSubmitsGroupType,
                          optionCaptureVulkanSubmitsGroupType);
  set_when_option_present(cfg.vulkan.player.skipNonDeterministicImages,
                          optionSkipNonDeterministicImages);
  set_when_option_present(cfg.opengl.player.captureFramesHashes, optionCaptureFramesHashes);
  set_when_option_present(cfg.common.player.captureScreenshot, optionCaptureScreenshot);
  set_when_option_present(cfg.opengl.player.captureDraws, optionCaptureDraws);
  set_when_option_present(cfg.opengl.player.capture2DTexs, optionCapture2DTexs);
  set_when_option_present(cfg.opencl.player.captureImages, optionClCaptureImages);
  set_when_option_present(cfg.opencl.player.removeSourceLengths, optionClRemoveSourceLengths);
  set_when_option_present(cfg.opengl.player.captureDraws2DTexs, optionCaptureDraws2DTexs);
  set_when_option_present(cfg.opencl.player.aubSignaturesCL, optionAubSignaturesCL);

#if defined(GITS_PLATFORM_WINDOWS)
  if (optionRenderDocCaptureFrames.Present()) {
    if (optionRenderDocCaptureVKSubmits.Present()) {
      throw std::runtime_error("Option 'renderDocCaptureFrames' cannot be combined with option "
                               "'renderDocCaptureVKSubmits'");
    }
    cfg.vulkan.player.renderDoc.mode.setFromString("Frames");
    cfg.vulkan.player.renderDoc.captureRange = optionRenderDocCaptureFrames.Value();
  }

  if (optionRenderDocCaptureVKSubmits.Present()) {
    if (optionRenderDocCaptureFrames.Present()) {
      throw std::runtime_error("Option 'renderDocCaptureVKSubmits' cannot be combined with option "
                               "'renderDocCaptureFrames'");
    }
    if (optionCaptureVulkanSubmits.Present() || optionCaptureVulkanSubmitsResources.Present()) {
      throw std::runtime_error("Option 'renderDocCaptureVKSubmits' cannot be combined with option "
                               "'captureVulkanSubmits' or 'captureVulkanSubmitsResources'");
    }
    cfg.vulkan.player.renderDoc.mode.setFromString("QueueSubmit");
    cfg.vulkan.player.renderDoc.captureRange = optionRenderDocCaptureVKSubmits.Value();
  }

  if (optionRenderDocDllPath.Present()) {
    std::filesystem::path renderDocDllPath = optionRenderDocDllPath.Value();
    cfg.vulkan.player.renderDoc.dllPath = std::filesystem::absolute(renderDocDllPath);
  }

  if (optionRenderDocContinuousCapture.Present()) {
    if (!optionRenderDocCaptureFrames.Present() && !optionRenderDocCaptureVKSubmits.Present()) {
      throw std::runtime_error(
          "Option 'renderDocContinuousCapture' requires option 'renderDocCaptureFrames' or "
          "'renderDocCaptureVKSubmits' to be specified");
    }
    cfg.vulkan.player.renderDoc.continuousCapture = optionRenderDocContinuousCapture.Value();
  }

  if (optionRenderDocEnableUI.Present()) {
    if (!optionRenderDocCaptureFrames.Present() && !optionRenderDocCaptureVKSubmits.Present()) {
      throw std::runtime_error(
          "Option 'renderDocEnableUI' requires option 'renderDocCaptureFrames' or "
          "'renderDocCaptureVKSubmits' to be specified");
    }
    cfg.vulkan.player.renderDoc.enableUI = optionRenderDocEnableUI.Value();
  }
#endif

  set_when_option_present(cfg.common.shared.libEGL, optionLibEGL);
  set_when_option_present(cfg.common.shared.libGLESv1, optionLibGLESv1);
  set_when_option_present(cfg.common.shared.libGLESv1, optionLibGLESv2);
  set_when_option_present(cfg.common.shared.libGL, optionLibGL);
  if (optionLibVK.Present()) {
    std::filesystem::path libVKPath = optionLibVK.Value();
    if (std::filesystem::is_directory(libVKPath)) {
      libVKPath /= cfg.common.shared.libVK;
    }
    cfg.common.shared.libVK = std::move(libVKPath);
  }

  set_when_option_present(cfg.opengl.player.captureFinishFrame, optionCaptureFinishFrame);
  set_when_option_present(cfg.opengl.player.captureReadPixels, optionCaptureReadPixels);
  set_when_option_present(cfg.opengl.player.captureFlushFrame, optionCaptureFlushFrame);
  set_when_option_present(cfg.opengl.player.captureBindFboFrame, optionCaptureBindFboFrame);
  set_when_option_present(cfg.opengl.player.keepDraws, optionKeepDraws);
  set_when_option_present(cfg.opengl.player.keepFrames, optionKeepFrames);
  set_when_option_present(cfg.opengl.player.minimalConfig, optionMinimalConfig);

  if (optionTokenBurstLimit.Present()) {
    cfg.common.player.tokenBurst = optionTokenBurstLimit.Value();
    if (cfg.common.player.tokenBurst == 0) {
      throw std::runtime_error("tokenBurstLimit must be greater then 0");
    }
  }
  if (optionTokenBurstNum.Present()) {
    cfg.common.player.tokenBurstNum = optionTokenBurstNum.Value();
    if (cfg.common.player.tokenBurstNum == 0) {
      throw std::runtime_error("tokenBurstNum must be greater then 0");
    }
  }
  if (optionLoadWholeStreamBeforePlayback.Present()) {
    if (optionTokenBurstLimit.Present()) {
      throw std::runtime_error("loadWholeStreamBeforePlayback option is mutually exclusive with "
                               "tokenBurstLimit option.");
    }
    cfg.common.player.loadWholeStreamBeforePlayback = optionLoadWholeStreamBeforePlayback.Value();
  }

  set_when_option_present(cfg.opencl.player.captureReads, optionClCaptureReads);
  set_when_option_present(cfg.opencl.player.captureKernels, optionClCaptureKernels);
  set_when_option_present(cfg.opencl.player.omitReadOnlyObjects, optionClOmitReadOnlyObjects);
  set_when_option_present(cfg.opencl.player.dumpLayoutOnly, optionClDumpLayoutOnly);
  set_when_option_present(cfg.opencl.player.injectBufferResetAfterCreate,
                          optionClInjectBufferResetAfterCreate);
  set_when_option_present(cfg.levelzero.player.dumpLayoutOnly, optionL0DumpLayoutOnly);
  if (optionL0CaptureKernels.Present()) {
    auto captureKernelsVec = cfg.levelzero.ParseCaptureKernels(optionL0CaptureKernels.Value());
    if (captureKernelsVec.size() == 3) {
      cfg.levelzero.player.captureKernels = BitRange(captureKernelsVec[2]);
      cfg.levelzero.player.captureCommandLists = BitRange(captureKernelsVec[1]);
      cfg.levelzero.player.captureCommandQueues = BitRange(captureKernelsVec[0]);
    } else if (captureKernelsVec.size() == 2) {
      cfg.levelzero.player.captureKernels = BitRange(true);
      cfg.levelzero.player.captureCommandLists = BitRange(captureKernelsVec[1]);
      cfg.levelzero.player.captureCommandQueues = BitRange(captureKernelsVec[0]);
    } else {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  set_when_option_present(cfg.levelzero.player.captureAfterSubmit, optionL0CaptureAfterSubmit);
  set_when_option_present(cfg.levelzero.player.captureImages, optionL0CaptureImages);
  set_when_option_present(cfg.levelzero.player.dumpSpv, optionL0DumpSpv);
  set_when_option_present(cfg.levelzero.player.captureInputKernels, optionL0CaptureInputKernels);
  set_when_option_present(cfg.levelzero.player.injectBufferResetAfterCreate,
                          optionL0InjectBufferResetAfterCreate);
  set_when_option_present(cfg.levelzero.player.disableNullIndirectPointersInBuffer,
                          optionL0DisableNullIndirectPointersInBuffer);
  set_when_option_present(cfg.levelzero.player.disableAddressTranslation,
                          optionL0DisableAddressTranslation);
  set_when_option_present(cfg.levelzero.player.omitOriginalAddressCheck,
                          optionL0OmitOriginalAddressCheck);
  if (cfg.levelzero.player.captureInputKernels && cfg.levelzero.player.captureAfterSubmit) {
    Log(ERR) << "l0CaptureInputKernels and l0CaptureAfterSubmit options are mutually exclusive.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  if (optionForceWindowPos.Present()) {
    if (optionFullscreen.Present()) {
      throw std::runtime_error("Fullscreen windows must start at 0x0, you can't change their "
                               "position with --forceWindowPos.");
    }

    cfg.common.player.forceWindowPos.enabled = true;
    std::pair<uint32_t, uint32_t> windowCoords;
    if (!parse_pair(optionForceWindowPos.Value(), windowCoords)) {
      throw std::runtime_error("Couldn't parse window position parameter.");
    }
    cfg.common.player.forceWindowPos.x = windowCoords.first;
    cfg.common.player.forceWindowPos.y = windowCoords.second;
  }
  if (optionForceWindowSize.Present()) {
    cfg.common.player.forceWindowSize.enabled = true;
    std::pair<uint32_t, uint32_t> windowSize;
    if (!parse_pair(optionForceWindowSize.Value(), windowSize)) {
      throw std::runtime_error("Couldn't parse window size parameter.");
    }
    cfg.common.player.forceWindowSize.width = windowSize.first;
    cfg.common.player.forceWindowSize.height = windowSize.second;
  }
  if (optionForceInvisibleWindows.Present()) {
    cfg.common.player.forceInvisibleWindows = true;
  }

  set_when_option_present(cfg.common.shared.libClPath, optionLibOCLPath);

  if (optionForceScissor.Present()) {
    cfg.common.player.forceScissor.enabled = true;
    std::vector<uint32_t> scissorCoords;
    if (!parse_vector(optionForceScissor.Value(), scissorCoords, 4)) {
      throw std::runtime_error("Couldn't parse scissor coords");
    }
    cfg.common.player.forceScissor.x = scissorCoords[0];
    cfg.common.player.forceScissor.y = scissorCoords[1];
    cfg.common.player.forceScissor.width = scissorCoords[2];
    cfg.common.player.forceScissor.height = scissorCoords[3];
  }
  if (optionAffectViewport.Present()) {
    cfg.opengl.player.affectViewport = true;
    if (!parse_vector(optionAffectViewport.Value(), cfg.opengl.player.affectedViewport, 2)) {
      throw std::runtime_error("Couldn't parse scissor coords");
    }
  }
#if defined GITS_PLATFORM_WINDOWS
  if (optionForceDesktopResolution.Present()) {
    cfg.common.player.forceDesktopResolution.enabled = true;
    std::pair<vi_uint, vi_uint> forcedDesktopResolution;
    if (!parse_pair(optionForceDesktopResolution.Value(), forcedDesktopResolution)) {
      throw std::runtime_error("Couldn't parse resolution");
    }
    cfg.common.player.forceDesktopResolution.width = forcedDesktopResolution.first;
    cfg.common.player.forceDesktopResolution.height = forcedDesktopResolution.second;
  }
#endif

  // There is exactly one unrecognized non-option string in cmdline. Its stream path.
  if (!options.NotConsumed().empty()) {
    std::filesystem::path stream_path = std::filesystem::absolute(options.NotConsumed()[0]);

    if (!std::filesystem::exists(stream_path)) {
      throw std::runtime_error("Specified stream path: '" + stream_path.string() +
                               "' does not exist");
    }

    if (std::filesystem::is_directory(stream_path)) {
      Log(INFO) << "Specified stream path: '" << stream_path
                << "' is a directory, attempting to find a stream file.";
      std::vector<std::filesystem::path> matching_paths;
      for (const auto& dirEntry : std::filesystem::directory_iterator(stream_path)) {
        if (std::filesystem::is_directory(dirEntry)) {
          continue;
        }
        auto entryPathStr = dirEntry.path().string();
        if (ends_with(entryPathStr, ".gits2") || ends_with(entryPathStr, ".gits2.gz")) {
          matching_paths.push_back(dirEntry.path());
        }
      }

      if (matching_paths.empty()) {
        throw std::runtime_error("Specified directory does not contain a stream file.");
      }
      if (matching_paths.size() > 1) {
        throw std::runtime_error(
            "Too many stream files in specified directory. Specify stream file explicitly.");
      }
      stream_path = matching_paths[0];
      Log(INFO) << "Using stream file '" << stream_path << "'\n";
    }

    cfg.common.player.streamPath = stream_path;
    cfg.common.player.streamDir = stream_path.parent_path();
  }
  Config::Set(cfg);

  if (argc > 1 && cfg.common.player.helpGroup == "") {
    Log(INFO)
        << "Options defined in the command line; Please note that any options set via the command "
           "line will take precedence over those specified in the configuration file.";
  }

  if (cfg.common.player.helpGroup != "") {
    // show usage screen
    Log(INFO, NO_PREFIX) << "Usage: ";
    Log(INFO, NO_PREFIX) << "  " << options.AppName() << " [options] stream.gits2"
                         << "\n";

    options.Usage(cfg.common.player.helpGroup, console_columns());

    Log(INFO, NO_PREFIX) << "Keys:";
    Log(INFO, NO_PREFIX) << " - <ESC>    - Exit gitsPlayer";
    Log(INFO, NO_PREFIX) << " - <SPACE>  - STOP/RESUME playback\n";

    Log(INFO, NO_PREFIX) << "Please use -hh option for listing help on specific group of options.";
    return true;
  }

  if (cfg.common.player.version) {
    // Print version and quit.
    CGits& inst = CGits::Instance();
    Log(INFO, NO_PREFIX) << inst << "\n";
    return true;
  }

  // check if file to play specified
  if (options.NotConsumed().empty() && cfg.common.player.streamPath.empty()) {
    throw std::runtime_error("Error: GITS file not specified");
  } else if (options.NotConsumed().size() > 1) {
    std::stringstream str;
    str << "Error: Too many non-option command line parameters:\n    ";
    std::copy(options.NotConsumed().begin(), options.NotConsumed().end(),
              std::ostream_iterator<std::string>(str, "\n    "));
    throw std::runtime_error(str.str());
  }

  return false;
}

template <>
CEnumParser<TraceData>::CEnumParser() {
  if (_map.empty()) {
    _map["vkstructs"] = TraceData::VK_STRUCTS;
    _map["framenumber"] = TraceData::FRAME_NUMBER;
  }
}
} // namespace gits
