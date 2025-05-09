// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <sstream>
#include <string>

#include "enumsAuto.h"
#include "stringToType.h"
#include <args.hxx>

#include "argumentsAuto.h"

namespace gits {
enum ArgumentParsingResult {
  ParsingSyntaxError,
  ParsingSemanticError,
  ParsingSuccess,
  ShowHelp
};

struct ArgumentParser {
  args::ArgumentParser Parser;
  ArgumentParsingResult ParsingResult;
  args::Positional<std::filesystem::path> StreamPath;
  args::HelpFlag Help;
  args::ValueFlag<std::string> HelpMenu;
  args::Flag Version;
  args::Flag SkipEnvironment;
  args::ValueFlag<std::filesystem::path> ConfigFile;
  ArgConfiguration ArgumentConfig;
  std::stringstream Output;
  std::string ArgumentString;

  ArgumentParser(const std::vector<std::string>& arguments)
      : Parser("Program description"),
        ParsingResult(ArgumentParsingResult::ParsingSuccess),
        StreamPath(Parser, "path to stream", "Path to a folder or file"),
        Help(Parser, "help", "Display this help menu", {'h', "help"}),
        HelpMenu(
            Parser, "filter help", "Filter help by sections/platforms/API/operation/...", {"hh"}),
        Version(Parser, "version", "Print version and quit", {'v', "version"}),
        SkipEnvironment(
            Parser, "skip environment", "Don't use environment variables", {"skip-environment"}),
        ConfigFile(Parser,
                   "config file",
                   "Use this configfile (and all default values)",
                   {"config", "configFile", "configfile", "Configfile", "ConfigFile"}),
        ArgumentConfig(Parser) {
    Parser.Prog("gitsPlayer");
    try {
      Parser.ParseCLI(arguments);
    } catch (const args::Help&) {
      Output << Parser;
      ParsingResult = ArgumentParsingResult::ShowHelp;
    } catch (const args::ParseError& e) {
      Output << e.what() << std::endl;
      ParsingResult = ArgumentParsingResult::ParsingSyntaxError;
    } catch (const args::Error& e) {
      Output << e.what() << std::endl;
      ParsingResult = ArgumentParsingResult::ParsingSyntaxError;
    } catch (const std::exception& e) {
      Output << e.what() << std::endl;
      ParsingResult = ArgumentParsingResult::ParsingSyntaxError;
    }

    if (ParsingResult == ArgumentParsingResult::ParsingSuccess) {
      if (!Validate()) {
        ParsingResult = ArgumentParsingResult::ParsingSemanticError;
      }
    }
  }

  ArgumentParser(int argc, char* argv[])
      : ArgumentParser(std::vector<std::string>(argv, argv + argc)) {
    //nothing
  }

  bool Validate() {
    return ArgumentConfig.Validate();
  }
};
} // namespace gits
