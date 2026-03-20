// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "arguments.h"
#include <args.hxx>
#include <iostream>
#include <vector>
#include <filesystem>

bool ParseArguments(int argc, char* argv[], Arguments& args) {
  args::ArgumentParser parser("CCode replay");
  args::HelpFlag help(parser, "help", "Display this help", {'h', "help"});
  args::Flag screenshots(parser, "screenshots", "Enable screenshot capture", {'s', "screenshots"});
  args::ValueFlag<std::string> outputDir(parser, "output-dir", "Directory for screenshot output",
                                         {'o', "output-dir"}, "out");

  std::vector<std::string> argvVec(argv + 1, argv + argc);
  try {
    parser.ParseArgs(argvVec);
    args.EnableScreenshots = screenshots;
    args.OutputDir = std::filesystem::absolute(args::get(outputDir));
    return true;
  } catch (const args::Help&) {
    std::cout << parser;
    return false;
  } catch (const args::Error& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return false;
  }
}
